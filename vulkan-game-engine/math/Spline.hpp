#pragma once
#include <Defines.hpp>
#include <math/Vector.hpp>
#include <vector>

namespace noxcain
{	
	class CubicCurve
	{
		NxVector3D p0;
		NxVector3D p1;
		NxVector3D p2;
		NxVector3D p3;
		
		DOUBLE length;
		void updateLength();
		NxVector3D get_tangent( DOUBLE t ) const;

	public:
		CubicCurve( const NxVector3D& control_point_0, const NxVector3D& control_point_1, const NxVector3D& control_point_2, const NxVector3D& control_point_3 );
		//void setStart( const NxVector3D&& point, const NxVector3D& tangent );
		//void setEnd( const NxVector3D&& point, const NxVector3D& tangent );

		NxVector3D get_position( DOUBLE t ) const;
		NxVector3D get_direction( DOUBLE t ) const;
		DOUBLE get_length() const { return length; };
	};

	class CubicSpline
	{
		std::vector<CubicCurve> curves;
		DOUBLE length;
	public:
		CubicSpline( const std::vector<NxVector3D>& control_points );
		
		struct Tangent
		{
			NxVector3D slider_position;
			NxVector3D direction;
		};

		Tangent get_tangent( DOUBLE t ) const;
		NxVector3D get_position( DOUBLE t ) const;
		NxVector3D get_direction( DOUBLE t ) const;
		DOUBLE get_length() const { return length; }
	};
}
