#pragma once
#include "gc_ptr_node_container.h"
#include "gc_tref.h"

class garbage_collection;

class gc_scan_thread
{
public:
	gc_scan_thread(garbage_collection& gc, uint32_t scan_num, uint32_t scan_length, uint64_t node_buffer_size);
	~gc_scan_thread();

	void add_node(gc_ptr_node* node);
	bool remove_node(int64_t index);
	float empty_ratio();
	uint32_t idle_count();
	void shrink();
	bool is_shrink();

private:
	garbage_collection&		m_gc;
	const uint32_t			m_scan_num;
	uint32_t				m_scan_length;
	uint64_t				m_scan_index;
	ptr_node_container		m_nodes;
	gc_tref					m_tref;
	gc_mark					m_mark;
	bool					m_quit;
	bool					m_shrinking;
	bool					m_scanning;
	bool					m_ptr_changed;
	std::thread				m_scan_thread;

	void scan_func();
};