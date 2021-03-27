#pragma once
#include "gc_ptr_node.h"
#include <deque>

class bool_guard 
{
public:
	bool_guard(bool b) :
		m_b(b)
	{
		m_b = true;
	}

	~bool_guard() 
	{
		m_b = false;
	}
private:

	bool &m_b;
};

enum tr_node_type { tr_live, tr_has_extern_node, tr_break };

class gc_tref
{
public:
	gc_tref(uint64_t gc_mark);
	~gc_tref();

	tr_node_type tref(gc_ptr_node* gc_node);
	bool stop();
	bool is_stop();
	std::vector<gc_ptr_node*>& get_connected_component();

private:

	std::vector<uint32_t>							m_dfn;
	std::vector<uint32_t>							m_low;
	std::deque<gc_ptr_node*>						m_st;
	std::vector<gc_ptr_node*>						m_connected_component;

	const uint64_t									m_gc_mark;
	gc_ptr_node										*m_start_node;
	uint32_t										m_index;
	bool											m_find_start_node;
	bool											m_trefing;
	bool											m_stop;
	
	void init_tref();
	void init_gc_node(gc_ptr_node *gc_node);
	void tarjan(gc_ptr_node *gc_node);

	void visit(gc_ptr_node* gc_node);
	bool is_visited(gc_ptr_node* gc_node);
	bool in_stack(gc_ptr_node* gc_node);
	bool has_extern_node();
};

