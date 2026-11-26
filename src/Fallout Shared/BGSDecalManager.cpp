#include "Fallout Shared/BGSDecalManager.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldM.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSOcclusionQuery.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/RendererSettingCollection.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/BGSDecalEmitter.h"
#include "Fallout Shared/BGSDecalNode.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSImpactData.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/DecalData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/DecalCaster.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout Shared/TempEffects/BSTempEffectGeometryDecal.h"
#include "Fallout Shared/TempEffects/BSTempEffectParticle.h"
#include "Fallout Shared/TempEffects/BSTempEffectSimpleDecal.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/MenuConsole.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVisibleArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpClosestRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "SDK/MSVC/string.h"

#include <cmath>

namespace bhkConvert {} // namespace bhkConvert

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> BGSDecalManager::fDecalLifetime("fDecalLifetime:Display", 10.0f);
SettingT<RendererSettingCollection> BGSDecalManager::iMaxDecalsPerFrame("iMaxDecalsPerFrame:Display", 10);
SettingT<RendererSettingCollection> BGSDecalManager::iMaxSkinDecalsPerFrame("iMaxSkinDecalsPerFrame:Display", 3);
SettingT<RendererSettingCollection> BGSDecalManager::iMaxDecals("uMaxDecals:Decals", 100);
SettingT<INISettingCollection> BGSDecalManager::iMaxSkinDecals("uMaxSkinDecals:Decals", 24u);
SettingT<INISettingCollection> BGSDecalManager::iMaxSkinDecalPerActor("uMaxSkinDecalPerActor:Decals", 3u);
SettingT<INISettingCollection> BGSDecalManager::bProfileDecals("bProfileDecals:Decals", false);
SettingT<INISettingCollection> BGSDecalManager::bDebugDecals("bDebugDecals:Decals", false);
SettingT<INISettingCollection> BGSDecalManager::bDecalOcclusionQuery("bDecalOcclusionQuery:Decals", true);
SettingT<INISettingCollection> BGSDecalManager::bDecalMultithreaded("bDecalMultithreaded:Decals", false);
SettingT<INISettingCollection> BGSDecalManager::bDecals("bDecals:Decals", true);
SettingT<INISettingCollection> BGSDecalManager::bSkinnedDecals("bSkinnedDecals:Decals", true);
SettingT<GameSettingCollection> fSprayDecalsDistance("fSprayDecalsDistance", 64.0f);
SettingT<GameSettingCollection> fSprayDecalsGravity("fSprayDecalsGravity", 128.0f);
SettingT<GameSettingCollection> iSprayDecalsDebug("iSprayDecalsDebug", 0);
NiPointer<NiAVObject> BGSDecalManager::spDebugDecalBox;

BGSDecalManager::BGSDecalManager()
  : spQueryTexture(nullptr), PendingSimpleDecalList(), DecalEmitterList(), spQueryAccum(nullptr),
    spQueryCamera(nullptr) {
    spQueryAccum = new BSShaderAccumulator(99, 1u, 759u);
    spQueryCamera = new NiCamera();
}

BGSDecalManager::~BGSDecalManager() {
    DecalCaster::ClearInstance();
    BGSDecalManager::spDebugDecalBox = nullptr;
    PendingSimpleDecalList.RemoveAll();
    DecalEmitterList.RemoveAll();
    spQueryAccum = nullptr;
    spQueryCamera = nullptr;
}

void BGSDecalManager::UpdateSimpleDecals() {
    NiTListItem<NiPointer<BSTempEffectSimpleDecal>>* prev;
    NiTListItem<NiPointer<BSTempEffectSimpleDecal>>* iter;
    if (PendingSimpleDecalList.m_uiCount) {
        NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pRenderer->LockRenderer();
        iter = PendingSimpleDecalList.m_pkHead;
        while (iter) {
            prev = iter;
            BSTempEffectSimpleDecal* pdecal = PendingSimpleDecalList.GetNext(iter).m_pObject;
            if (pdecal) {
                if (pdecal->bOcclusionQuery) {
                    IssueDecalOcclusionQuery(pdecal);
                    pdecal->bOcclusionQuery = false;
                }
                if (!pdecal->pOcclusionQuery) {
                    pdecal->Initialize();
                }
                else if (pdecal->CheckOcclusionQueryResults()) {
                    if (pdecal->iCurrentOcclusionResult) {
                        if (BGSDecalManager::bDebugDecals.Bool()) {
                            NiAVObject* pDecalParent = pdecal->GetDecalParent();
                            Warning(
                                "DECAL: Decal Succeeded Occlusion Query : %s",
                                pDecalParent->m_kName.GetHandleString()
                            );
                        }
                        pdecal->Initialize();
                    }
                    else {
                        if (BGSDecalManager::bDebugDecals.Bool()) {
                            NiAVObject* pDecalParent = pdecal->GetDecalParent();
                            Warning(
                                "DECAL: Decal Failed Occlusion Query : %s",
                                pDecalParent->m_kName.GetHandleString()
                            );
                        }
                        pdecal->bFinished = true;
                    }
                }
                if (pdecal->bFinalize && !pdecal->bFinished) {
                    pdecal->FinalizeGeometry();
                }
                if (pdecal->bFinished) {
                    if (pdecal->GetIsValid()) {
                        ++BGSDecalManager::iDecalCount;
                        if (BGSDecalManager::bDebugDecals.Bool()) {
                            NiAVObject* pDecalParent = pdecal->GetDecalParent();
                            Warning(
                                "DECAL: Placing Simple Decal #%d : %s",
                                BGSDecalManager::iDecalCount,
                                pDecalParent->m_kName.GetHandleString()
                            );
                        }
                        if ((BGSDecalManager::iDecalDebugFlags & BGSDecalManager::DEBUG_WIREFRAME_CLIPPING_PLANES) !=
                            0) {
                            NiColorA kColor(0.0f, 1.0f, 0.0f, 1.0f);
                            pdecal->CreateDebugBox(kColor, true);
                        }
                        if ((BGSDecalManager::iDecalDebugFlags & BGSDecalManager::DEBUG_CLIPPING_PLANES) != 0) {
                            NiColorA kColor(0.0f, 1.0f, 0.0f, 0.25f);
                            pdecal->CreateDebugBox(kColor, false);
                        }
                    }
                    else if ((BGSDecalManager::iDecalDebugFlags & BGSDecalManager::DEBUG_FAILED_DECALS) != 0) {
                        if ((BGSDecalManager::iDecalDebugFlags & BGSDecalManager::DEBUG_WIREFRAME_CLIPPING_PLANES) !=
                            0) {
                            NiColorA kColor(1.0f, 0.0f, 0.0f, 1.0f);
                            pdecal->CreateDebugBox(kColor, true);
                        }
                        if ((BGSDecalManager::iDecalDebugFlags & BGSDecalManager::DEBUG_CLIPPING_PLANES) != 0) {
                            NiColorA kColor(1.0f, 0.0f, 0.0f, 0.25f);
                            pdecal->CreateDebugBox(kColor, false);
                        }
                    }
                    PendingSimpleDecalList.RemovePos(prev);
                }
            }
        }
        static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->UnlockRenderer();
    }
}

void BGSDecalManager::UpdateDecalEmitters() {
    NiTListItem<BGSDecalEmitter*>* pos = DecalEmitterList.m_pkHead;
    while (pos) {
        NiTListItem<BGSDecalEmitter*>* prev = pos;
        BGSDecalEmitter* pemitter = DecalEmitterList.GetNext(pos);
        if (pemitter) {
            pemitter->Update();
            if (pemitter->bFinished) {
                DecalEmitterList.RemovePos(prev);
                delete pemitter;
            }
        }
    }
}

void BGSDecalManager::UpdateDecals() {
    BGSDecalManager::ClearDecalsThisFrame();
    UpdateSimpleDecals();
    UpdateDecalEmitters();
}

void BGSDecalManager::AddDecal(
    DECAL_CREATION_DATA& arDecalData,
    BGSDecalManager::DECAL_TYPE aeDecalType,
    bool abForceAdd
) {
    NiAVObject* pAVObj;
    NiTPointerList<NiAVObject*> listavobj;

    char acBuffer[260];
    _LARGE_INTEGER End;
    _LARGE_INTEGER TicksPerSecond;
    _LARGE_INTEGER Start;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_EFFECTS_OVERHEAD,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BGSDecalManager.cpp",
        440
    );
    bClearQueryTexture = true;
    if (aeDecalType == BGSDecalManager::DECAL_TYPE_SIMPLE) {
        if (BGSDecalManager::bDecals.Bool() && arDecalData.pAVObj && !arDecalData.pAVObj->GetNoDecals()) {
            NiPoint3 kEyePos = pPlayer->GetLocationOnReference();
            float fDist = (arDecalData.pOrigin - kEyePos).Length();
            double fTicksPerMicrosecond = 1000.0;
            Start.u.LowPart = 0;
            if (BGSDecalManager::bProfileDecals.Bool()) {
                QueryPerformanceFrequency(&TicksPerSecond);
                fTicksPerMicrosecond = static_cast<double>(TicksPerSecond.QuadPart) * 1e-06;
                QueryPerformanceCounter(&Start);
            }
            if (static_cast<double>(fDist) <= Renderer::GetDecalLODStart() || arDecalData.bModelSpace || abForceAdd) {
                if (BGSDecalManager::bDebugDecals.Bool()) {
                    Warning("DECAL: Simple Decal Instantiated");
                }
                AddGeometryDecalRecurse(arDecalData);
            }
            if (BGSDecalManager::bProfileDecals.Bool()) {
                QueryPerformanceCounter(&End);
                int64_t iTicks = End.QuadPart - Start.QuadPart;
                BSsprintf(
                    acBuffer,
                    255,
                    "AddGeometryDecalRecurse: Ticks: %lld, Microseconds: %f",
                    iTicks,
                    static_cast<double>(iTicks) / fTicksPerMicrosecond
                );
                InitializationFlow(acBuffer, 0);
            }
        }
    }
    else if (aeDecalType == BGSDecalManager::DECAL_TYPE_PROJECTED) {
        if (BGSDecalManager::bSkinnedDecals.Bool() && arDecalData.pAVObj && !arDecalData.pAVObj->GetNoDecals()) {
            NiPoint3 kEyePos = pPlayer->GetLocationOnReference();
            NiBound kBound = arDecalData.pAVObj->GetWorldBound();
            double dDistance = (kBound.m_kCenter - kEyePos).Length();
            float fDist = dDistance - kBound.m_fRadius;
            if ((static_cast<double>(fDist) <= Renderer::GetSkinnedDecalLODStart() &&
                 BGSDecalManager::iSkinnedDecalCount < BGSDecalManager::iMaxSkinDecals.UInt()) ||
                abForceAdd) {
                AddProjectedDecalRecurse(arDecalData);
            }
            else if (
                BGSDecalManager::iSkinnedDecalCount >= BGSDecalManager::iMaxSkinDecals.UInt() &&
                BGSDecalManager::bDebugDecals.Bool()
            ) {
                Warning("DECAL: Reached Max Skin Decal Limit! : %d", BGSDecalManager::iMaxSkinDecals.Int());
            }
        }
        else if (BGSDecalManager::bSkinnedDecals.Bool() && arDecalData.pObjRefr) {
            NiPoint3 kEyePos = pPlayer->GetLocationOnReference();
            NiBound kBound = arDecalData.pObjRefr->Get3D()->GetWorldBound();
            double dDistance = (kBound.m_kCenter - kEyePos).Length();
            float fDist = dDistance - kBound.m_fRadius;
            NiAVObject* pAVObject = arDecalData.pObjRefr->Get3D();
            NiNode* pNode = pAVObject ? pAVObject->IsNode() : nullptr;
            if (pNode && !pAVObject->GetNoDecals()) {
                arDecalData.pAVObj = pNode;
                if ((static_cast<double>(fDist) <= Renderer::GetSkinnedDecalLODStart() &&
                     BGSDecalManager::iSkinnedDecalCount < BGSDecalManager::iMaxSkinDecals.UInt()) ||
                    abForceAdd) {
                    AddProjectedDecalRecurse(arDecalData);
                }
                else if (
                    BGSDecalManager::iSkinnedDecalCount >= BGSDecalManager::iMaxSkinDecals.UInt() &&
                    BGSDecalManager::bDebugDecals.Bool()
                ) {
                    Warning("DECAL: Reached Max Skin Decal Limit! : %d", BGSDecalManager::iMaxSkinDecals.Int());
                }
            }
        }
        else if (BGSDecalManager::bDecals.Bool()) {
            arDecalData.pDirection.Unitize();
            if (!BGSDecalManager::pDecalCaster) {
                float fSize = BSMAX(arDecalData.fDecalHeight, arDecalData.fDecalWidth);
                BGSDecalManager::pDecalCaster = DecalCaster::GetInstance(fSize, true);
            }
            BGSDecalManager::pDecalCaster->SetCollision(L_BIPED, false);
            BGSDecalManager::pDecalCaster->SetCollision(L_DEADBIP, false);
            bhkWorld* pWorld = arDecalData.pParentCell->GetbhkWorld();
            BGSDecalManager::pDecalCaster->Add(pWorld);
            int32_t iCount = BGSDecalManager::pDecalCaster->Pick(
                arDecalData.pOrigin,
                arDecalData.pOrigin + arDecalData.pDirection * 80.0f,
                32.0f
            );
            if (!iCount) {
                NiPoint3 kDir = arDecalData.pDirection + -NiPoint3::UNIT_Z * 2.0f;
                kDir.Unitize();
                arDecalData.pDirection = kDir;
                iCount = BGSDecalManager::pDecalCaster
                             ->Pick(arDecalData.pOrigin, arDecalData.pOrigin + kDir * 80.0f, 32.0f);
            }
            ::new (&listavobj) NiTPointerList<BSAnimGroupSequence*>;
            for (int32_t i = 0; i < iCount; ++i) {
                pAVObj = BGSDecalManager::pDecalCaster->GetAVObject(static_cast<uint32_t>(i));
                if (pAVObj) {
                    NiPoint3 kHavokPos;
                    NiPoint3 kHavokNormal;
                    BGSDecalManager::pDecalCaster->GetCollisionData(static_cast<uint32_t>(i), kHavokPos, kHavokNormal);
                    kHavokNormal.Unitize();
                    TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pAVObj);
                    if (!pAVObj->GetNoDecals() &&
                        pRef &&
                        BGSDecalManager::IsValidDecalImpact(pRef, BGSDecalManager::DECAL_TYPE_SIMPLE) &&
                        !listavobj.FindPos(pAVObj, nullptr)) {
                        arDecalData.pOrigin = kHavokPos;
                        arDecalData.pDirection = kHavokNormal;
                        arDecalData.pAVObj = pAVObj;
                        AddGeometryDecalRecurse(arDecalData);
                        listavobj.AddHead(pAVObj);
                    }
                }
            }
            listavobj.RemoveAll();
        }
    }
    else if (
        aeDecalType == BGSDecalManager::DECAL_TYPE_PERMENANT_SIMPLE &&
        BGSDecalManager::bDecals.Bool() &&
        arDecalData.pAVObj &&
        !arDecalData.pAVObj->GetNoDecals()
    ) {
        arDecalData.bPermanent = true;
        AddGeometryDecalRecurse(arDecalData);
    }
}

bool BGSDecalManager::AddGeometryDecalRecurse(DECAL_CREATION_DATA& arDecalData) {
    if (BGSDecalManager::iDecalsThisFrame < BGSDecalManager::iMaxDecalsPerFrame.Int()) {
        if (arDecalData.pAVObj) {
            const char* pName = arDecalData.pAVObj->m_kName.GetHandleString();
            if (pName &&
                (!std::strncmp(pName, "Decal Node", 10u) ||
                 !std::strncmp(pName, pcString, 5u) ||
                 !std::strncmp(pName, "FaceGen", 7u) ||
                 !std::strncmp(pName, "BSFaceGen", 9u) ||
                 !std::strncmp(pName, "Bip01", 5u) ||
                 !std::strncmp(pName, "Debug Decal Box", 15u))) {
                return true;
            }
            if (!NiObject::IsKindOf_0(&NiTriStrips::ms_RTTI, arDecalData.pAVObj) ||
                arDecalData.pAVObj->GetAppCulled()) {
                if (arDecalData.pAVObj->IsNode()) {
                    NiNode* pNode = static_cast<NiNode*>(arDecalData.pAVObj);
                    if (pNode && pNode->GetAppCulled()) {
                        return true;
                    }
                    for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                        NiAVObject* pChild = pNode->GetAt(i);
                        if (pChild) {
                            arDecalData.pAVObj = pChild;
                            AddGeometryDecalRecurse(arDecalData);
                        }
                    }
                }
            }
            else {
                BSTempEffectSimpleDecal* pdecal;
                NiGeometry* pgeom = static_cast<NiGeometry*>(arDecalData.pAVObj);
                if (pgeom) {
                    BSShaderPPLightingProperty* pshade = static_cast<BSShaderPPLightingProperty*>(
                        pgeom->m_PropertyState.GetShade()
                    );
                    if (pshade && pshade->GetFlag(BSShaderProperty::BSSP_FLAG_DECAL)) {
                        return true;
                    }
                }
                NiNode* pnode = arDecalData.pAVObj->m_pkParent;
                BGSDecalNode* pdecalnode = nullptr;
                bool bfoundnode = false;
                if (!pnode) {
                    return false;
                }
                for (int32_t i = 0; i < pnode->GetArrayCount(); ++i) {
                    NiNode* pchild = static_cast<NiNode*>(pnode->GetAt(i));
                    if (pchild && NiObject::IsExactKindOf(&BGSDecalNode::ms_RTTI, pchild)) {
                        bfoundnode = true;
                        pdecalnode = static_cast<BGSDecalNode*>(pchild);
                        break;
                    }
                }
                if (bfoundnode && arDecalData.fPlacementRadius > 0.0) {
                    // The list holds NiPointer<NiRefObject>; its items are BSTempEffects.
                    BGSDecalNode::DecalList* plist = pdecalnode->pDecalList;
                    while (plist && !plist->IsEmpty()) {
                        float decalsize;
                        BSTempEffect* ptempeffect = static_cast<BSTempEffect*>(plist->m_item.m_pObject);
                        BSTempEffectSimpleDecal* pdata = static_cast<BSTempEffectSimpleDecal*>(ptempeffect);
                        plist = plist->m_pkNext;
                        NiPoint3 decalpos = pdata->GetOrigin();
                        float radius = pdata->fWidth;
                        NiPoint3 deltaSquared = arDecalData.pOrigin - decalpos;
                        deltaSquared = NiPoint3::ComponentProduct(deltaSquared, deltaSquared);
                        if (!(arDecalData.fDecalHeight < static_cast<double>(arDecalData.fDecalWidth))) {
                            decalsize = arDecalData.fDecalHeight;
                        }
                        else {
                            decalsize = arDecalData.fDecalWidth;
                        }
                        double sumRadiiSquared = decalsize + radius;
                        sumRadiiSquared = sumRadiiSquared * sumRadiiSquared;
                        if (sumRadiiSquared / arDecalData.fPlacementRadius >=
                            deltaSquared.x + deltaSquared.y + deltaSquared.z) {
                            return false;
                        }
                    }
                }
                ++BGSDecalManager::iDecalsThisFrame;
                if (BGSDecalManager::bDebugDecals.Bool()) {
                    Warning("DECAL: Non-Skinned decal count this frame: #%d", BGSDecalManager::iDecalsThisFrame);
                }
                pdecal = new BSTempEffectSimpleDecal(pdecalnode, BGSDecalManager::fDecalLifetime, arDecalData);
                NiTriStrips* ptristrips = static_cast<NiTriStrips*>(arDecalData.pAVObj);
                if (ptristrips->GetActiveTriangleCount() > 1000u &&
                    BGSDecalManager::bDecalOcclusionQuery.Bool() &&
                    arDecalData.bUseOcclusionQuery) {
                    pdecal->bOcclusionQuery = true;
                    pdecal->bClearQueryTexture = bClearQueryTexture;
                    bClearQueryTexture = false;
                }
                if (arDecalData.pParentCell) {
                    NiPointer<BSTempEffectSimpleDecal> spdecal(pdecal);
                    PendingSimpleDecalList.AddTail(spdecal);
                }
            }
        }
        return true;
    }

    if (BGSDecalManager::bDebugDecals.Bool()) {
        Warning("DECAL: Reached max non-skinned decals per frame. Ignoring decal.");
    }
    return false;
}

bool BGSDecalManager::AddProjectedDecalRecurse(DECAL_CREATION_DATA& arDecalData) {
    // The setting is a float compared with the double 0.0: NaN goes on.
    if (fDecalLifetime.Float() <= 0.0) {
        return false;
    }
    // The compare is signed.
    if (iSkinnedDecalsThisFrame >= static_cast<int32_t>(iMaxSkinDecalsPerFrame.UInt())) {
        if (bDebugDecals.Bool()) {
            Warning("DECAL: Reached max skinned decals per frame. Ignoring decal.");
        }
        return true;
    }
    NiPointer<NiSourceTexture> spDiffuseTexture(nullptr);
    if (arDecalData.pTexSet) {
        NiPointer<NiTexture> spTexture(nullptr);
        BSTextureSet* pTextureSet = arDecalData.pTexSet;
        pTextureSet->GetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, spTexture);
        NiTexture* pTexture = spTexture;
        if (NiObject::IsKindOf_0(&NiSourceTexture::ms_RTTI, pTexture)) {
            spDiffuseTexture = static_cast<NiSourceTexture*>(pTexture);
        }
    }
    if (!spDiffuseTexture) {
        return false;
    }
    if (!arDecalData.pAVObj) {
        return true;
    }
    const char* pName = arDecalData.pAVObj->GetName().GetHandleString();
    if (pName &&
        (!std::strncmp(pName, "Decal", 5) ||
         !std::strncmp(pName, "FaceGen", 7) ||
         !std::strncmp(pName, "Bip01", 5) ||
         !std::strncmp(pName, "BSFaceGen", 9) ||
         !std::strncmp(pName, "Hair", 4))) {
        return true;
    }
    if (arDecalData.pAVObj->GetAppCulled()) {
        return true;
    }
    if (!arDecalData.pAVObj->IsTriBasedGeom()) {
        if (!arDecalData.pAVObj->IsNode()) {
            return true;
        }
        NiNode* pNode = static_cast<NiNode*>(arDecalData.pAVObj);
        for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
            NiAVObject* pChild = pNode->GetAt(i);
            if (!pChild) {
                continue;
            }
            arDecalData.pAVObj = pChild;
            if (AddProjectedDecalRecurse(arDecalData) || !arDecalData.pAVObj) {
                continue;
            }
            if (arDecalData.pSkinnedDecalNode) {
                NiNode* pParent = arDecalData.pAVObj->GetParent();
                NiAVObject* pDecalNode = nullptr;
                for (uint32_t j = 0; j < pParent->GetArrayCount(); ++j) {
                    NiAVObject* pSibling = pParent->GetAt(j);
                    if (pSibling && NiObject::IsExactKindOf(&BGSDecalNode::ms_RTTI, pSibling)) {
                        pDecalNode = pSibling;
                        break;
                    }
                }
                if (pDecalNode) {
                    static_cast<NiNode*>(pDecalNode)->AttachChild(arDecalData.pSkinnedDecalNode, true);
                }
            }
            return false;
        }
        return true;
    }
    NiTriBasedGeom* pGeometry = static_cast<NiTriBasedGeom*>(arDecalData.pAVObj);
    // The compare is signed.
    if (arDecalData.iIndex > 0) {
        --arDecalData.iIndex;
        return true;
    }
    NiProperty* pProperty = pGeometry->GetProperty(NiProperty::SHADE);
    BSShader* pShader;
    pShader = pGeometry->GetShader();
    if (pProperty && pShader) {
        NiPointer<BSTempEffectGeometryDecal> spTempEffectDecal(nullptr);
        if (pGeometry->GetSkinInstance()) {
            NiNode* pParent = arDecalData.pAVObj->GetParent();
            BGSDecalNode* pDecalNode = nullptr;
            for (uint32_t i = 0; i < pParent->GetArrayCount(); ++i) {
                NiAVObject* pChild = pParent->GetAt(i);
                if (pChild && NiObject::IsExactKindOf(&BGSDecalNode::ms_RTTI, pChild)) {
                    pDecalNode = static_cast<BGSDecalNode*>(pChild);
                    break;
                }
            }
            if (!pDecalNode) {
                pDecalNode = new BGSDecalNode(pParent, true);
            }
            else if (pDecalNode->GetChildCount() >= iMaxSkinDecalPerActor.UInt()) {
                // The compare is unsigned.
                return true;
            }
            ++iSkinnedDecalsThisFrame;
            if (bDebugDecals.Bool()) {
                Warning("DECAL: Skinned decal count this frame: #%d", iSkinnedDecalsThisFrame);
            }
            BSTempEffectGeometryDecal* pNewDecal = new BSTempEffectGeometryDecal(
                arDecalData.pParentCell,
                fDecalLifetime,
                &arDecalData,
                pGeometry,
                arDecalData.pOrigin,
                arDecalData.pDirection,
                arDecalData.fDecalWidth,
                arDecalData.fRotation,
                arDecalData.iBodyParts
            );
            {
                NiPointer<BSTempEffectGeometryDecal> spNewDecal(pNewDecal);
                BSTempEffect* pEffect = spNewDecal;
                pEffect->Initialize();
                // The compare is signed.
                if (spNewDecal->GetNumVerts() <= 0) {
                    return true;
                }
                if (!arDecalData.pSkinnedDecalNode) {
                    NiNode* pSkinnedNode = new NiNode(0);
                    arDecalData.pSkinnedDecalNode = pSkinnedNode;
                    arDecalData.pSkinnedDecalNode->SetName(FixedStrings::QSkinnedDecalNode());
                    pDecalNode->AttachChild(arDecalData.pSkinnedDecalNode, true);
                    ++iSkinnedDecalCount;
                    if (bDebugDecals.Bool()) {
                        Warning("DECAL: Placing Skin Decal #%d", iSkinnedDecalCount);
                    }
                }
                pEffect = spNewDecal;
                NiAVObject* pDecal3D = pEffect->Get3D();
                arDecalData.pSkinnedDecalNode->AttachChild(pDecal3D, true);
                NiNode* pSkinnedDecalNode = arDecalData.pSkinnedDecalNode;
                spNewDecal->SetDecalParent(pSkinnedDecalNode);
                {
                    NiPointer<NiRefObject> spDecal(spNewDecal);
                    pDecalNode->pDecalList->AddHead(spDecal);
                }
                NiUpdateData kUpdateData(0.0f, false, false);
                pDecalNode->Update(kUpdateData);
                ReferenceProcessLists.AddTempEffect(spNewDecal);
                spTempEffectDecal = static_cast<BSTempEffectGeometryDecal*>(spNewDecal);
            }
        }
        if (arDecalData.pClone) {
            NiAVObject* pCloneGeometry = nullptr;
            for (uint32_t i = 0; i < arDecalData.pClone->GetArrayCount() && !pCloneGeometry; ++i) {
                NiAVObject* pChild = arDecalData.pClone->GetAt(i);
                if (!pChild) {
                    continue;
                }
                if (pChild->IsTriBasedGeom()) {
                    pCloneGeometry = pChild;
                }
                else if (pChild->IsNode()) {
                    NiNode* pChildNode = static_cast<NiNode*>(pChild);
                    for (uint32_t j = 0; j < pChildNode->GetArrayCount() && !pCloneGeometry; ++j) {
                        NiAVObject* pGrandChild = pChildNode->GetAt(j);
                        if (pGrandChild && pGrandChild->IsTriBasedGeom()) {
                            pCloneGeometry = pGrandChild;
                        }
                    }
                }
            }
            if (pCloneGeometry) {
                NiTriBasedGeom* pCloneGeom = static_cast<NiTriBasedGeom*>(pCloneGeometry);
                NiProperty* pCloneProperty = pCloneGeom->GetProperty(NiProperty::SHADE);
                BSShader* pCloneShader = pCloneGeom->GetShader();
                if (pCloneProperty && pCloneShader) {
                    NiShadeProperty* pShadeProperty = static_cast<NiShadeProperty*>(pCloneProperty);
                    if (BSSM_ShaderPropertyIsLighting(pShadeProperty) ? pShadeProperty : nullptr) {
                        NiPointer<BSTempEffectGeometryDecal> spCloneDecal(nullptr);
                        if (pGeometry->GetSkinInstance()) {
                            BSTempEffectGeometryDecal* pNewDecal = new BSTempEffectGeometryDecal(
                                arDecalData.pParentCell,
                                fDecalLifetime,
                                &arDecalData,
                                pCloneGeom,
                                arDecalData.pOrigin,
                                -arDecalData.pDirection,
                                1.0f,
                                arDecalData.fRotation,
                                arDecalData.iBodyParts
                            );
                            spCloneDecal = pNewDecal;
                            // Initialize is called through the raw pointer without a null check.
                            BSTempEffect* pEffect = pNewDecal;
                            pEffect->Initialize();
                        }
                        if (spCloneDecal) {
                            ReferenceProcessLists.AddTempEffect(spCloneDecal);
                        }
                    }
                }
            }
        }
    }
    // The compare is signed.
    return arDecalData.iIndex < 0;
}

void BGSDecalManager::IssueDecalOcclusionQuery(BSTempEffectSimpleDecal* apDecal) {
    NiMatrix3 kProduct;
    NiPoint3 normal;
    NiPoint3 tangent;
    NiPoint3 start;
    NiPoint3 end;
    NiPoint3 binormal;
    NiMatrix3 adjustedrot;
    NiMatrix3 rot;
    NiMatrix3 kzrot;
    normal = apDecal->GetDirection();
    normal.Unitize();
    normal *= 50.0f;
    NiPoint3 pos = apDecal->GetOrigin() + normal;
    NiPoint3 kNegNormal = -normal;
    ::new (&rot) NiMatrix3(BSUtilities::GetRotationZToVector(kNegNormal));
    float fAngle = apDecal->fRotation;
    kzrot.MakeZRotation(fAngle);
    rot = *::new (&kProduct) NiMatrix3(kzrot * rot);
    rot.GetRow(0, tangent);
    rot.GetRow(1u, binormal);
    rot.GetRow(2u, normal);
    adjustedrot.SetCol(0, normal);
    adjustedrot.SetCol(1u, tangent);
    adjustedrot.SetCol(2u, binormal);
    double dHalfWidth = apDecal->fWidth / 2.0;
    NiAVObject* pDecalParent = apDecal->GetDecalParent();
    float fwidth = 1.0 / pDecalParent->m_kWorld.m_fScale * dHalfWidth;
    double dHalfHeight = apDecal->GetDecalHeight() / 2.0;
    pDecalParent = apDecal->GetDecalParent();
    float fheight = 1.0 / pDecalParent->m_kWorld.m_fScale * dHalfHeight;
    float frustumNear = apDecal->fDepth + 50.0;
    NiFrustum frustum(false);
    frustum.m_bOrtho = true;
    frustum.m_fNear = 0.1f;
    frustum.m_fLeft = -fwidth;
    frustum.m_fRight = fwidth;
    frustum.m_fTop = fheight;
    frustum.m_fBottom = -fheight;
    frustum.m_fFar = frustumNear;
    spQueryCamera->SetTranslate(pos);
    spQueryCamera->SetRotate(adjustedrot);
    spQueryCamera->SetViewFrustum(frustum);
    NiUpdateData kUpdateData(0.0f, false, false);
    spQueryCamera->Update(kUpdateData);
    if ((BGSDecalManager::iDecalDebugFlags & BGSDecalManager::DEBUG_QUERY_CAMERA_DIRECTION) != 0) {
        start = pos;
        end = pos + spQueryCamera->GetWorldDirection() * frustumNear;
        NiLines*
            plines = MakeLine(start, NiColorA(0.0f, 0.0f, 1.0f, 1.0f), end, NiColorA(1.0f, 0.0f, 0.0f, 1.0f), true);
        TES::pInstance->AddTempDebugObject(plines, 30.0f);
    }
    if (BGSDecalManager::bDebugDecals.Bool()) {
        NiPoint3 vec = spQueryCamera->GetWorldDirection();
        pDecalParent = apDecal->GetDecalParent();
        Warning("DECAL: Issuing Occlusion Query: %s", pDecalParent->m_kName.GetHandleString());
        Warning("DECAL: Origin: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        Warning("DECAL: Direction: (%.2f, %.2f, %.2f)", vec.x, vec.y, vec.z);
        Warning(
            "DECAL: Frustum: Near=0.1f, Far=%.2f, Left=%.2f, Right=%.2f, Top=%.2f, Bottom=%.2f )",
            frustumNear,
            -fwidth,
            fwidth,
            fheight,
            -fheight
        );
    }
    if (!spQueryTexture.m_pObject) {
        NiDX9Renderer* pDX9Renderer = BSShaderManager::GetRenderer();
        BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
        spQueryTexture = pTextureManager->BorrowRenderedTexture(
            pDX9Renderer,
            BSTextureManager::TEX_DECAL_OCCLUSION,
            0,
            nullptr,
            0
        );
    }
    if (apDecal->bClearQueryTexture) {
        BSRenderedTexture::StartOffscreen(7u, spQueryTexture->GetGroup());
    }
    else {
        BSRenderedTexture::StartOffscreen(0, spQueryTexture->GetGroup());
    }
    BSOcclusionQuery* pQuery = new BSOcclusionQuery;
    apDecal->pOcclusionQuery = pQuery;
    apDecal->pOcclusionQuery->Begin();
    const NiRect<float>* pViewPort = &spQueryCamera->GetViewPort();
    NiRenderer::GetRenderer()->SetScreenSpaceCameraData(pViewPort);
    BSCullingProcess culler(nullptr);
    culler.SetAccumulator(spQueryAccum.m_pObject);
    ShadowSceneNode* pShadowSceneNode = BSShaderManager::GetShadowSceneNode(0);
    spQueryAccum->SetShadowSceneNode(pShadowSceneNode);
    spQueryAccum->SetRenderMode(BSShaderManager::BSSM_RENDER_SILHOUETTE);
    pDecalParent = apDecal->GetDecalParent();
    BSShaderUtil::AccumulateScene(spQueryCamera.m_pObject, pDecalParent, culler);
    BSShaderUtil::RenderScene(spQueryCamera.m_pObject, spQueryAccum.m_pObject, false);
    culler.SetAccumulator(nullptr);
    apDecal->pOcclusionQuery->End();
    BSRenderedTexture::StopOffscreen();
}

void BGSDecalManager::ClearPendingQueries() {
    uint32_t iresult;
    NiTListItem<NiPointer<BSTempEffectSimpleDecal>>* iter;
    if (PendingSimpleDecalList.m_uiCount) {
        NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        pRenderer->LockRenderer();
        iter = PendingSimpleDecalList.m_pkHead;
        while (iter) {
            BSTempEffectSimpleDecal* pdecal = PendingSimpleDecalList.GetNext(iter).m_pObject;
            if (pdecal && pdecal->pOcclusionQuery) {
                iresult = 0;
                pdecal->pOcclusionQuery->GetResult(&iresult, true);
                if (pdecal->pOcclusionQuery) {
                    delete pdecal->pOcclusionQuery;
                }
                pdecal->pOcclusionQuery = nullptr;
            }
        }
        static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer())->UnlockRenderer();
    }
}

BGSDecalManager* BGSDecalManager::GetSingleton() {
    if (BGSDecalManager::pSingleton) {
        return BGSDecalManager::pSingleton;
    }
    BGSDecalManager* pManager = new BGSDecalManager;
    BGSDecalManager::pSingleton = pManager;
    return BGSDecalManager::pSingleton;
}

void BGSDecalManager::ClearDecalsThisFrame() {
    BGSDecalManager::iDecalsThisFrame = 0;
    BGSDecalManager::iSkinnedDecalsThisFrame = 0;
}

bool BGSDecalManager::IsValidDecalImpact(TESObjectREFR* apCollidee, BGSDecalManager::DECAL_TYPE aeDecalType) {
    bool bValid;
    switch (apCollidee->GetObjectReference()->GetFormType()) {
        case 35u:
        case 42u:
        case 43u:
        case 51u: {
            bValid = false;
            break;
        }
        default: {
            bValid = true;
            break;
        }
    }
    return bValid;
}

void BGSDecalManager::AddEmitter(BGSDecalEmitter* apEmitter) {
    if (apEmitter) {
        BGSDecalManager::GetSingleton()->DecalEmitterList.AddTail(apEmitter);
    }
}

void BGSDecalManager::RemoveAllDecals() {
    BGSDecalManager::iDecalsThisFrame = 0;
    BGSDecalManager::iSkinnedDecalsThisFrame = 0;
    BGSDecalManager::iDecalCount = 0;
    BGSDecalManager::iSkinnedDecalCount = 0;
    BGSDecalManager::GetSingleton()->PendingSimpleDecalList.RemoveAll();
    NiTPointerList<BGSDecalEmitter*>* emitterList = &BGSDecalManager::GetSingleton()->DecalEmitterList;
    NiTListItem<BGSDecalEmitter*>* iter = emitterList->m_pkHead;
    while (iter) {
        BGSDecalEmitter* pemitter = emitterList->GetNext(iter);
        if (pemitter) {
            delete pemitter;
        }
    }
    emitterList->RemoveAll();
}

BGSDecalManager* BGSDecalManager::Destroy_() {
    BGSDecalManager* pManager = BGSDecalManager::pSingleton;
    delete BGSDecalManager::pSingleton;
    BGSDecalManager::pSingleton = nullptr;
    return pManager;
}

DECAL_CREATION_DATA::DECAL_CREATION_DATA()
  : pOrigin(NiPoint3::ZERO), pDirection(NiPoint3::ZERO), pSurfaceNormal(NiPoint3::ZERO), pObjRefr(nullptr),
    pAVObj(nullptr), pClone(nullptr), pTexSet(nullptr), iIndex(-1), fDecalWidth(0.0f), fDecalHeight(0.0f),
    fDecalDepth(0.0f), fRotation(0.0f), pParentCell(nullptr), fParallaxOcclusionScale(0.0f), pSkinnedDecalNode(nullptr),
    fSpecular(4.0f), fDecalEpsilon(15.0f), fPlacementRadius(16.0f), fVertexColor(1.0f, 1.0f, 1.0f), iBodyParts(0),
    cSubTex(0), bPermanent(false), bUseOcclusionQuery(true), bUseParallaxOcclusion(false), bAlphaTesting(true),
    bAlphaBlending(false), ucParallaxOcclusionMaxPasses(0), bModelSpace(false), bForceFade(false), bTwoSided(false) {}

BGSDecalEmitter::BGSDecalEmitter(
    BSTempEffectParticle* apParticleData,
    BGSImpactData* apImpactData,
    int32_t aiDecalsToEmit
)
  : iDecalsToEmit(aiDecalsToEmit), bFinished(false), pDecalImpactData(apImpactData), spParticleData(apParticleData) {}

BGSDecalEmitter::~BGSDecalEmitter() {
    spParticleData = nullptr;
}

void BGSDecalEmitter::Update() {
    if (MenuConsole::Instance(true)->IsVisible() || StartMenu::PauseMenuIsActive()) {
        return;
    }
    if (!spParticleData->GetCell() || !spParticleData->GetParticleObject()) {
        bFinished = true;
        return;
    }
    if (iDecalsToEmit <= 0 || !pDecalImpactData) {
        bFinished = true;
        return;
    }
    double dAge = spParticleData->GetAge();
    // The emitter finishes when the lifetime is below the age or either is NaN.
    if (!(spParticleData->GetLifetime() >= dAge)) {
        bFinished = true;
        return;
    }
    NiPoint3 kDirection;
    spParticleData->GetParticleObject()->GetLocalTransform().m_Rotate.GetRow(2, kDirection);
    kDirection.Unitize();
    NiPoint3 kFrom = spParticleData->GetParticleObject()->GetTranslate();
    float fMaxWidth = pDecalImpactData->GetDecalMaxWidth();
    float fMinWidth = static_cast<float>(pDecalImpactData->GetMinWidth());
    float fWidth = RandomFloat(fMinWidth, fMaxWidth);
    DecalCaster* pCaster = DecalCaster::GetInstance(fWidth, true);
    pCaster->SetCustomPickCollisions(DecalCaster::CUSTOM_PICK_ACTORS);
    pCaster->SetCollision(L_BIPED, false);
    pCaster->SetCollision(L_DEADBIP, false);
    TESObjectCELL* pCell = spParticleData->GetCell();
    bhkWorld* pWorld = pCell ? pCell->GetbhkWorld() : nullptr;
    alignas(16) bhkPickData kPick;
    if (!pWorld) {
        return;
    }
    pCaster->Add(static_cast<bhkWorldM*>(pWorld));
    float fDistance = fSprayDecalsDistance.Float();
    NiPoint3 kTo = kFrom + kDirection * fDistance;
    kTo.x = RandomFloat(-20.0f, 20.0f) + kTo.x;
    kTo.y = RandomFloat(-20.0f, 20.0f) + kTo.y;
    kTo.z = RandomFloat(-20.0f, 20.0f) + kTo.z;
    // The drop is computed in double (the age over the lifetime, times the negated gravity) and rounded to float.
    double dGravity = -static_cast<double>(fSprayDecalsGravity.Float());
    double dDropAge = spParticleData->GetAge();
    float fDrop = static_cast<float>(dDropAge / spParticleData->GetLifetime() * dGravity);
    kTo.z = kTo.z + fDrop;
    // The compare is signed.
    if (static_cast<int32_t>(iSprayDecalsDebug.UInt()) > 0) {
        // The end colour is built first.
        alignas(4) uint8_t aucEndColor[sizeof(NiColorA)];
        NiColorA* pEndColor = ::new (aucEndColor) NiColorA(0.0f, 1.0f, 1.0f, 1.0f);
        alignas(4) uint8_t aucStartColor[sizeof(NiColorA)];
        NiColorA* pStartColor = ::new (aucStartColor) NiColorA(0.0f, 1.0f, 0.0f, 1.0f);
        NiLines* pLine = MakeLine(kFrom, *pStartColor, kTo, *pEndColor, true);
        TES::pInstance->AddTempDebugObject(pLine, 30.0f);
    }
    CFilter kFilter(0);
    kFilter.SetLayer(L_CUSTOMPICK1);
    kPick.SetFilter(kFilter);
    kPick.NiSetFrom(kFrom);
    kPick.NiSetTo(kTo);
    alignas(16) uint8_t aucCollector[sizeof(hkpAllRayHitCollector)];
    hkpAllRayHitCollector* pCollector = ::new (aucCollector) hkpAllRayHitCollector;
    kPick.SetAllCollector(pCollector);
    // The pick's result byte is deliberately never read.
    pWorld->PickObject(kPick);
    int32_t iNumHits = kPick.GetAllCollector()->getHits().getSize();
    bool bPlaced = false;
    // The compare is signed.
    for (int32_t i = 0; !bPlaced && i < iNumHits; ++i) {
        hkpCollidable* pCollidable = kPick.GetAllCollector()->getHits()[i].m_rootCollidable;
        if (!pCollidable) {
            continue;
        }
        uint32_t iKey = 0xFFFFFFFF;
        for (int32_t j = 0; j < 8; ++j) {
            if (kPick.GetAllCollector()->getHits()[i].m_shapeKeys[j] == 0xFFFFFFFF) {
                break;
            }
            iKey = kPick.GetAllCollector()->getHits()[i].m_shapeKeys[j];
        }
        // The collidable's shape through hkpCdBody's shared one-field getter.
        bhkShape* pShape = bhkShape::Getbhk(static_cast<const hkpCdBody*>(pCollidable)->getShape());
        if (!pShape) {
            continue;
        }
        // The material is fetched but never used.
        pShape->GetSubshapeMaterial(iKey);
        BGSTextureSet* pTexSet = pDecalImpactData ? pDecalImpactData->GetDecalTextureSet() : nullptr;
        if (!pTexSet) {
            continue;
        }
        NiPoint3 kDelta = kTo - kFrom;
        float fHitFraction = kPick.GetAllCollector()->getHits()[i].m_hitFraction;
        NiPoint3 kHitPoint = kFrom + fHitFraction * kDelta;
        NiPoint3 kNormal;
        HK2NINS(kNormal, kPick.GetAllCollector()->getHits()[i].m_normal);
        bool bLand = CFilter(pCollidable->getCollisionFilterInfo()).GetGroup() == 1;
        NiAVObject* pObject = nullptr;
        if (bLand) {
            pObject = TES::pInstance->GetLandGeometry(kHitPoint);
            if (pObject && pObject->GetParent()) {
                pObject = pObject->GetParent();
            }
        }
        else {
            pObject = bhkUtilFunctions::GetNiAVObject(pCollidable);
        }
        TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pObject);
        if (!bLand && !(pRef && BGSDecalManager::IsValidDecalImpact(pRef, BGSDecalManager::DECAL_TYPE_SIMPLE))) {
            continue;
        }
        TESObjectCELL* pDecalCell = TES::pInstance->GetCellForPoint(kHitPoint);
        alignas(4) uint8_t aucData[sizeof(DECAL_CREATION_DATA)];
        DECAL_CREATION_DATA& kData = *::new (aucData) DECAL_CREATION_DATA;
        kData.pOrigin = kHitPoint;
        kData.pDirection = kNormal;
        kData.fDecalWidth = fWidth;
        kData.fDecalHeight = fWidth;
        kData.fDecalDepth = 48.0f;
        kData.fRotation = RandomFloatTwoPi();
        kData.cSubTex = static_cast<char>(static_cast<int32_t>(std::floor(RandomFloat(0.0f, 4.0f))));
        kData.pAVObj = pObject;
        kData.pTexSet = pTexSet;
        kData.bUseParallaxOcclusion = pDecalImpactData->GetIsParallax();
        kData.fParallaxOcclusionScale = pDecalImpactData->GetDecalParallaxScale();
        kData.ucParallaxOcclusionMaxPasses = pDecalImpactData->GetParallaxPasses();
        kData.bAlphaTesting = pDecalImpactData->GetIsAlphaTest();
        kData.bAlphaBlending = pDecalImpactData->GetIsAlphaBlend();
        kData.fSpecular = pDecalImpactData->GetDecalShininess();
        kData.fDecalEpsilon = pDecalImpactData->GetDecalAngleThreshold();
        kData.fPlacementRadius = pDecalImpactData->GetPlacementRadius();
        // The colour's bytes 2, 1 and 0 over 255.0 (double), each rounded to float; the point's x takes byte 0.
        float fByte2 = static_cast<float>(static_cast<uint8_t>(pDecalImpactData->GetDecalColor() >> 16) / 255.0);
        float fByte1 = static_cast<float>(static_cast<uint8_t>(pDecalImpactData->GetDecalColor() >> 8) / 255.0);
        float fByte0 = static_cast<float>(static_cast<uint8_t>(pDecalImpactData->GetDecalColor()) / 255.0);
        NiPoint3 kColor(fByte0, fByte1, fByte2);
        kData.fVertexColor.r = kColor.x;
        kData.fVertexColor.g = kColor.y;
        kData.fVertexColor.b = kColor.z;
        pDecalCell->AddDecal(kData, BGSDecalManager::DECAL_TYPE_SIMPLE, false);
        bPlaced = true;
    }
    if (bPlaced) {
        --iDecalsToEmit;
    }
    pCollector->~hkpAllRayHitCollector();
}

float BGSImpactData::GetDecalMaxWidth() {
    return DData.GetDecalMaxWidth();
}

BGSTextureSet* BGSImpactData::GetDecalTextureSet() {
    return pDecalTextureSet;
}

float BGSImpactData::GetPlacementRadius() {
    return Data.fPlacementRadius;
}

float BGSImpactData::GetDecalAngleThreshold() {
    return Data.fAngleThreshold;
}

void DecalCaster::SetCollision(COL_LAYER aeLayer, bool abEnable) {
    bhkCollisionFilter::SetLayerCollision(L_CUSTOMPICK1, aeLayer, abEnable);
}

// The fDecalLOD0:Display setting.
float Renderer::GetDecalLODStart() {
    return Renderer::fDecalLODStart;
}

float BSTempEffect::GetLifetime() {
    return fLifetime;
}

float BSTempEffect::GetAge() {
    return fAge;
}

TESObjectCELL* BSTempEffect::GetCell() {
    return pCell;
}

int32_t BSTempEffectGeometryDecal::GetNumVerts() {
    return iNumVerts;
}

void BSTempEffectGeometryDecal::SetDecalParent(NiNode* apNode) {
    spDecalParent = apNode;
}

NiFixedString& FixedStrings::QSkinnedDecalNode() {
    return *Skinned_Decal_Node;
}
