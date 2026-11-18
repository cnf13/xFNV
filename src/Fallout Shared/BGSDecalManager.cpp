#include "Fallout Shared/BGSDecalManager.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldM.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSOcclusionQuery.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
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
#include "Fallout Shared/Havok/SpecificItemCollector.h"
#include "Fallout Shared/Renderer.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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
SettingT<RendererSettingCollection> gs_iMaxDecalsPerFrame_Display("iMaxDecalsPerFrame:Display", 10);
SettingT<RendererSettingCollection> BGSDecalManager::iMaxSkinDecalsPerFrame("iMaxSkinDecalsPerFrame:Display", 3);
SettingT<RendererSettingCollection> gs_uMaxDecals_Decals("uMaxDecals:Decals", 100);
SettingT<INISettingCollection> gs_uMaxSkinDecals_Decals("uMaxSkinDecals:Decals", 24u);
SettingT<INISettingCollection> BGSDecalManager::iMaxSkinDecalPerActor("uMaxSkinDecalPerActor:Decals", 3u);
SettingT<INISettingCollection> gs_bProfileDecals_Decals("bProfileDecals:Decals", false);
SettingT<INISettingCollection> BGSDecalManager::bDebugDecals("bDebugDecals:Decals", false);
SettingT<INISettingCollection> gs_bDecalOcclusionQuery_Decals("bDecalOcclusionQuery:Decals", true);
SettingT<INISettingCollection> BGSDecalManager::bDecalMultithreaded("bDecalMultithreaded:Decals", false);
SettingT<INISettingCollection> gs_bDecals_Decals("bDecals:Decals", true);
SettingT<INISettingCollection> gs_bSkinnedDecals_Decals("bSkinnedDecals:Decals", true);
SettingT<GameSettingCollection> fSprayDecalsDistance("fSprayDecalsDistance", 64.0f);
SettingT<GameSettingCollection> fSprayDecalsGravity("fSprayDecalsGravity", 128.0f);
SettingT<GameSettingCollection> iSprayDecalsDebug("iSprayDecalsDebug", 0);
NiPointer<NiAVObject> BGSDecalManager::spDebugDecalBox;

int32_t BSSM_ShaderPropertyIsLighting(NiShadeProperty* apShaderProp) {
    if (!apShaderProp) {
        return 0;
    }
    int32_t shaderPropertyType = apShaderProp->GetShaderPropertyType();
    return (int32_t)((uint8_t)((shaderPropertyType >= 1) && (shaderPropertyType <= 12)));
}

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
        NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        Renderer_->LockRenderer();
        iter = PendingSimpleDecalList.m_pkHead;
        while (true) {
            BSTempEffectSimpleDecal* pdecal;
            do {
                if (!iter) {
                    NiDX9Renderer* v5 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                    v5->UnlockRenderer();
                    return;
                }
                prev = iter;
                pdecal = PendingSimpleDecalList.GetNext(iter).m_pObject;
            }
            while (!pdecal);
            if (pdecal->bOcclusionQuery) {
                IssueDecalOcclusionQuery(pdecal);
                pdecal->bOcclusionQuery = false;
            }
            if (!pdecal->pOcclusionQuery) {
                goto LABEL_17;
            }
            if (pdecal->CheckOcclusionQueryResults()) {
                if (pdecal->iCurrentOcclusionResult) {
                    if (BGSDecalManager::bDebugDecals.Bool()) {
                        NiAVObject* DecalParent = pdecal->GetDecalParent();
                        Warning(
                            "DECAL: Decal Succeeded Occlusion Query : %s", (&DecalParent->m_kName)->GetHandleString()
                        );
                    }
                LABEL_17:;
                    pdecal->Initialize();
                    goto LABEL_18;
                }
                if (BGSDecalManager::bDebugDecals.Bool()) {
                    NiAVObject* v3 = pdecal->GetDecalParent();
                    Warning("DECAL: Decal Failed Occlusion Query : %s", (&v3->m_kName)->GetHandleString());
                }
                pdecal->bFinished = true;
            }
        LABEL_18:;
            if (pdecal->bFinalize && !pdecal->bFinished) {
                pdecal->FinalizeGeometry();
            }
            if (pdecal->bFinished) {
                if (pdecal->GetIsValid()) {
                    ++BGSDecalManager::iDecalCount;
                    if (BGSDecalManager::bDebugDecals.Bool()) {
                        NiAVObject* v4 = pdecal->GetDecalParent();
                        Warning(
                            "DECAL: Placing Simple Decal #%d : %s", BGSDecalManager::iDecalCount,
                            (&v4->m_kName)->GetHandleString()
                        );
                    }
                    if ((BGSDecalManager::iDecalDebugFlags & 1) != 0) {
                        NiColorA color(0.0f, 1.0f, 0.0f, 1.0f);
                        pdecal->CreateDebugBox(color, true);
                    }
                    if ((BGSDecalManager::iDecalDebugFlags & 2) != 0) {
                        NiColorA color_2(0.0f, 1.0f, 0.0f, 0.25f);
                        pdecal->CreateDebugBox(color_2, false);
                    }
                }
                else {
                    if ((BGSDecalManager::iDecalDebugFlags & 16) != 0) {
                        if ((BGSDecalManager::iDecalDebugFlags & 1) != 0) {
                            NiColorA color_3(1.0f, 0.0f, 0.0f, 1.0f);
                            pdecal->CreateDebugBox(color_3, true);
                        }
                        if ((BGSDecalManager::iDecalDebugFlags & 2) != 0) {
                            NiColorA color_4(1.0f, 0.0f, 0.0f, 0.25f);
                            pdecal->CreateDebugBox(color_4, false);
                        }
                    }
                }
                PendingSimpleDecalList.RemovePos(prev);
            }
        }
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
    NiAVObject* pavobj;
    NiTPointerList<NiAVObject*> listavobj;
    char acBuffer[260];
    _LARGE_INTEGER End;
    _LARGE_INTEGER TicksPerSecond;
    _LARGE_INTEGER Start;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_EFFECTS_OVERHEAD, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BGSDecalManager.cpp", 440
    );
    bClearQueryTexture = true;
    if (aeDecalType == BGSDecalManager::DECAL_TYPE_SIMPLE) {
        if ((gs_bDecals_Decals.Bool()) && (arDecalData.pAVObj) && (!arDecalData.pAVObj->GetNoDecals())) {
            NiPoint3 keyepos = pPlayer->GetLocationOnReference();
            float fdist = (arDecalData.pOrigin - keyepos).Length();
            double fTicksPerMicrosecond = 1000.0;
            int32_t finaltricount = 0;
            Start.u.LowPart = 0;
            if (gs_bProfileDecals_Decals.Bool()) {
                QueryPerformanceFrequency(&TicksPerSecond);
                fTicksPerMicrosecond = static_cast<double>(TicksPerSecond.QuadPart) * 1e-06;
                QueryPerformanceCounter(&Start);
            }
            if (static_cast<double>(fdist) <= Renderer::GetDecalLODStart() || (arDecalData.bModelSpace || abForceAdd)) {
                if (BGSDecalManager::bDebugDecals.Bool()) {
                    Warning("DECAL: Simple Decal Instantiated");
                }
                AddGeometryDecalRecurse(arDecalData);
            }
            if (gs_bProfileDecals_Decals.Bool()) {
                QueryPerformanceCounter(&End);
                int64_t iTicks = End.QuadPart - Start.QuadPart;
                BSsprintf(
                    acBuffer, 255, "AddGeometryDecalRecurse: Ticks: %lld, Microseconds: %f", iTicks,
                    static_cast<double>(iTicks) / fTicksPerMicrosecond
                );
                InitializationFlow(acBuffer, 0);
            }
        }
    }
    else {
        if (aeDecalType == BGSDecalManager::DECAL_TYPE_PROJECTED) {
            if (gs_bSkinnedDecals_Decals.Bool() && (arDecalData.pAVObj && !arDecalData.pAVObj->GetNoDecals())) {
                NiPoint3 keyepos_3 = pPlayer->GetLocationOnReference();
                NiBound bound = arDecalData.pAVObj->GetWorldBound();
                double v24 = (bound.m_kCenter - keyepos_3).Length();
                float fdist_3 = v24 - bound.m_fRadius;
                if ((static_cast<double>(fdist_3) <= Renderer::GetSkinnedDecalLODStart() &&
                     BGSDecalManager::iSkinnedDecalCount < gs_uMaxSkinDecals_Decals.UInt()) ||
                    abForceAdd) {
                    AddProjectedDecalRecurse(arDecalData);
                }
                else {
                    if (BGSDecalManager::iSkinnedDecalCount >= gs_uMaxSkinDecals_Decals.UInt() &&
                        BGSDecalManager::bDebugDecals.Bool()) {
                        Warning("DECAL: Reached Max Skin Decal Limit! : %d", gs_uMaxSkinDecals_Decals.Int());
                    }
                }
            }
            else {
                if (gs_bSkinnedDecals_Decals.Bool() && arDecalData.pObjRefr) {
                    NiNode* pnode;
                    NiPoint3 keyepos_2 = pPlayer->GetLocationOnReference();
                    NiNode* v9 = arDecalData.pObjRefr->Get3D();
                    NiBound bound_2 = v9->GetWorldBound();
                    double v23 = (bound_2.m_kCenter - keyepos_2).Length();
                    float fdist_2 = v23 - bound_2.m_fRadius;
                    NiAVObject* pavobject = (NiAVObject*)arDecalData.pObjRefr->Get3D();
                    if (pavobject) {
                        pnode = pavobject->IsNode();
                    }
                    else {
                        pnode = nullptr;
                    }
                    if ((pnode) && (!pavobject->GetNoDecals())) {
                        arDecalData.pAVObj = (NiAVObject*)pnode;
                        if ((static_cast<double>(fdist_2) <= Renderer::GetSkinnedDecalLODStart() &&
                             BGSDecalManager::iSkinnedDecalCount < gs_uMaxSkinDecals_Decals.UInt()) ||
                            abForceAdd) {
                            AddProjectedDecalRecurse(arDecalData);
                        }
                        else {
                            if (BGSDecalManager::iSkinnedDecalCount >= gs_uMaxSkinDecals_Decals.UInt() &&
                                BGSDecalManager::bDebugDecals.Bool()) {
                                Warning("DECAL: Reached Max Skin Decal Limit! : %d", gs_uMaxSkinDecals_Decals.Int());
                            }
                        }
                    }
                }
                else {
                    if (gs_bDecals_Decals.Bool()) {
                        arDecalData.pDirection.Unitize();
                        if (!BGSDecalManager::pDecalCaster) {
                            float a2 = BSMAX(arDecalData.fDecalHeight, arDecalData.fDecalWidth);
                            BGSDecalManager::pDecalCaster = DecalCaster::GetInstance(a2, true);
                        }
                        BGSDecalManager::pDecalCaster->SetCollision(L_BIPED, false);
                        BGSDecalManager::pDecalCaster->SetCollision(L_DEADBIP, false);
                        bhkWorld* v14 = arDecalData.pParentCell->GetbhkWorld();
                        BGSDecalManager::pDecalCaster->Add(v14);
                        int32_t icount = BGSDecalManager::pDecalCaster->Pick(
                            arDecalData.pOrigin, arDecalData.pOrigin + arDecalData.pDirection * 80.0f, 32.0f
                        );
                        if (!icount) {
                            NiPoint3 dir = arDecalData.pDirection + -NiPoint3_UNIT_Z * 2.0f;
                            dir.Unitize();
                            arDecalData.pDirection = dir;
                            icount = BGSDecalManager::pDecalCaster->Pick(
                                arDecalData.pOrigin, arDecalData.pOrigin + dir * 80.0f, 32.0f
                            );
                        }
                        ::new (&listavobj) NiTPointerList<BSAnimGroupSequence*>;
                        for (int32_t i = 0; i < icount; ++i) {
                            pavobj = BGSDecalManager::pDecalCaster->GetAVObject(static_cast<uint32_t>(i));
                            if (pavobj) {
                                NiPoint3 havokpos;
                                NiPoint3 havoknormal;
                                BGSDecalManager::pDecalCaster->GetCollisionData(
                                    static_cast<uint32_t>(i), havokpos, havoknormal
                                );
                                havoknormal.Unitize();
                                TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pavobj);
                                if ((!pavobj->GetNoDecals()) && (pref) &&
                                    (BGSDecalManager::IsValidDecalImpact(pref, BGSDecalManager::DECAL_TYPE_SIMPLE) &&
                                     !listavobj.FindPos(pavobj, nullptr))) {
                                    arDecalData.pOrigin = havokpos;
                                    arDecalData.pDirection = havoknormal;
                                    arDecalData.pAVObj = pavobj;
                                    AddGeometryDecalRecurse(arDecalData);
                                    NiAVObjectList_AddHead(&listavobj, pavobj);
                                }
                            }
                        }
                        listavobj.RemoveAll();
                    }
                }
            }
        }
        else {
            if ((aeDecalType == BGSDecalManager::DECAL_TYPE_PERMENANT_SIMPLE && gs_bDecals_Decals.Bool()) &&
                (arDecalData.pAVObj) && (!arDecalData.pAVObj->GetNoDecals())) {
                arDecalData.bPermanent = true;
                AddGeometryDecalRecurse(arDecalData);
            }
        }
    }
}

bool BGSDecalManager::AddGeometryDecalRecurse(DECAL_CREATION_DATA& arDecalData) {
    int32_t* v2 = &gs_iMaxDecalsPerFrame_Display.Int();
    if (BGSDecalManager::iDecalsThisFrame < *v2) {
        if (arDecalData.pAVObj) {
            const char* pname = (&arDecalData.pAVObj->m_kName)->GetHandleString();
            if (pname &&
                (!std::strncmp(pname, "Decal Node", 10u) ||
                 (!std::strncmp(pname, pcString, 5u) ||
                  (!std::strncmp(pname, "FaceGen", 7u) ||
                   (!std::strncmp(pname, "BSFaceGen", 9u) ||
                    (!std::strncmp(pname, "Bip01", 5u) || !std::strncmp(pname, "Debug Decal Box", 15u))))))) {
                return true;
            }
            if (!NiObject::IsKindOf_0(&NiTriStrips::ms_RTTI, arDecalData.pAVObj) ||
                arDecalData.pAVObj->GetAppCulled()) {
                if (arDecalData.pAVObj->IsNode()) {
                    NiNode* pnode_2 = static_cast<NiNode*>(arDecalData.pAVObj);
                    if (pnode_2 && pnode_2->GetAppCulled()) {
                        return true;
                    }
                    for (uint32_t i_2 = 0; i_2 < pnode_2->GetArrayCount(); ++i_2) {
                        NiAVObject* child = pnode_2->GetAt(i_2);
                        if (child) {
                            arDecalData.pAVObj = child;
                            AddGeometryDecalRecurse(arDecalData);
                        }
                    }
                }
            }
            else {
                BSTempEffectSimpleDecal* pdecal;
                NiGeometry* pgeom = static_cast<NiGeometry*>(arDecalData.pAVObj);
                if (pgeom) {
                    BSShaderPPLightingProperty* pshade =
                        static_cast<BSShaderPPLightingProperty*>(pgeom->m_PropertyState.GetShade());
                    if ((pshade) && (pshade->GetFlag(26u))) {
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
                void* apMemory = NiMemObject::operator new(sizeof(BSTempEffectSimpleDecal));
                if (apMemory) {
                    float afLifetime = BGSDecalManager::fDecalLifetime.operator float();
                    pdecal = ::new (apMemory) BSTempEffectSimpleDecal(pdecalnode, afLifetime, arDecalData);
                }
                else {
                    pdecal = nullptr;
                }
                NiTriStrips* ptristrips = static_cast<NiTriStrips*>(arDecalData.pAVObj);
                if (ptristrips->GetActiveTriangleCount() > 1000u && gs_bDecalOcclusionQuery_Decals.Bool() &&
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
    if (iSkinnedDecalsThisFrame >= (int32_t)iMaxSkinDecalsPerFrame.UInt()) {
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
        NiTexture* pTexture = spTexture.operator->();
        if (NiObject::IsKindOf_0(&NiSourceTexture::ms_RTTI, pTexture)) {
            spDiffuseTexture = static_cast<NiSourceTexture*>(pTexture);
        }
    }
    if (!spDiffuseTexture.operator->()) {
        return false;
    }
    if (!arDecalData.pAVObj) {
        return true;
    }
    const char* pName = arDecalData.pAVObj->GetName().GetHandleString();
    if (pName &&
        (!std::strncmp(pName, "Decal", 5) || !std::strncmp(pName, "FaceGen", 7) || !std::strncmp(pName, "Bip01", 5) ||
         !std::strncmp(pName, "BSFaceGen", 9) || !std::strncmp(pName, "Hair", 4))) {
        return true;
    }
    // Tests the returned byte.
    if ((uint8_t)arDecalData.pAVObj->GetAppCulled()) {
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
                void* pMemory = NiMemObject::operator new(sizeof(BGSDecalNode));
                pDecalNode = pMemory ? ::new (pMemory) BGSDecalNode(pParent, true) : nullptr;
            }
            else if (pDecalNode->GetChildCount() >= iMaxSkinDecalPerActor.UInt()) {
                // The compare is unsigned.
                return true;
            }
            ++iSkinnedDecalsThisFrame;
            if (bDebugDecals.Bool()) {
                Warning("DECAL: Skinned decal count this frame: #%d", iSkinnedDecalsThisFrame);
            }
            void* pMemory = NiMemObject::operator new(sizeof(BSTempEffectGeometryDecal));
            BSTempEffectGeometryDecal* pNewDecal = pMemory
                ? ::new (pMemory) BSTempEffectGeometryDecal(
                      arDecalData.pParentCell, fDecalLifetime.operator float(), &arDecalData, pGeometry,
                      arDecalData.pOrigin, arDecalData.pDirection, arDecalData.fDecalWidth, arDecalData.fRotation,
                      arDecalData.iBodyParts
                  )
                : nullptr;
            {
                NiPointer<BSTempEffectGeometryDecal> spNewDecal(pNewDecal);
                BSTempEffect* pEffect = spNewDecal.operator->();
                pEffect->Initialize();
                // The compare is signed.
                if (spNewDecal.operator->()->GetNumVerts() <= 0) {
                    return true;
                }
                if (!arDecalData.pSkinnedDecalNode) {
                    void* pNodeMemory = NiMemObject::operator new(sizeof(NiNode));
                    NiNode* pSkinnedNode = pNodeMemory ? ::new (pNodeMemory) NiNode(0) : nullptr;
                    arDecalData.pSkinnedDecalNode = pSkinnedNode;
                    arDecalData.pSkinnedDecalNode->SetName(FixedStrings::QSkinnedDecalNode());
                    pDecalNode->AttachChild(arDecalData.pSkinnedDecalNode, true);
                    ++iSkinnedDecalCount;
                    if (bDebugDecals.Bool()) {
                        Warning("DECAL: Placing Skin Decal #%d", iSkinnedDecalCount);
                    }
                }
                pEffect = spNewDecal.operator->();
                NiAVObject* pDecal3D = pEffect->Get3D();
                arDecalData.pSkinnedDecalNode->AttachChild(pDecal3D, true);
                NiNode* pSkinnedDecalNode = arDecalData.pSkinnedDecalNode;
                spNewDecal.operator->()->SetDecalParent(pSkinnedDecalNode);
                {
                    NiPointer<NiRefObject> spDecal(spNewDecal.operator->());
                    pDecalNode->pDecalList->AddHead(spDecal);
                }
                NiUpdateData kUpdateData;
                kUpdateData.~NiUpdateData();
                ::new (&kUpdateData) NiUpdateData(0.0f, false, false);
                pDecalNode->Update(kUpdateData);
                ReferenceProcessLists.AddTempEffect(spNewDecal.operator->());
                spTempEffectDecal = spNewDecal.operator->();
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
                            void* pMemory = NiMemObject::operator new(sizeof(BSTempEffectGeometryDecal));
                            BSTempEffectGeometryDecal* pNewDecal = pMemory
                                ? ::new (pMemory) BSTempEffectGeometryDecal(
                                      arDecalData.pParentCell, fDecalLifetime.operator float(), &arDecalData,
                                      pCloneGeom, arDecalData.pOrigin, -arDecalData.pDirection, 1.0f,
                                      arDecalData.fRotation, arDecalData.iBodyParts
                                  )
                                : nullptr;
                            spCloneDecal = pNewDecal;
                            // Initialize is called through the raw pointer without a null check, as the game does.
                            BSTempEffect* pEffect = pNewDecal;
                            pEffect->Initialize();
                        }
                        if (spCloneDecal.operator->()) {
                            ReferenceProcessLists.AddTempEffect(spCloneDecal.operator->());
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
    BSOcclusionQuery* v18;
    XFNV_INLINED_LOCAL(NiColorA, v23, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(NiColorA, apMemory, std::nullopt, std::nullopt);
    NiMatrix3 rot_2;
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
    normal.operator*=(50.0f);
    NiPoint3 pos = apDecal->GetOrigin() + normal;
    NiPoint3 afTime_4 = -normal;
    ::new (&rot) NiMatrix3(BSUtilities::GetRotationZToVector(afTime_4));
    float angle_4 = apDecal->fRotation;
    kzrot.MakeZRotation(angle_4);
    rot = *::new (&rot_2) NiMatrix3(kzrot * rot);
    rot.GetRow(0, tangent);
    rot.GetRow(1u, binormal);
    rot.GetRow(2u, normal);
    adjustedrot.SetCol(0, normal);
    adjustedrot.SetCol(1u, tangent);
    adjustedrot.SetCol(2u, binormal);
    double v20 = apDecal->fWidth / 2.0;
    NiAVObject* DecalParent = apDecal->GetDecalParent();
    float fwidth = 1.0 / DecalParent->m_kWorld.m_fScale * v20;
    double v19 = apDecal->GetDecalHeight() / 2.0;
    NiAVObject* v4 = apDecal->GetDecalParent();
    float fheight = 1.0 / v4->m_kWorld.m_fScale * v19;
    float frustumNear = apDecal->fDepth + 50.0;
    NiFrustum frustum(false);
    frustum.m_bOrtho = true;
    frustum.m_fNear = 0.1f;
    frustum.m_fLeft = -fwidth;
    frustum.m_fRight = fwidth;
    frustum.m_fTop = fheight;
    frustum.m_fBottom = -fheight;
    frustum.m_fFar = frustumNear;
    spQueryCamera.m_pObject->SetTranslate(pos);
    spQueryCamera.m_pObject->SetRotate(adjustedrot);
    spQueryCamera.m_pObject->SetViewFrustum(frustum);
    NiUpdateData UpdateData(0.0f, false, false);
    spQueryCamera.m_pObject->Update(UpdateData);
    if ((BGSDecalManager::iDecalDebugFlags & 4) != 0) {
        start = pos;
        end = pos + spQueryCamera.m_pObject->GetWorldDirection() * frustumNear;
        NiLines* plines = MakeLine(
            start, *::new (&v23) NiColorA(0.0f, 0.0f, 1.0f, 1.0f), end,
            *::new (&apMemory) NiColorA(1.0f, 0.0f, 0.0f, 1.0f), true
        );
        TES::pInstance->AddTempDebugObject(plines, 30.0f);
    }
    if (BGSDecalManager::bDebugDecals.Bool()) {
        NiPoint3 vec = spQueryCamera.m_pObject->GetWorldDirection();
        NiAVObject* v7 = apDecal->GetDecalParent();
        Warning("DECAL: Issuing Occlusion Query: %s", (&v7->m_kName)->GetHandleString());
        Warning("DECAL: Origin: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        Warning("DECAL: Direction: (%.2f, %.2f, %.2f)", vec.x, vec.y, vec.z);
        Warning(
            "DECAL: Frustum: Near=0.1f, Far=%.2f, Left=%.2f, Right=%.2f, Top=%.2f, Bottom=%.2f )", frustumNear, -fwidth,
            fwidth, fheight, -fheight
        );
    }
    if (!spQueryTexture.m_pObject) {
        NiDX9Renderer* fR_4 = BSShaderManager::GetRenderer();
        BSTextureManager* TextureManager = BSShaderManager::TextureManagerPtr();
        spQueryTexture.operator=(
            TextureManager->BorrowRenderedTexture(fR_4, BSTextureManager::TEX_DECAL_OCCLUSION, 0, nullptr, 0)
        );
    }
    if (apDecal->bClearQueryTexture) {
        NiRenderTargetGroup* Group = spQueryTexture.m_pObject->GetGroup();
        BSRenderedTexture::StartOffscreen(7u, Group);
    }
    else {
        NiRenderTargetGroup* v10 = spQueryTexture.m_pObject->GetGroup();
        BSRenderedTexture::StartOffscreen(0, v10);
    }
    BSOcclusionQuery* v22 = static_cast<BSOcclusionQuery*>(ourMalloc(sizeof(BSOcclusionQuery)));
    if (v22) {
        v18 = ::new (v22) BSOcclusionQuery;
    }
    else {
        v18 = nullptr;
    }
    apDecal->pOcclusionQuery = v18;
    apDecal->pOcclusionQuery->Begin();
    const NiRect<float>* angle_4b = &spQueryCamera.m_pObject->GetViewPort();
    NiRenderer* Renderer_ = NiRenderer::GetRenderer();
    Renderer_->SetScreenSpaceCameraData(angle_4b);
    BSCullingProcess culler(nullptr);
    culler.SetAccumulator(spQueryAccum.m_pObject);
    ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
    spQueryAccum.m_pObject->SetShadowSceneNode(ShadowSceneNode_);
    spQueryAccum.m_pObject->SetRenderMode(BSShaderManager::BSSM_RENDER_SILHOUETTE);
    NiAVObject* v13 = apDecal->GetDecalParent();
    BSShaderUtil::AccumulateScene(spQueryCamera.m_pObject, v13, culler);
    BSShaderUtil::RenderScene(spQueryCamera.m_pObject, spQueryAccum.m_pObject, false);
    culler.SetAccumulator(nullptr);
    apDecal->pOcclusionQuery->End();
    BSRenderedTexture::StopOffscreen();
}

void BGSDecalManager::ClearPendingQueries() {
    uint32_t iresult;
    NiTListItem<NiPointer<BSTempEffectSimpleDecal>>* iter;
    if (PendingSimpleDecalList.m_uiCount) {
        NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        Renderer_->LockRenderer();
        iter = PendingSimpleDecalList.m_pkHead;
        while (iter) {
            BSTempEffectSimpleDecal* pdecal = (&PendingSimpleDecalList.GetNext(iter))->m_pObject;
            if ((pdecal) && (pdecal->pOcclusionQuery)) {
                iresult = 0;
                pdecal->pOcclusionQuery->GetResult(&iresult, true);
                if (pdecal->pOcclusionQuery) {
                    delete pdecal->pOcclusionQuery;
                }
                pdecal->pOcclusionQuery = nullptr;
            }
        }
        NiDX9Renderer* v2 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
        v2->UnlockRenderer();
    }
}

BGSDecalManager* BGSDecalManager::GetSingleton() {
    BGSDecalManager* v1;
    if (BGSDecalManager::pSingleton) {
        return BGSDecalManager::pSingleton;
    }
    void* v2 = ourMalloc(sizeof(BGSDecalManager));
    if (v2) {
        v1 = ::new (v2) BGSDecalManager;
    }
    else {
        v1 = nullptr;
    }
    BGSDecalManager::pSingleton = v1;
    return BGSDecalManager::pSingleton;
}

void BGSDecalManager::ClearDecalsThisFrame() {
    BGSDecalManager::iDecalsThisFrame = 0;
    BGSDecalManager::iSkinnedDecalsThisFrame = 0;
}

bool BGSDecalManager::IsValidDecalImpact(TESObjectREFR* apCollidee, BGSDecalManager::DECAL_TYPE aeDecalType) {
    bool result;
    TESBoundObject* ObjectReference = apCollidee->GetObjectReference();
    switch (ObjectReference->GetFormType()) {
        case 35u:
        case 42u:
        case 43u:
        case 51u: {
            result = false;
            break;
        }
        default: {
            result = true;
            break;
        }
    }
    return result;
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
    BGSDecalManager* result = BGSDecalManager::pSingleton;
    delete BGSDecalManager::pSingleton;
    BGSDecalManager::pSingleton = nullptr;
    return result;
}

DECAL_CREATION_DATA::DECAL_CREATION_DATA()
  : pOrigin(NiPoint3_ZERO), pDirection(NiPoint3_ZERO), pSurfaceNormal(NiPoint3_ZERO), pObjRefr(nullptr),
    pAVObj(nullptr), pClone(nullptr), pTexSet(nullptr), iIndex(-1), fDecalWidth(0.0f), fDecalHeight(0.0f),
    fDecalDepth(0.0f), fRotation(0.0f), pParentCell(nullptr), fParallaxOcclusionScale(0.0f), pSkinnedDecalNode(nullptr),
    fSpecular(4.0f), fDecalEpsilon(15.0f), fPlacementRadius(16.0f), fVertexColor(1.0f, 1.0f, 1.0f), iBodyParts(0),
    cSubTex(0), bPermanent(false), bUseOcclusionQuery(true), bUseParallaxOcclusion(false), bAlphaTesting(true),
    bAlphaBlending(false), ucParallaxOcclusionMaxPasses(0), bModelSpace(false), bForceFade(false), bTwoSided(false) {}

bhkPickData::bhkPickData()
  : hkLength(hkVector4::getZero()), pCache(nullptr), pCloseCollector(nullptr), pAllCollector(nullptr),
    bPickFailed(false) {}

const hkVector4& bhkPickData::GethkTo() {
    CalcTo();
    return m_to;
}

void bhkPickData::SetFrom(const hkVector4& arFrom) {
    m_from = arFrom;
}

void bhkPickData::SetAllCollector(hkpAllRayHitCollector* apCollector) {
    pAllCollector = apCollector;
    pCloseCollector = nullptr;
}

void bhkPickData::SetClosestCollector(hkpClosestRayHitCollector* apCollector) {
    pCloseCollector = static_cast<SpecificItemCollector*>(apCollector);
    pAllCollector = nullptr;
}

bool bhkPickData::GetPickFailed() {
    return (bool)((uint8_t)bPickFailed);
}

void bhkPickData::SetLength(const hkVector4& arLength) {
    hkLength = arLength;
}

void bhkPickData::SetTo(const hkVector4& arTo) {
    SetLength(hkVector4::getZero());
    m_to = arTo;
}

void bhkPickData::SetFilter(CFilter aiFilter) {
    m_filterInfo = aiFilter;
}

void bhkPickData::NiSetFrom(const NiPoint3& arFrom) {
    hkVector4 hkv;
    this->SetFrom(NI2HK(hkv, arFrom));
}

void bhkPickData::NiSetTo(const NiPoint3& arTo) {
    hkVector4 kTo;
    SetTo(NI2HK(kTo, arTo));
}

// Builds the hit point in a Havok vector and converts it back to game units.
void bhkPickData::NiCalcHitPoint(NiPoint3& arPoint) {
    hkVector4 kPoint;
    CalcHitPoint(kPoint);
    HK2NI(arPoint, kPoint);
}

void bhkPickData::NiSetLength(const NiPoint3& arLength) {
    hkVector4 kHkLength;
    SetLength(NI2HK(kHkLength, arLength));
}

BGSDecalEmitter::BGSDecalEmitter(BSTempEffectParticle* a1, BGSImpactData* a2, int32_t a3)
  : iDecalsToEmit(a3), bFinished(false), pDecalImpactData(a2), spParticleData(a1) {}

BGSDecalEmitter::~BGSDecalEmitter() {
    spParticleData = nullptr;
}

void BGSDecalEmitter::Update() {
    if (MenuConsole::Instance(true)->IsVisible() || StartMenu::PauseMenuIsActive()) {
        return;
    }
    if (!spParticleData.operator->()->GetCell() || !spParticleData.operator->()->GetParticleObject()) {
        bFinished = true;
        return;
    }
    if (iDecalsToEmit <= 0 || !pDecalImpactData) {
        bFinished = true;
        return;
    }
    double dAge = spParticleData.operator->()->GetAge();
    // The emitter finishes when the lifetime is below the age or either is NaN.
    if (!(spParticleData.operator->()->GetLifetime() >= dAge)) {
        bFinished = true;
        return;
    }
    NiPoint3 kDirection;
    kDirection.~NiPoint3();
    ::new (&kDirection) NiPoint3;
    spParticleData.operator->()->GetParticleObject()->GetLocalTransform().m_Rotate.GetRow(2, kDirection);
    kDirection.Unitize();
    NiPoint3 kFrom = spParticleData.operator->()->GetParticleObject()->GetTranslate();
    float fMaxWidth = pDecalImpactData->GetDecalMaxWidth();
    float fMinWidth = (float)pDecalImpactData->GetMinWidth();
    float fWidth = RandomFloat(fMinWidth, fMaxWidth);
    DecalCaster* pCaster = DecalCaster::GetInstance(fWidth, true);
    pCaster->SetCustomPickCollisions(DecalCaster::CUSTOM_PICK_ACTORS);
    pCaster->SetCollision(L_BIPED, false);
    pCaster->SetCollision(L_DEADBIP, false);
    TESObjectCELL* pCell = spParticleData.operator->()->GetCell();
    bhkWorld* pWorld = pCell ? pCell->GetbhkWorld() : nullptr;
    alignas(16) bhkPickData kPick;
    kPick.~bhkPickData();
    ::new (&kPick) bhkPickData;
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
    double dGravity = -(double)fSprayDecalsGravity.Float();
    double dDropAge = spParticleData.operator->()->GetAge();
    float fDrop = (float)(dDropAge / spParticleData.operator->()->GetLifetime() * dGravity);
    kTo.z = kTo.z + fDrop;
    // The compare is signed.
    if ((int32_t)iSprayDecalsDebug.UInt() > 0) {
        // The end colour is built first.
        alignas(4) uint8_t aucEndColor[sizeof(NiColorA)];
        NiColorA* pEndColor = ::new (aucEndColor) NiColorA(0.0f, 1.0f, 1.0f, 1.0f);
        alignas(4) uint8_t aucStartColor[sizeof(NiColorA)];
        NiColorA* pStartColor = ::new (aucStartColor) NiColorA(0.0f, 1.0f, 0.0f, 1.0f);
        NiLines* pLine = MakeLine(kFrom, *pStartColor, kTo, *pEndColor, true);
        TES::pInstance->AddTempDebugObject(pLine, 30.0f);
    }
    CFilter kFilter;
    kFilter.~CFilter();
    ::new (&kFilter) CFilter(0);
    kFilter.SetLayer(L_CUSTOMPICK1);
    kPick.SetFilter(kFilter);
    kPick.NiSetFrom(kFrom);
    kPick.NiSetTo(kTo);
    alignas(16) uint8_t aucCollector[sizeof(hkpAllRayHitCollector)];
    hkpAllRayHitCollector* pCollector = ::new (aucCollector) hkpAllRayHitCollector;
    kPick.SetAllCollector(pCollector);
    // The pick's result byte is deliberately never read, as the game does.
    pWorld->PickObject(kPick);
    int32_t iNumHits = kPick.GetAllCollector()->getHits().getSize();
    bool bPlaced = false;
    // The compare is signed.
    for (int32_t i = 0; !bPlaced && i < iNumHits; ++i) {
        hkpCollidable* pCollidable =
            kPick.GetAllCollector()->getHits().operator[]<hkpWorldRayCastOutput>(i).m_rootCollidable;
        if (!pCollidable) {
            continue;
        }
        uint32_t iKey = 0xFFFFFFFF;
        for (int32_t j = 0; j < 8; ++j) {
            if (kPick.GetAllCollector()->getHits().operator[]<hkpWorldRayCastOutput>(i).m_shapeKeys[j] == 0xFFFFFFFF) {
                break;
            }
            iKey = kPick.GetAllCollector()->getHits().operator[]<hkpWorldRayCastOutput>(i).m_shapeKeys[j];
        }
        // The collidable's shape through hkpCdBody's shared one-field getter.
        bhkShape* pShape = bhkShape::Getbhk(static_cast<const hkpCdBody*>(pCollidable)->getShape());
        if (!pShape) {
            continue;
        }
        // The material is fetched but never used, as the game does.
        pShape->GetSubshapeMaterial(iKey);
        BGSTextureSet* pTexSet = pDecalImpactData ? pDecalImpactData->GetDecalTextureSet() : nullptr;
        if (!pTexSet) {
            continue;
        }
        NiPoint3 kDelta = kTo - kFrom;
        float fHitFraction = kPick.GetAllCollector()->getHits().operator[]<hkpWorldRayCastOutput>(i).m_hitFraction;
        NiPoint3 kHitPoint = kFrom + fHitFraction * kDelta;
        NiPoint3 kNormal;
        kNormal.~NiPoint3();
        ::new (&kNormal) NiPoint3;
        HK2NINS(kNormal, kPick.GetAllCollector()->getHits().operator[]<hkpWorldRayCastOutput>(i).m_normal);
        CFilter kHitFilter;
        bool bLand = XFNV_RECONSTRUCT(CFilter)(&kHitFilter, pCollidable->getCollisionFilterInfo())->GetGroup() == 1;
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
        kData.cSubTex = (char)(int32_t)std::floor(RandomFloat(0.0f, 4.0f));
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
        float fByte2 = (float)((double)(uint8_t)(pDecalImpactData->GetDecalColor() >> 16) / 255.0);
        float fByte1 = (float)((double)(uint8_t)(pDecalImpactData->GetDecalColor() >> 8) / 255.0);
        float fByte0 = (float)((double)(uint8_t)pDecalImpactData->GetDecalColor() / 255.0);
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
    return gs_fDecalLOD0_Display.operator float();
}

NiPoint3& HK2NINS(NiPoint3& arNiP, const hkVector4& arhkV) {
    arNiP.x = arhkV.getSimdAt(0).operator float();
    arNiP.y = arhkV.getSimdAt(1).operator float();
    arNiP.z = arhkV.getSimdAt(2).operator float();
    return arNiP;
}

float NI2HK(float afValue) {
    return bhkConvert::fBS2HkScaleSC * afValue;
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

float NiAVObject::GetWorldScale() {
    return m_kWorld.m_fScale;
}

uint32_t NiAVObject::GetNoDecals() {
    return GetFlag(NiAVObject::NO_DECALS);
}

const NiRect<float>& NiCamera::GetViewPort() const {
    return m_kPort;
}

// The products are rounded to float before the three-float constructor runs, and z is computed first.
NiPoint3 NiPoint3::ComponentProduct(const NiPoint3& p0, const NiPoint3& p1) {
    float fZ = p0.z * p1.z;
    float fY = p0.y * p1.y;
    float fX = p0.x * p1.x;
    return NiPoint3(fX, fY, fZ);
}

uint32_t NiRenderer::LockCount() {
    return m_kRendererLock.GetCurrentLockCount();
}

bool NiRenderer::GetInsideFrameState() {
    return m_eFrameState == FRAMESTATE_INSIDE_FRAME || m_eFrameState == FRAMESTATE_INSIDE_OFFSCREEN_FRAME;
}

bool NiRenderer::Check_RenderTargetGroupActive(const char*, bool bExpectedValue) {
    return GetInsideFrameState() && m_bRenderTargetGroupActive == bExpectedValue;
}

// The release build's lock count is always 0.
uint32_t NiCriticalSection::GetCurrentLockCount() {
    return 0;
}

hkpWorldRayCastInput::hkpWorldRayCastInput() : m_enableShapeCollectionFilter(false), m_filterInfo(0) {}

hkpWorldRayCastOutput::hkpWorldRayCastOutput() : m_rootCollidable(nullptr) {}

hkpWorldRayCastOutput::hkpWorldRayCastOutput(const hkpWorldRayCastOutput& __that)
  : hkpShapeRayCastOutput(&__that), m_rootCollidable(__that.m_rootCollidable) {}

void hkpAllRayHitCollector::reset() {
    m_hits.clear();
    hkpRayHitCollector::reset();
}

const hkArray<hkpWorldRayCastOutput, hkContainerHeapAllocator>& hkpAllRayHitCollector::getHits() const {
    return m_hits;
}

hkpRayHitCollector::hkpRayHitCollector() {
    reset();
}

hkpRayHitCollector::~hkpRayHitCollector() {}

hkpShapeRayCastCollectorOutput::hkpShapeRayCastCollectorOutput() : m_hitFraction(1.0f), m_extraInfo(-1) {}

hkpShapeRayCastCollectorOutput* hkpShapeRayCastCollectorOutput::hkpShapeRayCastCollectorOutput_() {
    this->~hkpShapeRayCastCollectorOutput();
    ::new (this) hkpShapeRayCastCollectorOutput;
    return this;
}
