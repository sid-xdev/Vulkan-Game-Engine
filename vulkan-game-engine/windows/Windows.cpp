#include <windows/Windows.hpp>

#include <logic/GameLogicEngine.hpp>
#include <tools/ResultHandler.hpp>

LRESULT CALLBACK noxcain::Window::processMessage( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam )
{
	switch( uMsg )
	{
		case WM_KEYDOWN:
		{
			LogicEngine::set_event( InputEventTypes::KEY_DOWN, 0, 0, UINT32( wParam ) );
			break;
		}
		case WM_KEYUP:
		{
			LogicEngine::set_event( InputEventTypes::KEY_UP, 0, 0, UINT32( wParam ) );
			break;
		}
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		{
			LogicEngine::set_event( InputEventTypes::REGION_KEY_DOWN, INT16( lParam ), INT16( lParam >> 16 ), 0x01 );
			break;
		}
		case WM_XBUTTONDOWN:
		case WM_XBUTTONDBLCLK:
		{
			LogicEngine::set_event( InputEventTypes::REGION_KEY_DOWN, INT16( lParam ), INT16( lParam >> 16 ), 0x01 );
			break;
		}
		case WM_LBUTTONUP:
		{
			LogicEngine::set_event( InputEventTypes::REGION_KEY_UP, INT16( lParam ), INT16( lParam >> 16 ), 0x01 );
			break;
		}
		case WM_MOUSEMOVE:
		{
			LogicEngine::set_event( InputEventTypes::REGION_MOVE, INT16( lParam ), INT16( lParam >> 16 ), 0x00 );
			break;
		}
		default:
		{
			break;
		}
	}
	if( uMsg == WM_KEYDOWN || uMsg == WM_KEYUP )
	{
		
	}
	return NULL;
}


vk::SurfaceKHR noxcain::Window::createSurface( const vk::Instance& instance ) const
{
	vk::SurfaceKHR surface;
	while( windowStatus == WindowStatus::eNone );

	if( windowStatus == WindowStatus::eWorking )
	{
		surface = ResultHandler(vk::Result::eSuccess) << instance.createWin32SurfaceKHR( vk::Win32SurfaceCreateInfoKHR( vk::Win32SurfaceCreateFlagsKHR(), getWindowModul( msWindow ), msWindow ) );
	}
	return surface;	
}


void noxcain::Window::add_surface_extension( std::vector<const char*>& extensions ) const
{
	extensions.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
}


noxcain::Window::Window( HINSTANCE hInstance, const  WindowClass& windowClass, INT32 x, INT32 y, UINT32 width, UINT32 height )
{
	windowThread = std::thread( &windowThreadFunction, this, hInstance, windowClass.GetClassId(), x, y, width, height );
}

void noxcain::Window::windowThreadFunction( noxcain::Window* wrapper, HINSTANCE hInstance, LPCSTR classId, INT32 x, INT32 y, UINT32 width, UINT32 height )
{
	HWND wndHandle = CreateWindowEx( NULL, classId, NULL, WS_POPUP | WS_VISIBLE, x, y, width, height, NULL, NULL, hInstance, reinterpret_cast<void*>( wrapper ) );

	if( wndHandle )
	{	
		RAWINPUTDEVICE device = 
		{
			1, //usUsagePage
			2, //usUsage
			0, //flags
			wndHandle //hwndTarget = 
		};
		
		bool sc = RegisterRawInputDevices( &device, 1, sizeof( RAWINPUTDEVICE ) );

		wrapper->msWindow = wndHandle;
		wrapper->windowStatus = WindowStatus::eWorking;
		MSG msg = {};
		BOOL result;
		while( result = GetMessage( &msg, NULL, 0, 0 ) )
		{
			if( result == -1 )
			{
				DebugBreak();
			}
			else
			{
				DispatchMessage( &msg );
			}
		}
	}
	wrapper->msWindow = NULL;
	wrapper->windowStatus = WindowStatus::eDestroyed;
}

noxcain::Window::~Window()
{
	if( windowThread.joinable() )
	{
		PostMessage( msWindow, WM_DESTROY, 0, 0 );
		windowThread.join();
	}
}

LRESULT CALLBACK noxcain::WindowClass::windowProc_( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam )
{
	std::chrono::time_point<std::chrono::steady_clock> time = std::chrono::steady_clock::now();

	if( uMsg == WM_CREATE )
	{
		SetWindowLongPtr( hWnd, 0, (LONG_PTR)( reinterpret_cast<CREATESTRUCT*>( lParam )->lpCreateParams ) );
	}
	else if( hWnd != NULL )
	{
		Window* window = reinterpret_cast<Window*>( GetWindowLongPtr( hWnd, 0 ) );
		if( window )
		{
			if( uMsg == WM_INPUT || uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST || uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST )
			{
				return window->processMessage( hWnd, uMsg, wParam, lParam );
			}
			else if( uMsg == WM_DESTROY )
			{
				PostQuitMessage( 0 );
			}
		}
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

noxcain::WindowClass::WindowClass( HINSTANCE hInstance ) : windowClassProperties_(
	{
		sizeof( WNDCLASSEX ),  //UINT      cbSize;
		CS_OWNDC,			   //UINT      style;
		windowProc_,
		0,                     //int       cbClsExtra;
		sizeof( Window* ),     //int       cbWndExtra;
		hInstance,             //HINSTANCE hInstance;
		NULL,                  //HICON     hIcon;
		NULL,                  //HCURSOR   hCursor;
		NULL,//HBRUSH( COLOR_WINDOW ),                  //HBRUSH    hbrBackground;
		NULL,                  //LPCTSTR   lpszMenuName;
		CLASS_NAME.c_str(),    //LPCTSTR   lpszClassName;
		NULL,                  //HICON     hIconSm;
	} )
{
	windowClassId_ = RegisterClassEx( &windowClassProperties_ );
}

noxcain::WindowClass::~WindowClass()
{	
	if( windowClassId_ )
	{
		UnregisterClass( GetClassId(), NULL );
	}
}

LPCSTR noxcain::WindowClass::GetClassId() const
{
	LPCSTR longPointer = 0x0;
	reinterpret_cast<ATOM*>( &longPointer )[0] = windowClassId_;
	return longPointer;
}

const std::string noxcain::WindowClass::CLASS_NAME( "nxVulkanDefaultWindowClass" );