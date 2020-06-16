#pragma once
#include <Defines.hpp>
#include <array>

namespace noxcain
{	
	constexpr DOUBLE PI = 3.141592653589793238L;
	
	class NxMatrix4x4;

	class NxVector3D
	{
	protected:
		std::array<DOUBLE,3> points;

	public:
		NxVector3D();
		NxVector3D( DOUBLE x, DOUBLE y, DOUBLE z );

		NxVector3D( const NxVector3D& vector );

		~NxVector3D();

		bool operator==( const NxVector3D& vector ) const;
		bool operator!=( const NxVector3D& vector ) const;

		operator const std::array<DOUBLE, 3>& ( ) const
		{
			return points;
		}

		NxVector3D& operator=( const NxVector3D& vector );

		NxVector3D operator-() const;

		DOUBLE dot( const NxVector3D& vector )const;
		
		NxVector3D operator*( DOUBLE scalar )const;
		NxVector3D& operator*=( DOUBLE scalar );

		NxVector3D operator*( const NxVector3D& vector )const;
		NxVector3D& operator*=( const NxVector3D& vector );

		NxVector3D operator/( DOUBLE scalar )const;
		NxVector3D& operator/=( DOUBLE scalar );

		NxVector3D operator/( const NxVector3D& vector )const;
		NxVector3D& operator/=( const NxVector3D& vector );

		NxVector3D operator+( const NxVector3D& vector )const;
		NxVector3D& operator+=( const NxVector3D& vector );

		NxVector3D operator+( DOUBLE scalar )const;
		NxVector3D& operator+=( DOUBLE scalar );

		NxVector3D operator-( const NxVector3D& vector )const;
		NxVector3D& operator-=( const NxVector3D& vector );

		NxVector3D operator-( DOUBLE scalar )const;
		NxVector3D& operator-=( DOUBLE scalar );

		NxVector3D cross( const NxVector3D& vector ) const;

		DOUBLE get_length() const;
		DOUBLE GetSquaredLength() const;

		NxVector3D getNormalizedCopy() const;
		NxVector3D& normalize();

		inline void setX( DOUBLE x )
		{
			points[0] = x;
		}

		inline void setY( DOUBLE y )
		{
			points[1] = y;
		}

		inline void setZ( DOUBLE z )
		{
			points[2] = z;
		}

		inline DOUBLE getX() const
		{
			return points[0];
		}

		inline DOUBLE getY() const
		{
			return points[1];
		}

		inline DOUBLE getZ() const
		{
			return points[2];
		}

		inline const DOUBLE& operator[]( unsigned int index ) const
		{
			return points[index];
		}

		inline DOUBLE& operator[]( unsigned int index )
		{
			return points[index];
		}
	};

	NxVector3D operator*( DOUBLE scalar, const NxVector3D& vector );
	NxVector3D operator+( DOUBLE scalar, const NxVector3D& vector );

	NxVector3D inverseRotation( const NxVector3D& xAxis, const NxVector3D& yAxis, const NxVector3D& zAxis, const NxVector3D& sourcePoint );
}
