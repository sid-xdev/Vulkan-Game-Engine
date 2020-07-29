#include "ScrollBar.hpp"

#include <algorithm>
#include <cmath>

noxcain::DOUBLE noxcain::ScrollBarBase::get_slider_working_area_length() const
{
	return get_active_bar_length() - get_total_slider_length();
}

void noxcain::ScrollBarBase::set_slider_size( DOUBLE size )
{
	slider_length = std::clamp( size, min_slider_width, 1.0 );
}

void noxcain::ScrollBarBase::set_top( const Region& anchor, DOUBLE offset )
{
	bar.set_top_anchor( anchor, offset );
}

void noxcain::ScrollBarBase::set_bottom( const Region& anchor, DOUBLE offset )
{
	bar.set_bottom_anchor( anchor, offset );
}

void noxcain::ScrollBarBase::set_left( const Region& anchor, DOUBLE offset )
{
	bar.set_left_anchor( anchor, offset );
}

void noxcain::ScrollBarBase::set_right( const Region& anchor, DOUBLE offset )
{
	bar.set_right_anchor( anchor, offset );
}

void noxcain::ScrollBarBase::set_bar_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha )
{
	bar.set_color( red, green, blue, alpha );
}

void noxcain::ScrollBarBase::set_slider_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha )
{
	slider.set_background_color( red, green, blue, alpha );
	scale_button_left_top.set_background_color( red, green, blue, alpha );
	scale_button_right_bottom.set_background_color( red, green, blue, alpha );
}

void noxcain::ScrollBarBase::set_slider_highlight_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha )
{
	slider.set_highlight_color( red, green, blue, alpha );
	scale_button_left_top.set_highlight_color( red, green, blue, alpha );
	scale_button_right_bottom.set_highlight_color( red, green, blue, alpha );
}

void noxcain::ScrollBarBase::set_slider_drag_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha )
{
	slider.set_active_color( red, green, blue, alpha );
	scale_button_left_top.set_active_color( red, green, blue, alpha );
	scale_button_right_bottom.set_active_color( red, green, blue, alpha );
}

void noxcain::ScrollBarBase::deactivate()
{
	slider.hide();
	scale_button_left_top.hide();
	scale_button_right_bottom.hide();

	slider.set_over_handler();
	scale_button_left_top.set_over_handler();
	scale_button_right_bottom.set_over_handler();

	drag_tracer.remove();
	RegionalEventRecieverNode::deactivate();
}

void noxcain::ScrollBarBase::activate()
{
	slider.show();
	scale_button_left_top.show();
	scale_button_right_bottom.show();
	RegionalEventRecieverNode::activate();
}

void noxcain::ScrollBarBase::hide()
{
	bar.hide();
	deactivate();
}

void noxcain::ScrollBarBase::show()
{
	bar.show();
	activate();
}

bool noxcain::ScrollBarBase::is_hidden() const
{
	return bar.is_hidden();
}

bool noxcain::ScrollBarBase::is_shown() const
{
	return bar.is_shown();
}

bool noxcain::ScrollBarBase::is_activ() const
{
	return RegionalEventRecieverNode::active;
}

void noxcain::ScrollBarBase::set_position( DOUBLE pos )
{
	slider_position = std::clamp( pos, 0.0, 1.0 );
}

void noxcain::ScrollBarBase::set_depth_level( UINT32 level )
{
	bar.set_depth_level( level );
	slider.set_depth_level( level + 1 );
	scale_button_left_top.set_depth_level( level + 1 );
	scale_button_right_bottom.set_depth_level( level + 1 );
}

noxcain::ScrollBarBase::operator const noxcain::Region& ( ) const
{
	return bar;
}

noxcain::ScrollBarBase::operator noxcain::Region& ( )
{
	return bar;
}

void noxcain::ScrollBarBase::make_scalable( DOUBLE relative_min_slider_size )
{
	min_slider_width = std::clamp( relative_min_slider_size, 0.0, 1.0 );
	
	scale_button_left_top.show();
	scale_button_right_bottom.show();

	slider_region.add_branch( scale_button_left_top );
	slider_region.add_branch( scale_button_right_bottom );
}

void noxcain::ScrollBarBase::fix_scale()
{
	scale_button_left_top.hide();
	scale_button_right_bottom.hide();

	scale_button_left_top.cut_branch();
	scale_button_right_bottom.cut_branch();

	slider.get_area().set_same_region( slider_region );
}

noxcain::ScrollBarBase::ScrollBarBase( DOUBLE frame_size, GameUserInterface& ui, RegionalEventExclusivTracer& tracer ): bar_endcap_size( frame_size ),
	bar( ui.labels ), slider( ui ), scale_button_left_top( ui ), scale_button_right_bottom( ui ), drag_tracer( tracer )
{
	scale_button_left_top.get_area().set_top_anchor( slider_region );
	scale_button_left_top.get_area().set_left_anchor( slider_region );

	scale_button_right_bottom.get_area().set_right_anchor( slider_region );
	scale_button_right_bottom.get_area().set_bottom_anchor( slider_region );

	this->add_branch( slider_region );
	slider_region.add_branch( slider );
	set_depth_level( 0 );

	//dynamic slider position
	auto slider_position_getter = [this]() -> DOUBLE
	{
		return  bar_endcap_size + slider_position * get_slider_working_area_length();
	};
	slider_region.set_left_anchor( bar, Region::DynamicValue( slider_position_getter ) );

	// dynamic slider size
	std::function<DOUBLE()> slide_size_getter = [this]()
	{
		return slider_length * get_active_bar_length();
	};
	slider_region.set_width( slide_size_getter );

	auto disbale_handler = [this]( const RegionalKeyEvent& key_event, BaseButton& reciever ) -> bool
	{
		reciever.set_over_handler();
		drag_tracer.remove();
		return true;
	};

	slider.set_down_handler( [this]( const RegionalKeyEvent& key_event, BaseButton& reciever ) -> bool
	{
		if( key_event.get_key_code() == RegionalKeyEvent::KeyCodes::LEFT_MOUSE )
		{
			slider_move_start = get_event_value( key_event );
			drag_tracer.set_exclusiv( slider );
			reciever.set_over_handler( std::bind( &ScrollBarBase::move_slider, this, std::placeholders::_1, std::placeholders::_2 ) );
			return true;
		}
		return false;
	} );

	slider.set_up_handler( disbale_handler );

	// scale button events
	// handle left/top button
	scale_button_left_top.set_down_handler( [this]( const RegionalKeyEvent& key_event, BaseButton& reciever )
	{
		if( key_event.get_key_code() == RegionalKeyEvent::KeyCodes::LEFT_MOUSE )
		{
			slider_move_start = get_event_value( key_event );
			drag_tracer.set_exclusiv( reciever );
			reciever.set_over_handler( std::bind( &ScrollBarBase::rescale_slider, this, -1.0, std::placeholders::_1, std::placeholders::_2 ) );
			return true;
		}
		return false;
	} );
	scale_button_left_top.set_up_handler( disbale_handler );

	// handle right/bottom button
	scale_button_right_bottom.set_down_handler( [this]( const RegionalKeyEvent& key_event, BaseButton& reciever )
	{
		if( key_event.get_key_code() == RegionalKeyEvent::KeyCodes::LEFT_MOUSE )
		{
			slider_move_start = get_event_value( key_event );
			drag_tracer.set_exclusiv( reciever );
			reciever.set_over_handler( std::bind( &ScrollBarBase::rescale_slider, this, 1.0, std::placeholders::_1, std::placeholders::_2 ) );
			return true;
		}
		return false;
	} );
	scale_button_right_bottom.set_up_handler( disbale_handler );

	// non scalable per default
	fix_scale();
	hide();
}

bool noxcain::ScrollBarBase::move_slider( const RegionalKeyEvent& key_event, BaseButton& reciever )
{
	auto width = get_slider_working_area_length();
	if( width >= MAGIC_NUMBER )
	{
		DOUBLE move_value = get_event_value( key_event );
		DOUBLE move_delta = move_value - slider_move_start;
		slider_move_start = move_value;
		set_position( slider_position + ( move_delta / width ) );
	}
	else
	{
		set_position( 0 );
	}
	return true;
}

bool noxcain::ScrollBarBase::rescale_slider( DOUBLE direction, const RegionalKeyEvent& key_event, BaseButton& reciever )
{
	DOUBLE bar_length = get_active_bar_length();
	if( bar_length > 0 )
	{
		DOUBLE move_value = get_event_value( key_event );
		DOUBLE delta = direction * ( move_value - slider_move_start );
		slider_move_start = move_value;

		DOUBLE new_pixel_position = get_active_bar_length() * slider_position * ( 1.0-slider_length ) - delta;

		slider_move_start = move_value;
		bool is_growing = delta > 0;
		DOUBLE old_length = slider_length;
		if( is_growing && ( slider_position <= MAGIC_NUMBER || slider_position >= ( 1.0 - MAGIC_NUMBER ) ) )
		{
			set_slider_size( slider_length + delta/bar_length );
		}
		else
		{
			set_slider_size( slider_length + 2*delta/bar_length );
		}
		if( old_length != slider_length )
		{
			DOUBLE move_area_length = get_slider_working_area_length();
			if( move_area_length > 0 )
			{
				set_position( new_pixel_position/get_slider_working_area_length() );
			}
			else
			{
				set_position( 0 );
			}
		}
	}
	return true;
}

noxcain::VerticalScrollBar::VerticalScrollBar( DOUBLE frame_size, GameUserInterface& ui, RegionalEventExclusivTracer& tracer ):
	ScrollBarBase( frame_size, ui, tracer )
{
	scale_button_left_top.get_area().set_right_anchor( slider_region );
	scale_button_right_bottom.get_area().set_left_anchor( slider_region );

	auto width_getter = [this]()
	{
		return scale_button_left_top.get_area().get_width();
	};
	scale_button_left_top.get_area().set_height( width_getter );
	scale_button_right_bottom.get_area().set_height( width_getter );

	slider_region.set_left_anchor( bar, -bar_endcap_size );
	slider_region.set_right_anchor( bar, bar_endcap_size );
}

void noxcain::VerticalScrollBar::make_scalable( DOUBLE relative_min_slider_size )
{
	ScrollBarBase::make_scalable( relative_min_slider_size );

	DOUBLE distance = fmax( 1.0, bar_endcap_size );

	slider.get_area().set_vertical_anchor( VerticalAnchorType::TOP, scale_button_left_top, VerticalAnchorType::BOTTOM, distance );
	slider.get_area().set_vertical_anchor( VerticalAnchorType::BOTTOM, scale_button_right_bottom, VerticalAnchorType::TOP, -distance );
}

noxcain::DOUBLE noxcain::VerticalScrollBar::get_total_slider_length() const
{
	return slider_region.get_height();
}

noxcain::DOUBLE noxcain::VerticalScrollBar::get_active_bar_length() const
{
	return bar.get_height() - 2*bar_endcap_size;
}

noxcain::DOUBLE noxcain::VerticalScrollBar::get_event_value( const RegionalKeyEvent& event ) const
{
	return event.get_y_position();
}

noxcain::HorizontalScrollBar::HorizontalScrollBar( DOUBLE frame_size, GameUserInterface& ui, RegionalEventExclusivTracer& tracer ) :
	ScrollBarBase( frame_size, ui, tracer )
{
	scale_button_left_top.get_area().set_bottom_anchor( slider_region );
	scale_button_right_bottom.get_area().set_top_anchor( slider_region );

	auto width_getter = [this]()
	{
		return scale_button_left_top.get_area().get_height();
	};
	scale_button_left_top.get_area().set_width( width_getter );
	scale_button_right_bottom.get_area().set_width( width_getter );

	slider_region.set_top_anchor( bar, -bar_endcap_size );
	slider_region.set_bottom_anchor( bar, bar_endcap_size );
}

void noxcain::HorizontalScrollBar::make_scalable( DOUBLE relative_min_slider_size )
{
	ScrollBarBase::make_scalable( relative_min_slider_size );

	DOUBLE distance = fmax( 1.0, bar_endcap_size );

	slider.get_area().set_horizontal_anchor( HorizontalAnchorType::LEFT, scale_button_left_top, HorizontalAnchorType::RIGHT, distance );
	slider.get_area().set_horizontal_anchor( HorizontalAnchorType::RIGHT, scale_button_right_bottom, HorizontalAnchorType::LEFT, -distance );
}

noxcain::HorizontalScrollBar::~HorizontalScrollBar()
{
}

noxcain::DOUBLE noxcain::HorizontalScrollBar::get_total_slider_length() const
{
	return slider_region.get_width();
}

noxcain::DOUBLE noxcain::HorizontalScrollBar::get_active_bar_length() const
{
	return bar.get_width() - 2*bar_endcap_size;
}

noxcain::DOUBLE noxcain::HorizontalScrollBar::get_event_value( const RegionalKeyEvent& event ) const
{
	return event.get_x_position();
}