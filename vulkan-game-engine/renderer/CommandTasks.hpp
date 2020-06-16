#pragma once
#include <Defines.hpp>

#include <renderer/CommandSubpassTask.hpp>
#include <tools/TimeFrame.hpp>

#include <vulkan/vulkan.hpp>

namespace noxcain
{
	struct SpecializationMeta
	{
		SpecializationMeta() = default;
		SpecializationMeta( SpecializationMeta&& ) = default;
		std::vector<vk::SpecializationMapEntry> descriptions;
		std::vector<BYTE> data;
	};

	// helper for specialization memory layout
	template<typename ... Types>
	SpecializationMeta createSpecialization( Types ... specializationValues )
	{
		SpecializationMeta meta;
		meta.descriptions.reserve( sizeof...( specializationValues ) );
		meta.data.resize( ( sizeof( specializationValues ) + ... ) );

		auto fill = [&meta]( auto&& value )
		{
			uint32_t offset = meta.descriptions.empty() ? 0 : meta.descriptions.back().offset + meta.descriptions.back().size;
			meta.descriptions.emplace_back( uint32_t( meta.descriptions.size() ), offset, uint32_t( sizeof( value ) ) );
			*( reinterpret_cast<std::remove_reference_t<decltype( value )>*>( meta.data.data() + offset ) ) = value;
		};

		( fill( specializationValues ), ... );
		return meta;
	};
	
	class VectorText2D;

	class OverlayTask : public SubpassTask<OverlayTask>
	{
	public:
		OverlayTask();
		~OverlayTask();

		bool buffer_independent_preparation();
		bool buffer_dependent_preparation( CommandData& pool_data );
		bool record( const std::vector<vk::CommandBuffer>& buffers );

	private:
		TimeFrameCollector time_col = TimeFrameCollector( "Overlay" );
		bool setup_layouts();

		vk::Extent2D old_extent;

		vk::PipelineLayout post_pipeline_layout;
		vk::Pipeline post_pipeline;
		inline bool build_post_pipeline();

		vk::PipelineLayout label_pipeline_layout;
		vk::Pipeline label_pipeline;
		inline bool build_label_pipeline();

		vk::PipelineLayout text_pipeline_layout;
		vk::Pipeline text_pipeline;
		inline bool build_text_pipeline();
	};

	class GeometryTask : public SubpassTask<GeometryTask>
	{
	public:
		GeometryTask();
		~GeometryTask();

		bool buffer_independent_preparation();
		bool buffer_dependent_preparation( CommandData& pool_data );
		bool record( const std::vector<vk::CommandBuffer>& buffers );

	private:
		TimeFrameCollector time_col = TimeFrameCollector( "Geometry" );
		vk::Extent2D old_extent;
		bool setup_layout();

		vk::PipelineLayout geomtry_pipeline_layout;
		vk::Pipeline geomtry_pipeline;
		inline bool build_geomtry_pipeline();
	};

	class VectorDecalTask : public SubpassTask<VectorDecalTask>
	{
	public:
		VectorDecalTask();
		~VectorDecalTask();

		bool buffer_independent_preparation();
		bool buffer_dependent_preparation( CommandData& pool_data );
		bool record( const std::vector<vk::CommandBuffer>& buffers );

	private:
		TimeFrameCollector time_col = TimeFrameCollector( "Vector" );
		vk::Extent2D old_extent;
		bool setup_layout();

		vk::PipelineLayout vector_decal_pipeline_layout;
		vk::Pipeline vector_decal_pipeline;
		inline bool build_vector_decal_pipeline();
	};

	class SamplingTask : public SubpassTask<SamplingTask>
	{
	public:
		SamplingTask();
		~SamplingTask();

		bool buffer_independent_preparation();
		bool buffer_dependent_preparation( CommandData& pool_data );
		bool record( const std::vector<vk::CommandBuffer>& buffers );

	private:
		TimeFrameCollector time_col = TimeFrameCollector( "Shading" );
		vk::Extent2D old_extent;
		bool setup_layouts();

		vk::PipelineLayout sampling_pipeline_layout;
		vk::Pipeline edge_detection_pipeline;
		vk::Pipeline sampled_pipeline;
		vk::Pipeline unsampled_pipeline;
		inline bool build_shading_pipelines();
		inline bool build_edge_detection_pipeline();
	};
}