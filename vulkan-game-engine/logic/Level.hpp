/**
 * @file Level.hpp
 *
 * @author Daniel Rzehak
 *
 */
#pragma once

#include <Defines.hpp>

#include <math/Matrix.hpp>
#include <logic/UserInterface.hpp>
#include <logic/RegionEventReceiver.hpp>
#include <logic/SceneGraph.hpp>
#include <tools/TimeFrame.hpp>

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
	class GeometryObject;

	
	/// <summary>
	///This base class is the bridge between any level logic and the render engine.
	///Use the protected membersand the virtual function update_level_logic
	///to ensure your objects are correctly deliverd to the engine.
	/// </summary>
	class GameLevel
	{
	public:
		template<typename T>
		using RenderableContainer = std::vector<std::reference_wrapper<typename Renderable<T>::List>>;
		using UserInterfaceContainer = std::vector<std::reference_wrapper<GameUserInterface>>;
		enum class Status
		{
			STARTING,
			RUNNING,
			FINISHED
		};

		/// <summary>
		/// updates the internal key states so they can used in the level logic
		/// </summary>
		/// <param name="key_events">key input events</param>
		void update_key_events( const std::vector<KeyEvent>& key_events );

		/// <summary>
		/// updates the game logic per frame
		/// </summary>
		/// <param name="delta_time">time delta to last frame</param>
		/// <param name="region_key_events">regional input events</param>
		void update_logic( const std::chrono::nanoseconds& delta_time, const std::vector<RegionalKeyEvent>& region_key_events );

		GameLevel();
		virtual ~GameLevel();

		/// <summary>
		/// get combination of camera position, direction and perspectiv matrix
		/// </summary>
		/// <returns>complete camera frustum matrix</returns>
		NxMatrix4x4 get_active_camera() const;

		/// <summary>
		/// get current level status
		/// </summary>
		/// <returns>status</returns>
		Status get_level_status() const;

		/// <summary>
		/// delivers all renderable (visible) geometry objects
		/// </summary>
		/// <returns>lists of geometry</returns>
		const RenderableContainer<GeometryObject>& get_geometry_objects() const
		{
			return geometry_renderables;
		}

		/// <summary>
		/// returns visible vector based 3d glyph elements (mostly text)
		/// </summary>
		/// <returns>glyph lists</returns>
		const RenderableContainer<VectorText3D>& get_vector_decals() const
		{
			return vector_decal_renderables;
		}

		/// <summary>
		/// returns a vector of different ui objects
		/// </summary>
		/// <returns>ui list</returns>
		const UserInterfaceContainer& get_user_interfaces() const
		{
			return user_interfaces;
		}

		/// <summary>
		/// returns height of renderable screen
		/// </summary>
		/// <returns>height in pixel</returns>
		UINT32 get_ui_height() const;

		/// <summary>
		/// returns width of renderable screen
		/// </summary>
		/// <returns>width in pixel</returns>
		UINT32 get_ui_width() const;

	protected:

		/// <summary>
		/// returns the root node of current scene graph
		/// </summary>
		/// <returns>root node</returns>
		SceneGraphNode& get_scene_root();

		/// <summary>
		/// returns the root of the 2d screen ogject tree
		/// </summary>
		/// <returns>root node</returns>
		PassivRecieverNode& get_screen_root();

		/// <summary>
		/// updates the interal status (for example to finish the game)
		/// </summary>
		/// <param name="new status">new game status</param>
		void set_status( Status new_status );

		/// <summary>
		/// Returns the handler for exclusiv regional input events.
		/// Pass this handler to 2d elements to lets them receive regional events whithout check other elements
		/// </summary>
		/// <returns>handler</returns>
		RegionalEventExclusivTracer& get_exclusiv_handler()
		{
			return regional_exclusiv;
		}

		//performance profil time frame collector
		TimeFrameCollector time_collector;

		//event handler for input keys
		std::vector<KeyEventHandler> key_handlers;
		
		//current active global camera settings
		NxMatrix4x4 activeCameraPerspective;
		NxMatrix4x4 activeCameraPosition;

		//contains refs to the current randerable objects
		RenderableContainer<GeometryObject> geometry_renderables;
		RenderableContainer<VectorText3D> vector_decal_renderables;
		void clear_user_interfaces();
		void add_user_interface( GameUserInterface& new_interface );

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
		UserInterfaceContainer user_interfaces;
		RegionalEventExclusivTracer regional_exclusiv;
		
		/// <summary>
		/// core implementaion of the level logic
		/// </summary>
		/// <param name="delta_time">time between to frames</param>
		virtual void update_level_logic( const std::chrono::nanoseconds& delta_time ) = 0;
	};
}