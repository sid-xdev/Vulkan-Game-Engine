#include "CommandSubmit.hpp"

#include <renderer/GraphicEngineConstants.hpp>
#include <renderer/GameGraphicEngine.hpp>
#include <renderer/RenderQuery.hpp>

#include <logic/GameLogicEngine.hpp>
#include <tools/ResultHandler.hpp>

noxcain::CommandSubmit::CommandSubmit() : submit_thread( &CommandSubmit::submit_loop, this )
{
	free_buffer_ids.resize( RECORD_RING_SIZE );
	for( UINT32 index = 0; index < RECORD_RING_SIZE; ++index )
	{
		free_buffer_ids[index] = index;
	}
}

noxcain::CommandSubmit::~CommandSubmit()
{
	{
		std::unique_lock lock( submit_mutex );
		status = Status::EXIT;
		submit_condition.notify_all();
	}

	if( submit_thread.joinable() )
	{
		submit_thread.join();
	}

	vk::Device device = GraphicEngine::get_device();
	if( device )
	{
		ResultHandler r_handler( vk::Result::eSuccess );
		r_handler << device.waitIdle();
		if( r_handler.all_okay() )
		{
			device.destroyFence( frame_end_fence );
			for( const auto& semaphore : semaphores )
			{
				device.destroySemaphore( semaphore );
			}
		}
	}
}

noxcain::INT32 noxcain::CommandSubmit::get_free_buffer_id()
{
	std::unique_lock lock( submit_mutex );
	submit_condition.wait( lock, [this]() { return !free_buffer_ids.empty() || status == Status::EXIT; } );
	if( status == Status::EXIT )
	{
		return -1;
	}
	
	INT32 id( free_buffer_ids.back() );
	free_buffer_ids.pop_back();
	return id;
}

bool noxcain::CommandSubmit::set_newest_command_buffer( SubmitCommandBufferData buffer_data )
{
	std::unique_lock lock( submit_mutex );
	if( status != Status::EXIT )
	{		
		// last buffer was not used ( graphic card to slow ) --> give it free
		if( buffer_available )
		{
			free_buffer_ids.push_back( available_buffer.id );
			submit_condition.notify_all();
		}
		// set new buffer
		else
		{
			buffer_available = true;
		}
		available_buffer = buffer_data;
		return true;
	}
	return false;
}

bool noxcain::CommandSubmit::check_swapchain()
{
	std::unique_lock lock( submit_mutex );
	if( status == Status::RECREAT_SWAPCHAIN )
	{
		if( GraphicEngine::recreate_swapchain( lost_surface ) )
		{
			create_semaphores();
			status = Status::SUBMIT;
			if( buffer_available )
			{
				buffer_available = false;
				free_buffer_ids.push_back( available_buffer.id );
			}
		}
		else
		{
			status = Status::EXIT;
		}
		lost_surface = false;
		submit_condition.notify_all();
	}

	return status != Status::EXIT;
}

void noxcain::CommandSubmit::recreate_swapchain( bool recreate_surface )
{
	std::unique_lock lock( submit_mutex );
	status = Status::RECREAT_SWAPCHAIN;
	lost_surface = recreate_surface;

	submit_condition.wait( lock, [this]() -> bool
	{
		return status != Status::RECREAT_SWAPCHAIN;
	} );
}

noxcain::UINT32 noxcain::CommandSubmit::submit_loop()
{
	Watcher watcher( [this]()
	{
		std::unique_lock lock( submit_mutex );
		status = Status::EXIT;
		submit_condition.notify_all();
	} );
	ResultHandler<vk::Result> r_handler( vk::Result::eSuccess );
	r_handler.add_warnings( { vk::Result::eErrorOutOfDateKHR, vk::Result::eSuboptimalKHR, vk::Result::eErrorSurfaceLostKHR } );

	const vk::Device device = GraphicEngine::get_device();
	const vk::Queue& queue = device.getQueue( GraphicEngine::get_graphic_queue_family_index(), 0 );

	create_semaphores();
	frame_end_fence = r_handler << device.createFence( vk::FenceCreateInfo( vk::FenceCreateFlags() ) );

	if( r_handler.all_okay() )
	{
		while( status != Status::EXIT )
		{
			r_handler.reset();
			const auto start_time = std::chrono::steady_clock::now();

			// get command buffers
			SubmitCommandBufferData current_buffers;
			{
				std::unique_lock lock( submit_mutex );
				submit_condition.wait( lock, [this]
				{
					return buffer_available || status == Status::EXIT;
				} );
				current_buffers = available_buffer;
				buffer_available = false;
			}

			time_collection_all.start_frame( 0.0, 0.7, 0.3, 1.0, "" );

			// submit command buffers
			{
				const vk::SwapchainKHR& swapChain = GraphicEngine::get_swapchain();
				UINT32 image_index = r_handler << device.acquireNextImageKHR( swapChain, GRAPHIC_TIMEOUT_DURATION.count(), get_semaphore( SemaphoreIds::ACQUIRE ), vk::Fence() );
				if( r_handler.all_okay() )
				{
					vk::PipelineStageFlags render_stage_flags( vk::PipelineStageFlagBits::eColorAttachmentOutput );
					vk::PipelineStageFlags sampling_stage_flags( vk::PipelineStageFlagBits::eFragmentShader );

					r_handler << queue.submit(
						{
							vk::SubmitInfo(
								1, &get_semaphore( SemaphoreIds::ACQUIRE ), &render_stage_flags,
								1, &( current_buffers.main_buffer ),
								1, &get_semaphore( SemaphoreIds::RENDER ) ),
							vk::SubmitInfo(
								1, &get_semaphore( SemaphoreIds::RENDER ), &sampling_stage_flags,
								1, &( current_buffers.finalize_command_buffers[image_index] ),
								1, &get_semaphore( SemaphoreIds::SUPER_SAMPLING ) )
						},
						frame_end_fence );

					if( r_handler.all_okay() )
					{
						r_handler << queue.presentKHR( vk::PresentInfoKHR( 1, &get_semaphore( SemaphoreIds::SUPER_SAMPLING ), 1, &swapChain, &image_index ) );
						if( !r_handler.is_critical() )
						{
							r_handler << device.waitForFences( { frame_end_fence }, VK_TRUE, ~UINT64( 0 ) );
							r_handler << device.resetFences( { frame_end_fence } );

							if( !r_handler.is_critical() )
							{
								std::array<UINT64, RenderQuery::TIMESTAMP_COUNT> time_stamps;
								auto end_time = std::chrono::steady_clock::now();
								auto timestamp_pool = GraphicEngine::get_render_query().get_timestamp_pool();
								if( timestamp_pool )
								{
									DOUBLE timestamp_period = GraphicEngine::get_physical_device().getProperties().limits.timestampPeriod;
									/*r_handler <<*/ device.getQueryPoolResults( timestamp_pool, 0, RenderQuery::TIMESTAMP_COUNT, vk::ArrayProxy<UINT64>( time_stamps ), 0, vk::QueryResultFlagBits::e64 );
									if( r_handler.all_okay() )
									{
										time_collection_gpu.add_time_frame(
											end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::START] ) ) ),
											end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::AFTER_GEOMETRY] ) ) ),
											0.8, 0.8, 0.0, 0.8, "geometry" );

										time_collection_gpu.add_time_frame(
											end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::AFTER_GEOMETRY] ) ) ),
											end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::AFTER_GLYPHS] ) ) ),
											0.8, 0.0, 0.0, 0.8, "vector" );

										time_collection_gpu.add_time_frame(
											end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::AFTER_GLYPHS] ) ) ),
											end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::AFTER_SHADING] ) ) ),
											0.8, 0.0, 0.8, 0.8, "shading" );

										time_collection_gpu.add_time_frame(
											end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::BEFOR_POST] ) ) ),
											end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::AFTER_POST] ) ) ),
											0.0, 0.0, 0.8, 0.8, "post" );

										time_collection_gpu.add_time_frame(
											end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::BEFOR_OVERLAY] ) ) ),
											end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::AFTER_OVERLAY] ) ) ),
											0.0, 0.8, 0.8, 0.8, "overlay" );
									}
								}

							}

						}
					}
				}
			}

			// free command buffers
			{
				std::unique_lock lock( submit_mutex );
				free_buffer_ids.push_back( current_buffers.id );
				submit_condition.notify_all();
			}

			time_collection_all.end_frame();
			const auto end_time = std::chrono::steady_clock::now();
			LogicEngine::set_gpu_cycle_duration( end_time - start_time );

			if( !r_handler.all_okay() && !r_handler.is_critical() )
			{
				recreate_swapchain( r_handler.get_last_error() == vk::Result::eErrorSurfaceLostKHR );
			}

			if( r_handler.is_critical() ) break;
		}
	}
	GraphicEngine::finish();
	return 0;
}

void noxcain::CommandSubmit::create_semaphores()
{
	ResultHandler r_handler( vk::Result::eSuccess );
	vk::Device device = GraphicEngine::get_device();
	for( std::size_t index = 0; index < SEMOPHORE_COUNT; ++index )
	{
		device.destroySemaphore( semaphores[index] );
	}

	for( std::size_t index = 0; index < SEMOPHORE_COUNT; ++index )
	{
		semaphores[index] = r_handler << device.createSemaphore( vk::SemaphoreCreateInfo( vk::SemaphoreCreateFlags() ) );
	}
}
