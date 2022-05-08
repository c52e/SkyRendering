#pragma once

#include <unordered_set>

template<class T>
class ObjectsSet {
protected:
	ObjectsSet() {
		objects.insert(static_cast<T*>(this));
	}
	ObjectsSet(const ObjectsSet&) : ObjectsSet() {
	}

	~ObjectsSet() {
		objects.erase(static_cast<T*>(this));
	}

	static const std::unordered_set<T*>& GetObjects() {
		return objects;
	}
	
private:
	static inline std::unordered_set<T*> objects;
};
