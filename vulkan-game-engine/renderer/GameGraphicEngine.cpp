#include "GameGraphicEngine.hpp"

#include <tools/ResultHandler.hpp>
#include <renderer/CommandManager.hpp>
#include <renderer/DescriptorSetManager.hpp>
#include <renderer/GraphicCore.hpp>
#include <renderer/MemoryManagement.hpp>
#include <renderer/ShaderManager.hpp>
#include <renderer/RenderQuery.hpp>

#include <resources/GameResourceEngine.hpp>
#include <logic/GameLogicEngine.hpp>

#include "fstream"

const float pi = 3.1415926535897F;

std::unique_ptr<noxcain::GraphicEngine> noxcain::GraphicEngine::engine = std::unique_ptr<GraphicEngine>( new GraphicEngine() );

bool noxcain::GraphicEngine::clear()
{
	ResultHandler r_handler( vk::Result::eSuccess );
	vk::Device device = core->getLogicalDevice();
	if( device )
	{
		r_handler << core->getLogicalDevice().waitIdle();
		if( r_handler.all_okay() )
		{
			commands.reset( nullptr );
			descriptor_sets.reset( nullptr );
			memory.reset( nullptr );
			shader.reset( nullptr );
			return true;
		}
	}
	return false;
}

void noxcain::GraphicEngine::initialize()
{
	commands.reset( new CommandManager() );
	descriptor_sets.reset( new DescriptorSetManager() );
	memory.reset( new MemoryManager() );
	shader.reset( new ShaderManager() );
	render_query.reset( new RenderQuery() );
}

bool noxcain::GraphicEngine::run( std::shared_ptr<PresentationSurface> os_surface )
{	
	if( engine->core->initialize( os_surface ) )
	{
		bool successful_initialization = false;
		while( !successful_initialization )
		{
			engine->initialize();
			successful_initialization = engine->memory->allocate_game_memory();
			successful_initialization = successful_initialization && engine->shader->initialize();
			if( !successful_initialization )
			{
				engine->clear();

				if( !engine->core->switchToNextPhysicalDevice() )
				{
					return false;
				}
			}
		}
		engine->commands->start_loop();
		return true;
	}
	return false;
}

vk::Device noxcain::GraphicEngine::get_device()
{
	return engine->core->getLogicalDevice();
}

vk::PhysicalDevice noxcain::GraphicEngine::get_physical_device()
{
	return engine->core->getPhysicalDevice();
}

vk::SwapchainKHR noxcain::GraphicEngine::get_swapchain()
{
	return engine->core->getSwapChain();
}

noxcain::UINT32 noxcain::GraphicEngine::get_swapchain_image_count()
{
	return engine->core->get_swapchain_image_count();
}

vk::Format noxcain::GraphicEngine::get_swapchain_image_format()
{
	return engine->core->getPresentationSurfaceFormat();
}

vk::ImageView noxcain::GraphicEngine::get_swapchain_image_view( std::size_t image_index )
{
	return engine->core->getImageView( image_index );
}

bool noxcain::GraphicEngine::recreate_swapchain( bool recreate_surface )
{
	return engine->core->recreate_swapchain( recreate_surface );
}

noxcain::MemoryManager& noxcain::GraphicEngine::get_memory_manager()
{
	return *engine->memory;
}

noxcain::DescriptorSetManager& noxcain::GraphicEngine::get_descriptor_set_manager()
{
	return *engine->descriptor_sets;
}

vk::Extent2D noxcain::GraphicEngine::get_window_resolution()
{
	return engine->core->get_window_extent();
}

noxcain::UINT32 noxcain::GraphicEngine::get_graphic_queue_family_index()
{
	return engine->core->get_graphic_queue_family_index();
}

noxcain::RenderQuery& noxcain::GraphicEngine::get_render_query()
{
	return *engine->render_query;
}

vk::ShaderModule noxcain::GraphicEngine::get_shader( FragmentShaderIds shader_id )
{
	return engine->shader->get( shader_id );
}

vk::ShaderModule noxcain::GraphicEngine::get_shader( ComputeShaderIds shader_id )
{
	return engine->shader->get( shader_id );
}

vk::ShaderModule noxcain::GraphicEngine::get_shader( VertexShaderIds shader_id )
{
	return engine->shader->get( shader_id );
}

void noxcain::GraphicEngine::finish()
{
	engine->core->close_surface_base();
}

noxcain::GraphicEngine::GraphicEngine() : core( new GraphicCore() )
{
}

noxcain::GraphicEngine::~GraphicEngine()
{
}
