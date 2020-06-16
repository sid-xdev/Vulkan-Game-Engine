#include "CommandTasks.hpp"

#include <renderer/DescriptorSetManager.hpp>
#include <renderer/GameGraphicEngine.hpp>
#include <logic/GameLogicEngine.hpp>
#include <tools/ResultHandler.hpp>

noxcain::SamplingTask::SamplingTask()
{
}

noxcain::SamplingTask::~SamplingTask()
{
	shutdown_task();
	vk::Device device = GraphicEngine::get_device();
	if( device )
	{
		ResultHandler r_handler( vk::Result::eSuccess );
		r_handler << device.waitIdle();
		if( r_handler.all_okay() )
		{
			device.destroyPipeline( sampled_pipeline );
			device.destroyPipeline( unsampled_pipeline );
			device.destroyPipelineLayout( sampling_pipeline_layout );
		}
	}
}

bool noxcain::SamplingTask::buffer_independent_preparation()
{
	if( !sampling_pipeline_layout )
	{
		if( !setup_layouts() ) return false;
	}

	auto resolution = LogicEngine::get_graphic_settings().get_accumulated_resolution();
	vk::Extent2D current_extent( resolution.width, resolution.height );
	if( is_new_render_pass || old_extent != current_extent )
	{
		if( !build_shading_pipelines() || !build_edge_detection_pipeline() ) return false;
		old_extent = current_extent;
		is_new_render_pass = false;
	};

	return true;
}

bool noxcain::SamplingTask::buffer_dependent_preparation( CommandData& pool_data )
{
	ResultHandler r_handle( vk::Result::eSuccess );
	return buffer_preparation( pool_data, 2 );
}

bool noxcain::SamplingTask::record( const std::vector<vk::CommandBuffer>& buffers )
{
	bool multi_sampling = LogicEngine::get_graphic_settings().current_sample_count > 1;
	TimeFrame frame( time_col, 0.4F, 0.0F, 0.6F, 1.0F, "record" );
	ResultHandler r_handler( vk::Result::eSuccess );
	vk::CommandBufferInheritanceInfo inheritage( render_pass, subpass_index, frame_buffers.empty() ? vk::Framebuffer() : frame_buffers.front() );
	
	//EDGE DETECTION
	if( multi_sampling )
	{
		const auto edge_detection_buffer = buffers.front();
		r_handler << edge_detection_buffer.begin( vk::CommandBufferBeginInfo( vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eRenderPassContinue, &inheritage ) );
		edge_detection_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, edge_detection_pipeline );
		edge_detection_buffer.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, sampling_pipeline_layout, 0, { GraphicEngine::get_descriptor_set_manager().get_basic_set( BasicDescriptorSets::SHADING_INPUT_ATTACHMENTS ) }, {} );
		edge_detection_buffer.draw( 3, 1, 0, 0 );
		r_handler << edge_detection_buffer.end();
	}

	//SHADING
	const auto shading_buffer = buffers.back();
	if( multi_sampling )
	{
		inheritage.setSubpass( inheritage.subpass + 1 );
	}
	r_handler << shading_buffer.begin( vk::CommandBufferBeginInfo( vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eRenderPassContinue, &inheritage ) );
	
	shading_buffer.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, sampling_pipeline_layout, 0, { GraphicEngine::get_descriptor_set_manager().get_basic_set( BasicDescriptorSets::SHADING_INPUT_ATTACHMENTS ) }, {} );
	shading_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, unsampled_pipeline );
	shading_buffer.draw( 3, 1, 0, 0 );
	
	if( multi_sampling )
	{
		shading_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, sampled_pipeline );
		shading_buffer.draw( 3, 1, 0, 0 );
	}
	r_handler << shading_buffer.end();

	return r_handler.all_okay();
}

bool noxcain::SamplingTask::setup_layouts()
{
	auto device = GraphicEngine::get_device();
	if( device )
	{
		ResultHandler r_handler( vk::Result::eSuccess );
		std::array<vk::DescriptorSetLayout, 1> descriptor_sets =
		{
			GraphicEngine::get_descriptor_set_manager().get_layout( DescriptorSetLayouts::INPUT_ATTACHMENT_3 )
		};

		sampling_pipeline_layout = r_handler << device.createPipelineLayout( vk::PipelineLayoutCreateInfo( vk::PipelineLayoutCreateFlags(), descriptor_sets.size(), descriptor_sets.data(), 0, nullptr ) );

		return r_handler.all_okay();
	}
	return false;
}

inline bool noxcain::SamplingTask::build_edge_detection_pipeline()
{
	const auto g_settings = LogicEngine::get_graphic_settings();
	if( g_settings.current_sample_count > 1 )
	{
		auto specialization = createSpecialization( g_settings.current_sample_count );
		vk::SpecializationInfo specialization_info( specialization.descriptions.size(), specialization.descriptions.data(), specialization.data.size(), specialization.data.data() );

		auto resolution = LogicEngine::get_graphic_settings().get_accumulated_resolution();

		//SHADER STAGES
		std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages =
		{
			vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, GraphicEngine::get_shader( VertexShaderIds::FULL_SCREEN ), "main", nullptr ),
			vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, GraphicEngine::get_shader( FragmentShaderIds::EDGE_DETECTION ), "main", &specialization_info )
		};

		//VIEWPORTS AND SCISSORS
		std::array<vk::Viewport, 1> viewports =
		{
			vk::Viewport( 0.0F, 0.0F, float( resolution.width ), float( resolution.height ), 0.0F, 1.0F )
		};

		std::array<vk::Rect2D, viewports.size()> scissors =
		{
			vk::Rect2D( vk::Offset2D( 0, 0 ), vk::Extent2D( resolution.width, resolution.height ) )
		};

		vk::PipelineViewportStateCreateInfo viewport_sate(
			vk::PipelineViewportStateCreateFlags(),
			viewports.size(), viewports.data(),
			scissors.size(), scissors.data() );

		//RASTERIZATION
		vk::PipelineRasterizationStateCreateInfo rasterization_state(
			vk::PipelineRasterizationStateCreateFlags(),
			VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise,
			VK_FALSE, 0.0F, 0.0F, 0.0F, 1.0F );

		//MULTISAMPLING
		vk::PipelineMultisampleStateCreateInfo multisample_state(
			vk::PipelineMultisampleStateCreateFlags(), vk::SampleCountFlagBits::e1, VK_FALSE, 0.0F, nullptr, VK_FALSE );

		//DEPTH STENCIL
		vk::StencilOpState stencilOp( vk::StencilOp::eZero, vk::StencilOp::eZero, vk::StencilOp::eZero, vk::CompareOp::eAlways, 0, 1, 0 );

		vk::PipelineDepthStencilStateCreateInfo depth_stencil_state(
			vk::PipelineDepthStencilStateCreateFlags(), VK_FALSE, VK_FALSE, vk::CompareOp::eNever, VK_FALSE, VK_TRUE,
			stencilOp, vk::StencilOpState(), 0.0F, 0.0F );

		//COLOR BLENDING
		vk::PipelineColorBlendStateCreateInfo color_blend_state(
			vk::PipelineColorBlendStateCreateFlags(), VK_FALSE, vk::LogicOp::eClear, 0, nullptr, { 0.0F, 0.0F, 0.0F, 0.0F } );

		//VERTEX ASSEMBLY
		vk::PipelineVertexInputStateCreateInfo vertex_state(
			vk::PipelineVertexInputStateCreateFlags(), 0, nullptr, 0, nullptr );

		vk::PipelineInputAssemblyStateCreateInfo assembly_state( vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleStrip, VK_FALSE );

		//CREATION
		ResultHandler r_handler( vk::Result::eSuccess );
		const vk::Device& device = GraphicEngine::get_device();
		if( !device )
		{
			//TODO log message
			return false;
		}

		if( edge_detection_pipeline )
		{
			r_handler << device.waitIdle();
			if( r_handler.all_okay() )
			{
				device.destroyPipeline( edge_detection_pipeline );
			}
			else
			{
				return false;
			}
		}

		edge_detection_pipeline = r_handler << device.createGraphicsPipeline( vk::PipelineCache(), vk::GraphicsPipelineCreateInfo(
			vk::PipelineCreateFlags(),
			shader_stages.size(), shader_stages.data(),
			&vertex_state,
			&assembly_state,
			nullptr,
			&viewport_sate,
			&rasterization_state,
			&multisample_state,
			&depth_stencil_state,
			&color_blend_state,
			nullptr,
			sampling_pipeline_layout,
			render_pass, subpass_index, vk::Pipeline(), -1 ) );
		return r_handler.all_okay();
	}
	return true;
}

inline bool noxcain::SamplingTask::build_shading_pipelines()
{
	const auto g_settings = LogicEngine::get_graphic_settings();
	bool multi_sampling = g_settings.current_sample_count > 1;
	auto specialization = createSpecialization( g_settings.current_sample_count );
	vk::SpecializationInfo specialization_info( specialization.descriptions.size(), specialization.descriptions.data(), specialization.data.size(), specialization.data.data() );

	std::array<vk::PipelineShaderStageCreateInfo, 2> multisample_shader_stages =
	{
		vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, GraphicEngine::get_shader(VertexShaderIds::FULL_SCREEN ), "main", nullptr ),
		vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, GraphicEngine::get_shader( FragmentShaderIds::MULTI_SHADING ), "main", &specialization_info )
	};

	std::array<vk::PipelineShaderStageCreateInfo, 2> singlesample_shader_stages =
	{
		vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, GraphicEngine::get_shader( VertexShaderIds::FULL_SCREEN ), "main", nullptr ),
		multi_sampling ?
		vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, GraphicEngine::get_shader( FragmentShaderIds::MULTI_SHADING ), "main", nullptr ) :
		vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, GraphicEngine::get_shader( FragmentShaderIds::SINGLE_SHADING ), "main", nullptr )
	};

	//VIEWPORTS AND SCISSORS
	const auto resolution = g_settings.get_accumulated_resolution();
	std::array<vk::Viewport, 1> viewports =
	{
		vk::Viewport( 0.0F, 0.0F, float( resolution.width ), float( resolution.height ), 0.0F, 1.0F )
	};

	std::array<vk::Rect2D, viewports.size()> scissors =
	{
		vk::Rect2D( vk::Offset2D( 0, 0 ), vk::Extent2D( resolution.width, resolution.height ) )
	};

	vk::PipelineViewportStateCreateInfo viewport_sate(
		vk::PipelineViewportStateCreateFlags(),
		viewports.size(), viewports.data(),
		scissors.size(), scissors.data() );

	//RASTERIZATION
	vk::PipelineRasterizationStateCreateInfo rasterization_state(
		vk::PipelineRasterizationStateCreateFlags(),
		VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise,
		VK_FALSE, 0.0F, 0.0F, 0.0F, 1.0F );

	//MULTISAMPLING
	vk::PipelineMultisampleStateCreateInfo multisample_state(
		vk::PipelineMultisampleStateCreateFlags(), vk::SampleCountFlagBits::e1, VK_FALSE, 0.0F, nullptr, VK_FALSE );

	//DEPTH STENCIL
	vk::StencilOpState stencil_off( vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eEqual, 1, 0, 0 );
	vk::StencilOpState stencil_on( vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eEqual, 1, 0, 1 );

	vk::PipelineDepthStencilStateCreateInfo multisample_depth_stencil_state(
		vk::PipelineDepthStencilStateCreateFlags(), VK_FALSE, VK_FALSE, vk::CompareOp::eNever, VK_FALSE, VK_TRUE,
		stencil_on, vk::StencilOpState(), 0.0F, 0.0F );

	vk::PipelineDepthStencilStateCreateInfo singlesample_depth_stencil_state(
		vk::PipelineDepthStencilStateCreateFlags(), VK_FALSE, VK_FALSE, vk::CompareOp::eNever, VK_FALSE, VK_TRUE,
		stencil_off, vk::StencilOpState(), 0.0F, 0.0F );

	//COLOR BLENDING
	std::array<vk::PipelineColorBlendAttachmentState, 1> attachment_state =
	{
		vk::PipelineColorBlendAttachmentState( VK_FALSE, vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB )
	};

	vk::PipelineColorBlendStateCreateInfo color_blend_state(
		vk::PipelineColorBlendStateCreateFlags(), VK_FALSE, vk::LogicOp::eClear, attachment_state.size(), attachment_state.data(), { 0.0F, 0.0F, 0.0F, 0.0F } );

	//VERTEX ASSEMBLY
	vk::PipelineVertexInputStateCreateInfo vertex_state(
		vk::PipelineVertexInputStateCreateFlags(), 0, nullptr, 0, nullptr );

	vk::PipelineInputAssemblyStateCreateInfo assembly_state( vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleStrip, VK_FALSE );

	//CREATION
	ResultHandler r_handler( vk::Result::eSuccess );
	const vk::Device& device = GraphicEngine::get_device();
	if( !device )
	{
		//TODO log message
		return false;
	}

	if( sampled_pipeline || unsampled_pipeline )
	{
		r_handler << device.waitIdle();
		if( r_handler.all_okay() )
		{
			device.destroyPipeline( sampled_pipeline );
			device.destroyPipeline( unsampled_pipeline );
		}
		else
		{
			return false;
		}
	}

	if( multi_sampling )
	{
		sampled_pipeline = r_handler << device.createGraphicsPipeline( vk::PipelineCache(), vk::GraphicsPipelineCreateInfo(
			vk::PipelineCreateFlags(),
			multisample_shader_stages.size(), multisample_shader_stages.data(),
			&vertex_state,
			&assembly_state,
			nullptr,
			&viewport_sate,
			&rasterization_state,
			&multisample_state,
			&multisample_depth_stencil_state,
			&color_blend_state,
			nullptr,
			sampling_pipeline_layout,
			render_pass, subpass_index + 1, vk::Pipeline(), -1 ) );
	}

	unsampled_pipeline = r_handler << device.createGraphicsPipeline( vk::PipelineCache(), vk::GraphicsPipelineCreateInfo(
		vk::PipelineCreateFlags(),
		singlesample_shader_stages.size(), singlesample_shader_stages.data(),
		&vertex_state,
		&assembly_state,
		nullptr,
		&viewport_sate,
		&rasterization_state,
		&multisample_state,
		&singlesample_depth_stencil_state,
		&color_blend_state,
		nullptr,
		sampling_pipeline_layout,
		render_pass, subpass_index + ( multi_sampling ? 1 : 0 ), vk::Pipeline(), -1 ) );

	return r_handler.all_okay();
}
