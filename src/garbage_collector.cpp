#include "garbage_collector.hpp"

GarbageCollector::GarbageCollector() {}

GarbageCollector::~GarbageCollector() {
	for (GCObject* obj : heap) {
		delete obj;
	}
}

GCObject* GarbageCollector::allocate(GCObject* obj) {
	heap.push_back(obj);
	return obj;
}

void GarbageCollector::add_root(GCObject* obj) {
	roots.insert(obj);
}

void GarbageCollector::remove_root(GCObject* obj) {
	roots.erase(obj);
}

void GarbageCollector::add_root_container(const IterableOfRuntimeValuePtr auto& iterable) {
	root_containers.emplace_back(std::make_unique<IterableWrapper<std::decay_t<decltype(iterable)>>>(iterable));
}

void GarbageCollector::mark() {
	for (GCObject* root : roots) {
		mark_object(root);
	}

	//const std::deque<RuntimeValue*>& value_stack_container = value_stack._Get_container();

	//for (GCObject* root : value_stack_container) {
	//	mark_object(root);
	//}

	for (const auto& iterable_ptr : root_containers) {
		for (auto item : *iterable_ptr) {
			mark_object(item);
		}
	}
}

void GarbageCollector::mark_object(GCObject* obj) {
	if (obj == nullptr || obj->marked) return;
	obj->marked = true;

	for (GCObject* referenced : obj->get_references()) {
		mark_object(referenced);
	}
}

void GarbageCollector::sweep() {
	for (auto it = heap.begin(); it != heap.end(); ) {
		if (!(*it)->marked) {
			delete* it;
			it = heap.erase(it);
		}
		else {
			(*it)->marked = false;
			++it;
		}
	}
}

void GarbageCollector::collect() {
	mark();
	sweep();
}
