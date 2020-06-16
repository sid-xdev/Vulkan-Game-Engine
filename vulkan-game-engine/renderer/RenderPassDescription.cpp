#include "RenderPassDescription.hpp"

#include <renderer/GameGraphicEngine.hpp>
#include <tools/ResultHandler.hpp>

std::size_t noxcain::RenderPassDescription::get_attachment_count() const
{
	return attachments.size();
}

noxcain::RenderPassDescription::FormatHandle noxcain::RenderPassDescription::get_format_handle( vk::Format initial_format )
{
	FormatHandle new_handle;
	new_handle.index = format_bindings.size();
	format_bindings.emplace_back().format = initial_format;
	return new_handle;
}

noxcain::RenderPassDescription::SampleCountHandle noxcain::RenderPassDescription::get_sample_count_handle( vk::SampleCountFlagBits initial_sample_count )
{
	SampleCountHandle new_handle;
	new_handle.index = sample_count_bindings.size();
	sample_count_bindings.emplace_back().sample_count = initial_sample_count;
	return new_handle;
}

noxcain::RenderPassDescription::AttachmentHandle noxcain::RenderPassDescription::add_attachment( FormatHandle format, SampleCountHandle sample_count, vk::AttachmentLoadOp load_operation, vk::AttachmentStoreOp store_operation, vk::AttachmentLoadOp stencil_load_operation, vk::AttachmentStoreOp stencil_store_operation, vk::ImageLayout initial_image_layout, vk::ImageLayout final_image_layout )
{
	if( format && sample_count )
	{
		AttachmentHandle new_handle;
		new_handle.index = attachments.size();

		auto& format_binding = format_bindings[format.index];
		auto& sample_count_binding = sample_count_bindings[sample_count.index];

		attachments.emplace_back( vk::AttachmentDescriptionFlags(), format_binding.format, sample_count_binding.sample_count,
								  load_operation, store_operation, stencil_load_operation, stencil_store_operation,
								  initial_image_layout, final_image_layout );

		format_binding.attachments.push_back( new_handle );
		sample_count_binding.attachments.push_back( new_handle );
		return new_handle;
	}
	return AttachmentHandle();
}

noxcain::RenderPassDescription::SubpassHandle noxcain::RenderPassDescription::add_subpass( const std::vector<AttachmentHandleReference>& input_attachment_references,
																						   const std::vector<AttachmentHandleReference>& color_attachment_references,
																						   const std::vector<AttachmentHandleReference>& resolve_attachment_references,
																						   const std::vector<AttachmentHandle>& preserved_attachment_indices,
																						   const AttachmentHandleReference depth_stencil_attachment_reference )
{
	bool invalid_handle = false;
	
	if( resolve_attachment_references.empty() || resolve_attachment_references.size() == color_attachment_references.size() )
	{
		auto& new_collection = subpass_data_buffers.emplace_back();

		for( const auto& input_ref : input_attachment_references )
		{
			if( !input_ref.attachment )
			{
				//TODO error
				return SubpassHandle();
			}
			new_collection.input_attachment_refs.emplace_back( input_ref.attachment.index, input_ref.layout );
		}

		for( const auto& color_ref : color_attachment_references )
		{
			if( !color_ref.attachment )
			{
				//TODO error
				return SubpassHandle();
			}
			new_collection.color_attachment_refs.emplace_back( color_ref.attachment.index, color_ref.layout );
		}

		for( const auto& resolve_ref : resolve_attachment_references )
		{
			if( !resolve_ref.attachment )
			{
				//TODO error
				return SubpassHandle();
			}
			new_collection.resolve_attachment_refs.emplace_back( resolve_ref.attachment.index, resolve_ref.layout );
		}

		for( const auto& preserv_ref : preserved_attachment_indices )
		{
			if( !preserv_ref )
			{
				//TODO error
				return SubpassHandle();
			}
			new_collection.preserved_attachment_indices.emplace_back( preserv_ref.index );
		}

		new_collection.has_depth_stencil_attachment = static_cast<bool>( depth_stencil_attachment_reference.attachment );
		if( new_collection.has_depth_stencil_attachment )
		{
			new_collection.depth_stencil_attachment_ref = vk::AttachmentReference( depth_stencil_attachment_reference.attachment.index, depth_stencil_attachment_reference.layout );
		}

		SubpassHandle new_handle;
		new_handle.index = subpass_data_buffers.size() - 1;
		return new_handle;
	}
	return SubpassHandle();
}

bool noxcain::RenderPassDescription::add_dependency( SubpassHandle start_subpass, vk::PipelineStageFlagBits start_stage, vk::AccessFlagBits start_access, SubpassHandle final_subpass, vk::PipelineStageFlagBits final_stage, vk::AccessFlagBits final_access )
{
	if( start_subpass && final_subpass )
	{
		dependencies.emplace_back( start_subpass.index, final_subpass.index, start_stage, final_stage, start_access, final_access );
		return true;
	}
	//TODO add message
	return false;
}

bool noxcain::RenderPassDescription::update_format( FormatHandle handle, vk::Format new_value )
{
	if( handle )
	{
		if( new_value != format_bindings[handle.index].format )
		{
			outdated = true;
			format_bindings[handle.index].format = new_value;
			for( auto& att : format_bindings[handle.index].attachments )
			{
				attachments[att.index].setFormat( new_value );
			}
		}
		return true;
	}
	return false;
}

bool noxcain::RenderPassDescription::update_sample_count( SampleCountHandle handle, vk::SampleCountFlagBits new_value )
{
	if( handle )
	{
		if( new_value != sample_count_bindings[handle.index].sample_count )
		{
			outdated = true;
			sample_count_bindings[handle.index].sample_count = new_value;
			for( auto& att : sample_count_bindings[handle.index].attachments )
			{
				attachments[att.index].setSamples( new_value );
			}
		}
		return true;
	}
	return false;
}

vk::RenderPass noxcain::RenderPassDescription::get_render_pass()
{
	ResultHandler<vk::Result> result_handler( vk::Result::eSuccess );
	if( outdated || !render_pass )
	{
		GraphicEngine::get_device().waitIdle();
		GraphicEngine::get_device().destroy( render_pass );

		std::vector<vk::SubpassDescription> subpasses;
		subpasses.reserve( subpass_data_buffers.size() );
		for( const auto& subpass : subpass_data_buffers )
		{
			subpasses.emplace_back( 
				vk::SubpassDescription(
					vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, 
					subpass.input_attachment_refs.size(), subpass.input_attachment_refs.empty() ? nullptr : subpass.input_attachment_refs.data(),
					subpass.color_attachment_refs.size(), subpass.color_attachment_refs.empty() ? nullptr : subpass.color_attachment_refs.data(),
					subpass.resolve_attachment_refs.empty() ? nullptr : subpass.resolve_attachment_refs.data(),
					subpass.has_depth_stencil_attachment ? &subpass.depth_stencil_attachment_ref : nullptr,
					subpass.preserved_attachment_indices.size(), subpass.preserved_attachment_indices.empty() ? nullptr : subpass.preserved_attachment_indices.data() ) );
		}

		render_pass = result_handler << GraphicEngine::get_device().createRenderPass( vk::RenderPassCreateInfo( vk::RenderPassCreateFlags(),
																												attachments.size(), attachments.data(),
																												subpasses.size(), subpasses.data(),
																												dependencies.size(), dependencies.data() ) );
	}
	
	if( result_handler.all_okay() )
	{
		outdated = false;
		return render_pass;
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
			GraphicEngine::get_device().destroyRenderPass( render_pass );
		}
	}
}

noxcain::RenderPassDescription::operator bool() const
{
	return static_cast<bool>( render_pass );
}
