#include "ShaderManager.hpp"

#include <renderer/GameGraphicEngine.hpp>
#include <tools/ResultHandler.hpp>

#include <fstream>

vk::ShaderModule noxcain::ShaderManager::createShader( const uint32_t* shader_binary, std::size_t size ) const
{
	ResultHandler<vk::Result> resultHandler( vk::Result::eSuccess );
	const vk::Device& device = GraphicEngine::get_device();

	return resultHandler << device.createShaderModule( vk::ShaderModuleCreateInfo( vk::ShaderModuleCreateFlags(), size, shader_binary ) );
}

bool noxcain::ShaderManager::set( VertexShaderIds id, vk::ShaderModule shader )
{
	shaderModuls[static_cast<UINT32>( id )] = shader;
	return bool(shader);
}

bool noxcain::ShaderManager::set( FragmentShaderIds id, vk::ShaderModule shader )
{
	shaderModuls[VERTEX_SHADER_COUNT + static_cast<UINT32>( id )] = shader;
	return bool( shader );
}

bool noxcain::ShaderManager::set( ComputeShaderIds id, vk::ShaderModule shader )
{
	shaderModuls[VERTEX_SHADER_COUNT + FRAGMENT_SHADER_COUNT + static_cast<UINT32>( id )] = shader;
	return bool( shader );
}

noxcain::ShaderManager::ShaderManager()
{
}

bool noxcain::ShaderManager::initialize() noexcept( false )
{
#define SHADER_ARRAY( shader ) shader,  sizeof( shader ) //*sizeof( uint32_t )
	bool all_okay = true;
	all_okay = all_okay && set( VertexShaderIds::GLYPH_QUAD_2D, createShader( SHADER_ARRAY( vertex_shader_glyph_quad_2D ) ) );
	all_okay = all_okay && set( VertexShaderIds::FULL_SCREEN, createShader( SHADER_ARRAY( vertex_shader_full_screen ) ) );
	all_okay = all_okay && set( VertexShaderIds::DEFERRED_GEOMETRY, createShader( SHADER_ARRAY( vertex_shader_deferred_geometry ) ) );
	all_okay = all_okay && set( VertexShaderIds::GLYPH_QUAD_3D, createShader( SHADER_ARRAY( vertex_shader_glyph_quad_3D ) ) );
	all_okay = all_okay && set( VertexShaderIds::NORMAL, createShader( SHADER_ARRAY( vertex_shader_normal ) ) );
	all_okay = all_okay && set( VertexShaderIds::LABEL, createShader( SHADER_ARRAY( vertex_shader_label ) ) );

	all_okay = all_okay && set( FragmentShaderIds::GLYPH_CONTOUR_2D, createShader( SHADER_ARRAY( fragment_shader_glyph_contour_2D ) ) );
	all_okay = all_okay && set( FragmentShaderIds::GLYPH_CONTOUR_3D, createShader( SHADER_ARRAY( fragment_shader_glyph_contour_3D ) ) );
	all_okay = all_okay && set( FragmentShaderIds::FINALIZE, createShader( SHADER_ARRAY( fragment_shader_finalize ) ) );
	all_okay = all_okay && set( FragmentShaderIds::DEFERRED_GEOMETRY, createShader( SHADER_ARRAY( fragment_shader_deferred_geometry ) ) );
	all_okay = all_okay && set( FragmentShaderIds::MULTI_SHADING, createShader( SHADER_ARRAY( fragment_shader_multi_shading ) ) );
	all_okay = all_okay && set( FragmentShaderIds::SINGLE_SHADING, createShader( SHADER_ARRAY( fragment_shader_single_shading ) ) );
	all_okay = all_okay && set( FragmentShaderIds::NORMAL, createShader( SHADER_ARRAY( fragment_shader_normal ) ) );
	all_okay = all_okay && set( FragmentShaderIds::LABEL, createShader( SHADER_ARRAY( fragment_shader_label ) ) );
	all_okay = all_okay && set( FragmentShaderIds::EDGE_DETECTION, createShader( SHADER_ARRAY( fragment_shader_edge_detection ) ) );
	return all_okay;
}

noxcain::ShaderManager::~ShaderManager()
{
	const vk::Device device = GraphicEngine::get_device();
	if( device )
	{
		ResultHandler<vk::Result> r_handler( vk::Result::eSuccess );
		r_handler << device.waitIdle();
		if( r_handler.all_okay() )
		{
			for( vk::ShaderModule& shaderModul : shaderModuls )
			{
				device.destroyShaderModule( shaderModul );
			}
		}
	}
}

vk::ShaderModule noxcain::ShaderManager::get( VertexShaderIds id ) const
{
	return shaderModuls[static_cast<UINT32>( id )];
}

vk::ShaderModule noxcain::ShaderManager::get( FragmentShaderIds id ) const
{
	return shaderModuls[VERTEX_SHADER_COUNT + static_cast<UINT32>( id )];
}

vk::ShaderModule noxcain::ShaderManager::get( ComputeShaderIds id ) const
{
	return shaderModuls[VERTEX_SHADER_COUNT + FRAGMENT_SHADER_COUNT + static_cast<UINT32>( id )];
}
