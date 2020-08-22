#include "RenderPassDescription.hpp"

#include <renderer/GameGraphicEngine.hpp>
#include <tools/ResultHandler.hpp>

std::size_t noxcain::RenderPassDescription::get_attachment_count() const
{
	if( outdated )
	{
		return 0;
	}
	return last_attachment_count;
}

noxcain::RenderPassDescription::FormatHandle noxcain::RenderPassDescription::get_format_handle( vk::Format initial_format )
{
	FormatHandle new_handle;
	new_handle.index = formats.size();
	formats.emplace_back( initial_format );
	return new_handle;
}

noxcain::RenderPassDescription::SampleCountHandle noxcain::RenderPassDescription::get_sample_count_handle( vk::SampleCountFlagBits initial_sample_count )
{
	SampleCountHandle new_handle;
	new_handle.index = sample_counts.size();
	sample_counts.emplace_back( initial_sample_count );
	return new_handle;
}

noxcain::RenderPassDescription::AttachmentHandle noxcain::RenderPassDescription::add_attachment( 
	FormatHandle format, SampleCountHandle sample_count,
	vk::AttachmentDescriptionFlags description_flags,
	vk::AttachmentLoadOp load_operation, vk::AttachmentStoreOp store_operation,
	vk::AttachmentLoadOp stencil_load_operation, vk::AttachmentStoreOp stencil_store_operation,
	vk::ImageLayout initial_image_layout, vk::ImageLayout final_image_layout, SamplingMode sampling_mode )
{
	if( format && sample_count )
	{
		AttachmentHandle new_handle;
		new_handle.index = attachment_descriptions.size();

		AttachmentDescription description = { format, sample_count, description_flags,
											  load_operation, store_operation, stencil_load_operation, stencil_store_operation,
											  initial_image_layout, final_image_layout, sampling_mode, 0 };
		attachment_descriptions.emplace_back( description );
		return new_handle;
	}
	return AttachmentHandle();
}

noxcain::RenderPassDescription::SubpassHandle noxcain::RenderPassDescription::add_subpass( std::vector<AttachmentHandleReference> input_attachment_references,
																						   std::vector<AttachmentHandleReference> color_attachment_references,
																						   std::vector<AttachmentHandleReference> resolve_attachment_references,
																						   std::vector<AttachmentHandle> preserved_attachment,
																						   AttachmentHandleReference depth_stencil_attachment_reference,
																						   SamplingMode sampling_mode )
{
	if( resolve_attachment_references.empty() || resolve_attachment_references.size() == color_attachment_references.size() )
	{
		SubpassHandle new_handle;
		new_handle.index = subpass_data_buffers.size();
		auto& new_collection = subpass_data_buffers.emplace_back();

		new_collection.input_attachment_refs.swap( input_attachment_references );
		new_collection.color_attachment_refs.swap( color_attachment_references );
		new_collection.resolve_attachment_refs.swap( resolve_attachment_references );
		new_collection.preserved_attachments.swap( preserved_attachment );
		new_collection.depth_stencil_attachment_ref = depth_stencil_attachment_reference;
		new_collection.sampling_mode = sampling_mode;
		return new_handle;
	}
	return SubpassHandle();
}

bool noxcain::RenderPassDescription::add_dependency( vk::DependencyFlags flags,
													 SubpassHandle start_subpass, vk::PipelineStageFlagBits start_stage, vk::AccessFlagBits start_access,
													 SubpassHandle final_subpass, vk::PipelineStageFlagBits final_stage, vk::AccessFlagBits final_access,
													 SamplingMode sampling_mode )
{
	if( start_subpass && final_subpass )
	{
		DependencyDescription description = { flags, start_subpass, start_stage, start_access, final_subpass, final_stage, final_access, sampling_mode };
		dependency_descriptions.emplace_back( description );
		return true;
	}
	return false;
}

bool noxcain::RenderPassDescription::update_format( FormatHandle handle, vk::Format new_value )
{
	if( handle )
	{
		if( new_value != formats[handle.index] )
		{
			outdated = true;
			formats[handle.index] = new_value;
		}
		return true;
	}
	return false;
}

bool noxcain::RenderPassDescription::update_sample_count( SampleCountHandle handle, vk::SampleCountFlagBits new_value )
{
	if( handle )
	{
		if( new_value != sample_counts[handle.index] )
		{
			outdated = true;
			sample_counts[handle.index] = new_value;
		}
		return true;
	}
	return false;
}

void noxcain::RenderPassDescription::set_decider( SampleCountHandle handle )
{
	decider = handle;
}

vk::RenderPass noxcain::RenderPassDescription::get_render_pass()
{
	ResultHandler<vk::Result> result_handler( vk::Result::eSuccess );
	if( outdated || !last_render_pass )
	{
		GraphicEngine::get_device().waitIdle();
		GraphicEngine::get_device().destroy( last_render_pass );
		last_render_pass = vk::RenderPass();

		last_attachment_count = 0;
		std::vector<vk::AttachmentDescription> vulkan_attachment_descriptions;
		vulkan_attachment_descriptions.reserve( attachment_descriptions.size() );
		for( auto& attachment_description : attachment_descriptions )
		{
			if( attachment_description.sampling_mode == SamplingMode::BOTH ||
				( ( !decider || sample_counts[decider.index] == vk::SampleCountFlagBits::e1 ) && attachment_description.sampling_mode == SamplingMode::SINGLE ) ||
				( decider && sample_counts[decider.index] != vk::SampleCountFlagBits::e1 && attachment_description.sampling_mode == SamplingMode::MULTI ) )
			{
				attachment_description.attachment_index = vulkan_attachment_descriptions.size();
				auto& vulkan_attachment_description = vulkan_attachment_descriptions.emplace_back();

				vulkan_attachment_description.flags = attachment_description.flags;
				vulkan_attachment_description.format = formats[attachment_description.format.index];
				vulkan_attachment_description.samples = sample_counts[attachment_description.sample_count.index];
				vulkan_attachment_description.initialLayout = attachment_description.initial_image_layout;
				vulkan_attachment_description.finalLayout = attachment_description.final_image_layout;
				vulkan_attachment_description.loadOp = attachment_description.load_operation;
				vulkan_attachment_description.storeOp = attachment_description.store_operation;
				vulkan_attachment_description.stencilLoadOp = attachment_description.stencil_load_operation;
				vulkan_attachment_description.stencilStoreOp = attachment_description.stencil_store_operation;

				++last_attachment_count;
			}
		}

		std::vector<vk::SubpassDescription> vulkan_subpass_descriptions;
		std::vector<std::vector<vk::AttachmentReference>> vulkan_input_attachments;
		std::vector<std::vector<vk::AttachmentReference>> vulkan_color_attachments;
		std::vector<std::vector<vk::AttachmentReference>> vulkan_resolve_attachments;
		std::vector<vk::AttachmentReference> vulkan_depth_stencil_attachments;
		std::vector<std::vector<UINT32>> vulkan_preserved_attachments;
		vulkan_subpass_descriptions.reserve( subpass_data_buffers.size() );
		vulkan_input_attachments.reserve( subpass_data_buffers.size() );
		vulkan_color_attachments.reserve( subpass_data_buffers.size() );
		vulkan_resolve_attachments.reserve( subpass_data_buffers.size() );
		vulkan_depth_stencil_attachments.reserve( subpass_data_buffers.size() );
		vulkan_preserved_attachments.reserve( subpass_data_buffers.size() );

		for( auto& subpass_description : subpass_data_buffers )
		{
			if( subpass_description.sampling_mode == SamplingMode::BOTH ||
				( ( !decider || sample_counts[decider.index] == vk::SampleCountFlagBits::e1 ) && subpass_description.sampling_mode == SamplingMode::SINGLE ) ||
				( decider && sample_counts[decider.index] != vk::SampleCountFlagBits::e1 && subpass_description.sampling_mode == SamplingMode::MULTI ) )
			{
				subpass_description.subpass_index = vulkan_subpass_descriptions.size();
				auto& vulkan_subpass_description = vulkan_subpass_descriptions.emplace_back();
				vulkan_subpass_description.flags = vk::SubpassDescriptionFlags();
				vulkan_subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
				
				auto& vulkan_input = vulkan_input_attachments.emplace_back();
				vulkan_input.reserve( subpass_description.input_attachment_refs.size() );
				for( const auto& input_ref : subpass_description.input_attachment_refs )
				{
					vulkan_input.emplace_back( vk::AttachmentReference( attachment_descriptions[input_ref.attachment.index].attachment_index, input_ref.layout ) );
				}
				vulkan_subpass_description.inputAttachmentCount = vulkan_input.size();
				vulkan_subpass_description.pInputAttachments = vulkan_input.empty() ? nullptr : vulkan_input.data();

				auto& vulkan_color = vulkan_color_attachments.emplace_back();
				vulkan_color.reserve( subpass_description.color_attachment_refs.size() );
				for( const auto& color_ref : subpass_description.color_attachment_refs )
				{
					vulkan_color.emplace_back( vk::AttachmentReference( attachment_descriptions[color_ref.attachment.index].attachment_index, color_ref.layout ) );
				}
				vulkan_subpass_description.colorAttachmentCount = vulkan_color.size();
				vulkan_subpass_description.pColorAttachments = vulkan_color.empty() ? nullptr : vulkan_color.data();

				auto& vulkan_resolve = vulkan_resolve_attachments.emplace_back();
				vulkan_resolve.reserve( subpass_description.resolve_attachment_refs.size() );
				for( const auto& resolve_ref : subpass_description.resolve_attachment_refs )
				{
					vulkan_resolve.emplace_back( vk::AttachmentReference( attachment_descriptions[resolve_ref.attachment.index].attachment_index, resolve_ref.layout ) );
				}
				vulkan_subpass_description.pResolveAttachments = vulkan_resolve.empty() ? nullptr : vulkan_resolve.data();

				auto& vulkan_preserved = vulkan_preserved_attachments.emplace_back();
				vulkan_preserved.reserve( subpass_description.preserved_attachments.size() );
				for( const auto& preserved_ref : subpass_description.preserved_attachments )
				{
					vulkan_preserved.emplace_back( attachment_descriptions[preserved_ref.index].attachment_index );
				}
				vulkan_subpass_description.preserveAttachmentCount = vulkan_preserved.size();
				vulkan_subpass_description.pPreserveAttachments = vulkan_preserved.empty() ? nullptr : vulkan_preserved.data();

				auto& vulkan_depth_stencil = vulkan_depth_stencil_attachments.emplace_back();
				bool has_stencil_depth_attachment = ( subpass_description.depth_stencil_attachment_ref.attachment.index >= 0 );
				if( has_stencil_depth_attachment )
				{
					vulkan_depth_stencil = vk::AttachmentReference( attachment_descriptions[subpass_description.depth_stencil_attachment_ref.attachment.index].attachment_index,
																	subpass_description.depth_stencil_attachment_ref.layout );
					vulkan_subpass_description.pDepthStencilAttachment = &vulkan_depth_stencil;
				}
				else
				{
					vulkan_subpass_description.pDepthStencilAttachment = nullptr;
				}
			}
		}

		std::vector<vk::SubpassDependency> vulkan_dependency_descriptions;
		vulkan_dependency_descriptions.reserve( dependency_descriptions.size() );
		for( const auto& dependency_description : dependency_descriptions )
		{
			if( dependency_description.sampling_mode == SamplingMode::BOTH ||
				( ( !decider || sample_counts[decider.index] == vk::SampleCountFlagBits::e1 ) && dependency_description.sampling_mode == SamplingMode::SINGLE ) ||
				( decider && sample_counts[decider.index] != vk::SampleCountFlagBits::e1 && dependency_description.sampling_mode == SamplingMode::MULTI ) )
			{
				auto& vulkan_dependency_description = vulkan_dependency_descriptions.emplace_back();

				vulkan_dependency_description.dependencyFlags;
				vulkan_dependency_description.srcSubpass = subpass_data_buffers[dependency_description.start_subpass.index].subpass_index;
				vulkan_dependency_description.srcStageMask = dependency_description.start_stage;
				vulkan_dependency_description.srcAccessMask = dependency_description.start_access;
				vulkan_dependency_description.dstSubpass = subpass_data_buffers[dependency_description.final_subpass.index].subpass_index;
				vulkan_dependency_description.dstStageMask = dependency_description.final_stage;
				vulkan_dependency_description.dstAccessMask = dependency_description.final_access;
			}
		}

		last_render_pass = result_handler << GraphicEngine::get_device().createRenderPass(
			vk::RenderPassCreateInfo(
				vk::RenderPassCreateFlags(),
				vulkan_attachment_descriptions.size(), vulkan_attachment_descriptions.empty() ? nullptr : vulkan_attachment_descriptions.data(),
				vulkan_subpass_descriptions.size(), vulkan_subpass_descriptions.empty() ? nullptr : vulkan_subpass_descriptions.data(),
				vulkan_dependency_descriptions.size(), vulkan_dependency_descriptions.empty() ? nullptr : vulkan_dependency_descriptions.data() ) );
	}
	
	if( result_handler.all_okay() )
	{
		outdated = false;
		return last_render_pass;
	}
	return vk::RenderPass();
}

noxcain::RenderPassDescription::~RenderPassDescription()
{	
	vk::Device device = GraphicEngine::get_device();
	if( device )
	{
		ResultHandler r_handler( vk::Result::eSuccess );
		r_handler << GraphicEngine::get_device().waitIdle();
		if( r_handler.all_okay() )
		{
			GraphicEngine::get_device().destroyRenderPass( last_render_pass );
		}
	}
}

noxcain::RenderPassDescription::operator bool() const
{
	return static_cast<bool>( last_render_pass );
}
