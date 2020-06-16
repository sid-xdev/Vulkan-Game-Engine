#pragma once
#include <Defines.hpp>

#include <chrono>
#include <vector>
#include <mutex>
#include <string>
#include <array>

namespace noxcain
{
	typedef std::chrono::time_point<std::chrono::steady_clock> debugTimePoint;

	constexpr static std::size_t MAX_TIME_FRAMES = 100;

	struct TimeFrameData
	{
		std::string description;
		std::array<DOUBLE, 4> color;
		debugTimePoint start_frame;
		debugTimePoint end;
	};

	class TimeFrameCollection
	{
		friend class TimeFrameCollector;
		
		TimeFrameCollection( const std::string& name );
		std::size_t add( const TimeFrameData& time_frame );

		std::string description;
		std::array<TimeFrameData, MAX_TIME_FRAMES> dataBuffer;
		std::size_t count;
	public:

		class ConstIterator
		{
			friend class TimeFrameCollection;
			const TimeFrameCollection& collection;
			std::size_t index = 0;
			ConstIterator( const TimeFrameCollection& base_collection, std::size_t index );
		public:
			ConstIterator& operator++();
			bool operator!=( const ConstIterator& other ) const;
			const TimeFrameData& operator*() const;
			const TimeFrameData* operator->() const;
		};

		const std::string& get_description() const
		{
			return description;
		}

		ConstIterator begin() const;
		ConstIterator end() const;
	};

	class TimeFrameCollector
	{
	private:
		static std::vector<std::unique_ptr<TimeFrameCollection>> collections;
		static std::mutex frame_mutex;
		static bool is_activ;

		const std::size_t id;
		TimeFrameData current_time_frame;
	public:
		TimeFrameCollector( std::string name );
		TimeFrameCollector( std::size_t id );
		
		void start_frame( DOUBLE red, DOUBLE green, DOUBLE blue, DOUBLE alpha = 1.0, const std::string& description = "" );
		void end_frame();
		void end_is_start( DOUBLE red, DOUBLE green, DOUBLE blue, DOUBLE alpha = 1.0, const std::string& description = "" );
		void add_time_frame( const debugTimePoint& start_frame, const debugTimePoint& end, DOUBLE red, DOUBLE green, DOUBLE blue, DOUBLE alpha = 1.0, const std::string& description = "" );

		static std::size_t block_id( const std::string& description );

		static const std::vector<std::unique_ptr<TimeFrameCollection>>& get_time_frames();
		static void activate();
		static void deactivate();
	};

	class TimeFrame
	{
	public:
		TimeFrame( TimeFrameCollector& frame, DOUBLE red, DOUBLE green, DOUBLE blue, DOUBLE alpha = 1.0, const std::string& description = "" );
		~TimeFrame();
	private:
		TimeFrameCollector& collector;
	};
}