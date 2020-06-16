#include "DescriptorSetManager.hpp"

#include <renderer/GameGraphicEngine.hpp>

#include <resources/GameResourceEngine.hpp>
#include <resources/FontResource.hpp>

#include <tools/ResultHandler.hpp>

bool noxcain::DescriptorSetManager::update_basic_sets( const std::vector<BasicDescriptorSetUpdate>& updates )
{
	std::vector<vk::WriteDescriptorSet> writes;

	std::unique_lock lock( descriptor_set_mutex );

	for( const auto& basic_updates : updates )
	{
		
		if( !basic_descriptor_sets[static_cast<std::size_t>( basic_updates.set )].set )
		{
			create_descriptor_sets();
			if( !basic_descriptor_sets[static_cast<std::size_t>( basic_updates.set )].set )
			{
				return false;
			}
		}
		
		const auto current_set = basic_descriptor_sets[static_cast<std::size_t>( basic_updates.set )].set;
		const auto current_layout = basic_descriptor_sets[static_cast<std::size_t>( basic_updates.set )].layout;
		writes.reserve( writes.size() + basic_updates.updates.size() );

		for( const auto& update : basic_updates.updates )
		{
			auto [result, current_type] = current_layout->get_binding_type( update.binding );
			if( result )
			{
				auto& current_write = writes.emplace_back();

				current_write.dstSet = current_set;
				current_write.dstBinding = update.binding;
				current_write.descriptorType = current_type;
				current_write.dstArrayElement = update.start_element;
				current_write.descriptorCount = update.descriptor_count;

				current_write.pImageInfo = nullptr;
				current_write.pBufferInfo = nullptr;
				current_write.pTexelBufferView = nullptr;

				switch( update.info_type )
				{
					case DescriptorUpdateInfoTypes::BUFFER_INFO:
					{
						current_write.pBufferInfo = static_cast<vk::DescriptorBufferInfo*>( update.descriptor_data );
						break;
					}

					case DescriptorUpdateInfoTypes::IMAGE_INFO:
					{
						current_write.pImageInfo = static_cast<vk::DescriptorImageInfo*>( update.descriptor_data );
						break;
					}

					case DescriptorUpdateInfoTypes::BUFFER_VIEW:
					{
						current_write.pTexelBufferView = static_cast<vk::BufferView*>( update.descriptor_data );
						break;
					}

					default:
					{
						break;
					}
				}
			}
		}
	}

	vk::Device device = GraphicEngine::get_device();
	if( device )
	{
		device.updateDescriptorSets( writes, {} );
		return true;
	}
	return false;
}

noxcain::DescriptorSetManager::DescriptorSetManager()
{
	fixed_finalize_sampler = SamplerDescription::create_sampler_description( vk::SamplerCreateInfo(
		vk::SamplerCreateFlags(), vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest,
		vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0.0F,
		VK_FALSE, 0.0F, VK_FALSE, vk::CompareOp::eNever, 0.0F, 0.0F, vk::BorderColor::eFloatOpaqueBlack, VK_FALSE ) );

	for( auto& descriptor_set : descriptor_set_layouts )
	{
		descriptor_set = DescriptorSetLayoutDescription::create_descriptor_set_layout_description();
	}

	const auto& resource_limits = ResourceEngine::get_engine().get_resource_limits();
	descriptor_set_layouts[( std::size_t )DescriptorSetLayouts::GLYPH]->add_binding( vk::DescriptorType::eStorageBuffer, resource_limits.glyph_count, vk::ShaderStageFlagBits::eFragment );
	descriptor_set_layouts[( std::size_t )DescriptorSetLayouts::GLYPH]->add_binding( vk::DescriptorType::eStorageBuffer, resource_limits.glyph_count, vk::ShaderStageFlagBits::eFragment );

	descriptor_set_layouts[( std::size_t )DescriptorSetLayouts::INPUT_ATTACHMENT_3]->add_binding( vk::DescriptorType::eInputAttachment, 1, vk::ShaderStageFlagBits::eFragment );
	descriptor_set_layouts[( std::size_t )DescriptorSetLayouts::INPUT_ATTACHMENT_3]->add_binding( vk::DescriptorType::eInputAttachment, 1, vk::ShaderStageFlagBits::eFragment );
	descriptor_set_layouts[( std::size_t )DescriptorSetLayouts::INPUT_ATTACHMENT_3]->add_binding( vk::DescriptorType::eInputAttachment, 1, vk::ShaderStageFlagBits::eFragment );

	descriptor_set_layouts[( std::size_t )DescriptorSetLayouts::FIXED_SAMPLED_TEXTURE_1]->add_binding( vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, { fixed_finalize_sampler } );

	basic_descriptor_sets[( std::size_t )BasicDescriptorSets::SHADING_INPUT_ATTACHMENTS].layout = descriptor_set_layouts[( std::size_t )DescriptorSetLayouts::INPUT_ATTACHMENT_3];
	basic_descriptor_sets[( std::size_t )BasicDescriptorSets::FINALIZED_MASTER_TEXTURE].layout = descriptor_set_layouts[( std::size_t )DescriptorSetLayouts::FIXED_SAMPLED_TEXTURE_1];
	basic_descriptor_sets[( std::size_t )BasicDescriptorSets::GLYPHS].layout = descriptor_set_layouts[( std::size_t )DescriptorSetLayouts::GLYPH];

	for( const auto& basic_descriptor_set : basic_descriptor_sets )
	{
		const auto current_type_counts = basic_descriptor_set.layout->get_type_count();
		bool no_entry = true;
		for( const auto& current_type_count : current_type_counts )
		{
			for( auto& found_type_count : pool_sizes )
			{
				if( found_type_count.type == current_type_count.type )
				{
					found_type_count.descriptorCount += current_type_count.descriptorCount;
					no_entry = false;
					break;
				}
			}
			if( no_entry )
			{
				pool_sizes.emplace_back( current_type_count.type, current_type_count.descriptorCount );
			}
		}
	}
}	

noxcain::DescriptorSetManager::~DescriptorSetManager()
{
	const vk::Device& device = GraphicEngine::get_device();
	if( device )
	{
		ResultHandler<vk::Result> r_handle( vk::Result::eSuccess );
		r_handle << device.waitIdle();
		if( r_handle.all_okay() )
		{
			device.destroyDescriptorPool( pool );
		}
	}
}

vk::DescriptorSetLayout noxcain::DescriptorSetManager::get_layout( DescriptorSetLayouts layout_id )
{
	std::shared_lock shared_lock( descriptor_set_mutex );
	return descriptor_set_layouts[static_cast<std::size_t>( layout_id )]->get_layout();
}

vk::DescriptorSet noxcain::DescriptorSetManager::get_basic_set( BasicDescriptorSets set )
{
	std::shared_lock shared_lock( descriptor_set_mutex );

	if( !basic_descriptor_sets[static_cast<std::size_t>( set )].set )
	{
		shared_lock.unlock();
		std::unique_lock exclusive_lock( descriptor_set_mutex );
		create_descriptor_sets();
		exclusive_lock.unlock();
		shared_lock.lock();
	}
	return basic_descriptor_sets[static_cast<std::size_t>( set )].set;
}

void noxcain::DescriptorSetManager::create_descriptor_sets()
{
	if( !pool )
	{
		vk::Device device = GraphicEngine::get_device();
		if( !device )
		{
			//TODO add error 
			return;
		}

		ResultHandler<vk::Result> r_handler( vk::Result::eSuccess );
		pool = r_handler << device.createDescriptorPool( vk::DescriptorPoolCreateInfo( 
			vk::DescriptorPoolCreateFlags(), basic_descriptor_sets.size(), pool_sizes.size(), pool_sizes.data() ) );

		if( !r_handler.all_okay() )
		{
			return;
		}

		std::vector<vk::DescriptorSetLayout> layouts;
		for( auto& basic_descriptor_set : basic_descriptor_sets )
		{
			layouts.emplace_back( basic_descriptor_set.layout->get_layout() );
		}

		std::vector<vk::DescriptorSet> sets = r_handler << device.allocateDescriptorSets( vk::DescriptorSetAllocateInfo( pool, layouts.size(), layouts.data() ) );

		if( !r_handler.all_okay() )
		{
			return;
		}

		std::size_t set_offset = 0;

		for( std::size_t index = 0; index < basic_descriptor_sets.size(); ++index )
		{
			basic_descriptor_sets[index].set = sets[set_offset + index];
		}
		set_offset += basic_descriptor_sets.size();
	}
}
