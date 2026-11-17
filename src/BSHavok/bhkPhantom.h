#pragma once

#include "BSHavok/bhkWorldObject.h"

// The creation info of a phantom; adds nothing to the world object's.
class hkPhantomCinfo : public hkWorldObjectCinfo {
public:
    hkPhantomCinfo();
    ~hkPhantomCinfo();
};

// A world object that only reports what overlaps it instead of being simulated.
class bhkPhantom : public bhkWorldObject {
public:
    bhkPhantom();
    ~bhkPhantom() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    bool Remove() override;

    // Tells the world to re-evaluate the wrapped phantom's overlaps after its filter info changed.
    void UpdateCFilter() override;

    // Clears the phantom's state flags.
    void ResetPhantomFlags();

    // Returns the wrapper that apPhantom's user data points back to, as bhkEntity::Getbhk does.
    static bhkPhantom* Getbhk(const hkpWorldObject* apPhantom);

    uint8_t ucFlags;
    uint8_t padding15[3];

    static NiRTTI ms_RTTI;
    // Live wrappers of this class, counted by the constructors.
    static inline uint32_t iCount;
};
