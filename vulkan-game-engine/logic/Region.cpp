#include "Region.hpp"

noxcain::Region::Region( const Region& target, DOUBLE width, DOUBLE height,
					   std::pair<HorizontalAnchorType, VerticalAnchorType> ownAnchorRef, std::pair<HorizontalAnchorType, VerticalAnchorType> targetAnchorRef, std::pair<DOUBLE, DOUBLE> offset ):
	bottom( *this, AnchorType::OPPOSITE_START ), top( *this, AnchorType::OPPOSITE_END ), left( *this, AnchorType::OPPOSITE_START ), right( *this, AnchorType::OPPOSITE_END ),
	width( width ), height( height )
{
	set_horizontal_anchor( ownAnchorRef.first, target, targetAnchorRef.first, offset.first );
	set_vertical_anchor( ownAnchorRef.second, target, targetAnchorRef.second, offset.second );
}

noxcain::Region::AnchorType noxcain::Region::get_anchor_type( HorizontalAnchorType type )
{
	switch( type )
	{
		case noxcain::HorizontalAnchorType::LEFT:
			return AnchorType::START;
		case noxcain::HorizontalAnchorType::RIGHT:
			return AnchorType::END;
		default:
			return AnchorType::CENTER;
	}
}

noxcain::Region::AnchorType noxcain::Region::get_anchor_type( VerticalAnchorType type )
{
	switch( type )
	{
		case noxcain::VerticalAnchorType::BOTTOM:
			return AnchorType::START;
		case noxcain::VerticalAnchorType::TOP:
			return AnchorType::END;
		default:
			return AnchorType::CENTER;
	}
}

noxcain::DOUBLE noxcain::Region::get_top() const
{
	return top.get_value() + offset_top;
}

noxcain::DOUBLE noxcain::Region::get_bottom() const
{
	return bottom.get_value() + offset_bottom - ( is_vertical_centered ? 0.5 * height : 0 );
}

noxcain::DOUBLE noxcain::Region::get_left() const
{
	return left.get_value() + offset_left - ( is_horizontal_centered ? 0.5 * width : 0 );
}

noxcain::DOUBLE noxcain::Region::get_right() const
{
	return right.get_value() + offset_right;
}

noxcain::DOUBLE noxcain::Region::VerticalAnchor::get_value() const
{
	switch( type )
	{
		case noxcain::Region::AnchorType::START:
		{
			return parent->get_bottom();
		}
		case noxcain::Region::AnchorType::END:
		{
			return parent->get_top();
		}
		case noxcain::Region::AnchorType::OPPOSITE_START:
		{
			return parent->get_top() - parent->height;
		}
		case noxcain::Region::AnchorType::OPPOSITE_END:
		{
			return parent->get_bottom() + parent->height;
		}
		case noxcain::Region::AnchorType::CENTER:
		{
			return parent->get_bottom() + 0.5F * parent->get_height();
		}
		default:
		{
			return 0;
		}
	}
}

noxcain::DOUBLE noxcain::Region::HorizontalAnchor::get_value() const
{
	switch( type )
	{
		case noxcain::Region::AnchorType::START:
		{
			return parent->get_left();
		}
		case noxcain::Region::AnchorType::END:
		{
			return parent->get_right();
		}
		case noxcain::Region::AnchorType::OPPOSITE_START:
		{
			return parent->get_right() - parent->width;
		}
		case noxcain::Region::AnchorType::OPPOSITE_END:
		{
			return parent->get_left() + parent->width;
		}
		case noxcain::Region::AnchorType::CENTER:
		{
			return parent->get_left() + 0.5F * parent->get_width();
		}
		default:
		{
			return 0;
		}
	}
}

noxcain::Region::Region() : left(  *this, AnchorType::ZERO ), right( *this, AnchorType::OPPOSITE_END ), bottom( *this, AnchorType::ZERO ), top( *this, AnchorType::OPPOSITE_END )
{
}

void noxcain::Region::set_size( const DynamicValue& new_width, const DynamicValue& new_height )
{
	width = new_width;
	height = new_height;
}

void noxcain::Region::set_width( const DynamicValue& new_width )
{
	width = new_width;
}

void noxcain::Region::set_height( const DynamicValue& new_height )
{
	height = new_height;
}

void noxcain::Region::set_width( const std::function<DOUBLE()>& width_getter )
{
	width = width_getter;
}

void noxcain::Region::set_height( const std::function<DOUBLE()>& height_getter )
{
	height = height_getter;
}

void noxcain::Region::set_top_anchor( const Region& target, const DynamicValue& offset )
{
	set_vertical_anchor( VerticalAnchorType::TOP, target, VerticalAnchorType::TOP, offset );
}

void noxcain::Region::set_bottom_anchor( const Region& target, const DynamicValue& offset )
{
	set_vertical_anchor( VerticalAnchorType::BOTTOM, target, VerticalAnchorType::BOTTOM, offset );
}

void noxcain::Region::set_left_anchor( const Region& target, const DynamicValue& offset )
{
	set_horizontal_anchor( HorizontalAnchorType::LEFT, target, HorizontalAnchorType::LEFT, offset );
}

void noxcain::Region::set_right_anchor( const Region& target, const DynamicValue& offset )
{
	set_horizontal_anchor( HorizontalAnchorType::RIGHT, target, HorizontalAnchorType::RIGHT, offset );
}

void noxcain::Region::set_anchor( const Region& target,
								 HorizontalAnchorType own_horizontal_anchor,
								 VerticalAnchorType own_vertical_anchor,
								 HorizontalAnchorType target_horizontal_anchor,
								 VerticalAnchorType target_vertical_anchor,
								  const DynamicValue& x_offset, const DynamicValue& y_offset )
{
	set_horizontal_anchor( own_horizontal_anchor, target, target_horizontal_anchor, x_offset );
	set_vertical_anchor( own_vertical_anchor, target, target_vertical_anchor, y_offset );
}

void noxcain::Region::set_anchor( const Region& target, HorizontalAnchorType horizontal_anchor, VerticalAnchorType vertical_anchor, const DynamicValue& x_offset, const DynamicValue& y_offset )
{
	set_horizontal_anchor( horizontal_anchor, target, horizontal_anchor, x_offset );
	set_vertical_anchor( vertical_anchor, target, vertical_anchor, y_offset );
}

void noxcain::Region::set_same_region( const Region& target, const DynamicValue& x_offset, const DynamicValue& y_offset )
{
	set_top_anchor( target, -y_offset );
	set_bottom_anchor( target, y_offset );
	set_left_anchor( target, x_offset );
	set_right_anchor( target, -x_offset );
}

bool noxcain::Region::check_region( INT32 x, INT32 y ) const
{
	return get_bottom() <= y && y < get_top() && get_left() <= x && x < get_right();
}

void noxcain::Region::set_vertical_anchor( VerticalAnchorType ownAnchorRef, const Region& target, VerticalAnchorType targetAnchorRef, const DynamicValue& offset )
{
	switch( ownAnchorRef )
	{
		case VerticalAnchorType::BOTTOM:
		{
			is_vertical_centered = false;
			bottom = VerticalAnchor( target, get_anchor_type( targetAnchorRef ) );
			offset_bottom = offset;
			if( top.get_type() == AnchorType::ZERO ) top = VerticalAnchor( *this, AnchorType::OPPOSITE_END );
			break;
		}
		case VerticalAnchorType::TOP:
		{
			is_vertical_centered = false;
			top = VerticalAnchor( target, get_anchor_type( targetAnchorRef ) );
			offset_top = offset;
			if( bottom.get_type() == AnchorType::ZERO ) bottom = VerticalAnchor( *this, AnchorType::OPPOSITE_START );
			break;
		}
		default:
		{
			is_vertical_centered = true;
			bottom = VerticalAnchor( target, get_anchor_type( targetAnchorRef ) );
			top = VerticalAnchor( *this, AnchorType::OPPOSITE_END );
			offset_bottom = offset;
			offset_top = DynamicValue( 0 );
			break;
		}
	}
}

void noxcain::Region::set_horizontal_anchor( HorizontalAnchorType ownAnchorRef, const Region& target, HorizontalAnchorType targetAnchorRef, const DynamicValue& offset )
{
	switch( ownAnchorRef )
	{
		case HorizontalAnchorType::LEFT:
		{
			is_horizontal_centered = false;
			left = HorizontalAnchor( target, get_anchor_type( targetAnchorRef ) );
			offset_left = offset;
			if( right.get_type() == AnchorType::ZERO ) right = HorizontalAnchor( *this, AnchorType::OPPOSITE_END );
			break;
		}
		case HorizontalAnchorType::RIGHT:
		{
			is_horizontal_centered = false;
			right = HorizontalAnchor( target, get_anchor_type( targetAnchorRef ) );
			offset_right = offset;
			if( left.get_type() == AnchorType::ZERO ) left = HorizontalAnchor( *this, AnchorType::OPPOSITE_START );
			break;
		}
		default:
		{
			is_horizontal_centered = true;
			left = HorizontalAnchor( target, get_anchor_type( targetAnchorRef ) );
			right = HorizontalAnchor( *this, AnchorType::OPPOSITE_END );
			is_horizontal_centered = true;
			offset_left = offset;
			offset_right = DynamicValue( 0 );
			break;
		}
	}
}

noxcain::DOUBLE noxcain::Region::DynamicValue::get() const
{
	if( value_getter )
	{
		return sign*value_getter();
	}
	return sign*fallback_value;
}

noxcain::Region::DynamicValue& noxcain::Region::DynamicValue::operator=( DOUBLE fixed_value )
{
	fallback_value = fixed_value;
	value_getter = std::function<DOUBLE()>();
	return *this;
}

noxcain::Region::DynamicValue& noxcain::Region::DynamicValue::operator=( const std::function<DOUBLE()>& new_getter )
{
	value_getter = new_getter;
	return *this;
}

noxcain::Region::DynamicValue noxcain::Region::DynamicValue::operator-() const
{
	DynamicValue new_value = *this;
	new_value.sign = -new_value.sign;
	return new_value;
}

noxcain::Region::DynamicValue::DynamicValue( DOUBLE fixed_value ) : fallback_value( fixed_value )
{

}

noxcain::Region::DynamicValue::DynamicValue( const std::function<DOUBLE()>& dynamic_value_getter ) : value_getter( dynamic_value_getter ), fallback_value(1.0)
{
}

noxcain::Region::DynamicValue::operator noxcain::DOUBLE() const
{
	return get();
}
