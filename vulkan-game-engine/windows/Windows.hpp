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
		WNDCLASSEX windowClassProperties_;
		static LRESULT CALLBACK windowProc_( _In_ HWND   hwnd, _In_ UINT   uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam );
		ATOM windowClassId_ = NULL;

	public:
		WindowClass( const WindowClass& ) = delete;
		WindowClass( WindowClass&& ) = delete;
		WindowClass& operator=( const WindowClass& ) = delete;

		WindowClass( HINSTANCE hInstance );
		~WindowClass();

		LPCSTR GetClassId() const;
	};
	
	class Window : public PresentationSurface
	{
	private:
		friend class WindowClass;

		enum class WindowStatus
		{
			eNone,
			eWorking,
			eDestroyed
		};

		volatile WindowStatus windowStatus = WindowStatus::eNone;
		HWND msWindow = NULL;

		LRESULT processMessage( _In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam );

		std::thread windowThread;
		static void windowThreadFunction( noxcain::Window* wrapper, HINSTANCE hInstance, LPCSTR classId, INT32 x, INT32 y, UINT32 width, UINT32 height );

		INT32 lastMouseX = 0;
		INT32 lastMouseY = 0;

		static HINSTANCE getWindowModul( HWND hWnd )
		{
			return (HINSTANCE)( GetWindowLongPtr( hWnd, GWLP_HINSTANCE ) );
		}

	public:	
		
		vk::SurfaceKHR createSurface( const vk::Instance& instance ) const override;
		void add_surface_extension( std::vector<const char*>& extensions ) const override;
		
		Window( HINSTANCE hInstance, const  WindowClass& windowClass, INT32 x, INT32 y, UINT32 width, UINT32 height );

		Window( const Window& ) = delete;
		Window( Window&& ) = delete;
		Window& operator= ( Window& ) = delete;

		~Window();
	};
}