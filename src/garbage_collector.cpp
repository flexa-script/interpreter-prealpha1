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

void GarbageCollector::add_root(std::variant<std::weak_ptr<GCObject>, RuntimeValue**> obj) {
	roots.push_back(obj);
}

void GarbageCollector::remove_root(std::variant<std::weak_ptr<GCObject>, RuntimeValue**> obj) {
	std::visit([this](auto&& arg) {
		using T = std::decay_t<decltype(arg)>;

		if constexpr (std::is_same_v<T, std::weak_ptr<GCObject>>) {
			if (auto obj_ptr = arg.lock()) {
				auto it = std::find_if(roots.begin(), roots.end(), [&obj_ptr](const auto& root_variant) {
					return std::visit([&obj_ptr](auto&& root) -> bool {
						using RootType = std::decay_t<decltype(root)>;
						if constexpr (std::is_same_v<RootType, std::weak_ptr<GCObject>>) {
							return !root.owner_before(obj_ptr) && !obj_ptr.owner_before(root);
						}
						return false;
						}, root_variant);
					});

				if (it != roots.end()) {
					roots.erase(it);
				}
			}
		}
		else if constexpr (std::is_same_v<T, RuntimeValue**>) {
			auto it = std::find_if(roots.begin(), roots.end(), [arg](const auto& root_variant) {
				return std::visit([arg](auto&& root) -> bool {
					using RootType = std::decay_t<decltype(root)>;
					if constexpr (std::is_same_v<RootType, std::weak_ptr<GCObject>>) {
						if (auto root_ptr = root.lock()) {
							return root_ptr.get() == *arg;
						}
					}
					return false;
					}, root_variant);
				});

			if (it != roots.end()) {
				roots.erase(it);
			}
		}
		}, obj);
}

void GarbageCollector::add_root_container(std::vector<RuntimeValue*>& root_container) {
	root_containers.emplace_back(root_container);
}


void GarbageCollector::mark() {
	size_t i = 0;
	while (i < roots.size()) {
		std::visit([this, i](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, std::weak_ptr<GCObject>>) {
				if (auto root = arg.lock()) {
					mark_object(root.get());
				}
				else {
					roots.erase(roots.begin() + i);
				}
			}
			else if constexpr (std::is_same_v<T, RuntimeValue**>) {
				mark_object(*arg);
			}
			}, roots[i]);
		++i;
	}

	for (const auto& iterable_ptr : root_containers) {
		for (auto item : iterable_ptr) {
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
