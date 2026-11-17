#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"

#include <stdint.h>

class hkpPhysicsSystem;
class hkpRigidBody;
class hkxMesh;

void cleanupLoadedObjecthkpDisplayBindingDataPhysicsSystem(void* p);
void cleanupLoadedObjecthkpDisplayBindingData(void* p);
void cleanupLoadedObjecthkpDisplayBindingDataRigidBody(void* p);
void finishLoadedObjecthkpDisplayBindingDataRigidBody(void* p, int32_t finishing);
// Havok SDK signature (hkpUtilities.lib, tied by reloc,target).
const void* getVtablehkpDisplayBindingDataRigidBody();
void finishLoadedObjecthkpDisplayBindingDataPhysicsSystem(void* p, int32_t finishing);
// Havok SDK signature (hkpUtilities.lib, tied by reloc,target).
const void* getVtablehkpDisplayBindingDataPhysicsSystem();
void finishLoadedObjecthkpDisplayBindingData(void* p, int32_t finishing);
// Havok SDK signature (hkpUtilities.lib, tied by reloc,target).
const void* getVtablehkpDisplayBindingData();

// Associates saved physics objects with their display meshes.
struct hkpDisplayBindingData : public hkReferencedObject {
	struct RigidBody : public hkReferencedObject {
		RigidBody(hkFinishLoadedObjectFlag aFlag);
		~RigidBody() override;

		static const hkClass& staticClass();

		hkRefPtr<hkpRigidBody> m_rigidBody;
		hkRefPtr<hkxMesh> m_displayObject;
		hkMatrix4 m_rigidBodyFromDisplayObjectTransform;
	};

	struct PhysicsSystem : public hkReferencedObject {
		PhysicsSystem(hkFinishLoadedObjectFlag aFlag);
		~PhysicsSystem() override;

		static const hkClass& staticClass();

		hkArray<hkRefPtr<RigidBody>> m_bindings;
		hkRefPtr<hkpPhysicsSystem> m_system;
	};

	hkpDisplayBindingData(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_rigidBodyBindings(aFlag), m_physicsSystemBindings(aFlag) {}

	~hkpDisplayBindingData() override;

	static const hkClass& staticClass();

	hkArray<hkRefPtr<RigidBody>> m_rigidBodyBindings;
	hkArray<hkRefPtr<PhysicsSystem>> m_physicsSystemBindings;
};

extern hkClass hkpDisplayBindingDataRigidBodyClass;
extern hkTypeInfo hkpDisplayBindingDataRigidBodyTypeInfo;
extern hkClass hkpDisplayBindingDataPhysicsSystemClass;
extern hkTypeInfo hkpDisplayBindingDataPhysicsSystemTypeInfo;
extern hkClass hkpDisplayBindingDataClass;
extern hkTypeInfo hkpDisplayBindingDataTypeInfo;
inline constexpr fnvd::Vtable hkpDisplayBindingData_RigidBody_vftable{};
inline constexpr fnvd::Vtable hkpDisplayBindingData_PhysicsSystem_vftable{};
inline constexpr fnvd::Vtable hkpDisplayBindingData_vftable{};
