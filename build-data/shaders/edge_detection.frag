#version 450

layout (constant_id = 0) const uint NUM_SAMPLES = 1;

layout( input_attachment_index = 0, set = 0, binding = 0 ) uniform subpassInputMS colorInput;
layout( input_attachment_index = 1, set = 0, binding = 1 ) uniform subpassInputMS normalInput;
layout( input_attachment_index = 2, set = 0, binding = 2 ) uniform subpassInputMS positionInput;

void main()
{	
	vec4 base_normal = subpassLoad( normalInput, 0 );
	vec4 base_position = subpassLoad( positionInput, 0 );
	for( int sampleId = 1; sampleId < NUM_SAMPLES; ++sampleId )
	{
		if( subpassLoad( normalInput, sampleId ) != base_normal ||  subpassLoad( positionInput, sampleId ) != base_position )
		{
			discard;
		}
	}
}