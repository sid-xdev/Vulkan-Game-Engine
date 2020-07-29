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
        static int32_t input_event_callback(struct android_app* app, AInputEvent* event);
		static void application_command_callback( android_app *app, int32_t cmd );
		bool is_engine_started = false;

		android_app* app_state;
		ANativeWindow* used_window = nullptr;
	};

	class AndroidFile : public NxFile
	{
	public:
		static void set_manager( AAssetManager* manager );
		void open( const char* path ) override;
		bool is_open() const override;
		NxFile& seekg( UINT32 offset ) override;
		UINT32 tellg() override;
		NxFile& read( char* buffer, std::size_t count ) override;
		void close() override;
	private:
		static AAssetManager* asset_manager;
		AAsset* file = nullptr;
	};
}