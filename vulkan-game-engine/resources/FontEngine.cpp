#include "FontEngine.hpp"

#include <ResourceFile.hpp>
#include <fstream>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>

void noxcain::FontEngine::createUnicodeMap( ResourceFile& font_file, UINT32 offset )
{
	font_file.set_position( offset );

	UINT16 cmapVersion = font_file.read_fundamental<UINT16>();
	UINT16 nCmapTables = font_file.read_fundamental<UINT16>();

	struct EncodingRecord
	{
		UINT16 platformId;
		UINT16 encodingId;
		UINT32 offset;
	};
	std::vector<EncodingRecord> encodingRecords( nCmapTables );

	for( UINT32 encodingIndex = 0; encodingIndex < nCmapTables; ++encodingIndex )
	{
		encodingRecords[encodingIndex].platformId = font_file.read_fundamental<UINT16>();
		encodingRecords[encodingIndex].encodingId = font_file.read_fundamental<UINT16>();
		encodingRecords[encodingIndex].offset = font_file.read_fundamental<UINT32>();
	}

	struct Format4
	{
		UINT32 format = 0;
		UINT32 length = 0;
		UINT32 language = 0;
		UINT32 segCountX2 = 0;
		UINT32 searchRange = 0;
		UINT32 entrySelector = 0;
		UINT32 rangeShift = 0;
		std::vector<UINT32> endCode;
		std::vector<UINT32> startCode;
		std::vector<UINT32> idDelta;
		std::vector<UINT32> idRangeOffset;
		UINT64 glyphIdArray = 0;
	} format4;

	bool has_record = false;
	for( const EncodingRecord& record : encodingRecords )
	{
		if( record.platformId == 3 && record.encodingId == 1 )
		{
			font_file.set_position( UINT64( offset ) + record.offset );
			format4.format = font_file.read_fundamental<UINT16>();
			format4.length = font_file.read_fundamental<UINT16>();
			format4.language = font_file.read_fundamental<UINT16>();
			format4.segCountX2 = font_file.read_fundamental<UINT16>();
			format4.searchRange = font_file.read_fundamental<UINT16>();
			format4.entrySelector = font_file.read_fundamental<UINT16>();
			format4.rangeShift = font_file.read_fundamental<UINT16>();

			format4.endCode.resize( format4.segCountX2 / 2 );
			for( UINT32 index = 0; index < format4.endCode.size(); ++index )
			{
				format4.endCode[index] = font_file.read_fundamental<UINT16>();
			}
			font_file.read_fundamental<UINT16>(); //reserve pad

			format4.startCode.resize( format4.segCountX2 / 2 );
			for( UINT32 index = 0; index < format4.startCode.size(); ++index )
			{
				format4.startCode[index] = font_file.read_fundamental<UINT16>();
			}

			format4.idDelta.resize( format4.segCountX2 / 2 );
			for( UINT32 index = 0; index < format4.idDelta.size(); ++index )
			{
				format4.idDelta[index] = font_file.read_fundamental<UINT16>();
			}

			format4.idRangeOffset.resize( format4.segCountX2 / 2 );
			for( UINT32 index = 0; index < format4.idRangeOffset.size(); ++index )
			{
				format4.idRangeOffset[index] = font_file.read_fundamental<UINT16>();
			}

			format4.glyphIdArray = font_file.get_position();
			has_record = true;
			break;
		}
	}

	if( has_record )
	{
		const UINT32 segment_count = format4.endCode.size();

		unicode_ranges.reserve( segment_count );
		for( UINT32 segIndex = 0; segIndex < segment_count; ++segIndex )
		{
			if( format4.idRangeOffset[segIndex] == 0 )
			{
				unicode_ranges.emplace_back( format4.startCode[segIndex] );
				unicode_ranges.emplace_back( format4.endCode[segIndex] );
				unicode_ranges.emplace_back( ( format4.idDelta[segIndex] + format4.startCode[segIndex] ) % 65536 );
			}
			else
			{
				std::size_t count = std::size_t(format4.endCode[segIndex]) - format4.startCode[segIndex] + 1;
				for( std::size_t index = 0; index < count; ++index )
				{
					//get back from glyphId array to start of idRangeOffset array and then offset to current idRangOffset entry ( - segment count + segment id )
					//next add id range offset and step back into glyph Id Array and add offset from range start ( delta( start code, unicode )
					UINT16 glyphArrayIndex = format4.glyphIdArray + sizeof( UINT16 )*( index + segIndex - segment_count ) + format4.idRangeOffset[segIndex];
					font_file.set_position( glyphArrayIndex );
					UINT32 glyph_index = font_file.read_fundamental<UINT16>();
					if( glyph_index )
					{
						glyph_index = ( glyph_index + format4.idDelta[segIndex] ) % 65536;
						unicode_ranges.emplace_back( format4.startCode[segIndex] + index );
						unicode_ranges.emplace_back( format4.startCode[segIndex] + index );
						unicode_ranges.emplace_back( glyph_index );
					}
				}
			}
		}
	}
}

noxcain::UINT32 noxcain::FontEngine::getNumGlyphs( ResourceFile& font_file, UINT32 offset )
{
	font_file.set_position( offset );
	UINT16 major = font_file.read_fundamental<UINT16>();
	UINT16 minor = font_file.read_fundamental<UINT16>();
	return font_file.read_fundamental<UINT16>();
}

bool noxcain::FontEngine::isLongOffset( ResourceFile& font_file, UINT32 offset )
{
	font_file.set_position( offset );

	UINT16 majorVersion = font_file.read_fundamental<UINT16>();
	UINT16 minorVersion = font_file.read_fundamental<UINT16>();

	UINT16 majorFontVersion = font_file.read_fundamental<UINT16>();
	UINT16 minorFontVersion = font_file.read_fundamental<UINT16>();

	UINT32 checkSumAdjustment = font_file.read_fundamental<UINT32>();
	UINT32 magicNumber = font_file.read_fundamental<UINT32>();

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
	UINT16 flags = font_file.read_fundamental<UINT16>();
	if( ( flags & 0x3 ) != 0x3 )
	{
		int stop = 1;
	}
	units_per_em = font_file.read_fundamental<UINT16>();
	UINT64 created = font_file.read_fundamental<UINT64>();
	UINT64 modified = font_file.read_fundamental<UINT64>();
	INT16 xMin = font_file.read_fundamental<INT16>();
	INT16 yMin = font_file.read_fundamental<INT16>();
	INT16 xMax = font_file.read_fundamental<INT16>();
	INT16 yMax = font_file.read_fundamental<INT16>();
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
	UINT16 macStyle = font_file.read_fundamental<UINT16>();

	UINT16 lowestRecPPEM = font_file.read_fundamental<UINT16>();

	/*
	0 : Fully mixed directional glyphs;
	1: Only strongly left to right;
	2: Like 1 but also contains neutrals;
	-1: Only strongly right to left;
	-2: Like - 1 but also contains neutrals.
	*/
	INT16 fontDirectionHint = font_file.read_fundamental<INT16>();
	INT16 indexToLocFormat = font_file.read_fundamental<INT16>();
	INT16 glyphDataFormat = font_file.read_fundamental<INT16>();
	return indexToLocFormat;
}

void noxcain::FontEngine::createHorizontalMetrics( ResourceFile& font_file, UINT32 hheaOffset, UINT32 hmtxOffset )
{
	font_file.set_position( hheaOffset );

	UINT16 	majorVersion = font_file.read_fundamental<UINT16>();
	UINT16 	minorVersion = font_file.read_fundamental<UINT16>();
	ascender = FLOAT32( font_file.read_fundamental<INT16>() ) / units_per_em;
	descender = FLOAT32( font_file.read_fundamental<INT16>() ) / units_per_em;
	line_gap = FLOAT32( font_file.read_fundamental<INT16>() ) / units_per_em;
	UINT16 	advanceWidthMax = font_file.read_fundamental<UINT16>();
	INT16 	minLeftSideBearing = font_file.read_fundamental<INT16>();
	INT16 	minRightSideBearing = font_file.read_fundamental<INT16>();
	INT16 	xMaxExtent = font_file.read_fundamental<INT16>() / units_per_em;
	INT16 	caretSlopeRise = font_file.read_fundamental<INT16>();
	INT16 	caretSlopeRun = font_file.read_fundamental<INT16>();
	INT16 	caretOffset = font_file.read_fundamental<INT16>();
	INT16 reserved;
	reserved = font_file.read_fundamental<INT16>();
	reserved = font_file.read_fundamental<INT16>();
	reserved = font_file.read_fundamental<INT16>();
	reserved = font_file.read_fundamental<INT16>();
	INT16 	metricDataFormat = font_file.read_fundamental<INT16>();
	UINT16 	numberOfHMetrics = font_file.read_fundamental<UINT16>();

	font_file.set_position( hmtxOffset );
	for( UINT32 index = 0; index < numberOfHMetrics; ++index )
	{
		advance_widths[index] = FLOAT32( font_file.read_fundamental<INT16>() ) / units_per_em;
		left_bearings[index] = FLOAT32( font_file.read_fundamental<INT16>() ) / units_per_em;
	}

	for( UINT32 index = numberOfHMetrics; index < advance_widths.size(); ++index )
	{
		advance_widths[index] = advance_widths[numberOfHMetrics - 1];
		left_bearings[index] = FLOAT32( font_file.read_fundamental<INT16>() ) / units_per_em;
	}
}

void noxcain::FontEngine::createVerticalMetrics( ResourceFile& font_file, UINT32 vheaOffset, UINT32 vmtxOffset )
{
}

void noxcain::FontEngine::computeGlyph( ResourceFile& font_file, UINT32 tableOffset, const std::vector<UINT32>& glyphOffsets )
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
		UINT32 maxFloate32;

		// the end of the last band technical doesent matter
		// when a curtve doesent fit in another band it ends in the last automatically
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
					std::max<FLOAT32>( curvePoints[off1], std::max<FLOAT32>( curvePoints[std::size_t( off1 ) + 2], curvePoints[std::size_t( off1 ) + 4] ) ) >
					std::max<FLOAT32>( curvePoints[off2], std::max<FLOAT32>( curvePoints[std::size_t( off2 )+ 2], curvePoints[std::size_t( off2 ) + 4] ) );
			} );
		}
		
		return std::array<UINT32, 2>( { nBands, nCurves } );
	};


	std::vector<RawSimpleGlyph> rawGlyphs;
	rawGlyphs.reserve( glyphOffsets.size() - 1 );
	std::vector<CompositeGlyph> compositeGlyphs;

	for( UINT32 glyph_index = 0; glyph_index < glyphOffsets.size() - 1; ++glyph_index )
	{
		if( glyphOffsets[glyph_index] != glyphOffsets[std::size_t( glyph_index ) + 1] )
		{
			glyph_indices[glyph_index] = rawGlyphs.size();
			
			font_file.set_position( UINT64( tableOffset ) + glyphOffsets[glyph_index] );

			const INT16 numberOfContours = font_file.read_fundamental<INT16>();

			glyph_corners.push_back( FLOAT32( font_file.read_fundamental<INT16>() ) / units_per_em ); // minX
			glyph_corners.push_back( FLOAT32( font_file.read_fundamental<INT16>() ) / units_per_em ); // minY
			glyph_corners.push_back( FLOAT32( font_file.read_fundamental<INT16>() ) / units_per_em ); // maxX
			glyph_corners.push_back( FLOAT32( font_file.read_fundamental<INT16>() ) / units_per_em ); // maxY

			rawGlyphs.push_back( RawSimpleGlyph() );

			//read as simple Glyph and unpack
			if( numberOfContours >= 0 )
			{
				rawGlyphs.back().endPointIndices.resize( numberOfContours );
				readSimpleGlyph( font_file, rawGlyphs.back().endPointIndices, rawGlyphs.back().flags, rawGlyphs.back().xCoords, rawGlyphs.back().yCoords );
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

					newComponent.flags = font_file.read_fundamental<UINT16>();
					newComponent.index = font_file.read_fundamental<UINT16>();

					switch( newComponent.flags & 0x3 )
					{
						case 0x1:
						{
							newComponent.argument1 = font_file.read_fundamental<UINT16>();
							newComponent.argument2 = font_file.read_fundamental<UINT16>();
							break;
						}
						case 0x2:
						{
							newComponent.argument1 = font_file.read_fundamental<INT8>();
							newComponent.argument2 = font_file.read_fundamental<INT8>();
							break;
						}
						case 0x3:
						{
							newComponent.argument1 = font_file.read_fundamental<INT16>();
							newComponent.argument2 = font_file.read_fundamental<INT16>();
							break;
						}
						default:
						{
							newComponent.argument1 = font_file.read_fundamental<UINT8>();
							newComponent.argument2 = font_file.read_fundamental<UINT8>();
							break;
						}
					}

					if( newComponent.flags & 0x0008 )
					{
						newComponent.transformation1 = newComponent.transformation4 = font_file.read_f2dot14();
					}
					else if( newComponent.flags & 0x0040 )
					{
						newComponent.transformation1 = font_file.read_f2dot14();
						newComponent.transformation4 = font_file.read_f2dot14();
					}
					else if( newComponent.flags & 0x0080 )
					{
						newComponent.transformation1 = font_file.read_f2dot14();
						newComponent.transformation2 = font_file.read_f2dot14();
						newComponent.transformation3 = font_file.read_f2dot14();
						newComponent.transformation4 = font_file.read_f2dot14();
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

			std::size_t corner_index = 4*(std::size_t)compositeGlyph.glyph_index;

			for( UINT32 pointIndex = 0; pointIndex < componentGlyph.flags.size(); ++pointIndex )
			{
				DOUBLE value_x = component.transformation1*componentGlyph.xCoords[pointIndex] + component.transformation2*componentGlyph.yCoords[pointIndex] + offsetX;
				DOUBLE value_y = component.transformation3*componentGlyph.xCoords[pointIndex] + component.transformation4*componentGlyph.yCoords[pointIndex] + offsetY;
				if( component.flags & 0x0004 )
				{
					value_x = INT32( value_x + 0.5 );
					value_y = INT32( value_y + 0.5 );
				}

				newGlyph.xCoords.push_back( value_x );
				newGlyph.yCoords.push_back( value_y );
			}
		}
	}

	point_maps.resize( glyph_count );
	offset_maps.resize( glyph_count );

	auto check_corner_points = [this]( UINT32 glyph_index, const std::vector<FLOAT32>& curve_points )
	{
		std::size_t corner_index = 4*( std::size_t )glyph_index;
		const FLOAT32& x_value = curve_points.back();
		const FLOAT32& y_value = curve_points[curve_points.size() - 2];
		if( x_value < glyph_corners[corner_index] ) glyph_corners[corner_index] = x_value;
		else if( x_value > glyph_corners[corner_index+2] ) glyph_corners[corner_index+2] = x_value;

		if( y_value < glyph_corners[corner_index+1] ) glyph_corners[corner_index+1] = y_value;
		else if( y_value > glyph_corners[corner_index+3] ) glyph_corners[corner_index+3] = y_value;
	};

	//rebuild glyphs 2 map
	for( UINT32 rawGlyphIndex = 0; rawGlyphIndex < rawGlyphs.size(); ++rawGlyphIndex )
	{
		std::vector<UINT32> curveStartOffsets;
		std::vector<FLOAT32> curvePoints;
		
		const RawSimpleGlyph& glyph = rawGlyphs[rawGlyphIndex];
		UINT32 startIndex = 0;
		curvePoints.reserve( 4 * glyph.flags.size() );

		FLOAT32 min_x = glyph.xCoords[0]/units_per_em;
		FLOAT32 max_x = glyph.xCoords[0]/units_per_em;
		FLOAT32 min_y = glyph.yCoords[0]/units_per_em;
		FLOAT32 max_y = glyph.yCoords[0]/units_per_em;

		for( UINT16 endIndex : glyph.endPointIndices )
		{
			const UINT32 size = endIndex - startIndex + 1;
			UINT32 pointCount = 0;
			UINT32 startOffset = curvePoints.size();
			bool shouldOnCurve = true;
			for( UINT32 relIndex = 0; relIndex <= size; ++relIndex )
			{	
				UINT32 index = relIndex % size;
				
				FLOAT32 currPointX = glyph.xCoords[std::size_t( startIndex ) + index] / units_per_em;
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

					FLOAT32 middlePointX = 0.5*( prevPointX + currPointX );
					FLOAT32 middlePointY = 0.5*( prevPointY + currPointY );

					if( middlePointX < min_x ) min_x = middlePointX;
					else if( middlePointX > max_x ) max_x = middlePointX;
					curvePoints.push_back( middlePointX );

					if( middlePointY < min_y ) min_y = middlePointY;
					else if( middlePointY > max_y ) max_y = middlePointY;
					curvePoints.push_back( middlePointY );
					
					++pointCount;
				}
				
				if( relIndex < size || pointCount % 2 == 0 )
				{
					if( currPointX < min_x ) min_x = currPointX;
					else if( currPointX > max_x ) max_x = currPointX;
					curvePoints.push_back( currPointX );

					if( currPointY < min_y ) min_y = currPointY;
					else if( currPointY > max_y ) max_y = currPointY;
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

		std::size_t corner_index = 4*( std::size_t )rawGlyphIndex;
		glyph_corners[corner_index] = min_x;
		glyph_corners[corner_index+1] = min_y;
		glyph_corners[corner_index+2] = max_x;
		glyph_corners[corner_index+3] = max_y;

		std::vector<Band> xBands( 32 );
		std::array<UINT32, 2> xBandCounts = fillBand( curvePoints, curveStartOffsets, xBands, 0, &glyph_corners[std::size_t( 4 ) * rawGlyphIndex] );

		std::vector<Band> yBands( 32 );
		std::array<UINT32, 2> yBandCounts = fillBand( curvePoints, curveStartOffsets, yBands, 1, &glyph_corners[std::size_t( 4 ) * rawGlyphIndex] );

		const UINT32 bandCount = std::max<UINT32>( xBandCounts[0], yBandCounts[0] );
		const UINT32 curveCount = xBandCounts[1] + yBandCounts[1];

		std::vector<UINT32>& currOffsetMap = offset_maps[rawGlyphIndex];
		std::vector<FLOAT32>& currPointMap = point_maps[rawGlyphIndex];

		currOffsetMap.resize( std::size_t( bandCount )*4 + curveCount );

		currPointMap.reserve( std::size_t( bandCount ) * 2 + curvePoints.size() );
		currPointMap.resize( std::size_t( bandCount ) * 2 );
		currPointMap.insert( currPointMap.end(), curvePoints.begin(), curvePoints.end() );

		UINT32 currentBandOffset = 4 * bandCount;
		UINT32 currentBandIndex = 0;
		for( UINT32 bandIndex = 0; bandIndex < xBands.size(); ++bandIndex )
		{
			if( !xBands[bandIndex].curveOffsets.empty() )
			{
				//at the start of the point maps are the band ends 
				//frist x then y 
				currPointMap[2 * std::size_t( currentBandIndex )] = xBands[bandIndex].end;
				//the offset map begins with the curve count in the band and the offset to the curve offsets
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

void noxcain::FontEngine::readSimpleGlyph( ResourceFile& font_file, std::vector<UINT16>& endPointIndices, std::vector<BYTE>& flags, std::vector<DOUBLE>& xCoords, std::vector<DOUBLE>& yCoords )
{
	for( UINT32 index = 0; index < endPointIndices.size(); ++index )
	{
		endPointIndices[index] = font_file.read_fundamental<UINT16>();
	}

	UINT16 instructionLength = font_file.read_fundamental<UINT16>();
	std::vector<BYTE> instructions( instructionLength );

	for( UINT32 index = 0; index < instructions.size(); ++index )
	{
		instructions[index] = font_file.read_fundamental<BYTE>();
	}

	while( flags.size() < std::size_t( endPointIndices.back() ) + 1 )
	{
		BYTE flag = font_file.read_fundamental<BYTE>();
		flags.push_back( flag );
		if( flag & 0x08 )
		{
			BYTE count = font_file.read_fundamental<BYTE>();
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
				xCoords.push_back( lastValue - font_file.read_fundamental<BYTE>() );
				break;
			}
			case 0x10:
			{
				xCoords.push_back( lastValue );
				break;
			}
			case 0x12:
			{
				xCoords.push_back( lastValue + font_file.read_fundamental<BYTE>() );
				break;
			}
			default:
			{
				xCoords.push_back( lastValue + font_file.read_fundamental<INT16>() );
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
				yCoords.push_back( lastValue - font_file.read_fundamental<BYTE>() );
				break;
			}
			case 0x20:
			{
				yCoords.push_back( lastValue );
				break;
			}
			case 0x24:
			{
				yCoords.push_back( lastValue + font_file.read_fundamental<BYTE>() );
				break;
			}
			default:
			{
				yCoords.push_back( lastValue + font_file.read_fundamental<INT16>() );
				break;
			}
		}
		lastValue = yCoords.back();
	}

	
	int stop = 0;
}

void noxcain::FontEngine::readCompositeGlyph( ResourceFile& font_file )
{
	UINT16 flags = 0;
	do
	{
		flags = font_file.read_fundamental<UINT16>();
		UINT16 glyph_index = font_file.read_fundamental<UINT16>();

		DOUBLE argument1;
		DOUBLE argument2;

		switch( flags & 0x3 )
		{
			case 0x1:
			{
				argument1 = font_file.read_fundamental<UINT16>();
				argument2 = font_file.read_fundamental<UINT16>();
				break;
			}
			case 0x2:
			{
				argument1 = font_file.read_fundamental<INT8>();
				argument2 = font_file.read_fundamental<INT8>();
				break;
			}
			case 0x3:
			{
				argument1 = font_file.read_fundamental<INT16>();
				argument2 = font_file.read_fundamental<INT16>();
				break;
			}
			default:
			{
				argument1 = font_file.read_fundamental<UINT8>();
				argument2 = font_file.read_fundamental<UINT8>();
				break;
			}
		}

		if( flags & 0x0008 )
		{
			DOUBLE a1 = font_file.read_f2dot14();
		}
		else if( flags & 0x0040 )
		{
			DOUBLE a1 = font_file.read_f2dot14();
			DOUBLE a2 = font_file.read_f2dot14();
		}
		else if( flags & 0x0080 )
		{
			DOUBLE a1 = font_file.read_f2dot14();
			DOUBLE a2 = font_file.read_f2dot14();
			DOUBLE a3 = font_file.read_f2dot14();
			DOUBLE a4 = font_file.read_f2dot14();
		}
	} while( flags & 0x0020 );
}

bool noxcain::FontEngine::readFont( const std::string& fontPath )
{
	std::ifstream file( fontPath );
	ResourceFile font_file;
	if( font_file.open( fontPath, false ) )
	{
		const UINT32 version = font_file.read_fundamental<UINT32>();
		const UINT16 nTables = font_file.read_fundamental<UINT16>();
		const UINT16 searchRange = font_file.read_fundamental<UINT16>();
		const UINT16 entrySelector = font_file.read_fundamental<UINT16>();
		const UINT16 rangeShift = font_file.read_fundamental<UINT16>();

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
			record.tag = font_file.read_fundamental<UINT32>();
			record.checkSum = font_file.read_fundamental<UINT32>();
			record.offset = font_file.read_fundamental<UINT32>();
			record.length = font_file.read_fundamental<UINT32>();

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

		const UINT32 nGlyphs = getNumGlyphs( font_file, maxp.offset );
		const bool longOffset = isLongOffset( font_file, head.offset );
		
		//advanceHeights.resize( nGlyphs );
		advance_widths.resize( nGlyphs );

		//topBearings.resize( nGlyphs );
		left_bearings.resize( nGlyphs );

		glyph_indices.resize( nGlyphs, INVALID_UNICODE );

		createHorizontalMetrics( font_file, hhea.offset, hmtx.offset );
		if( vhea.length > 0 )
		{
			createVerticalMetrics( font_file, vhea.offset, vmtx.offset );
		}
		

		font_file.set_position( loca.offset );
		std::vector<UINT32> glyphTableOffsets( std::size_t( nGlyphs ) + 1 );
		for( UINT32 glyph_index = 0; glyph_index < nGlyphs + 1; ++glyph_index )
		{
			glyphTableOffsets[glyph_index] = longOffset ? font_file.read_fundamental<UINT32>() : UINT32( 2 )*font_file.read_fundamental<UINT16>();
		}

		computeGlyph( font_file, glyf.offset, glyphTableOffsets );

		createUnicodeMap( font_file, cmap.offset );
		
		if( get_glyph_count() )
		{
			return true;
		}
		font_file.close();
	}
	return false;
}
