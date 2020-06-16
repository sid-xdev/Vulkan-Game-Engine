#include <Defines.hpp>
#include <shader/shader.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>

namespace noxcain
{
	class ShaderManager
	{
	public:
		
	private:

		std::array<vk::ShaderModule, VERTEX_SHADER_COUNT + FRAGMENT_SHADER_COUNT + COMPUTE_SHADER_COUNT> shaderModuls;
		vk::ShaderModule createShader( const uint32_t* shader_binary, std::size_t size ) const;

		bool set( VertexShaderIds id, vk::ShaderModule shader );
		bool set( FragmentShaderIds id, vk::ShaderModule shader );
		bool set( ComputeShaderIds id, vk::ShaderModule shader );

	public:
		ShaderManager( const ShaderManager& ) = delete;
		ShaderManager& operator=( const ShaderManager& ) = delete;
	
		ShaderManager();
		bool initialize() noexcept( false );
		~ShaderManager();

		vk::ShaderModule get( VertexShaderIds id ) const;
		vk::ShaderModule get( FragmentShaderIds id ) const;
		vk::ShaderModule get( ComputeShaderIds id ) const;
	};
}