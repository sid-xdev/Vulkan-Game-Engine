#include "BoundingBox.hpp"

noxcain::BoundingBox::BoundingBox( DOUBLE minX, DOUBLE minY, DOUBLE minZ, DOUBLE maxX, DOUBLE maxY, DOUBLE maxZ )
{
	min_corner = { minX, minY, minZ };
	max_corner = { maxX, maxY, maxZ };
}

noxcain::DOUBLE noxcain::BoundingBox::get_width() const
{
	return max_corner[0] - min_corner[0];
}

noxcain::DOUBLE noxcain::BoundingBox::get_height() const
{
	return max_corner[1] - min_corner[1];
}

noxcain::DOUBLE noxcain::BoundingBox::get_depth() const
{
	return max_corner[2] - min_corner[2];
}

noxcain::DOUBLE noxcain::BoundingBox::get_left() const
{
	return min_corner[0];
}

noxcain::DOUBLE noxcain::BoundingBox::get_right() const
{
	return max_corner[0];
}

noxcain::DOUBLE noxcain::BoundingBox::get_bottom() const
{
	return min_corner[1];
}

noxcain::DOUBLE noxcain::BoundingBox::get_top() const
{
	return max_corner[1];
}

noxcain::DOUBLE noxcain::BoundingBox::get_back() const
{
	return min_corner[2];
}

noxcain::DOUBLE noxcain::BoundingBox::get_front() const
{
	return max_corner[2];
}
