#ifndef GARBAGE_COLLECTOR_HPP
#define GARBAGE_COLLECTOR_HPP

#include <vector>
#include <unordered_set>
#include <memory>
#include <ranges>
#include <type_traits>

#include "gcobject.hpp"
#include "types.hpp"

template <typename T>
concept IterableOfRuntimeValuePtr =
std::ranges::range<T> &&
std::is_pointer_v<std::ranges::range_value_t<T>> &&
std::is_same_v<std::ranges::range_value_t<T>, RuntimeValue*>;

struct IterableWrapperBase {
    virtual ~IterableWrapperBase() = default;
    virtual auto begin() const->std::ranges::iterator_t<const std::vector<RuntimeValue*>> = 0;
    virtual auto end() const->std::ranges::iterator_t<const std::vector<RuntimeValue*>> = 0;
};

template <typename T>
struct IterableWrapper : IterableWrapperBase {
    T iterable;

    explicit IterableWrapper(const T& iterable) : iterable(iterable) {}

    std::vector<RuntimeValue*>::const_iterator begin() const override { return std::ranges::begin(iterable); }
    std::vector<RuntimeValue*>::const_iterator end() const override { return std::ranges::end(iterable); }
};

class GarbageCollector {
private:
    std::vector<GCObject*> heap;
    std::unordered_set<GCObject*> roots;
    std::vector<std::unique_ptr<IterableWrapperBase>> root_containers;

public:
    GarbageCollector();
    ~GarbageCollector();

    GCObject* allocate(GCObject* obj);

    void add_root(GCObject* obj);
    void remove_root(GCObject* obj);

    void add_root_container(const IterableOfRuntimeValuePtr auto& iterable) {
        root_containers.emplace_back(std::make_unique<IterableWrapper<std::decay_t<decltype(iterable)>>>(iterable));
    }

    void mark();
    void mark_object(GCObject* obj);
    void sweep();
    void collect();

};

#endif // !GARBAGE_COLLECTOR_HPP
