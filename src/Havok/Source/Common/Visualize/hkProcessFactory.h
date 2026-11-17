#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Reconstruction.hpp"

class hkCriticalSection;
class hkProcess;
class hkProcessContext;

// The SDK's HK_SINGLETON_IMPLEMENTATION create function: a new factory from the heap.
// Havok SDK signature (hkVisualize.lib, tied by bytes,data).
hkReferencedObject* hkProcessFactorycreate();

// The registry of the visual debugger's processes: each name with the function that creates the process. Only the
// members the reconstructed code calls are declared.
class hkProcessFactory : public hkReferencedObject, public hkSingleton<hkProcessFactory> {
public:
	// A process's create function.
	using CreationFunction = hkProcess* (*)(const hkArray<hkProcessContext*>& contexts);

	// One registered process: its name, create function and tag (the factory's 12-byte entries; members from the
	// SDK, layout from Havok's release hkvisualize.pdb).
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
	// Not reconstructed yet. Creates the process registered as name with contexts: null when getProcessId finds no tag
	// for name, else the process the tag overload (0xDD92F0) creates; both inside the import calls 0xFDF05C and
	// 0xFDF100 on the factory's +0x18 (thiscall, ret 8, 0xDD9330).
	hkProcess* createProcess(const char* name, hkArray<hkProcessContext*>& contexts);
	// The SDK's and PDB's createProcess(int tag, ...) overload, named apart here: a second
	// createProcess would make the emitted registration of 0xDD9330 ambiguous. The process the create function
	// registered at tag (the 12-byte entries at +0xC, the function at +0x4) makes from contexts, called inside the
	// import calls 0xFDF05C and 0xFDF100 on the factory's +0x18 (thiscall, ret 8, 0xDD92F0).
	hkProcess* createProcessByTag(int32_t tag, hkArray<hkProcessContext*>& contexts);
	// The tag of the process registered as name, or -1.
	int32_t getProcessId(const char* name);
	// The name registered at tag (the entry's hkStringPtr without its flag bit), read inside
	// the import calls 0xFDF05C and 0xFDF100 on the factory's +0x18 (thiscall, ret 4, 0xDD9240; IDA keeps it inside
	// hkIArchive's vector deleting destructor).
	const char* getProcessName(int32_t id);

	// The tag the next registered process gets.
	int32_t m_freeTag;
	hkArray<ProcessIdPair> m_name2creationFunction;
	hkCriticalSection* m_criticalSection;

	// hkSingleton<hkProcessFactory>::s_instance.
	static inline hkProcessFactory* s_instance;
};
inline constexpr fnvd::Vtable hkProcessFactory_vftable{};
