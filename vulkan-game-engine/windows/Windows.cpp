#include <windows/Windows.hpp>

#include <vulkan/vulkan.hpp>
#include <renderer/GameGraphicEngine.hpp>
#include <logic/GameLogicEngine.hpp>
#include <tools/ResultHandler.hpp>

LRESULT CALLBACK noxcain::Window::process_message( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam )
{
	INT32 height = 0;
	RECT rectangle = { 0 };
	if( GetClientRect( hWnd, &rectangle ) )
	{
		height = max( 0, rectangle.bottom - rectangle.top - 1 );
	}

	switch( uMsg )
	{
		case WM_KEYDOWN:
		{
			/*
			if( wParam == 0x57 )
			{
				RECT rect;
				GetWindowRect( window_handle, &rect );
				SetWindowPos( window_handle, NULL, rect.left, 0, ( rect.right - rect.left )/2, ( rect.bottom - rect.top )/2, NULL );
			}

			if( wParam == 0x51 )
			{
				LogicEngine::pause();
			}

			if( wParam == 0x45 )
			{
				LogicEngine::resume();
			}
			*/
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
			LogicEngine::set_event( InputEventTypes::REGION_KEY_DOWN, INT16( lParam ), height - INT16( lParam >> 16 ), 0x01 );
			break;
		}
		case WM_XBUTTONDOWN:
		case WM_XBUTTONDBLCLK:
		{
			LogicEngine::set_event( InputEventTypes::REGION_KEY_DOWN, INT16( lParam ), height - INT16( lParam >> 16 ), 0x01 );
			break;
		}
		case WM_LBUTTONUP:
		{
			LogicEngine::set_event( InputEventTypes::REGION_KEY_UP, INT16( lParam ), height - INT16( lParam >> 16 ), 0x01 );
			break;
		}
		case WM_MOUSEMOVE:
		{
			LogicEngine::set_event( InputEventTypes::REGION_MOVE, INT16( lParam ), height - INT16( lParam >> 16 ), 0x00 );
			break;
		}
		default:
		{
			break;
		}
	}
	return NULL;
}


vk::SurfaceKHR noxcain::Window::create_surface( const vk::Instance& instance )
{
	vk::SurfaceKHR surface;
	ResultHandler r_handler( vk::Result::eSuccess );
	if( operator bool() )
	{
		surface = r_handler << instance.createWin32SurfaceKHR( vk::Win32SurfaceCreateInfoKHR( vk::Win32SurfaceCreateFlagsKHR(), get_window_modul( window_handle ), window_handle ) );
	}

	if( surface && r_handler.all_okay() )
	{
		return surface;
	}
	return vk::SurfaceKHR();
}


void noxcain::Window::add_surface_extension( std::vector<const char*>& extensions ) const
{
	extensions.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
}

void noxcain::Window::close() const
{
	if( operator bool() )
	{
		PostMessage( window_handle, WM_DESTROY, NULL, NULL );
	}
}

noxcain::Window::operator bool() const
{
	return IsWindow( window_handle );
}

bool noxcain::Window::window_changed() const
{
	//TODO should never happen under windows?
	//important for android and co.
	return GetActiveWindow() != window_handle;
}

noxcain::Window::Window( std::shared_ptr<WindowClass> window_class ) : window_class( window_class )
{
}

RECT noxcain::Window::get_display_configs()
{
	std::vector<MONITOR> monitors;
	RECT final_rectangle = { 0 };

	while( EnumDisplayMonitors( NULL, NULL, display_callback, reinterpret_cast<LPARAM>( &monitors ) ) );
	
	for( const auto& current_monitor : monitors )
	{
		if( current_monitor.rectangle.left == 0 && current_monitor.rectangle.top == 0 )
		{
			final_rectangle = current_monitor.rectangle;
			break;
		}
	}
	return final_rectangle;
}

BOOL noxcain::Window::display_callback( HMONITOR monitor, HDC device_context, LPRECT rectangle, LPARAM app_parameter )
{
	auto list = reinterpret_cast<std::vector<MONITOR>*>( app_parameter );
	if( list->size() && list->front().handle == monitor )
	{
		return false;
	}

	MONITOR current_monitor;
	current_monitor.handle = monitor;
	current_monitor.rectangle = *rectangle;
	list->emplace_back( current_monitor );
	return true;
}

void noxcain::Window::draw()
{
	if( window_class )
	{
		//TODO get config start parameters

		RECT window_rectangle = get_display_configs();

		HWND w_handle = CreateWindowEx( NULL, window_class->get_class_id(), NULL, WS_POPUP, 
										window_rectangle.left, window_rectangle.top, window_rectangle.right - window_rectangle.left, window_rectangle.bottom - window_rectangle.top,
										NULL, NULL, window_class->get_modul(), reinterpret_cast<void*>( this ) );
		if( w_handle )
		{
			window_handle = w_handle;
			ShowWindow( w_handle, SW_SHOW );

			if( !noxcain::GraphicEngine::run( shared_from_this() ) )
			{
				close();
			}

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
		window_handle = NULL;
	}
}

noxcain::Window::~Window()
{
	if( operator bool() )
	{
		PostMessage( window_handle, WM_DESTROY, 0, 0 );
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
				return window->process_message( hWnd, uMsg, wParam, lParam );
			}

			switch( uMsg )
			{
				case WM_DESTROY:
				{
					PostQuitMessage( 0 );
					break;
				}
				case WM_ACTIVATE:
				{
					if( wParam == WA_INACTIVE )
					{
						ShowWindow( window->window_handle, SW_MINIMIZE );
					}
					else
					{
						ShowWindow( window->window_handle, SW_RESTORE );
					}
					break;
				}
				default:
				{
					break;
				}
			}
		}
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

noxcain::WindowClass::WindowClass( HINSTANCE hInstance ) : window_class_properties(
	{
		sizeof( WNDCLASSEX ),  //UINT      cbSize;
		CS_OWNDC,			   //UINT      style;
		windowProc_,
		0,                     //int       cbClsExtra;
		sizeof( Window* ),     //int       cbWndExtra;
		hInstance,             //HINSTANCE hInstance;
		NULL,                  //HICON     hIcon;
		LoadCursor( NULL, IDC_ARROW ),                  //HCURSOR   hCursor;
		NULL,//HBRUSH( COLOR_WINDOW ),                  //HBRUSH    hbrBackground;
		NULL,                  //LPCTSTR   lpszMenuName;
		CLASS_NAME.c_str(),    //LPCTSTR   lpszClassName;
		NULL,                  //HICON     hIconSm;
	} )
{
	window_class_id = RegisterClassEx( &window_class_properties );
}

noxcain::WindowClass::~WindowClass()
{	
	if( window_class_id )
	{
		UnregisterClass( get_class_id(), NULL );
	}
}

LPCSTR noxcain::WindowClass::get_class_id() const
{
	LPCSTR longPointer = 0x0;
	reinterpret_cast<ATOM*>( &longPointer )[0] = window_class_id;
	return longPointer;
}

const std::string noxcain::WindowClass::CLASS_NAME( "nxVulkanDefaultWindowClass" );

void noxcain::WindowsFile::close()
{
	file.close();
}

void noxcain::WindowsFile::open( const char* path )
{
	file.open( std::string("Assets/") + path, std::ios::binary );
}

bool noxcain::WindowsFile::is_open() const
{
	return file.is_open();
}

noxcain::NxFile& noxcain::WindowsFile::seekg( UINT32 offset )
{
	file.seekg( offset );
	return *this;
}

UINT32 noxcain::WindowsFile::tellg()
{
	return file.tellg();
}

noxcain::NxFile& noxcain::WindowsFile::read( char* buffer, std::size_t count )
{
	file.read( buffer, count );
	return *this;
}
