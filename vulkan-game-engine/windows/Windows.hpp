#pragma once
#include <PresentationSurface.hpp>

#include <Windows.h>
#include <string>
#include <thread>
#include <mutex>

namespace noxcain
{	
	class WindowClass
	{
	private:
		static const std::string CLASS_NAME;
		WNDCLASSEX window_class_properties;
		static LRESULT CALLBACK windowProc_( _In_ HWND   hwnd, _In_ UINT   uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam );
		ATOM window_class_id = NULL;

	public:
		explicit operator bool() const
		{
			return window_class_id != 0;
		}
		WindowClass( const WindowClass& ) = delete;
		WindowClass( WindowClass&& ) = delete;
		WindowClass& operator=( const WindowClass& ) = delete;

		WindowClass( HINSTANCE hInstance );
		~WindowClass();

		LPCSTR get_class_id() const;
		HINSTANCE get_modul() const
		{
			return window_class_properties.hInstance;
		}
	};
	
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
	public:	
		void draw();
		vk::SurfaceKHR create_surface( const vk::Instance& instance ) override;
		void add_surface_extension( std::vector<const char*>& extensions ) const override;
		void close() const override;
		explicit operator bool() const override;
		bool window_changed() const override;
		
		Window( std::shared_ptr<WindowClass> window_class );

		~Window();
	};
}