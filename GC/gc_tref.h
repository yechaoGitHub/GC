#pragma once
#include "gc_ptr_node.h"
#include <unordered_map>
#include <list>

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

struct tarjan_node 
{
	tarjan_node(uint32_t index_ = 0, uint32_t child_index_ = 0, uint32_t circle_count_ = 0) :
		index(index_),
		child_index(circle_count_),
		circle_count(circle_count_)
	{

	}

	uint32_t	index;
	uint32_t	child_index;
	uint32_t	circle_count;
};

class gc_tref
{
public:
	gc_tref();
	~gc_tref();

	tr_node_type tref(v_gc_ptr_node* node, uint32_t thread_num, const gc_mark &mark);
	bool stop();
	bool is_stop();
	std::vector<v_gc_ptr_node*>& get_connected_component();

private:
	std::unordered_map<v_gc_ptr_node*, tarjan_node>	m_tref_nodes;
	std::vector<uint32_t>							m_dfn;
	std::vector<uint32_t>							m_low;
	std::list<v_gc_ptr_node*>							m_st;
	std::vector<v_gc_ptr_node*>						m_connected_component;
	gc_mark											m_gc_mark;
	uint32_t										m_thread_num;
	uint32_t										m_index;
	v_gc_ptr_node*										m_start_node;
	bool											m_find_start_node;
	bool											m_trefing;
	bool											m_stop;
	
	void initialize_tref(v_gc_ptr_node* node);
	void tarjan(v_gc_ptr_node* node);
	bool count_node(v_gc_ptr_node* node);
	bool is_cur_step_node(v_gc_ptr_node* node);
	v_gc_ptr_node* get_child_tref_node(v_gc_ptr_node* node);
	void visit(v_gc_ptr_node* node);
	bool is_visited(v_gc_ptr_node* node);
	bool in_stack(v_gc_ptr_node* node);
	bool has_extern_node();
};

