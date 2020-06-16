#pragma once
#include <Defines.hpp>

#include <array>
#include <chrono>
#include <vector>

namespace noxcain
{	
	using KeyCode = UINT32;

	class KeyEvent
	{
		bool down;
		UINT32 code = 0;
		std::chrono::steady_clock::time_point time_stamp;
	public:
		KeyEvent( bool is_pushed, UINT32 key_code );
		bool is_pushed() const;
		bool is_released() const;
		KeyCode get_key() const;
		const std::chrono::steady_clock::time_point& get_time()
		{
			return time_stamp;
		}
	};
	
	class KeyEventHandler
	{
	private:
		enum class States
		{
			PUSHED,   // was pushed this frame the first time
			DOWN,     // was not released and is still down
			RELEASED, // was released this frame
			UP        // was not pushed again after the last released
		} state = States::UP;
		bool is_activ = true;
		std::array<UINT32, 4> keys = { 0x00, 0x00, 0x00, 0x00 };
		UINT32 pushed_mask = 0;

	public:

		// key is down
		bool is_pushed() const;

		// key got pushed after it was released
		// state holds only for one frame
		bool got_pushed() const;

		// key is up and not used at all
		bool is_released() const;

		// key got released after it was pushed
		// state holds only for one frame
		bool got_released() const;
		
		// simulate release off all keys
		void release();

		// simulate push off a button and holds it down until release
		void push( UINT32 pos = 0 );

		// ignores the key when deactivated
		void deactivate( bool trigger_release = true );
		void activate();

		void set_key( std::size_t pos, UINT32 key );

		// 
		void check_trigger( const std::vector<KeyEvent>& keyEvent );
	};
}