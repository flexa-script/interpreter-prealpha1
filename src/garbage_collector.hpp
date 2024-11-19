#ifndef GARBAGE_COLLECTOR_HPP
#define GARBAGE_COLLECTOR_HPP

#include <vector>
#include <unordered_set>
#include <memory>
#include <ranges>
#include <type_traits>
#include <variant>

#include "gcobject.hpp"
#include "types.hpp"

class GarbageCollector {
private:
    std::vector<GCObject*> heap;
    std::vector<std::variant<std::weak_ptr<GCObject>, RuntimeValue**>> roots;
    std::vector<std::vector<RuntimeValue*>> root_containers;

public:
    GarbageCollector();
    ~GarbageCollector();

    GCObject* allocate(GCObject* obj);

    void add_root(std::variant<std::weak_ptr<GCObject>, RuntimeValue**> obj);
    void remove_root(std::variant<std::weak_ptr<GCObject>, RuntimeValue**> obj);

    void add_root_container(std::vector<RuntimeValue*>& root_container);

    void mark();
    void mark_object(GCObject* obj);
    void sweep();
    void collect();

};

#endif // !GARBAGE_COLLECTOR_HPP
