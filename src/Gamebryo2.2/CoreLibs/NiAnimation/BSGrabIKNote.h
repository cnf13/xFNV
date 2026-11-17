#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/BSAnimNote.h"

class NiTextKey;

// A note that starts or stops grab IK on one arm.
class BSGrabIKNote : public BSAnimNote {
public:
    // The arm that grabs.
    enum GRABIK_ARM {
        BS_GIK_LEFT_ARM,
        BS_GIK_RIGHT_ARM,
        BS_GIK_NUM_ARMS,
    };

    // Whether the note starts or stops the grab.
    enum GRABIK_STATE {
        BS_GIK_NONE,
        BS_GIK_START,
        BS_GIK_STOP,
    };

    BSGrabIKNote();
    ~BSGrabIKNote() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    // LinkObject only calls the NiObject implementation.
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    bool ParseTextKey(const NiTextKey* apTextKey) override;

    static NiObject* CreateObject();

    GRABIK_ARM eArm;
    float fGain;
    GRABIK_STATE eState;

    static NiRTTI ms_RTTI;
};
