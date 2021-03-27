#include "gc_tref.h"
#include <algorithm>
#include <assert.h>

gc_tref::gc_tref(uint64_t gc_mark):
	m_gc_mark(gc_mark),
	m_start_node(nullptr),
	m_index(0),
	m_find_start_node(false),
	m_trefing(false),
	m_stop(false)
{
}

gc_tref::~gc_tref()
{
}

tr_node_type gc_tref::tref(gc_ptr_node* gc_node)
{
	bool_guard g(m_trefing);

	init_tref();

	tarjan(gc_node);

	assert(gc_node->ref_count >= gc_node->circle_count);

	if (gc_node->ref_count > gc_node->circle_count)
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

std::vector<gc_ptr_node*>& gc_tref::get_connected_component()
{
	return m_connected_component;
}

void gc_tref::init_tref()
{
	m_start_node = nullptr;
	m_index = 0;
	m_find_start_node = false;
	m_trefing = false;
	m_stop = false;

	m_dfn.clear();
	m_low.clear();
	m_st.clear();
	m_connected_component.clear();
}

void gc_tref::init_gc_node(gc_ptr_node *gc_node)
{
	gc_node->circle_count = 0;
}

void gc_tref::tarjan(gc_ptr_node* gc_node)
{
	if (m_stop) return;

	uint32_t index = m_index++;
	visit(gc_node);
	gc_node->index = index;
	m_dfn.push_back(index);
	m_low.push_back(index);
	m_st.push_back(gc_node);

	size_t child_size = gc_node->child_ptrs.size();
	for (uint32_t i = 0; i < child_size; i++)
	{
		gc_ptr_node *ch_node = gc_node->get_child_node(i);

		bool v = is_visited(ch_node);
		if (!v) 
		{
			tarjan(ch_node);
			uint32_t par_index = m_low[gc_node->index];
			uint32_t ch_index = m_low[ch_node->index];
			uint32_t low_index = (std::min)(par_index, ch_index);
			m_low[gc_node->index] = m_low[low_index];
		}
		else 
		{
			ch_node->circle_count++;
			if (in_stack(ch_node)) 
			{
				uint32_t index_a = m_low[gc_node->index];
				uint32_t index_b = m_dfn[ch_node->index];
				uint32_t low_index = (std::min)(index_a, index_b);
				m_low[gc_node->index] = m_low[low_index];
			}
		}
	}

	if (m_dfn[gc_node->index] ==
		m_low[gc_node->index])
	{
		gc_ptr_node* st_node(nullptr);
		do
		{
			st_node = m_st.back();
			m_st.pop_back();
			if (!m_find_start_node)
			{
				m_find_start_node = (st_node == m_start_node);
			}
			m_connected_component.push_back(st_node);

		} while (gc_node != st_node);

		if (!m_find_start_node)
		{
			m_connected_component.clear();
		}
	}
}

void gc_tref::visit(gc_ptr_node* gc_node)
{
	gc_node->gc_mark |= m_gc_mark;
}

bool gc_tref::is_visited(gc_ptr_node* gc_node)
{
	return gc_node->gc_mark & m_gc_mark;
}

bool gc_tref::in_stack(gc_ptr_node* node)
{
	return std::find(m_st.begin(), m_st.end(), node) != m_st.end();
}

bool gc_tref::has_extern_node()
{
	for (auto& gc_node : m_connected_component)
	{
		if (gc_node->ref_count - gc_node->circle_count > 1)
		{
			return true;
		}
	}

	return false;
}
