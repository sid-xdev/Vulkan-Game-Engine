#include "Level.hpp"

#include <renderer/GameGraphicEngine.hpp>

#include <logic/GeometryLogic.hpp>
#include <logic/InputEventHandler.hpp>
#include <logic/Quad2D.hpp>
#include <logic/VectorText3D.hpp>
#include <logic/VectorText2D.hpp>

#include <math/Vector.hpp>

#include <tools/TimeFrame.hpp>

noxcain::SceneGraphNode& noxcain::GameLevel::get_scene_root()
{
	return *scene_root;
}

noxcain::PassivRecieverNode& noxcain::GameLevel::get_screen_root()
{
	return *ui_root;
}

void noxcain::GameLevel::set_status( Status newStatus )
{
	status = newStatus;
}

void noxcain::GameLevel::clear_user_interfaces()
{
	for( GameUserInterface& user_interface : user_interfaces )
	{
		if( user_interface.regional_event_root )
		{
			user_interface.regional_event_root->cut_branch();
		}
	}
	user_interfaces.clear();
}

void noxcain::GameLevel::add_user_interface( GameUserInterface& new_interface )
{
	user_interfaces.emplace_back( new_interface );
}

void noxcain::GameLevel::update_key_events( const std::vector<KeyEvent>& key_events )
{
	
	TimeFrame time_frame( time_collector, 0.8, 0.0, 0.0, 0.8, "key events" );
	for( auto& key_handler : key_handlers )
	{
		key_handler.check_trigger( key_events );
	}
}

void noxcain::GameLevel::update_logic( const std::chrono::nanoseconds& deltaTime, const std::vector<RegionalKeyEvent>& region_key_events )
{
	time_collector.start_frame( 0.0, 0.8, 0.8, 0.8, "region input events" );

	const auto& window_resolution = GraphicEngine::get_window_resolution();
	ui_root->set_size( window_resolution.width, window_resolution.height );

	if( !regional_exclusiv.hit( region_key_events ) )
	{
		ui_root->collapse_branch();
		for( GameUserInterface& interface : user_interfaces )
		{
			if( interface.regional_event_root )
			{
				ui_root->add_branch( *interface.regional_event_root );
			}
		}
		ui_root->hit_tree( region_key_events, interaction_parent_stack, interaction_children_stack, interaction_miss_stack );
	}

	time_collector.end_is_start( 0.0, 0.8, 0.0, 0.8, "level logic" );

	update_level_logic( deltaTime );

	time_collector.end_is_start( 0.8, 0.8, 0.0, 0.8, "update scene graph" );

	scene_root->update_global_matrices( scene_graph_parent_stack, scene_graph_children_stack );

	time_collector.end_frame();

	for( GameUserInterface& ui : user_interfaces )
	{
		ui.sort();
	}
}

noxcain::GameLevel::GameLevel() :
	ui_root( std::make_unique<PassivRecieverNode>() ),
	scene_root( std::make_unique<SceneGraphNode>() )
{
}

noxcain::GameLevel::~GameLevel()
{
}

noxcain::NxMatrix4x4 noxcain::GameLevel::get_active_camera() const
{
	return activeCameraPerspective * activeCameraPosition.inverse();
}

noxcain::GameLevel::Status noxcain::GameLevel::get_level_status() const
{
	return status;
}

noxcain::UINT32 noxcain::GameLevel::get_ui_height() const
{
	if( ui_root )
	{
		return UINT32( ui_root->get_height() );
	}
	return 0;
}

noxcain::UINT32 noxcain::GameLevel::get_ui_width() const
{
	if( ui_root )
	{
		return UINT32( ui_root->get_width() );
	}
	return 0;
}
