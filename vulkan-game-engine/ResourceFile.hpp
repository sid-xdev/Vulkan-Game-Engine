#pragma once
#ifdef __ANDROID__
#include <android/AndroidSurface.hpp>
#else
#include <fstream>
#endif

#include <Defines.hpp>

#include <concepts>

namespace noxcain
{
	/// <summary>
	/// concept for os unspecific file stream
	/// </summary>
	/// <typeparam name="FileType"></typeparam>
	template<class FileType>
	concept IsFileStream = requires ( FileType file, const std::string& string, UINT64 size, char* buffer, std::ios::openmode mode )
	{
		{ file.close() };
		{ file.open( string, mode ) };
		{ file.seekg( size ) };
		{ file.read( buffer, size ) };
		{ file.tellg() };
	}
	&& requires ( const FileType file )
	{
		{ file.is_open() };
	};

	/// <summary>
	/// Wrapper for reading resource files
	/// </summary>
	template<IsFileStream StreamType>
	class ResourceFileStream final : private StreamType
	{
	public:
		/// <summary>
		/// Constructor determine runtime enviroment endianness
		ResourceFileStream();
		~ResourceFileStream();

		/// <summary>
		/// read_fundamental data blocks at current position and interprets them as fundamental type in the correct endianness
		/// </summary>
		template <typename T>
		T read_fundamental();

		/// <summary>
		/// special read_fundamental for compact float in font file
		/// </summary>
		DOUBLE read_f2dot14();

		/// <summary>
		/// open data stream to resource
		/// </summary>
		/// <param name="resource_path">resource path</param>
		/// <param name="is_resource_little_endian">is fundamental data written in little endian</param>
		/// <returns>true if successful</returns>
		bool open( const std::string& resource_path, bool is_resource_little_endian = true );

		/// <summary>
		/// close data stream
		/// </summary>
		void close();

		/// <summary>
		/// get current stream position
		/// </summary>
		/// <returns>current stream position</returns>
		UINT64 get_position();

		/// <summary>
		/// set current stream position
		/// </summary>
		/// <returns>ref to stream object</returns>
		ResourceFileStream<StreamType>& set_position( UINT64 position );

	private:
		bool need_endianness_correction = false;
	};

	template<IsFileStream StreamType>
	ResourceFileStream<StreamType>::ResourceFileStream()
	{
	}

	template<IsFileStream StreamType>
	DOUBLE ResourceFileStream<StreamType>::read_f2dot14()
	{
		char word[2];
		StreamType::read( word, 2 );
		signed char front = 0;
		front = front | ( word[0] & 0x80 ) | ( ( word[0] & 0x40 ) >> 6 );
		word[0] &= 0x3F;
		if( need_endianness_correction )
		{
			char temp = word[1];
			word[1] = word[0];
			word[0] = temp;
		}

		UINT16 frac;
		memcpy( &frac, word, 2 );

		return DOUBLE( front ) + ( frac / 16384 );
	}

	template<IsFileStream StreamType>
	template<typename T>
	inline T ResourceFileStream<StreamType>::read_fundamental()
	{
		T value;
		constexpr std::streamsize size = sizeof( T );
		StreamType::read( reinterpret_cast<char*>( &value ), size );

		if( need_endianness_correction )
		{
			for( std::size_t index = 0; index < size / 2; ++index )
			{
				BYTE* bytes = reinterpret_cast<BYTE*>( &value );
				BYTE byte = bytes[index];
				bytes[index] = bytes[size - 1 - index];
				bytes[size - 1 - index] = byte;
			}
		}
		return value;
	}

	template<IsFileStream StreamType>
	ResourceFileStream<StreamType>::~ResourceFileStream()
	{
	}

	template<IsFileStream StreamType>
	bool ResourceFileStream<StreamType>::open( const std::string& resource_path, bool is_resource_little_endian )
	{
		UINT16 a = 1;
		static bool system_is_little_endian = ( reinterpret_cast<BYTE*>( &a ) )[0] == 1;

		need_endianness_correction = is_resource_little_endian != system_is_little_endian;
		
		StreamType::open( resource_path, std::ios::binary );
		return StreamType::is_open();
	}

	template<IsFileStream StreamType>
	void ResourceFileStream<StreamType>::close()
	{
		StreamType::close();
	}

	template<IsFileStream StreamType>
	UINT64 ResourceFileStream<StreamType>::get_position()
	{
		return UINT64( StreamType::tellg() );
	}

	template<IsFileStream StreamType>
	ResourceFileStream<StreamType>& noxcain::ResourceFileStream<StreamType>::set_position( UINT64 position )
	{
		StreamType::seekg( position );
		return *this;
	}

#ifdef __ANDROID__
	using ResourceFile = ResourceFileStream<AndroidFile>
#else
	using ResourceFile = ResourceFileStream<std::ifstream>;
#endif

}
