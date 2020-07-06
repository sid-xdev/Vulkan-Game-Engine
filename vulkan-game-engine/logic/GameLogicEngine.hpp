#pragma once
#include <Defines.hpp>

#include <math/Matrix.hpp>
#include <math/Vector.hpp>

#include <logic/Renderable.hpp>

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

	struct GraphicSettings
	{
		UINT32 max_sample_count = 0;
		UINT32 current_sample_count = 0;

		FLOAT32 max_super_sampling_factor = 0;
		FLOAT32 current_super_sampling_factor = 0;

		ResolutionSetting current_resolution;
		ResolutionSetting get_accumulated_resolution() const
		{
			ResolutionSetting setting;
			setting.width = UINT32( current_resolution.width * current_super_sampling_factor );
			setting.height = UINT32( current_resolution.height * current_super_sampling_factor );
			return setting;
		}
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
		static GraphicSettings get_graphic_settings()
		{
			std::unique_lock<std::mutex> lock( engine->settings_mutex );
			return engine->graphic_settings;
		}

		static NxMatrix4x4 get_camera_matrix();

		static void set_graphic_settings( UINT32 sampleCount, FLOAT32 superSamplingFactor, UINT32 width, UINT32 height );

		template<typename T>
		using RenderableContainer = std::vector<std::reference_wrapper<typename Renderable<T>::List>>;
		static const RenderableContainer<VectorText2D>& get_vector_labels();
		static const RenderableContainer<GeometryObject>& get_geometry_objects();
		static const RenderableContainer<VectorText3D>& get_vector_decals();
		static const RenderableContainer<RenderableQuad2D>& get_color_labels();

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
		std::mutex settings_mutex;
		GraphicSettings graphic_settings;
		
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