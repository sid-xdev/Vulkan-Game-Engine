#include "FontResource.hpp"

#include <resources/BoundingBox.hpp>
#include <resources/GameResourceEngine.hpp>


noxcain::FontResource::FontResource(
	UINT32 font_offset,
	std::vector<UnicodeRange>&& unicode_ranges,
	std::vector<CharacterInfo>&& character_infos,
	DOUBLE ascender,
	DOUBLE descender,
	DOUBLE line_gap,
	std::vector<std::size_t>&& point_data_offset_resource_ids,
	std::vector<std::size_t>&& point_data_resource_ids,
	std::size_t vertex_resource_id ) :
	font_offset( font_offset ), ascender( ascender ), descender( descender ), line_gap( line_gap ), vertex_resource_id( vertex_resource_id ),
	unicode_map( std::move( unicode_ranges ) ), character_infos( std::move( character_infos ) ),
	point_data_offset_resource_ids( point_data_offset_resource_ids ), point_data_resource_ids( point_data_resource_ids )
{
}

noxcain::FontResource::CharacterInfo noxcain::FontResource::get_character_info( UINT32 unicode ) const
{

	UINT32 char_index = get_character_index( unicode );
	if( char_index < character_infos.size() )
	{
		return character_infos[char_index];
	}
	return  { INVALID_UNICODE, 0.0 };
}

std::size_t noxcain::FontResource::get_glyph_count() const
{
	return point_data_offset_resource_ids.size();
}

const noxcain::BoundingBox noxcain::FontResource::get_character_bounding_box( UINT32 unicode ) const
{
	std::size_t char_index = get_character_index( unicode );
	if( char_index >= character_infos.size() || character_infos[char_index].glyph_index == INVALID_UNICODE )
	{
		return BoundingBox();
	}
	std::array<FLOAT32, 8> glyphQuad;
	ResourceEngine::get_engine().get_subresources()[vertex_resource_id].getData( glyphQuad.data(), 8 * sizeof( FLOAT32 ), ( std::size_t( font_offset ) + character_infos[char_index].glyph_index ) * 8 * sizeof( FLOAT32 ) );
	
	return BoundingBox( glyphQuad[0], glyphQuad[7], 0.0, glyphQuad[6], glyphQuad[1], 0.0 );
}

noxcain::UINT32 noxcain::FontResource::get_character_index( UINT32 unicode ) const
{
	if( unicode_map.size() && unicode >= unicode_map.front().start && unicode <= unicode_map.back().end )
	{
		for( const auto& unicode_range : unicode_map )
		{
			if( unicode <= unicode_range.end )
			{
				if( unicode >= unicode_range.start )
				{
					return unicode_range.index + unicode - unicode_range.start;
				}
				else
				{
					break;
				}
			}
		}
	}
	return 0;
}
