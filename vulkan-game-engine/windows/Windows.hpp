#pragma once
#include <PresentationSurface.hpp>
#include <Windows.h>
#include <string>
#include <thread>
#include <mutex>
#include <fstream>

namespace noxcain
{	
	/// <summary>
	/// Defines global window properties 
	/// </summary>
	class WindowClass
	{
	private:
		static const std::string CLASS_NAME;
		WNDCLASSEX window_class_properties;
		static LRESULT CALLBACK windowProc_( _In_ HWND   hwnd, _In_ UINT   uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam );
		ATOM window_class_id = NULL;

	public:
		/// <summary>
		/// Checks if class is valid and registered.
		/// </summary>
		/// <returns>true if successful registered</returns>
		explicit operator bool() const
		{
			return window_class_id != 0;
		}
		WindowClass( const WindowClass& ) = delete;
		WindowClass( WindowClass&& ) = delete;
		WindowClass& operator=( const WindowClass& ) = delete;

		WindowClass( HINSTANCE hInstance );
		~WindowClass();

		/// <summary>
		/// get class name
		/// </summary>
		/// <returns>class name as string</returns>
		LPCSTR get_class_id() const;
		
		/// <summary>
		/// get window class modul
		/// </summary>
		/// <returns>Non-null modul(likely the same as passed to winmain)</returns>
		HINSTANCE get_modul() const
		{
			return window_class_properties.hInstance;
		}
	};
	
	/// <summary>
	/// The class manage all os-specific surface and input functionality
	/// including the surface message loop
	/// </summary>
	class Window : public PresentationSurface
	{
	private:
		friend class WindowClass;
		
		std::shared_ptr<WindowClass> window_class;
		HWND window_handle = NULL;

		LRESULT process_message( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam );

		INT32 last_mouse_x = 0;
		INT32 last_mouse_y = 0;

		static HINSTANCE get_window_modul( HWND hWnd )
		{
			return (HINSTANCE)( GetWindowLongPtr( hWnd, GWLP_HINSTANCE ) );
		}

		struct MONITOR
		{
			HMONITOR handle = NULL;
			RECT rectangle = { 0 };
		};

		/// <summary>
		/// Determine position and size for window creation.
		/// </summary>
		/// <returns>chosen window position and size</returns>
		static RECT get_display_configs();

		/// <summary>
		/// Callback for EnumDisplayMonitors 
		/// </summary>
		static BOOL display_callback( HMONITOR monitor, HDC device_context, LPRECT rectangle, LPARAM app_parameter );

	public:
		/// <summary>
		/// Create window and start processing message queue.
		/// called in WinMain
		/// </summary>
		void draw();

		vk::SurfaceKHR create_surface( const vk::Instance& instance ) override;
		void add_surface_extension( std::vector<const char*>& extensions ) const override;
		void close() const override;
		explicit operator bool() const override;
		bool window_changed() const override;
		
		Window( std::shared_ptr<WindowClass> window_class );

		~Window();
	};

	/// <summary>
	/// Hides os-specififc file operations
	/// </summary>
	class WindowsFile : public NxFile
	{
	public:
		void close() override;
		void open( const char* path ) override;
		bool is_open() const override;
		NxFile& seekg( UINT32 offset ) override;
		UINT32 tellg() override;
		NxFile& read( char* buffer, std::size_t count ) override;
	private:
		std::ifstream file;
	};
}