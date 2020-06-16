#pragma once

#include <Defines.hpp>
#include <array>

#include <resources/BoundingBox.hpp>

namespace noxcain
{
	class GeometryResource
	{
		BoundingBox bounding_box;
		std::size_t vertices_id;
		std::size_t indices_id;
	public:
		GeometryResource( std::size_t vertices_subresource_id, std::size_t vertex_indices_subresource_id, const BoundingBox& bounding_box );
		std::size_t get_vertex_buffer_id() const
		{
			return vertices_id;
		}

		std::size_t get_index_buffer_id() const
		{
			return indices_id;
		}

		const BoundingBox& get_bounding_box() const
		{
			return bounding_box;
		}
	};
}