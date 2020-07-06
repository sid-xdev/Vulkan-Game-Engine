#pragma once

#include <renderer/GameGraphicEngine.hpp>

#include <vector>
#include <array>

namespace noxcain
{
	class MemoryManager
	{
	public:
		struct Block
		{
			vk::Buffer buffer;
			UINT64 offset = 0;
			UINT64 size = 0;
		};

		struct ImageBinding
		{
			vk::Image image;
			UINT64 offset = 0;
			UINT64 size = 0;
			UINT32 memoryIndex = 0;

			vk::ImageView view;
			vk::Extent3D extent;
			vk::Format format = vk::Format::eUndefined;
			vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e8;
			vk::ImageAspectFlags viewAspect = vk::ImageAspectFlagBits::eColor;
		};

		enum class RenderDestinationImages : std::size_t
		{
			COLOR = 0,
			COLOR_RESOLVED,
			NORMAL,
			POSITION,

			DEPTH_SAMPLED,
			STENCIL_UNSAMPLED,

			POST_PROCESSING
		};

		MemoryManager();
		~MemoryManager();

	private:
		MemoryManager( const MemoryManager& ) = delete;
		MemoryManager& operator=( const MemoryManager& ) = delete;
		

		static constexpr std::size_t namedBlockCount = 3;
		static constexpr std::size_t hostBlockCount = 1;
		static constexpr std::size_t RENDER_DESTINATION_IMAGE_COUNT = ( std::size_t )RenderDestinationImages::POST_PROCESSING + 1;
		static constexpr std::size_t resourceImageCount = 0;
		static constexpr std::size_t samplerCount = 1;

		struct BufferBinding
		{
			vk::Buffer buffer;
			UINT64 offset = 0;
			UINT64 size = 0;
			std::size_t memoryIndex = 0;
			std::size_t linkedBlockCount = 0;
		};

		struct BlockBinding
		{
			std::size_t bufferIndex = 0;
			UINT64 offset = 0;
			UINT64 size = 0;
		};

		struct MemoryCounter
		{
			vk::DeviceMemory memory;
			std::size_t usageCount = 0;
		};

		struct ImageRequest : public vk::ImageCreateInfo
		{
			vk::MemoryPropertyFlags memoryProperties;
			ImageBinding& destImageBinding;
			ImageRequest( ImageBinding& destImageBinding, vk::MemoryPropertyFlags memoryProperties, vk::ImageCreateInfo createInfo ) :
				ImageRequest::ImageCreateInfo( createInfo ), memoryProperties( memoryProperties ), destImageBinding( destImageBinding ){}
		};

		struct BlockRequest : public vk::BufferCreateInfo
		{
			vk::MemoryPropertyFlags memoryProperties;
			BlockBinding& destBlockBinding;
			BlockRequest( BlockBinding& destBlockBinding, vk::MemoryPropertyFlags memoryProperties, vk::BufferCreateInfo createInfo ) :
				BlockRequest::BufferCreateInfo( createInfo ), memoryProperties( memoryProperties ), destBlockBinding( destBlockBinding ) {}
		};
		
		struct MapRange
		{
			vk::DeviceSize offset = 0;
			vk::DeviceSize size = 0;
			void* memoryPointer = nullptr;
		};

		std::vector<ImageBinding> main_render_destinations;

		BlockBinding hostTransferBlock;
		std::vector<BlockBinding> resourceBlocks;
		std::vector<ImageBinding> resourceImages;
		
		std::vector<MemoryCounter> memory;
		std::vector<BufferBinding> buffers;
		std::vector<MapRange> mapRanges;

		std::array<vk::Sampler,samplerCount> samplers;
		
		inline bool request_resource_memory();

		vk::Format find_format( std::initializer_list<vk::Format> formats,
							   vk::ImageUsageFlags imageFeatures, vk::FormatFeatureFlags formatFeatures, 
							   bool tilingOptimal, vk::Extent3D& wantedSize, vk::SampleCountFlagBits& wantedSampleCount ) const;

		bool create_managed_memory( const std::vector<BlockRequest>& inBufferRequests, const std::vector<ImageRequest>& inImageRequests ) noexcept( false );
		
	public:
		bool allocate_game_memory();
		bool has_render_destination_memory();

		struct RenderDestinationFormats
		{
			vk::Format color;
			vk::Format depth;
			vk::Format stencil;
		};
		RenderDestinationFormats select_main_render_formats();
		bool setup_main_render_destination();

		//return: true if format has changed
		void free_main_render_destination_memory();

		Block get_block( std::size_t blockIndex ) const;
		Block get_transfer_source_block() const;

		void* map_transfer_source_block();
		void unmap_transfer_source_block();
		void flush_transfer_source_block();

		const ImageBinding& get_image( RenderDestinationImages id ) const;

		vk::DeviceMemory get_unmanaged_memory( vk::DeviceSize size, vk::MemoryPropertyFlags memoryProperties, UINT32 typeIndexFilter = 0xFFFFFFFF );
	};
}
