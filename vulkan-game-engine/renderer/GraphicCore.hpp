#pragma once

#include <Defines.hpp>
#include <vulkan/vulkan.hpp>

namespace noxcain
{
	class PresentationSurface;
	
	class GraphicCore
	{
	private:

		std::shared_ptr<PresentationSurface> surface_base = nullptr;

		vk::Extent2D surfaceExtent;
		vk::Format surfaceFormat = vk::Format::eUndefined;
		
		UINT32 presentationImageCount = 3;
		vk::PresentModeKHR presentationMode = vk::PresentModeKHR::eFifoRelaxed;

		std::vector<const char*> necessary_instance_extensions = { VK_KHR_SURFACE_EXTENSION_NAME };
		const std::array<const char*, 1> necessaryDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		vk::Instance instance;
		vk::Device logical_device;

		vk::SurfaceKHR surface;
		vk::SwapchainKHR swapChain;

		UINT32 deviceIndex = 0;
		struct PhysicalDeviceCandidate
		{
			vk::PhysicalDevice device;
			vk::PhysicalDeviceProperties properties;
			UINT32 queueFamilyIndex = 0;
			UINT32 queueCount = 0;
		};
		std::vector<PhysicalDeviceCandidate> candidates;

		std::vector<vk::ImageView> swapChainImageViews;

		bool pick_physical_device();
		bool create_device();
		bool create_swapchain( const vk::SwapchainKHR& oldSwapChain = vk::SwapchainKHR() );

	public:

		GraphicCore( const GraphicCore& ) = delete;
		GraphicCore& operator=( const GraphicCore& ) = delete;
		GraphicCore& operator=( GraphicCore&& ) = delete;

		GraphicCore();
		bool initialize( std::shared_ptr<PresentationSurface> os_surface );
		~GraphicCore();

		bool use_next_physical_device();

		UINT32 get_graphic_queue_family_index() const;
		UINT32 get_swapchain_image_count() const;

		vk::Extent2D get_window_extent() const;
		vk::Format get_presentation_surface_format() const;

		vk::PhysicalDevice get_physical_device() const;
		vk::Device get_logical_device() const;
		vk::SurfaceKHR get_surface() const;
		vk::SwapchainKHR get_swapchain() const;
		vk::ImageView get_image_view( UINT32 index ) const;

		void close_surface_base() const;
		bool recreate_swapchain( bool recreate_surface );
	};
}
