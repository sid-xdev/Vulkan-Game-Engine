#include "HexField.hpp"

#include <logic/GeometryLogic.hpp>
#include <logic/VectorText3D.hpp>

#include <resources/FontResource.hpp>
#include <resources/GeometryResource.hpp>
#include <resources/GameResourceEngine.hpp>

void noxcain::MineSweeperLevel::HexField::update_position( UINT32 unicode )
{
	mine_count_decal.show();
	mine_count_decal.set_text( std::string( { char( unicode ) } ) );
	const auto& font = ResourceEngine::get_engine().get_font( mine_count_decal.get_font_id() );

	const auto& hexBoundingBox = field_geometry.get_bounding_box();
	const auto& numBoundingBox = font.getCharBoundingBox( unicode );

	DOUBLE size = ( 0.6*hexBoundingBox.get_height() ) / numBoundingBox.get_height();
	
	mine_count_decal.set_font_size( size );
	mine_count_decal.set_local_matrix( NxMatrix4x4().translation( {
		-size *( numBoundingBox.get_left() + 0.5*numBoundingBox.get_width() ),
		-size *( font.getDescender() + numBoundingBox.get_bottom() + 0.5*numBoundingBox.get_height() ),
		0.0 } ) );
}

noxcain::DOUBLE noxcain::MineSweeperLevel::HexField::get_hex_side_length()
{
	return 0.5 * ResourceEngine::get_engine().get_geometry( geometry_id ).get_bounding_box().get_width();
}

const noxcain::BoundingBox& noxcain::MineSweeperLevel::HexField::get_object_space_bounding_box()
{
	return ResourceEngine::get_engine().get_geometry( geometry_id ).get_bounding_box();
}

void noxcain::MineSweeperLevel::HexField::set_mine_neighbor_count( UINT8 count )
{
	switch( count )
	{
		case 0:
		{
			mine_count_decal.hide();
			break;
		}
		case 1:
		{
			update_position( 0x0031 );
			mine_count_decal.set_font_color( 0.12, 1.0, 0.0 );
			break;
		}
		case 2:
		{
			update_position( 0x0032 );
			mine_count_decal.set_font_color( 0.0, 0.44, 0.87 );
			break;
		}
		case 3:
		{
			update_position( 0x0033 );
			mine_count_decal.set_font_color( 0.64, 0.21, 0.93 );
			break;
		}
		case 4:
		{
			update_position( 0x0034 );
			mine_count_decal.set_font_color( 0.90, 0.80, 0.50 );
			break;
		}
		case 5:
		{
			update_position( 0x0035 );
			mine_count_decal.set_font_color(  1.00, 0.50, 0.00 );
			break;
		}
		case 6:
		{
			update_position( 0x0036 );
			mine_count_decal.set_font_color( 1.00, 0.00, 0.00 );
			break;
		}
		default:
		{
			update_position( 0x0025 );
			mine_count_decal.set_font_color( 0.62, 0.62, 0.62 );
			break;
		}
	}
}

void noxcain::MineSweeperLevel::HexField::set_mine_number( UINT32 number )
{
	mine_count_decal.show();
	mine_count_decal.set_font_color( 0.0, 0.0, 0.0 );
	mine_count_decal.set_text( std::to_string( number ) );
	mine_count_decal.set_font_size( 1.0 );

	const auto& hexBoundingBox = field_geometry.get_bounding_box();

	DOUBLE size_width = ( 0.6 * hexBoundingBox.get_width() ) / ( mine_count_decal.get_width() );
	DOUBLE size_height = ( 0.6 * hexBoundingBox.get_height() ) / ( mine_count_decal.get_height() );

	mine_count_decal.set_font_size( size_width < size_height ? size_width : size_height );
	
	mine_count_decal.set_local_matrix( NxMatrix4x4().translation( {
		-0.5*mine_count_decal.get_width(),
		-0.5*mine_count_decal.get_height(),
		0.0 } ) );
}

noxcain::MineSweeperLevel::HexField::HexField( MineSweeperLevel& owner ) :
	owner( owner ),
	field_geometry( owner.geometry_list ),
	mine_count_decal( owner.vector_dacal_list )
{
	owner.grid->add_branch( *this );
	add_branch( field_geometry );
	field_geometry.add_branch( mine_count_decal );
	field_geometry.set_geometry( geometry_id );
	mine_count_decal.hide();
}

noxcain::MineSweeperLevel::HexField::~HexField()
{
}

void noxcain::MineSweeperLevel::HexField::set_font_id( std::size_t font_id )
{
	mine_count_decal.set_font( UINT32( font_id ) );
}

std::size_t noxcain::MineSweeperLevel::HexField::get_font_id() const
{
	return mine_count_decal.get_font_id();
}
