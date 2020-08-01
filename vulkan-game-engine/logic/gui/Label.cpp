#include "Label.hpp"

#include <resources/FontResource.hpp>
#include <resources/BoundingBox.hpp>
#include <cmath>

noxcain::VectorTextLabel2D::VectorTextLabel2D( GameUserInterface& ui ) :
	text_content( ui.texts ), background( ui.labels ), frame( ui.labels )
{
	std::function<DOUBLE()> size_getter = [this]() ->DOUBLE
	{
		return frame_size;
	};
	background.set_same_region( frame, size_getter, size_getter );
	
	set_text_alignment( VerticalTextAlignments::BOTTOM );
	set_text_alignment( HorizontalTextAlignments::LEFT );
	set_auto_resize( AutoResizeModes::FULL );
}

void noxcain::VectorTextLabel2D::set_depth_level( UINT32 level )
{
	frame.set_depth_level( level );
	background.set_depth_level( level + 1 );
	text_content.set_depth_level( level + 1 );
}

void noxcain::VectorTextLabel2D::set_auto_resize( AutoResizeModes mode )
{
	switch( mode )
	{
		case noxcain::VectorTextLabel2D::AutoResizeModes::FULL:
		{
			frame.set_width( [this]()
			{
				return text_content.get_width() + 2 + 2*frame_size;
			} );

			frame.set_height( [this]()
			{
				return text_content.get_height() + 2 + 2*frame_size;
			} );
			break;
		}
		case noxcain::VectorTextLabel2D::AutoResizeModes::WIDTH:
		{
			frame.set_width( [this]()
			{
				return text_content.get_width() + 2 + 2*frame_size;
			} );

			frame.set_height( std::function<DOUBLE()>() );

			break;
		}
		case noxcain::VectorTextLabel2D::AutoResizeModes::HEIGHT:
		{
			frame.set_height( [this]()
			{
				return text_content.get_height() + 2 + 2*frame_size;
			} );
			frame.set_width( std::function<DOUBLE()>() );
			break;
		}
		case noxcain::VectorTextLabel2D::AutoResizeModes::NONE:
		{
			frame.set_height( std::function<DOUBLE()>() );
			frame.set_width( std::function<DOUBLE()>() );
			break;
		}
		default:
			break;
	}
}

void noxcain::VectorTextLabel2D::set_text_alignment( HorizontalTextAlignments alignment )
{
	switch( alignment )
	{
		case noxcain::VectorTextLabel2D::HorizontalTextAlignments::LEFT:
		{
			text_content.set_left_anchor( background, 1 );
			text_content.get_text().set_text_alignment( VectorText::Alignments::LEFT );
			break;
		}
		case noxcain::VectorTextLabel2D::HorizontalTextAlignments::RIGHT:
		{
			text_content.set_right_anchor( background, -1 );
			text_content.get_text().set_text_alignment( VectorText::Alignments::RIGHT );
			break;
		}
		case noxcain::VectorTextLabel2D::HorizontalTextAlignments::CENTER:
		{
			text_content.set_horizontal_anchor( HorizontalAnchorType::CENTER, background, HorizontalAnchorType::CENTER );
			text_content.get_text().set_text_alignment( VectorText::Alignments::CENTER );
			break;
		}
		default:
			break;
	}
}

void noxcain::VectorTextLabel2D::set_text_alignment( VerticalTextAlignments alignment )
{
	switch( alignment )
	{
		case noxcain::VectorTextLabel2D::VerticalTextAlignments::TOP:
		{
			text_content.set_top_anchor( background, -1 );
			break;
		}
		case noxcain::VectorTextLabel2D::VerticalTextAlignments::BOTTOM:
		{
			text_content.set_bottom_anchor( background, 1 );
			break;
		}
		case noxcain::VectorTextLabel2D::VerticalTextAlignments::CENTER:
		{
			text_content.set_vertical_anchor( VerticalAnchorType::CENTER, background, VerticalAnchorType::CENTER );
			break;
		}
		default:
			break;
	}
}

void noxcain::VectorTextLabel2D::set_frame_size( DOUBLE size )
{
	frame_size = fmax( 0.0, size );
	if( frame_size && !is_hidden )
	{
		frame.show();
	}
	else
	{
		frame.hide();
	}
}

void noxcain::VectorTextLabel2D::set_frame_color( DOUBLE red, DOUBLE green, DOUBLE blue, DOUBLE alpha )
{
	frame.set_color( red, blue, green, alpha );
	if( alpha && !is_hidden )
	{
		frame.show();
	}
	else
	{
		frame.hide();
	}
}

void noxcain::VectorTextLabel2D::set_background_color( DOUBLE red, DOUBLE green, DOUBLE blue, DOUBLE alpha )
{
	background.set_color( red, blue, green, alpha );
	if( alpha && !is_hidden )
	{
		background.show();
	}
	else
	{
		background.hide();
	}
}

void noxcain::VectorTextLabel2D::set_centered_icon( DOUBLE size, UINT32 unicode )
{
	text_content.get_text().set_size( size );
	text_content.get_text().set_unicodes( { unicode } );
	text_content.get_text().set_text_alignment( VectorText::Alignments::LEFT );

	std::function get_x_offset = [this]()
	{
		const auto unicodes = get_text_element().get_unicodes();
		if( unicodes.size() )
		{
			const DOUBLE font_size = get_text_element().get_size();
			const BoundingBox icon_bounding_box = get_text_element().get_font().get_character_bounding_box( unicodes.front() );
			return -font_size*icon_bounding_box.get_left() + 0.5*( background.get_width() - font_size*icon_bounding_box.get_width() );
		}
		return 0.0;
	};
	text_content.set_left_anchor( background, get_x_offset );

	std::function get_y_offset = [this]()
	{
		const auto unicodes = get_text_element().get_unicodes();
		if( unicodes.size() )
		{
			const DOUBLE font_size = get_text_element().get_size();
			const auto& font = get_text_element().get_font();
			const BoundingBox icon_bounding_box = font.get_character_bounding_box( unicodes.front() );
			
			return -font_size*( icon_bounding_box.get_bottom() - font.get_descender() ) + 0.5*( background.get_height() - font_size*icon_bounding_box.get_height() );
		}
		return 0.0;
	};
	text_content.set_bottom_anchor( background, get_y_offset );
}

void noxcain::VectorTextLabel2D::hide()
{
	background.hide();
	frame.hide();
	text_content.hide();
	is_hidden = true;
}

void noxcain::VectorTextLabel2D::show()
{
	if( background.get_color()[3] ) background.show();
	if( frame_size && frame.get_color()[3] ) frame.show();
	if( text_content.get_text().get_color()[3] ) text_content.show();
	is_hidden = false;
}
