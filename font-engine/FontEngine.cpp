#include "FontEngine.hpp"

#include <fstream>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>

#include <resources/ResourceTools.hpp>


void noxcain::FontEngine::create_unicode_map( EndianSafeStream& es_stream )
{
	UINT16 cmapVersion = es_stream.read<UINT16>();
	UINT16 nCmapTables = es_stream.read<UINT16>();

	struct EncodingRecord
	{
		UINT16 platformId;
		UINT16 encodingId;
		UINT32 offset;
	};
	std::vector<EncodingRecord> encodingRecords( nCmapTables );

	for( UINT32 encodingIndex = 0; encodingIndex < nCmapTables; ++encodingIndex )
	{
		encodingRecords[encodingIndex].platformId = es_stream.read<UINT16>();
		encodingRecords[encodingIndex].encodingId = es_stream.read<UINT16>();
		encodingRecords[encodingIndex].offset = es_stream.read<UINT32>();
	}

	struct Format4
	{
		UINT16 format = 0;
		UINT16 length = 0;
		UINT16 language = 0;
		UINT16 segCountX2 = 0;
		UINT16 searchRange = 0;
		UINT16 entrySelector = 0;
		UINT16 rangeShift = 0;
		std::vector<UINT16> endCode;
		UINT16 reservedPad = 0;
		std::vector<UINT16> startCode;
		std::vector<INT16> idDelta;
		std::vector<UINT16> idRangeOffset;
		UINT64 glyphIdArray = 0;
	} format4;

	bool has_record = false;
	std::streampos base_stream_pos = es_stream.tellg();
	for( const EncodingRecord& record : encodingRecords )
	{
		if( record.platformId == 3 && record.encodingId == 1 )
		{
			es_stream.seekg( base_stream_pos + std::streamoff( record.offset ) );
			format4.format = es_stream.read<UINT16>();
			format4.length = es_stream.read<UINT16>();
			format4.language = es_stream.read<UINT16>();
			format4.segCountX2 = es_stream.read<UINT16>();
			format4.searchRange = es_stream.read<UINT16>();
			format4.entrySelector = es_stream.read<UINT16>();
			format4.rangeShift = es_stream.read<UINT16>();

			format4.endCode.resize( format4.segCountX2 / 2 );
			for( UINT32 index = 0; index < format4.endCode.size(); ++index )
			{
				format4.endCode[index] = es_stream.read<UINT16>();
			}
			format4.reservedPad = es_stream.read<UINT16>();

			format4.startCode.resize( format4.segCountX2 / 2 );
			for( UINT32 index = 0; index < format4.startCode.size(); ++index )
			{
				format4.startCode[index] = es_stream.read<UINT16>();
			}

			format4.idDelta.resize( format4.segCountX2 / 2 );
			for( UINT32 index = 0; index < format4.idDelta.size(); ++index )
			{
				format4.idDelta[index] = es_stream.read<INT16>();
			}

			format4.idRangeOffset.resize( format4.segCountX2 / 2 );
			for( UINT32 index = 0; index < format4.idRangeOffset.size(); ++index )
			{
				format4.idRangeOffset[index] = es_stream.read<UINT16>();
			}

			format4.glyphIdArray = es_stream.tellg();
			has_record = true;
			break;
		}
	}

	if( has_record )
	{
		unicode_map.reserve( glyph_count );

		for( UINT32 segIndex = 0; segIndex < format4.endCode.size(); ++segIndex )
		{
			if( format4.startCode[segIndex] != 0xFFFF && format4.endCode[segIndex] != 0xFFFF )
			{
				unicode_ranges.push_back( { format4.startCode[segIndex], format4.endCode[segIndex] } );
				for( UINT32 unicode = format4.startCode[segIndex]; unicode <= format4.endCode[segIndex]; ++unicode )
				{
					if( format4.idRangeOffset[segIndex] == 0 )
					{
						unicode_map.push_back( ( format4.idDelta[segIndex] + unicode ) % 0x10000 );
					}
					else
					{
						UINT16 glyph_array_index = format4.glyphIdArray + sizeof( UINT16 ) * segIndex - format4.segCountX2 + sizeof( UINT16 ) * ( format4.startCode[segIndex] - unicode ) + format4.idRangeOffset[segIndex];
						es_stream.seekg( glyph_array_index );
						unicode_map.push_back( es_stream.read<UINT16>() );
					}
				}
			}
		}
	}
}

noxcain::UINT16 noxcain::FontEngine::get_num_glyphs( EndianSafeStream& es_stream )
{
	UINT16 major = es_stream.read<UINT16>();
	UINT16 minor = es_stream.read<UINT16>();
	return es_stream.read<UINT16>();
}

bool noxcain::FontEngine::is_offset_long( EndianSafeStream& es_stream )
{
	UINT16 majorVersion = es_stream.read<UINT16>();
	UINT16 minorVersion = es_stream.read<UINT16>();

	UINT16 majorFontVersion = es_stream.read<UINT16>();
	UINT16 minorFontVersion = es_stream.read<UINT16>();

	UINT32 checkSumAdjustment = es_stream.read<UINT32>();
	UINT32 magicNumber = es_stream.read<UINT32>();

	/*
	Bit 0: Baseline for font at y = 0;
	Bit 1: Left sidebearing point at x = 0 ( relevant only for TrueType rasterizers ) � see the note below regarding variable fonts;
	Bit 2: Instructions may depend on point size;
	Bit 3: Force ppem to integer values for all internal scaler math; may use fractional ppem sizes if this bit is clear;
	Bit 4: Instructions may alter advance width( the advance widths might not scale linearly );
	Bit 5: This bit is not used in OpenType, and should not be set in order to ensure compatible behavior on all platforms.If set, it may result in different behavior for vertical layout in some platforms. ( See Apple�s specification for details regarding behavior in Apple platforms. )
	Bits 6�10: These bits are not used in Opentype and should always be cleared. ( See Apple�s specification for details regarding legacy used in Apple platforms. )
	Bit 11: Font data is �lossless� as a result of having been subjected to optimizing transformation and/or compression( such as e.g.compression mechanisms defined by ISO / IEC 14496 - 18, MicroType Express, WOFF 2.0 or similar ) where the original font functionality and features are retained but the binary compatibility between input and output font files is not guaranteed.As a result of the applied transform, the DSIG table may also be invalidated.
	Bit 12: Font converted( produce compatible metrics )
	Bit 13: Font optimized for ClearType�.Note, fonts that rely on embedded bitmaps( EBDT ) for rendering should not be considered optimized for ClearType, and therefore should keep this bit cleared.
	Bit 14: Last Resort font.If set, indicates that the glyphs encoded in the 'cmap' subtables are simply generic symbolic representations of code point ranges and don�t truly represent support for those code points.If unset, indicates that the glyphs encoded in the 'cmap' subtables represent proper support for those code points.
	Bit 15: Reserved, set to 0
	*/
	UINT16 flags = es_stream.read<UINT16>();
	if( ( flags & 0x3 ) != 0x3 )
	{
		int stop = 1;
	}
	units_per_em = es_stream.read<UINT16>();
	UINT64 created = es_stream.read<UINT64>();
	UINT64 modified = es_stream.read<UINT64>();
	INT16 xMin = es_stream.read<INT16>();
	INT16 yMin = es_stream.read<INT16>();
	INT16 xMax = es_stream.read<INT16>();
	INT16 yMax = es_stream.read<INT16>();
	/*
	Bit 0 : Bold( if set to 1 );
	Bit 1: Italic( if set to 1 )
	Bit 2 : Underline( if set to 1 )
	Bit 3 : Outline( if set to 1 )
	Bit 4 : Shadow( if set to 1 )
	Bit 5 : Condensed( if set to 1 )
	Bit 6 : Extended( if set to 1 )
	Bits 7�15 : Reserved( set to 0 )
	*/
	UINT16 macStyle = es_stream.read<UINT16>();

	UINT16 lowestRecPPEM = es_stream.read<UINT16>();

	/*
	0 : Fully mixed directional glyphs;
	1: Only strongly left to right;
	2: Like 1 but also contains neutrals;
	-1: Only strongly right to left;
	-2: Like - 1 but also contains neutrals.
	*/
	INT16 fontDirectionHint = es_stream.read<INT16>();
	INT16 indexToLocFormat = es_stream.read<INT16>();
	INT16 glyphDataFormat = es_stream.read<INT16>();
	return indexToLocFormat;
}

void noxcain::FontEngine::create_horizontal_metrics( EndianSafeStream& es_stream, UINT32 hheaOffset, UINT32 hmtxOffset )
{
	es_stream.seekg( hheaOffset );

	UINT16 	majorVersion = es_stream.read<UINT16>();
	UINT16 	minorVersion = es_stream.read<UINT16>();
	ascender = FLOAT32( es_stream.read<INT16>() ) / units_per_em;
	descender = FLOAT32( es_stream.read<INT16>() ) / units_per_em;
	line_gap = FLOAT32( es_stream.read<INT16>() ) / units_per_em;
	UINT16 	advanceWidthMax = es_stream.read<UINT16>();
	INT16 	minLeftSideBearing = es_stream.read<INT16>();
	INT16 	minRightSideBearing = es_stream.read<INT16>();
	INT16 	xMaxExtent = es_stream.read<INT16>() / units_per_em;
	INT16 	caretSlopeRise = es_stream.read<INT16>();
	INT16 	caretSlopeRun = es_stream.read<INT16>();
	INT16 	caretOffset = es_stream.read<INT16>();
	INT16 reserved;
	reserved = es_stream.read<INT16>();
	reserved = es_stream.read<INT16>();
	reserved = es_stream.read<INT16>();
	reserved = es_stream.read<INT16>();
	INT16 	metricDataFormat = es_stream.read<INT16>();
	UINT16 	numberOfHMetrics = es_stream.read<UINT16>();

	es_stream.seekg( hmtxOffset );
	for( UINT32 index = 0; index < numberOfHMetrics; ++index )
	{
		advance_widths[index] = FLOAT32( es_stream.read<INT16>() ) / units_per_em;
		left_bearings[index] = FLOAT32( es_stream.read<INT16>() ) / units_per_em;
	}

	for( UINT32 index = numberOfHMetrics; index < advance_widths.size(); ++index )
	{
		advance_widths[index] = advance_widths[numberOfHMetrics - 1];
		left_bearings[index] = FLOAT32( es_stream.read<INT16>() ) / units_per_em;
	}
}

void noxcain::FontEngine::compute_glyph( EndianSafeStream& es_stream, const std::vector<UINT32>& glyph_offsets )
{
	struct RawSimpleGlyph
	{
		std::vector<UINT16> endPointIndices;
		std::vector<BYTE> flags;
		std::vector<DOUBLE> xCoords;
		std::vector<DOUBLE> yCoords;
	};
	struct Component
	{
		UINT16 flags = 0;
		UINT16 index = 0;
		DOUBLE argument1 = 0;
		DOUBLE argument2 = 0;

		DOUBLE transformation1 = 1;
		DOUBLE transformation2 = 0;
		DOUBLE transformation3 = 0;
		DOUBLE transformation4 = 1;
	};

	struct CompositeGlyph
	{
		UINT32 glyph_index = 0;
		std::vector<Component> components;
	};

	struct Band
	{
		FLOAT32 end = 0;
		std::vector<UINT32> curveOffsets;
		bool operator==( const Band& otherBand )
		{
			return curveOffsets == otherBand.curveOffsets;
		}
	};

	auto fillBand = []( const std::vector<FLOAT32>& curvePoints, const std::vector<UINT32>& curveStartOffsets, std::vector<Band>& bands, UINT32 offsetOffset, FLOAT32* corners )
	{
		const FLOAT32 glyphWidth = std::abs( corners[offsetOffset] - corners[offsetOffset + 2] );
		const FLOAT32 bandWidth = glyphWidth / bands.size();
		
		UINT32 nBands = bands.size();
		UINT32 nCurves = 0;
		FLOAT32 lastEnd = corners[offsetOffset];
		for( UINT32 bandIndex = 0; bandIndex < bands.size()-1; ++bandIndex )
		{	
			lastEnd = bands[bandIndex].end = lastEnd + bandWidth;
		}

		bands.back().end = corners[offsetOffset + 2] + bandWidth;

		for( UINT32 offset : curveStartOffsets )
		{
			FLOAT32 v1 = curvePoints[std::size_t( offset ) + offsetOffset];
			FLOAT32 v2 = curvePoints[std::size_t( offset ) + offsetOffset + 2];
			FLOAT32 v3 = curvePoints[std::size_t( offset ) + offsetOffset + 4];
			if( v1 == v2 && v2 == v3 )
			{
				continue;
			}

			for( UINT32 bandIndex = 0; bandIndex < bands.size(); ++bandIndex )
			{
				Band& band = bands[bandIndex];
				if( v1 > band.end && v2 > band.end && v3 > band.end )
				{
					continue;
				}

				band.curveOffsets.push_back( offset );
				++nCurves;

				if( v1 <= band.end && v2 <= band.end && v3 <= band.end )
				{
					break;
				}
			}
		}

		for( UINT32 bandIndex = 1; bandIndex < bands.size(); ++bandIndex )
		{
			if( bands[bandIndex] == bands[bandIndex - 1] )
			{
				nCurves -= bands[bandIndex - 1].curveOffsets.size();
				--nBands;
				bands[bandIndex - 1].curveOffsets.clear();
			}

		}

		for( Band& band : bands )
		{
			std::sort( band.curveOffsets.begin(), band.curveOffsets.end(), [&curvePoints, &offsetOffset]( UINT32 offset1, UINT32 offset2 )
			{
				UINT32 off1 = offset1 + 1 - offsetOffset;
				UINT32 off2 = offset2 + 1 - offsetOffset;
				return
					std::max( curvePoints[off1], std::max( curvePoints[std::size_t( off1 ) + 2], curvePoints[std::size_t( off1 ) + 4] ) ) >
					std::max( curvePoints[off2], std::max( curvePoints[std::size_t( off2 )+ 2], curvePoints[std::size_t( off2 ) + 4] ) );
			} );
		}
		
		return std::array<UINT32, 2>( { nBands, nCurves } );
	};


	std::vector<RawSimpleGlyph> rawGlyphs;
	rawGlyphs.reserve( glyph_offsets.size() - 1 );
	std::vector<CompositeGlyph> compositeGlyphs;

	std::streampos stream_start_position = es_stream.tellg();

	for( UINT32 glyph_index = 0; glyph_index < glyph_offsets.size() - 1; ++glyph_index )
	{
		if( glyph_offsets[glyph_index] != glyph_offsets[std::size_t( glyph_index ) + 1] )
		{
			glyph_indices[glyph_index] = rawGlyphs.size();
			
			es_stream.seekg( stream_start_position + std::streamoff( glyph_offsets[glyph_index] ) );

			const INT16 numberOfContours = es_stream.read<INT16>();

			glyph_corners.push_back( FLOAT32( es_stream.read<INT16>() ) / units_per_em ); // minX
			glyph_corners.push_back( FLOAT32( es_stream.read<INT16>() ) / units_per_em ); // minY
			glyph_corners.push_back( FLOAT32( es_stream.read<INT16>() ) / units_per_em ); // maxX
			glyph_corners.push_back( FLOAT32( es_stream.read<INT16>() ) / units_per_em ); // maxY

			rawGlyphs.push_back( RawSimpleGlyph() );

			//read as simple Glyph and unpack
			if( numberOfContours >= 0 )
			{
				rawGlyphs.back().endPointIndices.resize( numberOfContours );
				read_simple_glyph( es_stream, rawGlyphs.back().endPointIndices, rawGlyphs.back().flags, rawGlyphs.back().xCoords, rawGlyphs.back().yCoords );
			}
			//read as Composite glyph
			else
			{
				compositeGlyphs.push_back( CompositeGlyph() );
				CompositeGlyph& newCompositeGlyph = compositeGlyphs.back();
				newCompositeGlyph.glyph_index = glyph_index;

				do
				{
					newCompositeGlyph.components.push_back( Component() );
					Component& newComponent = newCompositeGlyph.components.back();

					newComponent.flags = es_stream.read<UINT16>();
					newComponent.index = es_stream.read<UINT16>();

					switch( newComponent.flags & 0x3 )
					{
						case 0x1:
						{
							newComponent.argument1 = es_stream.read<UINT16>();
							newComponent.argument2 = es_stream.read<UINT16>();
							break;
						}
						case 0x2:
						{
							newComponent.argument1 = es_stream.read<INT8>();
							newComponent.argument2 = es_stream.read<INT8>();
							break;
						}
						case 0x3:
						{
							newComponent.argument1 = es_stream.read<INT16>();
							newComponent.argument2 = es_stream.read<INT16>();
							break;
						}
						default:
						{
							newComponent.argument1 = es_stream.read<UINT8>();
							newComponent.argument2 = es_stream.read<UINT8>();
							break;
						}
					}

					if( newComponent.flags & 0x0008 )
					{
						newComponent.transformation1 = newComponent.transformation4 = read_f2dot14( es_stream );
					}
					else if( newComponent.flags & 0x0040 )
					{
						newComponent.transformation1 = read_f2dot14( es_stream );
						newComponent.transformation4 = read_f2dot14( es_stream );
					}
					else if( newComponent.flags & 0x0080 )
					{
						newComponent.transformation1 = read_f2dot14( es_stream );
						newComponent.transformation2 = read_f2dot14( es_stream );
						newComponent.transformation3 = read_f2dot14( es_stream );
						newComponent.transformation4 = read_f2dot14( es_stream );
					}

					if( newComponent.flags & 0x0200 )
					{
						advance_widths[newCompositeGlyph.glyph_index] = advance_widths[newComponent.index];
						left_bearings[newCompositeGlyph.glyph_index] = left_bearings[newComponent.index];
						//advanceHeights[newCompositeGlyph.glyph_index] = advanceHeights[newComponent.index];
						//topBearings[newCompositeGlyph.glyph_index] = topBearings[newComponent.index];
					}
				} while( newCompositeGlyph.components.back().flags & 0x0020 );
			}
		}
	}

	glyph_count = rawGlyphs.size();

	//combine glyph components;
	for( const CompositeGlyph& compositeGlyph : compositeGlyphs )
	{
		RawSimpleGlyph& newGlyph = rawGlyphs[glyph_indices[compositeGlyph.glyph_index]];
		for( const Component& component : compositeGlyph.components )
		{
			const RawSimpleGlyph& componentGlyph = rawGlyphs[glyph_indices[component.index]];

			DOUBLE offsetX = 0;
			DOUBLE offsetY = 0;

			if( component.flags & 0x0002 )
			{
				offsetX = component.argument1;
				offsetY = component.argument2;
				if( component.flags & 0x0800 )
				{
					offsetX = component.transformation1*offsetX;
					offsetY = component.transformation4*offsetY;
				}
			}
			else
			{
				offsetX = newGlyph.xCoords[UINT16( component.argument2 )] - component.transformation1*componentGlyph.xCoords[UINT16( component.argument1 )];
				offsetY = newGlyph.yCoords[UINT16( component.argument2 )] - component.transformation4*componentGlyph.yCoords[UINT16( component.argument1 )];
			}

			for( const UINT16 idx : componentGlyph.endPointIndices )
			{
				newGlyph.endPointIndices.push_back( newGlyph.flags.size() + idx );
			}

			newGlyph.flags.reserve( newGlyph.flags.size() + componentGlyph.flags.size() );
			newGlyph.xCoords.reserve( newGlyph.xCoords.size() + componentGlyph.xCoords.size() );
			newGlyph.yCoords.reserve( newGlyph.yCoords.size() + componentGlyph.yCoords.size() );

			newGlyph.flags.insert( newGlyph.flags.end(), componentGlyph.flags.begin(), componentGlyph.flags.end() );

			for( UINT32 pointIndex = 0; pointIndex < componentGlyph.flags.size(); ++pointIndex )
			{
				DOUBLE valueX = component.transformation1*componentGlyph.xCoords[pointIndex] + component.transformation2*componentGlyph.yCoords[pointIndex] + offsetX;
				DOUBLE valueY = component.transformation3*componentGlyph.xCoords[pointIndex] + component.transformation4*componentGlyph.yCoords[pointIndex] + offsetY;
				if( component.flags & 0x0004 )
				{
					valueX = INT32( valueX + 0.5 );
					valueY = INT32( valueY + 0.5 );
				}
				newGlyph.xCoords.push_back( valueX );

				newGlyph.yCoords.push_back( valueY );
			}
		}
	}

	point_maps.resize( glyph_count );
	offset_maps.resize( glyph_count );

	//rebuild glyphs 2 map
	for( UINT32 rawGlyphIndex = 0; rawGlyphIndex < rawGlyphs.size(); ++rawGlyphIndex )
	{
		std::vector<UINT32> curveStartOffsets;
		std::vector<FLOAT32> curvePoints;
		
		const RawSimpleGlyph& glyph = rawGlyphs[rawGlyphIndex];
		UINT32 startIndex = 0;
		curvePoints.reserve( 4 * glyph.flags.size() );

		for( UINT16 endIndex : glyph.endPointIndices )
		{
			const UINT32 size = endIndex - startIndex + 1;
			UINT32 pointCount = 0;
			UINT32 startOffset = curvePoints.size();
			bool shouldOnCurve = true;
			for( UINT32 relIndex = 0; relIndex <= size; ++relIndex )
			{	
				UINT32 index = relIndex % size;
				
				FLOAT32 currPointX = glyph.xCoords[std::size_t(startIndex) + index] / units_per_em;
				FLOAT32 currPointY = glyph.yCoords[std::size_t( startIndex ) + index] / units_per_em;

				if( shouldOnCurve == bool( glyph.flags[std::size_t( startIndex ) + index] & 0x1 ) )
				{
					shouldOnCurve = !shouldOnCurve;
				}
				else
				{
					UINT32 prevIndex = ( index + size - 1 ) % size;
					DOUBLE prevPointX = glyph.xCoords[std::size_t( startIndex ) + prevIndex] / units_per_em;
					DOUBLE prevPointY = glyph.yCoords[std::size_t( startIndex ) + prevIndex] / units_per_em;

					curvePoints.push_back( 0.5*( prevPointX + currPointX ) );
					curvePoints.push_back( 0.5*( prevPointY + currPointY ) );
					++pointCount;
				}
				
				if( relIndex < size || pointCount % 2 == 0 )
				{
					curvePoints.push_back( currPointX );
					curvePoints.push_back( currPointY );
					++pointCount;
				}
			}

			for( UINT32 index = 0; index < pointCount-1; index += 2 )
			{
				curveStartOffsets.push_back( startOffset + 2 * index );
			}

			startIndex = endIndex + 1;
		}

		std::vector<Band> xBands( 32 );
		std::array<UINT32, 2> xBandCounts = fillBand( curvePoints, curveStartOffsets, xBands, 0, &glyph_corners[std::size_t( 4 ) * rawGlyphIndex] );

		std::vector<Band> yBands( 32 );
		std::array<UINT32, 2> yBandCounts = fillBand( curvePoints, curveStartOffsets, yBands, 1, &glyph_corners[std::size_t( 4 ) * rawGlyphIndex] );

		const UINT32 bandCount = std::max( xBandCounts[0], yBandCounts[0] );
		const UINT32 curveCount = xBandCounts[1] + yBandCounts[1];

		std::vector<UINT32>& currOffsetMap = offset_maps[rawGlyphIndex];
		std::vector<FLOAT32>& currPointMap = point_maps[rawGlyphIndex];

		/*
		UINT32 Offset map:
		0: count of curves in x band 0
		1: offset for curve offsets in offset map for x band 0
		2: count of curves in y band 0
		3: offset for curve offsets in offset map for y band 0
		0: count of curves in x band 1
		1: offset for curve offsets in offset map for x band 1
		2: count of curves in y band 1
		3: offset for curve offsets in offset map for y band 1
		...
		4*band count + 0: offset in point map for start point of curve 0 in x band 0
		4*band count + 1: offset in point map for start point of curve 1 in x band 0
		...
		4*band count + count of curves in x band 0 + 0: offset in point map for start point of curve 0 in x band 1
		4*band count + count of curves in x band 0 + 1: offset in point map for start point of curve 1 in x band 1
		...
		4*band count + 0: offset in point map for curve start point
		4*band count + 0: offset in point map for curve start point
		*/
		currOffsetMap.resize( std::size_t( bandCount )*4 + curveCount );

		/*
		FLOAT32 Point map:
		0: end of x band 0
		1: end of y band 0
		2: end of x band 1
		3: end of y band 1
		...
		2*band count  : x of point 0
		2*band count+1: y of point 0
		2*band count+2: x of point 1
		2*band count+3: y of point 1
		...
		*/
		currPointMap.reserve( std::size_t( bandCount ) * 2 + curvePoints.size() );
		currPointMap.resize( std::size_t( bandCount ) * 2 );
		currPointMap.insert( currPointMap.end(), curvePoints.begin(), curvePoints.end() );
		
		UINT32 currentBandOffset = 4 * bandCount;
		UINT32 currentBandIndex = 0;
		for( UINT32 bandIndex = 0; bandIndex < xBands.size(); ++bandIndex )
		{
			if( !xBands[bandIndex].curveOffsets.empty() )
			{
				currPointMap[2 * std::size_t( currentBandIndex )] = xBands[bandIndex].end;
				currOffsetMap[4 * std::size_t( currentBandIndex )] = xBands[bandIndex].curveOffsets.size();
				currOffsetMap[4 * std::size_t( currentBandIndex ) + 1] = currentBandOffset;
				for( UINT32 offset : xBands[bandIndex].curveOffsets )
				{
					currOffsetMap[currentBandOffset++] = offset + 2 * bandCount;
				}
				++currentBandIndex;
			}
		}

		currentBandIndex = 0;
		for( UINT32 bandIndex = 0; bandIndex < yBands.size(); ++bandIndex )
		{
			if( !yBands[bandIndex].curveOffsets.empty() )
			{
				currPointMap[2 * std::size_t( currentBandIndex ) + 1] = yBands[bandIndex].end;
				currOffsetMap[4 * std::size_t( currentBandIndex ) + 2] = yBands[bandIndex].curveOffsets.size();
				currOffsetMap[4 * std::size_t( currentBandIndex ) + 3] = currentBandOffset;
				for( UINT32 offset : yBands[bandIndex].curveOffsets )
				{
					currOffsetMap[currentBandOffset++] = offset + 2 * bandCount;
				}
				++currentBandIndex;
			}
		}
	}
}

void noxcain::FontEngine::read_simple_glyph( EndianSafeStream& es_stream, std::vector<UINT16>& endPointIndices, std::vector<BYTE>& flags, std::vector<DOUBLE>& xCoords, std::vector<DOUBLE>& yCoords )
{
	for( UINT32 index = 0; index < endPointIndices.size(); ++index )
	{
		endPointIndices[index] = es_stream.read<UINT16>();
	}

	UINT16 instructionLength = es_stream.read<UINT16>();
	std::vector<BYTE> instructions( instructionLength );

	for( UINT32 index = 0; index < instructions.size(); ++index )
	{
		instructions[index] = es_stream.read<BYTE>();
	}

	while( flags.size() < std::size_t( endPointIndices.back() ) + 1 )
	{
		BYTE flag = es_stream.read<BYTE>();
		flags.push_back( flag );
		if( flag & 0x08 )
		{
			BYTE count = es_stream.read<BYTE>();
			while( count-- )
			{
				flags.push_back( flag );
			}
		}
	}


	BYTE masterflag = 0x02 | 0x10;
	DOUBLE lastValue = 0;
	for( const BYTE flag : flags )
	{
		switch( flag & masterflag )
		{
			case 0x02:
			{
				xCoords.push_back( lastValue - es_stream.read<BYTE>() );
				break;
			}
			case 0x10:
			{
				xCoords.push_back( lastValue );
				break;
			}
			case 0x12:
			{
				xCoords.push_back( lastValue + es_stream.read<BYTE>() );
				break;
			}
			default:
			{
				xCoords.push_back( lastValue + es_stream.read<INT16>() );
				break;
			}
		}
		lastValue = xCoords.back();
	}

	masterflag = 0x04 | 0x20;
	lastValue = 0;
	for( const BYTE flag : flags )
	{
		switch( flag & masterflag )
		{
			case 0x04:
			{
				yCoords.push_back( lastValue - es_stream.read<BYTE>() );
				break;
			}
			case 0x20:
			{
				yCoords.push_back( lastValue );
				break;
			}
			case 0x24:
			{
				yCoords.push_back( lastValue + es_stream.read<BYTE>() );
				break;
			}
			default:
			{
				yCoords.push_back( lastValue + es_stream.read<INT16>() );
				break;
			}
		}
		lastValue = yCoords.back();
	}

	
	int stop = 0;
}

void noxcain::FontEngine::read_composite_glyph( EndianSafeStream& es_stream )
{
	UINT16 flags = 0;
	do
	{
		flags = es_stream.read<UINT16>();
		UINT16 glyph_index = es_stream.read<UINT16>();

		DOUBLE argument1;
		DOUBLE argument2;

		switch( flags & 0x3 )
		{
			case 0x1:
			{
				argument1 = es_stream.read<UINT16>();
				argument2 = es_stream.read<UINT16>();
				break;
			}
			case 0x2:
			{
				argument1 = es_stream.read<INT8>();
				argument2 = es_stream.read<INT8>();
				break;
			}
			case 0x3:
			{
				argument1 = es_stream.read<INT16>();
				argument2 = es_stream.read<INT16>();
				break;
			}
			default:
			{
				argument1 = es_stream.read<UINT8>();
				argument2 = es_stream.read<UINT8>();
				break;
			}
		}

		if( flags & 0x0008 )
		{
			DOUBLE a1 = read_f2dot14( es_stream );
		}
		else if( flags & 0x0040 )
		{
			DOUBLE a1 = read_f2dot14( es_stream );
			DOUBLE a2 = read_f2dot14( es_stream );
		}
		else if( flags & 0x0080 )
		{
			DOUBLE a1 = read_f2dot14( es_stream );
			DOUBLE a2 = read_f2dot14( es_stream );
			DOUBLE a3 = read_f2dot14( es_stream );
			DOUBLE a4 = read_f2dot14( es_stream );
		}
	} while( flags & 0x0020 );
}

noxcain::DOUBLE noxcain::FontEngine::read_f2dot14( std::istream& i_stream )
{
	char word[2];
	i_stream.read( word, 2 );
	signed char front = 0;
	front = front | ( word[0] & 0x80 ) | ( ( word[0] & 0x40 ) >> 6 );
	word[0] &= 0x3F;
	if( is_little_endian )
	{
		char temp = word[1];
		word[1] = word[0];
		word[0] = temp;
	}

	UINT16 frac;
	memcpy( &frac, word, 2 );

	return DOUBLE( front ) + ( DOUBLE(frac) / 0x4000 );
}

bool noxcain::FontEngine::write_font_file( std::string path )
{
	std::ofstream file( path, std::iostream::binary );
	if( file.is_open() )
	{
		
		///// start fix values
		

		//glyph count   UINT32
		//ascender		FLOT32
		//descender     FLOT32
		//line_gap,      FLOT32

		//// per char data

		//unicode ranges 2*UINT32
		//unicode to glyph index UINT32*glyphcount
		
		//char advance width FLOAT32 * glyph count
		//char to outlineset index UINT32 * glyph count
		
		//corners 8*FLOAT32

		///// per outlineset data

		//offset block
		//point block

		
		std::size_t buffer_size =
			sizeof( UINT32 ) + //glyph count
			sizeof( FLOAT32 ) + //ascender
			sizeof( FLOAT32 ) + //descender
			sizeof( FLOAT32 ) + //line gap

			sizeof( UINT32 ) + //range count
			unicode_ranges.size() * 2 * sizeof( UINT32 ) + //unicode ranges
			glyph_count * sizeof( UINT32 ) + //unicode to glyph
			
			glyph_count * ( sizeof( UINT32 ) + sizeof( FLOAT32 ) ) + //advance width + outline set index

			sizeof( UINT32 ) + //outlineset count
			glyph_corners.size() * sizeof( FLOAT32 ); //corners

		for( const auto& offset_map : offset_maps )
		{
			//field for point map size + map size
			buffer_size += sizeof( UINT32 ) + offset_map.size() * sizeof( UINT32 );
		}
		
		for( const auto& point_map : point_maps )
		{
			//field for point map size + map size
			buffer_size += sizeof( UINT32 ) + point_map.size() * sizeof( FLOAT32 );
		}

		std::vector<BYTE> buffer( buffer_size );

		UINT32 offset = 0;

		//glyph count
		offset = add_to_buffer( buffer, offset, glyph_count );

		//ascender
		offset = add_to_buffer( buffer, offset, ascender );

		//descender
		offset = add_to_buffer( buffer, offset, descender );

		//line_gap
		offset = add_to_buffer( buffer, offset, line_gap );

		//unicode ranges
		offset = add_to_buffer( buffer, offset, UINT32( unicode_ranges.size() ) );
		for( const auto& range : unicode_ranges )
		{
			offset = add_to_buffer( buffer, offset, range.start_code );
			offset = add_to_buffer( buffer, offset, range.end_code );
		}

		//unicode map
		for( const auto& glyph_index : unicode_map )
		{
			offset = add_to_buffer( buffer, offset, glyph_index );
		}

		//glyph data
		for( std::size_t index = 0; index < glyph_count; ++index )
		{
			offset = add_to_buffer( buffer, offset, advance_widths[index] );
			offset = add_to_buffer( buffer, offset, glyph_indices[index] );
		}

		//outline sets
		offset = add_to_buffer( buffer, offset, UINT32( offset_maps.size() ) );
		
		for( const auto& corner_coord : glyph_corners )
		{
			offset = add_to_buffer( buffer, offset, corner_coord );
		}
		
		for( const auto& offset_map : offset_maps )
		{
			offset = add_to_buffer( buffer, offset, UINT32( offset_map.size() ) );
			for( UINT32 offset_entry : offset_map )
			{
				offset = add_to_buffer( buffer, offset, offset_entry );
			}
		}

		for( const auto& point_map : point_maps )
		{
			offset = add_to_buffer( buffer, offset, UINT32( point_map.size() ) );
			for( FLOAT32 point_coord : point_map )
			{
				offset = add_to_buffer( buffer, offset, point_coord );
			}
		}

		if( offset == buffer_size )
		{
			file.write( (const char*)buffer.data(), buffer.size() );
			if( !file.bad() )
			{
				file.close();
				return true;
			}
		}
	}
	return false;
}

bool noxcain::FontEngine::read_font( const std::string& input_path, const std::string& output_path )
{
	std::ifstream font_file = std::ifstream( input_path, std::fstream::binary );
	if( font_file.is_open() )
	{
		UINT32 endianCheck = 1;
		bool is_little_endian = reinterpret_cast<unsigned char*>( &endianCheck )[0] > 0;

		EndianSafeStream es_stream( font_file.rdbuf(), is_little_endian );

		const UINT32 version = es_stream.read<UINT32>();
		const UINT16 nTables = es_stream.read<UINT16>();
		const UINT16 searchRange = es_stream.read<UINT16>();
		const UINT16 entrySelector = es_stream.read<UINT16>();
		const UINT16 rangeShift = es_stream.read<UINT16>();

		struct TableRecord
		{
			UINT32 tag = 0;
			UINT32 checkSum = 0;
			UINT32 offset = 0;
			UINT32 length = 0;
		} cmap, loca, glyf, maxp, head, hhea, vhea, hmtx, vmtx;
		
		for( UINT32 tableIndex = 0; tableIndex < nTables; ++tableIndex )
		{
			TableRecord record;
			record.tag = es_stream.read<UINT32>();
			record.checkSum = es_stream.read<UINT32>();
			record.offset = es_stream.read<UINT32>();
			record.length = es_stream.read<UINT32>();

			switch( record.tag )
			{
				case 'cmap':
					cmap = record;
					break;
				case 'loca':
					loca = record;
					break;
				case 'glyf':
					glyf = record;
					break;
				case 'maxp':
					maxp = record;
					break;
				case 'head':
					head = record;
					break;
				case 'hhea':
					hhea = record;
					break;
				case 'vhea':
					vhea = record;
					break;
				case 'hmtx':
					hmtx = record;
					break;
				case 'vmtx':
					vmtx = record;
					break;
				default:
					break;
			}
		};

		const UINT16 nGlyphs = get_num_glyphs( es_stream.seekg( maxp.offset ) );
		const bool longOffset = is_offset_long( es_stream.seekg( head.offset ) );
		
		//advanceHeights.resize( nGlyphs );
		advance_widths.resize( nGlyphs );

		//topBearings.resize( nGlyphs );
		left_bearings.resize( nGlyphs );

		glyph_indices.resize( nGlyphs, INVALID_UNICODE );

		create_horizontal_metrics( es_stream, hhea.offset, hmtx.offset );
		/*
		if( vhea.length > 0 )
		{
			create_vertical_metrics( vhea.offset, vmtx.offset );
		}
		*/

		es_stream.seekg( loca.offset );
		std::vector<UINT32> glyph_table_offsets( std::size_t( nGlyphs ) + 1 );
		for( UINT32 glyph_index = 0; glyph_index < nGlyphs + 1; ++glyph_index )
		{
			glyph_table_offsets[glyph_index] = longOffset ? es_stream.read<UINT32>() : UINT32( 2 )*es_stream.read<UINT16>();
		}

		compute_glyph( es_stream.seekg( glyf.offset ), glyph_table_offsets );

		create_unicode_map( es_stream.seekg( cmap.offset )  );
		
		if( glyph_count )
		{
			return write_font_file( output_path );
		}
	}
	return false;
}


