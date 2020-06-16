#include "GameResourceEngine.hpp"

#include <resources/FontEngine.hpp>
#include <resources/FontResource.hpp>
#include <resources/GeometryResource.hpp>

#include <math/Vector.hpp>

#include <cmath>

std::unique_ptr<noxcain::ResourceEngine> noxcain::ResourceEngine::resources;

std::mutex noxcain::ResourceEngine::resource_mutex;

void noxcain::ResourceEngine::read_font( const std::vector<std::string>& font_paths )
{
	std::vector<FontEngine> fonts( font_paths.size() );
	UINT32 total_glyph_count = 0;
	for( std::size_t font_index = 0; font_index < fonts.size(); ++font_index )
	{
		fonts[font_index].readFont( font_paths[font_index] );
		total_glyph_count += fonts[font_index].getGlyphCount();
	}

	const std::size_t vertex_buffer_size = sizeof( FLOAT32 ) * 8 * total_glyph_count;
	const std::size_t vertex_buffer_resource_id = addSubResource( vertex_buffer_size, GameSubResource::SubResourceType::eVertexBuffer );
	std::vector<BYTE> vertex_buffer( vertex_buffer_size );

	std::size_t font_offset = 0;

	for( const FontEngine& fe : fonts )
	{
		std::size_t current_glyph_count = fe.getGlyphCount();
		if( current_glyph_count )
		{
			std::vector<UINT32> unicode;
			unicode.reserve( fe.unicode_map.size() );
			for( const auto& uni : fe.unicode_map )
			{
				unicode.emplace_back( uni );
			}

			std::vector<FontResource::CharacterInfo> chars;
			chars.reserve( fe.glyphIndices.size() );
			for( std::size_t index = 0; index < fe.glyphIndices.size(); ++index )
			{
				chars.push_back( FontResource::CharacterInfo( { fe.glyphIndices[index], DOUBLE( fe.advanceWidths[index] ) } ) );
			}

			FLOAT32* memory = reinterpret_cast<FLOAT32*>( vertex_buffer.data() + sizeof( FLOAT32 ) * 8 * font_offset );
			for( std::size_t glyph_index = 0; glyph_index < current_glyph_count; ++glyph_index )
			{
				memory[8 * glyph_index] = fe.glyphCorners[4 * glyph_index];
				memory[8 * glyph_index + 1] = fe.glyphCorners[4 * glyph_index + 3];

				memory[8 * glyph_index + 2] = fe.glyphCorners[4 * glyph_index + 2];
				memory[8 * glyph_index + 3] = fe.glyphCorners[4 * glyph_index + 3];

				memory[8 * glyph_index + 4] = fe.glyphCorners[4 * glyph_index];
				memory[8 * glyph_index + 5] = fe.glyphCorners[4 *glyph_index + 1];

				memory[8 * glyph_index + 6] = fe.glyphCorners[4 * glyph_index + 2];
				memory[8 * glyph_index + 7] = fe.glyphCorners[4 * glyph_index + 1];
			}

			std::vector<std::size_t> offset_ids;
			offset_ids.reserve( current_glyph_count );
			for( const auto& map : fe.getGlyphOffsetMaps() )
			{
				std::vector<BYTE> mapBuffer( map.size() * sizeof( UINT32 ) );
				UINT32* mapMemory = reinterpret_cast<UINT32*>( mapBuffer.data() );
				for( std::size_t index = 0; index < map.size(); ++index )
				{
					mapMemory[index] = map[index];
				}

				const std::size_t id = addSubResource( mapBuffer.size(), GameSubResource::SubResourceType::eStorageBuffer );
				offset_ids.emplace_back( id );
				subresources[id].setData( std::move( mapBuffer ) );
			}

			std::vector<std::size_t> pointIds;
			pointIds.reserve( current_glyph_count );
			for( const auto& map : fe.getGlyphPointMaps() )
			{
				std::vector<BYTE> mapBuffer( map.size() * sizeof( FLOAT32 ) );
				FLOAT32* mapMemory = reinterpret_cast<FLOAT32*>( mapBuffer.data() );
				for( std::size_t index = 0; index < map.size(); ++index )
				{
					mapMemory[index] = map[index];
				}
				const std::size_t id = addSubResource( mapBuffer.size(), GameSubResource::SubResourceType::eStorageBuffer );
				pointIds.emplace_back( id );
				subresources[id].setData( std::move( mapBuffer ) );
			}

			font_resources.emplace_back( font_offset, std::move( unicode ), std::move( chars ), fe.ascender, fe.descender, fe.lineGap,
										 std::move( offset_ids ), std::move( pointIds ), vertex_buffer_resource_id );

			resource_limits.font_count++;

			font_offset += current_glyph_count;
		}
	}
	resource_limits.glyph_count = total_glyph_count;
	subresources[vertex_buffer_resource_id].setData( std::move( vertex_buffer ) );
}

void noxcain::ResourceEngine::read_hex_geometry()
{	
	constexpr std::size_t nCorners = 6;
	constexpr DOUBLE cornerRadius = 0.075;
	constexpr DOUBLE sideLength = 2.0;
	constexpr std::size_t nRings = 3;

	std::array<std::pair<DOUBLE, DOUBLE>, nCorners> cornerDirections;
	for( std::size_t index = 0; index < nCorners; ++index )
	{
		cornerDirections[index] = std::make_pair( std::cos( ( 2.0*PI / nCorners ) * index ), std::sin( ( 2.0*PI / nCorners ) * index ) );
	}
	const DOUBLE offset = cornerRadius / std::sqrt( 3.0 );

	DOUBLE maxX = 0, minX = 0, maxY = 0, minY = 0, maxZ = 0, minZ = 0;

	std::vector<BYTE> geometryVertices( 6 * sizeof( FLOAT32 ) * 2 * nCorners * nRings );
	FLOAT32* hexagonVertices = reinterpret_cast<FLOAT32*>( geometryVertices.data() );

	for( std::size_t index = 0; index < nCorners; ++index )
	{
		std::size_t valueIndex = index * 2 * nCorners;

		DOUBLE anchor1X = ( sideLength - cornerRadius ) * cornerDirections[index].first - offset * cornerDirections[( index + 1 ) % nCorners].first;
		DOUBLE anchor1Y = ( sideLength - cornerRadius ) * cornerDirections[index].second - offset * cornerDirections[( index + 1 ) % nCorners].second;

		DOUBLE anchor2X = ( sideLength - cornerRadius ) * cornerDirections[index].first + offset * cornerDirections[( index + 2 ) % nCorners].first;
		DOUBLE anchor2Y = ( sideLength - cornerRadius ) * cornerDirections[index].second + offset * cornerDirections[( index + 2 ) % nCorners].second;

		maxX = std::max( maxX, std::max( anchor1X, anchor2X ) );
		maxY = std::max( maxY, std::max( anchor1Y, anchor2Y ) );
		minX = std::min( minX, std::min( anchor1X, anchor2X ) );
		minY = std::min( minY, std::min( anchor1Y, anchor2Y ) );

		hexagonVertices[valueIndex++] = 0;
		hexagonVertices[valueIndex++] = 0;
		hexagonVertices[valueIndex++] = 1.0F;
		
		hexagonVertices[valueIndex++] = anchor1X;
		hexagonVertices[valueIndex++] = anchor1Y;
		hexagonVertices[valueIndex++] = 0;

		hexagonVertices[valueIndex++] = 0;
		hexagonVertices[valueIndex++] = 0;
		hexagonVertices[valueIndex++] = 1.0F;

		hexagonVertices[valueIndex++] = anchor2X;
		hexagonVertices[valueIndex++] = anchor2Y;
		hexagonVertices[valueIndex++] = 0;
		
		if( nRings > 1 )
		{
			valueIndex += 10 * nCorners;

			anchor1X += cornerRadius * cornerDirections[index].first;
			anchor1Y += cornerRadius * cornerDirections[index].second;
			anchor2X += cornerRadius * cornerDirections[index].first;
			anchor2Y += cornerRadius * cornerDirections[index].second;

			maxX = std::max( maxX, std::max( anchor1X, anchor2X ) );
			maxY = std::max( maxY, std::max( anchor1Y, anchor2Y ) );
			minX = std::min( minX, std::min( anchor1X, anchor2X ) );
			minY = std::min( minY, std::min( anchor1Y, anchor2Y ) );

			const auto& direction1 = cornerDirections[( index + nCorners - 2 ) % nCorners];
			const auto& direction2 = cornerDirections[( index + nCorners - 1 ) % nCorners];

			hexagonVertices[valueIndex++] = -direction1.second;
			hexagonVertices[valueIndex++] = direction1.first;
			hexagonVertices[valueIndex++] = 0.0F;

			hexagonVertices[valueIndex++] = anchor1X;
			hexagonVertices[valueIndex++] = anchor1Y;
			hexagonVertices[valueIndex++] = -cornerRadius;

			hexagonVertices[valueIndex++] = -direction2.second;
			hexagonVertices[valueIndex++] = direction2.first;
			hexagonVertices[valueIndex++] = 0.0F;

			hexagonVertices[valueIndex++] = anchor2X;
			hexagonVertices[valueIndex++] = anchor2Y;
			hexagonVertices[valueIndex++] = -cornerRadius;

			if( nRings > 2 )
			{
				valueIndex += 10 * nCorners;;

				hexagonVertices[valueIndex++] = -direction1.second;
				hexagonVertices[valueIndex++] = direction1.first;
				hexagonVertices[valueIndex++] = 0.0F;

				hexagonVertices[valueIndex++] = anchor1X;
				hexagonVertices[valueIndex++] = anchor1Y;
				hexagonVertices[valueIndex++] = -sideLength;

				hexagonVertices[valueIndex++] = -direction2.second;
				hexagonVertices[valueIndex++] = direction2.first;
				hexagonVertices[valueIndex++] = 0.0F;

				hexagonVertices[valueIndex++] = anchor2X;
				hexagonVertices[valueIndex++] = anchor2Y;
				hexagonVertices[valueIndex++] = -sideLength;
			}
		}
	}
	/*

		std::size_t ringIndex = 6*index;

		hexagonVertices[ringIndex++] = 0.0F;
		hexagonVertices[ringIndex++] = 0.0F;
		hexagonVertices[ringIndex++] = 1.0F;

		hexagonVertices[ringIndex++] = ( sideLength - cornerRadius ) * x;
		hexagonVertices[ringIndex++] = ( sideLength - cornerRadius ) * y;
		hexagonVertices[ringIndex++] = 0.0F;

		ringIndex += 30;

		hexagonVertices[ringIndex++] = x;
		hexagonVertices[ringIndex++] = y;
		hexagonVertices[ringIndex++] = 0.0F;
		
		hexagonVertices[ringIndex++] = ( sideLength ) * x;
		hexagonVertices[ringIndex++] = ( sideLength ) * y;
		hexagonVertices[ringIndex++] = -cornerRadius;

		ringIndex += 30;

		hexagonVertices[ringIndex++] = x;
		hexagonVertices[ringIndex++] = y;
		hexagonVertices[ringIndex++] = 0.0F;

		hexagonVertices[ringIndex++] = ( sideLength ) * x;
		hexagonVertices[ringIndex++] = ( sideLength ) * y;
		hexagonVertices[ringIndex++] = -( 2*sideLength );

	}
	*/
	std::vector<BYTE> geometryIndices( (2*nCorners + 4*nCorners*(nRings-1)) * sizeof( UINT32 ) );
	UINT32* hexagonIndices = reinterpret_cast<UINT32*>( geometryIndices.data() );
	
	std::size_t bufferPos = 0;
	hexagonIndices[bufferPos++] = 1;
	hexagonIndices[bufferPos++] = 0;
	hexagonIndices[bufferPos++] = 2;
	hexagonIndices[bufferPos++] = 11;
	hexagonIndices[bufferPos++] = 3;
	hexagonIndices[bufferPos++] = 10;
	hexagonIndices[bufferPos++] = 4;
	hexagonIndices[bufferPos++] = 9;
	hexagonIndices[bufferPos++] = 5;
	hexagonIndices[bufferPos++] = 8;
	hexagonIndices[bufferPos++] = 6;
	hexagonIndices[bufferPos++] = 7;
	
	std::size_t startVertexIndex = hexagonIndices[bufferPos-1];

	for( std::size_t ring = 0; ring < nRings - 1; ++ring )
	{
		for( std::size_t count = 0; count < 2 * nCorners - 1; ++count )
		{
			hexagonIndices[bufferPos++] = ( startVertexIndex + count ) % ( 2 * nCorners ) + 2 * nCorners * ( ring + 1 );
			hexagonIndices[bufferPos++] = ( startVertexIndex + 1 + count ) % ( 2 * nCorners ) + 2 * nCorners * ( ring );
		}
		std::size_t value = ( startVertexIndex + ( 2 * nCorners - 1 ) ) % ( 2 * nCorners ) + 2 * nCorners * ( ring + 1 );
		hexagonIndices[bufferPos++] = value;
		hexagonIndices[bufferPos++] = value + 1;
	}

	const std::size_t vertexIndex = addSubResource( geometryVertices.size(), GameSubResource::SubResourceType::eVertexBuffer );
	subresources[vertexIndex].setData( std::move( geometryVertices ) );

	const std::size_t indexIndex = addSubResource( geometryIndices.size(), GameSubResource::SubResourceType::eIndexBuffer );
	subresources.back().setData( std::move( geometryIndices ) );

	indexed_geometry_objects.emplace_back( vertexIndex, indexIndex, BoundingBox( minX, minY, -sideLength, maxX, maxY, 0 ) );
}

std::size_t noxcain::ResourceEngine::addSubResource( std::size_t resourceSize, GameSubResource::SubResourceType resourceType )
{
	std::size_t index = subresources.size();
	subresources.emplace_back( resourceSize, resourceType );
	resource_limits.max_size_per_resource = std::max( resource_limits.max_size_per_resource, resourceSize );
	return index;
}

noxcain::ResourceEngine::ResourceEngine()
{

	read_font( 
		{
			"Assets/Fonts/OpenSans-Regular.ttf",
			"Assets/Fonts/28 Days Later.ttf",
			"Assets/Fonts/Ornaments Salad.otf",
			"Assets/Fonts/zenda.ttf"
		} );
	read_hex_geometry();
}

noxcain::ResourceEngine::~ResourceEngine()
{
}

noxcain::ResourceEngine& noxcain::ResourceEngine::get_engine()
{
	std::unique_lock lock( resource_mutex );
	if( !resources ) resources.reset( new ResourceEngine );
	return *resources;
}

const noxcain::FontResource& noxcain::ResourceEngine::get_font( std::size_t index ) const
{
	if( index >= get_invalid_font_id() )
	{
		return font_resources[0];
	}
	return font_resources[index];
}

const noxcain::GeometryResource& noxcain::ResourceEngine::get_geometry( std::size_t index ) const
{
	return indexed_geometry_objects[index];
}

std::size_t noxcain::ResourceEngine::get_invalid_geomtry_id() const
{
	return indexed_geometry_objects.size();
}

std::size_t noxcain::ResourceEngine::get_invalid_font_id() const
{
	return font_resources.size();
}

const std::vector<noxcain::GameSubResource>& noxcain::ResourceEngine::get_subresources() const
{
	return subresources;
}

noxcain::SubResourceCollection noxcain::ResourceEngine::getSubResourcesMetaInfos( GameSubResource::SubResourceType wantedType ) const
{
	return SubResourceCollection( subresources, wantedType );
}
