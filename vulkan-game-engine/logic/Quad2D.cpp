#include "Quad2D.hpp"

noxcain::RenderableQuad2D::RenderableQuad2D( Renderable<RenderableQuad2D>::List& visibility_list ) : Renderable<RenderableQuad2D>( visibility_list )
{
}

void noxcain::RenderableQuad2D::set_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha )
{
	color = { red, green, blue, alpha };
}

void noxcain::RenderableQuad2D::set_color( const std::array<FLOAT32, 4>& label_color )
{
	color = label_color;
}

vk::Rect2D noxcain::RenderableQuad2D::record( const vk::CommandBuffer& command_buffer, vk::PipelineLayout pipeline_layout, vk::Rect2D last_scissor, vk::Rect2D default_scissor ) const
{
	std::array<BYTE, VERTEX_PUSH_SIZE> vertex_push_constants;
	std::array<BYTE, FRAGMENT_PUSH_SIZE> fragment_push_constants;

	FLOAT32* floats = reinterpret_cast<FLOAT32*>( vertex_push_constants.data() );
	floats[0] = FLOAT32( INT32( get_left() + 0.5 ) );
	floats[1] = FLOAT32( INT32( get_bottom() + 0.5 ) );
	floats[2] = FLOAT32( INT32( get_width() + 0.5 ) );
	floats[3] = FLOAT32( INT32( get_height() + 0.5 ) );



	floats = reinterpret_cast<FLOAT32*>( fragment_push_constants.data() );
	floats[0] = color[0];
	floats[1] = color[1];
	floats[2] = color[2];
	floats[3] = color[3];

	vk::Rect2D own_scissor = scissor ? vk::Rect2D( vk::Offset2D( INT32( scissor->get_left() + 0.5 ), INT32( default_scissor.extent.height - scissor->get_top() + 0.5 ) ), 
												   vk::Extent2D( INT32( scissor->get_width() + 0.5 ), INT32( scissor->get_height() + 0.5 ) ) ) : default_scissor;

	if( own_scissor != last_scissor )
	{
		command_buffer.setScissor( 0, { own_scissor } );
		last_scissor = own_scissor;
	}
	command_buffer.pushConstants( pipeline_layout, vk::ShaderStageFlagBits::eFragment, FRAGMENT_PUSH_OFFSET, UINT32( fragment_push_constants.size() ), fragment_push_constants.data() );
	command_buffer.pushConstants( pipeline_layout, vk::ShaderStageFlagBits::eVertex, VERTEX_PUSH_OFFSET, UINT32( vertex_push_constants.size() ), vertex_push_constants.data() );
	command_buffer.draw( 4, 1, 0, 0 );

	return last_scissor;
}
