#pragma once
#include <Defines.hpp>
#include <array>
#include <vector>
#include <string>

namespace noxcain
{
	class FontResource;
	
	class VectorText
	{
		friend class VectorText2D;
		friend class VectorText3D;
	public:

		VectorText();

		enum class Alignments
		{
			LEFT,
			CENTER,
			RIGHT
		};

		void set_text_alignment( Alignments alignment )
		{
			text_alignment = alignment;
		}

		void set_utf8( const std::string& utf8String );
		void set_size( DOUBLE size )
		{
			this->size = size;
		}

		DOUBLE get_size() const
		{
			return this->size;
		}

		std::size_t get_char_count() const
		{
			return unicodes.size();
		}

		bool empty() const;

		void set_font_id( std::size_t id );

		const FontResource& get_font() const;
		std::size_t get_font_id() const
		{
			return font_index;
		}

		const DOUBLE get_length() const
		{
			return size* max_width;
		}

		DOUBLE get_descender() const;

		void set_color( FLOAT32 red, FLOAT32 green, FLOAT32 blue, FLOAT32 alpha = 1.0 );
		void set_color( const std::array<FLOAT32, 4>& color );
		const std::array<FLOAT32, 4>& get_color() const
		{
			return colors[0];
		}
	
	private:
		struct GylphInstance
		{
			DOUBLE x_offset = 0;
			UINT32 glyph_id = 0;
			UINT8 color_index = 0;
			UINT8 line_index = 0;
		};

		Alignments text_alignment = Alignments::LEFT;

		void calculate_offsets();

		void set_base_color( const std::array<FLOAT32,4>& color );

		std::size_t font_index = 0;

		std::vector<UINT32> unicodes;
		std::vector<GylphInstance> glyphs;
		std::vector<DOUBLE> line_lengths;

		DOUBLE fixed_line_length = 0;
		DOUBLE line_height = 0;
		DOUBLE size = 1;
		DOUBLE max_width = 0;
		DOUBLE max_height = 0;

		std::vector<std::array<FLOAT32, 4>> colors = { { 1.0F, 1.0F, 1.0F, 1.0F } };
	};
}