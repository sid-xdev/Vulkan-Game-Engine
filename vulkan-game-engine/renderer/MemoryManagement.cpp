#include "MemoryManagement.hpp"

#include <renderer/DescriptorSetManager.hpp>
#include <renderer/GameGraphicEngine.hpp>

#include <resources/GameResourceEngine.hpp>
#include <resources/GameResource.hpp>
#include <resources/FontResource.hpp>

#include <logic/GameLogicEngine.hpp>
#include <tools/ResultHandler.hpp>

#include <algorithm>

bool noxcain::MemoryManager::request_resource_memory()
{
	const auto& resources = ResourceEngine::get_engine();

	resourceBlocks.resize( resources.get_subresources().size() );

	std::vector<BlockRequest> bufferReq;
	bufferReq.reserve( hostBlockCount + resourceBlocks.size() );

	bufferReq.push_back( BlockRequest( hostTransferBlock, vk::MemoryPropertyFlagBits::eHostVisible, vk::BufferCreateInfo(
		vk::BufferCreateFlags(), ResourceEngine::get_engine().get_resource_limits().max_size_per_resource,
		vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc,
		vk::SharingMode::eExclusive, 0, nullptr ) ) );

	for( const auto& meta : resources.getSubResourcesMetaInfos( GameSubResource::SubResourceType::eStorageBuffer ) )
	{
		bufferReq.push_back( BlockRequest( resourceBlocks[meta.id], vk::MemoryPropertyFlagBits::eDeviceLocal, vk::BufferCreateInfo(
			vk::BufferCreateFlags(), meta.size,
			vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
			vk::SharingMode::eExclusive, 0, nullptr ) ) );
	}

	for( const auto& meta : resources.getSubResourcesMetaInfos( GameSubResource::SubResourceType::eVertexBuffer ) )
	{
		bufferReq.push_back( BlockRequest( resourceBlocks[meta.id], vk::MemoryPropertyFlagBits::eDeviceLocal, vk::BufferCreateInfo(
			vk::BufferCreateFlags(), meta.size,
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			vk::SharingMode::eExclusive, 0, nullptr ) ) );
	}

	for( const auto& meta : resources.getSubResourcesMetaInfos( GameSubResource::SubResourceType::eIndexBuffer ) )
	{
		bufferReq.push_back( BlockRequest( resourceBlocks[meta.id], vk::MemoryPropertyFlagBits::eDeviceLocal, vk::BufferCreateInfo(
			vk::BufferCreateFlags(), meta.size,
			vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			vk::SharingMode::eExclusive, 0, nullptr ) ) );
	}

	if( create_managed_memory( bufferReq, std::vector<ImageRequest>() ) )
	{
		const auto& resource_engine = ResourceEngine::get_engine();
		const auto& limits = resource_engine.get_resource_limits();
		const auto& fonts = resource_engine.get_fonts();
		
		DescriptorSetManager::BasicDescriptorSetUpdate font_update;

		std::vector<vk::DescriptorBufferInfo> point_offset_buffers;
		std::vector<vk::DescriptorBufferInfo> point_buffers;

		point_offset_buffers.reserve( limits.glyph_count );
		point_buffers.reserve( limits.glyph_count );

		font_update.set = BasicDescriptorSets::GLYPHS;

		for( std::size_t font_id = 0; font_id < fonts.size(); ++font_id )
		{	
			// fill up offset buffers
			for( auto offset_subresource_id : fonts[font_id].get_offset_map_resource_ids() )
			{
				const auto& block = get_block( offset_subresource_id );
				point_offset_buffers.emplace_back( block.buffer, block.offset, block.size );
			}

			// fill up pointer buffers
			for( auto points_subresource_id : fonts[font_id].get_point_map_resource_ids() )
			{
				const auto& block = get_block( points_subresource_id );
				point_buffers.emplace_back( block.buffer, block.offset, block.size );
			}
		}

		font_update.updates.emplace_back( 0, 0, DescriptorSetManager::DescriptorUpdateInfoTypes::BUFFER_INFO, UINT32( point_offset_buffers.size() ), point_offset_buffers.data() );
		font_update.updates.emplace_back( 1, 0, DescriptorSetManager::DescriptorUpdateInfoTypes::BUFFER_INFO, UINT32( point_buffers.size() ), point_buffers.data() );

		return GraphicEngine::get_descriptor_set_manager().update_basic_sets( { font_update } );
	}
	return false;
}

bool noxcain::MemoryManager::setup_main_render_destination()
{	
	free_main_render_destination_memory();
	
	auto g_settings = LogicEngine::get_graphic_settings();
	auto resolution = g_settings.get_accumulated_resolution();
	UINT32 width = resolution.width;
	UINT32 height = resolution.height;
	
	auto sample_count = g_settings.get_sample_count();
	auto formats = select_main_render_formats( width, height, sample_count );
	
	vk::Extent3D extent( width, height, 1 );

	main_render_destinations.resize( RENDER_DESTINATION_IMAGE_COUNT );
	std::vector<ImageRequest> requests;
	requests.reserve( RENDER_DESTINATION_IMAGE_COUNT );

	//----------------------------------------------------------------------------//
	//                                  COLOR                                     //
	//----------------------------------------------------------------------------//

	requests.emplace_back( main_render_destinations[static_cast<std::size_t>( RenderDestinationImages::COLOR )], vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageCreateInfo(
		vk::ImageCreateFlags(), vk::ImageType::e2D, formats.color, extent, 1, 1, static_cast<vk::SampleCountFlagBits>( g_settings.get_sample_count() ), vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment,
		vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined ) );

	requests.emplace_back( main_render_destinations[static_cast<std::size_t>( RenderDestinationImages::COLOR_RESOLVED )], vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageCreateInfo(
		vk::ImageCreateFlags(), vk::ImageType::e2D, formats.color, extent, 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eSampled,
		vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined ) );

	//----------------------------------------------------------------------------//
	//                                  NORMAL                                    //
	//----------------------------------------------------------------------------//

	requests.emplace_back( main_render_destinations[static_cast<std::size_t>( RenderDestinationImages::NORMAL )], vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eLazilyAllocated, vk::ImageCreateInfo(
		vk::ImageCreateFlags(), vk::ImageType::e2D, formats.color, extent, 1, 1, static_cast<vk::SampleCountFlagBits>( g_settings.get_sample_count() ), vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment,
		vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined ) );

	//----------------------------------------------------------------------------//
	//                                POSITION                                    //
	//----------------------------------------------------------------------------//

	requests.emplace_back( main_render_destinations[static_cast<std::size_t>( RenderDestinationImages::POSITION )], vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eLazilyAllocated, vk::ImageCreateInfo(
		vk::ImageCreateFlags(), vk::ImageType::e2D, formats.color, extent, 1, 1, static_cast<vk::SampleCountFlagBits>( g_settings.get_sample_count() ), vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment,
		vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined ) );

	//----------------------------------------------------------------------------//
	//                                POST                                        //
	//----------------------------------------------------------------------------//

	requests.emplace_back( main_render_destinations[static_cast<std::size_t>( RenderDestinationImages::POST_PROCESSING )], vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageCreateInfo(
		vk::ImageCreateFlags(), vk::ImageType::e2D, formats.color, extent, 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eColorAttachment,
		vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined ) );

	//----------------------------------------------------------------------------//
	//                                DEPTH                                       //
	//----------------------------------------------------------------------------//

	main_render_destinations[( std::size_t ) RenderDestinationImages::DEPTH_SAMPLED].viewAspect = vk::ImageAspectFlagBits::eDepth;
	requests.emplace_back( main_render_destinations[(std::size_t) RenderDestinationImages::DEPTH_SAMPLED], vk::MemoryPropertyFlagBits::eLazilyAllocated | vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageCreateInfo(
		vk::ImageCreateFlags(), vk::ImageType::e2D, formats.depth, extent, 1, 1, static_cast<vk::SampleCountFlagBits>( g_settings.get_sample_count() ), vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eDepthStencilAttachment,
		vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined ) );

	//----------------------------------------------------------------------------//
	//                                STENCIL                                     //
	//----------------------------------------------------------------------------//

	main_render_destinations[( std::size_t ) RenderDestinationImages::STENCIL_UNSAMPLED].viewAspect = vk::ImageAspectFlagBits::eStencil;
	requests.emplace_back( main_render_destinations[( std::size_t ) RenderDestinationImages::STENCIL_UNSAMPLED], vk::MemoryPropertyFlagBits::eLazilyAllocated | vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageCreateInfo(
		vk::ImageCreateFlags(), vk::ImageType::e2D, formats.stencil, extent, 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eDepthStencilAttachment,
		vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined ) );

	//----------------------------------------------------------------------------//
	//                              MEMORY CREATION                               //
	//----------------------------------------------------------------------------//

	ResultHandler<vk::Result> r_handler( vk::Result::eSuccess );

	if( !create_managed_memory( std::vector<BlockRequest>(), requests ) )
	{
		free_main_render_destination_memory();
		return false;
	}


	for( auto& binding : main_render_destinations )
	{
		binding.view = r_handler << GraphicEngine::get_device().createImageView( vk::ImageViewCreateInfo(
			vk::ImageViewCreateFlags(),
			binding.image,
			vk::ImageViewType::e2D,
			binding.format,
			vk::ComponentMapping(),
			vk::ImageSubresourceRange( binding.viewAspect, 0, 1, 0, 1 ) ) );
	}

	if( r_handler.is_critical() )
	{
		return false;
	}

	auto& descriptor_set_manager = GraphicEngine::get_descriptor_set_manager();
	std::vector<DescriptorSetManager::BasicDescriptorSetUpdate> updates;
	
	// sampled input attachments
	auto& sampled_input_attachment = updates.emplace_back();
	vk::DescriptorImageInfo color_info( vk::Sampler(), get_image( RenderDestinationImages::COLOR ).view , vk::ImageLayout::eShaderReadOnlyOptimal );
	vk::DescriptorImageInfo normal_info( vk::Sampler(), get_image( RenderDestinationImages::NORMAL ).view, vk::ImageLayout::eShaderReadOnlyOptimal );
	vk::DescriptorImageInfo position_info( vk::Sampler(), get_image( RenderDestinationImages::POSITION ).view, vk::ImageLayout::eShaderReadOnlyOptimal );
	
	sampled_input_attachment.set = BasicDescriptorSets::SHADING_INPUT_ATTACHMENTS;
	sampled_input_attachment.updates.emplace_back( 0, 0, DescriptorSetManager::DescriptorUpdateInfoTypes::IMAGE_INFO, 1, &position_info );
	sampled_input_attachment.updates.emplace_back( 1, 0, DescriptorSetManager::DescriptorUpdateInfoTypes::IMAGE_INFO, 1, &normal_info );
	sampled_input_attachment.updates.emplace_back( 2, 0, DescriptorSetManager::DescriptorUpdateInfoTypes::IMAGE_INFO, 1, &color_info );

	// final texture attachments
	auto& final_texture_attachment = updates.emplace_back();
	vk::DescriptorImageInfo final_texture_info( vk::Sampler(), get_image( RenderDestinationImages::COLOR_RESOLVED ).view, vk::ImageLayout::eShaderReadOnlyOptimal );
	final_texture_attachment.set = BasicDescriptorSets::FINALIZED_MASTER_TEXTURE;
	final_texture_attachment.updates.emplace_back( 0, 0, DescriptorSetManager::DescriptorUpdateInfoTypes::IMAGE_INFO, 1, &final_texture_info );

	if( !descriptor_set_manager.update_basic_sets( updates ) )
	{
		//TODO log entry
		return false;
	}

	return  true;
}

noxcain::MemoryManager::MemoryManager()
{
}

vk::Format noxcain::MemoryManager::find_format( std::initializer_list<vk::Format> formats,
															  vk::ImageUsageFlags imageFeatures,
															  vk::FormatFeatureFlags formatFeatures, bool optimalTiling,
															  vk::Extent3D& wantedSize, vk::SampleCountFlagBits& wantedSampleCount ) const
{
	std::vector<std::pair<vk::Format, vk::ImageFormatProperties>> availableFormats;

	for( vk::Format imageFormat : formats )
	{
		vk::FormatProperties properties = GraphicEngine::get_physical_device().getFormatProperties( imageFormat );

		if( ( optimalTiling ? properties.optimalTilingFeatures & formatFeatures : properties.linearTilingFeatures & formatFeatures ) == formatFeatures )
		{
			auto imageFormatProps = GraphicEngine::get_physical_device().getImageFormatProperties(
				imageFormat,
				vk::ImageType::e2D,
				optimalTiling ? vk::ImageTiling::eOptimal : vk::ImageTiling::eLinear,
				imageFeatures,
				vk::ImageCreateFlags() );
			availableFormats.push_back( std::pair<vk::Format, vk::ImageFormatProperties>( imageFormat, imageFormatProps.value ) );
		}
	}

	if( availableFormats.empty() ) return vk::Format::eUndefined;

	auto sortSampleCount = [wantedSize]( const std::pair<vk::Format, vk::ImageFormatProperties>& left, const std::pair<vk::Format, vk::ImageFormatProperties>& right )
	{
		const auto leftSize = std::tie( left.second.maxExtent.width, left.second.maxExtent.height, left.second.maxExtent.depth );
		const auto rightSize = std::tie( right.second.maxExtent.width, right.second.maxExtent.height, right.second.maxExtent.depth );

		if( leftSize != rightSize )
			return leftSize > rightSize;
		else
		{
			vk::SampleCountFlagBits firstSampleCount =
				( left.second.sampleCounts & vk::SampleCountFlagBits::e64 ) ? vk::SampleCountFlagBits::e64 :
				( left.second.sampleCounts & vk::SampleCountFlagBits::e32 ) ? vk::SampleCountFlagBits::e32 :
				( left.second.sampleCounts & vk::SampleCountFlagBits::e16 ) ? vk::SampleCountFlagBits::e16 :
				( left.second.sampleCounts & vk::SampleCountFlagBits::e8 ) ? vk::SampleCountFlagBits::e8 :
				( left.second.sampleCounts & vk::SampleCountFlagBits::e4 ) ? vk::SampleCountFlagBits::e4 :
				( left.second.sampleCounts & vk::SampleCountFlagBits::e2 ) ? vk::SampleCountFlagBits::e2 :
				vk::SampleCountFlagBits::e1;

			vk::SampleCountFlagBits secondSampleCount =
				( right.second.sampleCounts & vk::SampleCountFlagBits::e64 ) ? vk::SampleCountFlagBits::e64 :
				( right.second.sampleCounts & vk::SampleCountFlagBits::e32 ) ? vk::SampleCountFlagBits::e32 :
				( right.second.sampleCounts & vk::SampleCountFlagBits::e16 ) ? vk::SampleCountFlagBits::e16 :
				( right.second.sampleCounts & vk::SampleCountFlagBits::e8 ) ? vk::SampleCountFlagBits::e8 :
				( right.second.sampleCounts & vk::SampleCountFlagBits::e4 ) ? vk::SampleCountFlagBits::e4 :
				( right.second.sampleCounts & vk::SampleCountFlagBits::e2 ) ? vk::SampleCountFlagBits::e2 :
				vk::SampleCountFlagBits::e1;

			return static_cast<INT32>( firstSampleCount ) > static_cast<INT32>( secondSampleCount );
		}
	};
	auto isSmaller = []( const vk::Extent3D& left, const vk::Extent3D& right )
	{
		return std::tie( left.width, left.height, left.depth ) < std::tie( right.width, right.height, right.depth );
	};

	std::sort( availableFormats.begin(), availableFormats.end(), sortSampleCount );

	auto currentFormat = availableFormats.front();

	if( isSmaller( currentFormat.second.maxExtent, wantedSize ) )
	{
		wantedSize = vk::Extent3D( std::min<UINT32>( currentFormat.second.maxExtent.width, wantedSize.width ),
								   std::min<UINT32>( currentFormat.second.maxExtent.height, wantedSize.height ),
								   std::min<UINT32>( currentFormat.second.maxExtent.depth, wantedSize.depth ) );
	}
	else
	{
		for( const auto& format : availableFormats )
		{
			if( !isSmaller( format.second.maxExtent, wantedSize ) )
			{
				currentFormat = format;
				if( currentFormat.second.sampleCounts & wantedSampleCount ) break;
			}
			else
			{
				break;
			}
		}
	}

	wantedSampleCount =
		( currentFormat.second.sampleCounts & vk::SampleCountFlagBits::e64 ) && ( vk::SampleCountFlagBits::e64 <= wantedSampleCount ) ? vk::SampleCountFlagBits::e64 :
		( currentFormat.second.sampleCounts & vk::SampleCountFlagBits::e32 ) && ( vk::SampleCountFlagBits::e32 <= wantedSampleCount ) ? vk::SampleCountFlagBits::e32 :
		( currentFormat.second.sampleCounts & vk::SampleCountFlagBits::e16 ) && ( vk::SampleCountFlagBits::e16 <= wantedSampleCount ) ? vk::SampleCountFlagBits::e16 :
		( currentFormat.second.sampleCounts & vk::SampleCountFlagBits::e8 ) && ( vk::SampleCountFlagBits::e8 <= wantedSampleCount ) ? vk::SampleCountFlagBits::e8 :
		( currentFormat.second.sampleCounts & vk::SampleCountFlagBits::e4 ) && ( vk::SampleCountFlagBits::e4 <= wantedSampleCount ) ? vk::SampleCountFlagBits::e4 :
		( currentFormat.second.sampleCounts & vk::SampleCountFlagBits::e2 ) && ( vk::SampleCountFlagBits::e2 <= wantedSampleCount ) ? vk::SampleCountFlagBits::e2 :
		vk::SampleCountFlagBits::e1;


	return currentFormat.first;
}

bool noxcain::MemoryManager::create_managed_memory(
	const std::vector<BlockRequest>& blockRequests, 
	const std::vector<ImageRequest>& imageRequests )
{
	ResultHandler r_handler( vk::Result::eSuccess );
	const vk::Device& device = GraphicEngine::get_device();
	const vk::PhysicalDevice& physicalDevice = GraphicEngine::get_physical_device();

	const vk::PhysicalDeviceMemoryProperties& memoryProperties = physicalDevice.getMemoryProperties();
	const vk::PhysicalDeviceLimits& limits = physicalDevice.getProperties().limits;

	//collects all buffer request of the same kind
	struct BufferBin
	{
		UINT32 memoryTypeBits = 0;
		vk::BufferCreateInfo createInfos;
		std::vector<const BlockRequest*> blocks;
	};
	std::vector<BufferBin> bufferBins;

	for( const BlockRequest& block : blockRequests )
	{
		vk::DeviceSize alignment =
			block.usage & ( vk::BufferUsageFlagBits::eStorageTexelBuffer | block.usage & vk::BufferUsageFlagBits::eUniformTexelBuffer ) ? limits.minTexelBufferOffsetAlignment :
			block.usage & vk::BufferUsageFlagBits::eStorageBuffer ? limits.minStorageBufferOffsetAlignment :
			block.usage & vk::BufferUsageFlagBits::eUniformBuffer ? limits.minUniformBufferOffsetAlignment :
			1;

		UINT32 memoryTypeBits = 0;
		for( UINT32 index = 0; index < memoryProperties.memoryTypeCount; ++index )
		{
			if( ( memoryProperties.memoryTypes[index].propertyFlags & block.memoryProperties ) == block.memoryProperties )
			{
				memoryTypeBits |= ( 0x1 << index );
			}
		}

		block.destBlockBinding.size = block.size;

		bool needNewBin = true;
		for( BufferBin& bufferBin : bufferBins )
		{
			UINT32 newMemoryTypeBits = bufferBin.memoryTypeBits & memoryTypeBits;
			if( newMemoryTypeBits &&
				bufferBin.createInfos.flags == block.flags &&
				bufferBin.createInfos.usage == block.usage &&
				bufferBin.createInfos.sharingMode == block.sharingMode )
			{
				if( block.sharingMode == vk::SharingMode::eConcurrent )
				{
					bool isEqual = bufferBin.createInfos.queueFamilyIndexCount == block.queueFamilyIndexCount;
					if( isEqual )
					{
						for( UINT32 index = 0; index < block.queueFamilyIndexCount; ++index )
						{
							if( bufferBin.createInfos.pQueueFamilyIndices[index] != block.pQueueFamilyIndices[index] )
							{
								isEqual = false;
								break;
							}
						}
					}

					if( !isEqual )
					{
						continue;
					}
				}

				needNewBin = false;
				bufferBin.memoryTypeBits = newMemoryTypeBits;
				bufferBin.blocks.emplace_back( &block );

				vk::DeviceSize padding = bufferBin.createInfos.size % alignment;
				if( padding > 0 ) padding = alignment - padding;

				block.destBlockBinding.offset = bufferBin.createInfos.size + padding;
				bufferBin.createInfos.setSize( block.destBlockBinding.offset + block.destBlockBinding.size );
			}
		}

		if( needNewBin )
		{
			block.destBlockBinding.offset = 0;
			bufferBins.push_back( BufferBin() );
			BufferBin& newBufferBin = bufferBins.back();

			newBufferBin.createInfos = block;
			newBufferBin.memoryTypeBits = memoryTypeBits;
			newBufferBin.blocks.emplace_back( &block );
		}
	}

	struct MemoryRequirements
	{
		void* identifier;
		vk::DeviceSize alignment;
	};

	struct MemoryBin
	{
		UINT32 memoryTypeBits = 0;
		std::vector<std::pair<ImageBinding*, vk::DeviceSize>> images;
		std::vector<std::pair<std::size_t, vk::DeviceSize>> buffers;
	};
	std::vector<MemoryBin> memoryBins;

	for( const ImageRequest& imageRequest : imageRequests )
	{
		UINT32 memoryTypeBits = 0;
		for( UINT32 index = 0; index < memoryProperties.memoryTypeCount; ++index )
		{
			if( ( memoryProperties.memoryTypes[index].propertyFlags & imageRequest.memoryProperties ) == imageRequest.memoryProperties )
			{
				memoryTypeBits |= ( 0x1 << index );
			}
		}

		auto newImage = device.createImage( imageRequest );
		if( newImage.result != vk::Result::eSuccess || !newImage.value )
		{
			//TODO error
			return false;
		}

		vk::MemoryRequirements memoryRequirements = device.getImageMemoryRequirements( newImage.value );

		imageRequest.destImageBinding.image = newImage.value;
		imageRequest.destImageBinding.offset = 0;
		imageRequest.destImageBinding.size = memoryRequirements.size;
		imageRequest.destImageBinding.memoryIndex = 0;
		imageRequest.destImageBinding.extent = imageRequest.extent;
		imageRequest.destImageBinding.format = imageRequest.format;
		imageRequest.destImageBinding.sampleCount = imageRequest.samples;

		memoryTypeBits &= memoryRequirements.memoryTypeBits;
		if( !memoryTypeBits )
		{
			memoryTypeBits = memoryRequirements.memoryTypeBits;
		}

		bool needNewBin = true;
		for( MemoryBin& memoryBin : memoryBins )
		{
			UINT32 newMemoryTypeBits = memoryBin.memoryTypeBits & memoryTypeBits;
			if( newMemoryTypeBits )
			{
				needNewBin = false;
				memoryBin.images.emplace_back( &imageRequest.destImageBinding, memoryRequirements.alignment );
			}
		}

		if( needNewBin )
		{
			memoryBins.push_back( MemoryBin() );
			MemoryBin& newMemoryBin = memoryBins.back();
			newMemoryBin.memoryTypeBits = memoryTypeBits;
			newMemoryBin.images.emplace_back( &imageRequest.destImageBinding, memoryRequirements.alignment );
		}
	}

	for( BufferBin& bufferBin : bufferBins )
	{
		auto newBuffer = device.createBuffer( bufferBin.createInfos );
		if( newBuffer.result != vk::Result::eSuccess || !newBuffer.value )
		{
			//TODO error
			return false;
		}
		vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements( newBuffer.value );

		std::size_t bufferIndex = buffers.size();
		buffers.push_back( { newBuffer.value, 0, memoryRequirements.size, 0, UINT32( bufferBin.blocks.size() ) } );

		for( const BlockRequest* block : bufferBin.blocks )
		{
			block->destBlockBinding.bufferIndex = bufferIndex;
		}

		UINT32 memoryTypeBits = bufferBin.memoryTypeBits & memoryRequirements.memoryTypeBits;
		if( !memoryTypeBits )
		{
			memoryTypeBits = memoryRequirements.memoryTypeBits;
		}

		bool needNewBin = true;
		for( MemoryBin& memoryBin : memoryBins )
		{
			UINT32 newMemoryTypeBits = memoryBin.memoryTypeBits & memoryTypeBits;
			if( newMemoryTypeBits )
			{
				needNewBin = false;

				memoryBin.buffers.push_back( { bufferIndex, memoryRequirements.alignment } );
			}
		}

		if( needNewBin )
		{
			memoryBins.push_back( MemoryBin() );
			MemoryBin& newMemoryBin = memoryBins.back();
			newMemoryBin.memoryTypeBits = memoryTypeBits;
			newMemoryBin.buffers.push_back( { bufferIndex, memoryRequirements.alignment } );
		}
	}

	for( MemoryBin& memoryBin : memoryBins )
	{
		vk::DeviceSize memorySize = 0;
		for( const auto& memReq : memoryBin.buffers )
		{
			vk::DeviceSize padding = memorySize % memReq.second;
			if( padding > 0 ) padding = memReq.second - padding;

			BufferBinding& currentBufferBinding = buffers[memReq.first];
			currentBufferBinding.offset = memorySize + padding;
			memorySize = currentBufferBinding.offset + currentBufferBinding.size;
		}

		for( const auto& memReq : memoryBin.images )
		{
			vk::DeviceSize padding = memorySize % memReq.second;
			if( padding > 0 ) padding = memReq.second - padding;

			ImageBinding& currentImageBinding = *memReq.first;
			currentImageBinding.offset = memorySize + padding;
			memorySize = currentImageBinding.offset + currentImageBinding.size;
		}

		vk::DeviceMemory newMemory;
		for( UINT32 index = 0; index < memoryProperties.memoryTypeCount; ++index )
		{
			if( ( ( 0x1 << index ) & memoryBin.memoryTypeBits ) && memoryProperties.memoryHeaps[memoryProperties.memoryTypes[index].heapIndex].size >= memorySize )
			{
				newMemory = r_handler << device.allocateMemory( vk::MemoryAllocateInfo( memorySize, index ) );
				break;
			}
		}

		if( r_handler.all_okay() && newMemory )
		{
			UINT32 memoryIndex = UINT32( memory.size() );
			memory.push_back( { newMemory, UINT32( memoryBin.buffers.size() + memoryBin.images.size() ) } );
			mapRanges.push_back( {} );

			for( const auto& memReq : memoryBin.buffers )
			{
				BufferBinding& currentBufferBinding = buffers[memReq.first];

				device.bindBufferMemory( currentBufferBinding.buffer, newMemory, currentBufferBinding.offset );
				currentBufferBinding.memoryIndex = memoryIndex;
			}

			for( const auto& memReq : memoryBin.images )
			{
				ImageBinding& currentImageBinding = *memReq.first;

				device.bindImageMemory( currentImageBinding.image, newMemory, currentImageBinding.offset );
				currentImageBinding.memoryIndex = memoryIndex;
			}
		}
	}
	return r_handler.all_okay();
}

inline noxcain::MemoryManager::RenderDestinationFormats noxcain::MemoryManager::select_main_render_formats( UINT32& width, UINT32& height, UINT32& sample_count )
{
	RenderDestinationFormats formats;
	bool new_settings = false;
	const auto& window_extent = GraphicEngine::get_window_resolution();

	vk::SampleCountFlagBits vk_sample_count = static_cast<vk::SampleCountFlagBits>( sample_count );

	vk::Extent3D available_extent( width, height, 1 );
	if( !available_extent.width || !available_extent.height )
	{
		available_extent = vk::Extent3D( window_extent, 1 );
		new_settings = true;
	}

	vk::SampleCountFlagBits available_sample_count = vk_sample_count;

	// check formats and available sizes and features
	formats.depth = find_format( {
		vk::Format::eD16Unorm, //depth only
		vk::Format::eD16UnormS8Uint,
		vk::Format::eD24UnormS8Uint }, //should always available
		vk::ImageUsageFlagBits::eDepthStencilAttachment,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment,
		true, available_extent, available_sample_count );

	vk::SampleCountFlagBits stencil_sample_count = vk::SampleCountFlagBits::e1;
	formats.stencil = find_format( {
		vk::Format::eS8Uint, // stencil only
		vk::Format::eD16UnormS8Uint,
		vk::Format::eD24UnormS8Uint },
		vk::ImageUsageFlagBits::eDepthStencilAttachment,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment,
		true, available_extent, stencil_sample_count );

	formats.color = find_format(
		{
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eR8G8B8A8Snorm,
			vk::Format::eR8G8B8A8Uscaled,
			vk::Format::eR8G8B8A8Sscaled,
			vk::Format::eR8G8B8A8Uint,
			vk::Format::eR8G8B8A8Sint,
			vk::Format::eR8G8B8A8Srgb,
			vk::Format::eB8G8R8A8Unorm,
			vk::Format::eB8G8R8A8Snorm,
			vk::Format::eB8G8R8A8Uscaled,
			vk::Format::eB8G8R8A8Sscaled,
			vk::Format::eB8G8R8A8Uint,
			vk::Format::eB8G8R8A8Sint,
			vk::Format::eB8G8R8A8Srgb,
			vk::Format::eA8B8G8R8UnormPack32,
			vk::Format::eA8B8G8R8SnormPack32,
			vk::Format::eA8B8G8R8UscaledPack32,
			vk::Format::eA8B8G8R8SscaledPack32,
			vk::Format::eA8B8G8R8UintPack32,
			vk::Format::eA8B8G8R8SintPack32,
			vk::Format::eA8B8G8R8SrgbPack32
		},
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eInputAttachment,
		vk::FormatFeatureFlagBits::eColorAttachmentBlend, true, available_extent, available_sample_count );

	if( new_settings || available_extent.width < width || available_extent.height < height || available_sample_count < vk_sample_count )
	{
		sample_count = static_cast<UINT32>( available_sample_count );
		width = available_extent.width;
		height = available_extent.height;

		LogicEngine::set_graphic_settings( sample_count, 1.0, width, height );
	}
	return formats;
}

noxcain::MemoryManager::~MemoryManager()
{
	const vk::Device& device = GraphicEngine::get_device();
	for( UINT32 index = 0; index < mapRanges.size(); ++index )
	{
		if( mapRanges[index].memoryPointer )
		{
			device.unmapMemory( memory[index].memory );
		}
	}
	
	for( ImageBinding& binding : main_render_destinations )
	{
		device.destroyImageView( binding.view );
		device.destroyImage( binding.image );
	}

	for( ImageBinding& binding : resourceImages )
	{
		device.destroyImageView( binding.view );
		device.destroyImage( binding.image );
	}


	for( BufferBinding& binding : buffers )
	{
		device.destroyBuffer( binding.buffer );
	}

	for( MemoryCounter memory : memory )
	{
		device.freeMemory( memory.memory );
	}

	for( auto& sampler : samplers )
	{
		device.destroySampler( sampler );
	}
}

bool noxcain::MemoryManager::allocate_game_memory()
{
	return request_resource_memory();
}

bool noxcain::MemoryManager::has_render_destination_memory()
{
	return !main_render_destinations.empty();
}

void noxcain::MemoryManager::free_main_render_destination_memory()
{
	vk::Device device = GraphicEngine::get_device();

	for( ImageBinding& binding : main_render_destinations )
	{
		device.destroyImageView( binding.view );
		device.destroyImage( binding.image );

		--memory[binding.memoryIndex].usageCount;
		if( !memory[binding.memoryIndex].usageCount )
		{
			device.freeMemory( memory[binding.memoryIndex].memory );
			memory[binding.memoryIndex].memory = vk::DeviceMemory();
		}

		binding = ImageBinding();
	}
}

noxcain::MemoryManager::Block noxcain::MemoryManager::get_block( std::size_t blockIndex ) const
{
	Block block;

	block.buffer = buffers[resourceBlocks[blockIndex].bufferIndex].buffer;
	block.offset = resourceBlocks[blockIndex].offset;
	block.size = resourceBlocks[blockIndex].size;

	return block;
}

noxcain::MemoryManager::Block noxcain::MemoryManager::get_transfer_source_block() const
{
	Block block;

	block.buffer = buffers[hostTransferBlock.bufferIndex].buffer;
	block.offset = hostTransferBlock.offset;
	block.size = hostTransferBlock.size;

	return block;
}

void* noxcain::MemoryManager::map_transfer_source_block()
{
	const vk::Device& device = GraphicEngine::get_device();
	return device.mapMemory( memory[buffers[hostTransferBlock.bufferIndex].memoryIndex].memory,
							 buffers[hostTransferBlock.bufferIndex].offset + hostTransferBlock.offset,
							 hostTransferBlock.size ).value;
}

void noxcain::MemoryManager::unmap_transfer_source_block()
{
	const vk::Device& device = GraphicEngine::get_device();
	device.unmapMemory( memory[buffers[hostTransferBlock.bufferIndex].memoryIndex].memory );
}

void noxcain::MemoryManager::flush_transfer_source_block()
{
	const vk::Device& device = GraphicEngine::get_device();
	vk::MappedMemoryRange range( memory[buffers[hostTransferBlock.bufferIndex].memoryIndex].memory,
								 0,
								 VK_WHOLE_SIZE );
	device.flushMappedMemoryRanges( { range } );
}

const noxcain::MemoryManager::ImageBinding& noxcain::MemoryManager::get_image( RenderDestinationImages id ) const
{
	return main_render_destinations[static_cast<std::size_t>( id )];
}

vk::DeviceMemory noxcain::MemoryManager::get_unmanaged_memory( vk::DeviceSize size, vk::MemoryPropertyFlags memoryProperties, UINT32 typeIndexFilter )
{
	vk::Device device = GraphicEngine::get_device();
	vk::PhysicalDevice phyDevice = GraphicEngine::get_physical_device();

	vk::PhysicalDeviceMemoryProperties memProps = phyDevice.getMemoryProperties();

	for( UINT32 typeIndex = 0; typeIndex < memProps.memoryTypeCount; ++typeIndex )
	{
		if( ( typeIndexFilter & ( 0x1 << typeIndex )) && ( memProps.memoryTypes[typeIndex].propertyFlags & memoryProperties ) == memoryProperties && memProps.memoryHeaps[memProps.memoryTypes[typeIndex].heapIndex].size >= size )
		{
			return device.allocateMemory( vk::MemoryAllocateInfo( size, typeIndex ) ).value;
		}
	}

	return vk::DeviceMemory();
}
