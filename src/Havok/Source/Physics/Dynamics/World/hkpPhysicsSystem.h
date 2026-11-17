#pragma once

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkTransform;
class hkpAction;
class hkpConstraintInstance;
class hkpPhantom;
class hkpRigidBody;

void tranformPhantom(hkpPhantom* phantom, const hkTransform& transformation);

// A set of rigid bodies, phantoms, constraints and actions that are added to a world and removed from it together.
class hkpPhysicsSystem : public hkReferencedObject {
public:
	// The reflection's member table (the PDB's hkpPhysicsSystem::Members).
	using MembersArray = const hkClassMember[7];

	hkpPhysicsSystem();

	hkpPhysicsSystem(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_rigidBodies(aFlag), m_constraints(aFlag), m_actions(aFlag), m_phantoms(aFlag),
		  m_name(aFlag) {}

	~hkpPhysicsSystem() override;

	virtual hkpPhysicsSystem* clone() const;
	// The exe returns the hkBool through a hidden result pointer (ret 4).
	virtual hkBool* hasContacts(hkBool* arResult);

	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
	void removeAll();
	int32_t removeRigidBody(int32_t a1);
	int32_t removeConstraint(int32_t a1);
	int32_t removeAction(int32_t a1);
	void addRigidBody(hkpRigidBody* rb);
	void addConstraint(hkpConstraintInstance* c);
	void addAction(hkpAction* a);
	// Not reconstructed yet. Moves the rigid bodies, constraints and phantoms by temptrans (thiscall, ret 4; the exe
	// copies the 64-byte transform at temptrans and passes it on to hkpConstraintInstance's transform, 0xCF30F0).
	void transform(const hkTransform& transformation);
	int32_t removePhantom(int32_t a1);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes).
	void removeNullPhantoms();
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
	void copy(const hkpPhysicsSystem& toCopy);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
	void addPhantom(hkpPhantom* p);

	static const hkClass& staticClass();

	hkArray<hkpRigidBody*> m_rigidBodies;
	hkArray<hkpConstraintInstance*> m_constraints;
	hkArray<hkpAction*> m_actions;
	hkArray<hkpPhantom*> m_phantoms;
	hkStringPtr m_name;
	hkUlong m_userData;
	hkBool m_active;

	static MembersArray Members;
};

extern hkClass hkpPhysicsSystemClass;
extern hkTypeInfo hkpPhysicsSystemTypeInfo;
inline constexpr fnvd::Vtable hkpPhysicsSystem_vftable{};
