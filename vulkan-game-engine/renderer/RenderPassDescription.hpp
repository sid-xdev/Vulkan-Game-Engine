#pragma once

#include <Defines.hpp>

#include <vulkan/vulkan.hpp>

namespace noxcain
{	
	class RenderPassDescription
	{
	private:
		template<typename TAG>
		class RenderpassObjectHandle
		{
		public:
			friend class RenderPassDescription;
			RenderpassObjectHandle() : index( -1 )
			{
			}
			inline explicit operator bool() const
			{
				return index >= 0;
			}
		private:
			INT32  index;
		};

	public:
		
		enum class SamplingMode
		{
			BOTH,
			SINGLE,
			MULTI,
		};

		using SubpassHandle = RenderpassObjectHandle<struct SubpassTag>;
		using AttachmentHandle = RenderpassObjectHandle<struct AttachmentTag>;
		using FormatHandle = RenderpassObjectHandle<struct FormatTag>;
		using SampleCountHandle = RenderpassObjectHandle<struct SampleCountTag>;

		struct AttachmentHandleReference
		{
			AttachmentHandle attachment;
			vk::ImageLayout layout;
		};

		std::size_t get_attachment_count() const;
		FormatHandle get_format_handle( vk::Format initial_format = vk::Format::eUndefined );
		SampleCountHandle get_sample_count_handle( vk::SampleCountFlagBits initial_sample_count = vk::SampleCountFlagBits::e1 );

		AttachmentHandle add_attachment( FormatHandle format, SampleCountHandle sample_count,
										 vk::AttachmentDescriptionFlags description_flags,
										 vk::AttachmentLoadOp load_operation, vk::AttachmentStoreOp store_operation,
										 vk::AttachmentLoadOp stencil_load_operation, vk::AttachmentStoreOp stencil_store_operation,
										 vk::ImageLayout initial_image_layout, vk::ImageLayout final_image_layout,
										 SamplingMode sampling_mode = SamplingMode::BOTH );
		
		SubpassHandle add_subpass( std::vector<AttachmentHandleReference> input_attachment_references = {},
								   std::vector<AttachmentHandleReference> color_attachment_references = {},
								   std::vector<AttachmentHandleReference> resolve_attachment_references = {},
								   std::vector<AttachmentHandle> preserved_attachment_indices = {},
								   AttachmentHandleReference depth_stencil_attachment_reference = { AttachmentHandle(), vk::ImageLayout::eUndefined },
								   SamplingMode sampling_mode = SamplingMode::BOTH );

		bool add_dependency( vk::DependencyFlags flags,
							 SubpassHandle start_subpass, vk::PipelineStageFlagBits start_stage, vk::AccessFlagBits start_access,
							 SubpassHandle final_subpass, vk::PipelineStageFlagBits final_stage, vk::AccessFlagBits final_access,
							 SamplingMode sampling_mode = SamplingMode::BOTH );

		bool update_format( FormatHandle handle, vk::Format new_value );
		bool update_sample_count( SampleCountHandle handle, vk::SampleCountFlagBits new_value );
		void set_decider( SampleCountHandle handle );

		vk::RenderPass get_render_pass();

		~RenderPassDescription();

		explicit operator bool() const;

	private:

		//the sample count which decides if single or multi sample
		SampleCountHandle decider;

		struct AttachmentDescription
		{
			FormatHandle format;
			SampleCountHandle sample_count;

			vk::AttachmentDescriptionFlags flags;
			vk::AttachmentLoadOp load_operation;
			vk::AttachmentStoreOp store_operation;
			vk::AttachmentLoadOp stencil_load_operation;
			vk::AttachmentStoreOp stencil_store_operation;
			vk::ImageLayout initial_image_layout;
			vk::ImageLayout final_image_layout;
			SamplingMode sampling_mode;

			UINT32 attachment_index = 0;
		};
		std::vector<AttachmentDescription> attachment_descriptions;

		struct SubpassBufferCollection
		{
			std::vector<AttachmentHandleReference> input_attachment_refs;
			std::vector<AttachmentHandleReference> color_attachment_refs;
			std::vector<AttachmentHandleReference> resolve_attachment_refs;
			std::vector<AttachmentHandle> preserved_attachments;
			AttachmentHandleReference depth_stencil_attachment_ref;
			SamplingMode sampling_mode;

			UINT32 subpass_index = 0;
		};
		std::vector<SubpassBufferCollection> subpass_data_buffers;

		struct DependencyDescription
		{
			vk::DependencyFlags flags;

			SubpassHandle start_subpass;
			vk::PipelineStageFlagBits start_stage;
			vk::AccessFlagBits start_access;

			SubpassHandle final_subpass;
			vk::PipelineStageFlagBits final_stage;
			vk::AccessFlagBits final_access;

			SamplingMode sampling_mode;
		};
		std::vector<DependencyDescription> dependency_descriptions;

		vk::RenderPass last_render_pass;
		UINT32 last_attachment_count;
		
		std::vector<vk::Format> formats;
		std::vector<vk::SampleCountFlagBits> sample_counts;

		bool outdated = true;
	};
}