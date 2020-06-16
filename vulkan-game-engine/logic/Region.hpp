#pragma once
#include <Defines.hpp>

#include <logic/RegionEventReceiver.hpp>

#include <array>
#include <vector>
#include <string>

namespace noxcain
{
	enum class VerticalAnchorType
	{
		TOP,
		BOTTOM,
		CENTER,
	};

	enum class HorizontalAnchorType
	{
		LEFT,
		RIGHT,
		CENTER,
	};

	class Region
	{
	public:
		class DynamicValue
		{
		public:
			
			DynamicValue& operator=( DOUBLE fixed_value );
			DynamicValue& operator=( const std::function<DOUBLE()>& new_getter );
			DynamicValue& operator=( const DynamicValue& ) = default;
			DynamicValue operator-() const;
			DynamicValue() = default;
			DynamicValue( DOUBLE fixed_value );
			DynamicValue( const std::function<DOUBLE()>& dynamic_value_getter );

			operator DOUBLE() const;

		private:
			inline DOUBLE get() const;
			DOUBLE fallback_value = 0.0;
			DOUBLE sign = 1.0;
			std::function<DOUBLE()> value_getter;
		};

		Region( const Region& ) = default;
		Region( Region&& ) = default;
		Region();
		Region(
			const Region& target, DOUBLE width, DOUBLE height,
			std::pair<HorizontalAnchorType, VerticalAnchorType> ownAnchorRef = { HorizontalAnchorType::LEFT, VerticalAnchorType::BOTTOM },
			std::pair<HorizontalAnchorType, VerticalAnchorType> targetAnchorRef = { HorizontalAnchorType::LEFT, VerticalAnchorType::BOTTOM },
			std::pair<DOUBLE, DOUBLE> offset = { 0,0 } );

		void set_vertical_anchor( VerticalAnchorType ownAnchorRef, const Region& target, VerticalAnchorType targetAnchorRef = VerticalAnchorType::BOTTOM, const DynamicValue& offset = 0 );
		void set_horizontal_anchor( HorizontalAnchorType ownAnchorRef, const Region& target, HorizontalAnchorType targetAnchorRef = HorizontalAnchorType::LEFT, const DynamicValue& offset = 0 );

		void set_size( const DynamicValue& width, const DynamicValue& height );
		
		void set_width( const DynamicValue& width );
		void set_height( const DynamicValue& height );

		void set_width( const std::function<DOUBLE()>& width_getter );
		void set_height( const std::function<DOUBLE()>& height_getter );

		void set_top_anchor( const Region& target, const DynamicValue& offset = 0 );
		void set_bottom_anchor( const Region& target, const DynamicValue& offset = 0 );
		void set_left_anchor( const Region& target, const DynamicValue& offset = 0 );
		void set_right_anchor( const Region& target, const DynamicValue& offset = 0 );

		void set_anchor( const Region& target,
						 HorizontalAnchorType own_horizontal_anchor,
						 VerticalAnchorType own_vertical_anchor,
						 HorizontalAnchorType target_horizontal_anchor,
						 VerticalAnchorType target_vertical_anchor,
						 const DynamicValue& x_offset = 0, const DynamicValue& y_offset = 0 );

		void set_anchor( const Region& target,
						 HorizontalAnchorType own_horizontal_anchor,
						 VerticalAnchorType own_vertical_anchor,
						 const DynamicValue& x_offset = 0, const DynamicValue& y_offset = 0 );

		void set_same_region( const Region& target, const DynamicValue& x_offset = 0, const DynamicValue& y_offset = 0 );

		DOUBLE get_top() const;
		DOUBLE get_bottom() const;
		DOUBLE get_left() const;
		DOUBLE get_right() const;

		DOUBLE get_width() const
		{
			if( right.anchored() && left.anchored() )
			{
				return get_right() - get_left();
			}
			return width;
		}

		DOUBLE get_height() const
		{
			if( top.anchored() && bottom.anchored() )
			{
				return get_top() - get_bottom();
			}
			return height;
		}

		bool check_region( INT32 x, INT32 y ) const;

	protected:
		enum class AnchorType
		{
			ZERO,
			START,
			END,
			OPPOSITE_END,
			OPPOSITE_START,
			CENTER,
		};

		static AnchorType get_anchor_type( HorizontalAnchorType type );
		static AnchorType get_anchor_type( VerticalAnchorType type );

		class VerticalAnchor
		{
			AnchorType type;
			const Region* parent = nullptr;
		public:
			VerticalAnchor( const Region& anchorOwner, AnchorType type ) : type( type ), parent( &anchorOwner )
			{
			}
			bool anchored() const
			{
				return type < AnchorType::OPPOSITE_END;
			}
			DOUBLE get_value() const;
			AnchorType get_type() const
			{
				return type;
			}
		} bottom, top;

		class HorizontalAnchor
		{
			AnchorType type;
			const Region* parent = nullptr;
			bool is_anchored = false;
		public:
			HorizontalAnchor( const Region& anchorOwner, AnchorType type ) : type( type ), parent( &anchorOwner ), is_anchored( type < AnchorType::OPPOSITE_END )
			{
			}
			bool anchored() const
			{
				return type < AnchorType::OPPOSITE_END;
			}
			DOUBLE get_value() const;
			AnchorType get_type() const
			{
				return type;
			}
		} left, right;

		DynamicValue offset_top = 0;
		DynamicValue offset_bottom = 0;
		DynamicValue offset_left = 0;
		DynamicValue offset_right = 0;
		bool is_vertical_centered = false;
		bool is_horizontal_centered = false;
		
		DynamicValue width = 0;
		DynamicValue height = 0;
	};

	class PassivRecieverNode : public RegionalEventRecieverNode, public Region
	{
	public:
		~PassivRecieverNode() override
		{
		}

		bool try_hit( const RegionalKeyEvent& key_event ) const override
		{
			return Region::check_region( key_event.get_x_position(), key_event.get_y_position() );
		}
	};
}
