#pragma once
#include <Defines.hpp>
#include <vector>

#include <resources/BoundingBox.hpp>

namespace noxcain
{
	class FontResource
	{
	public:
		static constexpr UINT32 INVALID_UNICODE = 0xFFFFFF;

		struct CharacterInfo
		{
			UINT32 glyph_index = INVALID_UNICODE;
			DOUBLE advance_width = 0;
		};
		
		FontResource( UINT32 font_offset,
					  std::vector<UINT32>&& unicode_map,
					  std::vector<CharacterInfo>&& characterInfos,
					  DOUBLE ascender,
					  DOUBLE descender,
					  DOUBLE lineGap,
					  std::vector<std::size_t>&& point_data_offset_resource_ids,
					  std::vector<std::size_t>&& point_data_resource_ids,
					  std::size_t vertex_resource_id );

		CharacterInfo getCharacterInfo( UINT32 unicode ) const;

		UINT32 get_font_offset() const
		{
			return font_offset;
		}

		DOUBLE getAscender() const
		{
			return ascender;
		}

		DOUBLE getDescender() const
		{
			return descender;
		}

		DOUBLE getLineGap() const
		{
			return lineGap;
		}

		std::size_t getGlyphCount() const;

		const std::vector<std::size_t>& getOffsetSubResourceIds() const
		{
			return point_data_offset_resource_ids;
		}
		const std::vector<std::size_t>& getPointSubResourceIds() const
		{
			return point_data_resource_ids;
		}

		std::size_t getVertexBlockId() const
		{
			return vertex_resource_id;
		}

		const BoundingBox getCharBoundingBox( UINT32 unicode ) const;

	private:

		std::size_t vertex_resource_id;
		std::vector<std::size_t> point_data_offset_resource_ids;
		std::vector<std::size_t> point_data_resource_ids;

		UINT32 font_offset = 0;

		const DOUBLE ascender;
		const DOUBLE descender;
		const DOUBLE lineGap;

		const std::vector<UINT32> unicode_map;
		
		const std::vector<CharacterInfo> characterInfos;
		//const std::vector<DOUBLE> leftBearings;
		//const std::vector<UINT32> glyphIndices;
	};
}