#pragma once

#include "Reconstruction.hpp"

class hkReferencedObject;

// Havok's singleton holder: no data, only the static instance pointer and the access functions. The instance
// pointer itself is declared where the exe's address belongs, on the singleton's own class (T::s_instance), because
// each instance has its own address. replaceInstance releases the old instance (removeReferenceLockUnchecked) and
// stores the new one.
template <typename T> class hkSingleton {
public:
	// The SDK's protected assignment, which the exe keeps out of line: returns this and copies nothing.
	hkSingleton& operator=(const hkSingleton& other);

	static T* getInstance();
	static void replaceInstance(T* t);
	static bool isInitialised();
};
// The SDK's inline bodies, on the singleton's own instance pointer T::s_instance.
template <typename T> T* hkSingleton<T>::getInstance() {
	return T::s_instance;
}

// Releases the old instance, then stores apInstance.
template <typename T> void hkSingleton<T>::replaceInstance(T* t) {
	if (T::s_instance) {
		T::s_instance->removeReferenceLockUnchecked();
	}
	T::s_instance = t;
}

template <typename T> bool hkSingleton<T>::isInitialised() {
	return T::s_instance != nullptr;
}

// Copies nothing.
template <typename T> hkSingleton<T>& hkSingleton<T>::operator=([[maybe_unused]] const hkSingleton& other) {
	return *this;
}
// One registered singleton, in a static list linked through m_next: its name, the function creating it and where the
// instance is stored (findByName, 0xCF69C0, walks m_next comparing m_name). Layout as IDA's struct and the PC PDB.
struct hkSingletonInitNode {
	typedef hkReferencedObject* (*SingletonCreationFunction)();

	// The SDK's constructor (inline, defined below the list head): the node links itself in at the list's head. The
	// exe's initialisers (hkSingletonRegister<Class>) keep only the two link stores: the other fields are image data.
	hkSingletonInitNode(const char* name, SingletonCreationFunction func, void** ptr);

	hkSingletonInitNode* findByName(const char* name);

	// Gives each dstBase node the instance of the srcBase node of its name.
	static void populate(hkSingletonInitNode* dstBase, const hkSingletonInitNode* srcBase);
	// Releases and clears the instance of each dstBase node a srcBase node shares a name with.
	static void depopulate(hkSingletonInitNode* dstBase, const hkSingletonInitNode* srcBase);

	const char* m_name;
	SingletonCreationFunction m_createFunc;
	hkSingletonInitNode* m_next;
	void** m_value;
};

// The head of the registered singletons' list (each node's static constructor links it in at the head).
inline hkSingletonInitNode* hkSingletonInitList;

inline hkSingletonInitNode::hkSingletonInitNode(const char* name, SingletonCreationFunction func, void** ptr)
	: m_name(name), m_createFunc(func), m_value(ptr) {
	m_next = hkSingletonInitList;
	hkSingletonInitList = this;
}
