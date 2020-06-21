#include "gc_tref.h"
#include <algorithm>
#include <assert.h>

gc_tref::gc_tref():
	m_thread_num(0),
	m_index(0),
	m_start_node(nullptr),
	m_find_start_node(false),
	m_trefing(false),
	m_stop(false)
{
}

gc_tref::~gc_tref()
{
}

tr_node_type gc_tref::tref(v_gc_ptr_node* node, uint32_t thread_num, const gc_mark& mark)
{
	bool_guard g(m_trefing);

	m_thread_num = thread_num;
	m_gc_mark = mark;
	if (!count_node(node)) 
	{
		return tr_live;
	}

	initialize_tref(node);
	tarjan(node);

	if (m_stop) 
	{
		return tr_break;
	}

	tarjan_node &tr_node = m_tref_nodes[node];
	assert(node->ref_count >= tr_node.circle_count);

	if (node->ref_count > tr_node.circle_count) 
	{
		return tr_live;
	}
	else 
	{
		if (has_extern_node()) 
		{
			return tr_live;
		}
		else 
		{
			return tr_has_extern_node;
		}
	}
}

bool gc_tref::stop()
{
	if (m_trefing) 
	{
		m_stop = true;
		return true;
	}
	else
	{
		return false;
	}
}

bool gc_tref::is_stop()
{
	return m_stop;
}

std::vector<v_gc_ptr_node*>& gc_tref::get_connected_component()
{
	return m_connected_component;
}

void gc_tref::initialize_tref(v_gc_ptr_node* node)
{
	m_stop = false;
	m_tref_nodes.clear();
	m_dfn.clear();
	m_low.clear();
	m_st.clear();
	m_connected_component.clear();
	m_index = 0;
	m_find_start_node = false;
	m_start_node = node;
}

void gc_tref::tarjan(v_gc_ptr_node* node)
{
	if (m_stop) return;

	visit(node);
	m_dfn.push_back(m_index);
	m_low.push_back(m_index);
	m_st.push_back(node);
	m_index++;

	tarjan_node& tpar_node = m_tref_nodes[node];

	v_gc_ptr_node* ch_node(nullptr);
	while (ch_node = get_child_tref_node(node))
	{
		if (m_stop) return;

		bool v = is_visited(ch_node);
		
		if (!v)
		{
			tarjan(ch_node);
			tarjan_node& tch_node = m_tref_nodes[ch_node];
			uint32_t par_index = m_low[tpar_node.index];
			uint32_t ch_index = m_low[tch_node.index];
			uint32_t low_index = (std::min)(par_index, ch_index);
			m_low[tpar_node.index] = m_low[low_index];
		}
		else if (in_stack(ch_node))
		{
			tarjan_node& tch_node = m_tref_nodes[ch_node];
			uint32_t index_a = m_low[tpar_node.index];
			uint32_t index_b = m_dfn[tch_node.index];
			uint32_t low_index = (std::min)(index_a, index_b);
			m_low[tpar_node.index] = m_low[low_index];
		}
	}

	if (m_dfn[tpar_node.index] ==
		m_low[tpar_node.index])
	{
		v_gc_ptr_node* st_node(nullptr);
		do
		{
			st_node = m_st.back();
			m_st.pop_back();
			if (!m_find_start_node)
			{
				m_find_start_node = (st_node == m_start_node);
			}
			m_connected_component.push_back(st_node);

		} while (node != st_node);

		if (!m_find_start_node)
		{
			m_connected_component.clear();
		}
	}
}

bool gc_tref::count_node(v_gc_ptr_node* node)
{
	uint64_t& node_step_count = const_cast<gc_ptr_node*>(node)->gc_marks[m_thread_num].step_count;
	uint64_t& node_loop_count = const_cast<gc_ptr_node*>(node)->gc_marks[m_thread_num].loop_count;

	if (m_gc_mark.step_count > node_step_count &&
		m_gc_mark.loop_count > node_loop_count)
	{
		node_step_count = m_gc_mark.step_count;
		node_loop_count = m_gc_mark.loop_count;

		return true;
	}
	else
	{
		return false;
	}
}

bool gc_tref::is_cur_step_node(v_gc_ptr_node* node)
{
	uint64_t& node_step_count = const_cast<gc_ptr_node*>(node)->gc_marks[m_thread_num].step_count;
	return m_gc_mark.step_count == node_step_count;
}

v_gc_ptr_node* gc_tref::get_child_tref_node(v_gc_ptr_node* node)
{
	v_gc_ptr_node* ch_node(nullptr);

	uint32_t& ch_index = m_tref_nodes[node].child_index;

	do
	{
		ch_node = const_cast<gc_ptr_node*>(node)->get_child_node(ch_index);
		ch_index++;
		if (ch_node &&
			(count_node(node) || is_cur_step_node(node)))
		{
			return ch_node;
		}

	} while (ch_node);

	return nullptr;
}

void gc_tref::visit(v_gc_ptr_node* node)
{
	if (m_tref_nodes.find(node) ==
		m_tref_nodes.end())
	{
		m_tref_nodes.insert(std::make_pair(node, tarjan_node(m_index, 0, 0)));
	}
	else
	{
		assert(0);
	}
}

bool gc_tref::is_visited(v_gc_ptr_node* node)
{
	if (m_tref_nodes.find(node) ==
		m_tref_nodes.end())
	{
		return false;
	}
	else
	{
		m_tref_nodes[node].circle_count++;
		return true;
	}
}

bool gc_tref::in_stack(v_gc_ptr_node* node)
{
	return std::find(m_st.begin(), m_st.end(), node) != m_st.end();
}

bool gc_tref::has_extern_node()
{
	for (auto& node : m_connected_component)
	{
		tarjan_node& tr_node = m_tref_nodes[node];

		if (node->ref_count - tr_node.circle_count > 1)
		{
			return true;
		}
	}

	return false;
}
