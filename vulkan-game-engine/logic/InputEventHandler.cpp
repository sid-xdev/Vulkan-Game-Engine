#include "InputEventHandler.hpp"

bool noxcain::KeyEventHandler::is_pushed() const
{
	return state == States::DOWN || state == States::PUSHED;
}

bool noxcain::KeyEventHandler::got_pushed() const
{
	return state == States::PUSHED;
}

bool noxcain::KeyEventHandler::is_released() const
{
	return state == States::UP || state == States::RELEASED;
}

bool noxcain::KeyEventHandler::got_released() const
{
	return state == States::RELEASED;
}

void noxcain::KeyEventHandler::release()
{
	
	if( pushed_mask )
	{
		state = States::RELEASED;
	}
	else
	{
		state = States::UP;
	}
	pushed_mask = 0;
}

void noxcain::KeyEventHandler::push( UINT32 pos )
{
	if( pushed_mask )
	{
		state = States::DOWN;
	}
	else
	{
		state = States::PUSHED;
	}
	pushed_mask |= 0x1 << pos;
}

void noxcain::KeyEventHandler::deactivate( bool release )
{
	is_activ = false;
	if( release ) this->release();
}

void noxcain::KeyEventHandler::activate()
{
	is_activ = true;
}

void noxcain::KeyEventHandler::set_key( std::size_t pos, UINT32 key )
{
	keys[pos] = key;
}

void noxcain::KeyEventHandler::check_trigger( const std::vector<KeyEvent>& keyEvents )
{
	if( is_activ )
	{
		auto old_push_mask = pushed_mask;
		for( const auto& keyEvent : keyEvents )
		{
			KeyCode keyCode = keyEvent.get_key();
			for( std::size_t index = 0; index < keys.size(); ++index )
			{
				if( keys[index] > 0x00 && keys[index] == keyCode )
				{
					if( keyEvent.is_pushed() )
					{
						pushed_mask |= 0x1 << index;
					}
					else
					{
						pushed_mask &= ( ~UINT32( 1 ) ) << index;
					}
				}
			}
		}

		if( pushed_mask )
		{
			if( old_push_mask )
			{
				state = States::DOWN;
			}
			else
			{
				state = States::PUSHED;
			}
		}
		else
		{
			if( old_push_mask )
			{
				state = States::RELEASED;
			}
			else
			{
				state = States::UP;
			}
		}
	}
}

noxcain::KeyEvent::KeyEvent( bool is_pushed, UINT32 keyCode ) : down( is_pushed ), code( keyCode ), time_stamp( std::chrono::steady_clock::now() )
{
}

bool noxcain::KeyEvent::is_pushed() const
{	
	return down;
}

bool noxcain::KeyEvent::is_released() const
{
	return !down;
}

noxcain::KeyCode noxcain::KeyEvent::get_key() const
{
	return code;
}
