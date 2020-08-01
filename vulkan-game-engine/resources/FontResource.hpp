#pragma once
#include <Defines.hpp>
#include <vector>

namespace noxcain
{
	class BoundingBox;
	
	class FontResource
	{
	public:
		static constexpr UINT32 INVALID_UNICODE = 0xFFFFFF;

		struct CharacterInfo
		{
			UINT32 glyph_index = INVALID_UNICODE;
			DOUBLE advance_width = 0;
		};

		struct UnicodeRange
		{
			UINT32 start = 0;
			UINT32 end = 0;
			UINT32 index = 0;
		};
		
		FontResource( UINT32 font_offset,
					  std::vector<UnicodeRange>&& unicode_map,
					  std::vector<CharacterInfo>&& character_infos,
					  DOUBLE ascender,
					  DOUBLE descender,
					  DOUBLE line_gap,
					  std::vector<std::size_t>&& point_data_offset_resource_ids,
					  std::vector<std::size_t>&& point_data_resource_ids,
					  std::size_t vertex_resource_id );

		CharacterInfo get_character_info( UINT32 unicode ) const;

		UINT32 get_font_offset() const
		{
			return font_offset;
		}

		DOUBLE get_ascender() const
		{
			return ascender;
		}

		DOUBLE get_descender() const
		{
			return descender;
		}

		DOUBLE get_line_gap() const
		{
			return line_gap;
		}

		std::size_t get_glyph_count() const;

		const std::vector<std::size_t>& get_offset_map_resource_ids() const
		{
			return point_data_offset_resource_ids;
		}
		const std::vector<std::size_t>& get_point_map_resource_ids() const
		{
			return point_data_resource_ids;
		}

		std::size_t get_vertex_block_id() const
		{
			return vertex_resource_id;
		}

		const BoundingBox get_character_bounding_box( UINT32 unicode ) const;

	private:

		std::size_t vertex_resource_id;
		std::vector<std::size_t> point_data_offset_resource_ids;
		std::vector<std::size_t> point_data_resource_ids;

		UINT32 font_offset = 0;

		const DOUBLE ascender;
		const DOUBLE descender;
		const DOUBLE line_gap;

		/// <summary>
		/// search for glyph index
		/// </summary>
		/// <param name="unicode"></param>
		/// <returns></returns>
		UINT32 get_character_index( UINT32 unicode ) const;
		const std::vector<UnicodeRange> unicode_map;
		
		const std::vector<CharacterInfo> character_infos;
		//const std::vector<DOUBLE> left_bearings;
		//const std::vector<UINT32> glyph_indices;
	};
}