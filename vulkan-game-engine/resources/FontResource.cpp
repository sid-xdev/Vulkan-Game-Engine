#include "FontResource.hpp"
#include <resources/GameResourceEngine.hpp>

noxcain::FontResource::FontResource(
	UINT32 font_offset,
	std::vector<UINT32>&& unicode_map,
	std::vector<CharacterInfo>&& characterInfos,
	DOUBLE ascender,
	DOUBLE descender,
	DOUBLE lineGap,
	std::vector<std::size_t>&& point_data_offset_resource_ids,
	std::vector<std::size_t>&& point_data_resource_ids,
	std::size_t vertex_resource_id ) :
	font_offset( font_offset ), ascender( ascender ), descender( descender ), lineGap( lineGap ), vertex_resource_id( vertex_resource_id ),
	unicode_map( std::move( unicode_map ) ), characterInfos( std::move( characterInfos ) ),
	point_data_offset_resource_ids( point_data_offset_resource_ids ), point_data_resource_ids( point_data_resource_ids )
{

}

noxcain::FontResource::CharacterInfo noxcain::FontResource::getCharacterInfo( UINT32 unicode ) const
{
	std::size_t charIndex = unicode > unicode_map.size() ? 0 : unicode_map[unicode];
	if( characterInfos.size() <= charIndex )
	{
		return  { INVALID_UNICODE, 0.0 };
	}
	return characterInfos[charIndex];
}

std::size_t noxcain::FontResource::getGlyphCount() const
{
	return point_data_offset_resource_ids.size();
}

const noxcain::BoundingBox noxcain::FontResource::getCharBoundingBox( UINT32 unicode ) const
{
	std::size_t charIndex = unicode > unicode_map.size() ? 0 : unicode_map[unicode];
	if( characterInfos[charIndex].glyph_index == INVALID_UNICODE )
	{
		return BoundingBox();
	}
	std::array<FLOAT32, 8> glyphQuad;
	ResourceEngine::get_engine().get_subresources()[vertex_resource_id].getData( glyphQuad.data(), 8 * sizeof( FLOAT32 ), std::size_t( font_offset + characterInfos[charIndex].glyph_index ) * 8 * sizeof( FLOAT32 ) );
	
	return BoundingBox( glyphQuad[0], glyphQuad[7], 0.0, glyphQuad[6], glyphQuad[1], 0.0 );
}
