#pragma once
#include <vulkan/vulkan.hpp>

namespace noxcain
{
	class PresentationSurface
	{
	public:
		virtual vk::SurfaceKHR createSurface( const vk::Instance& instance ) const = 0 ;
		virtual void add_surface_extension( std::vector<const char*>& extensions ) const = 0;
	};
};