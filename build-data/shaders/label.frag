#version 450

layout(push_constant) uniform PushConstants {
	float r;
	float g;
	float b;
	float a;
} push;

layout( location = 0 ) out vec4 color;

void main()
{
	color = vec4( push.r, push.g, push.b, push.a );
}