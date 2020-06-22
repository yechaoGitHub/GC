#include "garbage_collection.h"
#include "gc_ptr.hpp"
#include "gc_allocator.hpp"
#include <iostream>

struct vertex : public virtual enable_gc_ptr_form_raw
{
    vertex(uint32_t num_):
        num(num_),
        edges(get_gc_allocator<gc_ptr<vertex>>())
    {
       
    }

    ~vertex() 
    {
        std::cout << "release vertex: " << num << std::endl;
    }

    uint32_t                                                  num;
    std::vector<gc_ptr<vertex>, gc_allocator<gc_ptr<vertex>>> edges;


    void clear_up_gc_ptr() 
    {
        edges.clear();
    }
};

struct A 
{
    enum { need_clear_up_gc_ptr = 1};

    gc_ptr<A> a;

    void clear_up_gc_ptr()
    {
        a = nullptr;
    }
};

int main() 
{
    
    garbage_collection::start_up(1);

    gc_ptr<A> pa;
    pa.assign(new A);

    gc_ptr<vertex> v1;
    gc_ptr<vertex> v2;
    gc_ptr<vertex> v3;
    gc_ptr<vertex> v4;
    gc_ptr<vertex> v5;
    gc_ptr<vertex> v6;
    
    v1.assign(new vertex(1));
    v2.assign(new vertex(2));
    v3.assign(new vertex(3));
    v4.assign(new vertex(4));
    v5.assign(new vertex(5));
    v6.assign(new vertex(6));
    
    v1->edges.push_back(v3);
    v1->edges.push_back(v2);

    v2->edges.push_back(v4);

    v3->edges.push_back(v5);
    v3->edges.push_back(v4);

    v4->edges.push_back(v6);
    v4->edges.push_back(v1);

    v5->edges.push_back(v6);

    v1 = nullptr;
    v2 = nullptr;
    v3 = nullptr;
    v4 = nullptr;
    v5 = nullptr;
    v6 = nullptr;

    garbage_collection::shut_down();

    return 0;

}
