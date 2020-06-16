#include "SceneGraph.hpp"

#include <math/Matrix.hpp>

void noxcain::SceneGraphNode::set_local_matrix( const NxMatrix4x4& matrix )
{
	local_matrix = matrix;
}

void noxcain::SceneGraphNode::update_global_matrices( noxcain::SceneGraphNode::Stack& scene_graph_parent_stack, noxcain::SceneGraphNode::Stack& scene_graph_children_stack )
{
	scene_graph_parent_stack.clear();
	scene_graph_parent_stack.emplace_back( *this );

	while( !scene_graph_parent_stack.empty() )
	{
		scene_graph_children_stack.clear();
		for( SceneGraphNode& parent : scene_graph_parent_stack )
		{
			for( SceneGraphNode& child : parent.children )
			{
				child.global_matrix = parent.global_matrix*child.local_matrix;
				scene_graph_children_stack.emplace_back( child );
			}
		}
		scene_graph_parent_stack.swap( scene_graph_children_stack );
	}
	scene_graph_parent_stack.clear();
	scene_graph_children_stack.clear();
}
