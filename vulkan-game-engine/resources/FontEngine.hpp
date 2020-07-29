#pragma once
#include <Defines.hpp>

#ifdef WIN32
#include <windows/Windows.hpp>
#elif defined(ANDROID) || defined(__ANDROID__)
#include <android/AndroidSurface.hpp>
#endif


#include <vector>

namespace noxcain
{	
	class FontEngine
	{
		friend class ResourceEngine;
	private:
		UINT32 glyphCount = 0;
		UINT16 unitsPerEm = 0;
		FLOAT32 ascender = 0;
		FLOAT32	descender = 0;
		FLOAT32 lineGap = 0;

		std::vector<FLOAT32> advanceWidths;
		std::vector<FLOAT32> leftBearings;
		//std::vector<FLOAT32> advanceHeights;
		//std::vector<FLOAT32> topBearings;
		std::vector<UINT32>  glyphIndices;

		std::vector<FLOAT32> glyphCorners;
		std::vector<std::vector<FLOAT32>> pointMaps;
		std::vector<std::vector<UINT32>>  offsetMaps;

		//DEBUG
	public:
		struct Line
		{
			FLOAT32 start_x = 0.0;
			FLOAT32 start_y = 0.0;

			FLOAT32 middle_x = 0.0;
			FLOAT32 middle_y = 0.0;

			FLOAT32 end_x = 0.0;
			FLOAT32 end_y = 0.0;
		};
		std::vector<Line> lines;
	
		const std::vector<Line>& get_lines()
		{
			return lines;
		}
	private:

		bool isLittleEndian = false;
#ifdef WIN32
		WindowsFile font;
#elif defined(ANDROID) || defined(__ANDROID__)
		AndroidFile font;
#endif
		std::vector<UINT32> unicode_map;

		void createUnicodeMap( UINT32 offset );
		UINT32 getNumGlyphs( UINT32 offset );
		bool isLongOffset( UINT32 offset );

		void createHorizontalMetrics( UINT32 hheaOffset, UINT32 hmtxOffset );
		void createVerticalMetrics( UINT32 vheaOffset, UINT32 vmtxOffset );
		void computeGlyph( UINT32 tableOffset, const std::vector<UINT32>& glyphOffsets );
		void readSimpleGlyph( std::vector<UINT16>& endPointIndices, std::vector<BYTE>& flags, std::vector<DOUBLE>& xCoords, std::vector<DOUBLE>& yCoords );
		void readCompositeGlyph();

		DOUBLE readF2Dot14();

		template<typename T>
		T read()
		{
			T value;
			constexpr UINT32 size = sizeof( T );
			font.read( reinterpret_cast<char*>( &value ), size );

			if( isLittleEndian )
			{
				for( UINT32 index = 0; index < size / 2; ++index )
				{
					BYTE* bytes = reinterpret_cast<BYTE*>( &value );
					BYTE byte = bytes[index];
					bytes[index] = bytes[size - 1 - index];
					bytes[size - 1 - index] = byte;
				}
			}
			return value;
		}

	public:
		static constexpr UINT32 INVALID_UNICODE = 0xFFFFFF;
		FontEngine() = default;
		bool readFont( const std::string& fontPath );

		UINT32 getGlyphCount() const
		{
			return glyphCount;
		}

		const std::vector<std::vector<FLOAT32>>& getGlyphPointMaps() const
		{
			return pointMaps;
		}
		
		const std::vector<std::vector<UINT32>>& getGlyphOffsetMaps() const
		{
			return offsetMaps;
		}

		const std::vector<FLOAT32>& getGlyphCorners() const
		{
			return glyphCorners;
		}

		FLOAT32 getAscender() const { return ascender; }
		FLOAT32 getDescender() const { return descender; }
		FLOAT32 getLineGap() const { return lineGap; }
	};
}