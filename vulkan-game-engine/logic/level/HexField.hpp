#pragma once

#include <logic/level/MineSweeperLevel.hpp>
#include <logic/SceneGraph.hpp>
#include <logic/VectorText3D.hpp>
#include <logic/GeometryLogic.hpp>

namespace noxcain
{
	class MineSweeperLevel;
	class BoundingBox;

	class MineSweeperLevel::HexField : public SceneGraphNode
	{
		constexpr static std::size_t geometry_id = 0;
		MineSweeperLevel& owner;
		VectorText3D mine_count_decal;
		GeometryObject field_geometry;
		void update_position( UINT32 unicode );
	public:
		static DOUBLE get_hex_side_length();
		static const BoundingBox& get_object_space_bounding_box();
		void set_mine_neighbor_count( UINT8 count );
		void set_mine_number( UINT32 number );
		HexField( MineSweeperLevel& owner );
		~HexField();
		void set_font_id( std::size_t font_id );
		std::size_t get_font_id() const;
	};
}