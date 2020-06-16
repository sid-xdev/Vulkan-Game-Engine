#version 450

layout (constant_id = 0) const float emPerPixelWidth = 1;
layout (constant_id = 1) const float emPerPixelHeight = 1;

layout( location = 0 ) in vec2 inPosition;

layout( location = 0 ) out vec2 outUV;
layout( location = 1 ) out vec2 outSearcherX;
layout( location = 2 ) out vec2 outSearcherY;

layout(push_constant) uniform PushConstants {
    layout(offset=32) mat4 perspectiv;
} push;

const vec2 directions[4] =
{
	vec2( -1.0F, 1.0F ),
	vec2( 1.0F, 1.0F ),
	vec2( -1.0F, -1.0F ),
	vec2( 1.0F, -1.0F )
};

vec2 intersect( vec2 v1, vec2 v2, vec2 c )
{
	float check = v1.x*v2.y - v2.x*v1.y;
	if( check == 0 )
	{
		return vec2( 0.0F );
	}
	
	float s = ( v1.x * c.y - c.x * v1.y ) / check;
	float r = abs( v1.x ) > 0.01F ? ( c.x - s * v2.x )/ v1.x : ( c.y - s * v2.y )/ v1.y; 
	return vec2( r,s );
}

void main() 
{
	const vec2 emPerPixel = vec2( emPerPixelWidth, emPerPixelHeight );
	const uint vertexIndex = gl_VertexIndex%4;
	
	vec4 originalCorner = vec4( inPosition,0.0F,1.0F );
	vec4 advancedCorner = vec4( originalCorner.xy + directions[vertexIndex], 0.0F, 1.0F );
	
	originalCorner = push.perspectiv*originalCorner;
	advancedCorner = push.perspectiv*advancedCorner;
	
	float factor = length( vec2( 1.0F, 1.0F ) ) / length( ( advancedCorner/advancedCorner.w - originalCorner/originalCorner.w ).xy );
	
	outUV = inPosition + factor * directions[vertexIndex] * emPerPixel;
	
	gl_Position = push.perspectiv * vec4( outUV, 0.0F, 1.0F );
	
	{
		const vec4 searcherX = push.perspectiv * vec4( outUV + vec2( 1.0F, 0.0F ), 0.0F, 1.0F );
		outSearcherX = searcherX.xy/searcherX.w - gl_Position.xy/gl_Position.w;
	}
	
	{
		const vec4 searcherY = push.perspectiv * vec4( outUV + vec2( 0.0F, 1.0F ), 0.0F, 1.0F );
		outSearcherY = searcherY.xy/searcherY.w - gl_Position.xy/gl_Position.w;
	}
}