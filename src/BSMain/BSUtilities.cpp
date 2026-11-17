#include "BSMain/BSUtilities.h"

#include "BSShader/BSShaderProperty.h"
#include "BSShader/NoFaderShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFlipController.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDefaultAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFilename.h"
#include "SDK/MSVC/string.h"

#include <cmath>

// The platform (resolved) file name of a source texture: null when apTexture is null or its RTTI chain has no
// NiSourceTexture.
const char* BSUtilities::GetTextureName(NiTexture* apTexture) {
    NiSourceTexture* pTexture = nullptr;
    if (apTexture) {
        const NiRTTI* pkRTTI = apTexture->GetRTTI();
        bool bIsTexture = false;
        while (pkRTTI) {
            if (pkRTTI == &NiSourceTexture::ms_RTTI) {
                bIsTexture = true;
                break;
            }
            pkRTTI = pkRTTI->m_pkBaseRTTI;
        }
        pTexture = bIsTexture ? static_cast<NiSourceTexture*>(apTexture) : nullptr;
    }
    if (pTexture) {
        return pTexture->m_kPlatformFilename.GetHandle();
    }
    return nullptr;
}

char* BSUtilities::GetPersonalPath() {
    return pPersonalPath;
}

NiMatrix3 BSUtilities::GetRotationZToVectorTranspose(NiPoint3 apVector) {
    NiMatrix3 result;
    NiPoint3 R;
    NiMatrix3 krot;
    NiPoint3 T = apVector;
    T.Unitize();
    float v5 = std::fabs(static_cast<double>(T.x));
    float v4 = std::fabs(static_cast<double>(T.y));
    float v3 = std::fabs(static_cast<double>(T.z));
    if (!(v4 >= static_cast<double>(v5)) || !(v3 >= static_cast<double>(v5))) {
        if (!(v5 >= static_cast<double>(v4)) || !(v3 >= static_cast<double>(v4))) {
            float v6 = -T.y;
            float x = T.x;
            float v13 = 0.0f;
            R.x = v6;
            R.y = T.x;
            R.z = 0.0f;
        }
        else {
            float v7 = -T.z;
            float v15 = 0.0f;
            float v16 = T.x;
            R.x = v7;
            R.y = 0.0f;
            R.z = T.x;
        }
    }
    else {
        float v8 = -T.z;
        float v17 = 0.0f;
        float y = T.y;
        R.x = 0.0f;
        R.y = v8;
        R.z = T.y;
    }
    R.Unitize();
    NiPoint3 S = R.Cross(T);
    S.Unitize();
    R = S.Cross(T);
    R.Unitize();
    krot.m_pEntry[0][0] = R.x;
    krot.m_pEntry[1][0] = R.y;
    krot.m_pEntry[2][0] = R.z;
    krot.m_pEntry[0][1] = S.x;
    krot.m_pEntry[1][1] = S.y;
    krot.m_pEntry[2][1] = S.z;
    krot.m_pEntry[0][2] = T.x;
    krot.m_pEntry[1][2] = T.y;
    krot.m_pEntry[2][2] = T.z;
    result = krot;
    return result;
}

NiMatrix3 BSUtilities::GetRotationZToVector(NiPoint3 apVector) {
    NiMatrix3 result;
    NiPoint3 R;
    NiMatrix3 krot;
    NiPoint3 T = apVector;
    T.Unitize();
    float v5 = std::fabs(static_cast<double>(T.x));
    float v4 = std::fabs(static_cast<double>(T.y));
    float v3 = std::fabs(static_cast<double>(T.z));
    if (!(v4 >= static_cast<double>(v5)) || !(v3 >= static_cast<double>(v5))) {
        if (!(v5 >= static_cast<double>(v4)) || !(v3 >= static_cast<double>(v4))) {
            float v6 = -T.y;
            R.x = v6;
            R.y = T.x;
            R.z = 0.0f;
        }
        else {
            float v7 = -T.z;
            R.x = v7;
            R.y = 0.0f;
            R.z = T.x;
        }
    }
    else {
        float v8 = -T.z;
        R.x = 0.0f;
        R.y = v8;
        R.z = T.y;
    }
    R.Unitize();
    NiPoint3 S = R.Cross(T);
    S.Unitize();
    R = S.Cross(T);
    R.Unitize();
    krot.m_pEntry[0][0] = R.x;
    krot.m_pEntry[0][1] = R.y;
    krot.m_pEntry[0][2] = R.z;
    krot.m_pEntry[1][0] = S.x;
    krot.m_pEntry[1][1] = S.y;
    krot.m_pEntry[1][2] = S.z;
    krot.m_pEntry[2][0] = T.x;
    krot.m_pEntry[2][1] = T.y;
    krot.m_pEntry[2][2] = T.z;
    result = krot;
    return result;
}

NiAVObject* BSUtilities::GetObjectByName(NiAVObject* apScene, const char* apName, bool bTestScenegraph) {
    NiAVObject* pres;
    FNVD_INLINED_LOCAL(NiFixedString, name, std::nullopt, std::nullopt);
    if (!apScene || !apName) {
        return nullptr;
    }
    NiControllerManager* pcontrollerManager =
        static_cast<NiControllerManager*>(apScene->GetController(&NiControllerManager::ms_RTTI));
    // MAYBE-INLINED 0xC4B374: a write opening NiFixedString's uses: may be its inlined constructor
    name.m_kHandle = NiGlobalStringTable::AddString(apName);
    if (pcontrollerManager && (pres = pcontrollerManager->m_spObjectPalette.m_pObject->GetAVObject(name)) != nullptr) {
        // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
        NiGlobalStringTable::ReleaseHandle(name.GetHandle());
        return pres;
    }
    else {
        if (bTestScenegraph) {
            NiAVObject* v4 = apScene->GetObjectByName(name);
            // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
            NiGlobalStringTable::ReleaseHandle(name.GetHandle());
            return v4;
        }
        else {
            // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
            NiGlobalStringTable::ReleaseHandle(name.GetHandle());
            return nullptr;
        }
    }
}

NiAVObject* BSUtilities::GetObjectByName(NiAVObject* apScene, const char* apName) {
    return BSUtilities::GetObjectByName(apScene, apName, true);
}

NiAVObject* BSUtilities::GetObjectByName(NiAVObject* apScene, const NiFixedString& apName, bool bTestScenegraph) {
    if (!apScene || !(&apName)->GetHandle()) {
        return nullptr;
    }
    NiControllerManager* Controller =
        static_cast<NiControllerManager*>(apScene->GetController(&NiControllerManager::ms_RTTI));
    if (Controller) {
        NiAVObject* v4 = Controller->m_spObjectPalette.m_pObject->GetAVObject(apName);
        if (v4) {
            return v4;
        }
    }
    if (bTestScenegraph) {
        return apScene->GetObjectByName(apName);
    }
    return nullptr;
}

NiAVObject* BSUtilities::GetObjectByName(NiAVObject* apScene, const NiFixedString& apName) {
    return BSUtilities::GetObjectByName(apScene, apName, true);
}

bool BSUtilities::GetExternalEmittanceRecurse(NiAVObject* apObj) {
    int32_t type;
    NiAVObject* v2;
    uint32_t a1;
    uint32_t arrayCount;
    NiNode* v6;
    NoFaderShaderProperty* property;
    uint8_t result = 0;
    if (apObj) {
        type = NiShadeProperty::GetType();
        property = static_cast<NoFaderShaderProperty*>(apObj->GetProperty(type));
        if (property && property->BSShaderProperty::GetFlag(BSShaderProperty::BSSP_FLAG_EXTERNAL_EMITTANCE)) {
            return true;
        }
        else {
            v6 = apObj->IsNode();
            if (v6) {
                arrayCount = v6->GetArrayCount();
                for (a1 = 0; a1 < arrayCount; ++a1) {
                    v2 = v6->GetAt(a1);
                    if (BSUtilities::GetExternalEmittanceRecurse(v2)) {
                        return true;
                    }
                }
            }
        }
    }
    return (bool)result;
}

char* BSUtilities::GetLocalAppDataPath() {
    return pLocalAppDataPath;
}

// Sets the always-draw flag and FIXED_BOUND on apObject, then on every child of a node. The parameter is a NiAVObject
// because the children of m_kChildren, which are NiAVObjects, are passed to it, and the RTTI walk is the check that
// lets it treat one as a node.
void BSUtilities::SetAlwaysDraw(NiAVObject* apObject) {
    if (apObject) {
        apObject->SetAlwaysDraw(true);
        apObject->m_uFlags = static_cast<NiAVObject::NiFlags>(apObject->m_uFlags | NiAVObject::FIXED_BOUND);
    }
    NiNode* pNode = nullptr;
    if (apObject) {
        const NiRTTI* pkRTTI = apObject->GetRTTI();
        bool bIsNode = false;
        while (pkRTTI) {
            if (pkRTTI == &NiNode::ms_RTTI) {
                bIsNode = true;
                break;
            }
            pkRTTI = pkRTTI->m_pkBaseRTTI;
        }
        pNode = bIsNode ? static_cast<NiNode*>(apObject) : nullptr;
    }
    if (pNode) {
        for (uint32_t i = 0; i < pNode->m_kChildren.m_usSize; ++i) {
            BSUtilities::SetAlwaysDraw(pNode->m_kChildren.m_pBase[i].m_pObject);
        }
    }
}

bool BSUtilities::CompareFloat(float a, float b, float afErrorMargin) {
    float v4 = a - b;
    return (bool)((uint8_t)(((double)afErrorMargin) >= std::fabs(v4)));
}

bool BSUtilities::ComparePoints(NiPoint3* a, NiPoint3* b, float afMargin) {
    return (bool)((uint8_t)(BSUtilities::CompareFloat(a->x, b->x, afMargin) &&
                            (BSUtilities::CompareFloat(a->y, b->y, afMargin) &&
                             BSUtilities::CompareFloat(a->z, b->z, afMargin))));
}

void BSUtilities::ApplyTransformToVerticesAndNormals(
    const NiTransform& aTrans,
    uint32_t aiVertCount,
    NiPoint3* apInVerts,
    NiPoint3* apOutVerts,
    NiPoint3* apInNorms,
    NiPoint3* apOutNorms
) {
    float v6;
    float v7;
    float v8;
    uint32_t i;
    for (i = 0; i < aiVertCount; ++i) {
        if (apOutVerts) {
            v8 = (((((&aTrans)->m_Rotate.m_pEntry[0][0] * apInVerts[i].x) +
                    ((&aTrans)->m_Rotate.m_pEntry[0][1] * apInVerts[i].y)) +
                   ((&aTrans)->m_Rotate.m_pEntry[0][2] * apInVerts[i].z)) *
                  (&aTrans)->m_fScale) +
                (&aTrans)->m_Translate.x;
            apOutVerts[i].x = v8;
            v7 = (((((&aTrans)->m_Rotate.m_pEntry[1][0] * apInVerts[i].x) +
                    ((&aTrans)->m_Rotate.m_pEntry[1][1] * apInVerts[i].y)) +
                   ((&aTrans)->m_Rotate.m_pEntry[1][2] * apInVerts[i].z)) *
                  (&aTrans)->m_fScale) +
                (&aTrans)->m_Translate.y;
            apOutVerts[i].y = v7;
            v6 = (((((&aTrans)->m_Rotate.m_pEntry[2][0] * apInVerts[i].x) +
                    ((&aTrans)->m_Rotate.m_pEntry[2][1] * apInVerts[i].y)) +
                   ((&aTrans)->m_Rotate.m_pEntry[2][2] * apInVerts[i].z)) *
                  (&aTrans)->m_fScale) +
                (&aTrans)->m_Translate.z;
            apOutVerts[i].z = v6;
        }
        if (apOutNorms) {
            apOutNorms[i].x =
                ((((&aTrans)->m_Rotate.m_pEntry[0][0] * apInNorms[i].x) +
                  ((&aTrans)->m_Rotate.m_pEntry[0][1] * apInNorms[i].y)) +
                 ((&aTrans)->m_Rotate.m_pEntry[0][2] * apInNorms[i].z));
            apOutNorms[i].y =
                ((((&aTrans)->m_Rotate.m_pEntry[1][0] * apInNorms[i].x) +
                  ((&aTrans)->m_Rotate.m_pEntry[1][1] * apInNorms[i].y)) +
                 ((&aTrans)->m_Rotate.m_pEntry[1][2] * apInNorms[i].z));
            apOutNorms[i].z =
                ((((&aTrans)->m_Rotate.m_pEntry[2][0] * apInNorms[i].x) +
                  ((&aTrans)->m_Rotate.m_pEntry[2][1] * apInNorms[i].y)) +
                 ((&aTrans)->m_Rotate.m_pEntry[2][2] * apInNorms[i].z));
        }
    }
}

const char* BSUtilities::GetSequenceFileName(NiControllerSequence* apSequence) {
    if (apSequence->m_kName.GetHandle()) {
        std::strstr(apSequence->m_kName.GetHandle(), rdata_10C21E0);
    }
    return nullptr;
}

void BSUtilities::ProcessSelectiveUpdate(NiAVObject* apObject) {
    if (apObject) {
        apObject->m_uFlags |= NiAVObject::SELECTIVE_UPDATE;
        apObject->m_uFlags |= NiAVObject::SELECTIVE_UPDATE_TRANSFORMS;
        apObject->m_uFlags &= ~NiAVObject::SELECTIVE_UPDATE_RIGID;
        NiNode* pnode = apObject->IsNode();
        if (pnode) {
            for (uint32_t I = 0; I < pnode->m_kChildren.m_usSize; ++I) {
                BSUtilities::ProcessSelectiveUpdate(pnode->m_kChildren.m_pBase[I].m_pObject);
            }
        }
    }
}

void BSUtilities::SetSequenceFileName(NiControllerSequence* apSequence, const char* apName) {
    FNVD_INLINED_LOCAL(NiFixedString, v2, std::nullopt, std::nullopt);
    char pcDest[260];
    const char* pcSrc = apSequence->m_kName.GetHandle();
    NiFilename apMemory(apName);
    if (pcSrc) {
        strcpy_s(pcDest, 256, pcSrc);
    }
    else {
        pcDest[0] = 0;
    }
    strcat_s(pcDest, 256, rdata_10C21E0);
    strcat_s(pcDest, 256, apMemory.m_acFname);
    v2.m_kHandle = NiGlobalStringTable::AddString(pcDest);
    apSequence->m_kName.Assign(v2);
    // Destructor of v2.
    // EXE-INLINED none: NiGlobalStringTable::ReleaseHandle
    NiGlobalStringTable::ReleaseHandle(v2.GetHandle());
}

void BSUtilities::FindAnimatedTextures(NiAVObject* apObject, NiTPrimitiveArray<NiTexture*>& arTextureArray) {
    NiTexture* appkElement;
    if (apObject) {
        char v2;
        NiFlipController* pflip = static_cast<NiFlipController*>(apObject->GetController(&NiFlipController::ms_RTTI));
        if (!pflip && (apObject->IsTriBasedGeom() || apObject->IsParticlesGeom())) {
            NiTexturingProperty* ptexProp;
            NiTexturingProperty* Property = static_cast<NiTexturingProperty*>(apObject->GetProperty(5));
            if (Property) {
                char v5;
                for (const NiRTTI* i = Property->GetRTTI(); i; i = i->GetBaseRTTI()) {
                    if (i == &NiTexturingProperty::ms_RTTI) {
                        v5 = 1;
                        goto LABEL_13;
                    }
                }
                v5 = 0;
            LABEL_13:;
                ptexProp = v5 != 0 ? Property : nullptr;
            }
            else {
                ptexProp = nullptr;
            }
            if (ptexProp) {
                pflip = static_cast<NiFlipController*>(ptexProp->GetController(&NiFlipController::ms_RTTI));
            }
        }
        if (pflip) {
            uint32_t m_usSize = pflip->m_kTextures.m_usSize;
            for (uint32_t uiIndex = 0; uiIndex < m_usSize; ++uiIndex) {
                appkElement = pflip->GetTextureAt(uiIndex);
                if (appkElement) {
                    arTextureArray.Add(appkElement);
                }
            }
        }
        for (const NiRTTI* j = apObject->GetRTTI(); j; j = j->GetBaseRTTI()) {
            if (j == &NiNode::ms_RTTI) {
                v2 = 1;
                goto LABEL_28;
            }
        }
        v2 = 0;
    LABEL_28:;
        NiNode* pnode = v2 != 0 ? static_cast<NiNode*>(apObject) : nullptr;
        if (pnode) {
            for (uint32_t k = 0; k < pnode->m_kChildren.m_usSize; ++k) {
                BSUtilities::FindAnimatedTextures(pnode->m_kChildren.m_pBase[k].m_pObject, arTextureArray);
            }
        }
    }
}

void BSUtilities::ReflectCameraAboutArbitraryPlane(NiCamera* apCamera, NiPlane* apPlane, NiCamera* apReflectedCamera) {
    NiPoint3 v19;
    NiPoint3 col;
    NiPoint3 a3;
    NiPoint3 v29;
    NiPoint3 reflforward;
    NiPoint3 reflright;
    NiPoint3 reflup;
    NiPoint3 projVec;
    NiPoint3 v41;
    NiPoint3 campos = apCamera->m_kWorld.m_Translate;
    apCamera->m_kWorld.m_Rotate.GetCol(0, a3);
    NiPoint3 forward = a3;
    apCamera->m_kWorld.m_Rotate.GetCol(2u, col);
    NiPoint3 right = col;
    apCamera->m_kWorld.m_Rotate.GetCol(1u, v19);
    NiPoint3 up = v19;
    float v18 = apPlane->m_kNormal.Dot(campos) - apPlane->m_fConstant;
    float v15 = -apPlane->m_kNormal.x;
    float v16 = -apPlane->m_kNormal.y;
    float v17 = -apPlane->m_kNormal.z;
    v29.x = v15;
    v29.y = v16;
    v29.z = v17;
    float fScalar = v18 * 2.0;
    NiPoint3 reflect_campos = campos + v29 * fScalar;
    float v12 = -forward.x;
    float v13 = -forward.y;
    float v14 = -forward.z;
    projVec.x = v12;
    projVec.y = v13;
    projVec.z = v14;
    projVec.Unitize();
    float v10 = apPlane->m_kNormal.Dot(projVec) * 2.0;
    ::new (&reflforward) NiPoint3(v10 * apPlane->m_kNormal - projVec);
    v41.x = -right.x;
    v41.y = -right.y;
    v41.z = -right.z;
    float x = v41.x;
    float y = v41.y;
    float z = v41.z;
    projVec = v41;
    projVec.Unitize();
    float v9 = apPlane->m_kNormal.Dot(projVec) * 2.0;
    ::new (&reflright) NiPoint3(v9 * apPlane->m_kNormal - projVec);
    projVec = up;
    projVec.Unitize();
    float v8 = apPlane->m_kNormal.Dot(projVec) * 2.0;
    ::new (&reflup) NiPoint3(v8 * apPlane->m_kNormal - projVec);
    NiMatrix3 reflect_camrot(reflforward, reflup, reflright);
    apReflectedCamera->m_kLocal.m_Translate = reflect_campos;
    apReflectedCamera->m_kLocal.m_Rotate = reflect_camrot;
    float v7 = std::fabs(static_cast<double>(apCamera->m_kWorld.m_fScale));
    apReflectedCamera->m_kLocal.m_fScale = v7;
    apReflectedCamera->SetViewFrustum(apCamera->m_kViewFrustum);
    NiUpdateData UpdateData1(0.0f, false, false);
    apReflectedCamera->Update(UpdateData1);
}

void BSUtilities::GetInverseWorldMatrix(const NiTransform& aWorld, const NiSkinInstance* apSkin, _D3DMATRIX& aOut) {
    if (apSkin) {
        D3DXMATRIX kSkinMatrix;
        kSkinMatrix._43 = 0.0f;
        kSkinMatrix._42 = 0.0f;
        kSkinMatrix._34 = 0.0f;
        kSkinMatrix._32 = 0.0f;
        kSkinMatrix._31 = 0.0f;
        kSkinMatrix._24 = 0.0f;
        kSkinMatrix._23 = 0.0f;
        kSkinMatrix._21 = 0.0f;
        kSkinMatrix._14 = 0.0f;
        kSkinMatrix._13 = 0.0f;
        kSkinMatrix._12 = 0.0f;
        kSkinMatrix._44 = 1.0f;
        kSkinMatrix._33 = 1.0f;
        kSkinMatrix._22 = 1.0f;
        kSkinMatrix._11 = 1.0f;
        kSkinMatrix._41 = NiRenderer::PosAdjust.x;
        kSkinMatrix._42 = NiRenderer::PosAdjust.y;
        kSkinMatrix._43 = NiRenderer::PosAdjust.z;
        D3DXMatrixInverse_Thunk(&aOut, nullptr, &kSkinMatrix);
    }
    else {
        NiTransform kInvertedTransform;
        aWorld.Invert(kInvertedTransform);
        aOut._11 = kInvertedTransform.m_Rotate.m_pEntry[0][0] * kInvertedTransform.m_fScale;
        aOut._12 = kInvertedTransform.m_Rotate.m_pEntry[1][0] * kInvertedTransform.m_fScale;
        aOut._13 = kInvertedTransform.m_Rotate.m_pEntry[2][0] * kInvertedTransform.m_fScale;
        aOut._14 = 0.0f;
        aOut._21 = kInvertedTransform.m_Rotate.m_pEntry[0][1] * kInvertedTransform.m_fScale;
        aOut._22 = kInvertedTransform.m_Rotate.m_pEntry[1][1] * kInvertedTransform.m_fScale;
        aOut._23 = kInvertedTransform.m_Rotate.m_pEntry[2][1] * kInvertedTransform.m_fScale;
        aOut._24 = 0.0f;
        aOut._31 = kInvertedTransform.m_Rotate.m_pEntry[0][2] * kInvertedTransform.m_fScale;
        aOut._32 = kInvertedTransform.m_Rotate.m_pEntry[1][2] * kInvertedTransform.m_fScale;
        aOut._33 = kInvertedTransform.m_Rotate.m_pEntry[2][2] * kInvertedTransform.m_fScale;
        aOut._34 = 0.0f;
        aOut._41 = kInvertedTransform.m_Translate.x;
        aOut._42 = kInvertedTransform.m_Translate.y;
        aOut._43 = kInvertedTransform.m_Translate.z;
        aOut._44 = 1.0f;
    }
}
