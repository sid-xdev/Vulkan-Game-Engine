#version 450

#extension GL_GOOGLE_include_directive : enable
#include "shading.include"

layout( early_fragment_tests ) in;

layout( input_attachment_index = 0, set = 0, binding = 0 ) uniform subpassInput colorInput;
layout( input_attachment_index = 1, set = 0, binding = 1 ) uniform subpassInput normalInput;
layout( input_attachment_index = 2, set = 0, binding = 2 ) uniform subpassInput positionInput;

layout( location = 0 ) out vec4 outColor;

void main()
{	
	outColor = vec4( shading( subpassLoad( colorInput ).rgb, subpassLoad( positionInput ).xyz, subpassLoad( normalInput ).xyz ), 1.0F );
}