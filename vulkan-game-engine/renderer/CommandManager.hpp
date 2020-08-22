#pragma once
#include <Defines.hpp>

#include <renderer/GraphicEngineConstants.hpp>
#include <renderer/RenderPassDescription.hpp>

#include <vulkan/vulkan.hpp>
#include <array>
#include <thread>
#include <mutex>

namespace noxcain
{	
	class LoopState;
	class RenderPassDescription;
	
	class OverlayTask;
	class CommandSubmit;

	class CommandManager
	{
	public:
		~CommandManager();
		CommandManager( const CommandManager& ) = delete;
		CommandManager& operator=( const CommandManager& ) = delete;

		CommandManager();

		void start_loop();
		
	private:
		void run_render_loop();
		std::thread command_main_thread;

		void describe_deferred_render_pass();
		void describe_finalize_render_pass();
		
		RenderPassDescription::FormatHandle default_color_format;
		RenderPassDescription::FormatHandle stencil_format;
		RenderPassDescription::FormatHandle depth_format;
		RenderPassDescription::SampleCountHandle multi_sample_count;
		
		RenderPassDescription::FormatHandle swap_chain_image_format;

		RenderPassDescription deferred_render_pass;
		RenderPassDescription finalize_render_pass;

		std::array<vk::CommandPool, RECORD_RING_SIZE> command_pools;
		std::array<std::vector<vk::CommandBuffer>, RECORD_RING_SIZE> command_buffers;
		
		/// <summary>
		/// checks graphic settings and recreates renderpasses and buffers if necessary
		/// </summary>
		inline bool update_logic( CommandSubmit& submit_controller );
		inline bool check_settings( CommandSubmit& submit_controller );
		inline bool validate_render_passes();
		inline bool validate_frame_buffers();
		inline bool validate_command_buffers( std::size_t buffer_id );
		
		vk::Framebuffer deferred_frame_buffer;
		std::vector<vk::ClearValue> deferred_clear_colors;

		UINT32 old_frame_buffer_width = 0;
		UINT32 old_frame_buffer_height = 0;
		UINT32 old_sample_count = 0;

		std::vector<vk::Framebuffer> finalize_frame_buffers;
		std::vector<vk::ClearValue> finalize_clear_colors;
		vk::SwapchainKHR old_swapchain;

		vk::ClearColorValue clear_color;

		/// <summary>
		/// data transfer to gpu befor render loop
		/// </summary>
		bool initial_transfer();
	};
}