#include "GameResource.hpp"
#include <algorithm>

noxcain::GameSubResource::GameSubResource( std::size_t resourceSize, SubResourceType resourceType ) : type( resourceType ), size( resourceSize )
{
}

std::size_t noxcain::GameSubResource::getData( void* targetBuffer, std::size_t bufferSize, std::size_t dataOffset ) const
{
	std::size_t readSize = std::min( data.size(), dataOffset + bufferSize ) - dataOffset;
	for( std::size_t index = 0; index < readSize; ++index )
	{
		reinterpret_cast<BYTE*>( targetBuffer )[index] = data[dataOffset + index];
	}
	return readSize;
}

noxcain::SubResourceCollectionIterator::SubResourceCollectionIterator( const std::vector<GameSubResource>& subresources, GameSubResource::SubResourceType type, std::size_t startIndex ) :
	index( startIndex ), collection( subresources ), wantedType( type )
{
	while( index < collection.size() && collection[index].getType() != wantedType )
	{
		++index;
	}
}

bool noxcain::SubResourceCollectionIterator::operator!=( const SubResourceCollectionIterator& other ) const
{
	return other.index != index || other.wantedType != wantedType || &collection != &other.collection;
}

noxcain::SubResourceMetaInfo noxcain::SubResourceCollectionIterator::operator*() const
{
	return { index, collection[index].getSize() };
}

noxcain::SubResourceCollectionIterator& noxcain::SubResourceCollectionIterator::operator++()
{
	do
	{
		++index;
	} while( index < collection.size() && collection[index].getType() != wantedType );
	return *this;
}

noxcain::SubResourceCollectionIterator noxcain::SubResourceCollection::begin() const
{
	return SubResourceCollectionIterator( collection, wantedType, 0 );
}

noxcain::SubResourceCollectionIterator noxcain::SubResourceCollection::end() const
{
	return SubResourceCollectionIterator( collection, wantedType, collection.size() );
}
