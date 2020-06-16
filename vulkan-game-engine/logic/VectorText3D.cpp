#include "VectorText3D.hpp"
#include <math/Matrix.hpp>
#include <resources/FontResource.hpp>

noxcain::VectorText3D::VectorText3D( Renderable<VectorText3D>::List& visibility_list ) : Renderable<VectorText3D>( visibility_list )
{
}

noxcain::DOUBLE noxcain::VectorText3D::get_width()
{
	return text.size * text.max_width;
}

noxcain::DOUBLE noxcain::VectorText3D::get_height()
{
	return text.size * text.max_height;
}

void noxcain::VectorText3D::record( vk::CommandBuffer command_buffer, vk::PipelineLayout pipeline_layout, const NxMatrix4x4& camera ) const
{
	std::array<BYTE, 32>fragmentPushConstants;
	
	for( std::size_t current_glyph = 0; current_glyph< text.glyphs.size(); ++current_glyph )
	{
		
		const auto& glyph = text.glyphs[current_glyph];

		UINT32 current_glyph_id = glyph.glyph_id + text.get_font().get_font_offset();

		FLOAT32* targetColorBuffer = reinterpret_cast<FLOAT32*>( fragmentPushConstants.data() );
		targetColorBuffer[1] = FLOAT32( text.colors[glyph.color_index][0] ) / 0xFF;
		targetColorBuffer[2] = FLOAT32( text.colors[glyph.color_index][1] ) / 0xFF;
		targetColorBuffer[3] = FLOAT32( text.colors[glyph.color_index][2] ) / 0xFF;
		targetColorBuffer[4] = FLOAT32( text.colors[glyph.color_index][3] ) / 0xFF;

		reinterpret_cast<UINT32*>( targetColorBuffer )[0] = current_glyph_id;

		DOUBLE aligment_offset = 0;
		if( text.text_alignment == VectorText::Alignments::CENTER )
		{
			aligment_offset = 0.5 * ( text.max_width - text.line_lengths[glyph.line_index] );
		}
		else if( text.text_alignment == VectorText::Alignments::RIGHT )
		{
			aligment_offset = ( text.max_width - text.line_lengths[glyph.line_index] );
		}

		auto matrix = ( camera * global_matrix * NxMatrix4x4( {
				text.size,
				0,
				0,
				0,

				0,
				text.size,
				0,
				0,

				0,
				0,
				text.size,
				0,

				( glyph.x_offset + aligment_offset ) * text.size,
				(( text.line_lengths.size() - 1 - glyph.line_index ) * text.line_height ) * text.size,
				0,
				1 } ) ).gpuData();
	
		command_buffer.pushConstants( pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, UINT32( fragmentPushConstants.size() ), fragmentPushConstants.data() );
		command_buffer.pushConstants( pipeline_layout, vk::ShaderStageFlagBits::eVertex, 32, UINT32( matrix.size() ), matrix.data() );
		command_buffer.draw( 4, 1, current_glyph_id * 4, 0 );
	}
}
