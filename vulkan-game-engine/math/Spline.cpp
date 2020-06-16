#include <math/Spline.hpp>

void noxcain::CubicCurve::updateLength()
{
	length = 0.0;

	struct GaussLengendreCoefficient
	{
		DOUBLE abscissa; // xi
		DOUBLE weight;   // wi
	};

	static constexpr GaussLengendreCoefficient coefficients[] = //Gauss-Lengendre
	{
		{ -0.9061798459386640, 0.2369268850561891 },
		{ -0.5384693101056831, 0.4786286704993665 },
		{ 0.0, 0.5688888888888889 },
		{ 0.5384693101056831, 0.4786286704993665 },
		{ 0.9061798459386640, 0.2369268850561891 }
	};

	for( auto coefficient : coefficients )
	{
		length += ( coefficient.weight* get_tangent( 0.5 * ( 1.0 + coefficient.abscissa ) ) ).get_length();
	}

	length *= 0.5;
}

noxcain::NxVector3D noxcain::CubicCurve::get_tangent( DOUBLE t ) const
{
	const NxVector3D c0 = 3*( p1 - p0 );
	const NxVector3D c1 = 6*( p0 - 2*p1 + p2 );
	const NxVector3D c2 = 3*( p3 - p0 + 3*( p1-p2 ) );

	return ( c0 + t * ( c1 + t * c2 ) );
}

noxcain::CubicCurve::CubicCurve( const NxVector3D& control_point_0, const NxVector3D& control_point_1, const NxVector3D& control_point_2, const NxVector3D& control_point_3 ) :
	p0( control_point_0 ), p1( control_point_1 ), p2( control_point_2 ), p3( control_point_3 )
{
	updateLength();
}

noxcain::NxVector3D noxcain::CubicCurve::get_position( DOUBLE t ) const
{
	const NxVector3D c0 = p0;
	const NxVector3D c1 = 3*( p1 - p0 );
	const NxVector3D c2 = 3*( p0 - 2*p1 + p2 );
	const NxVector3D c3 = p3 - p0 + 3*( p1-p2 );

	return c0 + t * ( c1 + t * ( c2 + t * c3 ) );
}

noxcain::NxVector3D noxcain::CubicCurve::get_direction( DOUBLE t ) const
{
	return get_tangent( t ).normalize();
}

noxcain::CubicSpline::CubicSpline( const std::vector<NxVector3D>& control_points ) : length( 0 )
{
	if( control_points.size() > 3 )
	{
		curves.emplace_back( control_points[0], control_points[1], control_points[2], control_points[3] );
		length = curves.back().get_length();

		for( std::size_t index = 4; index < control_points.size(); index = index + 2 )
		{
			curves.emplace_back( control_points[index-1], 2*control_points[index-1] - control_points[index-2], control_points[index], control_points[index+1] );
			length += curves.back().get_length();
		}
	}
}

noxcain::CubicSpline::Tangent noxcain::CubicSpline::get_tangent( DOUBLE t ) const
{
	for( const auto& curve : curves )
	{
		DOUBLE subdivision = curve.get_length() / length;
		if( subdivision > t )
		{
			const DOUBLE parameter = t / subdivision;
			return { curve.get_position( parameter ), curve.get_direction( parameter ) };
		}
		t -= subdivision;
	}
	const auto& lastCurve = curves.back();
	return { lastCurve.get_position( 1.0 ), lastCurve.get_direction( 1.0 ) };
}

noxcain::NxVector3D noxcain::CubicSpline::get_position( DOUBLE t ) const
{
	for( const auto& curve : curves )
	{
		DOUBLE subdivision = curve.get_length() / length;
		if( subdivision > t )
		{
			return curve.get_position( t / subdivision );
		}
		t -= subdivision;
	}

	if( !curves.empty() )
	{
		return curves.back().get_position( 1.0 );
	}

	return NxVector3D();
}

noxcain::NxVector3D noxcain::CubicSpline::get_direction( DOUBLE t ) const
{
	for( const auto& curve : curves )
	{
		DOUBLE subdivision = curve.get_length() / length;
		if( subdivision > t )
		{
			return curve.get_direction( t / subdivision );
		}
		t -= subdivision;
	}

	if( !curves.empty() )
	{
		return curves.back().get_direction( 1.0 );
	}

	return NxVector3D();
}
