#pragma once
#include <Defines.hpp>
#include <renderer/GraphicEngineConstants.hpp>
#include <renderer/DescriptorSetDescription.hpp>

#include <array>
#include <vulkan/vulkan.hpp>


namespace noxcain
{
	enum class BasicDescriptorSets : std::size_t
	{
		SHADING_INPUT_ATTACHMENTS = 0,
		GLYPHS,
		FINALIZED_MASTER_TEXTURE
	};
	constexpr static std::size_t BASIC_DESCRIPTOR_SET_COUNT = static_cast<std::size_t>( BasicDescriptorSets::FINALIZED_MASTER_TEXTURE ) + 1;

	enum class DescriptorSetLayouts : std::size_t
	{
		GLYPH = 0,
		INPUT_ATTACHMENT_3,
		FIXED_SAMPLED_TEXTURE_1
	};
	constexpr static std::size_t DESCRIPTOR_SET_LAYOUT_COUNT = static_cast<std::size_t>( DescriptorSetLayouts::FIXED_SAMPLED_TEXTURE_1 ) + 1;
	
	class DescriptorSetManager
	{
	public:
		enum class DescriptorUpdateInfoTypes
		{
			IMAGE_INFO,
			BUFFER_INFO,
			BUFFER_VIEW,
			EXTERNAL
		};

		struct DescriptorUpdate
		{
			UINT32 binding = 0;
			UINT32 start_element = 0;
			DescriptorUpdateInfoTypes info_type;
			UINT32 descriptor_count = 0;
			void* descriptor_data = 0;
			DescriptorUpdate( UINT32 binding_, UINT32 start_element_, DescriptorUpdateInfoTypes info_type_, UINT32 descriptor_count_, void* descriptor_data_ ) :
				binding( binding_ ), start_element( start_element_ ), info_type( info_type_ ), descriptor_count( descriptor_count_ ), descriptor_data( descriptor_data_ )
			{
			}
		};
		

		struct BasicDescriptorSetUpdate
		{
			BasicDescriptorSets set = BasicDescriptorSets::SHADING_INPUT_ATTACHMENTS;
			std::vector<DescriptorUpdate> updates;
		};

		bool update_basic_sets( const std::vector<BasicDescriptorSetUpdate>& updates );

		DescriptorSetManager( const DescriptorSetManager& ) = delete;
		DescriptorSetManager& operator=( const DescriptorSetManager& ) = delete;
		DescriptorSetManager();
		~DescriptorSetManager();

		vk::DescriptorSetLayout get_layout( DescriptorSetLayouts layout_id );
		vk::DescriptorSet get_basic_set( BasicDescriptorSets set_id );

		//void update();
	private:
		void create_descriptor_sets();

		std::shared_mutex descriptor_set_mutex;
		vk::DescriptorPool pool;
		std::vector<vk::DescriptorPoolSize> pool_sizes;

		std::shared_ptr<SamplerDescription> fixed_finalize_sampler;

		std::array<DescriptorSetDescription, BASIC_DESCRIPTOR_SET_COUNT> basic_descriptor_sets;
		std::array<std::shared_ptr<DescriptorSetLayoutDescription>, DESCRIPTOR_SET_LAYOUT_COUNT> descriptor_set_layouts;
	};
}