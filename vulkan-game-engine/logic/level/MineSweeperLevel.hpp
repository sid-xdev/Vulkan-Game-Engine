#pragma once
#include <memory>
#include <any>
#include <logic/Level.hpp>


namespace noxcain
{	
	class CubicSpline;
	class BaseButton;

	class MineSweeperLevel : public GameLevel
	{
	private:
		Renderable<VectorText3D>::List vector_dacal_list;
		Renderable<GeometryObject>::List geometry_list;
		Renderable<RenderableQuad2D>::List color_label_list;
		Renderable<VectorText2D>::List vector_label_list;

		class HexField;
		enum class KeyEvents : std::size_t
		{
			eRotateGridLeft = 0,
			eRotateGridRight,
			eRotateGridUp,
			eRotateGridDown,
			FINSIH,

			ROTATE_CAMERA_LEFT,
			ROTATE_CAMERA_RIGHT,
			ROTATE_CAMERA_UP,
			ROTATE_CAMERA_DOWN,
			MOVE_CAMERA_FORWARD,
			MOVE_CAMERA_BACKWARD
		};
		static constexpr std::size_t KEY_EVENT_COUNT = ( std::size_t )KeyEvents::MOVE_CAMERA_BACKWARD + 1;
		KeyEventHandler& get_key( KeyEvents key );

		void setup_events();
		void check_events( const std::chrono::nanoseconds& delta );

		DOUBLE camera_screen_width = 0.0; 
		DOUBLE camera_screen_height = 0.0;
		DOUBLE camera_space_near = 0.0;
		DOUBLE camera_space_far = 0.0;

		DOUBLE camera_center_distance = 90.0;
		DOUBLE camera_vertical_angle = 0.0;
		DOUBLE camera_horizontal_angle = 0.0;

		constexpr static DOUBLE anglePerMs = 0.0003;
		constexpr static DOUBLE columnCount = 30;
		
		DOUBLE currentAngle = 0.0;
		DOUBLE lineAngle = 0.0;
		INT32 startIndex = 0;

		std::unique_ptr<SceneGraphNode> grid;
		std::vector<std::unique_ptr<HexField>> hex_fields;
		//std::unique_ptr<CubicSpline> fieldOrientationSpline;

		void update_camera();

		void update_level_logic( const std::chrono::nanoseconds& deltaTime ) override;

		void rotate_grid( DOUBLE milliseconds, const NxVector3D& rotationAxis );

		void setup_level();

		std::unique_ptr<VectorText2D> camera_distance_label;
		std::unique_ptr<BaseButton> debug_button;
		std::unique_ptr<BaseButton> switch_font_button;

	public:
		MineSweeperLevel();
		~MineSweeperLevel();
	};
}