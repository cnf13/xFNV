#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"

// Extra data holding an array of integers.
class NiIntegersExtraData : public NiExtraData {
public:
    NiIntegersExtraData();
    NiIntegersExtraData(const uint32_t uiSize, const int32_t* piValue);
    ~NiIntegersExtraData() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

    // Frees the array, then copies auiSize values from apiValue into a new one (empty when either is 0/null).
    void SetArray(uint32_t uiSize, const int32_t* piValue);

    // Copies the NiExtraData members and a fresh copy of the array into apDest (used by CreateClone).
    void CopyMembers(NiIntegersExtraData* pDest, NiCloningProcess& kCloning);

    static NiObject* CreateObject();

    uint32_t m_uiSize;
    int32_t* m_piValue;

    static NiRTTI ms_RTTI;
};
