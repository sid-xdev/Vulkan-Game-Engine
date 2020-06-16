#pragma once
#include <Defines.hpp>

#include <mutex>

namespace noxcain
{
	class Watcher
	{
	public:
		Watcher( bool& state, std::mutex& mutex, std::condition_variable& notification_object ) : watched_state( state ), thread_mutex( mutex ), notification_target( notification_object )
		{
		}
		~Watcher()
		{
			std::unique_lock<std::mutex> lock( thread_mutex );
			watched_state = !watched_state;
			notification_target.notify_all();
		}
	private:
		bool& watched_state;
		std::mutex& thread_mutex;
		std::condition_variable& notification_target;
	};
}