#pragma once

#include <Defines.hpp>
#include <array>

namespace noxcain
{
	class BoundingBox
	{
		std::array<DOUBLE, 3> min_corner = { {0.0,0.0,0.0} };
		std::array<DOUBLE, 3> max_corner = { 0,0,0 };

	public:
		BoundingBox() = default;
		BoundingBox( DOUBLE min_x, DOUBLE min_y, DOUBLE min_z, DOUBLE max_x, DOUBLE max_y, DOUBLE max_z );
		DOUBLE get_width() const;
		DOUBLE get_height() const;
		DOUBLE get_depth() const;

		DOUBLE get_left() const;
		DOUBLE get_right() const;

		DOUBLE get_bottom() const;
		DOUBLE get_top() const;

		DOUBLE get_back() const;
		DOUBLE get_front() const;
	};
}
