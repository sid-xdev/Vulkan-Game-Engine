#include "VectorText2D.hpp"
#include <resources/FontResource.hpp>

noxcain::VectorText2D::VectorText2D( Renderable<VectorText2D>::List& visibility_list ) : Renderable<VectorText2D>( visibility_list )
{
	width = [this]()
	{
		return text.size * text.max_width;
	};

	height = [this]()
	{
		return text.size * text.max_height;
	};
}

vk::Rect2D noxcain::VectorText2D::record( const vk::CommandBuffer& command_buffer, vk::PipelineLayout pipeline_layout, vk::Rect2D current_scissor, vk::Rect2D default_scissor ) const
{
	for( std::size_t current_glyph = 0; current_glyph < text.glyphs.size(); ++current_glyph )
	{
		std::array<BYTE, 64> vertex_push_constants;
		std::array<BYTE, 32> fragment_push_constants;

		const auto& glyph = text.glyphs[current_glyph];
		const UINT32 current_glyph_id = glyph.glyph_id + text.get_font().get_font_offset();

		( reinterpret_cast<UINT32*>( fragment_push_constants.data() ) )[0] = glyph.glyph_id;
		FLOAT32* targetColorBuffer = reinterpret_cast<FLOAT32*>( fragment_push_constants.data() );
		targetColorBuffer[1] = FLOAT32( text.colors[glyph.color_index][0] ) / 0xFF;
		targetColorBuffer[2] = FLOAT32( text.colors[glyph.color_index][1] ) / 0xFF;
		targetColorBuffer[3] = FLOAT32( text.colors[glyph.color_index][2] ) / 0xFF;
		targetColorBuffer[4] = FLOAT32( text.colors[glyph.color_index][3] ) / 0xFF;
		targetColorBuffer[5] = FLOAT32( text.size );

		DOUBLE aligment_offset = 0;
		if( text.text_alignment == VectorText::Alignments::CENTER )
		{
			aligment_offset = 0.5 * ( text.max_width - text.line_lengths[glyph.line_index] );
		}
		else if( text.text_alignment == VectorText::Alignments::RIGHT )
		{
			aligment_offset = ( text.max_width - text.line_lengths[glyph.line_index] );
		}

		FLOAT32* vertexPush = reinterpret_cast<FLOAT32*>( vertex_push_constants.data() );
		vertexPush[0] = FLOAT32( text.size );
		vertexPush[1] = 1;
		vertexPush[2] = 0;
		vertexPush[3] = FLOAT32( INT32( get_left() + text.size * ( aligment_offset + glyph.x_offset ) + 0.5 ) );
		vertexPush[4] = FLOAT32( INT32( get_bottom() + text.size * ( -text.get_descender() + ( text.line_lengths.size() - 1 - glyph.line_index ) * text.line_height ) + 0.5 ) );
		
		vk::Rect2D own_scissor = scissor ? vk::Rect2D( vk::Offset2D( INT32( scissor->get_left() + 0.5 ), INT32( default_scissor.extent.height - scissor->get_top() + 0.5 ) ), vk::Extent2D( UINT32( scissor->get_width() + 0.5 ), UINT32( scissor->get_height() + 0.5 ) ) ) : default_scissor;
		if( own_scissor != current_scissor )
		{
			command_buffer.setScissor( 0, { own_scissor } );
			current_scissor = own_scissor;
		}

		command_buffer.pushConstants( pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, UINT32( fragment_push_constants.size() ), fragment_push_constants.data() );
		command_buffer.pushConstants( pipeline_layout, vk::ShaderStageFlagBits::eVertex, 32, UINT32( vertex_push_constants.size() ), vertex_push_constants.data() );
		command_buffer.draw( 4U, 1U, glyph.glyph_id * 4U, 0 );
	}
	return current_scissor;
}
