#include "MineSweeperLevel.hpp"

#include <logic/level/HexField.hpp>

#include <logic/GameLogicEngine.hpp>
#include <logic/InputEventHandler.hpp>
#include <logic/Quad2D.hpp>
#include <logic/VectorText2D.hpp>
#include <logic/VectorText3D.hpp>

#include <logic/gui/Button.hpp>
#include <logic/gui/Label.hpp>

#include <resources/BoundingBox.hpp>
#include <resources/GameResourceEngine.hpp>

#include <math/Vector.hpp>
#include <math/Spline.hpp>

#include <cmath>

void noxcain::MineSweeperLevel::create_performance_hud()
{
	//fps displays
	cpu_cycle_label->set_top_anchor( get_screen_root() );
	gpu_cycle_label->set_vertical_anchor( VerticalAnchorType::TOP, *cpu_cycle_label, VerticalAnchorType::BOTTOM );

	cpu_cycle_label->set_left_anchor( get_screen_root(), 5 );
	gpu_cycle_label->set_left_anchor( get_screen_root(), 5 );

	cpu_cycle_label->get_text().set_size( 24 );
	gpu_cycle_label->get_text().set_size( 24 );

	cpu_cycle_label->show();
	gpu_cycle_label->show();

	// add debug button
	debug_button->get_area().set_vertical_anchor( VerticalAnchorType::TOP, *switch_font_button, VerticalAnchorType::BOTTOM, -5 );
	debug_button->get_area().set_left_anchor( get_screen_root(), 5 );
	debug_button->get_area().set_width( 100 );
	debug_button->get_area().set_height( 40 );

	debug_button->get_text_element().set_utf8( "PROFIL" );
	debug_button->get_text_element().set_size( 24 );
	debug_button->set_auto_resize( VectorTextLabel2D::AutoResizeModes::FULL );

	debug_button->set_click_handler( [this]( const RegionalKeyEvent&, BaseButton& ) -> bool
	{
		LogicEngine::show_performance_overlay();
		return true;
	} );

	debug_button->show();

	// add switch Font Button
	switch_font_button->get_area().set_vertical_anchor( VerticalAnchorType::TOP, *gpu_cycle_label, VerticalAnchorType::BOTTOM, -5 );
	switch_font_button->get_area().set_left_anchor( get_screen_root(), 5 );
	switch_font_button->get_area().set_width( 100 );
	switch_font_button->get_area().set_height( 40 );
	switch_font_button->get_text_element().set_utf8( "SWITCH FONT" );
	switch_font_button->get_text_element().set_size( 24 );
	switch_font_button->set_auto_resize( VectorTextLabel2D::AutoResizeModes::FULL );

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

	performance_ui_base->set_top_anchor( *switch_font_button );
	performance_ui_base->set_bottom_anchor( *debug_button );
	performance_ui_base->set_left_anchor( *switch_font_button );
	performance_ui_base->set_right_anchor( *switch_font_button );

	performance_ui_base->add_branch( *debug_button );
	performance_ui_base->add_branch( *switch_font_button );

	performance_ui.set_regional_event_root( *performance_ui_base );
}

void noxcain::MineSweeperLevel::create_default_hud()
{
	//exit button;
	exit_button->get_area().set_top_anchor( get_screen_root(), -10.0 );
	exit_button->get_area().set_right_anchor( get_screen_root(), -10.0 );
	exit_button->set_auto_resize( VectorTextLabel2D::AutoResizeModes::NONE );
#ifndef __ANDROID__
	exit_button->get_area().set_size( 49, 49 );
	exit_button->get_text_element().set_font_id( 2 );
	exit_button->set_centered_icon( 50, 0xF335 );

	exit_button->set_down_handler( [this]( const RegionalKeyEvent&, BaseButton& ) -> bool
	{
		set_status( Status::FINISHED );
		return true;
	} );

	exit_button->show();
#endif

	//config button;
	config_button->get_area().set_top_anchor( get_screen_root(), -10.0 );
	config_button->get_area().set_horizontal_anchor( HorizontalAnchorType::RIGHT, *exit_button, HorizontalAnchorType::LEFT, -5.0 );
	config_button->set_auto_resize( VectorTextLabel2D::AutoResizeModes::NONE );
	config_button->get_area().set_size( 49, 49 );
	config_button->get_text_element().set_font_id( 2 );
	config_button->set_centered_icon( 35, 0xF111 );

	config_button->set_click_handler( [this]( const RegionalKeyEvent& event , BaseButton& button ) -> bool
	{
		constexpr UINT32 gear_symbole = 0xF111;
		constexpr UINT32 arrow_symbole = 0xF171;
		if( button.get_text_element().get_unicodes().front() == gear_symbole )
		{
			display_settings();
			button.set_centered_icon( 35, arrow_symbole );
		}
		else
		{
			clear_user_interfaces();
			add_user_interface( performance_ui );
			add_user_interface( default_ui );
			button.set_centered_icon( 35, gear_symbole );
		}
		return true;
	});
	config_button->show();

	default_ui_base->set_bottom_anchor( *config_button );
	default_ui_base->set_top_anchor( *config_button );
	default_ui_base->set_left_anchor( *config_button );
	default_ui_base->set_right_anchor( *exit_button );

	default_ui_base->add_branch( *config_button );
	default_ui_base->add_branch( *exit_button );

	default_ui.set_regional_event_root( *default_ui_base );
}

void noxcain::MineSweeperLevel::create_settings_hud()
{
	DOUBLE main_size = 24;

	sampling_description_label->get_area().set_top_anchor( get_screen_root(), -20 );
	sampling_description_label->get_area().set_left_anchor( get_screen_root(), 20 );
	sampling_description_label->get_text_element().set_size( main_size );
	sampling_description_label->get_text_element().set_utf8( "ANTI-ALIASING" );
	sampling_description_label->show();

	sampling_decrease_button->get_area().set_bottom_anchor( *sampling_description_label );
	sampling_decrease_button->get_area().set_top_anchor( *sampling_description_label );
	sampling_decrease_button->get_area().set_horizontal_anchor( HorizontalAnchorType::LEFT, *sampling_description_label, HorizontalAnchorType::RIGHT, 0.5*main_size );
	sampling_decrease_button->set_auto_resize( VectorTextLabel2D::AutoResizeModes::NONE );
	sampling_decrease_button->get_area().set_width( [this]()
	{
		return sampling_decrease_button->get_area().get_height();
	} );
	sampling_decrease_button->set_centered_icon( 24, 0x2212 );
	sampling_decrease_button->show();
	sampling_decrease_button->set_click_handler( [this]( const RegionalKeyEvent& regional_event, BaseButton& event_reciever ) -> bool
	{
		auto settings = LogicEngine::get_graphic_settings();
		UINT32 new_count = settings.get_sample_count();

		if( settings.get_sample_count() > 1 )
		{
			new_count = new_count/2;
		}

		if( new_count <= 1 )
		{
			event_reciever.deactivate();
		}

		if( new_count < settings.get_max_sample_count() )
		{
			sampling_increase_button->activate();
		}
		sampling_description_value->get_text_element().set_utf8( std::to_string( new_count ) + "x" );
		LogicEngine::set_sample_count( new_count );
		return true;
	} );

	sampling_description_value->get_area().set_bottom_anchor( *sampling_decrease_button );
	sampling_description_value->get_area().set_top_anchor( *sampling_decrease_button );
	sampling_description_value->get_area().set_horizontal_anchor( HorizontalAnchorType::LEFT, *sampling_decrease_button, HorizontalAnchorType::RIGHT, 0.5*main_size );
	sampling_description_value->get_area().set_width( 2.0 * main_size * 1.4 );
	sampling_description_value->set_background_color( 0.2, 0.2, 0.2, 1.0 );
	sampling_description_value->get_text_element().set_size( main_size );
	sampling_description_value->set_text_alignment( VectorTextLabel2D::HorizontalTextAlignments::CENTER );
	sampling_description_value->set_auto_resize( VectorTextLabel2D::AutoResizeModes::NONE );
	
	sampling_description_value->show();

	sampling_increase_button->get_area().set_bottom_anchor( *sampling_description_label );
	sampling_increase_button->get_area().set_horizontal_anchor( HorizontalAnchorType::LEFT, *sampling_description_value, HorizontalAnchorType::RIGHT, 0.5*main_size );
	sampling_increase_button->set_auto_resize( VectorTextLabel2D::AutoResizeModes::NONE );
	sampling_increase_button->get_area().set_size( main_size * 1.4, main_size * 1.4 );
	sampling_increase_button->set_centered_icon( 24, 0x2B );
	sampling_increase_button->show();
	sampling_increase_button->set_click_handler( [this]( const RegionalKeyEvent& regional_event, BaseButton& event_reciever ) -> bool
	{
		auto settings = LogicEngine::get_graphic_settings();
		UINT32 new_count = settings.get_sample_count();

		if( settings.get_sample_count() < settings.get_max_sample_count() )
		{
			new_count = 2*new_count;
		}

		if( new_count >= settings.get_max_sample_count() )
		{
			event_reciever.deactivate();
		}

		if( new_count > 1 )
		{
			sampling_decrease_button->activate();
		}
		sampling_description_value->get_text_element().set_utf8( std::to_string( new_count ) + "x" );
		LogicEngine::set_sample_count( new_count );
		return true;
	} );

	settings_background->set_left_anchor( *sampling_description_label );
	settings_background->set_right_anchor( *sampling_increase_button );
	settings_background->set_top_anchor( *sampling_description_label );
	settings_background->set_bottom_anchor( *sampling_description_label );

	settings_background->add_branch( *sampling_decrease_button );
	settings_background->add_branch( *sampling_increase_button );

	settings_ui.set_regional_event_root( *settings_background );
}

void noxcain::MineSweeperLevel::display_settings()
{
	clear_user_interfaces();
	add_user_interface( settings_ui );
	add_user_interface( default_ui );

	const auto settings = LogicEngine::get_graphic_settings();
	sampling_description_value->get_text_element().set_utf8( std::to_string( settings.get_sample_count() ) + "x" );

	if( settings.get_sample_count() <= 1 ) sampling_decrease_button->deactivate();
	else sampling_decrease_button->activate();

	if( settings.get_sample_count() >= settings.get_max_sample_count() ) sampling_increase_button->deactivate();
	else sampling_increase_button->activate();
}

noxcain::MineSweeperLevel::MineSweeperLevel() :
	//fieldOrientationSpline( std::make_unique<CubicSpline>() ),
	performance_ui_base( std::make_unique<PassivRecieverNode>() ),
	cpu_cycle_label( std::make_unique<VectorText2D>( performance_ui.get_texts() ) ),
	gpu_cycle_label( std::make_unique<VectorText2D>( performance_ui.get_texts() ) ),
	debug_button( std::make_unique<BaseButton>( performance_ui ) ),
	switch_font_button( std::make_unique<BaseButton>( performance_ui ) ),
	
	default_ui_base( std::make_unique<PassivRecieverNode>() ),
	exit_button( std::make_unique<BaseButton>( default_ui ) ),
	config_button( std::make_unique<BaseButton>( default_ui ) ),

	settings_background( std::make_unique<PassivRecieverNode>() ),
	sampling_description_label( std::make_unique<VectorTextLabel2D>( settings_ui ) ),
	sampling_description_value( std::make_unique<VectorTextLabel2D>( settings_ui ) ),
	sampling_decrease_button( std::make_unique<BaseButton>( settings_ui ) ),
	sampling_increase_button( std::make_unique<BaseButton>( settings_ui ) )
{
	time_collector.name_collection( "Level logic" );

	vector_decal_renderables.emplace_back( vector_dacal_list );
	geometry_renderables.emplace_back( geometry_list );

	setup_events();
	grid = std::make_unique<SceneGraphNode>();
	get_scene_root().add_branch( *grid );

	setup_level();

	create_default_hud();
	create_performance_hud();
	create_settings_hud();

	add_user_interface( default_ui );
	add_user_interface( performance_ui );
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

	//hex_fields.emplace_back( std::make_unique<HexField>( *this ) );
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
	
	DOUBLE start_left = -5;
	DOUBLE start_top = -5;

	for( std::size_t row_index = 0; row_index < 5; ++row_index )
	{
		for( std::size_t column_index = 0; column_index < 5; ++column_index )
		{
			hex_fields.emplace_back( std::make_unique<HexField>( *this ) );
			auto& current_hex = hex_fields.back();

			const auto& hex_bounding_box = HexField::get_object_space_bounding_box();
			NxMatrix4x4 position;
			position.translation(
				{
					start_left + column_index * 0.75* hex_bounding_box.get_width(),
					start_top + row_index * hex_bounding_box.get_height() + ( column_index%2 ? 0.0 : 0.5*hex_bounding_box.get_height() ),
					0
				} );

			current_hex->set_local_matrix( position );
			//current_hex->set_local_matrix( centering * trans * NxMatrix4x4().rotation( { 1,0,0 }, -0.5*PI + lineAngle*(index + ( line % 2 ) * 0.5 - 1 ) ) * NxMatrix4x4().translation( { 0,0,realRadius + 0.5 } ) );
			current_hex->set_mine_number( count++ );
		}
	}
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

	check_events( deltaTime );
	update_camera();

	DOUBLE delta = std::chrono::duration_cast<std::chrono::duration<DOUBLE>>( deltaTime ).count();

	NxMatrix4x4 direction_change;
	direction_change.rotationZ( 2.0*delta * ( std::rand()%2 ? -1.0 : 1.0 ) );
	NxMatrix4x4 dummy;
	dummy[0] = last_direction[0];
	dummy[1] = last_direction[1];
	dummy[2] = last_direction[2];
	dummy = dummy * direction_change;
	last_direction[0] = dummy[0];
	last_direction[1] = dummy[1];
	last_direction[2] = dummy[2];
	last_direction.normalize();

	last_position = last_position + 5.0 * delta * last_direction;

	for( auto& hex : hex_fields )
	{
		auto matrix = hex->get_local_matrix();
		
		NxVector3D location( matrix[12], matrix[13], 0 );
		matrix[14] = std::max( 0.0, 30.0 - ( location - last_position ).get_length() ) / 30.0 * 7;
		hex->set_local_matrix( matrix );
	}

	if( last_position[0] < -50 || last_position[0] > 50 ||last_position[1] < -30 || last_position[1] > 30 )
	{
		last_direction = -last_direction;
	}
}

