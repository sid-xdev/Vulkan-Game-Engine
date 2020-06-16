#pragma once
#include <Defines.hpp>
#include <logic/TreeNode.hpp>

#include <vector>
#include <functional>

namespace noxcain
{
	class RegionalKeyEvent
	{
	public:
		enum class Events
		{
			NONE,
			DOWN,
			UP
		};

		enum class KeyCodes
		{
			NONE = 0x00,
			LEFT_MOUSE = 0x01,
			RIGHT_MOUSE = 0x02,
			MIDDLE_MOUSE = 0x04,
			EXTRA_MOUSE_1 = 0x08,
			EXTRA_MOUSE_2 = 0x10,
		};

		RegionalKeyEvent() = default;
		RegionalKeyEvent( KeyCodes key_code_, Events key_event_, INT32 x, INT32 y ) : key_event( key_event_ ), key_code( key_code_ ), x_position( x ), y_position( y )
		{
		}

		KeyCodes get_key_code() const
		{
			return key_code;
		}

		Events get_event() const
		{
			return key_event;
		}

		INT32 get_x_position() const
		{
			return x_position;
		}

		INT32 get_y_position() const
		{
			return y_position;
		}

		void set_key_event( Events key_event_ )
		{
			key_event = key_event_;
		}

		void set_key_code( KeyCodes key_code_ )
		{
			key_code = key_code_;
		}

		void set_key_code( INT32 x, INT32 y )
		{
			x_position = x;
			y_position = y;
		}

	private:
		Events key_event = Events::NONE;
		KeyCodes key_code = KeyCodes::NONE;
		INT32 x_position = 0;
		INT32 y_position = 0;
	};

	class RegionalEventRecieverNode : public TreeNode<RegionalEventRecieverNode>
	{
	public:
		friend class RegionalEventExclusivTracer;

		using Stack = std::vector<std::reference_wrapper<RegionalEventRecieverNode>>;
		void hit_tree( const std::vector<RegionalKeyEvent>& key_events, Stack& parent_stack, Stack& children_stack, Stack& miss_stack );

		void activate();
		void deactivate();

		virtual ~RegionalEventRecieverNode()
		{
		}

	protected:
		bool active = true;
		bool was_hitted = false;

		virtual bool try_hit( const RegionalKeyEvent& key_event ) const = 0;
		virtual bool hit( const RegionalKeyEvent& key_event )
		{
			return true;
		};
		virtual bool miss( const RegionalKeyEvent& key_event )
		{
			return true;
		};
	private:
		void hit_node( const RegionalKeyEvent& key_event );
		void miss_node( const RegionalKeyEvent& key_event );
	};

	class RegionalEventExclusivTracer
	{
	public:
		explicit operator bool() const
		{
			return exclusiv;
		}
		void set_exclusiv( RegionalEventRecieverNode& exclusiv_node )
		{
			exclusiv = &exclusiv_node;
		}
		void remove()
		{
			exclusiv = nullptr;
		}

		bool hit( const std::vector<RegionalKeyEvent>& events );

	private:
		RegionalEventRecieverNode* exclusiv = nullptr;

	};
}