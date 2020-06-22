#pragma once
#include <memory>
#include <vector>
#include <mutex>
#include <functional>

class gc_ptr_node;
typedef volatile gc_ptr_node v_gc_ptr_node;

struct gc_mark
{
	gc_mark() :
		loop_count(0),
		step_count(0)
	{
	}

	uint64_t loop_count;
	uint64_t step_count;
};

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

	volatile int64_t		        ref_count;
	int32_t							gc_num;
	int32_t							gc_pos;
	std::vector<gc_ptr_base*>       child_ptrs;
	std::vector<gc_mark>	        gc_marks;
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
