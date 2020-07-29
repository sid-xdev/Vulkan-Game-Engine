#pragma once

#include <logic/RegionEventReceiver.hpp>
#include <logic/Level.hpp>
#include <logic/Quad2D.hpp>
#include <logic/VectorText2D.hpp>

#include <vector>
#include <mutex>

namespace noxcain
{
	class BaseButton;
	class HorizontalScrollBar;
	class VectorTextLabel2D;
	
	class DebugLevel : public GameLevel
	{
	private:

		GameUserInterface ui;

		class TimeFrameLabel : public RegionalEventRecieverNode
		{
		public:
			TimeFrameLabel( DebugLevel& level );

			void setup_as_group_label( const Region& anchor_parent, DOUBLE x_offset, DOUBLE y_offset, DOUBLE width, DOUBLE height,
									   DOUBLE red, DOUBLE green, DOUBLE blue, const std::string& description );
			void setup_as_time_frame( const Region& group_label,
									  DOUBLE start, DOUBLE width,
									  DOUBLE red, DOUBLE green, DOUBLE blue,
									  const std::string& description, UINT32 milliseconds,
									  const Region& scissor );
			void hide();

			operator Region& ( );
			operator const Region& ( ) const;

			Region& get_region();
			void set_color( DOUBLE red, DOUBLE green, DOUBLE blue );



		private:
			bool try_hit( const RegionalKeyEvent& key_event ) const override;
			bool hit( const RegionalKeyEvent& key_event ) override;
			bool miss( const RegionalKeyEvent& key_event ) override;

			std::string time_description;
			VectorTextLabel2D& tooltip;
			RenderableQuad2D frame;
			RenderableQuad2D background;
			VectorText2D text;
		};

		enum class KeyEvents : std::size_t
		{
			EXIT,
		};

		static constexpr std::size_t KEY_EVENT_COUNT = ( std::size_t )KeyEvents::EXIT + 1;

		static constexpr DOUBLE LABEL_FRAME_WIDTH = 1;
		static constexpr DOUBLE LABEL_DISTANCE = 10;
		static constexpr DOUBLE BASE_COLOR_FAC = 0.2;

		mutable std::mutex visibilty_mutex;

		std::vector<std::unique_ptr<TimeFrameLabel>> time_frame_group_labels;
		std::vector<std::unique_ptr<TimeFrameLabel>> time_frame_labels;
		std::unique_ptr<PassivColorLabel> background;
		std::unique_ptr<HorizontalScrollBar> scroll_bar;
		std::unique_ptr<BaseButton> exit_button;
		std::unique_ptr<VectorTextLabel2D> tooltip;

		constexpr static std::chrono::nanoseconds PERFORMANCE_TIME_FRAME = std::chrono::milliseconds( 10 );
		std::chrono::steady_clock::time_point performance_time_stamp;
		std::unique_ptr<Region> scissor_label;

		void initialize();

		void setup_events();
		void check_events( const std::chrono::nanoseconds& delta );

		void update_level_logic( const std::chrono::nanoseconds& deltaTime ) override;

		bool is_started = false;
		bool is_on = false;
		bool initialized = false;

		std::chrono::steady_clock::time_point measurement_start_time;

	public:
		DebugLevel();
		~DebugLevel();
		
		void switch_on();
		void switch_off();
		bool on();
	};
}