#pragma once

#include <Defines.hpp>

#include <vulkan/vulkan.hpp>

namespace noxcain
{	
	class RenderPassDescription
	{
	private:
		class RenderpassObjectHandle
		{
		public:
			inline explicit operator bool() const
			{
				return index >= 0;
			}
		protected:
			INT32  index;

			friend class RenderPassDescription;
			RenderpassObjectHandle() : index( -1 )
			{
			}
		};

	public:
		
		class SubpassHandle final : public RenderpassObjectHandle
		{
		};

		class AttachmentHandle final : public RenderpassObjectHandle
		{
		};

		class FormatHandle final : public RenderpassObjectHandle
		{
		};

		class SampleCountHandle final : public RenderpassObjectHandle
		{
		};

		struct AttachmentHandleReference
		{
			AttachmentHandle attachment;
			vk::ImageLayout layout;
		};

		std::size_t get_attachment_count() const;
		FormatHandle get_format_handle( vk::Format initial_format = vk::Format::eUndefined );
		SampleCountHandle get_sample_count_handle( vk::SampleCountFlagBits initial_sample_count = vk::SampleCountFlagBits::e1 );

		AttachmentHandle add_attachment( FormatHandle format, SampleCountHandle sample_count,
										 vk::AttachmentLoadOp load_operation, vk::AttachmentStoreOp store_operation,
										 vk::AttachmentLoadOp stencil_load_operation, vk::AttachmentStoreOp stencil_store_operation,
										 vk::ImageLayout initial_image_layout, vk::ImageLayout final_image_layout );
		
		SubpassHandle add_subpass( const std::vector<AttachmentHandleReference>& input_attachment_references = {},
								   const std::vector<AttachmentHandleReference>& color_attachment_references = {},
								   const std::vector<AttachmentHandleReference>& resolve_attachment_references = {},
								   const std::vector<AttachmentHandle>& preserved_attachment_indices = {},
								   const AttachmentHandleReference depth_stencil_attachment_reference = { AttachmentHandle(), vk::ImageLayout::eUndefined } );

		bool add_dependency( SubpassHandle start_subpass, vk::PipelineStageFlagBits start_stage, vk::AccessFlagBits start_access,
							 SubpassHandle final_subpass, vk::PipelineStageFlagBits final_stage, vk::AccessFlagBits finbal_access );

		bool update_format( FormatHandle handle, vk::Format new_value );
		bool update_sample_count( SampleCountHandle handle, vk::SampleCountFlagBits new_value );

		vk::RenderPass get_render_pass();

		~RenderPassDescription();

		explicit operator bool() const;

	private:

		struct FormatBinding
		{
			vk::Format format;
			std::vector<AttachmentHandle> attachments;
		};

		struct SampleCountBinding
		{
			vk::SampleCountFlagBits sample_count;
			std::vector<AttachmentHandle> attachments;
		};

		struct SubpassBufferCollection
		{
			std::vector<vk::AttachmentReference> input_attachment_refs;
			std::vector<vk::AttachmentReference> color_attachment_refs;
			std::vector<vk::AttachmentReference> resolve_attachment_refs;
			std::vector<UINT32> preserved_attachment_indices;
			bool has_depth_stencil_attachment = false;
			vk::AttachmentReference depth_stencil_attachment_ref;
		};

		vk::RenderPass render_pass;
		std::vector<vk::AttachmentDescription> attachments;
		std::vector<vk::SubpassDependency> dependencies;
		std::vector<SubpassBufferCollection> subpass_data_buffers;
		
		std::vector<FormatBinding> format_bindings;
		std::vector<SampleCountBinding> sample_count_bindings;

		bool outdated = true;
	};
}