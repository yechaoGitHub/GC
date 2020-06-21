//#include "garbage_collection.h"
//#include "gc_ptr.hpp"
//#include <iostream>
//#include <math.h>
//#include <windows.h>
//#include <thread>
//#include <assert.h>
//#include <set>
//#include <map>
//#include <algorithm>
//
//garbage_collection gc;
//scan_thread sc(gc, 0, 1024);
//
//struct vertex : public virtual enable_ptr_form_raw
//{
//    vertex(uint32_t num_):
//        num(num_),
//        edges(get_gc_allocator<gc_ptr<vertex>>())
//    {
//       
//    }
//
//    void add_edge() 
//    {
//        edges.emplace_back(nullptr);
//    }
//
//    void remove_edge() 
//    {
//        edges.pop_back();
//    }
//
//    uint32_t                    num;
//    std::vector<gc_ptr<vertex>, gc_allocator<gc_ptr<vertex>>> edges;
//
//    gc_ptr<vertex>& get_edge(uint32_t index) 
//    {
//        return edges[index];
//    }
//};
//
//int main() 
//{
//    gc_ptr<vertex> v1;
//    gc_ptr<vertex> v2;
//    gc_ptr<vertex> v3;
//    gc_ptr<vertex> v4;
//    gc_ptr<vertex> v5;
//    gc_ptr<vertex> v6;
//    
//    v1.assign(new vertex(1));
//    v2.assign(new vertex(2));
//    v3.assign(new vertex(3));
//    v4.assign(new vertex(4));
//    v5.assign(new vertex(5));
//    v6.assign(new vertex(6));
//    
//    v1->add_edge();
//    v2->add_edge();
//    v3->add_edge();
//    v4->add_edge();
//    v5->add_edge();
//    
//    v1->add_edge();
//    v3->add_edge();
//    v4->add_edge();
//       
//    v1->get_edge(0) = v3;
//    v1->get_edge(1) = v2;
//    
//    v2->get_edge(0) = v4;
//    
//    v3->get_edge(0) = v5;
//    v3->get_edge(1) = v4;
//    
//    v4->get_edge(0) = v6;
//    v4->get_edge(1) = v1;
//    
//    v5->get_edge(0) = v6;
//
//    sc.add_node(v1.m_ptr_node);
//    sc.add_node(v2.m_ptr_node);
//    sc.add_node(v3.m_ptr_node);
//    sc.add_node(v4.m_ptr_node);
//    sc.add_node(v5.m_ptr_node);
//    sc.add_node(v6.m_ptr_node);
//
//    v1 = nullptr;
//    v2 = nullptr;
//    v3 = nullptr;
//    v4 = nullptr;
//    v5 = nullptr;
//    v6 = nullptr;
//
//
//    sc.scan_func();
//
//
//
//    return 0;
//
//}
