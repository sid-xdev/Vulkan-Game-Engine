#include "MineSweeperLevel.hpp"

#include <logic/level/HexField.hpp>

#include <logic/GameLogicEngine.hpp>
#include <logic/InputEventHandler.hpp>
#include <logic/Quad2D.hpp>
#include <logic/VectorText2D.hpp>
#include <logic/VectorText3D.hpp>

#include <logic/gui/Button.hpp>

#include <resources/BoundingBox.hpp>
#include <resources/GameResourceEngine.hpp>

#include <math/Vector.hpp>
#include <math/Spline.hpp>

#include <cmath>

noxcain::MineSweeperLevel::MineSweeperLevel() :
	//fieldOrientationSpline( std::make_unique<CubicSpline>() ),
	camera_distance_label( std::make_unique<VectorText2D>( vector_label_list ) ),
	cpu_cycle_label( std::make_unique<VectorText2D>( vector_label_list ) ),
	gpu_cycle_label( std::make_unique<VectorText2D>( vector_label_list ) ),
	debug_button( std::make_unique<BaseButton>( vector_label_list, color_label_list ) ),
	switch_font_button( std::make_unique<BaseButton>( vector_label_list, color_label_list ) )
{
	vector_decal_renderables.emplace_back( vector_dacal_list );
	color_label_renderables.emplace_back( color_label_list );
	geometry_renderables.emplace_back( geometry_list );
	vector_label_renderables.emplace_back( vector_label_list );
	
	//camera distance label
	camera_distance_label->set_top_anchor( get_screen_root() );
	camera_distance_label->set_right_anchor( get_screen_root() );
	camera_distance_label->set_anchor( get_screen_root(), HorizontalAnchorType::RIGHT, VerticalAnchorType::TOP, HorizontalAnchorType::RIGHT, VerticalAnchorType::TOP );
	camera_distance_label->get_text().set_size( 24 );

	//fps displays
	cpu_cycle_label->set_top_anchor( get_screen_root() );
	gpu_cycle_label->set_vertical_anchor( VerticalAnchorType::TOP, *cpu_cycle_label, VerticalAnchorType::BOTTOM );

	cpu_cycle_label->set_left_anchor( get_screen_root() );
	gpu_cycle_label->set_left_anchor( get_screen_root() );

	cpu_cycle_label->get_text().set_size( 24 );
	gpu_cycle_label->get_text().set_size( 24 );

	setup_events();
	grid = std::make_unique<SceneGraphNode>();
	get_scene_root().add_branch( *grid );

	setup_level();

	

	// add debug button
	get_screen_root().add_branch( *debug_button );
	debug_button->get_area().set_bottom_anchor( get_screen_root(), 30 );
	debug_button->get_area().set_right_anchor( get_screen_root(), -30 );
	debug_button->get_area().set_width( 100 );
	debug_button->get_area().set_height( 40 );

	debug_button->set_background_color( 0.4F, 0.4F, 0.4F );
	debug_button->set_highlight_color( 0.6F, 0.6F, 0.6F, 1.0F );
	debug_button->set_active_color( 0.8F, 0.8F, 0.8F );
	debug_button->get_text_element().set_color( 0.0F, 0.0F, 0.0F );
	debug_button->get_text_element().set_utf8( "PROFIL" );
	debug_button->get_text_element().set_size( 24 );
	debug_button->set_auto_resize( VectorTextLabel2D::AutoResizeModes::FULL );
	debug_button->set_frame_size( 2 );
	//debug_button->set_frame_color( 0.35, 0.35, 0.35 );
	debug_button->set_frame_color( 1.0, 1.0, 1.0 );

	debug_button->set_click_handler( []( const RegionalKeyEvent&, BaseButton& ) -> bool
	{
		LogicEngine::show_performance_overlay();
		return true;
	} );

	debug_button->show();

	// add switch Font Button
	get_screen_root().add_branch( *switch_font_button );
	switch_font_button->get_area().set_bottom_anchor( get_screen_root(), 30 );
	switch_font_button->get_area().set_left_anchor( get_screen_root(), 30 );
	switch_font_button->get_area().set_width( 100 );
	switch_font_button->get_area().set_height( 40 );

	switch_font_button->set_background_color( 0.4F, 0.4F, 0.4F );
	switch_font_button->set_highlight_color( 0.6F, 0.6F, 0.6F, 1.0F );
	switch_font_button->set_active_color( 0.8F, 0.8F, 0.8F );
	switch_font_button->get_text_element().set_color( 0.0F, 0.0F, 0.0F );
	switch_font_button->get_text_element().set_utf8( "SWITCH FONT" );
	switch_font_button->get_text_element().set_size( 24 );
	switch_font_button->set_auto_resize( VectorTextLabel2D::AutoResizeModes::FULL );
	switch_font_button->set_frame_size( 2 );
	switch_font_button->set_frame_color( 1.0, 1.0, 1.0 );
	//switch_font_button->set_frame_color( 1.0, 0, 0 );

	switch_font_button->set_click_handler( [this]( const RegionalKeyEvent&, BaseButton& ) -> bool
	{
		
		if( !hex_fields.empty() )
		{
			std::size_t next_font_id = ( hex_fields.front()->get_font_id() + 1 ) % ResourceEngine::get_engine().get_invalid_font_id();
			for( auto& hex_field : hex_fields )
			{
				hex_field->set_font_id( next_font_id );
			}
		}
		return true;
	} );

	switch_font_button->show();

	camera_distance_label->show();
}

noxcain::MineSweeperLevel::~MineSweeperLevel()
{
}


void noxcain::MineSweeperLevel::rotate_grid( DOUBLE milliseconds, const NxVector3D& rotationAxis )
{
	constexpr DOUBLE anglePerMs = 0.003;
	DOUBLE angle = anglePerMs * milliseconds;

	grid->set_local_matrix( grid->get_local_matrix()*NxMatrix4x4().rotation( rotationAxis, angle ) );
}

void noxcain::MineSweeperLevel::setup_level()
{
	constexpr DOUBLE ratio = 1.1;
	
	DOUBLE hexSideLength = HexField::get_hex_side_length();
	DOUBLE rowLength = hexSideLength * ( 1.5 * columnCount + 0.5 ); //columnCount;
	DOUBLE radius = 0.5*rowLength / ratio;
	const DOUBLE s = hexSideLength * std::sqrt( 3.0 );
	const UINT32 rowCount = UINT32( PI / std::asin( s / ( 2 * radius ) ) );
	lineAngle = ( 2.0 * PI ) / DOUBLE( rowCount );
	const DOUBLE realRadius = s / ( 2 * sin( 0.5 * lineAngle ) );
	
	UINT32 count = 0;

	NxVector3D end_point( 0, 30, 0 );
	splines.emplace_back( CubicSpline(
		{
			NxVector3D( 0, -30, 0 ), NxVector3D( 30, -20, 0 ),

			NxVector3D( 30, -10, 0 ), NxVector3D( 0, 0, 0 ),

			NxVector3D( -30, 20, 0 ), end_point,
/*
			NxVector3D( 0, -20, 55 ), NxVector3D( 0, 0, 55 ),

			NxVector3D( 0, 20, 60 ), NxVector3D( 0, 40, 60 ),

			NxVector3D( 0, 40, 20 ), NxVector3D( 0, 40, 0 )*/
		} ) );

	auto& spline = splines.back();
	DOUBLE curve_length = spline.get_length();;
	DOUBLE step_width = HexField::get_object_space_bounding_box().get_height();

	step_width = step_width / curve_length;

	DOUBLE current_step = 0;

	//hex_fields.emplace_back( std::make_unique<HexField>( *this ) );

	hex_fields.emplace_back( std::make_unique<HexField>( *this ) );
	/*
	while( current_step <= 1.0 )
	{
		
		auto& current_hex = hex_fields.back();

		const NxVector3D y_axis = spline.get_direction( current_step );
		const NxVector3D z_axis = ( end_point - spline.get_position( current_step ) ).cross( y_axis ).normalize();
		const NxVector3D x_axis = y_axis.cross( z_axis ).normalize();

		NxMatrix4x4 position( x_axis, y_axis, z_axis, spline.get_position( current_step ) );

		current_hex->set_local_matrix( position.translation( NxVector3D( 0, 0, -HexField::get_object_space_bounding_box().get_depth() ) ) );
		current_hex->set_mine_number( count++ );

		current_step += step_width;
	}
	*/
	//NxMatrix4x4 centering = NxMatrix4x4().translation( { -0.5 * rowLength + hexSideLength,0,0 } );
	//for( std::size_t index = 0; index < 11/*index < UINT32( 0.5 * rowCount + rowCount % 2 + 2 )*/; ++index )
	/*	
	{
		//for( std::size_t line = 0; line < columnCount; ++line )
		{

			//trans.translation( { line * 1.5 * hexSideLength, 0, 0 } );
			
			hex_fields.emplace_back( std::make_unique<HexField>( *this ) );
			auto& current_hex = hex_fields.back();
			
			
			

			const NxVector3D x_axis( 0.0, 0.0, 1.0 );
			const NxVector3D& y_axis = curve.get_direction( step_width * index );

			NxMatrix4x4 position( y_axis.cross( x_axis ).normalize(), y_axis, x_axis, curve.get_position( step_width * index ) );

			current_hex->set_local_matrix( position );
			//current_hex->set_local_matrix( centering * trans * NxMatrix4x4().rotation( { 1,0,0 }, -0.5*PI + lineAngle*(index + ( line % 2 ) * 0.5 - 1 ) ) * NxMatrix4x4().translation( { 0,0,realRadius + 0.5 } ) );
			current_hex->set_mine_number( count++ );
			//current_hex->set_mine_neighbor_count( index % 7 );
		}
	}
	*/
	camera_screen_width = /*2.0**/radius;
	camera_space_near = 0.5*radius;
	camera_space_far = camera_space_near + rowLength;

	//activeCameraPosition = NxMatrix4x4().translation( { 0,0, /*0.5* rowLength + camera_space_near + hexSideLength*/50 } );
}

noxcain::KeyEventHandler& noxcain::MineSweeperLevel::get_key( KeyEvents key )
{
	return key_handlers[static_cast<std::size_t>( key )];
}

void noxcain::MineSweeperLevel::setup_events()
{
	key_handlers.resize( KEY_EVENT_COUNT );

	//key_handlers[static_cast<std::size_t>( KeyEvents::eRotateGridLeft )].set_key( 0, 0x44 );
	//key_handlers[static_cast<std::size_t>( KeyEvents::eRotateGridRight )].set_key( 0, 0x41 );
	//key_handlers[static_cast<std::size_t>( KeyEvents::eRotateGridUp )].set_key( 0, 0x26 );
	//key_handlers[static_cast<std::size_t>( KeyEvents::eRotateGridDown )].set_key( 0, 0x28 );

	get_key( KeyEvents::FINSIH ).set_key( 0, 0x1B );
	get_key( KeyEvents::FINSIH ).set_key( 1, 0x02 );

	get_key( KeyEvents::MOVE_CAMERA_BACKWARD ).set_key( 0, 0x45 );
	get_key( KeyEvents::MOVE_CAMERA_FORWARD ).set_key( 0, 0x51 );
	get_key( KeyEvents::ROTATE_CAMERA_DOWN ).set_key( 0, 0x53 );
	get_key( KeyEvents::ROTATE_CAMERA_UP ).set_key( 0, 0x57 );
	get_key( KeyEvents::ROTATE_CAMERA_RIGHT ).set_key( 0, 0x44 );
	get_key( KeyEvents::ROTATE_CAMERA_LEFT ).set_key( 0, 0x41 );
}

void noxcain::MineSweeperLevel::check_events( const std::chrono::nanoseconds& delta )
{
	DOUBLE milliseconds = std::chrono::duration_cast<std::chrono::duration<DOUBLE, std::milli>>( delta ).count();

	if( get_key( KeyEvents::eRotateGridLeft ).is_pushed() )
	{
		rotate_grid( milliseconds, NxVector3D( { 0.0,1.0,0.0 } ) );
	}

	if( get_key( KeyEvents::eRotateGridRight ).is_pushed() )
	{
		rotate_grid( milliseconds, NxVector3D( { 0.0,-1.0,0.0 } ) );
	}

	if( get_key( KeyEvents::FINSIH ).got_pushed() )
	{
		set_status( Status::FINISHED );
	}

	if( get_key( KeyEvents::eRotateGridUp ).is_pushed() )
	{
		currentAngle -= milliseconds *anglePerMs;
	}

	if( get_key( KeyEvents::eRotateGridDown ).is_pushed() )
	{
		currentAngle += milliseconds *anglePerMs;
	}

	if( get_key( KeyEvents::eRotateGridDown ).is_pushed() )
	{
		currentAngle += milliseconds * anglePerMs;
	}

	if( get_key( KeyEvents::MOVE_CAMERA_BACKWARD ).is_pushed() )
	{
		camera_center_distance += milliseconds * 0.01;
	}

	if( get_key( KeyEvents::MOVE_CAMERA_FORWARD ).is_pushed() )
	{
		camera_center_distance -= milliseconds * 0.01;
	}

	if( get_key( KeyEvents::ROTATE_CAMERA_UP ).is_pushed() )
	{
		camera_vertical_angle += milliseconds * 0.001;
	}

	if( get_key( KeyEvents::ROTATE_CAMERA_DOWN ).is_pushed() )
	{
		camera_vertical_angle -= milliseconds * 0.001;
	}

	if( get_key( KeyEvents::ROTATE_CAMERA_LEFT ).is_pushed() )
	{
		camera_horizontal_angle += milliseconds * 0.001;
	}

	if( get_key( KeyEvents::ROTATE_CAMERA_RIGHT ).is_pushed() )
	{
		camera_horizontal_angle -= milliseconds * 0.001;
	}

	if( get_key( KeyEvents::ROTATE_CAMERA_RIGHT ).is_pushed() )
	{
		camera_horizontal_angle -= milliseconds * 0.001;
	}
}

void noxcain::MineSweeperLevel::update_camera()
{	
	auto reso = LogicEngine::get_graphic_settings().get_accumulated_resolution();
	
	if( reso.width == 0 || reso.height == 0 )
	{
		return;
	}
	
	DOUBLE ratio = DOUBLE( reso.width ) / DOUBLE( reso.height );

	camera_screen_height = camera_screen_width / ratio;
	DOUBLE right = 0.5 * camera_screen_width;
	DOUBLE top = 0.5 * -camera_screen_height;
	
	activeCameraPerspective.getColumn( 0 )[0] = camera_space_near / right;
	activeCameraPerspective.getColumn( 1 )[1] = camera_space_near / top;
	
	//activeCameraPerspective.getColumn( 0 )[0] = 2*camera_space_near / camera_screen_width;
	//activeCameraPerspective.getColumn( 1 )[1] = -2*camera_space_near / camera_screen_height;
	activeCameraPerspective.getColumn( 2 )[2] = camera_space_far / ( camera_space_near - camera_space_far );
	activeCameraPerspective.getColumn( 2 )[3] = -1.0;

	activeCameraPerspective.getColumn( 3 )[2] = ( camera_space_far * camera_space_near ) / ( camera_space_near - camera_space_far );
	activeCameraPerspective.getColumn( 3 )[3] = 0.0;

	activeCameraPosition = NxMatrix4x4().translation( { 0.0, 0.0, camera_center_distance } ).rotationX( camera_vertical_angle ).rotationY( camera_horizontal_angle );
}

void noxcain::MineSweeperLevel::update_level_logic( const std::chrono::nanoseconds& deltaTime )
{
	cycle_display_wait_time += deltaTime;
	if( cycle_display_wait_time > CYCLE_TIME_DISPLAY_REFRESH_RATE )
	{
		cpu_cycle_label->get_text().set_utf8( "CPU: " + std::to_string( std::chrono::seconds( 1 ) / LogicEngine::get_cpu_cycle_duration() ) + " fps" );
		gpu_cycle_label->get_text().set_utf8( "GPU: " + std::to_string( std::chrono::seconds( 1 ) / LogicEngine::get_gpu_cycle_duration() ) + " fps" );
		cycle_display_wait_time = std::chrono::nanoseconds( 0 );
	}
	DOUBLE xx = 0.5*std::chrono::duration_cast<std::chrono::duration<DOUBLE>>( deltaTime ).count();
	progress += xx;
	while( progress > 1.0 ) progress -= 1.0;

	check_events( deltaTime );
	update_camera();

	camera_distance_label->get_text().set_utf8( std::to_string( camera_center_distance ) );

	auto& current_hex = hex_fields.back();
	auto& spline = splines.back();

	auto end_point = spline.get_position( 1.0 );

	const NxVector3D y_axis = spline.get_direction( progress );
	const NxVector3D z_axis = NxVector3D( 1.0, 0.0, 0.0 ).cross( y_axis ).normalize();
	const NxVector3D x_axis = y_axis.cross( z_axis ).normalize();

	NxMatrix4x4 position( x_axis, y_axis, z_axis, spline.get_position( progress ) );

	current_hex->set_local_matrix( position.translation( NxVector3D( 0, 0, -HexField::get_object_space_bounding_box().get_depth() ) ) );
	current_hex->set_mine_number( progress * 100.0 );
}

