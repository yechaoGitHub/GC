#pragma once
#include "gc_ptr_node.h"

class lock_free_node_buffer
{
public:
	lock_free_node_buffer(uint64_t initial_capacity);
	~lock_free_node_buffer();

	void add_node(gc_ptr_node* node);
	gc_ptr_node* set_node(uint32_t index, gc_ptr_node* node);
	v_gc_ptr_node* operator[](uint64_t index);
	void shrink(gc_ptr_node* empty_elem);
	size_t size();

private:
	v_gc_ptr_node**		m_node_buffer;
	uint64_t			m_node_buffer_size;
	volatile int64_t	m_cur_pos;
	volatile int64_t	m_resizing;
	volatile int64_t	m_shrinking;

	void resize(uint64_t length);
};

class lock_free_deleted_stack
{
public:
	lock_free_deleted_stack(uint64_t initial_capacity);
	~lock_free_deleted_stack();

	void push(int64_t elem);
	bool pop(int64_t& elem);
	size_t size();
	void clear();

private:
	int64_t* m_buffer;
	uint64_t					m_buffer_size;
	volatile int64_t			m_cur_pos;
	volatile int64_t			m_pushing;
	volatile int64_t			m_poping;
	volatile int64_t			m_resizing;
	volatile int64_t			m_clearing;

	void resize(uint64_t length);
};

class ptr_node_container
{
public:
	ptr_node_container(uint64_t node_buffer_size);
	~ptr_node_container();

	void add_node(gc_ptr_node* node);
	bool remove_node(int64_t index);
	v_gc_ptr_node* operator[](uint64_t index);
	void shrink();
	float empty_ratio();
	size_t node_count();
	size_t size();

private:
	lock_free_node_buffer	m_nodes;
	lock_free_deleted_stack m_del_st;
};


