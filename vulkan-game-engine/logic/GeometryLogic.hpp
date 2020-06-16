#pragma once
#include <Defines.hpp>

#include <logic/Renderable.hpp>
#include <logic/SceneGraph.hpp>

#include <vector>
#include <vulkan\vulkan.hpp>

namespace noxcain
{
	class BoundingBox;
	class GeometryObject : public SceneGraphNode, public Renderable<GeometryObject>
	{
	public:
		static constexpr std::size_t VERTEX_PUSH_OFFSET = 0;
		static constexpr std::size_t VERTEX_PUSH_SIZE = 64;

		GeometryObject( Renderable<GeometryObject>::List& visibility_list );

		void record( vk::CommandBuffer command_buffer, vk::PipelineLayout layout, UINT32 index_count ) const;

		void set_geometry( std::size_t id )
		{
			geomtry_resource_id = id;
		}

		std::size_t get_geometry_id() const
		{
			return geomtry_resource_id;
		}

		const BoundingBox& get_bounding_box() const;
	
	private:
		std::size_t geomtry_resource_id = 0;
	};
}
