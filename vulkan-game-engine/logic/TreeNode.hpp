#pragma once
#include <list>
#include <functional>

namespace noxcain
{
	template<typename T>
	class TreeNode
	{
	public:
		void cut_branch();
		void collapse_branch();
		void add_branch( T& child );
		~TreeNode();
	protected:
		T* parent = nullptr;
		std::list<std::reference_wrapper<T>> children;
	};

	template<typename T>
	inline void TreeNode<T>::cut_branch()
	{
		if( parent )
		{
			parent->children.remove_if( [this]( const T& element )
			{
				return this == &element;
			} );
			parent = nullptr;
		}
	}

	template<typename T>
	inline void TreeNode<T>::collapse_branch()
	{
		T* saved_parent = nullptr;
		if( parent )
		{
			saved_parent = parent;
			parent->children.remove_if( [this]( const T& element )
			{
				return this == &element;
			} );
			parent = nullptr;	
		}

		for( T& child : children )
		{
			child.parent = saved_parent;
		}
		children.clear();
	}

	template<typename T>
	inline void TreeNode<T>::add_branch( T& child )
	{
		child.cut_branch();
		child.parent = static_cast<T*>( this );
		children.push_back( child );
	}

	template<typename T>
	inline TreeNode<T>::~TreeNode()
	{
		collapse_branch();
	}
}