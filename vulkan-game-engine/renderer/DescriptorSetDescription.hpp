#pragma once
#include <Defines.hpp>

#include <shared_mutex>
#include <vulkan/vulkan.hpp>


namespace noxcain
{
	class SamplerDescription
	{
	public:
		static std::shared_ptr<SamplerDescription> create_sampler_description( vk::SamplerCreateInfo create_info );
		~SamplerDescription();
		vk::Sampler get_sampler();
	private:
		SamplerDescription( const SamplerDescription& ) = delete;
		SamplerDescription& operator=( const SamplerDescription& ) = delete;
		SamplerDescription( vk::SamplerCreateInfo info );

		std::mutex sampler_mutex;
		vk::SamplerCreateInfo create_info;
		vk::Sampler sampler;
	};

	class DescriptorSetLayoutDescription
	{
	public:
		static std::shared_ptr<DescriptorSetLayoutDescription> create_descriptor_set_layout_description();
		bool add_binding( UINT32 binding, vk::DescriptorType descriptor_type, UINT32 descriptor_count, vk::ShaderStageFlags stage_flags, const std::vector<std::shared_ptr<SamplerDescription>>& immutable_samplers = {} );
		bool add_binding( vk::DescriptorType descriptor_type, UINT32 descriptor_count, vk::ShaderStageFlags stage_flags, const std::vector<std::shared_ptr<SamplerDescription>>& immutable_samplers = {} );
		vk::DescriptorSetLayout get_layout();

		~DescriptorSetLayoutDescription();
		
		std::vector<vk::DescriptorPoolSize> get_type_count() const;
		std::pair<bool, vk::DescriptorType> get_binding_type( UINT32 binding ) const;

	private:
		struct Binding
		{
			Binding( UINT32 binding_, vk::DescriptorType descriptor_type_, UINT32 descriptor_count_, vk::ShaderStageFlags stage_flags_, const std::vector<std::shared_ptr<SamplerDescription>>& immutable_samplers_ ) :
				binding( binding_ ), descriptor_type( descriptor_type_), descriptor_count( descriptor_count_ ), stage_flags( stage_flags_ ), immutable_samplers( immutable_samplers_ )
			{
			}
			UINT32 binding;
			vk::DescriptorType descriptor_type;
			UINT32 descriptor_count;
			vk::ShaderStageFlags stage_flags;
			std::vector<std::shared_ptr<SamplerDescription>> immutable_samplers;
		};
		mutable std::shared_mutex descriptor_set_layout_mutex;
		DescriptorSetLayoutDescription();
		DescriptorSetLayoutDescription( const DescriptorSetLayoutDescription& ) = delete;
		DescriptorSetLayoutDescription& operator=( const DescriptorSetLayoutDescription& ) = delete;
		std::vector<Binding> bindings;
		vk::DescriptorSetLayoutCreateFlags flags;
		vk::DescriptorSetLayout layout;

		bool add_binding_unlocked( UINT32 binding, vk::DescriptorType descriptor_type, UINT32 descriptor_count, vk::ShaderStageFlags stage_flags, const std::vector<std::shared_ptr<SamplerDescription>>& immutable_samplers = {} );
		void create_layout();
	};

	struct DescriptorSetDescription
	{
		vk::DescriptorSet set;
		std::shared_ptr<DescriptorSetLayoutDescription> layout;
	};
}