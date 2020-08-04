#pragma once
#include "gc_ptr.hpp"

template<typename T>
class gc_ptr;

template<typename T>
class gc_allocator
{
    template<typename U>
    friend class gc_allocator;

public:
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t diference_type;

    template<typename U>
    struct rebind
    {
        typedef gc_allocator<U> other;
    };

    gc_allocator(gc_ptr_node*& node) :
        m_node(node)
    {
        static_assert(std::is_base_of_v<gc_ptr_base, T>, L"±ÿ–Î «gc_ptr");
    }

    gc_allocator(const gc_allocator& rhd) :
        m_node(rhd.m_node)
    {
    }

    template <typename U>
    gc_allocator(const gc_allocator<U>& rhd) :
        m_node(rhd.m_node)
    {
    }

    T* allocate(const size_t count, const void* hint)
    {
        return m_std_allocator.allocate(count, hint);
    }

    T* allocate(const size_t count)
    {
        return m_std_allocator.allocate(count);
    }

    void deallocate(T* ptr, const size_t size)
    {
        if (m_node)
        {
            for (uint32_t i = 0; i < size; i++)
            {
#if _DEBUG
                m_node->remove_child((gc_ptr<T>*)&ptr[i]);
          
#else
                m_node->remove_child(&ptr[i]);
#endif
            }
        }

        m_std_allocator.deallocate(ptr, size);
    }

    template<typename... TArgs>
    void construct(T* ptr, TArgs&&... args)
    {
        m_std_allocator.construct(ptr, std::forward<TArgs>(args)...);
        if (m_node)
        {
            m_node->add_child(ptr);
        }
    }

    void destory(T* ptr)
    {
        m_std_allocator.destroy(ptr);
        if (m_node)
        {
            m_node->remove_child(ptr);
        }
    }

    T* address(T& ptr)
    {
        return m_std_allocator.address(ptr);
    }

    const T* address(const T& ptr)
    {
        return m_std_allocator.address(ptr);
    }

    size_t max_size() const
    {
        return m_std_allocator.max_size();
    }

private:

    gc_ptr_node*& m_node;
    std::allocator<T>	m_std_allocator;
};

class enable_gc_ptr_form_raw
{
    template<typename T>
    friend class gc_ptr;

    template<typename T>
    friend gc_ptr<T> get_gc_ptr_from_raw(T* raw);

    template<typename T>
    friend const gc_ptr<T> get_gc_ptr_from_raw(const T* raw);

    template<typename T>
    friend bool is_gc_managed(const T* raw);

public:
    enable_gc_ptr_form_raw() :
        m_base_node(nullptr)
    {

    }

    template<typename T>
    gc_allocator<T> get_gc_allocator()
    {
        return gc_allocator<T>(m_base_node);
    }

protected:
    virtual void assigned_to_gc_ptr() 
    {

    }

    virtual void clear_up_gc_ptr() 
    {

    }

private:
    gc_ptr_node* m_base_node;
};

template <class T, class U>
bool operator==(const gc_allocator<T>&, const gc_allocator<U>&) noexcept
{
    return true;
}

template <class T, class U>
bool operator!=(const gc_allocator<T>&, const gc_allocator<U>&) noexcept
{
    return false;
}
