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

void noxcain::GameLevel::update( const std::chrono::nanoseconds& deltaTime, const std::vector<KeyEvent>& key_events, const std::vector<RegionalKeyEvent>& region_key_events )
{
	static auto logic_time_frame_id = TimeFrameCollector::block_id( "Logic" );
	TimeFrameCollector logic_time_frame( logic_time_frame_id );

	const auto& window_resolution = GraphicEngine::get_window_resolution();
	ui_root->set_size( window_resolution.width, window_resolution.height );

	logic_time_frame.start_frame( 0.8, 0.0, 0.0, 0.8, "handle key events" );
	
	for( auto& key_handler : key_handlers )
	{
		key_handler.check_trigger( key_events );
	}

	if( !regional_exclusiv.hit( region_key_events ) )
	{
		ui_root->hit_tree( region_key_events, interaction_parent_stack, interaction_children_stack, interaction_miss_stack );
	}

	logic_time_frame.end_is_start( 0.0, 0.8, 0.0, 0.8, "level logic" );

	update_level_logic( deltaTime );

	logic_time_frame.end_is_start( 0.8, 0.8, 0.0, 0.8, "update scene graph" );

	scene_root->update_global_matrices( scene_graph_parent_stack, scene_graph_children_stack );

	logic_time_frame.end_frame();
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

const noxcain::GameLevel::RenderableContainer<noxcain::RenderableQuad2D>& noxcain::GameLevel::get_color_labels()
{
	for( Renderable<RenderableQuad2D>::List& renderables : color_label_renderables )
	{
		renderables.sort( []( const RenderableQuad2D& element_1, const RenderableQuad2D& element_2 )
		{
			return element_1.get_depth_level() < element_2.get_depth_level();
		} );
	}
	return color_label_renderables;
}

noxcain::UINT32 noxcain::GameLevel::get_ui_height() const
{
	return UINT32( ui_root->get_height() );
}

noxcain::UINT32 noxcain::GameLevel::get_ui_width() const
{
	return UINT32( ui_root->get_width() );
}
