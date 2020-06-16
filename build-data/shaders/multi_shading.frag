#version 450

#extension GL_GOOGLE_include_directive : enable
#include "shading.include"

layout( early_fragment_tests ) in;
layout( constant_id = 0 ) const uint NUM_SAMPLES = 1;

layout( input_attachment_index = 0, set = 0, binding = 0 ) uniform subpassInputMS colorInput;
layout( input_attachment_index = 1, set = 0, binding = 1 ) uniform subpassInputMS normalInput;
layout( input_attachment_index = 2, set = 0, binding = 2 ) uniform subpassInputMS positionInput;

layout( location = 0 ) out vec4 outColor;

vec3 resolveColor()
{
	vec3 texel = vec3(0.0F);

	for( int sampleId = 0; sampleId < NUM_SAMPLES; ++sampleId )
	{
		texel += shading( subpassLoad( colorInput, sampleId ).rgb, subpassLoad( positionInput, sampleId ).xyz, subpassLoad( normalInput, sampleId ).xyz );
		//subpassLoad( colorInput, sampleId ).rgb * clamp( dot( normalize(light - subpassLoad( positionInput, sampleId ).xyz), subpassLoad( normalInput, sampleId ).xyz ), 0.0, 1.0 );
	}
	return texel/NUM_SAMPLES;
}

void main()
{	
	outColor = vec4( resolveColor(), 1.0F );
}