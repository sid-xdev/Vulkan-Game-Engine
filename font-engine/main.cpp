#include "FontEngine.hpp"

#include <string>

int main( int argc, char* argv[] )
{
	noxcain::FontEngine font_engine;

	std::string in = "D:/Projekte/vulkan-game-engine/resources/Fonts/28 Days Later.ttf";
	std::string out = "D:/Desktop/test.nxf";

	return font_engine.read_font(in, out);
}