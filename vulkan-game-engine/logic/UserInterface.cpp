#include "UserInterface.hpp"

void noxcain::GameUserInterface::sort()
{
	bool sort_text = texts.sort( []( const VectorText2D& first, const VectorText2D& second ) ->bool
	{
		return first.get_depth_level() < second.get_depth_level();
	} );

	bool text_sort = labels.sort( []( const RenderableQuad2D& first, const RenderableQuad2D& second ) ->bool
	{
		return first.get_depth_level() < second.get_depth_level();
	} );

	if( sort_text || text_sort )
	{
		depth_order.clear();
		auto label_iter = labels.begin();
		auto text_iter = texts.begin();

		while( label_iter != labels.end() || text_iter != texts.end() )
		{
			UINT32 label_count = 0;
			while( label_iter != labels.end() && ( text_iter == texts.end() || label_iter->get().get_depth_level() <= text_iter->get().get_depth_level() ) )
			{
				label_count++;
				label_iter++;
			}

			UINT32 text_count = 0;
			while( text_iter != texts.end() && ( label_iter == labels.end() || text_iter->get().get_depth_level() < label_iter->get().get_depth_level() ) )
			{
				text_count++;
				text_iter++;
			}
			depth_order.emplace_back( std::array<UINT32, 2>( { label_count, text_count } ) );
		}
	}
}
