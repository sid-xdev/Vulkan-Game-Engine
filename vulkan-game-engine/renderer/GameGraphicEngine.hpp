#pragma once
#include <Defines.hpp>
#include <shader/shader.hpp>

#include <memory>
#include <vulkan/vulkan.hpp>

namespace noxcain
{
	class GraphicCore;
	class PresentationSurface;
	class DescriptorSetManager;
	class ShaderManager;
	class MemoryManager;
	class CommandManager;

	class  GraphicEngine
	{
	private:
		std::unique_ptr<GraphicCore> core;
		std::unique_ptr<DescriptorSetManager> descriptor_sets;
		std::unique_ptr<MemoryManager> memory;
		std::unique_ptr<ShaderManager> shader;
		std::unique_ptr<CommandManager> commands;
		
		bool clear();
		void initialize();
		
		//bool checkGraphicProperties();

		//void recreateRenderLoop( bool keepSwapChain );

		GraphicEngine( const GraphicEngine& ) = delete;
		GraphicEngine& operator=( const GraphicEngine& ) = delete;
		GraphicEngine& operator=( GraphicEngine&& ) = delete;

		static std::unique_ptr<GraphicEngine> engine;
		GraphicEngine();
	public:

		static bool run( const PresentationSurface& os_surface );

		static vk::Device get_device();
		static vk::PhysicalDevice get_physical_device();
		static vk::SwapchainKHR get_swapchain();
		static UINT32 get_swapchain_image_count();
		static vk::Format get_swapchain_image_format();
		static vk::ImageView get_swapchain_image_view( std::size_t image_index );
		
		static MemoryManager& get_memory_manager();

		static DescriptorSetManager& get_descriptor_set_manager();

		static vk::Extent2D get_window_resolution();

		static UINT32 get_graphic_queue_family_index();

		static vk::ShaderModule get_shader( FragmentShaderIds shader_id );
		static vk::ShaderModule get_shader( ComputeShaderIds shader_id );
		static vk::ShaderModule get_shader( VertexShaderIds shader_id );

		~GraphicEngine();
	};
}
