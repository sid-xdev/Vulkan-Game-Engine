#include "GameLogicEngine.hpp"

#include <resources/GameResourceEngine.hpp>
//#include "GameGraphicEngine.h"

#include <logic/GameLogicEngine.hpp>
#include <logic/DebugLevel.hpp>
#include <logic/Quad2D.hpp>
#include <logic/VectorText2D.hpp>
#include <logic/GeometryLogic.hpp>
#include <logic/VectorText3D.hpp>
#include <logic/InputEventHandler.hpp>

#include <logic/level/MineSweeperLevel.hpp>

#include <tools/TimeFrame.hpp>

std::unique_ptr<noxcain::LogicEngine> noxcain::LogicEngine::engine = std::unique_ptr<LogicEngine>( new LogicEngine() );

noxcain::LogicEngine::LogicEngine()
{	
	new_key_events.clear();
	new_region_key_events.clear();

	graphic_settings.current_sample_count = 1;
	graphic_settings.current_super_sampling_factor = 1.0F;

	logic_thread = std::thread( &LogicEngine::logic_update, this );
}

noxcain::UINT32 noxcain::LogicEngine::logic_update()
{
	if( !current_level )
	{
		current_level = std::make_unique<MineSweeperLevel>();
		debug_level = std::make_unique<DebugLevel>();
	}
	
	while( running() )
	{
		std::unique_lock status_lock( status_mutex );
		status_condition.wait( status_lock, [this]() { return status == Status::UPDATING || status == Status::EXIT; } );

		if( status == Status::UPDATING )
		{
			auto time_now = std::chrono::steady_clock::now();
			std::chrono::nanoseconds deltaTime = ( std::chrono::duration_cast<std::chrono::nanoseconds>( time_now - last_update_time_point ) );

			std::vector<KeyEvent> old_key_events;
			std::vector<RegionalKeyEvent> old_region_key_events;
			{
				std::unique_lock event_lock( event_mutex );
				old_key_events.swap( new_key_events );
				old_region_key_events.swap( new_region_key_events );
				if( old_region_key_events.empty() )
				{
					old_region_key_events.emplace_back( RegionalKeyEvent::KeyCodes::NONE, RegionalKeyEvent::Events::NONE, cursor_position.x, cursor_position.y );
				}
			}

			if( debug_level->on() )
			{
				debug_level->update( deltaTime, old_key_events, old_region_key_events );
			}
			else
			{
				current_level->update( deltaTime, old_key_events, old_region_key_events );
			}
			last_update_time_point = time_now;
			
			if( current_level->get_level_status() == GameLevel::Status::FINISHED )
			{
				finish_game();
				status = Status::EXIT;
			}
			else
			{
				status = Status::DORMANT;
			}
			status_condition.notify_all();
		}
	}
	
	return 0;
}

noxcain::LogicEngine::~LogicEngine()
{
	std::unique_lock lock( status_mutex );
	if( status != Status::EXIT )
	{
		//TODO emergency finishing
		status = Status::EXIT;
		status_condition.notify_all();
	}

	if( logic_thread.joinable() )
	{
		logic_thread.join();
	}
}

void noxcain::LogicEngine::finish_game()
{
}

const noxcain::LogicEngine::RenderableContainer<noxcain::VectorText2D>& noxcain::LogicEngine::get_vector_labels()
{
	std::unique_lock lock( engine->status_mutex );
	engine->status_condition.wait( lock, []()->bool
	{
		return engine->status == Status::DORMANT || engine->status == Status::EXIT;
	} );
	return engine->debug_level->on() ? engine->debug_level->get_vector_labels() : engine->current_level->get_vector_labels();
}

const noxcain::LogicEngine::RenderableContainer<noxcain::GeometryObject>& noxcain::LogicEngine::get_geometry_objects()
{
	std::unique_lock lock( engine->status_mutex );
	engine->status_condition.wait( lock, []()->bool
	{
		return engine->status == Status::DORMANT || engine->status == Status::EXIT;
	} );
	return engine->current_level->get_geometry_objects();
}

const noxcain::LogicEngine::RenderableContainer<noxcain::VectorText3D>& noxcain::LogicEngine::get_vector_decals()
{
	std::unique_lock lock( engine->status_mutex );
	engine->status_condition.wait( lock, []()->bool
	{
		return engine->status == Status::DORMANT || engine->status == Status::EXIT;
	} );
	return engine->current_level->get_vector_decals();
}

const noxcain::LogicEngine::RenderableContainer<noxcain::RenderableQuad2D>& noxcain::LogicEngine::get_color_labels()
{
	std::unique_lock lock( engine->status_mutex );
	engine->status_condition.wait( lock, []()->bool
	{
		return engine->status == Status::DORMANT || engine->status == Status::EXIT;
	} );
	return engine->debug_level->on() ? engine->debug_level->get_color_labels() : engine->current_level->get_color_labels();
}

void noxcain::LogicEngine::set_event( InputEventTypes type, INT32 param1, INT32 param2, UINT32 param3 )
{
	static auto correct_y = []( INT32 incorrect_y )
	{
		return engine->current_level->get_ui_height() - 1 - incorrect_y;
	};
	
	std::lock_guard<std::mutex> lock( engine->event_mutex );
	switch( type )
	{
		case InputEventTypes::KEY_DOWN:
		{
			engine->new_key_events.emplace_back( true, param3 );
			break;
		}
		case InputEventTypes::KEY_UP:
		{
			engine->new_key_events.emplace_back( false, param3 );
			break;
		}
		case InputEventTypes::REGION_KEY_DOWN:
		{
			engine->new_region_key_events.emplace_back( static_cast<RegionalKeyEvent::KeyCodes>( param3 ), RegionalKeyEvent::Events::DOWN, param1, correct_y( param2 ) );
			break;
		}
		case InputEventTypes::REGION_KEY_UP:
		{
			engine->new_region_key_events.emplace_back( static_cast<RegionalKeyEvent::KeyCodes>( param3 ), RegionalKeyEvent::Events::UP, param1, correct_y( param2 ) );
			break;
		}
		case InputEventTypes::REGION_MOVE:
		{
			engine->cursor_position.x = param1;
			engine->cursor_position.y = correct_y( param2 );
			break;
		}
	};
}

void noxcain::LogicEngine::update()
{
	std::unique_lock lock( engine->status_mutex );
	if( engine->status == Status::DORMANT )
	{
		engine->status = Status::UPDATING;
		engine->status_condition.notify_all();
	}

}

void noxcain::LogicEngine::finish()
{
	std::unique_lock lock( engine->status_mutex );
	if( engine->status != Status::EXIT )
	{
		engine->finish_game();
		engine->status = Status::EXIT;
		engine->status_condition.notify_all();
	}
}

bool noxcain::LogicEngine::running()
{
	std::unique_lock lock( engine->status_mutex );
	return engine->status != Status::EXIT;
}

void noxcain::LogicEngine::show_performance_overlay()
{
	engine->debug_level->switch_on();
}

noxcain::CursorPosition noxcain::LogicEngine::get_cursor_position()
{
	std::unique_lock lock( engine->event_mutex );
	return engine->cursor_position;
}

noxcain::NxMatrix4x4 noxcain::LogicEngine::get_camera_matrix()
{
	std::unique_lock lock( engine->status_mutex );
	engine->status_condition.wait( lock, []()->bool
	{
		return engine->status == Status::DORMANT || engine->status == Status::EXIT;
	} );
	return engine->current_level->get_active_camera();
}

void noxcain::LogicEngine::set_graphic_settings( UINT32 sampleCount, FLOAT32 superSamplingFactor, UINT32 width, UINT32 height )
{
	std::unique_lock lock( engine->settings_mutex );
	
	engine->graphic_settings.current_sample_count = sampleCount;
	engine->graphic_settings.current_super_sampling_factor = superSamplingFactor;
	engine->graphic_settings.current_resolution = { width, height };
}

