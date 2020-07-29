#pragma once

#include <logic/VectorText2D.hpp>
#include <logic/Quad2D.hpp>

namespace noxcain
{
	/// <summary>
	/// Pairs together 2D labels and text and the right render order
	/// </summary>
	struct GameUserInterface
	{
		Renderable<RenderableQuad2D>::List labels;
		Renderable<VectorText2D>::List texts;
		std::vector<std::array<UINT32, 2>> depth_order;
		void sort();
	};
}