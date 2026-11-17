#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Memory/MemoryClasses/hkMemoryClasses.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Serialize/Resource/hkResource.h"
#include "Reconstruction.hpp"

class hkClassNameRegistry;
class hkStatisticsCollector;
class hkTypeInfoRegistry;

// The PDB's baseOrSameClass of hkObjectResource.cpp: whether topLevelClassName's class
// (classReg's slot 4, getClassByName) is typeName's class or a superclass of it (hkClass::isSuperClass). The exe
// passes typeName in ecx and classReg in esi, topLevelClassName on the stack (usercall, ret 0, 0xE04E10); no exe
// call reaches it.
bool baseOrSameClass(const char* typeName, const char* topLevelClassName, const hkClassNameRegistry* classReg);
// The PDB's baseOrSameClass of hkPackfileData.cpp (the PDB's other one, of
// hkObjectResource.cpp, is src's baseOrSameClass at 0xE04E10, the same code); the address suffix is chosen here,
// as src cannot hold two free functions of that name. Whether baseName's class (registry slot 4, getClassByName) is
// name's class or a superclass of it (hkClass::isSuperClass). The exe passes name in ecx and reg in esi, baseName
// on the stack (usercall, ret 0, 0xE050C0); no exe call reaches it.
bool baseOrSameClass_E050C0(const char* name, const char* baseName, const hkClassNameRegistry* reg);

// Owns the allocations, imports, exports and type metadata of a loaded packfile.
class hkPackfileData : public hkResource {
public:
	struct Chunk;

	// Empty, with destructors enabled and the builtin type registry's class registry.
	hkPackfileData();

	~hkPackfileData() override;

	const char* getName() const override { return m_name; }
	void callDestructors() override;
	// Havok SDK signature (hkSerialize.lib, tied by bytes,vtable).
	void getImportsExports(hkArray<hkResource::Import, hkContainerHeapAllocator>& impOut,
		hkArray<hkResource::Export, hkContainerHeapAllocator>& expOut) const override;
	// Havok SDK signature (hkSerialize.lib, tied by bytes,vtable).
	void* getContentsPointer(const char* typeName, const hkTypeInfoRegistry* typeRegistry) const override;
	const char* getContentsTypeName() const override;
	// Havok SDK signature (hkSerialize.lib, tied by bytes,vtable).
	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;

	void setName(const char* n);
	void setPackfileClassNameRegistry(const hkClassNameRegistry* classReg);
	void removeExport(void* object);
	void removeImport(void** location);
	// Whether the tracked objects have been finished: the type map is not empty.
	int32_t finishedObjects() const;
	void addExport(const char* symbolName, void* object);
	void addImport(const char* symbolName, void** location);
	// Appends the chunk {p, n, c} to m_chunks (inline in the SDK's header; the exe's copy sits among
	// hkDataObjectUtil.cpp's functions, its callers Copier's copyIntoNativeArray, fillNativeMembers and findOrCopy)
	// (thiscall, ret 0xC, 0xDC3CE0).
	void addChunk(void* p, int32_t n, HK_MEMORY_CLASS c);
	void setContentsWithName(void* topLevelObject, const char* typeName);

	void* m_topLevelObject;
	char* m_name;
	hkPointerMap<void*, char const*, hkContainerHeapAllocator> m_trackedObjects;
	hkStringMap<hkTypeInfo const*, hkContainerHeapAllocator> m_trackedTypes;
	hkRefPtr<hkClassNameRegistry const> m_packfileClassRegistry;
	int32_t m_destructorsEnabled;
	hkArray<void*, hkContainerHeapAllocator> m_memory;
	hkArray<Chunk, hkContainerHeapAllocator> m_chunks;
	hkArray<hkResource::Export, hkContainerHeapAllocator> m_exports;
	hkArray<hkResource::Import, hkContainerHeapAllocator> m_imports;
};

// A block the packfile data owns: freed with its byte count and memory class (addChunk stores the three at +0, +4,
// +8, 0xDC3CE0).
struct hkPackfileData::Chunk {
	void* pointer;
	int32_t numBytes;
	HK_MEMORY_CLASS memClass;
};

inline constexpr fnvd::Vtable hkPackfileData_vftable{};
