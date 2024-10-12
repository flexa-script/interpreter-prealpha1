#ifndef GARBAGE_COLLECTOR_HPP
#define GARBAGE_COLLECTOR_HPP

#include <vector>
#include <unordered_set>

#include "gcobject.hpp"

class GarbageCollector {
private:
    std::vector<GCObject*> heap;
    std::unordered_set<GCObject*> roots;

public:
    ~GarbageCollector();

    GCObject* allocate(GCObject* obj);

    void add_root(GCObject* obj);
    void remove_root(GCObject* obj);

    void mark();
    void mark_object(GCObject* obj);
    void sweep();
    void collect();

};

#endif // !GARBAGE_COLLECTOR_HPP
