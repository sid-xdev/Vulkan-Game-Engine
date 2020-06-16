#include "GeometryResource.hpp"

noxcain::GeometryResource::GeometryResource( std::size_t vertices_subresource_id, std::size_t vertex_indices_subresource_id, const BoundingBox& bounding_box ) :
	vertices_id( vertices_subresource_id ),
	indices_id( vertex_indices_subresource_id ),
	bounding_box( bounding_box )
{
}
