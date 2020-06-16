#pragma once

#include <math/Matrix.hpp>
#include <logic/Renderable.hpp>
#include <logic/RegionEventReceiver.hpp>
#include <logic/SceneGraph.hpp>

#include <memory>
#include <chrono>
#include <vector>

namespace noxcain
{	
	class Region;
	class PassivRecieverNode;
	class KeyEventHandler;
	class KeyEvent;

	class VectorText3D;
	class VectorText2D;
	class RenderableQuad2D;
	class GeometryObject;

	class GameLevel
	{
	public:
		template<typename T>
		using RenderableContainer = std::vector<std::reference_wrapper<typename Renderable<T>::List>>;
		enum class Status
		{
			STARTING,
			RUNNING,
			FINISHED
		};

		// updates the internal level logic
		void update( const std::chrono::nanoseconds& deltaTime, const std::vector<KeyEvent>& key_events, const std::vector<RegionalKeyEvent>& region_key_events );

		GameLevel();
		virtual ~GameLevel();

		NxMatrix4x4 get_active_camera() const;

		Status get_level_status() const;

		const RenderableContainer<VectorText2D>& get_vector_labels() const
		{
			return vector_label_renderables;
		}
		const RenderableContainer<GeometryObject>& get_geometry_objects() const
		{
			return geometry_renderables;
		}
		const RenderableContainer<VectorText3D>& get_vector_decals() const
		{
			return vector_decal_renderables;
		}
		const RenderableContainer<RenderableQuad2D>& get_color_labels();

		UINT32 get_ui_height() const;
		UINT32 get_ui_width() const;

	protected:
		std::vector<KeyEventHandler> key_handlers;
		NxMatrix4x4 activeCameraPerspective;
		NxMatrix4x4 activeCameraPosition;

		SceneGraphNode& get_scene_root();
		PassivRecieverNode& get_screen_root();
		void set_status( Status newStatus );

		RenderableContainer<GeometryObject> geometry_renderables;
		RenderableContainer<VectorText3D>      vector_decal_renderables;
		RenderableContainer<VectorText2D>      vector_label_renderables;
		RenderableContainer<RenderableQuad2D>     color_label_renderables;

		RegionalEventExclusivTracer& get_exclusiv_handler()
		{
			return regional_exclusiv;
		}

	private:
		Status status = Status::STARTING;
		
		// level scene graph;
		SceneGraphNode::Stack scene_graph_parent_stack;
		SceneGraphNode::Stack scene_graph_children_stack;
		std::unique_ptr<SceneGraphNode> scene_root;
		
		// level interaction tree
		RegionalEventRecieverNode::Stack interaction_parent_stack;
		RegionalEventRecieverNode::Stack interaction_children_stack;
		RegionalEventRecieverNode::Stack interaction_miss_stack;
		std::unique_ptr<PassivRecieverNode> ui_root;
		RegionalEventExclusivTracer regional_exclusiv;
		
		// update loop methode
		virtual void update_level_logic( const std::chrono::nanoseconds& deltaTime ) = 0;
	};
}