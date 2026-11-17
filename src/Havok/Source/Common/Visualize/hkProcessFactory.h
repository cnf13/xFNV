#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"

class hkCriticalSection;
class hkProcess;
class hkProcessContext;

// The SDK's HK_SINGLETON_IMPLEMENTATION create function: a new factory from the heap.
// Havok SDK signature (hkVisualize.lib, tied by bytes,data).
hkReferencedObject* hkProcessFactorycreate();

// The registry of the visual debugger's processes: each name with the function that creates the process. Only some of
// its members are declared.
class hkProcessFactory : public hkReferencedObject, public hkSingleton<hkProcessFactory> {
public:
	// A process's create function.
	using CreationFunction = hkProcess* (*)(const hkArray<hkProcessContext*>& contexts);

	// One registered process: its name, create function and tag.
	struct ProcessIdPair {
		hkStringPtr m_name;
		CreationFunction m_processCreationFunction;
		int32_t m_tag;
	};

	hkProcessFactory();
	~hkProcessFactory() override;

	// Registers processName with creationFunction unless a process of that name is registered; returns the process's
	// tag.
	int32_t registerProcess(const char* viewerName, CreationFunction creationFunction);
	// Creates the process registered as name with contexts: null when getProcessId finds no tag for name, else the
	// process createProcessByTag creates.
	hkProcess* createProcess(const char* name, hkArray<hkProcessContext*>& contexts);
	// The createProcess(int tag, ...) overload: makes, from contexts, the process whose create function is stored at
	// tag.
	hkProcess* createProcessByTag(int32_t tag, hkArray<hkProcessContext*>& contexts);
	// The tag of the process registered as name, or -1.
	int32_t getProcessId(const char* name);
	// Returns the name registered at tag id (the entry's hkStringPtr without its flag bit).
	const char* getProcessName(int32_t id);

	// The tag the next registered process gets.
	int32_t m_freeTag;
	hkArray<ProcessIdPair> m_name2creationFunction;
	hkCriticalSection* m_criticalSection;

	// hkSingleton<hkProcessFactory>::s_instance.
	static inline hkProcessFactory* s_instance;
};
