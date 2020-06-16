#version 450

layout (constant_id = 0) const float width = 1;
layout (constant_id = 1) const float height = 1;

layout( location = 0 ) in vec2 position;

layout( location = 0 ) out vec2 uv;

layout(push_constant) uniform PushConstants {
    layout(offset=32) float pixelPerEm;
	float cosA;
	float sinA;
	float x;
	float y;
} push;

const vec2 directions[4] =
{
	vec2( -1.0F, 1.0F ),
	vec2( 1.0F, 1.0F ),
	vec2( -1.0F, -1.0F),
	vec2( 1.0F, -1.0F)
};

void main() 
{
	uv = 0.5F/push.pixelPerEm * directions[gl_VertexIndex%4] + position;
	gl_Position = vec4( ( push.pixelPerEm * mat2( push.cosA, push.sinA, -push.sinA, push.cosA )*uv + vec2( push.x, push.y ) )*vec2( width, -height ) + vec2( -1.0, 1.0 ), 0.0F, 1.0F ); //
}