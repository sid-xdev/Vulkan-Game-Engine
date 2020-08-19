#include "CommandManager.hpp"

#include <renderer/CommandSubmit.hpp>
#include <renderer/CommandTasks.hpp>
#include <renderer/GameGraphicEngine.hpp>
#include <renderer/GraphicEngineConstants.hpp>
#include <renderer/MemoryManagement.hpp>
#include <renderer/RenderQuery.hpp>

#include <logic/GameLogicEngine.hpp>

#include <resources/GameResourceEngine.hpp>


#include <tools/ResultHandler.hpp>
#include <tools/TimeFrame.hpp>

#include <vector>
#include <thread>
#include <chrono>

noxcain::CommandManager::CommandManager()
{
}

void noxcain::CommandManager::start_loop()
{
	comman_main_thread = std::thread( &CommandManager::run_render_loop, this );
}

void noxcain::CommandManager::run_render_loop()
{	
	ResultHandler<bool> r_handle_bool( true );
	TimeFrameCollector record_time_frame( "Frame CPU" );

	if( !initial_transfer() )
	{
		//TODO error
		return;
	}

	describe_deferred_render_pass();
	describe_finalize_render_pass();

	GeometryTask geometry_task;
	VectorDecalTask vector_decal_task;
	SamplingTask sampling_task;

	OverlayTask overlay_task;

	CommandSubmit submit;

	while( LogicEngine::is_running() && submit.check_swapchain() )
	{
		r_handle_bool.reset();
		const auto start_time = std::chrono::steady_clock::now();
		// validate all pre record and render objects with high priority 
		// and with out dependencie to command buffers
		{
			TimeFrame time_frame( record_time_frame, 0.0, 0.8, 0.0, 1.0, "pre buffer" );
			LogicEngine::update();
			if( !validate_render_passes() )
			{
				//TODO error?
				return;
			}

			// update render pass
			auto deferred_pass = deferred_render_pass.get_render_pass();
			geometry_task.set_render_pass( deferred_pass, 0 );
			vector_decal_task.set_render_pass( deferred_pass, 1 );
			sampling_task.set_render_pass( deferred_pass, 2 );
			overlay_task.set_render_pass( finalize_render_pass.get_render_pass(), 0 );
		}

		// wait for new free command buffer id
		INT32 id = submit.get_free_buffer_id();
		if( id < 0 )
		{
			return;
		}



		// validate all command buffer dependent objects 
		record_time_frame.start_frame( 0.0, 0.6, 0.2, 1.0, "buffer" );

		geometry_task.set_buffer_id( id );
		vector_decal_task.set_buffer_id( id );
		sampling_task.set_buffer_id( id );
		overlay_task.set_buffer_id( id );

		// check for frame buffers
		if( !validate_frame_buffers() )
		{
			return;
		}

		// update frame buffer
		geometry_task.set_frame_buffers( { deferred_frame_buffer } );
		vector_decal_task.set_frame_buffers( { deferred_frame_buffer } );
		sampling_task.set_frame_buffers( { deferred_frame_buffer } );
		overlay_task.set_frame_buffers( finalize_frame_buffers );

		// check own command buffers
		if( !validate_command_buffers( id ) )
		{
			return;
		}

		// start master recording
		record_time_frame.end_is_start( 0.0, 0.4, 0.4, 1.0, "record" );

		CommandSubmit::SubmitCommandBufferData buffer_data;
		buffer_data.id = id;
		buffer_data.main_buffer = command_buffers[id].front();
		buffer_data.finalize_command_buffers = std::vector<vk::CommandBuffer>( command_buffers[id].begin() + 1, command_buffers[id].end() );

		auto g_settings = LogicEngine::get_graphic_settings();
		auto resolution = g_settings.get_accumulated_resolution();
		bool multi_sampling = g_settings.current_sample_count > 1;

		vk::QueryPool timestamp_pool = GraphicEngine::get_render_query().get_timestamp_pool();

		buffer_data.main_buffer.begin( vk::CommandBufferBeginInfo( vk::CommandBufferUsageFlagBits::eOneTimeSubmit ) );

		if( timestamp_pool )
		{
			buffer_data.main_buffer.resetQueryPool( timestamp_pool, 0, RenderQuery::TIMESTAMP_COUNT );
		}

		buffer_data.main_buffer.beginRenderPass( vk::RenderPassBeginInfo( deferred_render_pass.get_render_pass(), deferred_frame_buffer,
																		  vk::Rect2D( vk::Offset2D( 0, 0 ), vk::Extent2D( resolution.width, resolution.height ) ),
																		  UINT32( deferred_clear_colors.size() ), deferred_clear_colors.data() ), vk::SubpassContents::eSecondaryCommandBuffers );

		// geometry commands
		r_handle_bool << geometry_task.wait_for_finish();
		if( r_handle_bool.all_okay() )
		{
			const auto& geometry_subcommand_buffers = geometry_task.get_finished_buffers();
			buffer_data.main_buffer.executeCommands( geometry_subcommand_buffers );
		}

		buffer_data.main_buffer.nextSubpass( vk::SubpassContents::eSecondaryCommandBuffers );

		// vector decal commands
		r_handle_bool << vector_decal_task.wait_for_finish();
		if( r_handle_bool.all_okay() )
		{
			const auto& vector_decal_subcommand_buffers = vector_decal_task.get_finished_buffers();
			buffer_data.main_buffer.executeCommands( vector_decal_subcommand_buffers );
		}
		buffer_data.main_buffer.nextSubpass( vk::SubpassContents::eSecondaryCommandBuffers );

		// unsampled commands
		r_handle_bool << sampling_task.wait_for_finish();
		if( r_handle_bool.all_okay() )
		{
			const auto sampling_subcommand_buffers = sampling_task.get_finished_buffers();
			if( multi_sampling )
			{
				buffer_data.main_buffer.executeCommands( { sampling_subcommand_buffers.front() } );
				buffer_data.main_buffer.nextSubpass( vk::SubpassContents::eSecondaryCommandBuffers );
			}
			buffer_data.main_buffer.executeCommands( { sampling_subcommand_buffers.back() } );
		}
		else if( multi_sampling )
		{
			buffer_data.main_buffer.nextSubpass( vk::SubpassContents::eSecondaryCommandBuffers );
		}

		buffer_data.main_buffer.endRenderPass();
		buffer_data.main_buffer.end();

		vk::Rect2D surfaceRectangle( vk::Offset2D( 0, 0 ), GraphicEngine::get_window_resolution() );

		r_handle_bool << overlay_task.wait_for_finish();
		if( r_handle_bool.all_okay() )
		{
			auto& sub_command_buffers = overlay_task.get_finished_buffers();
			for( std::size_t index = 0; index < buffer_data.finalize_command_buffers.size(); ++index )
			{
				const vk::CommandBuffer cBuffer = buffer_data.finalize_command_buffers[index];
				cBuffer.begin( vk::CommandBufferBeginInfo( vk::CommandBufferUsageFlags() ) );
				cBuffer.beginRenderPass( vk::RenderPassBeginInfo(
					finalize_render_pass.get_render_pass(), finalize_frame_buffers[index], surfaceRectangle, UINT32( finalize_clear_colors.size() ), finalize_clear_colors.data() ),
					vk::SubpassContents::eSecondaryCommandBuffers );

				// post commands
				cBuffer.executeCommands( { sub_command_buffers[2 * index] } );
				cBuffer.nextSubpass( vk::SubpassContents::eSecondaryCommandBuffers );

				// overlay commands
				cBuffer.executeCommands( { sub_command_buffers[2 * index + 1] } );
				cBuffer.endRenderPass();
				
				if( timestamp_pool )
				{
					cBuffer.writeTimestamp( vk::PipelineStageFlagBits::eBottomOfPipe, timestamp_pool, (UINT32)RenderQuery::TimeStampIds::END );
				}
				
				cBuffer.end();
			}
		}

		submit.set_newest_command_buffer( buffer_data );

		record_time_frame.end_frame();

		const auto end_time = std::chrono::steady_clock::now();
		LogicEngine::set_cpu_cycle_duration( end_time - start_time );
	}
	return;
}

bool noxcain::CommandManager::validate_render_passes()
{	
	if( GraphicEngine::get_memory_manager().has_render_destination_memory() )
	{
		deferred_render_pass.update_format( default_color_format, GraphicEngine::get_memory_manager().get_image( MemoryManager::RenderDestinationImages::COLOR ).format );
		deferred_render_pass.update_format( depth_format, GraphicEngine::get_memory_manager().get_image( MemoryManager::RenderDestinationImages::DEPTH_SAMPLED ).format );
		deferred_render_pass.update_format( stencil_format, GraphicEngine::get_memory_manager().get_image( MemoryManager::RenderDestinationImages::STENCIL_UNSAMPLED ).format );
	}
	else
	{
		auto formats = GraphicEngine::get_memory_manager().select_main_render_formats();
		deferred_render_pass.update_format( default_color_format, formats.color );
		deferred_render_pass.update_format( depth_format, formats.depth );
		deferred_render_pass.update_format( stencil_format, formats.stencil );
	}

	deferred_render_pass.update_sample_count( multi_sample_count, static_cast<vk::SampleCountFlagBits>( LogicEngine::get_graphic_settings().current_sample_count ) );
	finalize_render_pass.update_format( swap_chain_image_format, GraphicEngine::get_swapchain_image_format() );

	if( deferred_render_pass.get_render_pass() && finalize_render_pass.get_render_pass() )
	{
		return true;
	}
	return false;
}

bool noxcain::CommandManager::validate_frame_buffers()
{
	ResultHandler<vk::Result> r_handle( vk::Result::eSuccess );
	vk::Device device = GraphicEngine::get_device();
	
	auto graphic_settings = LogicEngine::get_graphic_settings();
	auto current_resolution = graphic_settings.get_accumulated_resolution();

	// validate deferred_frame_buffer
	if( !deferred_frame_buffer || graphic_settings.current_sample_count != old_sample_count || current_resolution.width != old_frame_buffer_width || current_resolution.height != old_frame_buffer_height )
	{
		r_handle << device.waitIdle();
		if( r_handle.all_okay() )
		{
			device.destroyFramebuffer( deferred_frame_buffer );
			deferred_frame_buffer = vk::Framebuffer();

			if( !GraphicEngine::get_memory_manager().setup_main_render_destination() )
			{
				return false;
			}
			auto attachment_count = deferred_render_pass.get_attachment_count();

			std::vector<vk::ImageView> views;
			deferred_clear_colors.clear();
			views.reserve( attachment_count );
			deferred_clear_colors.reserve( attachment_count );

			// attachment0 color
			views.push_back( GraphicEngine::get_memory_manager().get_image( MemoryManager::RenderDestinationImages::COLOR ).view );
			deferred_clear_colors.push_back( vk::ClearColorValue( std::array<FLOAT32, 4>( { 0.0F, 0.0F, 0.0F, 0.0F } ) ) );

			// attachment1 color resolved
			views.push_back( GraphicEngine::get_memory_manager().get_image( MemoryManager::RenderDestinationImages::COLOR_RESOLVED ).view );
			deferred_clear_colors.push_back( vk::ClearColorValue( std::array<FLOAT32, 4>( { 0.0F, 0.0F, 0.0F, 0.0F } ) ) );

			// attachment2 normal
			views.push_back( GraphicEngine::get_memory_manager().get_image( MemoryManager::RenderDestinationImages::NORMAL ).view );
			deferred_clear_colors.push_back( vk::ClearColorValue( std::array<FLOAT32, 4>( { 0.0F, 0.0F, 0.0F, 0.0F } ) ) );

			// attachment3 slider_position
			views.push_back( GraphicEngine::get_memory_manager().get_image( MemoryManager::RenderDestinationImages::POSITION ).view );
			deferred_clear_colors.push_back( vk::ClearColorValue( std::array<FLOAT32, 4>( { 0.0F, 0.0F, 0.0F, 0.0F } ) ) );

			// attachment4 depth only
			views.push_back( GraphicEngine::get_memory_manager().get_image( MemoryManager::RenderDestinationImages::DEPTH_SAMPLED ).view );
			deferred_clear_colors.push_back( vk::ClearDepthStencilValue( 1.0F, 0U ) );

			if( graphic_settings.current_sample_count > 1 )
			{
				// attachment5 stencil only
				views.push_back( GraphicEngine::get_memory_manager().get_image( MemoryManager::RenderDestinationImages::STENCIL_UNSAMPLED ).view );
				deferred_clear_colors.push_back( vk::ClearDepthStencilValue( 0.0F, 1U ) );
			}

			auto resolution = LogicEngine::get_graphic_settings().get_accumulated_resolution();
			vk::Extent3D extent( resolution.width, resolution.height, 1 );

			if( views.size() != attachment_count )
			{
				// error TODO
				return false;
			}

			deferred_frame_buffer = r_handle << device.createFramebuffer( vk::FramebufferCreateInfo( vk::FramebufferCreateFlags(), deferred_render_pass.get_render_pass(), UINT32( views.size() ), views.data(), extent.width, extent.height, extent.depth ) );

			if( deferred_clear_colors.size() != attachment_count )
			{
				deferred_clear_colors.clear();
				deferred_clear_colors.resize( attachment_count, vk::ClearColorValue() );
			}
		}
		if( !r_handle.all_okay() )
		{
			return false;
		}

		old_frame_buffer_width = current_resolution.width;
		old_frame_buffer_height = current_resolution.height;
		old_sample_count = graphic_settings.current_sample_count;
	}

	// validate finailze_frame_buffer 
	auto current_swapchain = GraphicEngine::get_swapchain();
	if( !old_swapchain || old_swapchain != current_swapchain || finalize_frame_buffers.empty() )
	{
		if( !current_swapchain )
		{
			// TODO error log
			return false;
		}

		if( !finalize_frame_buffers.empty() )
		{
			r_handle << device.waitIdle();
			if( r_handle.all_okay() )
			{
				for( auto frame_buffer : finalize_frame_buffers )
				{
					device.destroyFramebuffer( frame_buffer );
				}
				finalize_frame_buffers.clear();
			}
			else
			{
				return false;
			}
		}

		const auto swap_chain_image_count = GraphicEngine::get_swapchain_image_count();
		for( std::size_t index = 0; index < swap_chain_image_count; ++index )
		{
			std::vector<vk::ImageView> views =
			{
				GraphicEngine::get_swapchain_image_view( index ),
			};

			vk::Extent2D extent = GraphicEngine::get_window_resolution();

			auto attachment_count = finalize_render_pass.get_attachment_count();

			if( views.size() != attachment_count )
			{
				// error TODO
				return false;
			}

			if( finalize_clear_colors.size() != attachment_count )
			{
				finalize_clear_colors.clear();
				finalize_clear_colors.resize( attachment_count, vk::ClearColorValue() );
			}

			auto frame_buffer = r_handle << device.createFramebuffer( vk::FramebufferCreateInfo( vk::FramebufferCreateFlags(), finalize_render_pass.get_render_pass(), UINT32( views.size() ), views.data(), extent.width, extent.height, 1 ) );
			if( !r_handle.all_okay() )
			{
				return false;
			}
			finalize_frame_buffers.emplace_back( frame_buffer );
		}

		if( !r_handle.all_okay() )
		{
			return false;
		}
		old_swapchain = current_swapchain;
	}

	return r_handle.all_okay();
}

bool noxcain::CommandManager::validate_command_buffers( std::size_t buffer_id )
{
	ResultHandler<vk::Result> r_handle( vk::Result::eSuccess );

	vk::Device device = GraphicEngine::get_device();
	if( !command_pools[buffer_id] )
	{
		command_pools[buffer_id] = r_handle << device.createCommandPool( vk::CommandPoolCreateInfo( vk::CommandPoolCreateFlagBits::eTransient, GraphicEngine::get_graphic_queue_family_index() ) );
		if( !r_handle.all_okay() )
		{
			return false;
		}
	}
	else
	{
		device.resetCommandPool( command_pools[buffer_id], vk::CommandPoolResetFlags() );
	}

	UINT32 image_count = GraphicEngine::get_swapchain_image_count();
	if( command_buffers[buffer_id].size() != image_count )
	{
		device.freeCommandBuffers( command_pools[buffer_id], command_buffers[buffer_id] );
		command_buffers[buffer_id].clear();
	}

	if( command_buffers[buffer_id].empty() )
	{
		command_buffers[buffer_id] = r_handle << device.allocateCommandBuffers( vk::CommandBufferAllocateInfo( command_pools[buffer_id], vk::CommandBufferLevel::ePrimary, 1 + image_count ) );
		if( !r_handle.all_okay() )
		{
			return false;
		}
	}
	return true;
}

void noxcain::CommandManager::describe_deferred_render_pass()
{	
	bool multi_sampling = LogicEngine::get_graphic_settings().current_sample_count > 1;
	multi_sample_count = deferred_render_pass.get_sample_count_handle();
	auto fixed_single_sample_count = deferred_render_pass.get_sample_count_handle();

	default_color_format = deferred_render_pass.get_format_handle();
	stencil_format = deferred_render_pass.get_format_handle();
	depth_format = deferred_render_pass.get_format_handle();

	// attachment 0
	auto color_att = deferred_render_pass.add_attachment( default_color_format, multi_sample_count,
														  vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
														  vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
														  vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal );

	// attachment 1
	auto color_resolved_att = deferred_render_pass.add_attachment( default_color_format, fixed_single_sample_count,
																	vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
																	vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
																	vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal );

	// attachment 2
	auto normal_att = deferred_render_pass.add_attachment( default_color_format, multi_sample_count,
															vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
															vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
															vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal );

	// attachment 3
	auto position_att = deferred_render_pass.add_attachment( default_color_format, multi_sample_count,
															 vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
															 vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
															 vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal );

	// attachment 4
	auto depth_att = deferred_render_pass.add_attachment( depth_format, multi_sample_count,
														   vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
														   vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
														   vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilReadOnlyOptimal );

	// attachment 5
	auto stencil_att = RenderPassDescription::AttachmentHandle();
	if( multi_sampling )
	{
		stencil_att = deferred_render_pass.add_attachment( stencil_format, fixed_single_sample_count,
														   vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
														   vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
														   vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilReadOnlyOptimal );
	}

	auto geometry_subpass = deferred_render_pass.add_subpass( { // INPUT
															   },
															   { // COLOR
																   { color_att, vk::ImageLayout::eColorAttachmentOptimal },
															       { normal_att, vk::ImageLayout::eColorAttachmentOptimal },
															       { position_att, vk::ImageLayout::eColorAttachmentOptimal },
															   },
															   { //RESOLVE
															   },
															   { //PRESERVE
															   },
															   { depth_att, vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal } ); //DEPTH

	auto decal_subpass = deferred_render_pass.add_subpass( { // INPUT
															},
															{ // COLOR
																{ color_att, vk::ImageLayout::eColorAttachmentOptimal },
																{ normal_att, vk::ImageLayout::eColorAttachmentOptimal },
																{ position_att, vk::ImageLayout::eColorAttachmentOptimal }
															},
															{ // RESOLVE
															},
															{ // PRESERVED
															},
															{ depth_att, vk::ImageLayout::eDepthStencilReadOnlyOptimal } );
	auto edge_detection_subpass = RenderPassDescription::SubpassHandle();
	if( multi_sampling )
	{
		auto edge_detection_subpass = deferred_render_pass.add_subpass( { // INPUT
																			{ color_att, vk::ImageLayout::eShaderReadOnlyOptimal },
																			{ normal_att, vk::ImageLayout::eShaderReadOnlyOptimal },
																			{ position_att, vk::ImageLayout::eShaderReadOnlyOptimal }
																		},
																		{ // COLOR
																		},
																		{ // RESOLVE
																		},
																		{ // PRESERVE
																		},
																		{ stencil_att, vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal } );
	}

	auto shading_render_subpass = multi_sampling ?
		                                  deferred_render_pass.add_subpass( { // INPUT
																		        { color_att, vk::ImageLayout::eShaderReadOnlyOptimal },
																		        { normal_att, vk::ImageLayout::eShaderReadOnlyOptimal },
																		        { position_att, vk::ImageLayout::eShaderReadOnlyOptimal }
										                                    },
																			{ // COLOR
																				{ color_resolved_att, vk::ImageLayout::eColorAttachmentOptimal }
																			},
																			{ // RESOLVE
																			},
																			{ // PRESERVE
																			},
																			{ stencil_att, vk::ImageLayout::eDepthStencilReadOnlyOptimal } ) :
										  deferred_render_pass.add_subpass( { // INPUT
																				{ color_att, vk::ImageLayout::eShaderReadOnlyOptimal },
																				{ normal_att, vk::ImageLayout::eShaderReadOnlyOptimal },
																				{ position_att, vk::ImageLayout::eShaderReadOnlyOptimal }
																			},
																			{ // COLOR
																				{ color_resolved_att, vk::ImageLayout::eColorAttachmentOptimal }
																			},
																			{ // RESOLVE
																			},
																			{
																			} );// PRESERVE



	deferred_render_pass.add_dependency( geometry_subpass, vk::PipelineStageFlagBits::eLateFragmentTests, vk::AccessFlagBits::eDepthStencilAttachmentWrite,
										  decal_subpass, vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::AccessFlagBits::eDepthStencilAttachmentRead );

	if( multi_sampling )
	{
		deferred_render_pass.add_dependency( decal_subpass, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite,
											 edge_detection_subpass, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eShaderRead );

		deferred_render_pass.add_dependency( edge_detection_subpass, vk::PipelineStageFlagBits::eLateFragmentTests, vk::AccessFlagBits::eDepthStencilAttachmentWrite,
											 shading_render_subpass, vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::AccessFlagBits::eDepthStencilAttachmentRead );
	}
	else
	{
		deferred_render_pass.add_dependency( decal_subpass, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite,
											 shading_render_subpass, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eShaderRead );
	}
}


void noxcain::CommandManager::describe_finalize_render_pass()
{
	ResultHandler<vk::Result> resultHandler( vk::Result::eSuccess );

	swap_chain_image_format = finalize_render_pass.get_format_handle( GraphicEngine::get_swapchain_image_format() );
	auto fixed_single_sample_count = finalize_render_pass.get_sample_count_handle( vk::SampleCountFlagBits::e1 );

	auto final_render_target = finalize_render_pass.add_attachment( swap_chain_image_format, fixed_single_sample_count,
																	vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
																	vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
																	vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR );

	auto post_processing_subpass = finalize_render_pass.add_subpass(
		{// input
		},
		{// color
			{final_render_target, vk::ImageLayout::eColorAttachmentOptimal}
		} );

	auto overlay_subpass = finalize_render_pass.add_subpass(
		{// input
		},
		{// color
			{final_render_target, vk::ImageLayout::eColorAttachmentOptimal}
		} );

	finalize_render_pass.add_dependency( post_processing_subpass, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentRead,
										 overlay_subpass, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite );
}

bool noxcain::CommandManager::initial_transfer()
{
	ResultHandler resultHandler( vk::Result::eSuccess );
	
	const vk::Device& device = GraphicEngine::get_device();

	vk::Fence fence = resultHandler << device.createFence( vk::FenceCreateInfo() );
	vk::CommandPool initialCommandPool = resultHandler << device.createCommandPool( vk::CommandPoolCreateInfo( vk::CommandPoolCreateFlags(), GraphicEngine::get_graphic_queue_family_index() ) );
	std::vector<vk::CommandBuffer> commandBuffers = resultHandler << device.allocateCommandBuffers( vk::CommandBufferAllocateInfo( initialCommandPool, vk::CommandBufferLevel::ePrimary, 1 ) );

	if( device && resultHandler.all_okay() && commandBuffers.size() == 1 )
	{
		vk::Queue queue = device.getQueue( GraphicEngine::get_graphic_queue_family_index(), 0 );
		const auto& source = GraphicEngine::get_memory_manager().get_transfer_source_block();
		void* sourceBuffer = GraphicEngine::get_memory_manager().map_transfer_source_block();

		vk::CommandBuffer cb = commandBuffers[0];
		const auto& subresources = ResourceEngine::get_engine().get_subresources();
		
		for( std::size_t index = 0; index < subresources.size(); ++index )
		{
			subresources[index].getData( sourceBuffer, source.size );
			GraphicEngine::get_memory_manager().flush_transfer_source_block();
			const auto& destination = GraphicEngine::get_memory_manager().get_block( index );

			cb.begin( vk::CommandBufferBeginInfo( vk::CommandBufferUsageFlagBits::eOneTimeSubmit ) );
			cb.copyBuffer( source.buffer, destination.buffer, { vk::BufferCopy( source.offset, destination.offset, destination.size ) } );
			cb.end();

			queue.submit( { vk::SubmitInfo( 0, nullptr, nullptr, 1, &cb, 0, nullptr ) }, fence );
			device.waitForFences( fence, VK_TRUE, ~UINT64(0) );
			device.resetCommandPool( initialCommandPool, vk::CommandPoolResetFlags() );
			device.resetFences( { fence } );
		}

		device.destroyFence( fence );
		device.destroyCommandPool( initialCommandPool );
		GraphicEngine::get_memory_manager().unmap_transfer_source_block();
	}
	return resultHandler.all_okay();
}

/*
void noxcain::CommandManager::prepare_main_loop()
{
	vk::Result result;
	initial_transfer();
	
	//TODO destroy temp host objects?

	const vk::Device device = master.core->get_logical_device();
	const vk::SwapchainKHR swapChain = master.core->get_swapchain();
	const std::vector<vk::Image> images;
	const auto& swapChainReturn = device.getSwapchainImagesKHR( swapChain );
	const std::size_t nImages = swapChainReturn.value.size();

	acquire_semaphores.resize( nImages );
	for( std::size_t index = 0; index < nImages; ++index )
	{
		std::tie( result, acquire_semaphores[index] ) = device.createSemaphore( vk::SemaphoreCreateInfo() );
	}

	for( std::size_t index = 0; index < kSemaphoreCount; ++index )
	{
		std::tie( result, semaphores[index] ) = device.createSemaphore( vk::SemaphoreCreateInfo() );
	}

	std::tie( result, frame_end_fence ) = device.createFence( vk::FenceCreateInfo() );

	for( auto& commandInfo : command_infos )
	{
		const auto& [result, pool] = device.createCommandPool( vk::CommandPoolCreateInfo( vk::CommandPoolCreateFlagBits::eTransient, master.core->get_graphic_queue_family_index() ) );
		if( result == vk::Result::eSuccess )
		{
			commandInfo.pool = pool;
		}
		else
		{
			//TODO ERROR
		}
	}

	for( std::size_t jobIndex = 0; jobIndex < kCommanderTaskCount; ++jobIndex )
	{
		for( std::size_t ringIndex = 0; ringIndex < RECORD_RING_SIZE; ++ringIndex )
		{
			CommandTaskId jobId = static_cast<CommandTaskId>( jobIndex );
			auto& currentRecordTarget = getCommandInformation( jobId, ringIndex );
			const auto& [result,buffers] = device.allocateCommandBuffers( vk::CommandBufferAllocateInfo( 
				currentRecordTarget.pool,
				jobId == CommandTaskId::DEFERRED ||
				jobId == CommandTaskId::PRESENTATION ||
				jobId == CommandTaskId::OVERLAY_TEXT ?
				vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary ,
				jobId == CommandTaskId::PRESENTATION ? nImages : 1 ) );
			
			if( result == vk::Result::eSuccess )
			{
				currentRecordTarget.buffer = buffers;
			}
			else
			{
				//TODO ERROR
			}
		}
	}
	
	std::tie( result, timestamp_pool ) = device.createQueryPool( vk::QueryPoolCreateInfo( vk::QueryPoolCreateFlags(), vk::QueryType::eTimestamp, timestamp_count, vk::QueryPipelineStatisticFlags() ) );
}
*/
noxcain::CommandManager::~CommandManager()
{
	LogicEngine::finish();
	if( comman_main_thread.joinable() )
	{
		comman_main_thread.join();
	}

	vk::Device device = GraphicEngine::get_device();
	if( device )
	{
		ResultHandler r_handler( vk::Result::eSuccess );
		r_handler << device.waitIdle();
		if( r_handler.all_okay() )
		{
			device.destroyFramebuffer( deferred_frame_buffer );
			
			for( const auto& frame_buffer : finalize_frame_buffers )
			{
				device.destroyFramebuffer( frame_buffer );
			}
			
			for( const auto& pool : command_pools )
			{
				device.destroyCommandPool( pool );
			}
		}
	}
}

