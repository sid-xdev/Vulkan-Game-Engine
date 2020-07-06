#pragma once
#include <Defines.hpp>

#include <fstream>
#include <vector>

namespace noxcain
{
	class EndianSafeStream;

	class FontEngine
	{
	public:
		static constexpr UINT32 INVALID_UNICODE = 0xFFFFFF;
		FontEngine() = default;
		bool read_font( const std::string& input_path, const std::string& output_path );

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

		bool is_little_endian = false;
		struct UnicodeRange
		{
			UINT32 start_code = 0;
			UINT32 end_code = 0;
		};
		std::vector<UnicodeRange>unicode_ranges;
		std::vector<UINT32> unicode_map;

		void create_unicode_map( EndianSafeStream& es_stream );
		UINT16 get_num_glyphs( EndianSafeStream& es_stream );
		bool is_offset_long( EndianSafeStream& es_stream );

		void create_horizontal_metrics( EndianSafeStream& es_stream, UINT32 hheaOffset, UINT32 hmtxOffset );
		//void create_vertical_metrics( UINT32 vheaOffset, UINT32 vmtxOffset );
		void compute_glyph( EndianSafeStream& es_stream, const std::vector<UINT32>& glyphOffsets );
		void read_simple_glyph( EndianSafeStream& es_stream,std::vector<UINT16>& endPointIndices, std::vector<BYTE>& flags, std::vector<DOUBLE>& xCoords, std::vector<DOUBLE>& yCoords );
		void read_composite_glyph( EndianSafeStream& es_stream );

		DOUBLE read_f2dot14( std::istream& i_stream );

		bool write_font_file( std::string path );

		template<typename T>
		static UINT32 add_to_buffer( std::vector<BYTE>& buffer, UINT32 offset, T value );
	};
	
	template<typename T>
	inline UINT32 FontEngine::add_to_buffer( std::vector<BYTE>& buffer, UINT32 offset, T value )
	{
		*( reinterpret_cast<T*>( buffer.data() + offset ) ) = value;
		return offset + sizeof( value );
	}
}