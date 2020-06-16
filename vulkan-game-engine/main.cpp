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
	constexpr INT32 SCREEN_WIDTH = 1920U;
	constexpr INT32 SCREEN_HEIGHT = 1080U;

	noxcain::WindowClass wndClass( hInstance );
	noxcain::Window window( hInstance, wndClass, -SCREEN_WIDTH, 0, SCREEN_WIDTH, SCREEN_HEIGHT );

	return int( noxcain::GraphicEngine::run( window ) );
}
#elif __ANDROID__

#include "AndroidOS/AndroidSurface.h"
#include <android/native_window.h>

void android_main( struct android_app* state )
{
	noxcain::AndroidSurface surface;

	noxcain::GraphicEngine::run( surface );
}
#endif