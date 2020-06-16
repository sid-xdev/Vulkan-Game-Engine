#pragma once

#include <Defines.hpp>
#include <math/Vector.hpp>
#include <math/Matrix.hpp>

namespace noxcain
{
	class NxQuaternion
	{
	private:
		DOUBLE re_;
		NxVector3D im_;
	public:
		NxQuaternion( DOUBLE real, const NxVector3D& imaginary );

		NxQuaternion operator-( const NxQuaternion& quaternion ) const;
		NxQuaternion& operator-=( const NxQuaternion& quaternion );

		NxQuaternion operator+( const NxQuaternion& quaternion ) const;
		NxQuaternion& operator+=( const NxQuaternion& quaternion );

		NxQuaternion operator*( const NxQuaternion& quaternion ) const;
		NxQuaternion& operator*=( const NxQuaternion& quaternion );

		NxQuaternion operator*( DOUBLE scalar ) const;
		NxQuaternion& operator*=( DOUBLE scalar );

		NxQuaternion operator/( DOUBLE scalar ) const;
		NxQuaternion& operator/=( DOUBLE scalar );

		inline const NxVector3D& GetImaginaryPart() const
		{
			return im_;
		}

		inline DOUBLE GetRealPart() const
		{
			return re_;
		}

		NxQuaternion GetConjugate() const;
		NxQuaternion GetInverse() const;
		DOUBLE GetNorm() const;
		DOUBLE GetSquaredNorm() const;

		NxQuaternion& Normalize();

		NxVector3D RotateVector( const NxVector3D& vector ) const;

		static NxQuaternion Rotation( DOUBLE angle, const NxVector3D& rotationAxis );

		NxMatrix4x4 GetRotationMatrix() const;
	};
}