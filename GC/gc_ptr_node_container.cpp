#include "gc_ptr_node_container.h"
#include <assert.h>
#include <windows.h>
#include <thread>

lock_free_node_buffer::lock_free_node_buffer(uint64_t initial_capacity) :
	m_node_buffer(nullptr),
	m_node_buffer_size(0),
	m_cur_pos(0),
	m_resizing(0),
	m_shrinking(0),
	m_getting(0)
{
	m_node_buffer = new v_gc_ptr_node* [initial_capacity]();
	::memset(m_node_buffer, 0, initial_capacity * sizeof(gc_ptr_node*));
	assert(m_node_buffer);
	m_node_buffer_size = initial_capacity;
}

lock_free_node_buffer::~lock_free_node_buffer()
{
	delete m_node_buffer;
}

void lock_free_node_buffer::add_node(gc_ptr_node* node)
{
	while (true)
	{
		InterlockedIncrement64(&m_getting);
		if (m_resizing | m_shrinking)
		{
			InterlockedDecrement64(&m_getting);
			std::this_thread::yield();
		}
		else
		{
			break;
		}
	}

	int64_t add_pos = InterlockedIncrement64(&m_cur_pos) - 1;

	while (add_pos >= m_node_buffer_size)
	{
		std::this_thread::yield();
	}

	node->gc_pos = add_pos;

	InterlockedExchange64(reinterpret_cast<volatile int64_t*>(&m_node_buffer[add_pos]), reinterpret_cast<int64_t>(node));

	InterlockedDecrement64(&m_getting);

	if (add_pos >= m_node_buffer_size - 1)
	{
		resize(m_node_buffer_size * 1.5);
	}
}

gc_ptr_node* lock_free_node_buffer::set_node(uint32_t index, gc_ptr_node* node)
{
	while (true)
	{
		InterlockedIncrement64(&m_getting);
		if (m_resizing | m_shrinking)
		{
			InterlockedDecrement64(&m_getting);
			std::this_thread::yield();
		}
		else
		{
			break;
		}
	}

	assert(index < m_node_buffer_size);
	int64_t pre_v = InterlockedExchange64(reinterpret_cast<volatile int64_t*>(&m_node_buffer[index]), reinterpret_cast<int64_t>(node));

	InterlockedDecrement64(&m_getting);

	return reinterpret_cast<gc_ptr_node*>(pre_v);
}

v_gc_ptr_node* lock_free_node_buffer::operator[](uint64_t index)
{
	while (true)
	{
		InterlockedIncrement64(&m_getting);
		if (m_resizing | m_shrinking)
		{
			InterlockedDecrement64(&m_getting);
			std::this_thread::yield();
		}
		else
		{
			break;
		}
	}

	assert(index < m_node_buffer_size);
	v_gc_ptr_node* ret = m_node_buffer[index];

	InterlockedDecrement64(&m_getting);

	return ret;
}

void lock_free_node_buffer::shrink(gc_ptr_node* empty_elem)
{
	uint32_t null_pos(0), data_pos(0);
	bool find_data(false);

	while (true) 
	{
		if (InterlockedExchange64(&m_shrinking, 1)) 
		{
			continue;
		}

		if (m_resizing | m_getting) 
		{
			InterlockedExchange64(&m_shrinking, 0);
		}
		else 
		{
			break;
		}

		std::this_thread::yield();
	}
	
	for (uint32_t i = 0; i < m_cur_pos; i++)
	{
		if (!find_data && m_node_buffer[i] != empty_elem)
		{
			data_pos = i;
			find_data = true;
		}

		if (find_data && m_node_buffer[i] == empty_elem)
		{
			uint32_t data_len = i - data_pos;
			if (null_pos != data_pos)
			{
				::memmove(&m_node_buffer[null_pos], &m_node_buffer[data_pos], data_len * sizeof(gc_ptr_node*));
				for (uint32_t i = 0; i < data_len; i++)
				{
					m_node_buffer[i + null_pos]->gc_pos = i + null_pos;
				}

			}
			null_pos = null_pos + data_len;
			find_data = false;
		}
	}

	if (find_data)
	{
		uint32_t data_len = m_cur_pos - data_pos;
		::memmove(&m_node_buffer[null_pos], &m_node_buffer[data_pos], data_len * sizeof(gc_ptr_node*));
		for (uint32_t i = 0; i < data_len; i++)
		{
			m_node_buffer[i + null_pos]->gc_pos = i + null_pos;
		}
		null_pos = null_pos + data_len;
	}

	m_cur_pos = null_pos;

	::memset(&m_node_buffer[null_pos], 0, (m_node_buffer_size - m_cur_pos) * sizeof(gc_ptr_node*));

	InterlockedExchange64(&m_shrinking, 0);
}

void lock_free_node_buffer::resize(uint64_t length)
{
	assert(length >= m_node_buffer_size);

	while (true)
	{
		if (!InterlockedExchange64(&m_resizing, 1))
		{
			if (m_shrinking | m_getting)
			{
				InterlockedExchange64(&m_resizing, 0);
			}
			else
			{
				break;
			}
		}
		std::this_thread::yield();
	}

	v_gc_ptr_node** new_buffer = new v_gc_ptr_node * [length];
	assert(new_buffer);

	::memcpy(new_buffer, m_node_buffer, m_cur_pos * sizeof(gc_ptr_node*));
	::memset(&new_buffer[m_cur_pos], 0, (length - m_cur_pos) * sizeof(gc_ptr_node*));
	delete m_node_buffer;
	m_node_buffer = new_buffer;
	m_node_buffer_size = length;

	InterlockedExchange64(&m_resizing, 0);
}

size_t lock_free_node_buffer::size()
{
	return m_cur_pos;
}

size_t lock_free_node_buffer::capacity()
{
	return m_node_buffer_size;
}

bool lock_free_node_buffer::is_paused()
{
	return m_resizing | m_shrinking;
}

void lock_free_node_buffer::tidy(uint64_t length, gc_ptr_node* empty_elem)
{
	assert(length >= m_node_buffer_size);

	while (true)
	{
		if (InterlockedExchange64(&m_resizing, 1))
		{
			continue;
		}

		if (m_shrinking | m_getting)
		{
			InterlockedExchange64(&m_resizing, 0);
		}
		else
		{
			break;
		}

		std::this_thread::yield();
	}

	v_gc_ptr_node** new_buffer = new v_gc_ptr_node * [length];
	assert(new_buffer);
	
	uint32_t null_pos(0), data_pos(0);
	bool find_data(false);
	for (uint32_t i = 0; i < m_cur_pos; i++) 
	{
		if (!find_data && m_node_buffer[i] != empty_elem)
		{
			data_pos = i;
			find_data = true;
		}

		if (find_data && m_node_buffer[i] == empty_elem)
		{
			uint32_t data_len = i - data_pos;
			if (null_pos != data_pos)
			{
				::memmove(&new_buffer[null_pos], &m_node_buffer[data_pos], data_len * sizeof(gc_ptr_node*));
			}
			null_pos = null_pos + data_len;
			find_data = false;
		}
	}

	if (find_data)
	{
		uint32_t data_len = m_cur_pos - data_pos;
		::memmove(&new_buffer[null_pos], &m_node_buffer[data_pos], data_len * sizeof(gc_ptr_node*));
		null_pos = null_pos + data_len;
	}

	m_cur_pos = null_pos;
	::memset(&new_buffer[m_cur_pos], 0, (length - m_cur_pos) * sizeof(gc_ptr_node*));
	delete m_node_buffer;
	m_node_buffer = new_buffer;
	m_node_buffer_size = length;

	InterlockedExchange64(&m_resizing, 0);
}

lock_free_deleted_stack::lock_free_deleted_stack(uint64_t initial_capacity) :
	m_buffer(nullptr),
	m_buffer_size(0),
	m_cur_pos(0),
	m_pushing(0),
	m_poping(0),
	m_resizing(0)
{
	m_buffer = new int64_t[initial_capacity]();
	assert(m_buffer);
	m_buffer_size = initial_capacity;
}

lock_free_deleted_stack::~lock_free_deleted_stack()
{
	delete m_buffer;
}

void lock_free_deleted_stack::push(int64_t elem)
{
	while (true)
	{
		InterlockedIncrement64(&m_pushing);
		if (m_resizing | m_poping)
		{
			InterlockedDecrement64(&m_pushing);
			std::this_thread::yield();
		}
		else
		{
			break;
		}
	}

	int64_t add_pos = InterlockedIncrement64(&m_cur_pos) - 1;
	while (m_buffer_size <= add_pos);

	m_buffer[add_pos] = elem;

	InterlockedDecrement64(&m_pushing);

	if (add_pos >= m_buffer_size - 1)
	{
		resize(m_buffer_size * 1.5);
	}
}

bool lock_free_deleted_stack::pop(int64_t& elem)
{
	int64_t ret(0);
	int64_t pop_pos(0);
	int64_t old(0);
	int64_t new_v(0);

	while (true)
	{
		InterlockedIncrement64(&m_poping);
		if (m_resizing | m_pushing)
		{
			InterlockedDecrement64(&m_poping);
			std::this_thread::yield();
		}
		else
		{
			break;
		}
	}

	do {
		old = m_cur_pos;
		pop_pos = old - 1;
		new_v = (pop_pos > 0) ? (pop_pos) : (0);
	} while (InterlockedCompareExchange64(&m_cur_pos, new_v, old) != old);

	if (pop_pos < 0)
	{
		InterlockedDecrement64(&m_poping);
		return false;
	}

	elem = m_buffer[pop_pos];
	InterlockedDecrement64(&m_poping);

	return true;
}

size_t lock_free_deleted_stack::size()
{
	return m_cur_pos;
}

void lock_free_deleted_stack::clear()
{
	InterlockedExchange64(&m_cur_pos, 0);
}

void lock_free_deleted_stack::resize(uint64_t length)
{
	assert(length > m_buffer_size);

	while (true)
	{
		if (InterlockedExchange64(&m_resizing, 1))
		{
			continue;
		}

		if (m_pushing | m_poping)
		{
			InterlockedExchange64(&m_resizing, 0);
			std::this_thread::yield();
		}
		else
		{
			break;
		}
	}

	int64_t* new_buffer = new int64_t[length]();
	assert(new_buffer);

	::memcpy(new_buffer, m_buffer, m_cur_pos * sizeof(int64_t));
	delete m_buffer;
	m_buffer = new_buffer;
	m_buffer_size = length;

	InterlockedExchange64(&m_resizing, 0);
}

ptr_node_container::ptr_node_container(uint64_t node_buffer_size) :
	m_nodes(node_buffer_size),
	m_del_st(node_buffer_size)
{

}

ptr_node_container::~ptr_node_container()
{

}

void ptr_node_container::add_node(gc_ptr_node* node)
{
	int64_t free_pos(0);

	if (m_del_st.pop(free_pos))
	{
		node->gc_pos = free_pos;
		assert(!m_nodes.set_node(free_pos, node));
	}
	else
	{
		m_nodes.add_node(node);
	}
}

bool ptr_node_container::remove_node(int64_t index)
{
	gc_ptr_node* ptr = m_nodes.set_node(index, nullptr);
	if (ptr)
	{
		ptr->gc_pos = 0;
		ptr->gc_num = 0;
		m_del_st.push(index);
		return true;
	}

	return false;
}

v_gc_ptr_node* ptr_node_container::operator[](uint64_t index)
{
	return m_nodes[index];
}

void ptr_node_container::shrink()
{
	auto a = node_count();
	m_del_st.clear();
	m_nodes.shrink(nullptr);
	auto b = node_count();
	assert(a == b);
}

void ptr_node_container::resize(uint64_t length)
{
	m_nodes.resize(length);
}

void ptr_node_container::tidy(uint32_t length)
{
	auto a = node_count();
	m_del_st.clear();
	m_nodes.tidy(length, nullptr);
	auto b = node_count();
	assert(a == b);
}

float ptr_node_container::empty_ratio()
{
	return static_cast<float>(m_del_st.size()) / m_nodes.size();
}

float ptr_node_container::used_ratio()
{
	return static_cast<float>(m_nodes.size()) / m_nodes.capacity();
}

size_t ptr_node_container::node_count()
{
	return m_nodes.size() - m_del_st.size();
}

size_t ptr_node_container::size()
{
	return m_nodes.size();
}

bool ptr_node_container::is_paused()
{
	return m_nodes.is_paused();
}

