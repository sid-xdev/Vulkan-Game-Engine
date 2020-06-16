#pragma once
#include <Defines.hpp>

#include <chrono>

namespace noxcain
{
	constexpr std::chrono::milliseconds GRAPHIC_TIMEOUT_DURATION = std::chrono::milliseconds( 100 );
	constexpr static std::size_t RECORD_RING_SIZE = 2;
}
