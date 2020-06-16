#version 450

layout (constant_id = 0) const float screen_space_width_factor = 1;
layout (constant_id = 1) const float screen_space_height_factor = 1;

layout(push_constant) uniform PushConstants {
    layout(offset=16) float x;
	float y;
	float width;
	float height;
} push;

const vec2 uv[4] =
{
	vec2( 0.0F, 0.0F ),
	vec2( 1.0F, 0.0F ),
	vec2( 0.0F, 1.0F ),
	vec2( 1.0F, 1.0F )
};

void main() 
{
	const vec2 corner = vec2( screen_space_width_factor, screen_space_height_factor ) * vec2( push.x + (gl_VertexIndex%2) * push.width, push.y + ( gl_VertexIndex > 1 ? push.height : 0.0F ) ) + vec2( -1.0, 1.0 );
	gl_Position = vec4( corner, 0.0F, 1.0F );
}