#include <tools/TimeFrame.hpp>

noxcain::TimeFrameCollector::TimeFrameCollector( std::string name ) : id( TimeFrameCollector::block_id( name ) )
{
	
}

noxcain::TimeFrameCollector::TimeFrameCollector( std::size_t blocked_id ) : id( blocked_id )
{

}

void noxcain::TimeFrameCollector::start_frame( DOUBLE red, DOUBLE green, DOUBLE blue, DOUBLE alpha, const std::string& description )
{
	if( is_activ )
	{
		current_time_frame.start_frame = std::chrono::steady_clock::now();
		current_time_frame.color = { red, green, blue, alpha };
		current_time_frame.description = description;
	}
}

void noxcain::TimeFrameCollector::end_frame()
{
	if( is_activ && current_time_frame.start_frame.time_since_epoch().count() )
	{
		current_time_frame.end = std::chrono::steady_clock::now();
		if( frame_mutex.try_lock() )
		{
			collections[id]->add( current_time_frame );
			frame_mutex.unlock();
		}
		current_time_frame.start_frame = std::chrono::steady_clock::time_point();
	}
}

void noxcain::TimeFrameCollector::end_is_start( DOUBLE red, DOUBLE green, DOUBLE blue, DOUBLE alpha, const std::string& description )
{
	if( is_activ )
	{
		current_time_frame.end = std::chrono::steady_clock::now();
		if( current_time_frame.start_frame.time_since_epoch().count() )
		{
			if( frame_mutex.try_lock() )
			{
				collections[id]->add( current_time_frame );
				frame_mutex.unlock();
			}
		}
		current_time_frame.description = description;
		current_time_frame.color = { red, green, blue, alpha };
		current_time_frame.start_frame = current_time_frame.end;
	}
}

void noxcain::TimeFrameCollector::add_time_frame( const debugTimePoint& start_frame, const debugTimePoint& end, DOUBLE red, DOUBLE green, DOUBLE blue, DOUBLE alpha, const std::string& description )
{
	if( is_activ && frame_mutex.try_lock() )
	{
		collections[id]->add( TimeFrameData( 
			{
				description,
				{red,green,blue,alpha},
				start_frame,
				end 
			} ) );
		frame_mutex.unlock();
	}
}

std::size_t noxcain::TimeFrameCollector::block_id( const std::string& description )
{
	std::unique_lock lock( frame_mutex );
	collections.emplace_back( new TimeFrameCollection( description ) );
	return collections.size() - 1;
}

const std::vector<std::unique_ptr<noxcain::TimeFrameCollection>>& noxcain::TimeFrameCollector::get_time_frames()
{
	return collections;
}

void noxcain::TimeFrameCollector::activate()
{
	std::unique_lock lock( frame_mutex );
	is_activ = true;
}

void noxcain::TimeFrameCollector::deactivate()
{
	std::unique_lock lock( frame_mutex );
	is_activ = false;
}

noxcain::TimeFrameCollection::TimeFrameCollection( const std::string& name ) : description( name ), count( 0 )
{
}

std::size_t noxcain::TimeFrameCollection::add( const TimeFrameData& time_frame )
{
	dataBuffer[count % MAX_TIME_FRAMES] = time_frame;
	return ++count;
}

inline noxcain::TimeFrameCollection::ConstIterator::ConstIterator( const TimeFrameCollection& base_collection, std::size_t index ) : collection( base_collection ), index( index )
{
}

noxcain::TimeFrameCollection::ConstIterator noxcain::TimeFrameCollection::begin() const
{
	return ConstIterator( *this, 0 );
}

noxcain::TimeFrameCollection::ConstIterator noxcain::TimeFrameCollection::end() const
{
	return ConstIterator( *this, std::min( MAX_TIME_FRAMES, count ) );
}

noxcain::TimeFrameCollection::ConstIterator& noxcain::TimeFrameCollection::ConstIterator::operator++()
{
	++index;
	return *this;
}

bool noxcain::TimeFrameCollection::ConstIterator::operator!=( const ConstIterator& other ) const
{
	return &other.collection != &collection || other.index != index;
}

const noxcain::TimeFrameData& noxcain::TimeFrameCollection::ConstIterator::operator*() const
{
	return collection.dataBuffer[( ( collection.count > MAX_TIME_FRAMES ? collection.count % MAX_TIME_FRAMES : 0 ) + index ) % MAX_TIME_FRAMES];
}

const noxcain::TimeFrameData* noxcain::TimeFrameCollection::ConstIterator::operator->() const
{
	return &( operator*() );
}

std::vector<std::unique_ptr<noxcain::TimeFrameCollection>> noxcain::TimeFrameCollector::collections;
std::mutex noxcain::TimeFrameCollector::frame_mutex;
bool noxcain::TimeFrameCollector::is_activ = true;

noxcain::TimeFrame::TimeFrame( TimeFrameCollector& frame_collector, DOUBLE red, DOUBLE green, DOUBLE blue, DOUBLE alpha, const std::string& description ) : collector( frame_collector )
{
	collector.start_frame( red, green, blue, alpha, description );
}

noxcain::TimeFrame::~TimeFrame()
{
	collector.end_frame();
}
