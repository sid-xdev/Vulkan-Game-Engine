#pragma once

#include <functional>

namespace noxcain
{
	class Watcher
	{
	public:
		Watcher( std::function<void()> destruction_callback ) : callback( destruction_callback )
		{
		}
		~Watcher()
		{
			if( callback ) callback();
		}
	private:
		std::function<void()> callback;
	};
}