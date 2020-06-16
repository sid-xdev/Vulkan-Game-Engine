#pragma once

#include <Defines.hpp>
#include <array>

namespace noxcain
{
	class NxVector3D;

	class NxMatrix4x4
	{
	private:
		constexpr static std::size_t cSize = 16;
		std::array<DOUBLE, cSize> matrix = { { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } };

	public:

		//static std::vector<FLOAT32> packData( std::initializer_list<NxMatrix4x4>);

		constexpr std::size_t gpuSize() const
		{
			return cSize * sizeof( FLOAT32 );
		}

		const std::array<BYTE, sizeof(FLOAT32)*cSize> gpuData() const;
		void gpuData( BYTE* databuffer ) const;

		NxMatrix4x4() = default;
		NxMatrix4x4( const std::array<DOUBLE, cSize> & matrix );
		NxMatrix4x4( const NxVector3D& column0, const NxVector3D& column1, const NxVector3D& column2, const NxVector3D& column3 );
		NxMatrix4x4& operator=( const std::array<DOUBLE, cSize>& matrix );

		NxMatrix4x4 operator*( const NxMatrix4x4& other ) const;

		std::array<DOUBLE, 4> operator*( const std::array<DOUBLE, 4>& vector ) const;
		
		inline const DOUBLE& operator[]( const std::size_t index ) const
		{
			return matrix[index];
		}

		inline DOUBLE& operator[]( const std::size_t index )
		{
			return matrix[index];
		}

		inline const DOUBLE* getColumn( const std::size_t index ) const
		{
			return ( matrix.data() + 4 * index );
		}

		inline DOUBLE* getColumn( const std::size_t index )
		{
			return ( matrix.data() + 4 * index );
		}

		NxMatrix4x4& operator*( DOUBLE scalar );

		NxMatrix4x4& translation( const std::array<DOUBLE, 3>& vector );
		NxMatrix4x4& rotation( const NxVector3D& vector, const DOUBLE angle );
		NxMatrix4x4& rotationX( const DOUBLE angle );
		NxMatrix4x4& rotationY( const DOUBLE angle );
		NxMatrix4x4& rotationZ( const DOUBLE angle );

		NxMatrix4x4 inverse() const;
	};
}
