#include "Fallout Shared/TempEffects/BSTempEffectSimpleDecal.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/ScrapHeap.h"
#include "BSMain/BSOcclusionQuery.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderNoLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/Lighting30ShaderProperty.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDecalNode.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/BSTempNode.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Date.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/ProcessLists.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Reconstruction.hpp"

NiPointer<QueuedFile> BSTempEffectSimpleDecal::spPreloadedDecalTextureSets;
NiRTTI BSTempEffectSimpleDecal::ms_RTTI("BSTempEffectSimpleDecal", &BSTempEffect::ms_RTTI);

double BSMAX_0(double a1, float a2) {
    if (!(a2 < a1)) {
        return a2;
    }
    else {
        return a1;
    }
}

DecalVertex::DecalVertex() {}

BSTempEffectSimpleDecal::BSTempEffectSimpleDecal(
    BGSDecalNode* apParent,
    float afLifetime,
    DECAL_CREATION_DATA& arDecalData
)
  : BSTempEffect(arDecalData.pParentCell, afLifetime), bFinished(false), bFinalize(false), bOcclusionQuery(false),
    pOcclusionQuery(nullptr), spQueryTexture(nullptr), sp3D(nullptr), bPermanent(arDecalData.bPermanent),
    bValidDecal(false), bForceFade(arDecalData.bForceFade), pTextureSet(arDecalData.pTexSet),
    pOrigin(arDecalData.pOrigin), pVector(arDecalData.pDirection), iDecalVertexCount(0), iDecalTriangleCount(0),
    pParentNode(apParent), spTargetNode(arDecalData.pAVObj), fRotation(arDecalData.fRotation),
    fWidth(arDecalData.fDecalWidth), fHeight(arDecalData.fDecalHeight), fDepth(arDecalData.fDecalDepth),
    cSubTexture(arDecalData.cSubTex), bUseParallaxOcclusion(arDecalData.bUseParallaxOcclusion),
    fParallaxOcclusionScale(arDecalData.fParallaxOcclusionScale),
    ucParallaxOcclusionMaxPasses(arDecalData.ucParallaxOcclusionMaxPasses), bUseAlphaTesting(arDecalData.bAlphaTesting),
    bUseAlphaBlending(arDecalData.bAlphaBlending), fSpecular(arDecalData.fSpecular),
    fVertexColor(arDecalData.fVertexColor), bModelSpace(arDecalData.bModelSpace), bTwoSided(arDecalData.bTwoSided) {
    if (BSTempEffectSimpleDecal::iCurrentMaxDecalCount == -1) {
        BSTempEffectSimpleDecal::iCurrentMaxDecalCount = gs_uMaxDecals_Decals.Int();
    }
    float fEpsilonRadians = arDecalData.fDecalEpsilon * 0.01745329238474369;
    BSTempEffectSimpleDecal::fDecalEpsilon = std::cos(fEpsilonRadians);
    BSTempEffectSimpleDecal::fDecalEpsilon = BSMAX_0(0.01, BSTempEffectSimpleDecal::fDecalEpsilon);
}

BSTempEffectSimpleDecal::~BSTempEffectSimpleDecal() {
    spTargetNode = static_cast<NiAVObject*>(nullptr);
    pParentNode = nullptr;
    if (!bPermanent) {
        if (sp3D.m_pObject) {
            ShadowSceneNode::RemoveLightsFromGeom(static_cast<NiTriBasedGeom*>(sp3D.m_pObject));
            if (sp3D.m_pObject->m_pkParent) {
                NiNode* pParent = sp3D.m_pObject->m_pkParent;
                pParent->DetachChild(sp3D.m_pObject);
            }
            sp3D = static_cast<NiAVObject*>(nullptr);
        }
    }
    CleanUpMemory();
    if (bValidDecal) {
        --BGSDecalManager::iDecalCount;
    }
}

void BSTempEffectSimpleDecal::Initialize() {
    if (ProcessGeometry((&spTargetNode)->operator->())) {
        BSTempEffect::Initialize();
    }
}

bool BSTempEffectSimpleDecal::Update(float afTimeDelta) {
    bool bkeep = BSTempEffect::Update(afTimeDelta);
    if (bkeep || !sp3D.m_pObject) {
        return bkeep;
    }
    double PackageStartTime = fAge;
    float ftimer = PackageStartTime - fLifetime;
    if (!(ftimer >= 0.0) || !(ftimer <= 1.0)) {
        BGSDecalNode* pnode = static_cast<BGSDecalNode*>(sp3D.m_pObject->m_pkParent);
        if (pnode) {
            pnode->DetachChild(this);
        }
        return bkeep;
    }
    float falpha = (1.0 - ftimer) / 1.0;
    int32_t Type = NiMaterialProperty::GetType();
    NiMaterialProperty* pMaterial = static_cast<NiMaterialProperty*>(sp3D.m_pObject->GetProperty(Type));
    if (pMaterial) {
        if (falpha != pMaterial->m_fAlpha) {
            pMaterial->SetAlpha(falpha);
        }
    }
    return 1;
}

NiAVObject* BSTempEffectSimpleDecal::Get3D() {
    return (&sp3D)->operator->();
}

NiAVObject* BSTempEffectSimpleDecal::GetDecalParent() {
    return (&spTargetNode)->operator->();
}

bool BSTempEffectSimpleDecal::GetIsValid() {
    return (bool)((uint8_t)bValidDecal);
}

NiPoint3* BSTempEffectSimpleDecal::GetOrigin(NiPoint3* p2) {
    *p2 = pOrigin;
    return p2;
}

NiPoint3* BSTempEffectSimpleDecal::GetDirection(NiPoint3* a2) {
    *a2 = pVector;
    return a2;
}

bool BSTempEffectSimpleDecal::ProcessGeometry(NiAVObject* apAVObj) {
    NiPoint2 v34;
    NiPoint2 v35;
    NiPoint3 v36;
    FNVD_INLINED_LOCAL(NiPlane, v37, std::nullopt, std::nullopt);
    NiPoint3 v38;
    FNVD_INLINED_LOCAL(NiPlane, v39, std::nullopt, std::nullopt);
    NiPoint3 v40;
    FNVD_INLINED_LOCAL(NiPlane, v41, std::nullopt, std::nullopt);
    NiPoint3 v42;
    FNVD_INLINED_LOCAL(NiPlane, v43, std::nullopt, std::nullopt);
    NiPoint3 v44;
    FNVD_INLINED_LOCAL(NiPlane, v45, std::nullopt, std::nullopt);
    NiPoint3 dst;
    FNVD_INLINED_LOCAL(NiPlane, v47, std::nullopt, std::nullopt);
    NiMatrix3 rot_2;
    NiMatrix3 kinvobjrot;
    NiPoint3 kmodelspaceorigin;
    NiMatrix3 kobjrot;
    NiMatrix3 rot;
    NiPoint3 kpos;
    if (apAVObj && NiObject::IsKindOf_0(&NiTriStrips::ms_RTTI, apAVObj)) {
        NiMatrix3 kzrot;
        NiPoint3 tangent;
        NiPoint3 binormal;
        NiPoint3 decalcenter = pOrigin;
        NiPoint3 decalnormal = pVector;
        if (bModelSpace) {
            kobjrot = (*&apAVObj->m_kLocal).m_Rotate;
        }
        else {
            kobjrot = apAVObj->GetWorldTransform().m_Rotate;
        }
        kobjrot.Inverse(&kinvobjrot);
        NiPoint3 kmodelspaceray = kinvobjrot * decalnormal;
        if (bModelSpace) {
            kpos = decalcenter;
        }
        else {
            kpos = decalcenter - apAVObj->GetWorldTranslate();
            float v32 = 1.0 / apAVObj->m_kWorld.m_fScale;
            kpos = kpos * v32;
        }
        NiMatrix3::TransformVertices(kinvobjrot, NiPoint3_ZERO, 1u, &kpos, &kmodelspaceorigin);
        fWidth = 1.0 / apAVObj->m_kWorld.m_fScale * fWidth;
        fHeight = 1.0 / apAVObj->m_kWorld.m_fScale * this->GetDecalHeight();
        fDepth = 1.0 / apAVObj->m_kWorld.m_fScale * fDepth;
        pDecalCenter = kmodelspaceorigin;
        pDecalNormal = kmodelspaceray;
        ::new (&rot) NiMatrix3(BSUtilities::GetRotationZToVector(kmodelspaceray));
        kzrot.MakeZRotation(fRotation);
        rot = *::new (&rot_2) NiMatrix3(kzrot * rot);
        rot.GetRow(0, tangent);
        rot.GetRow(1u, binormal);
        float d = pDecalCenter.Dot(tangent);
        float v31 = -(fWidth * 0.5 - d);
        NiPoint3* v2 = ::new (&dst) NiPoint3(tangent.x, tangent.y, tangent.z);
        pClippingPlanes[0] = *::new (&v47) NiPlane(*v2, v31);
        float v30 = -(fWidth * 0.5 + d);
        NiPoint3* v3 = ::new (&v44) NiPoint3(-tangent.x, -tangent.y, -tangent.z);
        pClippingPlanes[1] = *::new (&v45) NiPlane(*v3, v30);
        d = pDecalCenter.Dot(binormal);
        float v29 = -(this->GetDecalHeight() * 0.5 - d);
        NiPoint3* v4 = ::new (&v42) NiPoint3(binormal.x, binormal.y, binormal.z);
        pClippingPlanes[2] = *::new (&v43) NiPlane(*v4, v29);
        float v28 = -(this->GetDecalHeight() * 0.5 + d);
        NiPoint3* v5 = ::new (&v40) NiPoint3(-binormal.x, -binormal.y, -binormal.z);
        pClippingPlanes[3] = *::new (&v41) NiPlane(*v5, v28);
        d = pDecalCenter.Dot(pDecalNormal);
        float v27 = -(fDepth + d);
        NiPoint3* v6 = ::new (&v38) NiPoint3(-pDecalNormal.x, -pDecalNormal.y, -pDecalNormal.z);
        pClippingPlanes[4] = *::new (&v39) NiPlane(*v6, v27);
        float v26 = -(fDepth - d);
        NiPoint3* v7 = ::new (&v36) NiPoint3(pDecalNormal.x, pDecalNormal.y, pDecalNormal.z);
        pClippingPlanes[5] = *::new (&v37) NiPlane(*v7, v26);
        ClipMesh(apAVObj);
        if (iDecalTriangleCount) {
            float one_over_w = 1.0 / fWidth;
            float one_over_h = 1.0 / this->GetDecalHeight();
            for (int32_t a = 0; a < iDecalVertexCount; ++a) {
                NiPoint2* v10;
                NiPoint3 v = pTempVertexArray[a] - pDecalCenter;
                double v8 = v.Dot(tangent);
                float v55 = v8 * one_over_w + 0.5;
                double v9 = v.Dot(binormal);
                float v52 = v9 * one_over_h + 0.5;
                int32_t subtex = cSubTexture;
                float s = v55 / 2.0;
                float t = v52 / 2.0;
                switch (subtex) {
                    case 0: {
                        break;
                    }
                    case 1: {
                        s = s + 0.5;
                        break;
                    }
                    case 2: {
                        t = t + 0.5;
                        break;
                    }
                    case 3: {
                        s = s + 0.5;
                        t = t + 0.5;
                        break;
                    }
                    default: {
                        s = s + s;
                        t = t + t;
                        break;
                    }
                }
                if (bPermanent) {
                    float v25 = 1.0 - s;
                    v10 = ::new (&v35) NiPoint2(v25, t);
                }
                else {
                    v10 = ::new (&v34) NiPoint2(s, t);
                }
                float v11 = v10->x;
                float v12 = v10->y;
                NiPoint2* pTempTexcoordArray = this->pTempTexcoordArray;
                int32_t v14 = a;
                pTempTexcoordArray[a].x = v11;
                pTempTexcoordArray[v14].y = v12;
            }
            ResizeDataArrays();
            NiTriShapeData* data = CreateTriData();
            sp3D.operator=(static_cast<NiAVObject*>(NiTriShape::Create(data)));
            if (sp3D.m_pObject) {
                pFinalVertexArray = 0;
                pFinalNormalArray = 0;
                pFinalTexcoordArray = 0;
                pFinalColorArray = 0;
                pFinalIndexArray = 0;
            }
            bFinalize = 1;
            return 1;
        }
        else {
            bFinished = 1;
            if (BGSDecalManager::bDebugDecals.Bool()) {
                Warning("DECAL: No Decal for : %s", (&spTargetNode.m_pObject->m_kName)->GetHandleString());
            }
            return 0;
        }
    }
    else {
        bFinished = 1;
        return 0;
    }
}

// Clips against planes 0, 1, 2, 3, 5 and 4, in that order, between the caller's and the scratch arrays.
int32_t BSTempEffectSimpleDecal::ClipPolygon(
    int32_t aiVertexCount,
    const int32_t* apInIndex,
    const NiPoint3* apInVertex,
    const NiPoint3* apInNormal,
    int32_t* apOutIndex,
    NiPoint3* apOutVertex,
    NiPoint3* apOutNormal
) {
    NiPoint3 kTempVertex[9];
    rt::eh_vector_constructor_iterator(kTempVertex, sizeof(NiPoint3), 9, &emptyPointConstructor);
    NiPoint3 kTempNormal[9];
    rt::eh_vector_constructor_iterator(kTempNormal, sizeof(NiPoint3), 9, &emptyPointConstructor);
    int32_t aiTempIndex[9];
    int32_t iCount = ClipPolygonAgainstPlane(
        &pClippingPlanes[0], aiVertexCount, apInIndex, apInVertex, apInNormal, aiTempIndex, kTempVertex, kTempNormal
    );
    if (!iCount) {
        return iCount;
    }
    iCount = ClipPolygonAgainstPlane(
        &pClippingPlanes[1], iCount, aiTempIndex, kTempVertex, kTempNormal, apOutIndex, apOutVertex, apOutNormal
    );
    if (!iCount) {
        return iCount;
    }
    iCount = ClipPolygonAgainstPlane(
        &pClippingPlanes[2], iCount, apOutIndex, apOutVertex, apOutNormal, aiTempIndex, kTempVertex, kTempNormal
    );
    if (!iCount) {
        return iCount;
    }
    iCount = ClipPolygonAgainstPlane(
        &pClippingPlanes[3], iCount, aiTempIndex, kTempVertex, kTempNormal, apOutIndex, apOutVertex, apOutNormal
    );
    if (!iCount) {
        return iCount;
    }
    iCount = ClipPolygonAgainstPlane(
        &pClippingPlanes[5], iCount, apOutIndex, apOutVertex, apOutNormal, aiTempIndex, kTempVertex, kTempNormal
    );
    if (!iCount) {
        return iCount;
    }
    return ClipPolygonAgainstPlane(
        &pClippingPlanes[4], iCount, aiTempIndex, kTempVertex, kTempNormal, apOutIndex, apOutVertex, apOutNormal
    );
}

bool BSTempEffectSimpleDecal::CheckOcclusionQueryResults() {
    if (!pOcclusionQuery->GetResult(&iCurrentOcclusionResult, 0)) {
        return false;
    }
    BSOcclusionQuery* pQuery = pOcclusionQuery;
    if (pQuery) {
        delete pQuery;
    }
    pOcclusionQuery = nullptr;
    return true;
}

// An input index of -1 is a vertex the clipping made: it is matched against pNewIndexMap by position, else added there.
// Other indices go through pOldToNewIndexArray. A new vertex's alpha fades with the angle between its normal and the
// decal's (0 at or below fDecalEpsilon).
bool BSTempEffectSimpleDecal::AddPolygon(
    int32_t aiVertexCount,
    int32_t* apIndex,
    NiPoint3* apVertex,
    const NiPoint3* apNormal
) {
    int32_t iCount = iDecalVertexCount;
    if (aiVertexCount + iCount >= 512) {
        return false;
    }
    if (!pTempVertexArray) {
        ScrapHeap* pHeap = MemoryManager::Instance().GetThreadScrapHeap();
        pTempVertexArray = static_cast<NiPoint3*>(pHeap->Allocate(512 * sizeof(NiPoint3), ScrapHeap::ALIGNMENT));
        pTempNormalArray = static_cast<NiPoint3*>(pHeap->Allocate(512 * sizeof(NiPoint3), ScrapHeap::ALIGNMENT));
        pTempTexcoordArray = static_cast<NiPoint2*>(pHeap->Allocate(512 * sizeof(NiPoint2), ScrapHeap::ALIGNMENT));
        pTempColorArray = static_cast<NiColorA*>(pHeap->Allocate(512 * sizeof(NiColorA), ScrapHeap::ALIGNMENT));
        pTempIndexArray = static_cast<uint16_t*>(pHeap->Allocate(3072 * sizeof(uint16_t), ScrapHeap::ALIGNMENT));
        NiTriBasedGeom* pTarget = static_cast<NiTriBasedGeom*>(spTargetNode.operator->());
        uint32_t uiTriangles = pTarget->GetActiveTriangleCount();
        pOldToNewIndexArray = static_cast<int16_t*>(_NiMalloc(uiTriangles * 3 * sizeof(int16_t)));
        ourMemset(pOldToNewIndexArray, -1, uiTriangles * 3 * sizeof(int16_t));
    }
    float fAlphaScale = 1.0f / (1.0f - fDecalEpsilon);
    for (int32_t i = 0; i < aiVertexCount; ++i) {
        int32_t iIndex = apIndex[i];
        if (iIndex != -1) {
            if (pOldToNewIndexArray[iIndex] != -1) {
                apIndex[i] = pOldToNewIndexArray[iIndex];
                continue;
            }
            pTempVertexArray[iCount] = apVertex[i];
            pOldToNewIndexArray[iIndex] = static_cast<int16_t>(iCount);
        }
        else {
            for (uint32_t j = 0; j < pNewIndexMap.GetSize(); ++j) {
                if (BSUtilities::ComparePoints(&apVertex[i], &pNewIndexMap.GetAt(j)->fVert, 0.01f)) {
                    iIndex = pNewIndexMap.GetAt(j)->iIndex;
                    break;
                }
            }
            if (iIndex != -1) {
                apIndex[i] = iIndex;
                continue;
            }
            pTempVertexArray[iCount] = apVertex[i];
            void* pMemory = ourMalloc(sizeof(DecalVertex));
            DecalVertex* pVertex = pMemory ? ::new (pMemory) DecalVertex : nullptr;
            pVertex->fVert = pTempVertexArray[iCount];
            pVertex->iIndex = static_cast<int16_t>(iCount);
            pNewIndexMap.Add(pVertex);
        }
        pTempNormalArray[iCount] = apNormal[i];
        NiPoint3 kNormal = pTempNormalArray[iCount];
        double dDot = pDecalNormal.Dot(kNormal);
        float fAlpha = (dDot / kNormal.Length() - fDecalEpsilon) * fAlphaScale;
        float fClampedAlpha = fAlpha > 0.0 ? fAlpha : 0.0f;
        alignas(NiColorA) uint8_t aColorStorage[sizeof(NiColorA)];
        NiColorA* pColor =
            ::new (aColorStorage) NiColorA(fVertexColor.r, fVertexColor.g, fVertexColor.b, fClampedAlpha);
        pTempColorArray[iCount] = *pColor;
        apIndex[i] = iCount;
        ++iCount;
    }
    uint16_t* pIndices = pTempIndexArray + iDecalTriangleCount * 3;
    iDecalTriangleCount = aiVertexCount + iDecalTriangleCount - 2;
    int32_t iOut = 0;
    for (int32_t k = 2; k < aiVertexCount; ++k) {
        pIndices[iOut++] = static_cast<uint16_t>(apIndex[0]);
        pIndices[iOut++] = static_cast<uint16_t>(apIndex[k - 1]);
        pIndices[iOut++] = static_cast<uint16_t>(apIndex[k]);
    }
    iDecalVertexCount = iCount;
    return true;
}

void BSTempEffectSimpleDecal::CleanUpMemory() {
    if (!bValidDecal) {
        MemoryManager* v1 = &MemoryManager::Instance();
        ScrapHeap* pScrapHeap = v1->GetThreadScrapHeap();
        if (pTempVertexArray) {
            pScrapHeap->Deallocate(pTempVertexArray);
        }
        if (pTempNormalArray) {
            pScrapHeap->Deallocate(pTempNormalArray);
        }
        if (pTempTexcoordArray) {
            pScrapHeap->Deallocate(pTempTexcoordArray);
        }
        if (pTempColorArray) {
            pScrapHeap->Deallocate(pTempColorArray);
        }
        if (pTempIndexArray) {
            pScrapHeap->Deallocate(pTempIndexArray);
        }
        if (pOldToNewIndexArray) {
            _NiFree(pOldToNewIndexArray);
        }
        pTempVertexArray = 0;
        pTempNormalArray = 0;
        pTempTexcoordArray = 0;
        pTempColorArray = 0;
        pTempIndexArray = 0;
        pOldToNewIndexArray = 0;
    }
    for (int32_t i = 0; i < pNewIndexMap.iSize; ++i) {
        void* apThis = pNewIndexMap.GetAt(i);
        if (apThis) {
            delete static_cast<Date*>(apThis);
        }
        pNewIndexMap.GetAt(i) = 0;
    }
    pNewIndexMap.Clear(1);
}

bool BSTempEffectSimpleDecal::FinalizeGeometry() {
    FNVD_INLINED_LOCAL(NiPointer<NiRefObject>, spDecal_3, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiPointer<NiRefObject>, spDecal_2, std::nullopt, std::nullopt);
    // The debug colour, built on each pass.
    bool bForceSolidColor = 0;
    if (sp3D.m_pObject) {
        NiFixedString* PermanentDecal;
        BSShaderPPLightingProperty* plightingProp;
        NiAlphaProperty* palphaProp;
        NiMaterialProperty* pmatprop;
        NiMaterialProperty* v35 =
            static_cast<NiMaterialProperty*>(NiMemObject::operator new(sizeof(NiMaterialProperty)));
        if (v35) {
            pmatprop = ::new (v35) NiMaterialProperty;
        }
        else {
            pmatprop = 0;
        }
        NiProperty* pkProperty = (NiProperty*)pmatprop;
        pmatprop->SetShineness(fSpecular);
        sp3D.m_pObject->AttachProperty(pmatprop);
        void* apMemory = NiMemObject::operator new(sizeof(NiAlphaProperty));
        if (apMemory) {
            palphaProp = ::new (apMemory) NiAlphaProperty;
        }
        else {
            palphaProp = 0;
        }
        palphaProp->SetAlphaBlending(bUseAlphaBlending);
        palphaProp->SetAlphaTesting(bUseAlphaTesting);
        palphaProp->SetTestMode(NiAlphaProperty::TEST_GREATER);
        palphaProp->SetTestRef(128);
        sp3D.m_pObject->AttachProperty(palphaProp);
        if (bTwoSided) {
            NiStencilProperty* pstencilprop;
            NiStencilProperty* v31 =
                static_cast<NiStencilProperty*>(NiMemObject::operator new(sizeof(NiStencilProperty)));
            if (v31) {
                pstencilprop = ::new (v31) NiStencilProperty;
            }
            else {
                pstencilprop = 0;
            }
            pstencilprop->SetDrawMode(NiStencilProperty::DRAW_BOTH);
            sp3D.m_pObject->AttachProperty(pstencilprop);
        }
        Lighting30ShaderProperty* v53 = 0;
        Lighting30ShaderProperty* p30ShaderProperty =
            static_cast<Lighting30ShaderProperty*>(NiMemObject::operator new(sizeof(Lighting30ShaderProperty)));
        if (p30ShaderProperty) {
            plightingProp = (BSShaderPPLightingProperty*)::new (p30ShaderProperty) Lighting30ShaderProperty;
        }
        else {
            plightingProp = 0;
        }
        plightingProp->SetShader(29);
        plightingProp->SetFlag(5u, 1);
        BSTextureSet* AsShaderTextureSet = pTextureSet->GetAsShaderTextureSet();
        plightingProp->LoadTextureSet(0, AsShaderTextureSet);
        if (plightingProp->GetTexture(BSShaderProperty::TEXTYPE_NORMAL, 0)) {
            if (plightingProp->GetTexture(BSShaderProperty::TEXTYPE_HEIGHT, 0)) {
                plightingProp->SetFlag(11u, 1);
                if (bUseParallaxOcclusion) {
                    plightingProp->SetFlag(28u, 1);
                    plightingProp->SetParallaxOccScale(fParallaxOcclusionScale);
                    plightingProp->SetMaxParallaxPasses(static_cast<float>(ucParallaxOcclusionMaxPasses));
                }
            }
            plightingProp->SetFlag(27u, 1);
            NiGeometry* pgeom = static_cast<NiGeometry*>(spTargetNode.m_pObject);
            NiPropertyState* ppropertyState = &pgeom->m_PropertyState;
            if (ppropertyState->GetAlpha()->IsEnabled()) {
                plightingProp->SetFlag(53u, 1);
            }
            sp3D.m_pObject->AttachProperty(plightingProp);
            BSShaderManager::PrepareObject(sp3D.m_pObject, 0, 0);
        }
        else {
            BSShaderNoLightingProperty* pnolightingProp;
            BSShaderNoLightingProperty* v27 =
                static_cast<BSShaderNoLightingProperty*>(NiMemObject::operator new(sizeof(BSShaderNoLightingProperty)));
            if (v27) {
                pnolightingProp = ::new (v27) BSShaderNoLightingProperty;
            }
            else {
                pnolightingProp = 0;
            }
            if (pTextureSet) {
                if ((BGSDecalManager::iDecalDebugFlags & 32) == 0) {
                    NiPointer<NiTexture> spTempTexture(static_cast<NiTexture*>(0));
                    pTextureSet->GetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, spTempTexture);
                    NiTexture* Xa = spTempTexture.m_pObject;
                    pnolightingProp->SetBaseMap(Xa);
                }
            }
            pnolightingProp->SetFlag(27u, 1);
            NiGeometry* pgeom = static_cast<NiGeometry*>(spTargetNode.m_pObject);
            NiPropertyState* ppropertyState = &pgeom->m_PropertyState;
            if (ppropertyState->GetAlpha()->IsEnabled()) {
                pnolightingProp->SetFlag(53u, 1);
            }
            sp3D.m_pObject->AttachProperty(pnolightingProp);
            if ((BGSDecalManager::iDecalDebugFlags & 32) != 0) {
                NiGeometry* spdata = static_cast<NiGeometry*>(sp3D.m_pObject);
                NiColorA* pcolors = spdata->GetColors();
                for (int32_t i = 0;; ++i) {
                    uint16_t VertexCount = spdata->GetVertexCount();
                    if (i >= VertexCount) {
                        break;
                    }
                    NiColorA kSolidColor(1.0f, 1.0f, 0.0f, 1.0f);
                    NiColorA* pSolid = &kSolidColor;
                    pcolors[i].r = pSolid->r;
                    pcolors[i].g = pSolid->g;
                    pcolors[i].b = pSolid->b;
                    pcolors[i].a = pSolid->a;
                }
            }
            BSShaderManager::PrepareObject(sp3D.m_pObject, 0, 0);
        }
        if (bPermanent) {
            PermanentDecal = FixedStrings::GetPermanentDecal();
        }
        else {
            PermanentDecal = FixedStrings::GetDecal();
        }
        sp3D.m_pObject->SetName(*PermanentDecal);
        TESHavokUtilities::RemoveHavokFromSceneGraph(sp3D.m_pObject);
        BSShaderManager::SetObjectTypeRecurse(sp3D.m_pObject, BSShaderManager::BSSM_OBJ_MISC);
        if (!bPermanent) {
            if (bForceFade) {
                ReferenceProcessLists.AddTempEffect(this);
            }
            else {
                ::new (&spDecal_2) NiPointer<NiRefObject>(this);
                BSTempEffect::kDecals.AddTail(spDecal_2);
                spDecal_2.~NiPointer<NiRefObject>();
                if (++BSTempEffect::iNumDecals > BSTempEffectSimpleDecal::iCurrentMaxDecalCount) {
                    ReferenceProcessLists.AddTempEffect(
                        static_cast<BSTempEffect*>((&BSTempEffect::kDecals)->m_item.m_pObject)
                    );
                    BSTempEffect::kDecals.RemoveHead();
                    --BSTempEffect::iNumDecals;
                    if (BGSDecalManager::bDebugDecals.Bool()) {
                        Warning(
                            "DECAL: Reached Max Decal Limit of %d! Removing oldest decal...",
                            BSTempEffectSimpleDecal::iCurrentMaxDecalCount
                        );
                    }
                }
            }
        }
        NiNode* pnode = spTargetNode.m_pObject->m_pkParent;
        BGSDecalNode* pdecalnode = 0;
        bool bfoundnode = 0;
        if (pnode) {
            for (int32_t i_2 = 0;; ++i_2) {
                uint32_t ArrayCount = pnode->GetArrayCount();
                if (i_2 >= ArrayCount) {
                    break;
                }
                NiNode* pchild = static_cast<NiNode*>(pnode->GetAt(static_cast<uint32_t>(i_2)));
                if (pchild && NiObject::IsExactKindOf(&BGSDecalNode::ms_RTTI, pchild)) {
                    bfoundnode = 1;
                    pdecalnode = static_cast<BGSDecalNode*>(pchild);
                    break;
                }
            }
            if (bfoundnode) {
                pParentNode = pdecalnode;
            }
            else {
                BGSDecalNode* v15;
                BGSDecalNode* v23 = static_cast<BGSDecalNode*>(NiMemObject::operator new(sizeof(BGSDecalNode)));
                if (v23) {
                    v15 = ::new (v23) BGSDecalNode(spTargetNode.m_pObject->m_pkParent, 0);
                }
                else {
                    v15 = 0;
                }
                pParentNode = v15;
            }
            pParentNode->AttachChild(this, 1);
            sp3D.m_pObject->SetTranslate(spTargetNode.m_pObject->GetTranslate());
            sp3D.m_pObject->SetRotate((&spTargetNode.m_pObject->m_kLocal)->m_Rotate);
            float Xb = spTargetNode.m_pObject->m_kLocal.m_fScale;
            sp3D.m_pObject->SetScale(Xb);
            sp3D.m_pObject->UpdateProperties();
            NiUpdateData UpdateData(fCellAppTime, 0, 0);
            sp3D.m_pObject->Update(UpdateData);
            NiAVObject* fB = Get3D();
            ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
            ShadowSceneNode_->UpdateObjectLighting(fB, 0);
            ::new (&spDecal_3) NiPointer<NiRefObject>(this);
            pParentNode->pDecalList->AddHead(spDecal_3);
            spDecal_3.~NiPointer<NiRefObject>();
            CleanUpMemory();
            bFinalize = 0;
            bValidDecal = 1;
            bFinished = 1;
            return 1;
        }
        else {
            bFinished = 1;
            return 0;
        }
    }
    else {
        bFinished = 1;
        return 0;
    }
}

NiTriShapeData* BSTempEffectSimpleDecal::CreateTriData() {
    NiTriShapeData* apMemory = static_cast<NiTriShapeData*>(NiMemObject::operator new(sizeof(NiTriShapeData)));
    if (apMemory) {
        // Both counts are passed truncated to their low 16 bits.
        return ::new (apMemory) NiTriShapeData(
            static_cast<uint16_t>(iDecalVertexCount), pFinalVertexArray, pFinalNormalArray, pFinalColorArray,
            pFinalTexcoordArray, 1, NiGeometryData::NBT_METHOD_NONE, static_cast<uint16_t>(iDecalTriangleCount),
            pFinalIndexArray
        );
    }
    else {
        return 0;
    }
}

void BSTempEffectSimpleDecal::ClipMesh(NiAVObject* apAVObj) {
    NiTriStrips* ptristrips = static_cast<NiTriStrips*>(apAVObj);
    const NiPoint3* pvertices = static_cast<NiGeometry*>(apAVObj)->NiGeometry::GetVertices();
    const NiPoint3* pnormals = static_cast<NiGeometry*>(apAVObj)->NiGeometry::GetNormals();
    NiTriStripsData* pgeomdata = static_cast<NiTriStripsData*>(static_cast<NiGeometry*>(apAVObj)->GetModelData());
    if (!pvertices || pnormals) {
        if (pgeomdata) {
            NiPoint3 newVertex[9];
            NiPoint3 newNormal[9];
            rt::eh_vector_constructor_iterator(newVertex, sizeof(NiPoint3), 9, &emptyPointConstructor);
            rt::eh_vector_constructor_iterator(newNormal, sizeof(NiPoint3), 9, &emptyPointConstructor);
            int32_t tricount = ptristrips->GetActiveTriangleCount();
            uint16_t* pindexlist = pgeomdata->m_pusStripLists;
            for (int32_t i = 0; i < tricount; ++i) {
                if (pindexlist[i] != pindexlist[i + 1] && pindexlist[i] != pindexlist[i + 2] &&
                    pindexlist[i + 1] != pindexlist[i + 2]) {
                    uint16_t i2;
                    uint16_t i1;
                    if (i % 2) {
                        i1 = pindexlist[i + 1];
                        i2 = pindexlist[i];
                    }
                    else {
                        i1 = pindexlist[i];
                        i2 = pindexlist[i + 1];
                    }
                    uint16_t i3 = pindexlist[i + 2];
                    const NiPoint3* v1 = &pvertices[i1];
                    const NiPoint3* v2 = &pvertices[i2];
                    const NiPoint3* v3 = &pvertices[i3];
                    NiPoint3 vec1 = *v2 - *v1;
                    NiPoint3 vec2 = *v3 - *v1;
                    NiPoint3 cross = vec1.Cross(vec2);
                    double v4 = pDecalNormal.Dot(cross);
                    if (cross.Length() * BSTempEffectSimpleDecal::fDecalEpsilon < v4) {
                        int32_t newIndex[9];
                        newIndex[0] = i1;
                        newIndex[1] = i2;
                        newIndex[2] = i3;
                        newVertex[0] = *v1;
                        newVertex[1] = *v2;
                        newVertex[2] = *v3;
                        newNormal[0] = pnormals[i1];
                        newNormal[1] = pnormals[i2];
                        newNormal[2] = pnormals[i3];
                        int32_t count = ClipPolygon(3, newIndex, newVertex, newNormal, newIndex, newVertex, newNormal);
                        if (count) {
                            if (!AddPolygon(count, newIndex, newVertex, newNormal)) {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    else {
        Warning("TEXTURES: Decal Parent '%s' has no normals!", (&apAVObj->m_kName)->GetHandleString());
    }
}

void BSTempEffectSimpleDecal::CreateDebugBox(NiColorA& arColor, bool abWireframe) {
    FNVD_INLINED_LOCAL(NiFixedString, v22, std::nullopt, std::nullopt);
    NiMatrix3 kinvobjrot;
    NiMatrix3 kobjrot;
    FNVD_INLINED_LOCAL(NiPointer<NiObject>, spobj, std::nullopt, std::nullopt);
    if (!BGSDecalManager::spDebugDecalBox.m_pObject) {
        BSStream stream;
        if (stream.BSStream::Load("Data\\Meshes\\Decals\\DebugDecalBox.NIF", 0)) {
            if (stream.GetObjectCount() == 1) {
                BGSDecalManager::spDebugDecalBox.operator=(static_cast<NiAVObject*>(stream.GetObjectAt(0)));
            }
            NiFixedString apString("Debug Decal Box");
            NiFixedString* kName = &apString;
            BGSDecalManager::spDebugDecalBox.m_pObject->SetName(*kName);
        }
    }
    if (BGSDecalManager::spDebugDecalBox.m_pObject) {
        BSShaderNoLightingProperty* pshaderprop;
        NiAlphaProperty* palpha;
        NiMatrix3 kzrot;
        ::new (&spobj) NiPointer<ActorPathingMessageQueue>(static_cast<ActorPathingMessageQueue*>(0));
        BGSDecalManager::spDebugDecalBox.m_pObject->CreateDeepCopy(spobj);
        NiNode* ptmp = static_cast<NiNode*>(spobj.m_pObject);
        NiFixedString* v14 = ::new (static_cast<void*>(&v22)) NiFixedString("DebugDecalBox01:0");
        NiAVObject* pavobj = ptmp->GetObjectByName(*v14);
        v22.~NiFixedString();
        NiGeometry* spdata = static_cast<NiGeometry*>(pavobj);
        TESHavokUtilities::RemoveHavokFromSceneGraph(ptmp);
        NiPoint3* pverts = spdata->GetVertices();
        NiColorA* pcolors = spdata->GetColors();
        for (int32_t i = 0;; ++i) {
            int32_t VertexCount = spdata->GetVertexCount();
            if (i >= VertexCount) {
                break;
            }
            pverts[i].x = pverts[i].x * fWidth;
            pverts[i].y = pverts[i].y * this->GetDecalHeight();
            pverts[i].z = (fDepth + fDepth) * pverts[i].z;
            pcolors[i] = arColor;
        }
        if (FloatEQ(pDecalNormal.x, 0.0f, 0.001f)) {
            pDecalNormal.x = 0.0f;
        }
        if (FloatEQ(pDecalNormal.y, 0.0f, 0.001f)) {
            pDecalNormal.y = 0.0f;
        }
        if (FloatEQ(pDecalNormal.z, 0.0f, 0.001f)) {
            pDecalNormal.z = 0.0f;
        }
        NiPoint3 decalnormal = pDecalNormal;
        if (decalnormal.operator==(NiPoint3_ZERO)) {
            decalnormal = pVector;
            if (bModelSpace) {
                kobjrot = (*&spTargetNode.m_pObject->m_kLocal).m_Rotate;
            }
            else {
                kobjrot = spTargetNode.m_pObject->GetWorldTransform().m_Rotate;
            }
            kobjrot.Inverse(&kinvobjrot);
            decalnormal = kinvobjrot * decalnormal;
        }
        NiMatrix3 kzvrot = BSUtilities::GetRotationZToVectorTranspose(decalnormal);
        kzrot.MakeZRotation(-fRotation);
        if (bModelSpace) {
            const NiPoint3* aVector_8a = &spTargetNode.m_pObject->GetTranslate();
            ptmp->SetTranslate(*aVector_8a);
            NiTransform* aVector_8b = &spTargetNode.m_pObject->m_kLocal;
            ptmp->SetRotate(aVector_8b->m_Rotate);
            float aVector_8c = spTargetNode.m_pObject->m_kLocal.m_fScale;
            ptmp->SetScale(aVector_8c);
            NiMatrix3 rot__kzrot = kzvrot * kzrot;
            pavobj->SetRotate(rot__kzrot);
            pavobj->SetTranslate(pDecalCenter);
        }
        else {
            NiMatrix3 rot__kzrot_2 = (&spTargetNode.m_pObject->GetWorldTransform())->m_Rotate * kzvrot * kzrot;
            pavobj->SetRotate(rot__kzrot_2);
            pavobj->SetTranslate(pOrigin);
        }
        ptmp->DetachAllProperties();
        NiAlphaProperty* v19 = static_cast<NiAlphaProperty*>(NiMemObject::operator new(sizeof(NiAlphaProperty)));
        if (v19) {
            palpha = ::new (v19) NiAlphaProperty;
        }
        else {
            palpha = 0;
        }
        palpha->SetAlphaBlending(1);
        ptmp->AttachProperty(palpha);
        BSShaderNoLightingProperty* v18 =
            static_cast<BSShaderNoLightingProperty*>(NiMemObject::operator new(sizeof(BSShaderNoLightingProperty)));
        if (v18) {
            pshaderprop = ::new (v18) BSShaderNoLightingProperty;
        }
        else {
            pshaderprop = 0;
        }
        if (abWireframe) {
            pshaderprop->SetFlag(48u, 1);
        }
        pavobj->AttachProperty(pshaderprop);
        BSShaderManager::PrepareObject(pavobj, 0, 0);
        if (bModelSpace) {
            BSTempNode* pn;
            BSTempNode* v17 = static_cast<BSTempNode*>(NiMemObject::operator new(sizeof(BSTempNode)));
            if (v17) {
                pn = ::new (v17) BSTempNode(30.0f);
            }
            else {
                pn = 0;
            }
            pn->AttachChild(ptmp, 1);
            NiNode* Parent = spTargetNode.m_pObject->m_pkParent;
            Parent->AttachChild(pn, 1);
        }
        else {
            TES::pInstance->AddTempDebugObject(ptmp, 30.0f);
        }
        NiNode* v9 = ptmp->m_pkParent;
        v9->UpdateWorldBound();
        NiUpdateData UpdateData(0.0f, 0, 0);
        ptmp->Update(UpdateData);
        spobj.~NiPointer<NiObject>();
    }
}

void BSTempEffectSimpleDecal::ResizeDataArrays() {
    NiColorA* v2;
    NiPoint2* v3;
    NiPoint3* v4;
    NiPoint3* v5;
    MemoryManager* v1 = &MemoryManager::Instance();
    ScrapHeap* pScrapHeap = v1->GetThreadScrapHeap();
    uint32_t numObjs = this->iDecalVertexCount;
    void* ptr = ourMalloc(numObjs <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * numObjs : SIZE_MAX);
    if (ptr) {
        rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), numObjs, &emptyPointConstructor);
        v5 = static_cast<NiPoint3*>(ptr);
    }
    else {
        v5 = 0;
    }
    pFinalVertexArray = v5;
    ourMemcpy(pFinalVertexArray, pTempVertexArray, sizeof(NiPoint3) * this->iDecalVertexCount);
    pScrapHeap->Deallocate(pTempVertexArray);
    pTempVertexArray = 0;
    uint32_t count = this->iDecalVertexCount;
    void* v11 = ourMalloc(count <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * count : SIZE_MAX);
    if (v11) {
        rt::eh_vector_constructor_iterator(v11, sizeof(NiPoint3), count, &emptyPointConstructor);
        v4 = static_cast<NiPoint3*>(v11);
    }
    else {
        v4 = 0;
    }
    pFinalNormalArray = v4;
    ourMemcpy(pFinalNormalArray, pTempNormalArray, sizeof(NiPoint3) * this->iDecalVertexCount);
    pScrapHeap->Deallocate(pTempNormalArray);
    pTempNormalArray = 0;
    uint32_t aiCount = this->iDecalVertexCount;
    void* v9 = ourMalloc(aiCount <= SIZE_MAX / sizeof(NiPoint2) ? sizeof(NiPoint2) * aiCount : SIZE_MAX);
    if (v9) {
        rt::eh_vector_constructor_iterator(v9, sizeof(NiPoint2), aiCount, &emptyPointConstructor);
        v3 = static_cast<NiPoint2*>(v9);
    }
    else {
        v3 = 0;
    }
    pFinalTexcoordArray = v3;
    ourMemcpy(pFinalTexcoordArray, pTempTexcoordArray, sizeof(NiPoint2) * this->iDecalVertexCount);
    pScrapHeap->Deallocate(pTempTexcoordArray);
    pTempTexcoordArray = 0;
    uint32_t iDecalVertexCount = this->iDecalVertexCount;
    void* v7 =
        ourMalloc(iDecalVertexCount <= SIZE_MAX / sizeof(NiColorA) ? sizeof(NiColorA) * iDecalVertexCount : SIZE_MAX);
    if (v7) {
        rt::eh_vector_constructor_iterator(v7, sizeof(NiColorA), iDecalVertexCount, &fnvd::constructElement<NiColorA>);
        v2 = static_cast<NiColorA*>(v7);
    }
    else {
        v2 = 0;
    }
    pFinalColorArray = v2;
    ourMemcpy(pFinalColorArray, pTempColorArray, sizeof(NiColorA) * this->iDecalVertexCount);
    pScrapHeap->Deallocate(pTempColorArray);
    pTempColorArray = 0;
    pFinalIndexArray = static_cast<uint16_t*>(_NiMalloc(3 * sizeof(uint16_t) * iDecalTriangleCount));
    ourMemcpy(pFinalIndexArray, pTempIndexArray, 3 * sizeof(uint16_t) * iDecalTriangleCount);
    pScrapHeap->Deallocate(pTempIndexArray);
    pTempIndexArray = 0;
}

int32_t BSTempEffectSimpleDecal::ClipPolygonAgainstPlane(
    NiPlane* apPlane,
    int32_t aiVertexCount,
    const int32_t* apInIndex,
    const NiPoint3* apInVertex,
    const NiPoint3* apInNormal,
    int32_t* apOutIndex,
    NiPoint3* apOutVertex,
    NiPoint3* apOutNormal
) {
    NiPlane* v9;
    NiPoint3* v10;
    NiPoint3* v11;
    NiPoint3* v12;
    NiPoint3* v13;
    NiPlane* v14;
    NiPoint3* v15;
    NiPoint3* v16;
    NiPoint3* v17;
    NiPoint3* v18;
    const NiPoint3* v19;
    NiPoint3* v20;
    const NiPoint3* v21;
    NiPoint3* v22;
    NiPoint3* fScalar_4;
    NiPoint3* fScalar_4a;
    NiPoint3* fScalar_4b;
    NiPoint3* fScalar_4c;
    float v27;
    float v28;
    double v29;
    double v30;
    float v31;
    float v32;
    double v33;
    double v34;
    int32_t v35;
    NiPoint3 v36;
    NiPoint3 v37;
    NiPoint3 v38;
    NiPoint3 v39;
    NiPoint3 v40;
    NiPoint3 v41;
    NiPoint3 v42;
    NiPoint3 v43;
    NiPoint3 v44;
    NiPoint3 v45;
    NiPoint3 v46;
    NiPoint3 v47;
    NiPoint3 v48;
    float v49;
    NiPoint3 v50;
    NiPoint3 v51;
    NiPoint3 v52;
    NiPoint3 v53;
    NiPoint3 v54;
    NiPoint3 v55;
    float fScalar;
    NiPoint3 v57;
    NiPoint3 pt;
    NiPoint3 v59;
    NiPoint3 v60;
    NiPoint3 v61;
    int32_t v62;
    int32_t j;
    int32_t i;
    uint8_t v67[12];
    int32_t v65 = 0;
    for (i = 0; i < aiVertexCount; ++i) {
        if (apPlane->WhichSide(apInVertex[i]) == 2) {
            v67[i] = (uint8_t)1;
            ++v65;
        }
        else {
            v67[i] = (uint8_t)0;
        }
    }
    if (v65 == aiVertexCount) {
        return 0;
    }
    int32_t result = 0;
    for (j = 0; j < aiVertexCount; ++j) {
        if (j) {
            v35 = j - 1;
        }
        else {
            v35 = aiVertexCount - 1;
        }
        v62 = v35;
        if (v67[j]) {
            if (!v67[v62]) {
                v60 = apInVertex[v62];
                pt = apInVertex[j];
                (v61 = (&v60)->operator-(pt));
                v9 = apPlane;
                v55.x = v9->m_kNormal.x;
                v55.y = v9->m_kNormal.y;
                v55.z = v9->m_kNormal.z;
                v34 = (double)(&v55)->Dot(v60);
                // The constant is negated and then added, so a NaN constant reaches fScalar with its sign flipped.
                v33 = v34 + -apPlane->GetConstant();
                fScalar = (float)(v33 / (&v55)->Dot(v61));
                if ((fScalar > 1.0) && (fScalar <= 1.000100016593933)) {
                    fScalar = (float)1.0;
                }
                if (!(fScalar < 0.0)) {
                    if (fScalar > 1.0) {
                        Warning(
                            "BSTempEffectSimpleDecal: Bad t value (%f) calculated while attempting to clip decal.  "
                            "See Jeff S.",
                            fScalar
                        );
                        fScalar = (float)1.0;
                    }
                }
                else {
                    Warning(
                        "BSTempEffectSimpleDecal: Bad t value (%f) calculated while attempting to clip decal.  "
                        "See Jeff S.",
                        fScalar
                    );
                    fScalar = (float)0.0;
                }
                apOutIndex[result] = (-1);
                fScalar_4 = &(v46 = (&pt)->operator*(fScalar));
                v32 = (float)(1.0 - fScalar);
                v10 = &(v47 = (&v60)->operator*(v32));
                v11 = &(v45 = v10->operator+(*fScalar_4));
                apOutVertex[result] = *v11;
                v59 = apInNormal[v62];
                v57 = apInNormal[j];
                fScalar_4a = &(v43 = (&v59)->operator*(fScalar));
                v31 = (float)(1.0 - fScalar);
                v12 = &(v44 = (&v57)->operator*(v31));
                v13 = &(v42 = v12->operator+(*fScalar_4a));
                apOutNormal[result++] = *v13;
            }
        }
        else {
            if (v67[v62]) {
                v53 = apInVertex[j];
                v51 = apInVertex[v62];
                (v54 = (&v53)->operator-(v51));
                v14 = apPlane;
                v48.x = v14->m_kNormal.x;
                v48.y = v14->m_kNormal.y;
                v48.z = v14->m_kNormal.z;
                v30 = (double)(&v48)->Dot(v53);
                v29 = v30 + -apPlane->GetConstant();
                v49 = (float)(v29 / (&v48)->Dot(v54));
                if (!(v49 < 0.0)) {
                    if (v49 > 1.0) {
                        Warning(
                            "BSTempEffectSimpleDecal: Bad t value (%f) calculated while attempting to clip decal.  "
                            "See Jeff S.",
                            v49
                        );
                        v49 = (float)1.0;
                    }
                }
                else {
                    Warning(
                        "BSTempEffectSimpleDecal: Bad t value (%f) calculated while attempting to clip decal.  "
                        "See Jeff S.",
                        v49
                    );
                    v49 = (float)0.0;
                }
                apOutIndex[result] = (-1);
                fScalar_4b = &(v40 = (&v51)->operator*(v49));
                v28 = (float)(1.0 - v49);
                v15 = &(v41 = (&v53)->operator*(v28));
                v16 = &(v39 = v15->operator+(*fScalar_4b));
                apOutVertex[result] = *v16;
                v52 = apInNormal[j];
                v50 = apInNormal[v62];
                fScalar_4c = &(v37 = (&v52)->operator*(v49));
                v27 = (float)(1.0 - v49);
                v17 = &(v38 = (&v50)->operator*(v27));
                v18 = &(v36 = v17->operator+(*fScalar_4c));
                apOutNormal[result++] = *v18;
            }
            apOutIndex[result] = apInIndex[j];
            v19 = &apInVertex[j];
            v20 = &apOutVertex[result];
            v20->x = v19->x;
            v20->y = v19->y;
            v20->z = v19->z;
            v21 = &apInNormal[j];
            v22 = &apOutNormal[result];
            v22->x = v21->x;
            v22->y = v21->y;
            v22->z = v21->z;
            ++result;
        }
    }
    return result;
}

void BSTempEffectSimpleDecal::PreloadDecalData() {
    if (!BSTempEffectSimpleDecal::spPreloadedDecalTextureSets.m_pObject) {
        QueuedFile* v0;
        AutoMemContext apMemory(
            MEM_CONTEXT_EFFECTS_OVERHEAD, 1,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectSimpleDecal.cpp", 287
        );
        QueuedFile* v1 = static_cast<QueuedFile*>(ourMalloc(sizeof(QueuedFile)));
        if (v1) {
            v0 = ::new (v1) QueuedFile(IO_TASK_PRIORITY_VERY_LOW);
        }
        else {
            v0 = 0;
        }
        BSTempEffectSimpleDecal::spPreloadedDecalTextureSets.operator=(v0);
    }
}

void BSTempEffectSimpleDecal::UnloadDecalData() {
    BSTempEffectSimpleDecal::spPreloadedDecalTextureSets.operator=(static_cast<QueuedFile*>(0));
}

void NiAVObject::DetachAllProperties() {
    m_kPropertyList.RemoveAll();
}
