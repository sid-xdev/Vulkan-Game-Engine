#pragma once
#include <Defines.hpp>

#include <ResourceFile.hpp>
#include <vector>

namespace noxcain
{	
	class FontEngine
	{
		friend class ResourceEngine;
	private:
		UINT32 glyph_count = 0;
		UINT16 units_per_em = 0;
		FLOAT32 ascender = 0;
		FLOAT32	descender = 0;
		FLOAT32 line_gap = 0;

		std::vector<FLOAT32> advance_widths;
		std::vector<FLOAT32> left_bearings;
		//std::vector<FLOAT32> advanceHeights;
		//std::vector<FLOAT32> topBearings;
		std::vector<UINT32>  glyph_indices;

		std::vector<FLOAT32> glyph_corners;
		std::vector<std::vector<FLOAT32>> point_maps;
		std::vector<std::vector<UINT32>>  offset_maps;
		std::vector<UINT32> unicode_ranges;
		
		void createUnicodeMap( ResourceFile& font_file, UINT32 offset );
		UINT32 getNumGlyphs( ResourceFile& font_file, UINT32 offset );
		bool isLongOffset( ResourceFile& font_file, UINT32 offset );

		void createHorizontalMetrics( ResourceFile& font_file, UINT32 hheaOffset, UINT32 hmtxOffset );
		void createVerticalMetrics( ResourceFile& font_file, UINT32 vheaOffset, UINT32 vmtxOffset );
		void computeGlyph( ResourceFile& font_file, UINT32 tableOffset, const std::vector<UINT32>& glyphOffsets );
		void readSimpleGlyph( ResourceFile& font_file, std::vector<UINT16>& endPointIndices, std::vector<BYTE>& flags, std::vector<DOUBLE>& xCoords, std::vector<DOUBLE>& yCoords );
		void readCompositeGlyph( ResourceFile& font_file );

	public:
		static constexpr UINT32 INVALID_UNICODE = 0xFFFFFF;
		FontEngine() = default;
		bool readFont( const std::string& fontPath );

		UINT32 get_glyph_count() const
		{
			return glyph_count;
		}

		const std::vector<std::vector<FLOAT32>>& getGlyphPointMaps() const
		{
			return point_maps;
		}
		
		const std::vector<std::vector<UINT32>>& getGlyphOffsetMaps() const
		{
			return offset_maps;
		}

		const std::vector<FLOAT32>& getGlyphCorners() const
		{
			return glyph_corners;
		}

		FLOAT32 get_ascender() const { return ascender; }
		FLOAT32 get_descender() const { return descender; }
		FLOAT32 get_line_gap() const { return line_gap; }
	};
}