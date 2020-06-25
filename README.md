Title         : C++ PauseLess Garbage Collector

# gc_ptr 

gc_ptr is a full parallel garbage collector. It run with multi scan thread and one garbage collect thread. Unlike Mark And Sweep GC, It has no Stop The World side effect. Working thread don't need any sync with GC Thread. It base on a new GC algorithm. GC thread collect garbage object by running circle reference check continuously and don't interrupte work thread.   

# GC Algorithm

![avatar](https://raw.githubusercontent.com/yechaoGitHub/GC/master/v2-4d06dd7a262f431e2ea215ff30b9aea1_720w.jpg)

Each vertex has two counter. reference countor mark as r, circle counter mark as c. Traverse this graph, if it meet the same vertex, the circle counter increased. Above graph is result of Traversing start with A. Because A's reference count greater than circle counter, so A is alived. Next, we assigned pa to null. So A's reference count equal to circle as 2. In this case, we can't simply judge A is lose reference, because there is a extern reference on E, pointed by pb. In above graph, A, B, E, D form a circle reference. By using Graph Theory, A,B,E,D are consider as a strongly connected component. In this connected component, if one vertex has extern reference, all vertex shall be alived. We can get connected component by using Tarjan algorithm.But how to find extern reference vertex? It is simple. Just traverse each vertex in connected component, if there is one vertex which reference count minus circle count greater than 1, this vertex must have extern reference. Such as E, r - c = 2. In other side, if pa, pb all assigned to null. A,B,E,D have no extern reference. It can be considered as they are all garbage vertex, need to be release. 

# Tarjan algorithm

https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm
