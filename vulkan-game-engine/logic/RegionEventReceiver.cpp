#include "RegionEventReceiver.hpp"

void noxcain::RegionalEventRecieverNode::hit_tree( const std::vector<RegionalKeyEvent>& key_events, Stack& parent_stack, Stack& children_stack, Stack& miss_stack )
{
	if( active )
	{
		parent_stack.clear();
		miss_stack.clear();

		for( const RegionalKeyEvent& key_event : key_events )
		{
			if( try_hit( key_event ) )
			{
				parent_stack.emplace_back( *this );
				while( !parent_stack.empty() )
				{
					children_stack.clear();
					for( RegionalEventRecieverNode& parent : parent_stack )
					{
						bool was_last_hit = true;
						for( RegionalEventRecieverNode& child : parent.children )
						{
							if( child.active )
							{
								if( child.try_hit( key_event ) )
								{
									was_last_hit = false;
									children_stack.emplace_back( child );
								}
								else
								{
									miss_stack.emplace_back( child );
								}
							}
						}
						if( was_last_hit )
						{
							parent.hit_node( key_event );
						}
					}
					parent_stack.swap( children_stack );
				}
			}
			else
			{
				miss_stack.emplace_back( *this );
			}

			while( !miss_stack.empty() )
			{
				children_stack.clear();
				for( RegionalEventRecieverNode& missed_node : miss_stack )
				{
					missed_node.miss_node( key_event );
					for( RegionalEventRecieverNode& child : missed_node.children )
					{
						if( child.active )
						{
							children_stack.emplace_back( child );
						}
					}
				}
				miss_stack.swap( children_stack );
			}

			parent_stack.clear();
			children_stack.clear();
			miss_stack.clear();
		}
	}
}

void noxcain::RegionalEventRecieverNode::activate()
{
	active = true;
	if( parent )
	{
		parent->activate();
	}
}

void noxcain::RegionalEventRecieverNode::deactivate()
{
	active = false;
	was_hitted = false;
}

void noxcain::RegionalEventRecieverNode::hit_node( const RegionalKeyEvent& key_event )
{
	was_hitted = hit( key_event );
}

void noxcain::RegionalEventRecieverNode::miss_node( const RegionalKeyEvent& key_event )
{
	was_hitted = !miss( key_event );
}

bool noxcain::RegionalEventExclusivTracer::hit( const std::vector<RegionalKeyEvent>& events )
{
	if( exclusiv )
	{
		for( const auto& reginal_event : events )
		{
			exclusiv->hit_node( reginal_event );
		}
		return true;
	}
	return false;
}
