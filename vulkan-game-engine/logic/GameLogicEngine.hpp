#pragma once
#include <Defines.hpp>

#include <math/Matrix.hpp>
#include <math/Vector.hpp>

#include <logic/Renderable.hpp>
#include <logic/Level.hpp>

#include <memory>
#include <thread>
#include <shared_mutex>
#include <chrono>
#include <list>
#include <condition_variable>
#include <vector>

namespace noxcain
{
	class KeyEvent;
	class KeyEventHandler;
	class Region;
	class RegionalKeyEvent;
	class VectorText2D;
	class RenderableQuad2D;
	class GeometryObject;
	class VectorText3D;

	struct ResolutionSetting
	{
		UINT32 width = 0;
		UINT32 height = 0;
	};

	struct CursorPosition
	{
		INT32 x = 0;
		INT32 y = 0;
	};

	class GraphicSetting
	{
	private:
		friend class LogicEngine;
		struct Settings
		{
			UINT32 max_sample_count = 0;
			UINT32 current_sample_count = 0;

			FLOAT32 max_super_sampling_factor = 0;
			FLOAT32 current_super_sampling_factor = 0;

			ResolutionSetting current_resolution;
		}&settings;

		std::shared_lock<std::shared_mutex> lock;
		GraphicSetting( std::shared_mutex& graphic_mutex, Settings& graphic_settings );
	public:
		GraphicSetting( const GraphicSetting& other );

		UINT32 get_max_sample_count() const;
		UINT32 get_sample_count() const;
		FLOAT32 get_max_super_sampling_factor() const;
		FLOAT32 get_super_sampling_factor() const;
		
		ResolutionSetting get_accumulated_resolution() const;
	};

	enum class InputEventTypes
	{
		KEY_UP = 0,
		KEY_DOWN = 1,
		REGION_KEY_UP,
		REGION_KEY_DOWN,
		REGION_MOVE
	};
	
	class GameLevel;
	class DebugLevel;

	class LogicEngine
	{
	public:
		//graphic properties
		static const GraphicSetting get_graphic_settings()
		{
			GraphicSetting settings( engine->read_settings_mutex, engine->read_graphic_settings );
			return settings;
		}

		static NxMatrix4x4 get_camera_matrix();

		static void set_sample_count( UINT32 sample_count );
		static void set_graphic_settings( UINT32 sampleCount, FLOAT32 superSamplingFactor, UINT32 width, UINT32 height );
		static void apply_graphic_settings();
		
		static const GameLevel::RenderableContainer<VectorText3D>& get_vector_decals();
		static const GameLevel::RenderableContainer<GeometryObject>& get_geometry_objects();
		static const GameLevel::UserInterfaceContainer& get_user_interfaces();

		static void set_event( InputEventTypes type, INT32 param1, INT32 param2 = 0, UINT32 param3 = 0 );

		static void update();
		static void finish();

		static void pause();
		static void resume();

		static bool is_running();

		static void show_performance_overlay();

		static CursorPosition get_cursor_position();

		static void set_cpu_cycle_duration( std::chrono::nanoseconds duration )
		{
			engine->cpu_cycle_duration = duration;
		}

		static void set_gpu_cycle_duration( std::chrono::nanoseconds duration )
		{
			engine->gpu_cycle_duration = duration;
		}

		static std::chrono::nanoseconds get_cpu_cycle_duration()
		{
			return engine->cpu_cycle_duration;
		}

		static std::chrono::nanoseconds get_gpu_cycle_duration()
		{
			return engine->gpu_cycle_duration;
		}

	private:
		//singelton
		LogicEngine();
		static std::unique_ptr<LogicEngine> engine;

		//logic update
		UINT32 logic_update();
		std::condition_variable status_condition;
		std::thread logic_thread;
		
		//time
		std::chrono::time_point<std::chrono::steady_clock> last_update_time_point;
		std::chrono::nanoseconds gpu_cycle_duration = std::chrono::seconds( 1 );
		std::chrono::nanoseconds cpu_cycle_duration = std::chrono::seconds( 1 );
		bool time_start_reset = true;

		//levels
		std::unique_ptr<GameLevel> current_level;
		std::unique_ptr<DebugLevel> debug_level;

		//inputEvents
		std::mutex event_mutex;
		std::vector<KeyEvent> new_key_events;
		std::vector<RegionalKeyEvent> new_region_key_events;

		CursorPosition cursor_position;

		//game status
		mutable std::mutex status_mutex;
		enum class Status
		{
			DORMANT,
			UPDATING,
			PAUSED,
			EXIT
		} status = Status::DORMANT;
		
		// special graphic settings sync
		std::shared_mutex read_settings_mutex;
		std::shared_mutex write_settings_mutex;
		GraphicSetting::Settings read_graphic_settings;
		GraphicSetting::Settings write_graphic_settings;
		
		// finish game 
		void finish_game();
	public:
		//singelton construction
		LogicEngine( const LogicEngine& ) = delete;
		LogicEngine( LogicEngine&& ) = delete;
		LogicEngine& operator=( const LogicEngine& ) = delete;
		LogicEngine& operator=( LogicEngine&& ) = delete;

		~LogicEngine();
	};
}