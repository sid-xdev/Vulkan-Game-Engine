#version 450

layout( location = 1 ) in vec3 inPosition;
layout( location = 0 ) in vec3 inNormal;

layout( location = 0 ) out vec3 outPosition;
layout( location = 1 ) out vec3 outNormal;

layout(push_constant) uniform PushConstants {
    mat4 worldPosition;
	mat4 camera;
} push;

void main() 
{
    const vec4 position = push.worldPosition*vec4( inPosition, 1.0F );
	
	outPosition = position.xyz;
	outNormal = ( push.worldPosition*vec4( inNormal,1.0F ) - push.worldPosition[3] ).xyz;
	
	gl_Position = push.camera * position;
}