#include "RenderQuery.hpp"

#include <renderer/GameGraphicEngine.hpp>
#include <tools/ResultHandler.hpp>

noxcain::RenderQuery::RenderQuery()
{
	ResultHandler result_handler( vk::Result::eSuccess );
	const vk::Device& device = GraphicEngine::get_device();

	timestamp_pool = result_handler << device.createQueryPool( vk::QueryPoolCreateInfo( vk::QueryPoolCreateFlags(), vk::QueryType::eTimestamp, TIMESTAMP_COUNT ) );
}

noxcain::RenderQuery::~RenderQuery()
{
	const vk::Device& device = GraphicEngine::get_device();
	if( device )
	{
		ResultHandler<vk::Result> r_handle( vk::Result::eSuccess );
		r_handle << device.waitIdle();
		if( r_handle.all_okay() )
		{
			device.destroyQueryPool( timestamp_pool );
		}
	}
}
