#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiIntegersExtraData.h"

class NiObjectNET;

// The "BSW" extra data of a model: an array of integers.
class BSWArray : public NiIntegersExtraData {
public:
    BSWArray();
    BSWArray(uint32_t aiSize, int32_t* apArray);
    ~BSWArray() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

    void Init();

protected:
    void CopyMembers(BSWArray* pDest, NiCloningProcess& arCloneData);

public:
    // Creates the "BSW" tag string stored in pTag.
    static void InitSDM();
    static NiObject* CreateObject();
    static BSWArray* Find(NiObjectNET* apObject);
    static void KillSDM();

    static inline NiFixedString* pTag;
    static NiRTTI ms_RTTI;
};
