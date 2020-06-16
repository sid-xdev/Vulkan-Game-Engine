#version 450

layout( location = 0 ) out vec4 outPosition;
layout( location = 1 ) out vec4 outNormal;
layout( location = 2 ) out vec4 outColor;

void main()
{
	outNormal   = vec4( 0.0F );
	outColor    = vec4( 1.0F );
}