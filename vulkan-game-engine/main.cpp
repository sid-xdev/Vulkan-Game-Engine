#include <renderer/GameGraphicEngine.hpp>

#ifdef WIN32

#include <windows/Windows.hpp>
#include <Windows.h>

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	std::shared_ptr window_class = std::make_shared<noxcain::WindowClass>( hInstance );
	if( *window_class )
	{
		std::make_shared<noxcain::Window>( window_class )->draw();
	}
	return 0;
}

#elif __ANDROID__

#include <android/AndroidSurface.hpp>

void android_main( struct android_app* app_state )
{
	auto surface = std::make_shared<noxcain::AndroidSurface>( app_state );
	surface->draw();
}
#endif