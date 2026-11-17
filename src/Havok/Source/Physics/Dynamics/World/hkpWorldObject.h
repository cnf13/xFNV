#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/DebugUtil/MultiThreadCheck/hkMultiThreadCheck.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationQueue.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class bhkWorldObject;
class hkMotionState;
class hkStatisticsCollector;
class hkpShape;
class hkpWorld;

// The value a property carries: a pointer sized word the engine stores whole (the PDB's and the SDK's name).
union hkpPropertyValue {
	// The SDK's inline hkpPropertyValue::getPtr: the stored word, untyped raw data, as a pointer.
	void* getPtr() const { return reinterpret_cast<void*>(static_cast<uintptr_t>(m_data)); }

	// The reflection class hkpPropertyValueClass (cdecl, 0xDA0610); the PDB's hkpPropertyValue::staticClass.
	static const hkClass& staticClass();

	uint64_t m_data;
};

// One entry of a world object's property list: a key and the value bound to it (the PDB's and the SDK's name).
class hkpProperty {
public:
	// The reflection class hkpPropertyClass (cdecl, 0xDA0630); the PDB's hkpProperty::staticClass.
	static const hkClass& staticClass();

	uint32_t m_key;
	uint32_t m_alignmentPadding;
	hkpPropertyValue m_value;
};

// The AI module's data a world object can carry (hkpWorldObject::m_aiData). The exe never allocates or extends one,
// only calls hkReferencedObject's methods on it (setAiObjectData 0xCD29C0 adds and removes references and stores it
// in m_aiData; getAiObjectData 0xCD29B0 returns m_aiData), so it adds nothing to hkReferencedObject's layout.
class hkaiObjectData : public hkReferencedObject {};

// An object a Havok world tracks: the world it belongs to, the collision hierarchy others see it through,
// and the properties the game code hangs off it.
class hkpWorldObject : public hkReferencedObject {
public:
	// What the object's broad phase handle (m_collidable.m_broadPhaseHandle.m_type) says it is.
	enum BroadPhaseType {
		BROAD_PHASE_INVALID = 0x0,
		BROAD_PHASE_ENTITY = 0x1,
		BROAD_PHASE_PHANTOM = 0x2,
		BROAD_PHASE_BORDER = 0x3,
		BROAD_PHASE_MAX_ID = 0x4,
	};

	// Whether a property write also runs the engine's threading checks: the PDB's enum (underlying int) and the
	// SDK's. The exe passes it as a dword and this build's editProperty ignores it.
	enum MtChecks : int32_t {
		MULTI_THREADING_CHECKS_ENABLE = 0x0,
		MULTI_THREADING_CHECKS_IGNORE = 0x1,
	};

	// The reflected members of hkpWorldObject, as the exe's .rdata holds them.
	using MembersArray = const hkClassMember[8];

	hkpWorldObject(const hkpShape* a1, hkpWorldObject::BroadPhaseType a2);
	hkpWorldObject(hkFinishLoadedObjectFlag flag);
	~hkpWorldObject() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;

	// A plain world object accepts any shape.
	virtual hkWorldOperation::Result setShape(const hkpShape*) { return hkWorldOperation::DONE; }
	virtual hkMotionState* getMotionState() = 0;

	// The value bound to key, zero when no entry has the key (the SDK's inline body; its MtChecks only drives the
	// debug build's read check). The exe's out-of-line copy (thiscall, ret 0xC, the value through the hidden result
	// pointer; 0xC6D840) is registered in hkpWorldObject.cpp.
	hkpPropertyValue getProperty(uint32_t key, MtChecks mtCheck = MULTI_THREADING_CHECKS_ENABLE) const {
		for (int32_t i = 0; i < m_properties.m_size; ++i) {
			if (m_properties.m_data[i].m_key == key) {
				return m_properties.m_data[i].m_value;
			}
		}
		hkpPropertyValue returnValue;
		returnValue.m_data = 0;
		return returnValue;
	}

	// Binds a value to a key, appending an entry when the key is not taken.
	void addProperty(uint32_t key, hkpPropertyValue value);

	// Unbinds a key, closing the hole it leaves, and returns the value it carried.
	hkpPropertyValue removeProperty(uint32_t key);

	// Stores aValue in the entry of aiKey, leaving the list alone when the key is absent (thiscall, ret 0x10; the
	// PDB's third parameter is the MtChecks the exe's body ignores).
	void editProperty(uint32_t key, hkpPropertyValue value, MtChecks mtCheck = MULTI_THREADING_CHECKS_ENABLE);

	// Whether the object is in a world (the SDK's inline body). The exe's out-of-line copy (thiscall, ret 4, the
	// hkBool through the hidden result pointer; 0x8CFFF0) is registered in hkpWorldObject.cpp.
	hkBool isAddedToWorld() const { return m_world != nullptr; }
	// The game object the Havok object belongs to (the SDK's inline body; the exe's out-of-line copy is the shared
	// one-field getter 0x84E3A0). PDB: hkUlong; src's m_userData is the bhkWorldObject it holds.
	bhkWorldObject* getUserData() const { return m_userData; }
	// Sets m_name to name (the SDK's inline body). The exe's out-of-line copy (thiscall, ret 4, 0x622F60) is
	// registered in DecalCaster.cpp.
	void setName(const char* name) { m_name = name; }
	// The world the object is in, m_world. Same function as TESForm::GetFormFlags (0x44DDC0), registered there.
	hkpWorld* getWorld() const { return m_world; }
	// &m_collidable (the SDK's inline body; the debug build's read check is compiled out). The exe's out-of-line copy
	// is the shared add-0x10 getter 0x460140.
	const hkpCollidable* getCollidable() const { return &m_collidable; }
	// The SDK's inline getMultiThreadCheck().markForRead() and unmarkForRead(): in the game's build the getter is a
	// one-field accessor and both marks are empty functions (0x43B5D0, 0x4534F0, 0x483710), so nothing is marked.
	// The exe's out-of-line copies (thiscall, ret 0; 0x43B5B0, 0x43B5F0) are registered in hkpWorldObject.cpp.
	void markForRead() {}
	void unmarkForRead() {}
	void markForWriteImpl();
	void checkReadWrite();
	void checkReadOnly() const;
	// Havok SDK signature (hkpDynamics.lib, tied by reloc).
	void setTreeData(void* data);
	void* getTreeData() const;
	// Havok SDK signature (hkpDynamics.lib, tied by target).
	void unlockProperty(uint32_t key);
	// Makes m_properties a copy of otherObj's (the SDK's inline body; the exe's out-of-line copy is thiscall, ret 4,
	// 0xCA92A0).
	void copyProperties(const hkpWorldObject* otherObj);
	void lockProperty(uint32_t key);
	hkaiObjectData* getAiObjectData();
	void setAiObjectData(hkaiObjectData* data);

	// PDB and SDK: hkpWorld*. Game code casts it to the game's world subclass where it needs that class.
	hkpWorld* m_world;
	bhkWorldObject* m_userData;
	hkpLinkedCollidable m_collidable;
	hkMultiThreadCheck m_multiThreadCheck;
	hkStringPtr m_name;
	hkArray<hkpProperty> m_properties;
	hkReferencedObject* m_aiData;
	void* m_treeData;

	static MembersArray Members;
};

inline constexpr fnvd::Vtable hkpWorldObject_vftable{};

// The reflection class of hkpWorldObject, which hkpPhantom and hkpEntity pass to the base statistics.
extern hkClass hkpWorldObjectClass;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpPropertyValueTypeInfo;
extern hkTypeInfo hkpPropertyTypeInfo;
extern hkClass hkpPropertyValueClass;
extern hkClass hkpPropertyClass;
