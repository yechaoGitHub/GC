//#include "garbage_collection.h"
//#include "gc_ptr.h"
//#include <iostream>
//#include <math.h>
//#include <windows.h>
//#include <thread>
//#include <assert.h>
//#include <set>
//#include <map>
//#include <algorithm>
//
//struct vertex : public virtual enable_ptr_form_raw
//{
//    vertex(uint32_t num_) :
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
//scan_thread scaner(0, 1024);
//
//
//class TRef
//{
//    struct tarjan_node
//    {
//        tarjan_node()
//        {}
//
//        tarjan_node(uint32_t index_, uint32_t child_index_, int64_t	circle_count_) :
//            index(index_),
//            child_index(circle_count_),
//            circle_count(circle_count_)
//        {
//
//        }
//
//        uint32_t	index;
//        uint32_t	child_index;
//        int64_t		circle_count;
//    };
//
//    std::map<gc_ptr<vertex>, tarjan_node>	    nodes;
//    std::vector<uint32_t>						        dfn;
//    std::vector<uint32_t>						        low;
//    std::list<gc_ptr<vertex>>						    st;
//    uint32_t									        index;
//
//    void visit(gc_ptr<vertex>& node)
//    {
//        if (nodes.find(node) ==
//            nodes.end())
//        {
//            nodes.insert(std::make_pair(node, tarjan_node(index, 0, 0)));
//        }
//        else
//        {
//            assert(0);
//        }
//    }
//
//    bool is_visited(gc_ptr<vertex>& node)
//    {
//        if (nodes.find(node) ==
//            nodes.end())
//        {
//            return false;
//        }
//        else
//        {
//            nodes[node].circle_count++;
//            return true;
//        }
//    }
//
//    bool in_stack(gc_ptr<vertex> node)
//    {
//        return std::find(st.begin(), st.end(), node) != st.end();
//    }
//
//    gc_ptr<vertex> get_child_tref_node(gc_ptr<vertex>& node)
//    {
//        gc_ptr<vertex>* ch_node(nullptr);
//        uint32_t& ch_index = nodes[node].child_index;
//
//        if (ch_index < node->edges.size())
//        {
//            gc_ptr<vertex> ret = node->edges[ch_index];
//            ch_index++;
//            return ret;
//        }
//        else
//        {
//            return nullptr;
//        }
//    }
//
//public:
//    TRef() :
//        index(0)
//    {
//    }
//
//    ~TRef()
//    {
//
//    }
//
//    void tarjan(gc_ptr<vertex>& node)
//    {
//        visit(node);
//
//        dfn.push_back(index);
//        low.push_back(index);
//        st.push_back(node);
//        index++;
//
//        tarjan_node& tpar_node = nodes[node];
//
//        gc_ptr<vertex> ch_node(nullptr);
//        while (ch_node = get_child_tref_node(node))
//        {
//            bool v = is_visited(ch_node);
//            if (!v)
//            {
//                tarjan(ch_node);
//                tarjan_node& tch_node = nodes[ch_node];
//                uint32_t par_index = low[tpar_node.index];
//                uint32_t ch_index = low[tch_node.index];
//                uint32_t low_index = (std::min)(par_index, ch_index);
//                low[tpar_node.index] = low[low_index];
//            }
//            else if (in_stack(ch_node))
//            {
//                tarjan_node& tch_node = nodes[ch_node];
//                uint32_t index_a = low[tpar_node.index];
//                uint32_t index_b = dfn[tch_node.index];
//                uint32_t low_index = (std::min)(index_a, index_b);
//                low[tpar_node.index] = low[low_index];
//            }
//        }
//
//        if (dfn[tpar_node.index] ==
//            low[tpar_node.index])
//        {
//            gc_ptr<vertex> st_node;
//            do
//            {
//                st_node = st.back();
//                st.pop_back();
//
//                std::cout << (st_node)->num << ' ';
//
//            } while (node != st_node);
//
//            std::cout << std::endl;
//        }
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
//    TRef trf;
//
//    trf.tarjan(v1);
//
//    //scaner.tarjan(v1.m_ptr_node);
//
//
//
//    return 0;
//}