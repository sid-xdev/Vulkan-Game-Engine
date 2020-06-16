#version 450

layout( early_fragment_tests ) in;

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec3 inNormal;

layout( location = 0 ) out vec4 outPosition;
layout( location = 1 ) out vec4 outNormal;
layout( location = 2 ) out vec4 outColor;

void main()
{
	outPosition = vec4( inPosition, 1.0F );
	outNormal   = vec4( inNormal, 1.0F );
	outColor    = vec4( vec3(1.0F), gl_SampleMaskIn[0] == 255 ? 0.0F : 1.0F );
}