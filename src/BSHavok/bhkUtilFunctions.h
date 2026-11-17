#pragma once

#include "BSHavok/bhkCollisionFilter.h"
#include "BSCore/BSSimpleArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Reconstruction.hpp"

#include <stdint.h>

enum HavokMaterialType : int32_t;
class bhkBlendCollisionObject;
class bhkCollisionObject;
class bhkPCollisionObject;
class bhkRigidBody;
class bhkShapeCollection;
class hkSphere;
class hkpCdBody;
class hkpCollidable;
class hkpRigidBody;
class hkpWorldObject;

// Free Havok helpers over scene graph objects.
namespace bhkUtilFunctions {

// A scene graph object and its depth: the breadth-first search's queue entry.
struct AVLevel {
	NiAVObject* pObj;
	uint32_t uiLevel;
};

// The wrapper of the shape collection apBody (or its parent) is in, or null.
bhkShapeCollection* FindbhkShapeCollection(const hkpCdBody* arBody);

NiAVObject* GetNiAVObject(const hkpCollidable* apCollidable);

// The blend collision object of apNode or below it (cdecl, 0xC81C50). IDA name; not in the
// PDB.
bhkBlendCollisionObject* FindbhkBlendCollisionObject(NiNode* apNode);

// The heir gain of apNode's blend collision object, 1 without one. IDA name; not in the PDB.
// Inferred type: return: float from 0xc82598 (bhkBlendCollisionObject::fHeirGain (result of
// bhkUtilFunctions::FindbhkBlendCollisionObject))
float GetHeirGain(NiNode* apNode);

// apBody's collision object when it is a bhkPCollisionObject, else null. IDA name; not in the PDB.
bhkPCollisionObject* IsbhkPCollisionObject(hkpWorldObject* apBody);

// The first collision object under apObject, searched breadth first to at most auiMaxLevel levels down.
bhkCollisionObject* FindCollisionObjectBFS(NiAVObject* apObject, const uint32_t auiMaxLevel);

// Whether the object's rigid body (from its collision object, else found through its BSXFlags) is awake. Not
// reconstructed yet.
bool IsRigidBodyActive(NiAVObject* apObject);

// Replaces the constraints of the object's ragdoll with ones of the type aeType.
void ReplaceConstraints(NiAVObject* apObject, hkpConstraintData::ConstraintType aeType);

// Damps the velocity of the object's rigid bodies by afDamp.
void DampBodiesVelocity(NiAVObject* apObject, float afDamp);

// The collision object of apObj or of the first node below it that has one.
bhkCollisionObject* FindNextCollisionObject(NiAVObject* apObj);

// Appends the rigid body of every collision object of apObject and of the nodes below it to
// arBodyA.
void GatherRigidBodies(NiAVObject* apObject, hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arBodyA);

// IDA name; not in the PDB. Writes the knocked-down movement of apObject into arOut; the exe reads abAlive as a byte
// (cdecl, 0xC821B0).
// Inferred type: apObject: NiNode* from 0x886bee (result of TESObjectREFR::Get3D), its members used at 0xc821bf; arOut:
// NiPoint3* from 0x886bee (parameter 0 of Actor::GetKnockedDownMovementDelta), its members used at 0xc821b0
void GetKnockedDownMovementDelta(NiNode* apObject, NiPoint3* arOut, bool abAlive);

// The rigid body under apRoot closest to arSphere, skipping the head with abIgnoreHead, or
// null (cdecl, 0xC806B0). The exe passes the caller's whole dword on; bool would be zero-extended.
bhkRigidBody* GetClosestBody(NiAVObject* apRoot, hkSphere& arSphere, int32_t abIgnoreHead);

// IDA name; not in the PDB. The summed mass of the rigid bodies of apObject and all nodes
// below it (cdecl, returns ST0 from a float, 0x62C300).
// Inferred type: return: float from 0x95f930 (passed to BSMAX 1); apObject: NiAVObject* from 0x62c3a6 (result of
// NiNode::GetAt), its members used at 0x62c35b
float GetTotalMass(NiAVObject* apObject);

// Sets or clears the use-velocity flag of the collision objects of
// apObject and all nodes below it. It tests the flag's low byte only.
void SetUseVelocityRecurse(bool abUseVelocity, NiAVObject* apObject);

// Not named in the PDB or IDA; name chosen here. A fixed wooden capsule body of height afHeight (at least twice the
// radius plus one) and radius afRadius, standing on the origin along z, in Havok units (cdecl, 0xC81380; the tree
// models' collision).
// Inferred type: afHeight: float from 0x66741f (BSTreeModel::fTrunkHeight (this of BSTreeModel::CreateGeometry));
// afRadius: float: used as one at 0xc813d4
bhkRigidBody* CreateCapsuleBody(float afHeight, float afRadius);

// The PDB's bhkUtilFunctions::FindBipedPart(NiAVObject*, BIPED_PART), returning its bhkRigidBody* (cdecl, 0xC7FF40);
// kept under this name for its caller in ArrowProjectile.cpp.
bhkRigidBody* FindBipedPart(NiAVObject* a1, int32_t a2);
// The material of a collidable's shape key, asked of the
// bhkShapeCollection behind its parent's shape.
// IDA: hkpCollidable*; the body reads only hkpCdBody members (m_parent, m_shape, m_shapeKey) and
// GetMaterialFromCollidable passes it the hkpCdBody it is given.
HavokMaterialType GetMaterialValue(const hkpCdBody* a1);
} // namespace bhkUtilFunctions

namespace bhkUtilFunctions {
const hkpRigidBody* FindConstrainedSystemRoot(const hkpRigidBody* apBody);
}  // namespace bhkUtilFunctions

namespace bhkUtilFunctions {
// The closest rigid body GetClosestBodyRec found so far and its contact with the sphere (PDB members and offsets;
// GetClosestBody's local, zeroed pBody first, 0xC806B0).
struct ClosestBody {
	bhkRigidBody* pBody;
	hkContactPoint Contact;
};

// IDA name GetClosestBodyFunctor; the PDB's GetClosestBodyRec, the recursion GetClosestBody
// starts: for apObject and every node below it, the rigid body whose shape is closer to arSphere than
// arClosest's goes into arClosest (cdecl, 0xC7FCC0). The exe passes the caller's whole dword on; bool would be
// zero-extended.
char GetClosestBodyRec(NiAVObject* apObject, hkSphere& arSphere, ClosestBody& arClosest, int32_t abIgnoreHead);
}  // namespace bhkUtilFunctions

namespace bhkUtilFunctions {
// Sets the layer of the collision filter of apObject's rigid body and of every one below it.
void SetLayer(NiAVObject* apObject, COL_LAYER aeLayer);
}  // namespace bhkUtilFunctions

namespace bhkUtilFunctions {
void RemoveSpringConstraints(NiAVObject* apObject);
}  // namespace bhkUtilFunctions

namespace bhkUtilFunctions {
void GatherConstrainedBodies(const hkpRigidBody* apBody,
	hkArray<const hkpRigidBody*, hkContainerHeapAllocator>& arBodyA);
}  // namespace bhkUtilFunctions

namespace bhkUtilFunctions {
// One step of FindCollisionObjectBFS's breadth-first search.
bhkCollisionObject* FindCollisionObjectBFSRec(NiAVObject* apObject,
	BSSimpleArray<bhkUtilFunctions::AVLevel, 1024>& apQueue, const uint32_t auiMaxLevel, uint32_t auiCurLevel);
}  // namespace bhkUtilFunctions
