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

NiPointer<QueuedFile> BSTempEffectSimpleDecal::spPreloadedDecalTextureSets;
NiRTTI BSTempEffectSimpleDecal::ms_RTTI("BSTempEffectSimpleDecal", &BSTempEffect::ms_RTTI);

double BSMAX_0(double adA, float afB) {
    if (afB < adA) {
        return adA;
    }

    return afB;
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
    if (!bPermanent && sp3D.m_pObject) {
        ShadowSceneNode::RemoveLightsFromGeom(static_cast<NiTriBasedGeom*>(sp3D.m_pObject));
        if (sp3D->m_pkParent) {
            NiNode* pParent = sp3D->m_pkParent;
            pParent->DetachChild(sp3D.m_pObject);
        }
        sp3D = static_cast<NiAVObject*>(nullptr);
    }

    CleanUpMemory();
    if (bValidDecal) {
        --BGSDecalManager::iDecalCount;
    }
}

void BSTempEffectSimpleDecal::Initialize() {
    if (ProcessGeometry(spTargetNode)) {
        BSTempEffect::Initialize();
    }
}

bool BSTempEffectSimpleDecal::Update(float afTimeDelta) {
    bool bkeep = BSTempEffect::Update(afTimeDelta);
    if (bkeep || !sp3D.m_pObject) {
        return bkeep;
    }
    double dAge = fAge;
    float ftimer = dAge - fLifetime;
    if (!(ftimer >= 0.0) || !(ftimer <= 1.0)) {
        BGSDecalNode* pnode = static_cast<BGSDecalNode*>(sp3D->m_pkParent);
        if (pnode) {
            pnode->DetachChild(this);
        }
        return bkeep;
    }
    float falpha = (1.0 - ftimer) / 1.0;
    int32_t iType = NiMaterialProperty::GetType();
    NiMaterialProperty* pMaterial = static_cast<NiMaterialProperty*>(sp3D->GetProperty(iType));
    if (pMaterial && falpha != pMaterial->m_fAlpha) {
        pMaterial->SetAlpha(falpha);
    }

    return true;
}

NiAVObject* BSTempEffectSimpleDecal::Get3D() {
    return sp3D;
}

NiAVObject* BSTempEffectSimpleDecal::GetDecalParent() {
    return spTargetNode;
}

bool BSTempEffectSimpleDecal::GetIsValid() {
    return bValidDecal;
}

NiPoint3 BSTempEffectSimpleDecal::GetOrigin() {
    return pOrigin;
}

NiPoint3 BSTempEffectSimpleDecal::GetDirection() {
    return pVector;
}

bool BSTempEffectSimpleDecal::ProcessGeometry(NiAVObject* apAVObj) {
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
            kobjrot = apAVObj->m_kLocal.m_Rotate;
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
            float fInvScale = 1.0 / apAVObj->m_kWorld.m_fScale;
            kpos = kpos * fInvScale;
        }
        NiMatrix3::TransformVertices(kinvobjrot, NiPoint3::ZERO, 1u, &kpos, &kmodelspaceorigin);
        fWidth = 1.0 / apAVObj->m_kWorld.m_fScale * fWidth;
        fHeight = 1.0 / apAVObj->m_kWorld.m_fScale * this->GetDecalHeight();
        fDepth = 1.0 / apAVObj->m_kWorld.m_fScale * fDepth;
        pDecalCenter = kmodelspaceorigin;
        pDecalNormal = kmodelspaceray;
        ::new (&rot) NiMatrix3(BSUtilities::GetRotationZToVector(kmodelspaceray));
        kzrot.MakeZRotation(fRotation);
        rot = kzrot * rot;
        rot.GetRow(0, tangent);
        rot.GetRow(1u, binormal);
        float d = pDecalCenter.Dot(tangent);
        float fConstant = -(fWidth * 0.5 - d);
        pClippingPlanes[0] = NiPlane(NiPoint3(tangent.x, tangent.y, tangent.z), fConstant);
        fConstant = -(fWidth * 0.5 + d);
        pClippingPlanes[1] = NiPlane(NiPoint3(-tangent.x, -tangent.y, -tangent.z), fConstant);
        d = pDecalCenter.Dot(binormal);
        fConstant = -(this->GetDecalHeight() * 0.5 - d);
        pClippingPlanes[2] = NiPlane(NiPoint3(binormal.x, binormal.y, binormal.z), fConstant);
        fConstant = -(this->GetDecalHeight() * 0.5 + d);
        pClippingPlanes[3] = NiPlane(NiPoint3(-binormal.x, -binormal.y, -binormal.z), fConstant);
        d = pDecalCenter.Dot(pDecalNormal);
        fConstant = -(fDepth + d);
        pClippingPlanes[4] = NiPlane(NiPoint3(-pDecalNormal.x, -pDecalNormal.y, -pDecalNormal.z), fConstant);
        fConstant = -(fDepth - d);
        pClippingPlanes[5] = NiPlane(NiPoint3(pDecalNormal.x, pDecalNormal.y, pDecalNormal.z), fConstant);
        ClipMesh(apAVObj);
        if (iDecalTriangleCount) {
            float one_over_w = 1.0 / fWidth;
            float one_over_h = 1.0 / this->GetDecalHeight();
            for (int32_t a = 0; a < iDecalVertexCount; ++a) {
                NiPoint3 v = pTempVertexArray[a] - pDecalCenter;
                double dTangentDist = v.Dot(tangent);
                float fU = dTangentDist * one_over_w + 0.5;
                double dBinormalDist = v.Dot(binormal);
                float fV = dBinormalDist * one_over_h + 0.5;
                int32_t subtex = cSubTexture;
                float s = fU / 2.0;
                float t = fV / 2.0;
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
                NiPoint2 kTexcoord;
                if (bPermanent) {
                    float fFlippedS = 1.0 - s;
                    kTexcoord = NiPoint2(fFlippedS, t);
                }
                else {
                    kTexcoord = NiPoint2(s, t);
                }
                NiPoint2* pTexcoords = pTempTexcoordArray;
                pTexcoords[a] = kTexcoord;
            }
            ResizeDataArrays();
            NiTriShapeData* data = CreateTriData();
            sp3D = static_cast<NiAVObject*>(NiTriShape::Create(data));
            if (sp3D.m_pObject) {
                pFinalVertexArray = nullptr;
                pFinalNormalArray = nullptr;
                pFinalTexcoordArray = nullptr;
                pFinalColorArray = nullptr;
                pFinalIndexArray = nullptr;
            }
            bFinalize = true;
            return true;
        }

        bFinished = true;
        if (BGSDecalManager::bDebugDecals.Bool()) {
            Warning("DECAL: No Decal for : %s", spTargetNode->m_kName.GetHandleString());
        }
        return false;
    }

    bFinished = true;
    return false;
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
    NiPoint3 kTempNormal[9];
    int32_t aiTempIndex[9];
    int32_t iCount = ClipPolygonAgainstPlane(
        &pClippingPlanes[0],
        aiVertexCount,
        apInIndex,
        apInVertex,
        apInNormal,
        aiTempIndex,
        kTempVertex,
        kTempNormal
    );
    if (!iCount) {
        return iCount;
    }
    iCount = ClipPolygonAgainstPlane(
        &pClippingPlanes[1],
        iCount,
        aiTempIndex,
        kTempVertex,
        kTempNormal,
        apOutIndex,
        apOutVertex,
        apOutNormal
    );
    if (!iCount) {
        return iCount;
    }
    iCount = ClipPolygonAgainstPlane(
        &pClippingPlanes[2],
        iCount,
        apOutIndex,
        apOutVertex,
        apOutNormal,
        aiTempIndex,
        kTempVertex,
        kTempNormal
    );
    if (!iCount) {
        return iCount;
    }
    iCount = ClipPolygonAgainstPlane(
        &pClippingPlanes[3],
        iCount,
        aiTempIndex,
        kTempVertex,
        kTempNormal,
        apOutIndex,
        apOutVertex,
        apOutNormal
    );
    if (!iCount) {
        return iCount;
    }
    iCount = ClipPolygonAgainstPlane(
        &pClippingPlanes[5],
        iCount,
        apOutIndex,
        apOutVertex,
        apOutNormal,
        aiTempIndex,
        kTempVertex,
        kTempNormal
    );
    if (!iCount) {
        return iCount;
    }
    return ClipPolygonAgainstPlane(
        &pClippingPlanes[4],
        iCount,
        aiTempIndex,
        kTempVertex,
        kTempNormal,
        apOutIndex,
        apOutVertex,
        apOutNormal
    );
}

bool BSTempEffectSimpleDecal::CheckOcclusionQueryResults() {
    if (!pOcclusionQuery->GetResult(&iCurrentOcclusionResult, false)) {
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
        NiTriBasedGeom* pTarget = NiSmartPointerCast<NiTriBasedGeom>(spTargetNode);
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
            DecalVertex* pVertex = new DecalVertex;
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
        NiColorA* pColor = ::new (aColorStorage)
            NiColorA(fVertexColor.r, fVertexColor.g, fVertexColor.b, fClampedAlpha);
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
        ScrapHeap* pScrapHeap = MemoryManager::Instance().GetThreadScrapHeap();
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
        pTempVertexArray = nullptr;
        pTempNormalArray = nullptr;
        pTempTexcoordArray = nullptr;
        pTempColorArray = nullptr;
        pTempIndexArray = nullptr;
        pOldToNewIndexArray = nullptr;
    }
    for (DecalVertex*& pDecalVertex : pNewIndexMap) {
        delete pDecalVertex;
        pDecalVertex = nullptr;
    }
    pNewIndexMap.Clear(true);
}

bool BSTempEffectSimpleDecal::FinalizeGeometry() {
    if (sp3D.m_pObject) {
        BSShaderPPLightingProperty* plightingProp;
        NiAlphaProperty* palphaProp;
        NiMaterialProperty* pmatprop;
        pmatprop = new NiMaterialProperty;
        pmatprop->SetShineness(fSpecular);
        sp3D->AttachProperty(pmatprop);
        palphaProp = new NiAlphaProperty;
        palphaProp->SetAlphaBlending(bUseAlphaBlending);
        palphaProp->SetAlphaTesting(bUseAlphaTesting);
        palphaProp->SetTestMode(NiAlphaProperty::TEST_GREATER);
        palphaProp->SetTestRef(128);
        sp3D->AttachProperty(palphaProp);
        if (bTwoSided) {
            NiStencilProperty* pstencilprop;
            pstencilprop = new NiStencilProperty;
            pstencilprop->SetDrawMode(NiStencilProperty::DRAW_BOTH);
            sp3D->AttachProperty(pstencilprop);
        }
        plightingProp = new Lighting30ShaderProperty;
        plightingProp->SetShader(29);
        plightingProp->SetFlag(BSShaderProperty::BSSP_SINGLEPASS, 1);
        BSTextureSet* pShaderTextureSet = pTextureSet->GetAsShaderTextureSet();
        plightingProp->LoadTextureSet(nullptr, pShaderTextureSet);
        if (plightingProp->GetTexture(BSShaderProperty::TEXTYPE_NORMAL, 0)) {
            if (plightingProp->GetTexture(BSShaderProperty::TEXTYPE_HEIGHT, 0)) {
                plightingProp->SetFlag(BSShaderProperty::BSSP_FLAG_PARALLAX, 1);
                if (bUseParallaxOcclusion) {
                    plightingProp->SetFlag(BSShaderProperty::BSSP_FLAG_PARALLAXOCCLUSION, 1);
                    plightingProp->SetParallaxOccScale(fParallaxOcclusionScale);
                    plightingProp->SetMaxParallaxPasses(static_cast<float>(ucParallaxOcclusionMaxPasses));
                }
            }
            plightingProp->SetFlag(BSShaderProperty::BSSP_FLAG_DYNAMIC_DECAL, 1);
            NiGeometry* pgeom = static_cast<NiGeometry*>(spTargetNode.m_pObject);
            NiPropertyState* ppropertyState = &pgeom->m_PropertyState;
            if (ppropertyState->GetAlpha()->IsEnabled()) {
                plightingProp->SetFlag(BSShaderProperty::BSSP_FLAG_ALPHA_DECAL, 1);
            }
            sp3D->AttachProperty(plightingProp);
            BSShaderManager::PrepareObject(sp3D.m_pObject, 0, 0);
        }
        else {
            BSShaderNoLightingProperty* pnolightingProp;
            pnolightingProp = new BSShaderNoLightingProperty;
            if (pTextureSet && (BGSDecalManager::iDecalDebugFlags & BGSDecalManager::DEBUG_WIREFRAME_DECALS) == 0) {
                NiPointer<NiTexture> spTempTexture(static_cast<NiTexture*>(nullptr));
                pTextureSet->GetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, spTempTexture);
                pnolightingProp->SetBaseMap(spTempTexture.m_pObject);
            }

            pnolightingProp->SetFlag(BSShaderProperty::BSSP_FLAG_DYNAMIC_DECAL, 1);
            NiGeometry* pgeom = static_cast<NiGeometry*>(spTargetNode.m_pObject);
            NiPropertyState* ppropertyState = &pgeom->m_PropertyState;
            if (ppropertyState->GetAlpha()->IsEnabled()) {
                pnolightingProp->SetFlag(BSShaderProperty::BSSP_FLAG_ALPHA_DECAL, 1);
            }
            sp3D->AttachProperty(pnolightingProp);
            if ((BGSDecalManager::iDecalDebugFlags & BGSDecalManager::DEBUG_WIREFRAME_DECALS) != 0) {
                NiGeometry* spdata = static_cast<NiGeometry*>(sp3D.m_pObject);
                NiColorA* pcolors = spdata->GetColors();
                for (int32_t i = 0; i < spdata->GetVertexCount(); ++i) {
                    NiColorA kSolidColor(1.0f, 1.0f, 0.0f, 1.0f);
                    pcolors[i] = kSolidColor;
                }
            }
            BSShaderManager::PrepareObject(sp3D.m_pObject, 0, 0);
        }
        NiFixedString* pName = bPermanent ? FixedStrings::GetPermanentDecal() : FixedStrings::GetDecal();
        sp3D->SetName(*pName);
        TESHavokUtilities::RemoveHavokFromSceneGraph(sp3D.m_pObject);
        BSShaderManager::SetObjectTypeRecurse(sp3D.m_pObject, BSShaderManager::BSSM_OBJ_MISC);
        if (!bPermanent) {
            if (bForceFade) {
                ReferenceProcessLists.AddTempEffect(this);
            }
            else {
                BSTempEffect::kDecals.AddTail(NiPointer<NiRefObject>(this));
                if (++BSTempEffect::iNumDecals > BSTempEffectSimpleDecal::iCurrentMaxDecalCount) {
                    ReferenceProcessLists.AddTempEffect(
                        static_cast<BSTempEffect*>(BSTempEffect::kDecals.m_item.m_pObject)
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
        NiNode* pnode = spTargetNode->m_pkParent;
        BGSDecalNode* pdecalnode = nullptr;
        bool bfoundnode = false;
        if (pnode) {
            for (int32_t i = 0; i < pnode->GetArrayCount(); ++i) {
                NiNode* pchild = static_cast<NiNode*>(pnode->GetAt(static_cast<uint32_t>(i)));
                if (pchild && NiObject::IsExactKindOf(&BGSDecalNode::ms_RTTI, pchild)) {
                    bfoundnode = true;
                    pdecalnode = static_cast<BGSDecalNode*>(pchild);
                    break;
                }
            }
            if (bfoundnode) {
                pParentNode = pdecalnode;
            }
            else {
                pParentNode = new BGSDecalNode(spTargetNode->m_pkParent, false);
            }
            pParentNode->AttachChild(this, true);
            sp3D->SetTranslate(spTargetNode->GetTranslate());
            sp3D->SetRotate(spTargetNode->m_kLocal.m_Rotate);
            sp3D->SetScale(spTargetNode->m_kLocal.m_fScale);
            sp3D->UpdateProperties();
            NiUpdateData kUpdateData(fCellAppTime, false, false);
            sp3D->Update(kUpdateData);
            NiAVObject* p3D = Get3D();
            BSShaderManager::GetShadowSceneNode(0)->UpdateObjectLighting(p3D, false);
            pParentNode->pDecalList->AddHead(NiPointer<NiRefObject>(this));
            CleanUpMemory();
            bFinalize = false;
            bValidDecal = true;
            bFinished = true;
            return true;
        }

        bFinished = true;
        return false;
    }

    bFinished = true;
    return false;
}

NiTriShapeData* BSTempEffectSimpleDecal::CreateTriData() {
    // Both counts are passed truncated to their low 16 bits.
    return new NiTriShapeData(
        static_cast<uint16_t>(iDecalVertexCount),
        pFinalVertexArray,
        pFinalNormalArray,
        pFinalColorArray,
        pFinalTexcoordArray,
        1,
        NiGeometryData::NBT_METHOD_NONE,
        static_cast<uint16_t>(iDecalTriangleCount),
        pFinalIndexArray
    );
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
            int32_t tricount = ptristrips->GetActiveTriangleCount();
            uint16_t* pindexlist = pgeomdata->m_pusStripLists;
            for (int32_t i = 0; i < tricount; ++i) {
                if (pindexlist[i] != pindexlist[i + 1] &&
                    pindexlist[i] != pindexlist[i + 2] &&
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
                    const NiPoint3* pVert1 = &pvertices[i1];
                    const NiPoint3* pVert2 = &pvertices[i2];
                    const NiPoint3* pVert3 = &pvertices[i3];
                    NiPoint3 vec1 = *pVert2 - *pVert1;
                    NiPoint3 vec2 = *pVert3 - *pVert1;
                    NiPoint3 cross = vec1.Cross(vec2);
                    double dDot = pDecalNormal.Dot(cross);
                    if (cross.Length() * BSTempEffectSimpleDecal::fDecalEpsilon < dDot) {
                        int32_t newIndex[9];
                        newIndex[0] = i1;
                        newIndex[1] = i2;
                        newIndex[2] = i3;
                        newVertex[0] = *pVert1;
                        newVertex[1] = *pVert2;
                        newVertex[2] = *pVert3;
                        newNormal[0] = pnormals[i1];
                        newNormal[1] = pnormals[i2];
                        newNormal[2] = pnormals[i3];
                        int32_t count = ClipPolygon(3, newIndex, newVertex, newNormal, newIndex, newVertex, newNormal);
                        if (count && !AddPolygon(count, newIndex, newVertex, newNormal)) {
                            break;
                        }
                    }
                }
            }
        }
    }
    else {
        Warning("TEXTURES: Decal Parent '%s' has no normals!", apAVObj->m_kName.GetHandleString());
    }
}

void BSTempEffectSimpleDecal::CreateDebugBox(NiColorA& arColor, bool abWireframe) {
    NiMatrix3 kinvobjrot;
    NiMatrix3 kobjrot;
    if (!BGSDecalManager::spDebugDecalBox.m_pObject) {
        BSStream stream;
        if (stream.BSStream::Load("Data\\Meshes\\Decals\\DebugDecalBox.NIF", nullptr)) {
            if (stream.GetObjectCount() == 1) {
                BGSDecalManager::spDebugDecalBox = static_cast<NiAVObject*>(stream.GetObjectAt(0));
            }
            NiFixedString kName("Debug Decal Box");
            BGSDecalManager::spDebugDecalBox->SetName(kName);
        }
    }
    if (BGSDecalManager::spDebugDecalBox.m_pObject) {
        BSShaderNoLightingProperty* pshaderprop;
        NiAlphaProperty* palpha;
        NiMatrix3 kzrot;
        NiPointer<NiObject> spobj(static_cast<NiObject*>(nullptr));
        BGSDecalManager::spDebugDecalBox->CreateDeepCopy(spobj);
        NiNode* ptmp = static_cast<NiNode*>(spobj.m_pObject);
        NiAVObject* pavobj = ptmp->GetObjectByName("DebugDecalBox01:0");
        NiGeometry* spdata = static_cast<NiGeometry*>(pavobj);
        TESHavokUtilities::RemoveHavokFromSceneGraph(ptmp);
        NiPoint3* pverts = spdata->GetVertices();
        NiColorA* pcolors = spdata->GetColors();
        for (int32_t i = 0; i < spdata->GetVertexCount(); ++i) {
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
        if (decalnormal == NiPoint3::ZERO) {
            decalnormal = pVector;
            if (bModelSpace) {
                kobjrot = spTargetNode->m_kLocal.m_Rotate;
            }
            else {
                kobjrot = spTargetNode->GetWorldTransform().m_Rotate;
            }
            kobjrot.Inverse(&kinvobjrot);
            decalnormal = kinvobjrot * decalnormal;
        }
        NiMatrix3 kzvrot = BSUtilities::GetRotationZToVectorTranspose(decalnormal);
        kzrot.MakeZRotation(-fRotation);
        if (bModelSpace) {
            ptmp->SetTranslate(spTargetNode->GetTranslate());
            ptmp->SetRotate(spTargetNode->m_kLocal.m_Rotate);
            ptmp->SetScale(spTargetNode->m_kLocal.m_fScale);
            NiMatrix3 kRot = kzvrot * kzrot;
            pavobj->SetRotate(kRot);
            pavobj->SetTranslate(pDecalCenter);
        }
        else {
            NiMatrix3 kRot = spTargetNode->GetWorldTransform().m_Rotate * kzvrot * kzrot;
            pavobj->SetRotate(kRot);
            pavobj->SetTranslate(pOrigin);
        }
        ptmp->DetachAllProperties();
        palpha = new NiAlphaProperty;
        palpha->SetAlphaBlending(true);
        ptmp->AttachProperty(palpha);
        pshaderprop = new BSShaderNoLightingProperty;
        if (abWireframe) {
            pshaderprop->SetFlag(BSShaderProperty::BSSP_FLAG_WIREFRAME, 1);
        }
        pavobj->AttachProperty(pshaderprop);
        BSShaderManager::PrepareObject(pavobj, 0, 0);
        if (bModelSpace) {
            BSTempNode* pn;
            pn = new BSTempNode(30.0f);
            pn->AttachChild(ptmp, 1);
            spTargetNode->m_pkParent->AttachChild(pn, 1);
        }
        else {
            TES::pInstance->AddTempDebugObject(ptmp, 30.0f);
        }
        ptmp->m_pkParent->UpdateWorldBound();
        NiUpdateData kUpdateData(0.0f, false, false);
        ptmp->Update(kUpdateData);
    }
}

void BSTempEffectSimpleDecal::ResizeDataArrays() {
    ScrapHeap* pScrapHeap = MemoryManager::Instance().GetThreadScrapHeap();
    pFinalVertexArray = new NiPoint3[this->iDecalVertexCount];
    ourMemcpy(pFinalVertexArray, pTempVertexArray, sizeof(NiPoint3) * this->iDecalVertexCount);
    pScrapHeap->Deallocate(pTempVertexArray);
    pTempVertexArray = nullptr;
    pFinalNormalArray = new NiPoint3[this->iDecalVertexCount];
    ourMemcpy(pFinalNormalArray, pTempNormalArray, sizeof(NiPoint3) * this->iDecalVertexCount);
    pScrapHeap->Deallocate(pTempNormalArray);
    pTempNormalArray = nullptr;
    pFinalTexcoordArray = new NiPoint2[this->iDecalVertexCount];
    ourMemcpy(pFinalTexcoordArray, pTempTexcoordArray, sizeof(NiPoint2) * this->iDecalVertexCount);
    pScrapHeap->Deallocate(pTempTexcoordArray);
    pTempTexcoordArray = nullptr;
    pFinalColorArray = new NiColorA[static_cast<uint32_t>(this->iDecalVertexCount)];
    ourMemcpy(pFinalColorArray, pTempColorArray, sizeof(NiColorA) * this->iDecalVertexCount);
    pScrapHeap->Deallocate(pTempColorArray);
    pTempColorArray = nullptr;
    pFinalIndexArray = static_cast<uint16_t*>(_NiMalloc(3 * sizeof(uint16_t) * iDecalTriangleCount));
    ourMemcpy(pFinalIndexArray, pTempIndexArray, 3 * sizeof(uint16_t) * iDecalTriangleCount);
    pScrapHeap->Deallocate(pTempIndexArray);
    pTempIndexArray = nullptr;
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
    uint8_t aucNegative[12];
    int32_t iNegativeCount = 0;
    for (int32_t i = 0; i < aiVertexCount; ++i) {
        if (apPlane->WhichSide(apInVertex[i]) == 2) {
            aucNegative[i] = 1;
            ++iNegativeCount;
        }
        else {
            aucNegative[i] = 0;
        }
    }
    if (iNegativeCount == aiVertexCount) {
        return 0;
    }
    int32_t iOutCount = 0;
    for (int32_t j = 0; j < aiVertexCount; ++j) {
        int32_t iPrev = j ? j - 1 : aiVertexCount - 1;
        if (aucNegative[j]) {
            if (!aucNegative[iPrev]) {
                NiPoint3 kPrev = apInVertex[iPrev];
                NiPoint3 kCurr = apInVertex[j];
                NiPoint3 kEdge = kPrev - kCurr;
                NiPoint3 kNormal = apPlane->m_kNormal;
                double dDist = kNormal.Dot(kPrev);
                // The constant is negated and then added, so a NaN constant reaches fScalar with its sign flipped.
                double dOffset = dDist + -apPlane->GetConstant();
                float fScalar = static_cast<float>(dOffset / kNormal.Dot(kEdge));
                if (fScalar > 1.0 && fScalar <= 1.000100016593933) {
                    fScalar = 1.0f;
                }
                if (fScalar < 0.0) {
                    Warning(
                        "BSTempEffectSimpleDecal: Bad t value (%f) calculated while attempting to clip decal.  "
                        "See Jeff S.",
                        fScalar
                    );
                    fScalar = 0.0f;
                }
                else if (fScalar > 1.0) {
                    Warning(
                        "BSTempEffectSimpleDecal: Bad t value (%f) calculated while attempting to clip decal.  "
                        "See Jeff S.",
                        fScalar
                    );
                    fScalar = 1.0f;
                }
                apOutIndex[iOutCount] = -1;
                NiPoint3 kCurrPart = kCurr * fScalar;
                float fPrevWeight = static_cast<float>(1.0 - fScalar);
                NiPoint3 kPrevPart = kPrev * fPrevWeight;
                NiPoint3 kVertex = kPrevPart + kCurrPart;
                apOutVertex[iOutCount] = kVertex;
                NiPoint3 kPrevNormal = apInNormal[iPrev];
                NiPoint3 kCurrNormal = apInNormal[j];
                NiPoint3 kPrevNormalPart = kPrevNormal * fScalar;
                float fCurrWeight = static_cast<float>(1.0 - fScalar);
                NiPoint3 kCurrNormalPart = kCurrNormal * fCurrWeight;
                NiPoint3 kOutNormal = kCurrNormalPart + kPrevNormalPart;
                apOutNormal[iOutCount++] = kOutNormal;
            }
        }
        else {
            if (aucNegative[iPrev]) {
                NiPoint3 kCurr = apInVertex[j];
                NiPoint3 kPrev = apInVertex[iPrev];
                NiPoint3 kEdge = kCurr - kPrev;
                NiPoint3 kNormal = apPlane->m_kNormal;
                double dDist = kNormal.Dot(kCurr);
                double dOffset = dDist + -apPlane->GetConstant();
                float fScalar = static_cast<float>(dOffset / kNormal.Dot(kEdge));
                if (fScalar < 0.0) {
                    Warning(
                        "BSTempEffectSimpleDecal: Bad t value (%f) calculated while attempting to clip decal.  "
                        "See Jeff S.",
                        fScalar
                    );
                    fScalar = 0.0f;
                }
                else if (fScalar > 1.0) {
                    Warning(
                        "BSTempEffectSimpleDecal: Bad t value (%f) calculated while attempting to clip decal.  "
                        "See Jeff S.",
                        fScalar
                    );
                    fScalar = 1.0f;
                }
                apOutIndex[iOutCount] = -1;
                NiPoint3 kPrevPart = kPrev * fScalar;
                float fCurrWeight = static_cast<float>(1.0 - fScalar);
                NiPoint3 kCurrPart = kCurr * fCurrWeight;
                NiPoint3 kVertex = kCurrPart + kPrevPart;
                apOutVertex[iOutCount] = kVertex;
                NiPoint3 kCurrNormal = apInNormal[j];
                NiPoint3 kPrevNormal = apInNormal[iPrev];
                NiPoint3 kCurrNormalPart = kCurrNormal * fScalar;
                float fPrevWeight = static_cast<float>(1.0 - fScalar);
                NiPoint3 kPrevNormalPart = kPrevNormal * fPrevWeight;
                NiPoint3 kOutNormal = kPrevNormalPart + kCurrNormalPart;
                apOutNormal[iOutCount++] = kOutNormal;
            }
            apOutIndex[iOutCount] = apInIndex[j];
            apOutVertex[iOutCount] = apInVertex[j];
            apOutNormal[iOutCount] = apInNormal[j];
            ++iOutCount;
        }
    }
    return iOutCount;
}

void BSTempEffectSimpleDecal::PreloadDecalData() {
    if (!BSTempEffectSimpleDecal::spPreloadedDecalTextureSets.m_pObject) {
        AutoMemContext kMemContext(
            MEM_CONTEXT_EFFECTS_OVERHEAD,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectSimpleDecal.cpp",
            287
        );
        QueuedFile* pQueuedFile = new QueuedFile(IO_TASK_PRIORITY_VERY_LOW);
        BSTempEffectSimpleDecal::spPreloadedDecalTextureSets = pQueuedFile;
    }
}

void BSTempEffectSimpleDecal::UnloadDecalData() {
    BSTempEffectSimpleDecal::spPreloadedDecalTextureSets = static_cast<QueuedFile*>(nullptr);
}

void NiAVObject::DetachAllProperties() {
    m_kPropertyList.RemoveAll();
}
