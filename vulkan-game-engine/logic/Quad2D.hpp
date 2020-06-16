#pragma once

#include <Defines.hpp>

#include <logic/Region.hpp>
#include <logic/Renderable.hpp>
#include <logic/RegionEventReceiver.hpp>

#include <vulkan/vulkan.hpp>

namespace noxcain
{
	class RenderableQuad2D : public Region, public Renderable<RenderableQuad2D>
	{
	public:
		static constexpr std::size_t VERTEX_PUSH_OFFSET = 16;
		static constexpr std::size_t FRAGMENT_PUSH_OFFSET = 0;

		static constexpr std::size_t VERTEX_PUSH_SIZE = 16;
		static constexpr std::size_t FRAGMENT_PUSH_SIZE = 16;

		RenderableQuad2D( Renderable<RenderableQuad2D>::List& visibility_list );

		void set_scissor( const Region* scissor_ )
		{
			scissor = scissor_;
		}
		void set_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha );
		void set_color( const std::array<FLOAT32, 4>& label_color );
		const std::array<FLOAT32, 4>& get_color() const
		{
			return color;
		}

		void set_depth_level( UINT32 depth_ )
		{
			depth = depth_;
		}

		UINT32 get_depth_level() const
		{
			return depth;
		}

		vk::Rect2D record( const vk::CommandBuffer& command_buffer, vk::PipelineLayout pipeline_layout, vk::Rect2D last_scissor, vk::Rect2D default_scissor ) const;
	private:
		UINT32 depth = 0;
		std::array<FLOAT32, 4> color = { 0.0, 0.0, 0.0, 0.0 };
		const Region* scissor = nullptr;
	};

	class PassivColorLabel final : public RenderableQuad2D, public RegionalEventRecieverNode
	{
	public:
		PassivColorLabel( Renderable<RenderableQuad2D>::List& visibility_list ) : RenderableQuad2D( visibility_list )
		{
		}
		~PassivColorLabel() override
		{
		}
	private:
		bool try_hit( const RegionalKeyEvent& regional_event ) const override
		{
			return Region::check_region( regional_event.get_x_position(), regional_event.get_y_position() );
		}
	};
}