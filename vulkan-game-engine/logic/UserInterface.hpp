#pragma once

#include <logic/VectorText2D.hpp>
#include <logic/Quad2D.hpp>

namespace noxcain
{	
	
	
	/// <summary>
	/// Pairs together 2D labels and text and the right render order
	/// </summary>
	class GameUserInterface
	{
		friend class GameLevel;
	public:
		struct OrderEntry
		{
			UINT32 label_count;
			UINT32 text_count;
		};

		Renderable<RenderableQuad2D>::List::Iterator get_label_iterator() const
		{
			return labels.begin();
		}

		Renderable<VectorText2D>::List::Iterator get_text_iterator() const
		{
			return texts.begin();
		}
		
		const std::vector<OrderEntry>& get_order() const
		{
			return depth_order;
		}

		Renderable<RenderableQuad2D>::List& get_labels()
		{
			return labels;
		}

		Renderable<VectorText2D>::List& get_texts()
		{
			return texts;
		}

		void set_regional_event_root( RegionalEventRecieverNode& node );
	private:
		RegionalEventRecieverNode* regional_event_root = nullptr;
		Renderable<RenderableQuad2D>::List labels;
		Renderable<VectorText2D>::List texts;
		std::vector<OrderEntry> depth_order;
		void sort();
	};
}