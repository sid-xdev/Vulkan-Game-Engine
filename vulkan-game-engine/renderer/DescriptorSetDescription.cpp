#include "DescriptorSetDescription.hpp"

#include <renderer/GameGraphicEngine.hpp>
#include <tools/ResultHandler.hpp>

noxcain::DescriptorSetLayoutDescription::DescriptorSetLayoutDescription()
{
}

bool noxcain::DescriptorSetLayoutDescription::add_binding_unlocked( UINT32 binding, vk::DescriptorType descriptor_type, UINT32 descriptor_count, vk::ShaderStageFlags stage_flags, const std::vector<std::shared_ptr<SamplerDescription>>& immutable_samplers )
{
	if( descriptor_count > 0 && ( immutable_samplers.empty() ||
		immutable_samplers.size() == descriptor_count && ( descriptor_type == vk::DescriptorType::eSampler || descriptor_type == vk::DescriptorType::eCombinedImageSampler ) ) )
	{
		bindings.emplace_back( binding, descriptor_type, descriptor_count, stage_flags, immutable_samplers );
		return true;
	}
	return false;
}

void noxcain::DescriptorSetLayoutDescription::create_layout()
{
}

std::shared_ptr<noxcain::DescriptorSetLayoutDescription> noxcain::DescriptorSetLayoutDescription::create_descriptor_set_layout_description()
{
	return std::shared_ptr<DescriptorSetLayoutDescription>( new DescriptorSetLayoutDescription() );
}

bool noxcain::DescriptorSetLayoutDescription::add_binding( UINT32 binding, vk::DescriptorType descriptor_type, UINT32 descriptor_count, vk::ShaderStageFlags stage_flags, const std::vector<std::shared_ptr<SamplerDescription>>& immutable_samplers )
{
	std::unique_lock lock( descriptor_set_layout_mutex );
	return add_binding_unlocked( binding, descriptor_type, descriptor_count, stage_flags, immutable_samplers );
}

bool noxcain::DescriptorSetLayoutDescription::add_binding( vk::DescriptorType descriptor_type, UINT32 descriptor_count, vk::ShaderStageFlags stage_flags, const std::vector<std::shared_ptr<SamplerDescription>>& immutable_samplers )
{
	std::unique_lock lock( descriptor_set_layout_mutex );
	UINT32 next_binding = bindings.empty() ? 0 : bindings.back().binding + 1;
	return add_binding_unlocked( next_binding, descriptor_type, descriptor_count, stage_flags, immutable_samplers );
}

vk::DescriptorSetLayout noxcain::DescriptorSetLayoutDescription::get_layout()
{
	std::shared_lock shared_lock( descriptor_set_layout_mutex );
	if( !layout )
	{
		shared_lock.unlock();
		std::unique_lock explicit_lock( descriptor_set_layout_mutex );
		if( !layout )
		{
			ResultHandler<vk::Result> r_handler( vk::Result::eSuccess );
			vk::Device device = GraphicEngine::get_device();
			if( device )
			{
				std::vector<vk::DescriptorSetLayoutBinding> real_bindings;
				std::vector<std::vector<vk::Sampler>> real_samplers;
				for( const auto& binding : bindings )
				{
					auto& current_samplers = real_samplers.emplace_back();
					for( auto& sampler_description : binding.immutable_samplers )
					{
						vk::Sampler real_sampler = sampler_description->get_sampler();
						if( !real_sampler )
						{
							//TODO error
							return vk::DescriptorSetLayout();
						}
						current_samplers.emplace_back( real_sampler );
					}

					real_bindings.emplace_back( binding.binding, binding.descriptor_type, binding.descriptor_count, binding.stage_flags, current_samplers.empty() ? nullptr : current_samplers.data() );
				}

				layout = r_handler << device.createDescriptorSetLayout( vk::DescriptorSetLayoutCreateInfo( flags, real_bindings.size(), real_bindings.empty() ? nullptr : real_bindings.data() ) );
				if( !r_handler.all_okay() )
				{
					return vk::DescriptorSetLayout();
				}
			}
			else
			{
				//TODO r_handler error no device ?
				return vk::DescriptorSetLayout();
			}
		}
		explicit_lock.unlock();
		shared_lock.lock();
	}
	return layout;
}

noxcain::DescriptorSetLayoutDescription::~DescriptorSetLayoutDescription()
{
	std::unique_lock lock( descriptor_set_layout_mutex );
	if( layout )
	{
		ResultHandler<vk::Result> r_handler( vk::Result::eSuccess );
		vk::Device device = GraphicEngine::get_device();
		if( device )
		{
			r_handler << device.waitIdle();
			if( r_handler.all_okay() )
			{
				device.destroyDescriptorSetLayout( layout );
			}
		}
	}
}

std::vector<vk::DescriptorPoolSize> noxcain::DescriptorSetLayoutDescription::get_type_count() const
{
	std::shared_lock lock( descriptor_set_layout_mutex );
	std::vector<vk::DescriptorPoolSize> type_counts;
	for( const auto& binding : bindings )
	{
		bool no_entry = true;
		for( auto& type_count : type_counts )
		{
			if( type_count.type == binding.descriptor_type )
			{
				type_count.descriptorCount += binding.descriptor_count;
				no_entry = false;
				break;
			}
		}
		if( no_entry )
		{
			type_counts.emplace_back( binding.descriptor_type, binding.descriptor_count );
		}
	}
	return type_counts;
}

std::pair<bool,vk::DescriptorType> noxcain::DescriptorSetLayoutDescription::get_binding_type( UINT32 binding ) const
{
	for( const auto& binding_entry : bindings )
	{
		if( binding_entry.binding == binding )
		{
			return std::make_pair( true, binding_entry.descriptor_type );
		}
	}
	return std::make_pair( false, vk::DescriptorType::eSampler );
}

std::shared_ptr<noxcain::SamplerDescription> noxcain::SamplerDescription::create_sampler_description( vk::SamplerCreateInfo create_info )
{
	return std::shared_ptr<SamplerDescription>( new SamplerDescription( create_info ) );
}

noxcain::SamplerDescription::~SamplerDescription()
{
	std::unique_lock lock( sampler_mutex );
	if( sampler )
	{
		vk::Device device = GraphicEngine::get_device();
		if( device )
		{
			ResultHandler<vk::Result> r_handler( vk::Result::eSuccess );
			r_handler << device.waitIdle();
			if( r_handler.all_okay() )
			{
				device.destroySampler( sampler );
			}
		}
	}
}

vk::Sampler noxcain::SamplerDescription::get_sampler()
{
	std::unique_lock lock( sampler_mutex );
	if( !sampler )
	{
		vk::Device device = GraphicEngine::get_device();
		if( device )
		{
			ResultHandler<vk::Result> r_handler( vk::Result::eSuccess );
			sampler = r_handler << device.createSampler( create_info );
			if( r_handler.all_okay() )
			{
				return sampler;
			}
		}
		return vk::Sampler();
	}
	return sampler;
}

noxcain::SamplerDescription::SamplerDescription( vk::SamplerCreateInfo info ) : create_info( info )
{
}
