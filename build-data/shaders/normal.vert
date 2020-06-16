#version 450

layout( location = 0 ) in vec3 inNormal;
layout( location = 1 ) in vec3 inPosition;

layout(push_constant) uniform PushConstants {
    mat4 worldPosition;
	mat4 camera;
} push;

void main() 
{
	if( gl_VertexIndex%2 == 0 )
	{
		vec3 pos = (push.worldPosition*vec4( inPosition, 1.0F )).xyz;
		vec3 normal = ( push.worldPosition*vec4( inNormal,1.0F ) - push.worldPosition[3] ).xyz;
		gl_Position = push.camera * vec4( pos + normal, 1.0F );
	}
	else
	{
		vec3 pos = (push.worldPosition*vec4( inNormal, 1.0F )).xyz;
		gl_Position = push.camera * vec4( pos, 1.0F );
	}
}