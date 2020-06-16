#include "GraphicCore.hpp"

#include <Defines.hpp>
#include <Version.hpp>
#include <PresentationSurface.hpp>

#include <tools/ResultHandler.hpp>

#include <array>

void noxcain::GraphicCore::pickPhysicalDevice()
{
	ResultHandler resultHandler( vk::Result::eSuccess );
	std::vector<vk::PhysicalDevice> devices = resultHandler << instance.enumeratePhysicalDevices();

	for( const vk::PhysicalDevice& device : devices )
	{
		std::vector<vk::ExtensionProperties> deviceExtensions = resultHandler << device.enumerateDeviceExtensionProperties();
		bool isPossibleDevice = true;
		for( const char* necessaryDeviceExtension : necessaryDeviceExtensions )
		{
			bool isExtensionAvailable = false;
			for( const vk::ExtensionProperties deviceExtension : deviceExtensions )
			{
				if( !strcmp( necessaryDeviceExtension, deviceExtension.extensionName ) )
				{
					isExtensionAvailable = true;
					break;
				}
			}

			if( !isExtensionAvailable )
			{
				isPossibleDevice = false;
				break;
			}
		}

		if( isPossibleDevice )
		{
			const std::vector<vk::QueueFamilyProperties>& queueFamilyProperties = device.getQueueFamilyProperties();

			for( UINT32 familyIndex = 0; familyIndex < queueFamilyProperties.size(); ++familyIndex )
			{
				if( queueFamilyProperties[familyIndex].queueFlags & vk::QueueFlagBits::eGraphics &&
					VK_TRUE == resultHandler << device.getSurfaceSupportKHR( familyIndex, surface ) )
				{
					PhysicalDeviceCandidate candidate;
					candidate.device = device;
					candidate.properties = device.getProperties();
					candidate.queueFamilyIndex = familyIndex;
					candidate.queueCount = queueFamilyProperties[familyIndex].queueCount;
					candidates.push_back( candidate );
					break;
				}
			}
		}
	}

	/*
	if( candidates.empty() )
	{
		throw std::exception( "No fitting physical graphic device available." );
	}
	*/

	std::sort( candidates.begin(), candidates.end(), []( const PhysicalDeviceCandidate& firstCandidate, const PhysicalDeviceCandidate& secondeCandidate )
	{
		if( firstCandidate.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && secondeCandidate.properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu )
		{
			return true;
		}

		if( firstCandidate.properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu && secondeCandidate.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu )
		{
			return false;
		}

		return firstCandidate.properties.limits.maxStorageBufferRange > secondeCandidate.properties.limits.maxStorageBufferRange;
	} );
}

void noxcain::GraphicCore::createDevice()
{
	ResultHandler resultHandler( vk::Result::eSuccess );
	

	
	std::array<FLOAT32, 1> priorities =
	{
		1.0F
	};

	std::array<vk::DeviceQueueCreateInfo, 1> queueCreateInfos =
	{
		vk::DeviceQueueCreateInfo( vk::DeviceQueueCreateFlags(), candidates[deviceIndex].queueFamilyIndex, priorities.size(), priorities.data() )
	};

	vk::DeviceCreateInfo deviceCreateInfo( vk::DeviceCreateFlags(), queueCreateInfos.size(), queueCreateInfos.data(), 0, nullptr, necessaryDeviceExtensions.size(), necessaryDeviceExtensions.data() );

	logical_device = resultHandler << getPhysicalDevice().createDevice( deviceCreateInfo );
}

void noxcain::GraphicCore::createSwapChain( const vk::SwapchainKHR& oldSwapChain )
{
	ResultHandler resultHandler( vk::Result::eSuccess );
	const vk::PhysicalDevice& physicalDevice = candidates[deviceIndex].device;
	
	const std::vector<vk::PresentModeKHR>& possiblePresentModes = resultHandler << physicalDevice.getSurfacePresentModesKHR( surface );
	presentationMode = vk::PresentModeKHR::eImmediate;
	std::vector<vk::PresentModeKHR> wantedPresentModes =
	{
		//vk::PresentModeKHR::eFifoRelaxed,
		//vk::PresentModeKHR::eFifo,
		vk::PresentModeKHR::eMailbox,
	};

	while( presentationMode == vk::PresentModeKHR::eImmediate && wantedPresentModes.size() > 0 )
	{
		for( const vk::PresentModeKHR& possiblePresentMode : possiblePresentModes )
		{
			if( possiblePresentMode == wantedPresentModes.back() )
			{
				presentationMode = wantedPresentModes.back();
				break;
			}
		}
		wantedPresentModes.pop_back();
	}

	const vk::SurfaceCapabilitiesKHR& surfaceCapabilities = resultHandler << physicalDevice.getSurfaceCapabilitiesKHR( surface );
	if( presentationMode != vk::PresentModeKHR::eMailbox ) presentationImageCount = 2;
	if( presentationImageCount < surfaceCapabilities.minImageCount ) presentationImageCount = surfaceCapabilities.minImageCount;
	if( surfaceCapabilities.maxImageCount && presentationImageCount > surfaceCapabilities.maxImageCount ) presentationImageCount = surfaceCapabilities.maxImageCount;

	const std::vector<vk::SurfaceFormatKHR>& surfaceFormates = resultHandler << physicalDevice.getSurfaceFormatsKHR( surface );

	surfaceExtent = surfaceCapabilities.currentExtent;
	surfaceFormat = surfaceFormates[0].format;

	swapChain = resultHandler << logical_device.createSwapchainKHR( vk::SwapchainCreateInfoKHR(
		vk::SwapchainCreateFlagsKHR(),
		surface,
		presentationImageCount,
		surfaceFormat,
		surfaceFormates[0].colorSpace,
		surfaceExtent,
		1,
		vk::ImageUsageFlagBits::eColorAttachment,
		vk::SharingMode::eExclusive,
		0, nullptr,
		surfaceCapabilities.currentTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		presentationMode, //vk::PresentModeKHR::eFifo ,
		VK_FALSE,
		oldSwapChain ) );

	std::vector<vk::Image> swapChainImages = resultHandler << logical_device.getSwapchainImagesKHR( swapChain );
	swapChainImageViews.resize( swapChainImages.size() );
	for( UINT32 swapChainImageIndex = 0; swapChainImageIndex < swapChainImages.size(); ++swapChainImageIndex )
	{
		swapChainImageViews[swapChainImageIndex] = resultHandler << logical_device.createImageView( vk::ImageViewCreateInfo(
			vk::ImageViewCreateFlags(), swapChainImages[swapChainImageIndex], vk::ImageViewType::e2D, surfaceFormates[0].format,
			vk::ComponentMapping( vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA ),
			vk::ImageSubresourceRange( vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 ) ) );
	}
}

noxcain::GraphicCore::GraphicCore()
{
}

bool noxcain::GraphicCore::initialize( const PresentationSurface& os_surface )
{
	ResultHandler result_handler( vk::Result::eSuccess );
	std::vector<vk::ExtensionProperties> available_extensions;

	available_extensions = result_handler << vk::enumerateInstanceExtensionProperties();
	if( result_handler.all_okay() )
	{
		os_surface.add_surface_extension( necessary_instance_extensions );

		for( const char* necessary_extension : necessary_instance_extensions )
		{
			bool is_extension_available = false;
			for( const vk::ExtensionProperties& availableExtension : available_extensions )
			{
				if( !strcmp( availableExtension.extensionName, necessary_extension ) )
				{
					is_extension_available = true;
					break;
				}
			}

			if( !is_extension_available )
			{
				return false;
			}
		}

		vk::ApplicationInfo application_info( noxcain::GAME_NAME, noxcain::GAME_VERSION, noxcain::GRAPHIC_ENGINE_NAME, noxcain::GRAPHIC_ENGINE_VERSION, VK_MAKE_VERSION( 1, 1, 83 ) );

		std::vector<const char*> validation_layers;
#ifndef NDEBUG
		validation_layers.push_back( "VK_LAYER_LUNARG_standard_validation" );
		validation_layers.push_back( "VK_LAYER_LUNARG_monitor" );
#endif // NDEBUG


		instance = result_handler << vk::createInstance( vk::InstanceCreateInfo( vk::InstanceCreateFlags(), &application_info,
																				validation_layers.size(), validation_layers.data(),
																				necessary_instance_extensions.size(), necessary_instance_extensions.data() ) );
		if( result_handler.all_okay() )
		{
			surface = os_surface.createSurface( instance );
			pickPhysicalDevice();
			createDevice();
			createSwapChain( swapChain );
		}
	}
	return result_handler.all_okay();
}

noxcain::GraphicCore::~GraphicCore()
{
	if( logical_device )
	{	
		for( const vk::ImageView& imageView : swapChainImageViews )
		{
			logical_device.destroyImageView( imageView );
		}

		logical_device.destroySwapchainKHR( swapChain );
		logical_device.destroy();
	}

	if( instance )
	{
		instance.destroySurfaceKHR( surface );
		instance.destroy();
	}
}

bool noxcain::GraphicCore::switchToNextPhysicalDevice()
{
	if( logical_device )
	{
		for( const vk::ImageView& imageView : swapChainImageViews )
		{
			logical_device.destroyImageView( imageView );
		}
		swapChainImageViews.clear();

		logical_device.destroySwapchainKHR( swapChain );
		logical_device.destroy();
		logical_device = vk::Device();
	}

	++deviceIndex;
	if( !( deviceIndex < candidates.size() ) )
	{
		return false;
	}

	createDevice();
	return true;
}

noxcain::UINT32 noxcain::GraphicCore::get_graphic_queue_family_index() const
{
	return candidates[deviceIndex].queueFamilyIndex;
}

noxcain::UINT32 noxcain::GraphicCore::get_swapchain_image_count() const
{
	return swapChainImageViews.size();
}

vk::Extent2D noxcain::GraphicCore::get_window_extent() const
{
	return surfaceExtent;
}

vk::Format noxcain::GraphicCore::getPresentationSurfaceFormat() const
{
	return surfaceFormat;
}

vk::PhysicalDevice noxcain::GraphicCore::getPhysicalDevice() const
{
	return candidates[deviceIndex].device;
}

vk::Device noxcain::GraphicCore::getLogicalDevice() const
{
	return logical_device;
}

vk::SurfaceKHR noxcain::GraphicCore::getSurface() const
{
	return surface;
}

vk::SwapchainKHR noxcain::GraphicCore::getSwapChain() const
{
	return swapChain;
}

vk::ImageView noxcain::GraphicCore::getImageView( UINT32 index ) const
{
	return swapChainImageViews[index];
}

void noxcain::GraphicCore::recreateSwapChain()
{
	for( const vk::ImageView& imageView : swapChainImageViews )
	{
		logical_device.destroyImageView( imageView );
	}
	swapChainImageViews.clear();

	vk::SwapchainKHR oldSwapChain = swapChain;

	createSwapChain( oldSwapChain );

	logical_device.destroySwapchainKHR( oldSwapChain );
}
