#pragma once
#include "gc_ptr_node.h"
#include <stdint.h>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <thread>

class gc_scan_thread;

class garbage_collection
{
	friend class gc_scan_thread;

public:
	static void start_up(uint32_t scan_thread_count);
	static void shut_down();
	
private:
	garbage_collection();
	~garbage_collection();

	static garbage_collection						m_gc;
	uint32_t										m_thread_count;
	volatile int64_t								m_wait_thread_count;
	std::vector<std::shared_ptr<gc_scan_thread>>	m_threads;
	std::thread										m_manager_thread;
	std::unordered_set<gc_ptr_node*>				m_garbage_node;
	std::mutex										m_lock;
	bool											m_quit;

	static bool add_ptr_node(gc_ptr_node *node);
	static void notify_ptr_changed(gc_ptr_node* node);
	static void post_eraser_node(v_gc_ptr_node *node);
	static void wait_for_garbage_collection();

	void gc_manager_func();
};
