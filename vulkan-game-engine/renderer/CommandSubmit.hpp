#pragma once

#include <array>
#include <mutex>
#include <thread>
#include <vector>
#include <vulkan/vulkan.hpp>

#include <renderer/CommandThreadTools.hpp>

#include <tools/TimeFrame.hpp>

namespace noxcain
{
	class CommandSubmit
	{
	public:
		struct SubmitCommandBufferData
		{
			UINT32 id = 0;
			vk::CommandBuffer main_buffer;
			std::vector<vk::CommandBuffer> finalize_command_buffers;
		};

		CommandSubmit();
		~CommandSubmit();

		INT32 get_free_buffer_id();
		bool set_newest_command_buffer( SubmitCommandBufferData buffer_data );
		void clean_command_buffer();
		bool check_swapchain();

	private:
		TimeFrameCollector time_collection_all = TimeFrameCollector( "GPU Overall" );
		TimeFrameCollector time_collection_gpu = TimeFrameCollector( "GPU Sub Tasks" );

		enum class Status
		{
			SUBMIT,
			RECREAT_SWAPCHAIN,
			EXIT
		} status = Status::SUBMIT;

		bool running() const;

		bool lost_surface = false;
		void signal_swapchain_recreation( bool recreate_surface );

		UINT32 submit_loop();
		mutable std::mutex submit_mutex;
		std::thread submit_thread;
		std::condition_variable submit_condition;

		bool buffer_available = false;
		SubmitCommandBufferData available_buffer;
		std::vector<UINT32> free_buffer_ids;
		
		enum class SemaphoreIds : std::size_t
		{
			ACQUIRE = 0,
			RENDER,
			SUPER_SAMPLING
		};
		constexpr static std::size_t SEMOPHORE_COUNT = 3;

		void create_semaphores();
		std::array<vk::Semaphore, SEMOPHORE_COUNT> semaphores = {};
		vk::Fence frame_end_fence;

		const vk::Semaphore& get_semaphore( SemaphoreIds id )
		{
			return semaphores[static_cast<std::size_t>( id )];
		}
	};
}