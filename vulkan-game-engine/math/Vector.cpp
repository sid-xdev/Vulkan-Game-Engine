#include <math/Vector.hpp>
#include <cmath>

noxcain::NxVector3D::NxVector3D() : points( {0, 0, 0} )
{
}

noxcain::NxVector3D::NxVector3D( DOUBLE x, DOUBLE y, DOUBLE z ) : points( { x, y, z } )
{
}

noxcain::NxVector3D::NxVector3D( const NxVector3D & vector ) : points( { 0, 0, 0 } )
{
	points[0] = vector.points[0];
	points[1] = vector.points[1];
	points[2] = vector.points[2];
}

noxcain::NxVector3D::~NxVector3D()
{
}

bool noxcain::NxVector3D::operator==( const NxVector3D & vector ) const
{
	return points[0] == vector.points[0] && points[1] == vector.points[1] && points[2] == vector.points[2];
}

bool noxcain::NxVector3D::operator!=( const NxVector3D & vector ) const
{
	return !(*this==vector);
}

noxcain::NxVector3D & noxcain::NxVector3D::operator=( const NxVector3D & vector )
{
	points[0] = vector.points[0];
	points[1] = vector.points[1];
	points[2] = vector.points[2];
	return *this;
}

noxcain::NxVector3D noxcain::NxVector3D::operator-() const
{
	return NxVector3D( -points[0], -points[1], -points[2] );
}

noxcain::DOUBLE noxcain::NxVector3D::dot( const NxVector3D & vector ) const
{
	return points[0] *vector.points[0] + points[1]*vector.points[1] + points[2]*vector.points[2];
}

noxcain::NxVector3D noxcain::NxVector3D::operator*( const NxVector3D& vector ) const
{
	return NxVector3D( points[0]*vector.points[0], points[1]*vector.points[1], points[2]*vector.points[2] );
}

noxcain::NxVector3D& noxcain::NxVector3D::operator*=( const NxVector3D& vector )
{
	points[0] *= vector.points[0];
	points[1] *= vector.points[1];
	points[2] *= vector.points[2];
	return *this;
}

noxcain::NxVector3D noxcain::NxVector3D::operator*( DOUBLE scalar ) const
{
	return NxVector3D( scalar*points[0], scalar*points[1], scalar*points[2] );
}

noxcain::NxVector3D & noxcain::NxVector3D::operator*=( DOUBLE scalar )
{
	points[0] *= scalar;
	points[1] *= scalar;
	points[2] *= scalar;
	return *this;
}

noxcain::NxVector3D noxcain::NxVector3D::operator/( DOUBLE scalar ) const
{
	return NxVector3D( points[0] / scalar, points[1] / scalar, points[2] / scalar );
}

noxcain::NxVector3D & noxcain::NxVector3D::operator/=( DOUBLE scalar )
{
	points[0] /= scalar;
	points[1] /= scalar;
	points[2] /= scalar;
	return *this;
}

noxcain::NxVector3D noxcain::NxVector3D::operator/( const NxVector3D & vector ) const
{
	return NxVector3D( points[0]/vector.points[0], points[1]/vector.points[1], points[2]/vector.points[2] );
}

noxcain::NxVector3D& noxcain::NxVector3D::operator/=( const NxVector3D & vector )
{
	points[0] /= vector.points[0];
	points[1] /= vector.points[1];
	points[2] /= vector.points[2];
	return *this;
}

noxcain::NxVector3D noxcain::NxVector3D::operator+( const NxVector3D& vector ) const
{
	return NxVector3D( points[0] + vector.points[0], points[1] + vector.points[1], points[2] + vector.points[2] );
}

noxcain::NxVector3D & noxcain::NxVector3D::operator+=( const NxVector3D& vector )
{
	points[0] += vector.points[0];
	points[1] += vector.points[1];
	points[2] += vector.points[2];
	return *this;
}

noxcain::NxVector3D noxcain::NxVector3D::operator+( DOUBLE scalar ) const
{
	return NxVector3D( points[0] + scalar, points[1] + scalar, points[2] + scalar );
}

noxcain::NxVector3D & noxcain::NxVector3D::operator+=( DOUBLE scalar )
{
	points[0] += scalar;
	points[1] += scalar;
	points[2] += scalar;
	return *this;
}

noxcain::NxVector3D noxcain::NxVector3D::operator-( const NxVector3D& vector ) const
{
	return NxVector3D( points[0] - vector.points[0], points[1] - vector.points[1], points[2] - vector.points[2] );
}

noxcain::NxVector3D & noxcain::NxVector3D::operator-=( const NxVector3D& vector )
{
	points[0] -= vector.points[0];
	points[1] -= vector.points[1];
	points[2] -= vector.points[2];
	return *this;
}

noxcain::NxVector3D noxcain::NxVector3D::operator-( DOUBLE scalar ) const
{
	return NxVector3D( points[0] - scalar, points[1] - scalar, points[2] - scalar );
}

noxcain::NxVector3D & noxcain::NxVector3D::operator-=( DOUBLE scalar )
{
	points[0] -= scalar;
	points[1] -= scalar;
	points[2] -= scalar;
	return *this;
}

noxcain::NxVector3D noxcain::NxVector3D::cross( const NxVector3D & vector ) const
{
	return NxVector3D( points[1]*vector.points[2] - points[2] * vector.points[1], points[2]*vector.points[0] - points[0] * vector.points[2], points[0]*vector.points[1] - points[1] * vector.points[0] );
}

noxcain::DOUBLE noxcain::NxVector3D::get_length() const
{
	return std::sqrt( GetSquaredLength() );
}

noxcain::DOUBLE noxcain::NxVector3D::GetSquaredLength() const
{
	return points[0]*points[0] + points[1]*points[1] + points[2]*points[2];
}

noxcain::NxVector3D noxcain::NxVector3D::getNormalizedCopy() const
{
	DOUBLE length = get_length();
	NxVector3D result = length > 0 ? *this / length : NxVector3D();
	return result;
}

noxcain::NxVector3D & noxcain::NxVector3D::normalize()
{
	DOUBLE length = get_length();
	if( length > 0 ) *this /= length;
	return *this;
}

noxcain::NxVector3D noxcain::operator*( DOUBLE scalar, const NxVector3D & vector )
{
	return vector*scalar;
}

noxcain::NxVector3D noxcain::operator+( DOUBLE scalar, const NxVector3D & vector )
{
	return vector+scalar;
}

noxcain::NxVector3D noxcain::inverseRotation( const NxVector3D& xAxis, const NxVector3D& yAxis, const NxVector3D& zAxis, const NxVector3D& sourcePoint )
{
	const DOUBLE ae = xAxis[0] * yAxis[1];
	const DOUBLE bf = yAxis[0] * zAxis[1];
	const DOUBLE cd = zAxis[0] * xAxis[1];
	const DOUBLE bd = -yAxis[0] * xAxis[1];
	const DOUBLE af = -xAxis[0] * zAxis[1];
	const DOUBLE ce = -zAxis[0] * yAxis[1];

	const DOUBLE det = 1.0 / ( ae * zAxis[2] + bf * xAxis[2] + cd * yAxis[2] + bd * zAxis[2] + af * yAxis[2] + ce * xAxis[2] );

	const NxVector3D row1( det*( yAxis[1] * zAxis[2] - zAxis[1] * yAxis[2] ), det*( zAxis[0] * yAxis[2] - yAxis[0] * zAxis[2] ), det*( bf + ce ) );
	const NxVector3D row2( det*( zAxis[1] * xAxis[2] - xAxis[1] * zAxis[2] ), det*( xAxis[0] * zAxis[2] - zAxis[0] * xAxis[2] ), det*( cd + af ) );
	const NxVector3D row3( det*( xAxis[1] * yAxis[2] - yAxis[1] * xAxis[2] ), det*( yAxis[0] * xAxis[2] - xAxis[0] * yAxis[2] ), det*( ae + bd ) );

	return NxVector3D( row1.dot( sourcePoint ), row2.dot( sourcePoint ), row3.dot( sourcePoint ) );
}
