#include "CommandTasks.hpp"

#include <tools/ResultHandler.hpp>

#include <renderer/GameGraphicEngine.hpp>
#include <renderer/MemoryManagement.hpp>
#include <renderer/RenderQuery.hpp>

#include <logic/GameLogicEngine.hpp>
#include <logic/GeometryLogic.hpp>

#include <resources/GameResourceEngine.hpp>
#include <resources/GeometryResource.hpp>


bool noxcain::GeometryTask::setup_layout()
{
	auto device = GraphicEngine::get_device();
	if( device )
	{
		ResultHandler<vk::Result> r_handler( vk::Result::eSuccess );

		std::array<vk::PushConstantRange, 1> push_constants =
		{
			vk::PushConstantRange( vk::ShaderStageFlagBits::eVertex, 0, 128 ),
		};

		std::array<vk::DescriptorSetLayout, 0> descriptor_set_layouts =
		{
		};

		geomtry_pipeline_layout = r_handler << device.createPipelineLayout( vk::PipelineLayoutCreateInfo( vk::PipelineLayoutCreateFlags(), descriptor_set_layouts.size(), descriptor_set_layouts.data(), push_constants.size(), push_constants.data() ) );

		return r_handler.all_okay();
	}
	return false;
}

bool noxcain::GeometryTask::build_geomtry_pipeline()
{
	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages =
	{
		vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, GraphicEngine::get_shader( VertexShaderIds::DEFERRED_GEOMETRY ), "main", nullptr ),
		vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, GraphicEngine::get_shader( FragmentShaderIds::DEFERRED_GEOMETRY ), "main", nullptr )
	};

	auto graphic_settings = LogicEngine::get_graphic_settings();
	auto resolution = graphic_settings.get_accumulated_resolution();
	const vk::Extent2D extent = vk::Extent2D( resolution.width, resolution.height );
	std::array<vk::Viewport, 1> viewports =
	{
		vk::Viewport( 0.0F, 0.0F, float( extent.width ), float( extent.height ), 0.0F, 1.0F )
	};

	std::array<vk::Rect2D, viewports.size()> scissors =
	{
		vk::Rect2D( vk::Offset2D( 0, 0 ), extent )
	};

	vk::PipelineViewportStateCreateInfo viewportSate(
		vk::PipelineViewportStateCreateFlags(),
		viewports.size(), viewports.data(),
		scissors.size(), scissors.data() );

	vk::PipelineRasterizationStateCreateInfo rasterizationState(
		vk::PipelineRasterizationStateCreateFlags(),
		VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise,
		VK_FALSE, 0.0F, 0.0F, 0.0F, 1.0F );

	vk::PipelineMultisampleStateCreateInfo multisampleState(
		vk::PipelineMultisampleStateCreateFlags(), static_cast<vk::SampleCountFlagBits>( graphic_settings.get_sample_count() ), VK_FALSE, 0.0F, nullptr, VK_FALSE );

	vk::PipelineDepthStencilStateCreateInfo depthStencilState(
		vk::PipelineDepthStencilStateCreateFlags(), VK_TRUE, VK_TRUE, vk::CompareOp::eLessOrEqual, VK_FALSE, VK_FALSE,
		vk::StencilOpState(), vk::StencilOpState(), 0.0F, 0.0F );

	vk::PipelineColorBlendAttachmentState disableBlendState( VK_FALSE,
															 vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
															 vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA );

	std::array<vk::PipelineColorBlendAttachmentState, 3> attachmentState =
	{
		disableBlendState,
		disableBlendState,
		disableBlendState
	};

	vk::PipelineColorBlendStateCreateInfo colorBlendState(
		vk::PipelineColorBlendStateCreateFlags(), VK_FALSE, vk::LogicOp::eClear, attachmentState.size(), attachmentState.data(), { 0.0F, 0.0F, 0.0F, 0.0F } );

	std::array<vk::VertexInputBindingDescription, 1> inputBindings =
	{
		vk::VertexInputBindingDescription( 0, 6 * sizeof( FLOAT32 ), vk::VertexInputRate::eVertex )
	};

	std::array<vk::VertexInputAttributeDescription, 2> inputAttributeDescription =
	{
		vk::VertexInputAttributeDescription( 0, 0, vk::Format::eR32G32B32Sfloat, 0 ),
		vk::VertexInputAttributeDescription( 1, 0, vk::Format::eR32G32B32Sfloat, 3 * sizeof( FLOAT32 ) )
	};

	vk::PipelineVertexInputStateCreateInfo vertexState(
		vk::PipelineVertexInputStateCreateFlags(), inputBindings.size(), inputBindings.data(), inputAttributeDescription.size(), inputAttributeDescription.data() );

	vk::PipelineInputAssemblyStateCreateInfo assemblyState( vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleStrip, VK_FALSE );

	ResultHandler r_handler( vk::Result::eSuccess );
	const vk::Device& device = GraphicEngine::get_device();
	if( !device )
	{
		//TODO log message
		return false;
	}

	if( geomtry_pipeline )
	{
		r_handler << device.waitIdle();
		if( r_handler.all_okay() )
		{
			device.destroyPipeline( geomtry_pipeline );
		}
		else
		{
			return false;
		}
	}

	geomtry_pipeline = r_handler << device.createGraphicsPipeline( vk::PipelineCache(), vk::GraphicsPipelineCreateInfo(
		vk::PipelineCreateFlags(),
		shaderStages.size(), shaderStages.data(),
		&vertexState,
		&assemblyState,
		nullptr,
		&viewportSate,
		&rasterizationState,
		&multisampleState,
		&depthStencilState,
		&colorBlendState,
		nullptr, geomtry_pipeline_layout, render_pass, subpass_index, vk::Pipeline(), -1 ) );
	return r_handler.all_okay();
}

noxcain::GeometryTask::GeometryTask()
{
}

noxcain::GeometryTask::~GeometryTask()
{
	shutdown_task();
	vk::Device device = GraphicEngine::get_device();
	if( device )
	{
		ResultHandler r_handler( vk::Result::eSuccess );
		r_handler << device.waitIdle();
		if( r_handler.all_okay() )
		{
			device.destroyPipeline( geomtry_pipeline );
			device.destroyPipelineLayout( geomtry_pipeline_layout );
		}
	}
}

bool noxcain::GeometryTask::buffer_independent_preparation()
{
	TimeFrame frame( time_col, 0.8F, 0.0F, 0.2F, 1.0F, "start preps" );
	
	if( !geomtry_pipeline_layout )
	{
		if( !setup_layout() ) return false;
	}

	auto resolution = LogicEngine::get_graphic_settings().get_accumulated_resolution();
	vk::Extent2D current_extent = vk::Extent2D( resolution.width, resolution.height );
	if( is_new_render_pass || old_extent != current_extent )
	{
		if( !build_geomtry_pipeline() ) return false;
		old_extent = current_extent;
		is_new_render_pass = false;
	};

	return  true;
}

bool noxcain::GeometryTask::buffer_dependent_preparation( CommandData& pool_data )
{
	TimeFrame frame( time_col, 0.6F, 0.0F, 0.4F, 1.0F, "buffer preps" );
	return buffer_preparation( pool_data, 1 );
}

bool noxcain::GeometryTask::record( const std::vector<vk::CommandBuffer>& buffers )
{
	TimeFrame frame( time_col, 0.4F, 0.0F, 0.6F, 1.0F, "record" );
	ResultHandler r_handler( vk::Result::eSuccess );
	const auto& geometry_lists = LogicEngine::get_geometry_objects();
	auto& c_buffer = buffers.front();
	auto& resources = ResourceEngine::get_engine();

	const vk::CommandBufferInheritanceInfo inharitage( render_pass, subpass_index, frame_buffers.empty() ? vk::Framebuffer() : frame_buffers.front() );
	r_handler << c_buffer.begin( vk::CommandBufferBeginInfo( vk::CommandBufferUsageFlagBits::eRenderPassContinue | vk::CommandBufferUsageFlagBits::eOneTimeSubmit, &inharitage ) );

	vk::QueryPool timestamp_pool = GraphicEngine::get_render_query().get_timestamp_pool();
	if( timestamp_pool )
	{
		c_buffer.writeTimestamp( vk::PipelineStageFlagBits::eBottomOfPipe, timestamp_pool, (UINT32)RenderQuery::TimeStampIds::START );
	}

	if( !geometry_lists.empty() )
	{
		c_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, geomtry_pipeline );

		std::size_t current_geomtry_id = resources.get_invalid_geomtry_id();
		std::size_t index_count = 0;

		const auto& cam = LogicEngine::get_camera_matrix().gpuData();
		c_buffer.pushConstants( geomtry_pipeline_layout, vk::ShaderStageFlagBits::eVertex, GeometryObject::VERTEX_PUSH_OFFSET+GeometryObject::VERTEX_PUSH_SIZE, cam.size(), cam.data() );

		for( const Renderable<GeometryObject>::List& geometries : geometry_lists )
		{
			for( const GeometryObject& geometry : geometries )
			{
				const std::size_t new_geomtry_id = geometry.get_geometry_id();

				if( new_geomtry_id != current_geomtry_id )
				{
					current_geomtry_id = new_geomtry_id;
					const auto& geometry_resource = resources.get_geometry( current_geomtry_id );
					const auto& vertex_block = GraphicEngine::get_memory_manager().get_block( geometry_resource.get_vertex_buffer_id() );
					const auto& index_block = GraphicEngine::get_memory_manager().get_block( geometry_resource.get_index_buffer_id() );

					index_count = index_block.size / sizeof( UINT32 );

					c_buffer.bindVertexBuffers( 0, { vertex_block.buffer }, { vertex_block.offset } );
					c_buffer.bindIndexBuffer( index_block.buffer, index_block.offset, vk::IndexType::eUint32 );
				}

				geometry.record( c_buffer, geomtry_pipeline_layout, index_count );
			}
		}
	}

	if( timestamp_pool )
	{
		c_buffer.writeTimestamp( vk::PipelineStageFlagBits::eBottomOfPipe, timestamp_pool, (UINT32)RenderQuery::TimeStampIds::AFTER_GEOMETRY );
	}

	r_handler << c_buffer.end();
	return r_handler.all_okay();
}