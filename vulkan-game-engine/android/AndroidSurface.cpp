#include "AndroidSurface.hpp"

#include <renderer/GameGraphicEngine.hpp>
#include <logic/GameLogicEngine.hpp>
#include <tools/ResultHandler.hpp>

#include <vulkan/vulkan.hpp>

vk::SurfaceKHR noxcain::AndroidSurface::create_surface( const vk::Instance& instance )
{
    vk::SurfaceKHR surface;
    ResultHandler r_handler( vk::Result::eSuccess );

    if( app_state->window )
    {
        used_window = app_state->window;
        surface = r_handler << instance.createAndroidSurfaceKHR( vk::AndroidSurfaceCreateInfoKHR( vk::AndroidSurfaceCreateFlagsKHR(), app_state->window ) );
    }

    if( surface && r_handler.all_okay() ) {
        return surface;
    }
    return vk::SurfaceKHR();
}

void noxcain::AndroidSurface::add_surface_extension( std::vector<const char*>& extensions ) const
{
    extensions.push_back( VK_KHR_ANDROID_SURFACE_EXTENSION_NAME );
}

noxcain::AndroidSurface::AndroidSurface(android_app *state) : app_state( state )
{
    app_state->onAppCmd = application_command_callback;
    app_state->userData = this;
}

void noxcain::AndroidSurface::draw() {
    int ident = -4;
    int events = 0;
    int fd = 0;
    struct android_poll_source *source;
    while(1) {
        while ((ident = ALooper_pollAll(-1, &fd, &events, (void **) &source)) >= 0) {
            // Process this event.
            if (source != NULL) {
                source->process(app_state, source);
            }

            if ( app_state->destroyRequested ) {
                return;
            }
        }
    }
}

void noxcain::AndroidSurface::close() const {
    if( app_state->activity )
    {
        ANativeActivity_finish( app_state->activity );
    }
}

noxcain::AndroidSurface::operator bool() const {
    return app_state->activity;
}

void noxcain::AndroidSurface::application_command_callback(struct android_app *app, int32_t cmd) {
    if (app->userData) {
        auto surface = reinterpret_cast<AndroidSurface *>( app->userData );

         switch( cmd )
         {
             case APP_CMD_INIT_WINDOW:
                 surface->start_engine();
                 LogicEngine::resume();
                 break;
             case APP_CMD_TERM_WINDOW:
                 LogicEngine::pause();
                 break;
         }
    }
}

void noxcain::AndroidSurface::start_engine() {
    if( !is_engine_started ) {
        if( noxcain::GraphicEngine::run(shared_from_this()) ){
            is_engine_started = true;
        } else {
            close();
        }
    }
}

bool noxcain::AndroidSurface::window_changed() const {
    return used_window != app_state->window;
}
