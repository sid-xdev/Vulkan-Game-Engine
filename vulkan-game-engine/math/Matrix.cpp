#include <math/Matrix.hpp>
#include <math/Vector.hpp>

#include <cmath>
#include <algorithm>

const std::array<noxcain::BYTE, 64> noxcain::NxMatrix4x4::gpuData() const
{
	std::array<noxcain::BYTE, 64> result;

	FLOAT32* target = reinterpret_cast<FLOAT32*>( result.data() );
	for( std::size_t index = 0; index < matrix.size(); ++index )
	{
		target[index] = FLOAT32( matrix[index] );
	}

	return result;
}

void noxcain::NxMatrix4x4::gpuData( BYTE* databuffer ) const
{
	FLOAT32* floatBuffer = reinterpret_cast<FLOAT32*>( databuffer );
	for( std::size_t index = 0; index < matrix.size(); ++index )
	{
		floatBuffer[index] = FLOAT32(matrix[index]);
	}
}

noxcain::NxMatrix4x4::NxMatrix4x4( const std::array<DOUBLE, 16>& matrix ) : matrix( matrix )
{
}

noxcain::NxMatrix4x4::NxMatrix4x4( const NxVector3D& column0, const NxVector3D& column1, const NxVector3D& column2, const NxVector3D& column3 )
{
	matrix[0] = column0[0];
	matrix[1] = column0[1];
	matrix[2] = column0[2];
	matrix[3] = 0.0;

	matrix[4] = column1[0];
	matrix[5] = column1[1];
	matrix[6] = column1[2];
	matrix[7] = 0.0;

	matrix[8] = column2[0];
	matrix[9] = column2[1];
	matrix[10] = column2[2];
	matrix[11] = 0.0;

	matrix[12] = column3[0];
	matrix[13] = column3[1];
	matrix[14] = column3[2];
	matrix[15] = 1.0;
}

noxcain::NxMatrix4x4 & noxcain::NxMatrix4x4::operator=( const std::array<DOUBLE, 16>& matrix )
{
	this->matrix = matrix;
	return *this;
}

noxcain::NxMatrix4x4 noxcain::NxMatrix4x4::operator*( const NxMatrix4x4 & other ) const
{
	return NxMatrix4x4(
		{
			matrix[0] * other.matrix[0] + matrix[4] * other.matrix[1] + matrix[8] * other.matrix[2] + matrix[12] * other.matrix[3],
			matrix[1] * other.matrix[0] + matrix[5] * other.matrix[1] + matrix[9] * other.matrix[2] + matrix[13] * other.matrix[3],
			matrix[2] * other.matrix[0] + matrix[6] * other.matrix[1] + matrix[10] * other.matrix[2] + matrix[14] * other.matrix[3],
			matrix[3] * other.matrix[0] + matrix[7] * other.matrix[1] + matrix[11] * other.matrix[2] + matrix[15] * other.matrix[3],

			matrix[0] * other.matrix[4] + matrix[4] * other.matrix[5] + matrix[8] * other.matrix[6] + matrix[12] * other.matrix[7],
			matrix[1] * other.matrix[4] + matrix[5] * other.matrix[5] + matrix[9] * other.matrix[6] + matrix[13] * other.matrix[7],
			matrix[2] * other.matrix[4] + matrix[6] * other.matrix[5] + matrix[10] * other.matrix[6] + matrix[14] * other.matrix[7],
			matrix[3] * other.matrix[4] + matrix[7] * other.matrix[5] + matrix[11] * other.matrix[6] + matrix[15] * other.matrix[7],

			matrix[0] * other.matrix[8] + matrix[4] * other.matrix[9] + matrix[8] * other.matrix[10] + matrix[12] * other.matrix[11],
			matrix[1] * other.matrix[8] + matrix[5] * other.matrix[9] + matrix[9] * other.matrix[10] + matrix[13] * other.matrix[11],
			matrix[2] * other.matrix[8] + matrix[6] * other.matrix[9] + matrix[10] * other.matrix[10] + matrix[14] * other.matrix[11],
			matrix[3] * other.matrix[8] + matrix[7] * other.matrix[9] + matrix[11] * other.matrix[10] + matrix[15] * other.matrix[11],

			matrix[0] * other.matrix[12] + matrix[4] * other.matrix[13] + matrix[8] * other.matrix[14] + matrix[12] * other.matrix[15],
			matrix[1] * other.matrix[12] + matrix[5] * other.matrix[13] + matrix[9] * other.matrix[14] + matrix[13] * other.matrix[15],
			matrix[2] * other.matrix[12] + matrix[6] * other.matrix[13] + matrix[10] * other.matrix[14] + matrix[14] * other.matrix[15],
			matrix[3] * other.matrix[12] + matrix[7] * other.matrix[13] + matrix[11] * other.matrix[14] + matrix[15] * other.matrix[15],
		} );
}

std::array<noxcain::DOUBLE, 4> noxcain::NxMatrix4x4::operator*( const std::array<DOUBLE, 4>& vector ) const
{
	
	return
	{
		matrix[0] * vector[0] + matrix[4] * vector[1] + matrix[8] * vector[2] + matrix[12] * vector[3],
		matrix[1] * vector[0] + matrix[5] * vector[1] + matrix[9] * vector[2] + matrix[13] * vector[3],
		matrix[2] * vector[0] + matrix[6] * vector[1] + matrix[10] * vector[2] + matrix[14] * vector[3],
		matrix[3] * vector[0] + matrix[7] * vector[1] + matrix[11] * vector[2] + matrix[15] * vector[3]
	};
	///std::array<FLOAT32, 4> result =
	//return result;
}

noxcain::NxMatrix4x4& noxcain::NxMatrix4x4::operator*( DOUBLE scalar )
{
	for( DOUBLE& value : matrix )
	{
		value *= scalar;
	}
	return *this;
}

noxcain::NxMatrix4x4 & noxcain::NxMatrix4x4::translation( const std::array<DOUBLE, 3>& vector )
{
	return *this = NxMatrix4x4(
		{
			1,0,0,0,
			0,1,0,0,
			0,0,1,0,
			vector[0],vector[1],vector[2],1
		} ) * matrix;
}

noxcain::NxMatrix4x4 & noxcain::NxMatrix4x4::rotation( const NxVector3D& vector, const DOUBLE angle )
{
	DOUBLE cos = std::cos( angle );
	DOUBLE sin = std::sin( angle );
	
	
	return *this = NxMatrix4x4(
		{
			vector[0] * vector[0] * ( 1 - cos ) + cos,
			vector[1] * vector[0] * ( 1 - cos ) + vector[2] * sin,
			vector[2] * vector[0] * ( 1 - cos ) - vector[1] * sin,
			0,

			vector[0] * vector[1] * ( 1 - cos ) - vector[2] * sin,
			vector[1] * vector[1] * ( 1 - cos ) + cos,
			vector[2] * vector[1] * ( 1 - cos ) + vector[0] * sin,
			0,

			vector[0] * vector[2] * ( 1 - cos ) + vector[1] * sin,
			vector[1] * vector[2] * ( 1 - cos ) - vector[0] * sin,
			vector[2] * vector[2] * ( 1 - cos ) + cos,
			0,

			0, 0, 0, 1
		} ) * matrix;
}

noxcain::NxMatrix4x4& noxcain::NxMatrix4x4::rotationX( const DOUBLE angle )
{
	return *this = NxMatrix4x4(
		{
			1.0, 0.0, 0.0, 0.0,
			0.0, std::cos( angle ), std::sin( angle ), 0.0,
			0.0, -std::sin( angle ), cos( angle ), 0.0,
			0.0, 0.0, 0.0, 1.0
		} ) * matrix;
}

noxcain::NxMatrix4x4& noxcain::NxMatrix4x4::rotationY( const DOUBLE angle )
{
	return *this = NxMatrix4x4(
		{
			std::cos( angle ), 0.0, std::sin( angle ), 0.0,
			0.0, 1.0, 0.0, 0.0,
			-std::sin( angle ), 0.0, cos( angle ), 0.0,
			0.0, 0.0, 0.0, 1.0
		} ) * matrix;
}

noxcain::NxMatrix4x4 & noxcain::NxMatrix4x4::rotationZ( const DOUBLE angle )
{
	return *this = NxMatrix4x4(
		{
			std::cos( angle ), std::sin( angle ), 0, 0,
			-std::sin( angle ), cos( angle ), 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		} )*matrix;
}

noxcain::NxMatrix4x4 noxcain::NxMatrix4x4::inverse() const
{
	double det;
	NxMatrix4x4 inv;

	inv[0] = matrix[5] * matrix[10] * matrix[15] -
		matrix[5] * matrix[11] * matrix[14] -
		matrix[9] * matrix[6] * matrix[15] +
		matrix[9] * matrix[7] * matrix[14] +
		matrix[13] * matrix[6] * matrix[11] -
		matrix[13] * matrix[7] * matrix[10];

	inv[4] = -matrix[4] * matrix[10] * matrix[15] +
		matrix[4] * matrix[11] * matrix[14] +
		matrix[8] * matrix[6] * matrix[15] -
		matrix[8] * matrix[7] * matrix[14] -
		matrix[12] * matrix[6] * matrix[11] +
		matrix[12] * matrix[7] * matrix[10];

	inv[8] = matrix[4] * matrix[9] * matrix[15] -
		matrix[4] * matrix[11] * matrix[13] -
		matrix[8] * matrix[5] * matrix[15] +
		matrix[8] * matrix[7] * matrix[13] +
		matrix[12] * matrix[5] * matrix[11] -
		matrix[12] * matrix[7] * matrix[9];

	inv[12] = -matrix[4] * matrix[9] * matrix[14] +
		matrix[4] * matrix[10] * matrix[13] +
		matrix[8] * matrix[5] * matrix[14] -
		matrix[8] * matrix[6] * matrix[13] -
		matrix[12] * matrix[5] * matrix[10] +
		matrix[12] * matrix[6] * matrix[9];

	inv[1] = -matrix[1] * matrix[10] * matrix[15] +
		matrix[1] * matrix[11] * matrix[14] +
		matrix[9] * matrix[2] * matrix[15] -
		matrix[9] * matrix[3] * matrix[14] -
		matrix[13] * matrix[2] * matrix[11] +
		matrix[13] * matrix[3] * matrix[10];

	inv[5] = matrix[0] * matrix[10] * matrix[15] -
		matrix[0] * matrix[11] * matrix[14] -
		matrix[8] * matrix[2] * matrix[15] +
		matrix[8] * matrix[3] * matrix[14] +
		matrix[12] * matrix[2] * matrix[11] -
		matrix[12] * matrix[3] * matrix[10];

	inv[9] = -matrix[0] * matrix[9] * matrix[15] +
		matrix[0] * matrix[11] * matrix[13] +
		matrix[8] * matrix[1] * matrix[15] -
		matrix[8] * matrix[3] * matrix[13] -
		matrix[12] * matrix[1] * matrix[11] +
		matrix[12] * matrix[3] * matrix[9];

	inv[13] = matrix[0] * matrix[9] * matrix[14] -
		matrix[0] * matrix[10] * matrix[13] -
		matrix[8] * matrix[1] * matrix[14] +
		matrix[8] * matrix[2] * matrix[13] +
		matrix[12] * matrix[1] * matrix[10] -
		matrix[12] * matrix[2] * matrix[9];

	inv[2] = matrix[1] * matrix[6] * matrix[15] -
		matrix[1] * matrix[7] * matrix[14] -
		matrix[5] * matrix[2] * matrix[15] +
		matrix[5] * matrix[3] * matrix[14] +
		matrix[13] * matrix[2] * matrix[7] -
		matrix[13] * matrix[3] * matrix[6];

	inv[6] = -matrix[0] * matrix[6] * matrix[15] +
		matrix[0] * matrix[7] * matrix[14] +
		matrix[4] * matrix[2] * matrix[15] -
		matrix[4] * matrix[3] * matrix[14] -
		matrix[12] * matrix[2] * matrix[7] +
		matrix[12] * matrix[3] * matrix[6];

	inv[10] = matrix[0] * matrix[5] * matrix[15] -
		matrix[0] * matrix[7] * matrix[13] -
		matrix[4] * matrix[1] * matrix[15] +
		matrix[4] * matrix[3] * matrix[13] +
		matrix[12] * matrix[1] * matrix[7] -
		matrix[12] * matrix[3] * matrix[5];

	inv[14] = -matrix[0] * matrix[5] * matrix[14] +
		matrix[0] * matrix[6] * matrix[13] +
		matrix[4] * matrix[1] * matrix[14] -
		matrix[4] * matrix[2] * matrix[13] -
		matrix[12] * matrix[1] * matrix[6] +
		matrix[12] * matrix[2] * matrix[5];

	inv[3] = -matrix[1] * matrix[6] * matrix[11] +
		matrix[1] * matrix[7] * matrix[10] +
		matrix[5] * matrix[2] * matrix[11] -
		matrix[5] * matrix[3] * matrix[10] -
		matrix[9] * matrix[2] * matrix[7] +
		matrix[9] * matrix[3] * matrix[6];

	inv[7] = matrix[0] * matrix[6] * matrix[11] -
		matrix[0] * matrix[7] * matrix[10] -
		matrix[4] * matrix[2] * matrix[11] +
		matrix[4] * matrix[3] * matrix[10] +
		matrix[8] * matrix[2] * matrix[7] -
		matrix[8] * matrix[3] * matrix[6];

	inv[11] = -matrix[0] * matrix[5] * matrix[11] +
		matrix[0] * matrix[7] * matrix[9] +
		matrix[4] * matrix[1] * matrix[11] -
		matrix[4] * matrix[3] * matrix[9] -
		matrix[8] * matrix[1] * matrix[7] +
		matrix[8] * matrix[3] * matrix[5];

	inv[15] = matrix[0] * matrix[5] * matrix[10] -
		matrix[0] * matrix[6] * matrix[9] -
		matrix[4] * matrix[1] * matrix[10] +
		matrix[4] * matrix[2] * matrix[9] +
		matrix[8] * matrix[1] * matrix[6] -
		matrix[8] * matrix[2] * matrix[5];

	det = matrix[0] * inv[0] + matrix[1] * inv[4] + matrix[2] * inv[8] + matrix[3] * inv[12];

	if( det == 0 ) return NxMatrix4x4();

	det = 1.0 / det;

	return inv * det;
}
