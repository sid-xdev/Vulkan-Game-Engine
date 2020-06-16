#pragma once
#include <Defines.hpp>
#include <vector>

namespace noxcain
{
	struct SubResourceMetaInfo
	{
		std::size_t id;
		std::size_t size;
	};

	class GameSubResource
	{
		friend class ResourceEngine;
	public:
		enum class SubResourceType : std::size_t
		{
			eIndexBuffer,
			eVertexBuffer,
			eStorageBuffer,
		};
	private:
		const std::size_t size;
		const SubResourceType type;
		std::vector<BYTE> data;

		void setData( std::vector<BYTE>&& rawData )
		{
			data = std::move( rawData );
		}

	public:
		
		GameSubResource( std::size_t resourceSize, SubResourceType resourceType );
		
		SubResourceType getType() const
		{
			return type;
		}
		
		std::size_t getSize() const
		{
			return size;
		}

		std::size_t getData( void* targetBuffer, std::size_t bufferSize, std::size_t dataOffset = 0 ) const;
	};

	class SubResourceCollectionIterator
	{
	private:
		const std::vector<GameSubResource>& collection;
		const GameSubResource::SubResourceType wantedType;
		std::size_t index;
	public:
		SubResourceCollectionIterator( const std::vector<GameSubResource>& subresources, GameSubResource::SubResourceType type, std::size_t startIndex );
		bool operator!=( const SubResourceCollectionIterator& other ) const;
		SubResourceMetaInfo operator*() const;

		SubResourceCollectionIterator& operator++();
	};

	class SubResourceCollection
	{
	private:
		const std::vector<GameSubResource>& collection;
		const GameSubResource::SubResourceType wantedType;
	public:
		SubResourceCollection( const std::vector<GameSubResource>& subresources, GameSubResource::SubResourceType type ) : collection( subresources ), wantedType( type ) {}
		SubResourceCollectionIterator begin() const;
		SubResourceCollectionIterator end() const;
	};
}
