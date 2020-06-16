#include "GeometryLogic.hpp"
#include <resources/GeometryResource.hpp>
#include <resources/GameResourceEngine.hpp>
#include <math/Matrix.hpp>

const noxcain::BoundingBox& noxcain::GeometryObject::get_bounding_box() const
{
	return ResourceEngine::get_engine().get_geometry( geomtry_resource_id ).get_bounding_box();
}

noxcain::GeometryObject::GeometryObject( Renderable<GeometryObject>::List& visibility_list ) : Renderable<GeometryObject>( visibility_list )
{
}

void noxcain::GeometryObject::record( vk::CommandBuffer command_buffer, vk::PipelineLayout layout, UINT32 index_count ) const
{
	const auto& pos = get_world_matrix().gpuData();
	command_buffer.pushConstants( layout, vk::ShaderStageFlagBits::eVertex, VERTEX_PUSH_OFFSET, VERTEX_PUSH_SIZE, pos.data() );
	command_buffer.drawIndexed( index_count, 1, 0, 0, 0 );
}
