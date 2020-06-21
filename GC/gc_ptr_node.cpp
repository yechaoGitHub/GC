#include "gc_ptr_node.h"
#include "windows.h"
#include "gc_ptr.hpp"

gc_ptr_node::gc_ptr_node() :
	ref_count(0),
	gc_num(-1),
	gc_pos(-1)
{
	gc_marks.assign(1, gc_mark());
}

gc_ptr_node::gc_ptr_node(uint32_t mark_count) :
	ref_count(0)
{
	gc_marks.assign(mark_count, gc_mark());
}

gc_ptr_node::~gc_ptr_node()
{
}

void gc_ptr_node::add_ref()
{
	InterlockedIncrement64(&ref_count);
}

void gc_ptr_node::reduce_ref()
{
	InterlockedDecrement64(&ref_count);
}

bool gc_ptr_node::add_child(gc_ptr_base* ptr)
{
	std::scoped_lock<std::mutex> al(lock);

	if (std::find(child_ptrs.begin(), child_ptrs.end(), ptr) !=
		child_ptrs.end())
	{
		return false;
	}

	child_ptrs.push_back(ptr);
	return true;
}

gc_ptr_base* gc_ptr_node::get_child(uint32_t index)
{
	std::scoped_lock<std::mutex> al(lock);

	if (index >= child_ptrs.size())
	{
		return nullptr;
	}

	return child_ptrs[index];
}

gc_ptr_node* gc_ptr_node::get_child_node(uint32_t index)
{
	std::scoped_lock<std::mutex> al(lock);

	if (index >= child_ptrs.size())
	{
		return nullptr;
	}

	return child_ptrs[index]->get_ptr_node();
}

bool gc_ptr_node::remove_child(gc_ptr_base* ptr)
{
	std::scoped_lock<std::mutex> al(lock);

	auto it = std::find(child_ptrs.begin(), child_ptrs.end(), ptr);

	if (it != child_ptrs.end())
	{
		child_ptrs.erase(it);
		return true;
	}
	else
	{
		return false;
	}
}

gc_ptr_base* gc_ptr_node::remove_child(uint32_t index)
{
	std::scoped_lock<std::mutex> al(lock);

	if (index >= child_ptrs.size())
	{
		return nullptr;
	}

	gc_ptr_base* erase_node = child_ptrs[index];
	child_ptrs.erase(child_ptrs.begin() + index);
	return erase_node;
}
