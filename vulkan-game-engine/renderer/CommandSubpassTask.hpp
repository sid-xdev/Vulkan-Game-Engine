#pragma once
#include <Defines.hpp>

#include <renderer/CommandThreadTools.hpp>
#include <renderer/GraphicEngineConstants.hpp>
#include <renderer/GameGraphicEngine.hpp>

#include <tools/ResultHandler.hpp>

#include <array>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace noxcain
{
	class LogicTask;
	
	template<typename T>
	class SubpassTask
	{
	public:
		
		void set_frame_buffers( const std::vector<vk::Framebuffer>& frame_buffers );
		void set_render_pass( vk::RenderPass render_pass, UINT32 subpass_index );
		void set_buffer_id( std::size_t buffer_id );

		bool wait_for_finish();

		const std::vector<vk::CommandBuffer>& get_finished_buffers();

	protected:
		
		std::vector<vk::Framebuffer> frame_buffers;
		vk::RenderPass render_pass;
		UINT32 subpass_index = 0;

		// stops loop
		bool shutdown = false;
		
		// when new frame starts
		bool is_new_render_pass = true; // render pass was updatet
		bool start_signaled = false;
		bool wait_for_start();

		// when buffer id is free
		std::size_t buffer_id = 0;
		bool buffer_signaled = false;
		bool wait_for_buffer();

		// when buffer id is free
		bool recording_signaled = false;
		bool wait_for_recording();

		// when recording complete
		bool single_use = true;
		bool finished = false;

		SubpassTask();
		
		// thread components
		mutable std::mutex task_mutex;
		mutable std::condition_variable task_condition;
		std::thread task_thread;
		void execute_task();

		// buffer preperations
		struct CommandData
		{
			vk::CommandPool pool;
			std::vector<vk::CommandBuffer> buffers;
		};
		std::vector<CommandData> command_data;
		bool buffer_preparation( CommandData& pool_data, std::size_t buffer_count );
		void shutdown_task();
	};

	template<typename T>
	inline bool SubpassTask<T>::wait_for_start()
	{
		std::unique_lock<std::mutex> lock( task_mutex );
		task_condition.wait( lock, [this]() { return start_signaled || shutdown; } );
		if( shutdown )
		{
			return false;
		}
		else
		{
			start_signaled = false;
			return true;
		}
	}

	template<typename T>
	inline bool SubpassTask<T>::wait_for_buffer()
	{
		std::unique_lock<std::mutex> lock( task_mutex );
		task_condition.wait( lock, [this]() { return buffer_signaled || shutdown; } );
		if( shutdown )
		{
			return false;
		}
		else
		{
			buffer_signaled = false;
			return true;
		}
	}

	template<typename T>
	inline bool SubpassTask<T>::wait_for_recording()
	{
		std::unique_lock<std::mutex> lock( task_mutex );
		task_condition.wait( lock, [this]() { return recording_signaled || shutdown; } );
		if( shutdown )
		{
			return false;
		}
		else
		{
			recording_signaled = false;
			return true;
		}
	}

	template<typename T>
	SubpassTask<T>::SubpassTask() :
		task_thread( &SubpassTask<T>::execute_task, this ),
		command_data( RECORD_RING_SIZE )
	{
	}

	template<typename T>
	void SubpassTask<T>::execute_task()
	{
		Watcher watcher( [this]()
		{
			std::unique_lock lock( task_mutex );
			shutdown = true;
			task_condition.notify_all();
		} );
		ResultHandler<bool> result_handler( true );

		while( !shutdown )
		{
			// wait for master signal to start
			result_handler << wait_for_start();
			if( result_handler.is_critical() ) return;

			// make any neccessary preparations possible without knowing witch pool to use
			result_handler << static_cast<T*>( this )->buffer_independent_preparation();
			if( result_handler.is_critical() ) return;

			// wait for master to select the next available pool
			result_handler << wait_for_buffer();
			if( result_handler.is_critical() ) return;

			// make any neccessary preparations now wich need a pool id
			result_handler << static_cast<T*>( this )->buffer_dependent_preparation( command_data[buffer_id] );
			if( result_handler.is_critical() ) return;

			// wait for master to give the signal that the framebuffer is finsihed
			result_handler << wait_for_recording();
			if( result_handler.is_critical() ) return;

			// record the commands in the buffers of the selected pool
			result_handler << static_cast<T*>( this )->record( command_data[buffer_id].buffers );
			if( result_handler.is_critical() ) return;

			// set the subtask state on finished and notify master
			std::unique_lock<std::mutex> lock( task_mutex );
			finished = true;
			task_condition.notify_all();
		}
	}

	template<typename T>
	inline bool SubpassTask<T>::buffer_preparation( CommandData& pool_data, std::size_t buffer_count )
	{
		ResultHandler<vk::Result> r_handler( vk::Result::eSuccess );

		vk::Device device = GraphicEngine::get_device();
		if( !pool_data.pool )
		{
			pool_data.pool = r_handler << device.createCommandPool( vk::CommandPoolCreateInfo( vk::CommandPoolCreateFlagBits::eTransient, GraphicEngine::get_graphic_queue_family_index() ) );
		}
		else
		{
			r_handler << device.resetCommandPool( pool_data.pool, vk::CommandPoolResetFlags() );
		}

		if( r_handler.all_okay() )
		{

			if( pool_data.buffers.size() && pool_data.buffers.size() != buffer_count )
			{
				device.freeCommandBuffers( pool_data.pool, pool_data.buffers );
				pool_data.buffers.clear();
			}

			if( pool_data.buffers.empty() )
			{
				pool_data.buffers = r_handler << device.allocateCommandBuffers( vk::CommandBufferAllocateInfo( pool_data.pool, vk::CommandBufferLevel::eSecondary, buffer_count ) );
			}
		}
		return r_handler.all_okay();
	}

	template<typename T>
	inline void SubpassTask<T>::shutdown_task()
	{
		{
			std::unique_lock<std::mutex> lock( task_mutex );
			shutdown = true;
			task_condition.notify_all();
		}

		if( task_thread.joinable() )
		{
			task_thread.join();
		}

		vk::Device device = GraphicEngine::get_device();
		if( device )
		{
			ResultHandler r_handler( vk::Result::eSuccess );
			r_handler << device.waitIdle();
			if( r_handler.all_okay() )
			{
				for( auto single_command_data : command_data )
				{
					device.destroyCommandPool( single_command_data.pool );
				}
				command_data.clear();
			}
		}
	}

	template<typename T>
	inline void SubpassTask<T>::set_frame_buffers( const std::vector<vk::Framebuffer>& new_frame_buffers )
	{
		std::unique_lock<std::mutex> lock( task_mutex );
		frame_buffers = new_frame_buffers;
		recording_signaled = true;
		task_condition.notify_all();
	}

	template<typename T>
	inline void SubpassTask<T>::set_render_pass( vk::RenderPass new_render_pass, UINT32 new_subpass_index )
	{
		std::unique_lock<std::mutex> lock( task_mutex );
		if( new_render_pass != render_pass || new_subpass_index != subpass_index )
		{
			render_pass = new_render_pass;
			subpass_index = new_subpass_index;
			is_new_render_pass = true;
		}
		start_signaled = true;
		task_condition.notify_all();
	}

	template<typename T>
	inline void SubpassTask<T>::set_buffer_id( std::size_t new_buffer_id )
	{
		std::unique_lock<std::mutex> lock( task_mutex );
		buffer_id = new_buffer_id;
		buffer_signaled = true;
		task_condition.notify_all();
	}

	template<typename T>
	bool SubpassTask<T>::wait_for_finish()
	{
		std::unique_lock<std::mutex> lock( task_mutex );
		task_condition.wait( lock, [this]() { return finished || shutdown; } );
		if( shutdown )
		{
			return false;
		}
		else
		{
			if( single_use )
			{
				finished = false;
			}
			return true;
		}
	}
	template<typename T>
	inline const std::vector<vk::CommandBuffer>& SubpassTask<T>::get_finished_buffers()
	{
		std::unique_lock<std::mutex> lock( task_mutex );
		if( single_use ) finished = false;
		return command_data[buffer_id].buffers;
	}
}
