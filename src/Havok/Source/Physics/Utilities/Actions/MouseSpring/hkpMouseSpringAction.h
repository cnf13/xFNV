#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkpEntity;
class hkpRigidBody;
struct hkStepInfo;

void cleanupLoadedObjecthkpMouseSpringAction(void* p);
void finishLoadedObjecthkpMouseSpringAction(void* p, int32_t finishing);
// Havok SDK signature (hkpUtilities.lib, tied by reloc).
const void* getVtablehkpMouseSpringAction();

// Havok's action that drags a body by a point with a spring (the mouse picking of the debugger and the telekinesis
// grab); the callbacks may change the force each step.
class hkpMouseSpringAction : public hkpUnaryAction {
public:
    using ApplyCallback = void (*)(hkpMouseSpringAction*, const hkStepInfo&, const hkVector4&);

    // The SDK's hkpMouseSpringAction(hkpRigidBody* body) on apMemory: the defaults and the action's name.
    hkpMouseSpringAction(hkpRigidBody* body);
    // The SDK's finish constructor: the loaded fields stay, the callback array starts empty.
    hkpMouseSpringAction(hkFinishLoadedObjectFlag aFlag) : hkpUnaryAction(aFlag.m_finishing) {}
    // The SDK's full constructor on apMemory: copies the callbacks and looks up the shape key of rb's shape nearest
    // to the mouse position.
    hkpMouseSpringAction(
        const hkVector4& positionInRbLocal,
        const hkVector4& mousePositionInWorld,
        const float springDamping,
        const float springElasticity,
        const float objectDamping,
        hkpRigidBody* rb,
        const hkArray<ApplyCallback, hkContainerHeapAllocator>* appliedCallbacks
    );
    ~hkpMouseSpringAction() override;

    void applyAction(const hkStepInfo& stepInfo) override;
    void entityRemovedCallback(hkpEntity* entity) override;
    hkpAction* clone(const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms) const override;

    void* GetEntity();
    void setMousePosition(const hkVector4& mousePositionInWorld);
    void setMaxRelativeForce(float newMax) {
        m_maxRelativeForce = newMax;
    }

    static const hkClass& staticClass();

    hkVector4 m_positionInRbLocal;
    hkVector4 m_mousePositionInWorld;
    float m_springDamping;
    float m_springElasticity;
    float m_maxRelativeForce;
    float m_objectDamping;
    uint32_t m_shapeKey;
    hkArray<ApplyCallback> m_applyCallbacks;
};

extern hkClass hkpMouseSpringActionClass;
extern hkTypeInfo hkpMouseSpringActionTypeInfo;
