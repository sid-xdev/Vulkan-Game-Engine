#pragma once
#include <Defines.hpp>
#include <vulkan/vulkan.hpp>

namespace noxcain
{
	/// <summary>
	/// interface between os and os independent vulkan instance
	/// </summary>
	class PresentationSurface : public std::enable_shared_from_this<PresentationSurface>
	{
	public:
		/// <summary>
		/// Creates os-dependend surface
		/// </summary>
		/// <param name="instance">iniatlized vulkan instance</param>
		/// <returns>surface for render presentation</returns>
		virtual vk::SurfaceKHR create_surface( const vk::Instance& instance ) = 0 ;

		/// <summary>
		/// Includes os-dependend extensions in the vulkan application extension list
		/// </summary>
		/// <param name="extensions">vulkan application extension list</param>
		virtual void add_surface_extension( std::vector<const char*>& extensions ) const = 0;
		
		/// <summary>
		/// Destroys the os-dependend presentation surface from within the vulkan engine
		/// </summary>
		virtual void close() const = 0;

		/// <summary>
		/// Checks for an external swap of the surface instance
		/// </summary>
		/// <returns>True if surface was externaly swap with another instance</returns>
		virtual bool window_changed() const = 0;

		/// <summary>
		/// triggers the main thread to create a new surface with swapchain
		/// </summary>
		/// <returns>True if surface was externaly swap with another instance</returns>
		virtual void recreate_swapchain( bool recreate_surface ) const = 0;

		virtual explicit operator bool() const
		{
			return false;
		}
	};
};