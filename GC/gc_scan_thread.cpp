#include "gc_scan_thread.h"
#include "garbage_collection.h"
#include <assert.h>
#include <thread> 

gc_scan_thread::gc_scan_thread(garbage_collection& gc, uint32_t scan_num, uint32_t scan_length, uint64_t node_buffer_size) :
	m_gc(gc),
	m_scan_num(scan_num),
	m_scan_length(scan_length),
	m_scan_index(0),
	m_nodes(node_buffer_size),
	m_quit(false),
	m_shrinking(false),
	m_scanning(false),
	m_ptr_changed(false)
{
	m_scan_thread = std::thread(&gc_scan_thread::scan_func, this);
}

gc_scan_thread::~gc_scan_thread()
{
	m_quit = true;
	m_scan_thread.join();
}

void gc_scan_thread::add_node(gc_ptr_node* node)
{
	while (m_shrinking);

	node->gc_num = m_scan_num;
	m_nodes.add_node(node);
}

bool gc_scan_thread::remove_node(int64_t index)
{
	return m_nodes.remove_node(index);
}

void gc_scan_thread::shrink()
{
	m_shrinking = true;
	m_nodes.shrink();
	m_shrinking = false;
}

bool gc_scan_thread::is_shrink()
{
	return m_shrinking;
}

float gc_scan_thread::empty_ratio()
{
	return m_nodes.empty_ratio();
}

uint32_t gc_scan_thread::idle_count()
{
	return 0;
}

void gc_scan_thread::scan_func()
{
	while (!m_quit)
	{
		
		m_scanning = true;
		m_mark.loop_count++;
		size_t size = m_nodes.size();
		for (uint32_t i = 0; i < m_scan_length; i++, 
			m_scan_index = (m_scan_index + 1ll) % size) 
		{
			v_gc_ptr_node* node = m_nodes[m_scan_index];
			if (!node) continue;
			
			if (node->ref_count == 0) 
			{
				//m_nodes.remove_node(i);
				m_gc.post_eraser_node(node);
			}

			m_mark.step_count++;
			tr_node_type type = m_tref.tref(node, m_scan_num, m_mark);
			if (type == tr_live || type == tr_break) continue;

			assert(type == tr_has_extern_node);
			if (type == tr_has_extern_node)
			{
				//m_nodes.remove_node(i);
				for (auto& node : m_tref.get_connected_component())
				{
					m_gc.post_eraser_node(node);
				}
			}
		}
		m_scanning = false;

		garbage_collection::wait_for_garbage_collection();
	}
}