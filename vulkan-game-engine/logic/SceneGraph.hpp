#pragma once
#include <logic/TreeNode.hpp>
#include <math/Matrix.hpp>

#include <vector>
#include <memory>


namespace noxcain
{
	class NxMatrix4x4;

	class SceneGraphNode : public TreeNode<SceneGraphNode>
	{
	public:
		using Stack = std::vector<std::reference_wrapper<SceneGraphNode>>;

		void set_local_matrix( const NxMatrix4x4& matrix );
		
		const NxMatrix4x4& get_local_matrix() const
		{
			return local_matrix;
		}

		const NxMatrix4x4& get_world_matrix() const
		{
			return global_matrix;
		}
		
		void update_global_matrices( Stack& scene_graph_parent_stack, Stack& scene_graph_children_stack );

	protected:
		NxMatrix4x4 local_matrix;
		NxMatrix4x4 global_matrix;
	};
}