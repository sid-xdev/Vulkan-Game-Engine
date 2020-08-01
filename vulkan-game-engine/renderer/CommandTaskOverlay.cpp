#include "CommandTasks.hpp"

#include <renderer/CommandThreadTools.hpp>
#include <renderer/DescriptorSetManager.hpp>
#include <renderer/GameGraphicEngine.hpp>
#include <renderer/MemoryManagement.hpp>
#include <renderer/RenderQuery.hpp>

#include <logic/GameLogicEngine.hpp>
#include <logic/Quad2D.hpp>
#include <logic/VectorText2D.hpp>

#include <resources/GameResourceEngine.hpp>
#include <resources/FontResource.hpp>

#include <tools/ResultHandler.hpp>
#include <tools/TimeFrame.hpp>


noxcain::OverlayTask::OverlayTask()
{
}

bool noxcain::OverlayTask::buffer_independent_preparation()
{
	if( !label_pipeline_layout )
	{
		if( !setup_layouts() ) return false;
	}

	auto current_extent = GraphicEngine::get_window_resolution();
	if( is_new_render_pass || old_extent != current_extent )
	{
		if( !build_text_pipeline() || !build_label_pipeline() || !build_post_pipeline() ) return false;
		old_extent = current_extent;
		is_new_render_pass = false;
	};

	return true;
}

bool noxcain::OverlayTask::buffer_dependent_preparation( CommandData& pool_data )
{
	ResultHandler<vk::Result> r_handle( vk::Result::eSuccess );

	UINT32 image_count = GraphicEngine::get_swapchain_image_count();
	return buffer_preparation( pool_data, 2 * std::size_t( image_count ) );
}

bool noxcain::OverlayTask::setup_layouts()
{
	const vk::Device& device = GraphicEngine::get_device();
	ResultHandler r_handler( vk::Result::eSuccess );

	// label pipeline layout

	std::array<vk::PushConstantRange, 2> label_push_constants =
	{
		vk::PushConstantRange( vk::ShaderStageFlagBits::eFragment, RenderableQuad2D::FRAGMENT_PUSH_OFFSET, RenderableQuad2D::FRAGMENT_PUSH_SIZE ),
		vk::PushConstantRange( vk::ShaderStageFlagBits::eVertex, RenderableQuad2D::VERTEX_PUSH_OFFSET, RenderableQuad2D::VERTEX_PUSH_SIZE ),
	};

	label_pipeline_layout = r_handler << device.createPipelineLayout( vk::PipelineLayoutCreateInfo( vk::PipelineLayoutCreateFlags(), 0, nullptr, label_push_constants.size(), label_push_constants.data() ) );

	// text pipeline layout

	std::array<vk::DescriptorSetLayout, 1> text_descriptor_sets =
	{
		GraphicEngine::get_descriptor_set_manager().get_layout( DescriptorSetLayouts::GLYPH )
	};

	std::array<vk::PushConstantRange, 2> text_push_constants =
	{
		vk::PushConstantRange( vk::ShaderStageFlagBits::eFragment, 0, 32 ),
		vk::PushConstantRange( vk::ShaderStageFlagBits::eVertex, 32, 64 )
	};

	text_pipeline_layout = r_handler << device.createPipelineLayout( vk::PipelineLayoutCreateInfo( vk::PipelineLayoutCreateFlags(), text_descriptor_sets.size(), text_descriptor_sets.data(), text_push_constants.size(), text_push_constants.data() ) );

	// post pipeline layout

	std::array<vk::DescriptorSetLayout, 1> post_descriptor_sets =
	{
		GraphicEngine::get_descriptor_set_manager().get_layout( DescriptorSetLayouts::FIXED_SAMPLED_TEXTURE_1 )
	};

	post_pipeline_layout = r_handler << device.createPipelineLayout( vk::PipelineLayoutCreateInfo( vk::PipelineLayoutCreateFlags(), post_descriptor_sets.size(), post_descriptor_sets.data(), 0, nullptr ) );

	return r_handler.all_okay();
}

bool noxcain::OverlayTask::build_post_pipeline()
{
	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages =
	{
		vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, GraphicEngine::get_shader( VertexShaderIds::FULL_SCREEN ), "main", nullptr ),
		vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, GraphicEngine::get_shader( FragmentShaderIds::FINALIZE ), "main", nullptr )
	};

	const vk::Extent2D& extent = GraphicEngine::get_window_resolution();
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
		vk::PipelineMultisampleStateCreateFlags(), vk::SampleCountFlagBits::e1, VK_FALSE, 0.0F, nullptr, VK_FALSE );

	vk::PipelineDepthStencilStateCreateInfo depthStencilState(
		vk::PipelineDepthStencilStateCreateFlags(), VK_FALSE, VK_FALSE, vk::CompareOp::eNever, VK_FALSE, VK_FALSE,
		vk::StencilOpState(), vk::StencilOpState(), 0.0F, 0.0F );

	std::array<vk::PipelineColorBlendAttachmentState, 1> attachmentState =
	{
		vk::PipelineColorBlendAttachmentState( VK_FALSE, vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB )
	};

	vk::PipelineColorBlendStateCreateInfo colorBlendState(
		vk::PipelineColorBlendStateCreateFlags(), VK_FALSE, vk::LogicOp::eClear, attachmentState.size(), attachmentState.data(), { 0.0F, 0.0F, 0.0F, 0.0F } );

	vk::PipelineVertexInputStateCreateInfo vertexState(
		vk::PipelineVertexInputStateCreateFlags(), 0, nullptr, 0, nullptr );

	vk::PipelineInputAssemblyStateCreateInfo assemblyState( vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleStrip, VK_FALSE );

	ResultHandler<vk::Result> r_handler( vk::Result::eSuccess );
	const vk::Device& device = GraphicEngine::get_device();
	if( !device )
	{
		//TODO log message
		return false;
	}

	if( post_pipeline )
	{
		r_handler << device.waitIdle();
		if( r_handler.all_okay() )
		{
			device.destroyPipeline( post_pipeline );
		}
		else
		{
			return false;
		}
	}

	post_pipeline = r_handler << device.createGraphicsPipeline( vk::PipelineCache(), vk::GraphicsPipelineCreateInfo(
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
		nullptr,
		post_pipeline_layout,
		render_pass, subpass_index, vk::Pipeline(), -1 ) );
	return r_handler.all_okay();
}

bool noxcain::OverlayTask::build_label_pipeline()
{
	ResultHandler resultHandler( vk::Result::eSuccess );
	const vk::Extent2D& extent = GraphicEngine::get_window_resolution();

	auto special = createSpecialization( FLOAT32( 2.0F / extent.width ), FLOAT32( -2.0F / extent.height ) );
	vk::SpecializationInfo specializationInfo( special.descriptions.size(), special.descriptions.data(), special.data.size(), special.data.data() );

	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages =
	{
		vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, GraphicEngine::get_shader( VertexShaderIds::LABEL ), "main", &specializationInfo ),
		vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, GraphicEngine::get_shader( FragmentShaderIds::LABEL ), "main", &specializationInfo )
	};

	std::array<vk::Viewport, 1> viewports =
	{
		vk::Viewport( 0.0F, 0.0F, static_cast<float>( extent.width ), static_cast<float>( extent.height ), 0.0F, 1.0F )
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
		VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise,
		VK_FALSE, 0.0F, 0.0F, 0.0F, 1.0F );

	vk::PipelineMultisampleStateCreateInfo multisampleState(
		vk::PipelineMultisampleStateCreateFlags(), vk::SampleCountFlagBits::e1, VK_FALSE, 0.0F, nullptr, VK_FALSE );

	vk::PipelineDepthStencilStateCreateInfo depthStencilState(
		vk::PipelineDepthStencilStateCreateFlags(), VK_FALSE, VK_FALSE, vk::CompareOp::eNever, VK_FALSE, VK_FALSE,
		vk::StencilOpState(), vk::StencilOpState(), 0.0F, 0.0F );

	std::array<vk::PipelineColorBlendAttachmentState, 1> attachmentState =
	{
		vk::PipelineColorBlendAttachmentState( VK_TRUE, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOneMinusDstAlpha, vk::BlendFactor::eOne, vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB ),
	};

	vk::PipelineColorBlendStateCreateInfo colorBlendState(
		vk::PipelineColorBlendStateCreateFlags(), VK_FALSE, vk::LogicOp::eClear, attachmentState.size(), attachmentState.data(), { 0.0F, 0.0F, 0.0F, 0.0F } );

	std::array<vk::VertexInputBindingDescription, 0> inputBindings =
	{
	};

	std::array<vk::VertexInputAttributeDescription, 0> inputAttributeDescription =
	{
	};

	vk::PipelineVertexInputStateCreateInfo vertexState(
		vk::PipelineVertexInputStateCreateFlags(), inputBindings.size(), inputBindings.data(), inputAttributeDescription.size(), inputAttributeDescription.data() );

	vk::PipelineInputAssemblyStateCreateInfo assemblyState( vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleStrip, VK_FALSE );

	std::array<vk::DynamicState, 1> dynamicStates =
	{
		vk::DynamicState::eScissor
	};
	vk::PipelineDynamicStateCreateInfo dynamicStateInfo( vk::PipelineDynamicStateCreateFlags(), dynamicStates.size(), dynamicStates.data() );

	ResultHandler<vk::Result> r_handler( vk::Result::eSuccess );
	const vk::Device& device = GraphicEngine::get_device();
	if( !device )
	{
		//TODO log message
		return false;
	}

	if( label_pipeline )
	{
		r_handler << device.waitIdle();
		if( r_handler.all_okay() )
		{
			device.destroyPipeline( label_pipeline );
		}
		else
		{
			return false;
		}
	}

	label_pipeline = resultHandler << device.createGraphicsPipeline( vk::PipelineCache(), vk::GraphicsPipelineCreateInfo(
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
		&dynamicStateInfo,
		label_pipeline_layout,
		render_pass, subpass_index + 1, vk::Pipeline(), -1 ) );
	return r_handler.all_okay();
}

bool noxcain::OverlayTask::record( const std::vector<vk::CommandBuffer>& buffers )
{
	TimeFrame frame( time_col, 0.4F, 0.0F, 0.6F, 1.0F, "record" );
	const auto& resources = ResourceEngine::get_engine();

	for( std::size_t index = 0; index < buffers.size() / 2; ++index )
	{
		vk::CommandBufferInheritanceInfo inhertiance( render_pass, subpass_index, frame_buffers[index] );
		vk::QueryPool timestamp_pool = GraphicEngine::get_render_query().get_timestamp_pool();

		// post subpass
		const auto post_buffer = buffers[2 * index];
		post_buffer.begin( vk::CommandBufferBeginInfo( vk::CommandBufferUsageFlagBits::eRenderPassContinue | vk::CommandBufferUsageFlagBits::eOneTimeSubmit, &inhertiance ) );
		
		if( timestamp_pool )
		{
			post_buffer.writeTimestamp( vk::PipelineStageFlagBits::eTopOfPipe, timestamp_pool, (UINT32)RenderQuery::TimeStampIds::BEFOR_POST );
		}
		
		post_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, post_pipeline );
		post_buffer.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, post_pipeline_layout, 0, { GraphicEngine::get_descriptor_set_manager().get_basic_set( BasicDescriptorSets::FINALIZED_MASTER_TEXTURE ) }, {} );
		post_buffer.draw( 3, 1, 0, 0 );
		
		if( timestamp_pool )
		{
			post_buffer.writeTimestamp( vk::PipelineStageFlagBits::eBottomOfPipe, timestamp_pool, (UINT32)RenderQuery::TimeStampIds::AFTER_POST );
		}
		
		post_buffer.end();

		// overlay subpass
		const auto overlay_buffer = buffers[2 * index + 1];
		inhertiance.setSubpass( inhertiance.subpass + 1 );
		overlay_buffer.begin( vk::CommandBufferBeginInfo( vk::CommandBufferUsageFlagBits::eRenderPassContinue | vk::CommandBufferUsageFlagBits::eOneTimeSubmit, &inhertiance ) );

		if( timestamp_pool )
		{
			overlay_buffer.writeTimestamp( vk::PipelineStageFlagBits::eTopOfPipe, timestamp_pool, (UINT32)RenderQuery::TimeStampIds::BEFOR_OVERLAY );
		}


		vk::Rect2D default_scissor( vk::Offset2D( 0, 0 ), GraphicEngine::get_window_resolution() );
		vk::Rect2D current_scissor;

		const auto& uis = LogicEngine::get_user_interfaces();



		for( const GameUserInterface& ui : uis )
		{
			auto label_iter = ui.labels.begin();
			auto text_iter = ui.texts.begin();
			UINT32 order_index = 0;
			while( order_index < ui.depth_order.size() )
			{
				const UINT32 label_count = ui.depth_order[order_index][0];
				if( label_count )
				{
					overlay_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, label_pipeline );
				}
				for( UINT32 index = 0; index < label_count; ++index, ++label_iter )
				{
					current_scissor = label_iter->get().record( overlay_buffer, label_pipeline_layout, current_scissor, default_scissor );
				}

				const UINT32 text_count = ui.depth_order[order_index][1];
				if( text_count )
				{
					const auto& vertex_block_info = GraphicEngine::get_memory_manager().get_block( ResourceEngine::get_engine().get_font( 0 ).get_vertex_block_id() );
					overlay_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, text_pipeline );
					overlay_buffer.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, text_pipeline_layout, 0, { GraphicEngine::get_descriptor_set_manager().get_basic_set( BasicDescriptorSets::GLYPHS ) }, {} );
					overlay_buffer.bindVertexBuffers( 0, { vertex_block_info.buffer }, { vertex_block_info.offset } );
				}
				for( UINT32 index = 0; index < text_count; ++index, ++text_iter )
				{
					current_scissor = text_iter->get().record( overlay_buffer, text_pipeline_layout, current_scissor, default_scissor );
				}
				++order_index;
			}
		}

		if( timestamp_pool )
		{
			overlay_buffer.writeTimestamp( vk::PipelineStageFlagBits::eBottomOfPipe, timestamp_pool, (UINT32)RenderQuery::TimeStampIds::AFTER_OVERLAY );
		}
		overlay_buffer.end();
	}
	return true;
}

bool noxcain::OverlayTask::build_text_pipeline()
{
	ResultHandler<vk::Result> r_handler( vk::Result::eSuccess );
	const vk::Extent2D& extent = GraphicEngine::get_window_resolution();

	auto special = createSpecialization( FLOAT32( 2.0F / extent.width ), FLOAT32( -2.0F / extent.height ), UINT32( ResourceEngine::get_engine().get_resource_limits().glyph_count ) );
	vk::SpecializationInfo specializationInfo( special.descriptions.size(), special.descriptions.data(), special.data.size(), special.data.data() );

	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages =
	{
		vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, GraphicEngine::get_shader( VertexShaderIds::GLYPH_QUAD_2D ), "main", &specializationInfo ),
		vk::PipelineShaderStageCreateInfo( vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, GraphicEngine::get_shader( FragmentShaderIds::GLYPH_CONTOUR_2D ), "main", &specializationInfo )
	};


	std::array<vk::Viewport, 1> viewports =
	{
		vk::Viewport( 0.0F, 0.0F, static_cast<float>( extent.width ), static_cast<float>( extent.height ), 0.0F, 1.0F )
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
		VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise,
		VK_FALSE, 0.0F, 0.0F, 0.0F, 1.0F );

	vk::PipelineMultisampleStateCreateInfo multisampleState(
		vk::PipelineMultisampleStateCreateFlags(), vk::SampleCountFlagBits::e1, VK_FALSE, 0.0F, nullptr, VK_FALSE );

	vk::PipelineDepthStencilStateCreateInfo depthStencilState(
		vk::PipelineDepthStencilStateCreateFlags(), VK_FALSE, VK_FALSE, vk::CompareOp::eNever, VK_FALSE, VK_FALSE,
		vk::StencilOpState(), vk::StencilOpState(), 0.0F, 0.0F );

	std::array<vk::PipelineColorBlendAttachmentState, 1> attachmentState =
	{
		vk::PipelineColorBlendAttachmentState( VK_TRUE, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOneMinusDstAlpha, vk::BlendFactor::eOne, vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB ),
	};

	vk::PipelineColorBlendStateCreateInfo colorBlendState(
		vk::PipelineColorBlendStateCreateFlags(), VK_FALSE, vk::LogicOp::eClear, attachmentState.size(), attachmentState.data(), { 0.0F, 0.0F, 0.0F, 0.0F } );

	std::array<vk::VertexInputBindingDescription, 1> inputBindings =
	{
		vk::VertexInputBindingDescription( 0, 2 * sizeof( FLOAT32 ), vk::VertexInputRate::eVertex )
	};

	std::array<vk::VertexInputAttributeDescription, 1> inputAttributeDescription =
	{
		vk::VertexInputAttributeDescription( 0, 0, vk::Format::eR32G32Sfloat, 0 )
	};

	vk::PipelineVertexInputStateCreateInfo vertexState(
		vk::PipelineVertexInputStateCreateFlags(), inputBindings.size(), inputBindings.data(), inputAttributeDescription.size(), inputAttributeDescription.data() );

	vk::PipelineInputAssemblyStateCreateInfo assemblyState( vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleStrip, VK_FALSE );

	std::array<vk::DynamicState, 1> dynamicStates =
	{
		vk::DynamicState::eScissor
	};
	vk::PipelineDynamicStateCreateInfo dynamicStateInfo( vk::PipelineDynamicStateCreateFlags(), dynamicStates.size(), dynamicStates.data() );

	const vk::Device& device = GraphicEngine::get_device();
	if( !device )
	{
		//TODO log message
		return false;
	}

	if( text_pipeline )
	{
		r_handler << device.waitIdle();
		if( r_handler.all_okay() )
		{
			device.destroyPipeline( text_pipeline );
		}
		else
		{
			return false;
		}
	}

	text_pipeline = r_handler << device.createGraphicsPipeline( vk::PipelineCache(), vk::GraphicsPipelineCreateInfo(
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
		&dynamicStateInfo,
		text_pipeline_layout,
		render_pass, subpass_index + 1, vk::Pipeline(), -1 ) );

	return r_handler.all_okay();
}

noxcain::OverlayTask::~OverlayTask()
{
	shutdown_task();
	vk::Device device = GraphicEngine::get_device();
	if( device )
	{
		ResultHandler r_handler( vk::Result::eSuccess );
		r_handler << device.waitIdle();
		if( r_handler.all_okay() )
		{
			device.destroyPipeline( post_pipeline );
			device.destroyPipeline( label_pipeline );
			device.destroyPipeline( text_pipeline );
			
			device.destroyPipelineLayout( post_pipeline_layout );
			device.destroyPipelineLayout( label_pipeline_layout );
			device.destroyPipelineLayout( text_pipeline_layout );
		}
	}
}