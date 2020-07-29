#pragma once
#include <logic/Level.hpp>
namespace noxcain
{
	struct GameUserInterface;
	
	class VectorTextLabel2D
	{
	public:
		VectorTextLabel2D( GameUserInterface& ui );

		enum class HorizontalTextAlignments
		{
			LEFT,
			RIGHT,
			CENTER
		};

		enum class VerticalTextAlignments
		{
			TOP,
			BOTTOM,
			CENTER
		};

		enum class AutoResizeModes
		{
			WIDTH,
			HEIGHT,
			FULL,
			NONE
		};
		
		const Region& get_area() const
		{
			return frame;
		}

		Region& get_area()
		{
			return frame;
		}

		operator Region& ( )
		{
			return frame;
		}

		operator const Region&() const
		{
			return frame;
		}

		const VectorText& get_text_element() const
		{
			return text_content.get_text();
		}

		VectorText& get_text_element()
		{
			return text_content.get_text();
		}

		void set_depth_level( UINT32 level );

		void set_auto_resize( AutoResizeModes mode = AutoResizeModes::FULL );
		void set_text_alignment( HorizontalTextAlignments alignment );
		void set_text_alignment( VerticalTextAlignments alignment );
		
		void set_frame_size( DOUBLE size );
		void set_frame_color( DOUBLE red, DOUBLE green, DOUBLE blue, DOUBLE alpha = 1.0 );
		void set_background_color( DOUBLE red, DOUBLE green, DOUBLE blue, DOUBLE alpha = 1.0 );

		void set_centered_icon( DOUBLE size, UINT32 unicode );

		virtual void hide();
		virtual void show();

		virtual ~VectorTextLabel2D()
		{
		};

		const bool hidden() const
		{
			return is_hidden;
		}

		const bool shown() const
		{
			return !is_hidden;
		}

	protected:
		bool is_hidden = true;
		DOUBLE frame_size = 0.0;
		VectorText2D text_content;
		RenderableQuad2D background;
		RenderableQuad2D frame;
	};
}
