#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkClassNameRegistry;
class hkStaticClassNameRegistry;
class hkTypeInfoRegistry;
class hkVtableClassRegistry;

// The registries of the types built into the executable: their type infos, classes by name and classes by vtable.
// The exe has no vtable of its own for this abstract class; its slots 3 to 7 are the PDB's intro virtuals.
class hkBuiltinTypeRegistry : public hkReferencedObject, public hkSingleton<hkBuiltinTypeRegistry> {
public:
	// The classes linked into the executable, null terminated (.rdata, 405 classes).
	using StaticLinkedClassesArray = const hkClass* const[406];
	// The PDB's name. The type infos linked into the executable, null terminated (.rdata, 352 records).
	using StaticLinkedTypeInfosArray = const hkTypeInfo* const[353];

	virtual hkTypeInfoRegistry* getTypeInfoRegistry() = 0;
	virtual hkClassNameRegistry* getClassNameRegistry() = 0;
	virtual hkVtableClassRegistry* getVtableClassRegistry() = 0;
	// Not reconstructed yet. thiscall, ret 8, 0xD477C0 (slot 6 of hkDefaultBuiltinTypeRegistry's table).
	virtual void addType(const hkTypeInfo* info, const hkClass* klass);
	virtual void reinitialize() = 0;

	// hkSingleton<hkBuiltinTypeRegistry>::s_instance. The packfile data's constructor reads it and takes the class
	// registry through slot 4 (getClassNameRegistry).
	static inline hkBuiltinTypeRegistry* s_instance;
	static StaticLinkedClassesArray StaticLinkedClasses;
	static StaticLinkedTypeInfosArray StaticLinkedTypeInfos;
};

namespace hkHavokCurrentClasses {
// The registry of StaticLinkedClasses under the current Havok version's name.
extern hkStaticClassNameRegistry hkHavokDefaultClassRegistry;
} // namespace hkHavokCurrentClasses

// The registry backed by the singleton registries. The exe's vtable overrides the three registry getters and
// reinitialize; addType is hkBuiltinTypeRegistry's and the destructor hkBaseObject's.
class hkDefaultBuiltinTypeRegistry : public hkBuiltinTypeRegistry {
public:
	hkDefaultBuiltinTypeRegistry();

	hkTypeInfoRegistry* getTypeInfoRegistry() override;
	hkClassNameRegistry* getClassNameRegistry() override;
	hkVtableClassRegistry* getVtableClassRegistry() override;
	void reinitialize() override;

	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void init();
};
inline constexpr fnvd::Vtable hkDefaultBuiltinTypeRegistry_vftable{};
