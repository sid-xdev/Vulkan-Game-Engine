#include <math/Quaternion.hpp>
#include <cmath>

noxcain::NxQuaternion::NxQuaternion( DOUBLE real, const noxcain::NxVector3D& imaginary ) : re_( real ), im_( imaginary )
{

}

noxcain::NxQuaternion noxcain::NxQuaternion::operator-( const NxQuaternion& quaternion ) const
{
	return NxQuaternion( re_ - quaternion.re_, im_ - quaternion.im_ );
}

noxcain::NxQuaternion& noxcain::NxQuaternion::operator-=( const NxQuaternion& quaternion )
{
	re_ -= quaternion.re_;
	im_ -= quaternion.im_;
	return *this;
}

noxcain::NxQuaternion noxcain::NxQuaternion::operator+( const NxQuaternion& quaternion ) const
{
	return NxQuaternion( re_ + quaternion.re_, im_ + quaternion.im_ );
}

noxcain::NxQuaternion& noxcain::NxQuaternion::operator+=( const NxQuaternion& quaternion )
{
	re_ += quaternion.re_;
	im_ += quaternion.im_;
	return *this;
}

noxcain::NxQuaternion noxcain::NxQuaternion::operator*( const NxQuaternion& quaternion ) const
{
	return NxQuaternion
	(
		re_* quaternion.re_ - im_.dot( quaternion.im_ ),
		re_*quaternion.im_ + quaternion.re_*im_ + im_.cross( quaternion.im_ )
	);
}

noxcain::NxQuaternion& noxcain::NxQuaternion::operator*=( const NxQuaternion& quaternion )
{
	return *this = *this * quaternion;
}

noxcain::NxQuaternion noxcain::NxQuaternion::operator*( DOUBLE scalar ) const
{
	return NxQuaternion( re_*scalar, im_*scalar );
}

noxcain::NxQuaternion& noxcain::NxQuaternion::operator*=( DOUBLE scalar )
{
	re_ *= scalar;
	im_ *= scalar;
	return *this;
}

noxcain::NxQuaternion noxcain::NxQuaternion::operator/( DOUBLE scalar ) const
{
	return NxQuaternion( re_/scalar, im_/scalar );
}

noxcain::NxQuaternion& noxcain::NxQuaternion::operator/=( DOUBLE scalar )
{
	re_ /= scalar;
	im_ /= scalar;
	return *this;
}

noxcain::NxQuaternion noxcain::NxQuaternion::GetConjugate() const
{
	return NxQuaternion( re_, -im_ );
}

noxcain::NxQuaternion noxcain::NxQuaternion::GetInverse() const
{
	return GetConjugate()/ GetSquaredNorm();
}

noxcain::DOUBLE noxcain::NxQuaternion::GetNorm() const
{
	return std::sqrt( GetSquaredNorm() );
}

noxcain::DOUBLE noxcain::NxQuaternion::GetSquaredNorm() const
{
	DOUBLE imScalar = im_.GetSquaredLength();
	return re_*re_ + imScalar*imScalar;
}

noxcain::NxQuaternion& noxcain::NxQuaternion::Normalize()
{
	return *this /= GetNorm();
}

noxcain::NxVector3D noxcain::NxQuaternion::RotateVector( const NxVector3D& vector ) const
{
	NxQuaternion temp( -( im_.dot( vector ) ), re_*vector + im_.cross( vector ) );
	return temp.re_*-im_ + re_*temp.im_ + temp.im_.cross( -im_ );
}

noxcain::NxQuaternion noxcain::NxQuaternion::Rotation( DOUBLE angle, const NxVector3D& rotationAxis )
{
	const DOUBLE angleHalf = angle / 2;
	return NxQuaternion( std::cos( angleHalf ), std::sin( angleHalf )*rotationAxis.getNormalizedCopy() );
}

noxcain::NxMatrix4x4 noxcain::NxQuaternion::GetRotationMatrix() const
{
	return
		NxMatrix4x4(
			{
				re_ * re_ + im_[0] * im_[0] - im_[1] * im_[1] - im_[2] * im_[2],
				2 * re_ * im_[2] + 2 * im_[0] * im_[1],
				2 * im_[0] * im_[2] - 2 * re_ * im_[1],
				0.0,

				2 * im_[0] * im_[1] - 2 * re_ * im_[2],
				re_ * re_ - im_[0] * im_[0] + im_[1] * im_[1] - im_[2] * im_[2],
				2 * re_ * im_[0] + 2 * im_[1] * im_[2],
				0.0,

				2 * re_ * im_[1] + 2 * im_[0] * im_[2],
				2 * im_[0] * im_[2] - 2 * re_ * im_[1],
				re_ * re_ - im_[0] * im_[0] - im_[1] * im_[1] + im_[2] * im_[2],
				0.0,

				0.0,0.0,0.0,1.0
			} );
}