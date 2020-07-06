#pragma once
#include <PresentationSurface.hpp>
#include <android_native_app_glue.h>

#include <thread>

namespace noxcain
{
	class AndroidSurface : public PresentationSurface
	{
	public:
		void draw();

		AndroidSurface( android_app* app_state );
		vk::SurfaceKHR create_surface( const vk::Instance& instance ) override;
		void add_surface_extension( std::vector<const char*>& extensions ) const override;
		explicit operator  bool() const override;
		void close() const override;
		bool window_changed() const override;
		void start_engine();
	private:
		static void application_command_callback( android_app *app, int32_t cmd );
		bool is_engine_started = false;

		android_app* app_state;
		ANativeWindow* used_window = nullptr;
	};
}