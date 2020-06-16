#pragma once
#include <logic/gui/Label.hpp>
#include <logic/RegionEventReceiver.hpp>

#include <array>

namespace noxcain
{
	class BaseButton : public VectorTextLabel2D, public RegionalEventRecieverNode
	{
	public:
		using Handler = std::function<bool( const RegionalKeyEvent& regional_event, BaseButton& event_reciever )>;

		BaseButton( RenderableList<VectorText2D>& text_list, RenderableList<RenderableQuad2D>& label_list );
		~BaseButton() override
		{
		}

		void set_background_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha = 1.0 )
		{
			normal_color = { red, green, blue, alpha };
			background.set_color( normal_color );
		}
		void set_highlight_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha = 1.0 )
		{
			over_color = { red, green, blue, alpha };
		}
		void set_active_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha = 1.0 )
		{
			click_color = { red, green, blue, alpha };
		}
		void set_deactive_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha = 1.0 )
		{
			inactive_color = { red, green, blue, alpha };
		}

		void set_frame_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha = 1.0 )
		{
			frame.set_color( red, green, blue, alpha );
		}

		void activate();
		void deactivate();

		void hide() override;
		void show() override;

		void set_enter_handler( const Handler& handler = Handler() )
		{
			enter_handler = handler;
		}
		void set_leave_handler( const Handler& handler = Handler() )
		{
			leave_handler = handler;
		}
		void set_over_handler( const Handler& handler = Handler() )
		{
			over_handler = handler;
		}
		void set_down_handler( const Handler& handler = Handler() )
		{
			down_handler = handler;
		}
		void set_up_handler( const Handler& handler = Handler() )
		{
			up_handler = handler;
		}
		void set_click_handler( const Handler& handler = Handler() )
		{
			click_handler = handler;
		}

	protected:

		Handler enter_handler;
		Handler leave_handler;
		Handler over_handler;

		Handler down_handler;
		Handler up_handler;
		Handler click_handler;

		RegionalKeyEvent::KeyCodes last_key_code = RegionalKeyEvent::KeyCodes::NONE;

		std::array<FLOAT32, 4> normal_color = { 0.0F,0.0F,0.0F,0.0F };
		std::array<FLOAT32, 4> over_color = { 0.0F,0.0F,0.0F,0.0F };
		std::array<FLOAT32, 4> click_color = { 0.0F,0.0F,0.0F,0.0F };
		std::array<FLOAT32, 4> inactive_color = { 0.0F,0.0F,0.0F,0.0F };

		std::array<FLOAT32, 4> active_text_color = { 0.0F,0.0F,0.0F,1.0F };
		std::array<FLOAT32, 4> inactive_text_color = { 0.0F, 0.0F, 0.0F, 0.3F };

		bool try_hit( const RegionalKeyEvent& regional_event ) const override;
		bool hit( const RegionalKeyEvent& key_event ) override;
		bool miss( const RegionalKeyEvent& key_event ) override;
	};
}