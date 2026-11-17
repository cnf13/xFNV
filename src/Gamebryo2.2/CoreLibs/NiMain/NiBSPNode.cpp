#include "Gamebryo2.2/CoreLibs/NiMain/NiBSPNode.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiBSPNode::ms_RTTI("NiBSPNode", &NiNode::ms_RTTI);

NiBSPNode::NiBSPNode() : NiNode(0) {
    m_kChildren.SetSize(2);
    m_kChildren.SetGrowBy(0);
}

NiBSPNode::~NiBSPNode() {}

const NiRTTI* NiBSPNode::GetRTTI() const {
    return &ms_RTTI;
}

// Constructs the clone as CreateObject does, copies NiNode's members, then the model plane.
NiObject* NiBSPNode::CreateClone(NiCloningProcess& kCloning) {
    NiBSPNode* pkObject = static_cast<NiBSPNode*>(NiMemObject::operator new(sizeof(NiBSPNode)));
    if (pkObject) {
        ::new (pkObject) NiBSPNode;
    }
    else {
        pkObject = nullptr;
    }
    NiNode::CopyMembers(pkObject, kCloning);
    pkObject->m_kModelPlane = m_kModelPlane;
    return pkObject;
}

void NiBSPNode::LoadBinary(NiStream& stream) {
    NiNode::LoadBinary(stream);
    m_kModelPlane.LoadBinary(stream);
}

void NiBSPNode::LinkObject(NiStream& stream) {
    NiNode::LinkObject(stream);
}

bool NiBSPNode::RegisterStreamables(NiStream& stream) {
    return NiNode::RegisterStreamables(stream);
}

void NiBSPNode::SaveBinary(NiStream& stream) {
    NiNode::SaveBinary(stream);
    m_kModelPlane.SaveBinary(stream);
}

bool NiBSPNode::IsEqual(NiObject* pObject) {
    if (!NiNode::IsEqual(pObject)) {
        return false;
    }
    return !(m_kModelPlane != static_cast<NiBSPNode*>(pObject)->m_kModelPlane);
}

void NiBSPNode::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiNode::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(m_kModelPlane.GetViewerString("m_kModelPlane"));
    pStrings->Add(m_kWorldPlane.GetViewerString("m_kWorldPlane"));
}

void NiBSPNode::ApplyTransform(const NiMatrix3& mat, const NiPoint3& trn, bool bOnLeft) {
    float v21;
    float v22;
    float v23;
    NiMatrix3 a3;
    if (bOnLeft) {
        NiPoint3 v17;
        NiMatrix3 a1;
        NiMatrix3* v6 = &(a3 = mat * m_kLocal.m_Rotate);
        a1 = m_kLocal.m_Rotate.TransposeTimes(*v6);
        float v18 = mat.m_pEntry[0][1] * m_kLocal.m_Translate.y + mat.m_pEntry[0][0] * m_kLocal.m_Translate.x +
            mat.m_pEntry[0][2] * m_kLocal.m_Translate.z;
        float v19 = mat.m_pEntry[1][0] * m_kLocal.m_Translate.x + mat.m_pEntry[1][1] * m_kLocal.m_Translate.y +
            mat.m_pEntry[1][2] * m_kLocal.m_Translate.z;
        float v20 = mat.m_pEntry[2][0] * m_kLocal.m_Translate.x + mat.m_pEntry[2][1] * m_kLocal.m_Translate.y +
            mat.m_pEntry[2][2] * m_kLocal.m_Translate.z;
        v21 = trn.x + v18;
        v22 = trn.y + v19;
        v23 = trn.z + v20;
        v18 = v21 - m_kLocal.m_Translate.x;
        v19 = v22 - m_kLocal.m_Translate.y;
        v20 = v23 - m_kLocal.m_Translate.z;
        v21 = m_kLocal.m_Rotate.m_pEntry[2][0] * v20 +
            (m_kLocal.m_Rotate.m_pEntry[0][0] * v18 + m_kLocal.m_Rotate.m_pEntry[1][0] * v19);
        v22 = m_kLocal.m_Rotate.m_pEntry[1][1] * v19 + m_kLocal.m_Rotate.m_pEntry[0][1] * v18 +
            m_kLocal.m_Rotate.m_pEntry[2][1] * v20;
        v23 = v18 * m_kLocal.m_Rotate.m_pEntry[0][2] + v19 * m_kLocal.m_Rotate.m_pEntry[1][2] +
            v20 * m_kLocal.m_Rotate.m_pEntry[2][2];
        float abOnLefta = 1.0 / m_kLocal.m_fScale;
        v17.x = abOnLefta * v21;
        v17.y = v22 * abOnLefta;
        v17.z = abOnLefta * v23;
        a1.Inverse(a3);
        uint32_t v7 = 0;
        v21 = a3.m_pEntry[1][0] * m_kModelPlane.m_kNormal.y + m_kModelPlane.m_kNormal.x * a3.m_pEntry[0][0] +
            a3.m_pEntry[2][0] * m_kModelPlane.m_kNormal.z;
        v22 = a3.m_pEntry[0][1] * m_kModelPlane.m_kNormal.x + a3.m_pEntry[1][1] * m_kModelPlane.m_kNormal.y +
            a3.m_pEntry[2][1] * m_kModelPlane.m_kNormal.z;
        float v8 = v22;
        double v9 = a3.m_pEntry[0][2] * m_kModelPlane.m_kNormal.x + a3.m_pEntry[1][2] * m_kModelPlane.m_kNormal.y;
        double v10 = a3.m_pEntry[2][2] * m_kModelPlane.m_kNormal.z;
        m_kModelPlane.m_kNormal.x = v21;
        m_kModelPlane.m_kNormal.y = v8;
        v23 = v9 + v10;
        m_kModelPlane.m_kNormal.z = v23;
        float abOnLeftb =
            m_kModelPlane.m_kNormal.y * v17.y + m_kModelPlane.m_kNormal.x * v17.x + m_kModelPlane.m_kNormal.z * v17.z;
        for (m_kModelPlane.m_fConstant = abOnLeftb + m_kModelPlane.m_fConstant; v7 < m_kChildren.m_usSize; ++v7) {
            NiAVObject* m_pObject = m_kChildren.m_pBase[v7].m_pObject;
            if (m_pObject) {
                m_pObject->ApplyTransform(a1, v17, true);
            }
        }
    }
    else {
        mat.Inverse(a3);
        uint32_t v12 = 0;
        v21 = a3.m_pEntry[1][0] * m_kModelPlane.m_kNormal.y + m_kModelPlane.m_kNormal.x * a3.m_pEntry[0][0] +
            a3.m_pEntry[2][0] * m_kModelPlane.m_kNormal.z;
        v22 = m_kModelPlane.m_kNormal.x * a3.m_pEntry[0][1] + a3.m_pEntry[1][1] * m_kModelPlane.m_kNormal.y +
            a3.m_pEntry[2][1] * m_kModelPlane.m_kNormal.z;
        double v14 = m_kModelPlane.m_kNormal.x * a3.m_pEntry[0][2] + a3.m_pEntry[1][2] * m_kModelPlane.m_kNormal.y;
        double v15 = a3.m_pEntry[2][2] * m_kModelPlane.m_kNormal.z;
        m_kModelPlane.m_kNormal.x = v21;
        m_kModelPlane.m_kNormal.y = v22;
        v23 = v14 + v15;
        m_kModelPlane.m_kNormal.z = v23;
        float abOnLeftc =
            m_kModelPlane.m_kNormal.y * trn.y + m_kModelPlane.m_kNormal.x * trn.x + m_kModelPlane.m_kNormal.z * trn.z;
        for (m_kModelPlane.m_fConstant = abOnLeftc + m_kModelPlane.m_fConstant; v12 < m_kChildren.m_usSize; ++v12) {
            NiAVObject* v16 = m_kChildren.m_pBase[v12].m_pObject;
            if (v16) {
                v16->ApplyTransform(mat, trn, true);
            }
        }
    }
}

void NiBSPNode::UpdateWorldData(NiUpdateData& UpdateData) {
    NiAVObject::UpdateWorldData(UpdateData);
    UpdateWorldPlane();
}

// Draws the child on the far side of the plane first. A camera on the plane decides by its view direction.
void NiBSPNode::OnVisible(NiCullingProcess& kCuller) {
    NiCamera* pkCamera = kCuller.m_pkCamera;
    int32_t iSide = m_kWorldPlane.WhichSide(pkCamera->m_kWorld.m_Translate);
    if (!iSide) {
        NiPoint3 kCameraDirection;
        kCameraDirection.x = pkCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
        kCameraDirection.y = pkCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
        kCameraDirection.z = pkCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
        iSide = m_kWorldPlane.WhichSide(kCameraDirection);
    }
    NiAVObject* pkLeft = m_kChildren.m_pBase[0].m_pObject;
    NiAVObject* pkRight = m_kChildren.m_pBase[1].m_pObject;
    if (iSide == 2) {
        if (pkRight) {
            pkRight->Cull(kCuller);
        }
        if (pkLeft) {
            pkLeft->Cull(kCuller);
        }
    }
    else {
        if (pkLeft) {
            pkLeft->Cull(kCuller);
        }
        if (pkRight) {
            pkRight->Cull(kCuller);
        }
    }
}

void NiBSPNode::UpdateWorldPlane() {
    double z;
    float v5 = ((m_kWorld.m_Rotate.m_pEntry[0][1] * m_kModelPlane.m_kNormal.y) +
                (m_kModelPlane.m_kNormal.x * m_kWorld.m_Rotate.m_pEntry[0][0])) +
        (m_kWorld.m_Rotate.m_pEntry[0][2] * m_kModelPlane.m_kNormal.z);
    float v6 = ((m_kWorld.m_Rotate.m_pEntry[1][0] * m_kModelPlane.m_kNormal.x) +
                (m_kWorld.m_Rotate.m_pEntry[1][1] * m_kModelPlane.m_kNormal.y)) +
        (m_kWorld.m_Rotate.m_pEntry[1][2] * m_kModelPlane.m_kNormal.z);
    float v7 = ((m_kWorld.m_Rotate.m_pEntry[2][0] * m_kModelPlane.m_kNormal.x) +
                (m_kWorld.m_Rotate.m_pEntry[2][1] * m_kModelPlane.m_kNormal.y)) +
        (m_kWorld.m_Rotate.m_pEntry[2][2] * m_kModelPlane.m_kNormal.z);
    double v1 = (double)((m_kWorld.m_Translate.y * v6) + (m_kWorld.m_Translate.x * v5));
    z = (double)m_kWorld.m_Translate.z;
    m_kWorldPlane.m_kNormal.x = v5;
    m_kWorldPlane.m_kNormal.y = v6;
    m_kWorldPlane.m_kNormal.z = v7;
    float v3 = (float)(v1 + (z * v7));
    float v4 = v3 + (m_kModelPlane.m_fConstant * m_kWorld.m_fScale);
    m_kWorldPlane.m_fConstant = v4;
}

NiObject* NiBSPNode::CreateObject() {
    NiBSPNode* pkObject = static_cast<NiBSPNode*>(NiMemObject::operator new(sizeof(NiBSPNode)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiBSPNode;
}
