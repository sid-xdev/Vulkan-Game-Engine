#include "VectorText.hpp"

#include <resources/GameResourceEngine.hpp>
#include <resources/FontResource.hpp>

#include <cmath>

void noxcain::VectorText::calculate_offsets()
{
	const auto& font = ResourceEngine::get_engine().get_font( font_index );
	
	glyphs.clear();
	line_lengths.clear();
	glyphs.reserve( unicodes.size() );

	max_width = 0;
	DOUBLE x_offset = 0;
	
	for( const auto unicode : unicodes )
	{
		const auto info = font.getCharacterInfo( unicode );
		
		if( ( fixed_line_length && x_offset > 0 && x_offset + info.advance_width > fixed_line_length ) || unicode == 0xA )
		{
			line_lengths.push_back( x_offset );
			x_offset = 0;
		}

		if( unicode != 0xA )
		{
			if( info.glyph_index != FontResource::INVALID_UNICODE )
			{
				glyphs.push_back( { x_offset, info.glyph_index, 0, UINT8( line_lengths.size() ) } );
			}
			x_offset += info.advance_width;
			if( max_width < x_offset ) max_width = x_offset;
		}
	};
	line_lengths.push_back( x_offset );
	max_height = line_lengths.size() * line_height - font.getLineGap();
}

void noxcain::VectorText::set_base_color( const std::array<FLOAT32, 4>& color )
{
	colors[0] = color;
}

noxcain::VectorText::VectorText()
{
	set_font_id( 1 );
}

void noxcain::VectorText::set_utf8( const std::string& utf8String )
{
	const static BYTE prefixBase = 0x80;

	unicodes.clear();
	unicodes.reserve( utf8String.size() );

	std::size_t charIndex = 0;
	std::size_t pos = 0;
	while( pos < utf8String.size() )
	{
		std::size_t length = 0;
		while( utf8String[pos] & ( prefixBase >> length++ ) );

		UINT32 unicode = utf8String[pos++] & ( 0xFF >> length );

		for( std::size_t index = 1; index < length - 1; ++index )
		{
			unicode = ( unicode << 6 ) | ( UINT32( 0x3F & utf8String[pos++] ) );
		}
		if( unicode == '\0' ) break;
		unicodes.push_back( unicode );
	};

	calculate_offsets();
}

void noxcain::VectorText::set_unicodes( const std::vector<UINT32>& new_unicodes )
{
	unicodes.assign( new_unicodes.begin(), new_unicodes.end() );
	calculate_offsets();
}

bool noxcain::VectorText::empty() const
{
	return glyphs.empty();
}

void noxcain::VectorText::set_font_id( std::size_t id )
{
	font_index = id;
	const auto& font = ResourceEngine::get_engine().get_font( font_index );
	line_height = font.getAscender() - font.getDescender() + font.getLineGap();
	calculate_offsets();
}

const noxcain::FontResource& noxcain::VectorText::get_font() const
{
	return ResourceEngine::get_engine().get_font( font_index );
}

noxcain::DOUBLE noxcain::VectorText::get_descender() const
{
	return get_font().getDescender();
}

void noxcain::VectorText::set_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha )
{
	set_base_color( { red, green, blue, alpha } );
}

void noxcain::VectorText::set_color( const std::array<FLOAT32, 4>& color )
{
	set_base_color( color );
}
