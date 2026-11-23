#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

#include <stdint.h>

class bhkBlendCollisionObject;
class bhkCollisionObject;
class bhkPCollisionObject;
class bhkRigidBody;
class bhkShapeCollection;
class hkpCdBody;
class hkpCollidable;
class hkpRigidBody;
class hkpWorldObject;
class hkSphere;

enum HavokMaterialType : int32_t;

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

    // The blend collision object of apNode or below it.
    bhkBlendCollisionObject* FindbhkBlendCollisionObject(NiNode* apNode);

    // The heir gain of apNode's blend collision object, 1 without one.
    float GetHeirGain(NiNode* apNode);

    // apBody's collision object when it is a bhkPCollisionObject, else null.
    bhkPCollisionObject* IsbhkPCollisionObject(hkpWorldObject* apBody);

    // The first collision object under apObject, searched breadth first to at most auiMaxLevel levels down.
    bhkCollisionObject* FindCollisionObjectBFS(NiAVObject* apObject, const uint32_t auiMaxLevel);

    // Whether the object's rigid body (from its collision object, else found through its BSXFlags) is awake.
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

    // Writes the knocked-down movement of apObject into arOut.
    void GetKnockedDownMovementDelta(NiNode* apObject, NiPoint3* arOut, bool abAlive);

    // The rigid body under apRoot closest to arSphere, skipping the head with abIgnoreHead, or null. abIgnoreHead is
    // int32_t because the caller's whole value is passed on; bool would be zero-extended.
    bhkRigidBody* GetClosestBody(NiAVObject* apRoot, hkSphere& arSphere, int32_t abIgnoreHead);

    // The summed mass of the rigid bodies of apObject and all nodes below it.
    float GetTotalMass(NiAVObject* apObject);

    // Sets or clears the use-velocity flag of the collision objects of
    // apObject and all nodes below it. It tests the flag's low byte only.
    void SetUseVelocityRecurse(bool abUseVelocity, NiAVObject* apObject);

    // A fixed wooden capsule body of height afHeight (at least twice the radius plus one) and radius afRadius, standing
    // on the origin along z, in Havok units (the tree models' collision).
    bhkRigidBody* CreateCapsuleBody(float afHeight, float afRadius);

    bhkRigidBody* FindBipedPart(NiAVObject* apObject, int32_t aiPart);
    // The material of a collidable's shape key, asked of the bhkShapeCollection behind its parent's shape. The body
    // reads only hkpCdBody members (m_parent, m_shape, m_shapeKey) and GetMaterialFromCollidable passes it the
    // hkpCdBody it is given.
    HavokMaterialType GetMaterialValue(const hkpCdBody* apCdBody);
} // namespace bhkUtilFunctions

namespace bhkUtilFunctions {
    const hkpRigidBody* FindConstrainedSystemRoot(const hkpRigidBody* apBody);
} // namespace bhkUtilFunctions

namespace bhkUtilFunctions {
    // The closest rigid body GetClosestBodyRec found so far and its contact with the sphere.
    struct ClosestBody {
        bhkRigidBody* pBody;
        hkContactPoint Contact;
    };

    // The recursion GetClosestBody starts: for apObject and every node below it, the rigid body whose shape is closer
    // to arSphere than arClosest's goes into arClosest. abIgnoreHead is int32_t because the caller's whole 32-bit value
    // is passed on; bool would be zero-extended.
    char GetClosestBodyRec(NiAVObject* apObject, hkSphere& arSphere, ClosestBody& arClosest, int32_t abIgnoreHead);
} // namespace bhkUtilFunctions

namespace bhkUtilFunctions {
    // Sets the layer of the collision filter of apObject's rigid body and of every one below it.
    void SetLayer(NiAVObject* apObject, COL_LAYER aeLayer);
} // namespace bhkUtilFunctions

namespace bhkUtilFunctions {
    void RemoveSpringConstraints(NiAVObject* apObject);
} // namespace bhkUtilFunctions

namespace bhkUtilFunctions {
    void GatherConstrainedBodies(
        const hkpRigidBody* apBody,
        hkArray<const hkpRigidBody*, hkContainerHeapAllocator>& arBodyA
    );
} // namespace bhkUtilFunctions

namespace bhkUtilFunctions {
    // One step of FindCollisionObjectBFS's breadth-first search.
    bhkCollisionObject* FindCollisionObjectBFSRec(
        NiAVObject* apObject,
        BSSimpleArray<bhkUtilFunctions::AVLevel, 1024>& apQueue,
        const uint32_t auiMaxLevel,
        uint32_t auiCurLevel
    );
} // namespace bhkUtilFunctions

// For the AVLevel array, SetAt inserts aValue at aiIndex, moving the later items up. Its caller (bhkUtilFunctions.cpp,
// apQueue.SetAt(0, ...)) relies on that. Defined in BSSimpleArray.cpp. Declared here, where AVLevel is complete: the
// declaration instantiates the class, and cl compiles its virtual members with it.
template <>
void BSSimpleArray<bhkUtilFunctions::AVLevel, 1024>::SetAt(uint32_t aiIndex, const bhkUtilFunctions::AVLevel& aValue);
