#version 450

layout( location = 0 ) out vec4 outColor;
layout( location = 0 ) in vec2 inUV;

layout (set = 0, binding = 0) uniform sampler2D superRender;

void main()
{
	outColor = texture( superRender, inUV );
}