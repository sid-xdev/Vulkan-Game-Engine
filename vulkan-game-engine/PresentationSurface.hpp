#pragma once
#include <vulkan/vulkan.hpp>

namespace noxcain
{
	class PresentationSurface : public std::enable_shared_from_this<PresentationSurface>
	{
	public:
		virtual vk::SurfaceKHR create_surface( const vk::Instance& instance ) = 0 ;
		virtual void add_surface_extension( std::vector<const char*>& extensions ) const = 0;
		virtual void close() const = 0;
		virtual bool window_changed() const = 0;
		virtual explicit operator bool() const
		{
			return false;
		}
	};
};