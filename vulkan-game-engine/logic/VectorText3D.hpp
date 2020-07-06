#pragma once
#include <Defines.hpp>

#include <logic/Renderable.hpp>
#include <logic/SceneGraph.hpp>
#include <logic/VectorText.hpp>

#include <vector>
#include <string>
#include <array>
#include <vulkan/vulkan.hpp>

namespace noxcain
{	
	class NxMatrix4x4;
	
	class VectorText3D : public SceneGraphNode, public Renderable<VectorText3D>
	{
	public:
		VectorText3D( Renderable<VectorText3D>::List& visibility_list );
		
		void set_text( std::string utf8 )
		{
			text.set_utf8( utf8 );
		}

		void set_font_size( DOUBLE size )
		{
			text.set_size( size );
		}
		DOUBLE get_font_size() const
		{
			return text.get_size();
		}

		void set_font( UINT32 font_id )
		{
			text.set_font_id( font_id );
		}
		UINT32 get_font_id() const
		{
			return UINT32( text.get_font_id() );
		}

		void set_font_color( const std::array<FLOAT32, 4>& color )
		{
			text.set_color( color );
		}
		void set_font_color( DOUBLE red, DOUBLE green, DOUBLE blue, DOUBLE alpha = 1.0 )
		{
			text.set_color( FLOAT32( red ), FLOAT32( green ), FLOAT32( blue ), FLOAT32( alpha ) );
		}

		DOUBLE get_width();
		DOUBLE get_height();

		void record( vk::CommandBuffer command_buffer, vk::PipelineLayout pipeline_layout, const NxMatrix4x4& camera ) const;
	private:
		VectorText text;
	};
}

