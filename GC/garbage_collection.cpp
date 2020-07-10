#include "garbage_collection.h"
#include "gc_scan_thread.h"
#include <algorithm>
#include <functional>
#include <windows.h>
#include <assert.h>

garbage_collection garbage_collection::m_gc;

garbage_collection::garbage_collection()
{
}

garbage_collection::~garbage_collection()
{
	if (m_gc.m_running) 
	{
		shut_down();
	}
}

void garbage_collection::start_up(uint32_t scan_thread_count)
{
	assert(!m_gc.m_running);

	m_gc.m_wait_thread_count = m_gc.m_thread_count = scan_thread_count;

	for (uint32_t i = 0; i < scan_thread_count; i++)
	{
		std::shared_ptr<gc_scan_thread> th = std::make_shared<gc_scan_thread>(m_gc, i, 50, 1024);
		m_gc.m_threads.emplace_back(th);
	}

	m_gc.m_running = true;
	m_gc.m_manager_thread = std::thread(&garbage_collection::gc_manager_func, &m_gc);
}

void garbage_collection::shut_down()
{
	m_gc.m_threads.clear();
	m_gc.m_running = false;
	m_gc.m_manager_thread.join();
}

bool garbage_collection::add_ptr_node(gc_ptr_node* node)
{
	static uint32_t add_thread_num(0);

	while (true) 
	{
		for (uint32_t i = 0; i < m_gc.m_thread_count; i++)
		{
			add_thread_num = (add_thread_num + 1) % m_gc.m_thread_count;

			if (!m_gc.m_threads[add_thread_num]->is_paused()) 
			{
				m_gc.m_threads[add_thread_num]->add_node(node);
				return true;
			}
		}

		std::this_thread::yield();
	}
}

void garbage_collection::notify_ptr_changed(gc_ptr_node* node)
{
	m_gc.m_threads[node->gc_num]->notify_ptr_change(node);
}

void garbage_collection::post_garbage_node(v_gc_ptr_node* node)
{
	std::scoped_lock<std::mutex> al(m_gc.m_lock);
	m_gc.m_garbage_node.insert(const_cast<gc_ptr_node*>(node));
}

void garbage_collection::wait_for_garbage_collection()
{
	InterlockedDecrement64(&m_gc.m_wait_thread_count);

	while (m_gc.m_wait_thread_count != m_gc.m_thread_count)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

uint32_t garbage_collection::get_scan_thread_count()
{
	return m_gc.m_thread_count;
}

void garbage_collection::gc_manager_func()
{
	uint32_t shrink_thread_num(0);

	while (m_running)
	{
		if (!m_wait_thread_count) 
		{ 
			for (auto &node : m_garbage_node) 
			{
				assert(m_threads[node->gc_num]->remove_node(node->gc_pos));
				node->clear_up_gc_ptr();
			}

			for (auto &node : m_garbage_node) 
			{
				delete node;
			}

			m_garbage_node.clear();

			uint32_t shrink_thread_count(0);
			for (uint32_t i = 0; i < m_thread_count; i++)
			{
				if ((static_cast<float>(shrink_thread_count) / m_thread_count) > 0.45) 
				{
					break;
				}

				uint32_t th_index = shrink_thread_num % m_thread_count;
				std::shared_ptr<gc_scan_thread> &th = m_threads[th_index];

				if (th->is_paused()) 
				{
					shrink_thread_num++;
				}
				else if(th->used_ratio() > 0.8)
				{
					th->tidy();
					shrink_thread_count++;
				}
				else if (th->empty_ratio() > 0.5) 
				{
					th->shrink();
					shrink_thread_count++;
				}
			}

			InterlockedExchange64(&m_wait_thread_count, m_thread_count);
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}
