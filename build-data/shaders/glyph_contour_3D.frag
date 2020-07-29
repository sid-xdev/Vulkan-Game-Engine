#version 450

layout( early_fragment_tests ) in;

layout( constant_id = 0 ) const float emPerPixelWidth = 1;
layout( constant_id = 1 ) const float emPerPixelHeight = 1;
layout( constant_id = 2 ) const uint  maxGlyphCount = 1;

layout( location = 0 ) out vec4 outPosition;
layout( location = 1 ) out vec4 outNormal;
layout( location = 2 ) out vec4 outColor;

layout( location = 0 ) in vec2 inUV;
layout( location = 1 ) in vec2 inSearcherX;
layout( location = 2 ) in vec2 inSearcherY;

layout(push_constant) uniform PushConstants {
	uint glyphIndex;
	float r;
	float g;
	float b;
	float a;
} push;

layout( set = 0, binding = 0 ) buffer readonly GlyphOffsets
{
	uint values[];
} offsets[maxGlyphCount];

layout( set = 0, binding = 1 ) buffer readonly GlyphPoints
{
	float values[];
} points[maxGlyphCount];



// solves distance on (1,0)
vec2 solvePolyY( const vec2 p1, const vec2 p2, const vec2 p3 )
{
	const vec2 a = p1 - 2.0F*p2 + p3;
	const vec2 b = p1 - p2;
	const float d = sqrt( max( b.y*b.y - a.y * p1.y, 0.0F ) );
	float t1;
	float t2;
	
	if( abs(a.y) > 0.0000001F )
	{
		const float ra = 1.0F / a.y;
		t1 = ( b.y - d ) * ra;
		t2 = ( b.y + d ) * ra;
	}
	else
	{
		const float rb = 0.5F / b.y;
		t1 = t2 = p1.y * rb;
	}
	
	return( vec2( ( a.x * t1 - 2.0F*b.x ) * t1 + p1.x, 
	              ( a.x * t2 - 2.0F*b.x ) * t2 + p1.x ) );
}

// solves distance on (0,1)
vec2 solvePolyX( const vec2 p1, const vec2 p2, const vec2 p3 )
{
	const vec2 a = p1 - 2.0F*p2 + p3;
	const vec2 b = p1 - p2;
	const float d = sqrt( max( b.x*b.x - a.x * p1.x, 0.0F ) );
	float t1;
	float t2;
	
	if( abs(a.x) > 0.0000001F )
	{
		const float ra = 1.0F / a.x;
		t1 = ( b.x - d ) * ra;
		t2 = ( b.x + d ) * ra;
	}
	else
	{
		const float rb = 0.5F / b.x;
		t1 = t2 = p1.x * rb;
	}
	
	return( vec2( ( a.y * t1 - 2.0F*b.y ) * t1 + p1.y, 
	              ( a.y * t2 - 2.0F*b.y ) * t2 + p1.y ) );
}

ivec2 calcRootCode( const float v1, const float v2, const float v3 )
{
	int a = int(floor(v1));
	int b = int(floor(v2));
	int c = int(floor(v3));

	return( ivec2 (~a & (b | c) | (~b & c ), a & (~b | ~c) | (b & ~c) ) );
}

bool testCurve( const ivec2 code )
{
	return( ( code.x | code.y ) < 0 );
}

bool testRoot1( const ivec2 code )
{
	return code.x < 0;
}

bool testRoot2( const ivec2 code )
{
	return code.y < 0;
}

const float MAX_SAMPLE_COUNT = 4.0F;

void main()
{	
	vec2 emPerPixel = vec2( emPerPixelWidth, emPerPixelHeight );
	
	const vec2 absXDirection = abs( inSearcherX/emPerPixel );
	const vec2 absYDirection = abs( inSearcherY/emPerPixel );
	
	//pixelPerEm at this point
	emPerPixel = 1.0F/vec2( max( absXDirection.x, absXDirection.y ), max( absYDirection.x, absYDirection.y ) );
	
	const uvec2 sampleCount = uvec2( clamp( 1000.0F*emPerPixel.x, 1.0F, MAX_SAMPLE_COUNT ) , clamp( 1000.0F*emPerPixel.y, 1.0F, MAX_SAMPLE_COUNT ) );
	
	const vec2 offsetStep = 1.0F/sampleCount;
	float coverage = 0.0F;
	
	for( float pixelOffset = -0.5F + 0.5F*offsetStep.x; pixelOffset < 0.5F; pixelOffset += offsetStep.x )
	{
		const vec2 posX = vec2( inUV.x + pixelOffset*emPerPixel.x , inUV.y );
		
		uint nXCurve = 0;
		uint xOffset = 0;
		
		for( uint bandIndex = 0; bandIndex < 32; ++bandIndex )
		{	
			if( posX.x <= points[push.glyphIndex].values[2*bandIndex] )
			{
				nXCurve = offsets[push.glyphIndex].values[4*bandIndex];
				xOffset = offsets[push.glyphIndex].values[4*bandIndex+1];
				break;
			}
		}
		
		for( uint curveIndex = 0; curveIndex < nXCurve; ++curveIndex )
		{
			const uint curveOffset = offsets[push.glyphIndex].values[xOffset+curveIndex];
			const vec2 startPoint   = ( vec2( points[push.glyphIndex].values[curveOffset],   points[push.glyphIndex].values[curveOffset+1] ) - posX );
			const vec2 controlPoint = ( vec2( points[push.glyphIndex].values[curveOffset+2], points[push.glyphIndex].values[curveOffset+3] ) - posX );
			const vec2 endPoint     = ( vec2( points[push.glyphIndex].values[curveOffset+4], points[push.glyphIndex].values[curveOffset+5] ) - posX );
			
			if( max( max( startPoint.y, controlPoint.y ), endPoint.y ) < 0.0F ) break;
			const ivec2 code = calcRootCode( startPoint.x, controlPoint.x, endPoint.x );
			if( testCurve( code ) )
			{
				const vec2 r = solvePolyX( startPoint, controlPoint, endPoint ) / emPerPixel.y;
				
				if( testRoot1(code) ) coverage -= clamp( r.x + 0.5F, 0.0F, 1.0F );
				if( testRoot2(code) ) coverage += clamp( r.y + 0.5F, 0.0F, 1.0F );
			}
		}
	}

	for( float pixelOffset = -0.5F + 0.5F*offsetStep.y; pixelOffset < 0.5F; pixelOffset += offsetStep.y )
	{
		const vec2 posY = vec2( inUV.x, inUV.y + pixelOffset*emPerPixel.y );
		
		uint nYCurve = 0;
		uint yOffset = 0;
		
		for( uint bandIndex = 0; bandIndex < 32; ++bandIndex )
		{	
			if( posY.y <= points[push.glyphIndex].values[2*bandIndex + 1] )
			{	
				nYCurve = offsets[push.glyphIndex].values[4*bandIndex+2];
				yOffset = offsets[push.glyphIndex].values[4*bandIndex+3];
				break;
			}
		}
		
		for( uint curveIndex = 0; curveIndex < nYCurve; ++curveIndex )
		{
			const uint curveOffset = offsets[push.glyphIndex].values[yOffset+curveIndex];
			const vec2 startPoint   = vec2( points[push.glyphIndex].values[curveOffset],   points[push.glyphIndex].values[curveOffset+1] ) - posY;
			const vec2 controlPoint = vec2( points[push.glyphIndex].values[curveOffset+2], points[push.glyphIndex].values[curveOffset+3] ) - posY;
			const vec2 endPoint     = vec2( points[push.glyphIndex].values[curveOffset+4], points[push.glyphIndex].values[curveOffset+5] ) - posY;
			
			if( max( max( startPoint.x, controlPoint.x ), endPoint.x ) < 0.0F ) break;
			const ivec2 code = calcRootCode( startPoint.y, controlPoint.y, endPoint.y );
			if( testCurve( code ) )
			{
				const vec2 r = solvePolyY( startPoint, controlPoint, endPoint ) / emPerPixel.x;
				
				if( testRoot1(code) ) coverage += clamp( r.x + 0.5F, 0.0F, 1.0F );
				if( testRoot2(code) ) coverage -= clamp( r.y + 0.5F, 0.0F, 1.0F );
			}
		}
	}
	
	outPosition = vec4( 0.0F );
	outNormal   = vec4( 0.0F );
	outColor    = vec4( push.r, push.g, push.b, push.a * coverage / ( sampleCount.x + sampleCount.y ) );
}