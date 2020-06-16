#pragma once

#include <Defines.hpp>
#include <logic/Quad2D.hpp>
#include <logic/RegionEventReceiver.hpp>
#include <logic/gui/Button.hpp>

namespace noxcain
{
	class ScrollBarBase : public RegionalEventRecieverNode
	{
	public:

		void set_position( DOUBLE pos );
		DOUBLE get_position() const
		{
			return slider_position;
		}

		void set_slider_size( DOUBLE size );
		DOUBLE get_scale() const
		{
			return slider_length;
		}
		
		void set_depth_level( UINT32 level );
		void set_top( const Region& anchor, DOUBLE offset = 0 );
		void set_bottom( const Region& anchor, DOUBLE offset = 0 );
		void set_left( const Region& anchor, DOUBLE offset = 0 );
		void set_right( const Region& anchor, DOUBLE offset = 0 );

		void set_bar_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha = 1.0 );
		void set_slider_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha = 1.0 );
		void set_slider_highlight_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha = 1.0 );
		void set_slider_drag_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha = 1.0 );

		operator const Region& ( ) const;
		operator Region& ( );

		void deactivate();
		void activate();

		void hide();
		void show();

		bool is_hidden() const;
		bool is_shown() const;
		bool is_activ() const;

		void fix_scale();

	protected:
		constexpr static DOUBLE MAGIC_NUMBER = 0.0001;

		virtual DOUBLE get_total_slider_length() const = 0;
		virtual DOUBLE get_active_bar_length() const = 0;
		virtual DOUBLE get_event_value( const RegionalKeyEvent& event ) const = 0;
		
		DOUBLE get_slider_working_area_length() const;
		void make_scalable( DOUBLE relative_min_slider_size );

		ScrollBarBase( DOUBLE frame_size, RenderableList<RenderableQuad2D>& label_list, RenderableList<VectorText2D>& text_list, RegionalEventExclusivTracer& tracer );
		RegionalEventExclusivTracer& drag_tracer;

		DOUBLE slider_move_start = 0;
		const DOUBLE bar_endcap_size;

		DOUBLE min_slider_width = 0.15;
		DOUBLE slider_length = min_slider_width;
		DOUBLE slider_position = 0;

		PassivRecieverNode slider_region;
		RenderableQuad2D bar;
		BaseButton slider;
		BaseButton scale_button_left_top;
		BaseButton scale_button_right_bottom;

	private:

		bool try_hit( const RegionalKeyEvent& regional_event ) const
		{
			return bar.check_region( regional_event.get_x_position(), regional_event.get_y_position() );
		}

		bool move_slider( const RegionalKeyEvent& key_event, BaseButton& reciever );
		bool rescale_slider( DOUBLE direction, const RegionalKeyEvent& key_event, BaseButton& reciever );
	};

	class HorizontalScrollBar final : public ScrollBarBase
	{
	public:
		HorizontalScrollBar( DOUBLE frame_size, RenderableList<RenderableQuad2D>& label_list, RenderableList<VectorText2D>& text_list, RegionalEventExclusivTracer& tracer );

		///<summary> activates the ability of the slider to be rescaled </summary>
		///<param name="relative_min_slider_size"> used to define the minimal size of the silder
		///relative to the bar length</param>
		void make_scalable( DOUBLE relative_min_slider_size );
		~HorizontalScrollBar();

	private:
		DOUBLE get_total_slider_length() const override;
		DOUBLE get_active_bar_length() const override;
		DOUBLE get_event_value( const RegionalKeyEvent& event ) const override;
	};

	class VerticalScrollBar final : public ScrollBarBase
	{
	public:
		VerticalScrollBar( DOUBLE frame_size, RenderableList<RenderableQuad2D>& label_list, RenderableList<VectorText2D>& text_list, RegionalEventExclusivTracer& tracer );

		///<summary> activates the ability of the slider to be rescaled </summary>
		///<param name="relative_min_slider_size"> used to define the minimal size of the silder
		///relative to the bar length</param>
		void make_scalable( DOUBLE relative_min_slider_size );

	private:
		DOUBLE get_total_slider_length() const override;
		DOUBLE get_active_bar_length() const override;
		DOUBLE get_event_value( const RegionalKeyEvent& event ) const override;
	};
}
