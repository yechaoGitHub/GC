#pragma once
#include "gc_ptr_node.h"
#include "gc_allocator.hpp"
#include "garbage_collection.h"
#include <stdint.h>
#include <type_traits>
#include <assert.h>

class enable_gc_ptr_form_raw;

template<typename T> class gc_ptr;

template<typename P, typename T>
gc_ptr<P> gc_ptr_dynamic_cast(gc_ptr<T>& origin_ptr);

template<typename P, typename T>
const gc_ptr<P> gc_ptr_dynamic_cast(const gc_ptr<T>& origin_ptr);

template<typename T>
class gc_ptr : public gc_ptr_base
{
    template<typename P>
    friend class gc_ptr;

    template<typename T1>
    friend gc_ptr<T1> get_gc_ptr_from_raw(T* raw);

    template<typename T1>
    friend const gc_ptr<T1> get_gc_ptr_from_raw(const T* raw);

    template<typename P, typename T1>
    friend gc_ptr<P> gc_ptr_static_cast(gc_ptr<T1>& origin_ptr);
    
    template<typename P, typename T1>
    friend const gc_ptr<P> gc_ptr_static_cast(const gc_ptr<T1>& origin_ptr);
   
    template<typename P, typename T1>
    friend gc_ptr<P> gc_ptr_dynamic_cast(gc_ptr<T1>& origin_ptr);
    
    template<typename P, typename T1>
    friend const gc_ptr<P> gc_ptr_dynamic_cast(const gc_ptr<T1>& origin_ptr);
   
public:
    gc_ptr() :
        m_par_node(nullptr), 
        m_ptr_node(nullptr),
        m_cast_data(nullptr)
    {

    }

    gc_ptr(void* null) :
        gc_ptr()
    {
        assert(null);
    }

    gc_ptr(const gc_ptr& rhd) :
        gc_ptr()
    {
        if (!rhd.is_null())
        {
            m_ptr_node = rhd.m_ptr_node;
            m_cast_data = rhd.m_cast_data;
            add_ref();
        }
    }

    gc_ptr(gc_ptr&& rhd) :
        gc_ptr()
    {
        if (!rhd.is_null())
        {
            m_ptr_node = rhd.m_ptr_node;
            m_cast_data = rhd.m_cast_data;
            add_ref();
        }
    }

    ~gc_ptr()
    {
        reduce_ref();
        if (m_par_node) 
        {
            m_par_node->remove_child(this);
        }
        m_ptr_node = nullptr;
        m_par_node = nullptr;
        m_cast_data = nullptr;
    }

    gc_ptr& operator= (const gc_ptr& rhd)
    {
        if (this == &rhd)
        {
            return *this;
        }

        release();
        if (!rhd.is_null())
        {
            m_ptr_node = rhd.m_ptr_node;
            m_cast_data = rhd.m_cast_data;
            add_ref();
        }

        return *this;
    }

    gc_ptr& operator= (gc_ptr&& rhd)
    {
        if (this == &rhd)
        {
            return *this;
        }

        release();
        if (!rhd.is_null())
        {
            m_ptr_node = rhd.m_ptr_node;
            m_cast_data = rhd.m_cast_data;
            rhd.m_ptr_node = nullptr;
            rhd.m_cast_data = nullptr;
        }

        return *this;
    }

    gc_ptr& operator= (void* null)
    {
        assert(null == nullptr);
        release();
        return *this;
    }

    T& operator* ()
    {
        return *m_cast_data;
    }

    const T& operator* () const
    {
        return *m_cast_data;
    }

    T* operator-> ()
    {
        return m_cast_data;
    }

    const T* operator-> () const
    {
        return m_cast_data;
    }

    bool operator== (const gc_ptr& rhd) const
    {
        return m_cast_data == rhd.m_cast_data;
    }

    bool operator!= (const gc_ptr& rhd) const
    {
        return !(m_cast_data == rhd.m_cast_data);
    }

    bool operator== (void* null) const
    {
        assert(!null);
        return is_null();
    }

    bool operator!= (void* null) const
    {
        assert(!null);
        return !is_null();
    }

    bool operator> (const gc_ptr& rhd) const
    {
        return m_cast_data > rhd.m_cast_data;
    }

    bool operator>= (const gc_ptr& rhd) const
    {
        return m_cast_data >= rhd.m_cast_data;
    }

    bool operator< (const gc_ptr& rhd) const
    {
        return m_cast_data < rhd.m_cast_data;
    }

    bool operator<= (const gc_ptr& rhd) const
    {
        return m_cast_data <= rhd.m_cast_data;
    }

    operator bool() const
    {
        return !is_null();
    }

    template<typename P>
    operator gc_ptr<P>()
    {
        static_assert(std::is_base_of_v<P, T>, L"����ִ����ʽת��");
        P* cast_data = static_cast<P*>(m_cast_data);
        assert(m_cast_data);
        return gc_ptr<P>(m_ptr_node, cast_data);
    }

    template<typename P>
    operator const gc_ptr<P>() const
    {
        static_assert(std::is_base_of_v<P, T>, L"����ִ����ʽת��");
        const P* cast_data = static_cast<const P*>(m_cast_data);
        assert(m_cast_data);
        return gc_ptr<P>(m_ptr_node, cast_data);
    }

    bool assign(T* object)
    {
        assert(object);

        gc_data_node<T>* node = new gc_data_node<T>;
        if (!node)
        {
            return false;
        }

        reduce_ref();
        node->data = object;
        if constexpr (std::is_base_of_v<enable_gc_ptr_form_raw, T>)
        {
            object->m_base_node = node;
            object->assigned_to_gc_ptr();
            node->clear_up_gc_ptr_func = std::bind(&T::clear_up_gc_ptr, object);
        }
        else if constexpr (T::need_clear_up_gc_ptr) 
        {
            node->clear_up_gc_ptr_func = std::bind(&T::clear_up_gc_ptr, object);
        }

        m_ptr_node = node;
        m_cast_data = object;

        garbage_collection::add_ptr_node(m_ptr_node);
        m_ptr_node->add_ref();

        return true;
    }

    void release()
    {
        reduce_ref();
        m_ptr_node = nullptr;
        m_cast_data = nullptr;
    }

    T* get_raw_ptr()
    {
        return m_cast_data;
    }

    const T* get_raw_ptr() const
    {
        return m_cast_data;
    }

    template<typename P>
    gc_ptr<P> gc_ptr_static_cast()
    {
        return gc_ptr_static_cast<P>(*this);
    }

    template<typename P>
    const gc_ptr<P> gc_ptr_static_cast() const
    {
        return gc_ptr_static_cast<P>(*this);
    }

    template<typename P>
    gc_ptr<P> gc_ptr_dynamic_cast()
    {
        return ::gc_ptr_dynamic_cast<P>(*this);
    }

    template<typename P>
    const gc_ptr<P> gc_ptr_dynamic_cast() const
    {
        return ::gc_ptr_dynamic_cast<P>(*this);
    }

    bool is_null() const
    {
        return m_cast_data == nullptr;
    }

    uint64_t get_ref_count() const
    {
        if (!is_null())
        {
            return m_ptr_node->ref_count;
        }
        else
        {
            return 0;
        }
    }

    template<typename P>
    void add_member_ptr(const gc_ptr<P>& ptr) const
    {
        assert(!this->is_null());
        assert(ptr.is_null() && ptr.m_par_node == nullptr);

        garbage_collection::notify_ptr_changed(m_ptr_node);
        m_ptr_node->add_child(const_cast<gc_ptr<P>*>(&ptr));
        ptr.m_par_node = m_ptr_node;
    }

    template<typename P>
    void remove_member_ptr(const gc_ptr<P>& ptr) const
    {
        assert(!this->is_null());
        assert(ptr.m_par_node == m_ptr_node);

        garbage_collection::notify_ptr_changed(m_ptr_node);
        m_ptr_node->remove_child(const_cast<gc_ptr<P>*>(&ptr));
        ptr.m_par_node = nullptr;
    }

public:
    mutable gc_ptr_node*    m_par_node;
    mutable gc_ptr_node*    m_ptr_node; 
    T*                      m_cast_data;

    gc_ptr(gc_ptr_node* ptr_node, T* cast_data) :
        m_ptr_node(ptr_node),
        m_cast_data(cast_data)
    {
        m_ptr_node->add_ref();
    }

    gc_ptr(const gc_ptr_node* ptr_node, const T* cast_data) :
        m_ptr_node(ptr_node),
        m_cast_data(cast_data)
    {
        m_ptr_node->add_ref();
    }

    void add_ref() const
    {
        if (!is_null())
        {
            garbage_collection::notify_ptr_changed(m_ptr_node);
            m_ptr_node->add_ref();
        }
    }

    void reduce_ref()
    {
        if (!is_null())
        {
            garbage_collection::notify_ptr_changed(m_ptr_node);
            m_ptr_node->reduce_ref();
        }
    }

    gc_ptr_node*& get_ptr_node() override
    {
        return m_ptr_node;
    }
};

template<typename P, typename T>
gc_ptr<P> gc_ptr_dynamic_cast(gc_ptr<T>& origin_ptr) 
{
    P* data = dynamic_cast<P*>(origin_ptr.get_raw_ptr());
    if (data != nullptr)
    {
        return gc_ptr<P>(origin_ptr.m_ptr_node, data);
    }
    else
    {
        return nullptr;
    }
}

template<typename P, typename T>
const gc_ptr<P> gc_ptr_dynamic_cast(const gc_ptr<T>& origin_ptr) 
{
    P* data = dynamic_cast<P*>(origin_ptr.get_raw_ptr());
    if (data != nullptr)
    {
        return gc_ptr<P>(origin_ptr.m_ptr_node, data);
    }
    else
    {
        return nullptr;
    }
}

template<typename T>
gc_ptr<T> get_gc_ptr_from_raw(T *raw)
{
    static_assert(std::is_base_of_v<enable_gc_ptr_form_raw, T>, L"û�м̳�EnablePtrFormRaw");
    assert(raw->m_base_node);

    return gc_ptr<T>(raw->m_base_node, raw);
}

template<typename T>
const gc_ptr<T> get_gc_ptr_from_raw(const T* raw)
{
    static_assert(std::is_base_of_v<enable_gc_ptr_form_raw, T>, L"û�м̳�EnablePtrFormRaw");
    assert(raw->m_base_node);

    return gc_ptr<T>(raw->m_base_node, raw);
}

template<typename P, typename T>
gc_ptr<P> gc_ptr_static_cast(gc_ptr<T> &origin_ptr)
{
    P* data = static_cast<P*>(origin_ptr.get_raw_ptr());
    if (data != nullptr)
    {
        return gc_ptr<P>(origin_ptr.m_ptr_node, data);
    }
    else
    {
        return nullptr;
    }
}

template<typename P, typename T>
const gc_ptr<P> gc_ptr_static_cast(const gc_ptr<T>& origin_ptr)
{
    P* data = static_cast<P*>(origin_ptr.get_raw_ptr());
    if (data != nullptr)
    {
        return gc_ptr<P>(origin_ptr.m_ptr_node, data);
    }
    else
    {
        return nullptr;
    }
}

