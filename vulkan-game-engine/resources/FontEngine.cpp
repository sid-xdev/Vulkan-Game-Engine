#include "FontEngine.hpp"

#include <fstream>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>

void noxcain::FontEngine::createUnicodeMap( UINT32 offset )
{
	font.seekg( offset );

	UINT16 cmapVersion = read<UINT16>();
	UINT16 nCmapTables = read<UINT16>();

	struct EncodingRecord
	{
		UINT16 platformId;
		UINT16 encodingId;
		UINT32 offset;
	};
	std::vector<EncodingRecord> encodingRecords( nCmapTables );

	for( UINT32 encodingIndex = 0; encodingIndex < nCmapTables; ++encodingIndex )
	{
		encodingRecords[encodingIndex].platformId = read<UINT16>();
		encodingRecords[encodingIndex].encodingId = read<UINT16>();
		encodingRecords[encodingIndex].offset = read<UINT32>();
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
	for( const EncodingRecord& record : encodingRecords )
	{
		if( record.platformId == 3 && record.encodingId == 1 )
		{
			font.seekg( std::streamoff( offset ) + record.offset );
			format4.format = read<UINT16>();
			format4.length = read<UINT16>();
			format4.language = read<UINT16>();
			format4.segCountX2 = read<UINT16>();
			format4.searchRange = read<UINT16>();
			format4.entrySelector = read<UINT16>();
			format4.rangeShift = read<UINT16>();

			format4.endCode.resize( format4.segCountX2 / 2 );
			for( UINT32 index = 0; index < format4.endCode.size(); ++index )
			{
				format4.endCode[index] = read<UINT16>();
			}
			format4.reservedPad = read<UINT16>();

			format4.startCode.resize( format4.segCountX2 / 2 );
			for( UINT32 index = 0; index < format4.startCode.size(); ++index )
			{
				format4.startCode[index] = read<UINT16>();
			}

			format4.idDelta.resize( format4.segCountX2 / 2 );
			for( UINT32 index = 0; index < format4.idDelta.size(); ++index )
			{
				format4.idDelta[index] = read<INT16>();
			}

			format4.idRangeOffset.resize( format4.segCountX2 / 2 );
			for( UINT32 index = 0; index < format4.idRangeOffset.size(); ++index )
			{
				format4.idRangeOffset[index] = read<UINT16>();
			}

			format4.glyphIdArray = font.tellg();
			has_record = true;
			break;
		}
	}

	if( has_record )
	{
		UINT32 count = 0;
		UINT32 lastSegment = 0;
		unicode_map.resize( INVALID_UNICODE, 0 );
		for( UINT32 unicode = 0; unicode < unicode_map.size(); ++unicode )
		{
			for( UINT32 segIndex = lastSegment; segIndex < format4.endCode.size(); ++segIndex )
			{
				if( format4.startCode[segIndex] <= unicode && unicode <= format4.endCode[segIndex] )
				{
					if( format4.idRangeOffset[segIndex] == 0 )
					{
						unicode_map[unicode] = ( format4.idDelta[segIndex] + unicode ) % INVALID_UNICODE;
					}
					else
					{
						UINT16 glyphArrayIndex = format4.glyphIdArray + sizeof( UINT16 ) * segIndex - format4.segCountX2 + sizeof( UINT16 ) * ( format4.startCode[segIndex] - unicode ) + format4.idRangeOffset[segIndex];
						font.seekg( glyphArrayIndex );
						unicode_map[unicode] = read<UINT16>();
					}
					lastSegment = segIndex;
					++count;
					break;
				}
			}
		}
	}
}

noxcain::UINT16 noxcain::FontEngine::getNumGlyphs( UINT32 offset )
{
	font.seekg( offset );
	UINT16 major = read<UINT16>();
	UINT16 minor = read<UINT16>();
	return read<UINT16>();
}

bool noxcain::FontEngine::isLongOffset( UINT32 offset )
{
	font.seekg( offset );

	UINT16 majorVersion = read<UINT16>();
	UINT16 minorVersion = read<UINT16>();

	UINT16 majorFontVersion = read<UINT16>();
	UINT16 minorFontVersion = read<UINT16>();

	UINT32 checkSumAdjustment = read<UINT32>();
	UINT32 magicNumber = read<UINT32>();

	/*
	Bit 0: Baseline for font at y = 0;
	Bit 1: Left sidebearing point at x = 0 ( relevant only for TrueType rasterizers ) — see the note below regarding variable fonts;
	Bit 2: Instructions may depend on point size;
	Bit 3: Force ppem to integer values for all internal scaler math; may use fractional ppem sizes if this bit is clear;
	Bit 4: Instructions may alter advance width( the advance widths might not scale linearly );
	Bit 5: This bit is not used in OpenType, and should not be set in order to ensure compatible behavior on all platforms.If set, it may result in different behavior for vertical layout in some platforms. ( See Apple’s specification for details regarding behavior in Apple platforms. )
	Bits 6–10: These bits are not used in Opentype and should always be cleared. ( See Apple’s specification for details regarding legacy used in Apple platforms. )
	Bit 11: Font data is “lossless” as a result of having been subjected to optimizing transformation and/or compression( such as e.g.compression mechanisms defined by ISO / IEC 14496 - 18, MicroType Express, WOFF 2.0 or similar ) where the original font functionality and features are retained but the binary compatibility between input and output font files is not guaranteed.As a result of the applied transform, the DSIG table may also be invalidated.
	Bit 12: Font converted( produce compatible metrics )
	Bit 13: Font optimized for ClearType™.Note, fonts that rely on embedded bitmaps( EBDT ) for rendering should not be considered optimized for ClearType, and therefore should keep this bit cleared.
	Bit 14: Last Resort font.If set, indicates that the glyphs encoded in the 'cmap' subtables are simply generic symbolic representations of code point ranges and don’t truly represent support for those code points.If unset, indicates that the glyphs encoded in the 'cmap' subtables represent proper support for those code points.
	Bit 15: Reserved, set to 0
	*/
	UINT16 flags = read<UINT16>();
	if( ( flags & 0x3 ) != 0x3 )
	{
		int stop = 1;
	}
	unitsPerEm = read<UINT16>();
	UINT64 created = read<UINT64>();
	UINT64 modified = read<UINT64>();
	INT16 xMin = read<INT16>();
	INT16 yMin = read<INT16>();
	INT16 xMax = read<INT16>();
	INT16 yMax = read<INT16>();
	/*
	Bit 0 : Bold( if set to 1 );
	Bit 1: Italic( if set to 1 )
	Bit 2 : Underline( if set to 1 )
	Bit 3 : Outline( if set to 1 )
	Bit 4 : Shadow( if set to 1 )
	Bit 5 : Condensed( if set to 1 )
	Bit 6 : Extended( if set to 1 )
	Bits 7–15 : Reserved( set to 0 )
	*/
	UINT16 macStyle = read<UINT16>();

	UINT16 lowestRecPPEM = read<UINT16>();

	/*
	0 : Fully mixed directional glyphs;
	1: Only strongly left to right;
	2: Like 1 but also contains neutrals;
	-1: Only strongly right to left;
	-2: Like - 1 but also contains neutrals.
	*/
	INT16 fontDirectionHint = read<INT16>();
	INT16 indexToLocFormat = read<INT16>();
	INT16 glyphDataFormat = read<INT16>();
	return indexToLocFormat;
}

void noxcain::FontEngine::createHorizontalMetrics( UINT32 hheaOffset, UINT32 hmtxOffset )
{
	font.seekg( hheaOffset );

	UINT16 	majorVersion = read<UINT16>();
	UINT16 	minorVersion = read<UINT16>();
	ascender = FLOAT32( read<INT16>() ) / unitsPerEm;
	descender = FLOAT32( read<INT16>() ) / unitsPerEm;
	lineGap = FLOAT32( read<INT16>() ) / unitsPerEm;
	UINT16 	advanceWidthMax = read<UINT16>();
	INT16 	minLeftSideBearing = read<INT16>();
	INT16 	minRightSideBearing = read<INT16>();
	INT16 	xMaxExtent = read<INT16>() / unitsPerEm;
	INT16 	caretSlopeRise = read<INT16>();
	INT16 	caretSlopeRun = read<INT16>();
	INT16 	caretOffset = read<INT16>();
	INT16 reserved;
	reserved = read<INT16>();
	reserved = read<INT16>();
	reserved = read<INT16>();
	reserved = read<INT16>();
	INT16 	metricDataFormat = read<INT16>();
	UINT16 	numberOfHMetrics = read<UINT16>();

	font.seekg( hmtxOffset );
	for( UINT32 index = 0; index < numberOfHMetrics; ++index )
	{
		advanceWidths[index] = FLOAT32( read<INT16>() ) / unitsPerEm;
		leftBearings[index] = FLOAT32( read<INT16>() ) / unitsPerEm;
	}

	for( UINT32 index = numberOfHMetrics; index < advanceWidths.size(); ++index )
	{
		advanceWidths[index] = advanceWidths[numberOfHMetrics - 1];
		leftBearings[index] = FLOAT32( read<INT16>() ) / unitsPerEm;
	}
}

void noxcain::FontEngine::createVerticalMetrics( UINT32 vheaOffset, UINT32 vmtxOffset )
{
}

void noxcain::FontEngine::computeGlyph( UINT32 tableOffset, const std::vector<UINT32>& glyphOffsets )
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
	rawGlyphs.reserve( glyphOffsets.size() - 1 );
	std::vector<CompositeGlyph> compositeGlyphs;

	for( UINT32 glyph_index = 0; glyph_index < glyphOffsets.size() - 1; ++glyph_index )
	{
		if( glyphOffsets[glyph_index] != glyphOffsets[std::size_t( glyph_index ) + 1] )
		{
			glyphIndices[glyph_index] = rawGlyphs.size();
			
			font.seekg( std::streamoff( tableOffset ) + glyphOffsets[glyph_index] );

			const INT16 numberOfContours = read<INT16>();

			glyphCorners.push_back( FLOAT32( read<INT16>() ) / unitsPerEm ); // minX
			glyphCorners.push_back( FLOAT32( read<INT16>() ) / unitsPerEm ); // minY
			glyphCorners.push_back( FLOAT32( read<INT16>() ) / unitsPerEm ); // maxX
			glyphCorners.push_back( FLOAT32( read<INT16>() ) / unitsPerEm ); // maxY

			rawGlyphs.push_back( RawSimpleGlyph() );

			//read as simple Glyph and unpack
			if( numberOfContours >= 0 )
			{
				rawGlyphs.back().endPointIndices.resize( numberOfContours );
				readSimpleGlyph( rawGlyphs.back().endPointIndices, rawGlyphs.back().flags, rawGlyphs.back().xCoords, rawGlyphs.back().yCoords );
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

					newComponent.flags = read<UINT16>();
					newComponent.index = read<UINT16>();

					switch( newComponent.flags & 0x3 )
					{
						case 0x1:
						{
							newComponent.argument1 = read<UINT16>();
							newComponent.argument2 = read<UINT16>();
							break;
						}
						case 0x2:
						{
							newComponent.argument1 = read<INT8>();
							newComponent.argument2 = read<INT8>();
							break;
						}
						case 0x3:
						{
							newComponent.argument1 = read<INT16>();
							newComponent.argument2 = read<INT16>();
							break;
						}
						default:
						{
							newComponent.argument1 = read<UINT8>();
							newComponent.argument2 = read<UINT8>();
							break;
						}
					}

					if( newComponent.flags & 0x0008 )
					{
						newComponent.transformation1 = newComponent.transformation4 = readF2Dot14();
					}
					else if( newComponent.flags & 0x0040 )
					{
						newComponent.transformation1 = readF2Dot14();
						newComponent.transformation4 = readF2Dot14();
					}
					else if( newComponent.flags & 0x0080 )
					{
						newComponent.transformation1 = readF2Dot14();
						newComponent.transformation2 = readF2Dot14();
						newComponent.transformation3 = readF2Dot14();
						newComponent.transformation4 = readF2Dot14();
					}

					if( newComponent.flags & 0x0200 )
					{
						advanceWidths[newCompositeGlyph.glyph_index] = advanceWidths[newComponent.index];
						leftBearings[newCompositeGlyph.glyph_index] = leftBearings[newComponent.index];
						//advanceHeights[newCompositeGlyph.glyph_index] = advanceHeights[newComponent.index];
						//topBearings[newCompositeGlyph.glyph_index] = topBearings[newComponent.index];
					}
				} while( newCompositeGlyph.components.back().flags & 0x0020 );
			}
		}
	}

	glyphCount = rawGlyphs.size();

	//combine glyph components;
	for( const CompositeGlyph& compositeGlyph : compositeGlyphs )
	{
		RawSimpleGlyph& newGlyph = rawGlyphs[glyphIndices[compositeGlyph.glyph_index]];
		for( const Component& component : compositeGlyph.components )
		{
			const RawSimpleGlyph& componentGlyph = rawGlyphs[glyphIndices[component.index]];

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

	pointMaps.resize( glyphCount );
	offsetMaps.resize( glyphCount );

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
				
				FLOAT32 currPointX = glyph.xCoords[std::size_t(startIndex) + index] / unitsPerEm;
				FLOAT32 currPointY = glyph.yCoords[std::size_t( startIndex ) + index] / unitsPerEm;

				if( shouldOnCurve == bool( glyph.flags[std::size_t( startIndex ) + index] & 0x1 ) )
				{
					shouldOnCurve = !shouldOnCurve;
				}
				else
				{
					UINT32 prevIndex = ( index + size - 1 ) % size;
					DOUBLE prevPointX = glyph.xCoords[std::size_t( startIndex ) + prevIndex] / unitsPerEm;
					DOUBLE prevPointY = glyph.yCoords[std::size_t( startIndex ) + prevIndex] / unitsPerEm;

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
		std::array<UINT32, 2> xBandCounts = fillBand( curvePoints, curveStartOffsets, xBands, 0, &glyphCorners[std::size_t( 4 ) * rawGlyphIndex] );

		std::vector<Band> yBands( 32 );
		std::array<UINT32, 2> yBandCounts = fillBand( curvePoints, curveStartOffsets, yBands, 1, &glyphCorners[std::size_t( 4 ) * rawGlyphIndex] );

		const UINT32 bandCount = std::max( xBandCounts[0], yBandCounts[0] );
		const UINT32 curveCount = xBandCounts[1] + yBandCounts[1];

		std::vector<UINT32>& currOffsetMap = offsetMaps[rawGlyphIndex];
		std::vector<FLOAT32>& currPointMap = pointMaps[rawGlyphIndex];

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

void noxcain::FontEngine::readSimpleGlyph( std::vector<UINT16>& endPointIndices, std::vector<BYTE>& flags, std::vector<DOUBLE>& xCoords, std::vector<DOUBLE>& yCoords )
{
	for( UINT32 index = 0; index < endPointIndices.size(); ++index )
	{
		endPointIndices[index] = read<UINT16>();
	}

	UINT16 instructionLength = read<UINT16>();
	std::vector<BYTE> instructions( instructionLength );

	for( UINT32 index = 0; index < instructions.size(); ++index )
	{
		instructions[index] = read<BYTE>();
	}

	while( flags.size() < std::size_t( endPointIndices.back() ) + 1 )
	{
		BYTE flag = read<BYTE>();
		flags.push_back( flag );
		if( flag & 0x08 )
		{
			BYTE count = read<BYTE>();
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
				xCoords.push_back( lastValue - read<BYTE>() );
				break;
			}
			case 0x10:
			{
				xCoords.push_back( lastValue );
				break;
			}
			case 0x12:
			{
				xCoords.push_back( lastValue + read<BYTE>() );
				break;
			}
			default:
			{
				xCoords.push_back( lastValue + read<INT16>() );
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
				yCoords.push_back( lastValue - read<BYTE>() );
				break;
			}
			case 0x20:
			{
				yCoords.push_back( lastValue );
				break;
			}
			case 0x24:
			{
				yCoords.push_back( lastValue + read<BYTE>() );
				break;
			}
			default:
			{
				yCoords.push_back( lastValue + read<INT16>() );
				break;
			}
		}
		lastValue = yCoords.back();
	}

	
	int stop = 0;
}

void noxcain::FontEngine::readCompositeGlyph()
{
	UINT16 flags = 0;
	do
	{
		flags = read<UINT16>();
		UINT16 glyph_index = read<UINT16>();

		DOUBLE argument1;
		DOUBLE argument2;

		switch( flags & 0x3 )
		{
			case 0x1:
			{
				argument1 = read<UINT16>();
				argument2 = read<UINT16>();
				break;
			}
			case 0x2:
			{
				argument1 = read<INT8>();
				argument2 = read<INT8>();
				break;
			}
			case 0x3:
			{
				argument1 = read<INT16>();
				argument2 = read<INT16>();
				break;
			}
			default:
			{
				argument1 = read<UINT8>();
				argument2 = read<UINT8>();
				break;
			}
		}

		if( flags & 0x0008 )
		{
			DOUBLE a1 = readF2Dot14();
		}
		else if( flags & 0x0040 )
		{
			DOUBLE a1 = readF2Dot14();
			DOUBLE a2 = readF2Dot14();
		}
		else if( flags & 0x0080 )
		{
			DOUBLE a1 = readF2Dot14();
			DOUBLE a2 = readF2Dot14();
			DOUBLE a3 = readF2Dot14();
			DOUBLE a4 = readF2Dot14();
		}
	} while( flags & 0x0020 );
}

noxcain::DOUBLE noxcain::FontEngine::readF2Dot14()
{
	char word[2];
	font.read( word, 2 );
	signed char front = 0;
	front = front | ( word[0] & 0x80 ) | ( ( word[0] & 0x40 ) >> 6 );
	word[0] &= 0x3F;
	if( isLittleEndian )
	{
		char temp = word[1];
		word[1] = word[0];
		word[0] = temp;
	}

	UINT16 frac;
	memcpy( &frac, word, 2 );

	return DOUBLE( front ) + ( frac / 16384 );
}

bool noxcain::FontEngine::readFont( const std::string& fontPath )
{
	font.open( fontPath, std::fstream::binary | std::fstream::in );
	if( font.is_open() )
	{
		UINT32 endianCheck = 1;
		isLittleEndian = reinterpret_cast<unsigned char*>( &endianCheck )[0] > 0;

		const UINT32 version = read<UINT32>();
		const UINT16 nTables = read<UINT16>();
		const UINT16 searchRange = read<UINT16>();
		const UINT16 entrySelector = read<UINT16>();
		const UINT16 rangeShift = read<UINT16>();

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
			record.tag = read<UINT32>();
			record.checkSum = read<UINT32>();
			record.offset = read<UINT32>();
			record.length = read<UINT32>();

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

		const UINT16 nGlyphs = getNumGlyphs( maxp.offset );
		const bool longOffset = isLongOffset( head.offset );
		
		//advanceHeights.resize( nGlyphs );
		advanceWidths.resize( nGlyphs );

		//topBearings.resize( nGlyphs );
		leftBearings.resize( nGlyphs );

		glyphIndices.resize( nGlyphs, 0xFFFF );

		createHorizontalMetrics( hhea.offset, hmtx.offset );
		if( vhea.length > 0 )
		{
			createVerticalMetrics( vhea.offset, vmtx.offset );
		}
		

		font.seekg( loca.offset );
		std::vector<UINT32> glyphTableOffsets( std::size_t( nGlyphs ) + 1 );
		for( UINT32 glyph_index = 0; glyph_index < nGlyphs + 1; ++glyph_index )
		{
			glyphTableOffsets[glyph_index] = longOffset ? read<UINT32>() : UINT32( 2 )*read<UINT16>();
		}

		computeGlyph( glyf.offset, glyphTableOffsets );

		createUnicodeMap( cmap.offset );
		
		if( getGlyphCount() )
		{
			return true;
		}
	}
	return false;
}


