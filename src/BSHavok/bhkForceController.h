#pragma once

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

class NiAVObject;

// A controller that pushes its target's rigid body with a force for a time.
class bhkForceController : public NiTimeController {
public:
    bhkForceController();
    ~bhkForceController() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void Update(NiUpdateData& UpdateData) override;
    bool TargetIsRequiredType() const override;
    virtual bool CanBeExtracted() {
        return false;
    }

    // Applies Force to the target's rigid body while its blend collision object's gain is below 1.
    void ApplyForce();

    // Removes the force controller from apObject and, through the vtable, from every node below it.
    static void RemoveForce(NiAVObject* apObject);
    // Starts a force controller on apSpot (or apObject when null): arForce, plus a damping-scaled share when the
    // object has a rigid body, for 0.2 seconds.
    static void TestForce(NiAVObject* apObject, hkVector4& arForce, NiAVObject* apSpot);
    // As TestForce on apObject, under the Havok heap context, lasting until afEndTime.
    static void TestForce2(NiAVObject* apObject, hkVector4& arForce, float afEndTime);

    // CreateClone, TestForce and TestForce2 allocate the object from the game's heap (MemoryManager).
    static void operator delete(void* apMemory) {
        MemoryManager::Instance().Deallocate(apMemory);
    }

    hkVector4 Force;
    float fDeltaTime;

    static NiRTTI ms_RTTI;
};
