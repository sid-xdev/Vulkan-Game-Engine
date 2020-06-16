#pragma once

#include <Defines.hpp>
#include <vulkan/vulkan.hpp>

namespace noxcain
{
	class PresentationSurface;
	
	class GraphicCore
	{
	private:

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

		void pickPhysicalDevice();
		void createDevice();
		void createSwapChain( const vk::SwapchainKHR& oldSwapChain = vk::SwapchainKHR() );

	public:

		GraphicCore( const GraphicCore& ) = delete;
		GraphicCore& operator=( const GraphicCore& ) = delete;
		GraphicCore& operator=( GraphicCore&& ) = delete;

		GraphicCore();
		bool initialize( const PresentationSurface& os_surface );
		~GraphicCore();

		bool switchToNextPhysicalDevice();

		UINT32 get_graphic_queue_family_index() const;
		UINT32 get_swapchain_image_count() const;

		vk::Extent2D get_window_extent() const;
		vk::Format getPresentationSurfaceFormat() const;

		vk::PhysicalDevice getPhysicalDevice() const;
		vk::Device getLogicalDevice() const;
		vk::SurfaceKHR getSurface() const;
		vk::SwapchainKHR getSwapChain() const;
		vk::ImageView getImageView( UINT32 index ) const;

		void recreateSwapChain();
	};
}
