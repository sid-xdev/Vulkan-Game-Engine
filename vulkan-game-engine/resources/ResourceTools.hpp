#pragma once
#include <Defines.hpp>

#include<istream>
namespace noxcain
{
	class EndianSafeStream : public std::istream
	{
	public:
		EndianSafeStream( std::streambuf* sb, bool switch_endian );

		template<typename T>
		T read()
		{
			T value;
			constexpr UINT32 size = sizeof( T );
			std::istream::read( reinterpret_cast<char*>( &value ), size );

			if( switch_endian )
			{
				for( UINT32 index = 0; index < size / 2; ++index )
				{
					BYTE* bytes = reinterpret_cast<BYTE*>( &value );
					BYTE byte = bytes[index];
					bytes[index] = bytes[size - 1 - index];
					bytes[size - 1 - index] = byte;
				}
			}
			return value;
		}

		inline EndianSafeStream& seekg( std::streampos offset )
		{
			std::istream::seekg( offset );
			return *this;
		}
	private:
		bool switch_endian = false;
	};
}
