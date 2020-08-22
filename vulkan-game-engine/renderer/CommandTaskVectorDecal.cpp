#include "CommandTasks.hpp"

#include <renderer/DescriptorSetManager.hpp>
#include <renderer/GameGraphicEngine.hpp>
#include <renderer/MemoryManagement.hpp>
#include <renderer/RenderQuery.hpp>

#include <logic/GameLogicEngine.hpp>
#include <logic/Level.hpp>
#include <logic/VectorText3D.hpp>

#include <resources/GameResourceEngine.hpp>
#include <resources/FontResource.hpp>

#include <tools/ResultHandler.hpp>

noxcain::VectorDecalTask::VectorDecalTask()
{
}

noxcain::VectorDecalTask::~VectorDecalTask()
{
	shutdown_task();
	vk::Device device = GraphicEngine::get_device();
	if( device )
	{
		ResultHandler r_handler( vk::Result::eSuccess );
		r_handler << device.waitIdle();
		if( r_handler.all_okay() )
		{
			device.destroyPipeline( vector_decal_pipeline );
			device.destroyPipelineLayout( vector_decal_pipeline_layout );
		}
	}
}

bool noxcain::VectorDecalTask::buffer_independent_preparation()
{
	if( !vector_decal_pipeline_layout )
	{
		if( !setup_layout() ) return false;
	}

	auto resolution = LogicEngine::get_graphic_settings().get_accumulated_resolution();
	vk::Extent2D current_extent = vk::Extent2D( resolution.width, resolution.height );
	if( is_new_render_pass || old_extent != current_extent )
	{
		if( !build_vector_decal_pipeline() ) return false;
		old_extent = current_extent;
		is_new_render_pass = false;
	};

	return  true;
}

bool noxcain::VectorDecalTask::buffer_dependent_preparation( CommandData& pool_data )
{
	ResultHandler r_handle( vk::Result::eSuccess );
	return buffer_preparation( pool_data, 1 );
}

bool noxcain::VectorDecalTask::record( const std::vector<vk::CommandBuffer>& buffers )
{
	TimeFrame frame( time_col, 0.4F, 0.0F, 0.6F, 1.0F, "record" );
	ResultHandler r_handler( vk::Result::eSuccess );

	const auto& resources = ResourceEngine::get_engine();

	const auto c_buffer = buffers.front();
	const auto& decal_list = LogicEngine::get_vector_decals();

	const vk::CommandBufferInheritanceInfo inharitage( render_pass, subpass_index, frame_buffers.empty() ? vk::Framebuffer() : frame_buffers.front() );
	r_handler << c_buffer.begin( vk::CommandBufferBeginInfo( vk::CommandBufferUsageFlagBits::eRenderPassContinue | vk::CommandBufferUsageFlagBits::eOneTimeSubmit, &inharitage ) );

	if( !decal_list.empty() )
	{
		const auto& vertex_block_info = GraphicEngine::get_memory_manager().get_block( ResourceEngine::get_engine().get_font( 0 ).get_vertex_block_id() );
		c_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, vector_decal_pipeline );
		c_buffer.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, vector_decal_pipeline_layout, 0, { GraphicEngine::get_descriptor_set_manager().get_basic_set( BasicDescriptorSets::GLYPHS ) }, {} );
		c_buffer.bindVertexBuffers( 0, { vertex_block_info.buffer }, { vertex_block_info.offset } );

		std::size_t current_font_id = ResourceEngine::get_engine().get_invalid_font_id();
		for( const Renderable<VectorText3D>::List& decals : decal_list )
		{
			for( const VectorText3D& decal_string : decals )
			{
				decal_string.record( c_buffer, vector_decal_pipeline_layout, LogicEngine::get_camera_matrix() );
			}
		}
	}

	vk::QueryPool timestamp_pool = GraphicEngine::get_render_query().get_timestamp_pool();
	if( timestamp_pool )
	{
		c_buffer.writeTimestamp( vk::PipelineStageFlagBits::eBottomOfPipe, timestamp_pool, (UINT32)RenderQuery::TimeStampIds::AFTER_GLYPHS );
	}

	r_handler << c_buffer.end();

	
	return r_handler.all_okay();
}

bool noxcain::VectorDecalTask::setup_layout()
{
	ResultHandler r_handler( vk::Result::eSuccess );
	
	vk::Device device = GraphicEngine::get_device();
	
	if( device )
	{
		std::array<vk::DescriptorSetLayout, 1> decal_descriptor_sets =
		{
			GraphicEngine::get_descriptor_set_manager().get_layout( DescriptorSetLayouts::GLYPH )
		};

		std::array<vk::PushConstantRange, 2> decal_push_constants =
		{
			vk::PushConstantRange( vk::ShaderStageFlagBits::eFragment, 0, 32 ),
			vk::PushConstantRange( vk::ShaderStageFlagBits::eVertex, 32, 64 )
		};

		vector_decal_pipeline_layout = r_handler << device.createPipelineLayout( vk::PipelineLayoutCreateInfo( vk::PipelineLayoutCreateFlags(), decal_descriptor_sets.size(), decal_descriptor_sets.data(), decal_push_constants.size(), decal_push_constants.data() ) );
		return r_handler.all_okay();
	}
	return false;
}

inline bool noxcain::VectorDecalTask::build_vector_decal_pipeline()
{
	const auto g_settings = LogicEngine::get_graphic_settings();
	const auto resolution = g_settings.get_accumulated_resolution();

	auto special = createSpecialization( FLOAT32( 2.0F / resolution.width ), FLOAT32( 2.0F / resolution.height ), UINT32( ResourceEngine::get_engine().get_resource_limits().glyph_count ) );
	vk::SpecializationInfo specializationInfo( special.descriptions.size(), special.descriptions.data(), special.data.size(), special.data.data() );

	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages =
	{
		vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, GraphicEngine::get_shader( VertexShaderIds::GLYPH_QUAD_3D ), "main", &specializationInfo ),
		vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, GraphicEngine::get_shader( FragmentShaderIds::GLYPH_CONTOUR_3D ), "main", &specializationInfo )
	};


	std::array<vk::Viewport, 1> viewports =
	{
		vk::Viewport( 0.0F, 0.0F, FLOAT32( resolution.width ), FLOAT32( resolution.height ), 0.0F, 1.0F )
	};

	std::array<vk::Rect2D, viewports.size()> scissors =
	{
		vk::Rect2D( vk::Offset2D( 0, 0 ), vk::Extent2D( resolution.width, resolution.height ) )
	};

	vk::PipelineViewportStateCreateInfo viewport_state(
		vk::PipelineViewportStateCreateFlags(),
		viewports.size(), viewports.data(),
		scissors.size(), scissors.data() );

	vk::PipelineRasterizationStateCreateInfo rasterization_state(
		vk::PipelineRasterizationStateCreateFlags(),
		VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise,
		VK_TRUE, -0.1F, 0.0F, 0.0F, 1.0F );

	vk::PipelineMultisampleStateCreateInfo multisample_state(
		vk::PipelineMultisampleStateCreateFlags(), static_cast<vk::SampleCountFlagBits>( g_settings.get_sample_count() ), VK_FALSE, 0.0F, nullptr, VK_FALSE, VK_FALSE );

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state(
		vk::PipelineDepthStencilStateCreateFlags(), VK_TRUE, VK_FALSE, vk::CompareOp::eLessOrEqual, VK_FALSE, VK_FALSE,
		vk::StencilOpState(), vk::StencilOpState(), 0.0F, 0.0F );

	std::array<vk::PipelineColorBlendAttachmentState, 3> color_blend_attachment_state =
	{
		vk::PipelineColorBlendAttachmentState( VK_TRUE, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eZero, vk::BlendFactor::eOne, vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB ),

		vk::PipelineColorBlendAttachmentState( VK_TRUE, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eZero, vk::BlendFactor::eOne, vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB ),

		vk::PipelineColorBlendAttachmentState( VK_TRUE, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eZero, vk::BlendFactor::eOne, vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB ),
	};

	vk::PipelineColorBlendStateCreateInfo color_blend_state(
		vk::PipelineColorBlendStateCreateFlags(), VK_FALSE, vk::LogicOp::eClear, color_blend_attachment_state.size(), color_blend_attachment_state.data(), { 0.0F, 0.0F, 0.0F, 0.0F } );

	std::array<vk::VertexInputBindingDescription, 1> input_bindings =
	{
		vk::VertexInputBindingDescription( 0, 2 * sizeof( FLOAT32 ), vk::VertexInputRate::eVertex )
	};

	std::array<vk::VertexInputAttributeDescription, 1> input_attribute_description =
	{
		vk::VertexInputAttributeDescription( 0, 0, vk::Format::eR32G32Sfloat, 0 )
	};

	vk::PipelineVertexInputStateCreateInfo vertex_state(
		vk::PipelineVertexInputStateCreateFlags(), input_bindings.size(), input_bindings.data(), input_attribute_description.size(), input_attribute_description.data() );

	vk::PipelineInputAssemblyStateCreateInfo assembly_state( vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleStrip, VK_FALSE );

	ResultHandler r_handler( vk::Result::eSuccess );
	const vk::Device& device = GraphicEngine::get_device();
	if( !device )
	{
		//TODO log message
		return false;
	}

	if( vector_decal_pipeline )
	{
		r_handler << device.waitIdle();
		if( r_handler.all_okay() )
		{
			device.destroyPipeline( vector_decal_pipeline );
		}
		else
		{
			return false;
		}
	}

	vector_decal_pipeline = r_handler << device.createGraphicsPipeline( vk::PipelineCache(), vk::GraphicsPipelineCreateInfo(
		vk::PipelineCreateFlags(),
		shaderStages.size(), shaderStages.data(),
		&vertex_state,
		&assembly_state,
		nullptr,
		&viewport_state,
		&rasterization_state,
		&multisample_state,
		&depth_stencil_state,
		&color_blend_state,
		nullptr, vector_decal_pipeline_layout, render_pass, subpass_index, vk::Pipeline(), -1 ) );
	return r_handler.all_okay();
}
