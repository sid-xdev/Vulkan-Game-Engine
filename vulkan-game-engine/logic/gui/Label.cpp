#include "Label.hpp"
#include <cmath>

noxcain::VectorTextLabel2D::VectorTextLabel2D( RenderableList<VectorText2D>& text_list, RenderableList<RenderableQuad2D>& label_list ) :
	text_content( text_list ), background( label_list ), frame( label_list )
{
	std::function<DOUBLE()> size_getter = [this]() ->DOUBLE
	{
		return frame_size;
	};
	background.set_same_region( frame, size_getter, size_getter );
	
	set_text_alignment( VerticalTextAlignments::BOTTOM );
	set_text_alignment( HorizontalTextAlignments::LEFT );
}

void noxcain::VectorTextLabel2D::set_depth_level( UINT32 level )
{
	frame.set_depth_level( level );
	background.set_depth_level( level + 1 );
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
	if( frame.get_color()[3] ) frame.show();
	if( text_content.get_text().get_color()[3] ) text_content.show();
	is_hidden = false;
}
