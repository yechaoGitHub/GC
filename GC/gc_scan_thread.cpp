#include "gc_scan_thread.h"
#include "garbage_collection.h"
#include <assert.h>
#include <thread> 
#include <future>

gc_scan_thread::gc_scan_thread(garbage_collection& gc, uint32_t scan_num, uint32_t scan_length, uint64_t node_buffer_size) :
	m_gc(gc),
	m_scan_num(scan_num),
	m_scan_length(scan_length),
	m_scan_index(0),
	m_nodes(node_buffer_size),
	m_quit(false),
	m_ptr_changed(false)
{
	m_scan_thread = std::thread(&gc_scan_thread::scan_func, this);
}

gc_scan_thread::~gc_scan_thread()
{
	wait_for_clear();
	m_quit = true;
	m_scan_thread.join();
}

void gc_scan_thread::add_node(gc_ptr_node* node)
{
	node->gc_num = m_scan_num;
	m_nodes.add_node(node);
}

bool gc_scan_thread::remove_node(int64_t index)
{
	return m_nodes.remove_node(index);
}

void gc_scan_thread::shrink()
{
	m_scan_index = 0;
	std::async(std::launch::async, &ptr_node_container::shrink, &m_nodes);
}

void gc_scan_thread::tidy()
{
	m_scan_index = 0;
	std::async(std::launch::async, &ptr_node_container::tidy, &m_nodes, m_nodes.size() * 1.5);
}

bool gc_scan_thread::is_paused()
{
	return m_nodes.is_paused();
}

void gc_scan_thread::notify_ptr_change(gc_ptr_node* ptr_node)
{
	gc_mark &node_mark = ptr_node->gc_marks[m_scan_num];

	if (m_mark.step_count <= node_mark.step_count &&
		m_mark.loop_count <= node_mark.loop_count)
	{
		m_tref.stop();
	}
}

float gc_scan_thread::empty_ratio()
{
	return m_nodes.empty_ratio();
}

float gc_scan_thread::used_ratio()
{
	return m_nodes.used_ratio();
}

void gc_scan_thread::scan_func()
{
	size_t size(0);
	while (!m_quit)
	{
		while (m_nodes.is_paused() || 
			!(size = m_nodes.size())) 
		{
			std::this_thread::yield();
		}

		m_mark.loop_count++;
		 
		for (uint32_t i = 0; i < m_scan_length; i++, 
			m_scan_index = (m_scan_index + 1ll) % size) 
		{
			v_gc_ptr_node* node = m_nodes[m_scan_index];
			if (!node) continue;
			
			if (node->ref_count == 0) 
			{
				//m_nodes.remove_node(i);
				m_gc.post_garbage_node(node);
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
					m_gc.post_garbage_node(node);
				}
			}
		}

		garbage_collection::wait_for_garbage_collection();
	}
}

void gc_scan_thread::wait_for_clear()
{
	while (m_nodes.node_count()) 
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}
