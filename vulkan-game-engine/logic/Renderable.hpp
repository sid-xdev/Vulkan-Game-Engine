#pragma once
#include <list>
#include <functional>

namespace noxcain
{
	template<typename T>
	class RenderableList final : private std::list<std::reference_wrapper<const T>>
	{
		using BaseList = typename std::list<std::reference_wrapper<const T>>;
	public:
		using Iterator = typename BaseList::const_iterator;
		
		explicit operator bool() const;
		Iterator begin() const;
		Iterator end() const;
		Iterator insert( const T& renderable );
		void erase( const Iterator& slider_position );
		bool sort( const std::function<bool( const T&, const T& )>& compare );
	private:
		enum class Status
		{
			NEED_NOTHING,
			NEED_SORT,
			NEED_MATCH
		} status;
	};
	
	template<typename T>
	class Renderable
	{
	public:
		using List = RenderableList<T>;
		~Renderable();

		bool is_hidden() const;
		bool is_shown() const;

		void hide();
		void show();
		Renderable<T>& operator=( const Renderable<T>& other ) = delete;
	protected:
		Renderable( const Renderable<T>& other );
		Renderable( Renderable<T>&& other );
		Renderable( List& visibility_list );
	private:
		Renderable<T>::List& renderable_list;
		typename Renderable<T>::List::Iterator list_position;
	};

	template<typename T>
	inline Renderable<T>::Renderable( const Renderable<T>& other ) : renderable_list( other.renderable_list ), list_position( other.renderable_list.end() )
	{
		if( other.is_shown() )
		{
			show();
		}
	}

	template<typename T>
	inline Renderable<T>::Renderable( Renderable<T>&& other ) : renderable_list( other.renderable_list ), list_position( std::move(other.list_position) )
	{
		if( other.is_shown() )
		{
			show();
		}
		other.hide();
	}

	template<typename T>
	inline Renderable<T>::Renderable( List& visibility_list ) : renderable_list( visibility_list ), list_position( visibility_list.end() )
	{
		show();
	}

	template<typename T>
	inline Renderable<T>::~Renderable()
	{
		hide();
	}

	template<typename T>
	inline bool Renderable<T>::is_hidden() const
	{
		return list_position == renderable_list.end();
	}

	template<typename T>
	inline bool Renderable<T>::is_shown() const
	{
		return !is_hidden();
	}

	template<typename T>
	inline void Renderable<T>::hide()
	{
		if( is_shown() )
		{
			renderable_list.erase( list_position );
			list_position = renderable_list.end();
		}
	}

	template<typename T>
	inline void Renderable<T>::show()
	{
		if( is_hidden() )
		{
			list_position = renderable_list.insert( *( static_cast<T*>(this) ) );
		}
	}

	template<typename T>
	inline RenderableList<T>::operator bool() const
	{
		return !BaseList::empty();
	}

	template<typename T>
	inline typename RenderableList<T>::Iterator RenderableList<T>::begin() const
	{
		return BaseList::begin();
	}
	template<typename T>
	inline typename RenderableList<T>::Iterator RenderableList<T>::end() const
	{
		return BaseList::end();
	}
	template<typename T>
	inline typename RenderableList<T>::Iterator RenderableList<T>::insert( const T& renderable )
	{
		status = Status::NEED_SORT;
		return BaseList::emplace( BaseList::begin(), renderable );
	}
	template<typename T>
	inline void RenderableList<T>::erase( const Iterator& slider_position )
	{
		BaseList::erase( slider_position );
		if( status == Status::NEED_NOTHING )
		{
			status = Status::NEED_MATCH;
		}
	}

	template<typename T>
	inline bool RenderableList<T>::sort( const std::function<bool( const T&, const T& )>& compare )
	{
		if( status == Status::NEED_SORT )
		{
			BaseList::sort( compare );
		}
		bool need_match = status != Status::NEED_NOTHING;
		status = Status::NEED_NOTHING;
		return need_match;
	}
}