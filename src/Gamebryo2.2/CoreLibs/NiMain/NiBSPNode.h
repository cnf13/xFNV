#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// a binary space partition node: its two children lie on either side of a plane
class NiBSPNode : public NiNode {
public:
    NiBSPNode();
    ~NiBSPNode() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& stream) override;
    void LinkObject(NiStream& stream) override;
    bool RegisterStreamables(NiStream& stream) override;
    void SaveBinary(NiStream& stream) override;
    bool IsEqual(NiObject* pObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;
    void ApplyTransform(const NiMatrix3& mat, const NiPoint3& trn, bool bOnLeft) override;
    void UpdateWorldData(NiUpdateData& UpdateData) override;
    void OnVisible(NiCullingProcess& kCuller) override;

    // moves the model plane into world space
    void UpdateWorldPlane();

    // The stream loader: a 0xCC-byte node with both planes and room for two children.
    static NiObject* CreateObject();

    NiPlane m_kModelPlane;
    NiPlane m_kWorldPlane;

    static NiRTTI ms_RTTI;
};
