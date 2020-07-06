#pragma once
#include <Defines.hpp>

#include <chrono>

namespace noxcain
{
	constexpr std::chrono::milliseconds GRAPHIC_TIMEOUT_DURATION = std::chrono::milliseconds( 100 );
	constexpr static UINT32 RECORD_RING_SIZE = 2;
}
