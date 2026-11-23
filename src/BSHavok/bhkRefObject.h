#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

#include <xmmintrin.h>

class hkpRigidBody;
class hkpShape;
class NiPoint3;

// The Gamebryo side of a Havok object: a scene graph object that owns the reference to the Havok object it wraps.
class bhkRefObject : public NiObject {
public:
    bhkRefObject();
    ~bhkRefObject() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

    // Replaces the wrapped Havok object, detaching the old one from the wrapper and attaching the new one.
    virtual void SethkObject(hkReferencedObject* aphkObject);

    // Attaches the wrapped Havok object to the wrapper (true) or detaches it (false).
    virtual void Link(bool abAttach);

    // Returns the wrapped body's hkpMotion::MotionType.
    int32_t GetMotionType();
    // The wrapped body's linear velocity in Gamebryo units, stored in apVelocity.
    void* GetLinearVelocityNi(NiPoint3* apVelocity);
    void SetMass(float afMass);

    // The wrapped Havok object.
    hkReferencedObject* GethkObject() const;
    // The wrapped Havok object, or null for a null wrapper.
    hkReferencedObject* operator*() const;

    hkReferencedObject* phkObject;

    static NiRTTI ms_RTTI;
};
