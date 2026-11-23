#pragma once

#include "BSHavok/bhkSimpleShapePhantom.h"

class bhkCharacterController;

// A phantom shaped like a box that follows a character controller; Add registers it with the world.
class bhkCollisionBox : public bhkSimpleShapePhantom {
public:
    bhkCollisionBox(hkSerializableCinfo* apInfo, bhkCharacterController* apCharController);
    ~bhkCollisionBox() override;

    const NiRTTI* GetRTTI() const override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    bool Add(bhkWorld* apWorld) override;
    virtual void Init(const hkSerializableCinfo& arInfo, bhkCharacterController* apCharController);
    using bhkSimpleShapePhantom::Init;

    // Moves the box and its shape's radius to the controller's phantom or position.
    void Update();

    // Frees to the game's heap (MemoryManager).
    static void operator delete(void* apMemory);

    bhkCharacterController* pCharController;
    bool bDisabled;

    static NiRTTI ms_RTTI;
    static inline bool bNoCollision;
};
