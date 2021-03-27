#pragma once
#include <memory>
#include <vector>
#include <mutex>
#include <functional>

class gc_ptr_base
{
public:
	virtual gc_ptr_node*& get_ptr_node() = 0;
};

class gc_ptr_node
{
public:
	gc_ptr_node();
	gc_ptr_node(uint32_t mark_count);
	virtual ~gc_ptr_node();

	std::atomic<uint64_t>		    ref_count;
	uint64_t						gc_mark;
	uint32_t						index;
	uint32_t						circle_count;
	std::vector<gc_ptr_base*>       child_ptrs;
	std::mutex                      lock;

	void add_ref();
	void reduce_ref();

	bool add_child(gc_ptr_base* ptr);
	gc_ptr_base* get_child(uint32_t index);
	gc_ptr_node* get_child_node(uint32_t index);
	bool remove_child(gc_ptr_base* ptr);
	gc_ptr_base* remove_child(uint32_t index);

	virtual void clear_up_gc_ptr();


};

template <typename T>
class gc_data_node : public gc_ptr_node
{
public:
	gc_data_node() :
		data(nullptr)
	{

	}

	gc_data_node(uint32_t mark_count):
		gc_ptr_node(mark_count)
	{

	}

	virtual ~gc_data_node()
	{
		delete data;
	}

	void clear_up_gc_ptr() 
	{
		clear_up_gc_ptr_func();
	}

	T* data;
	
	std::function<void(void)> clear_up_gc_ptr_func;
};
