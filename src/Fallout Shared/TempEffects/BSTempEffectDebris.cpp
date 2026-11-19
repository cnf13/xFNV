#include "Fallout Shared/TempEffects/BSTempEffectDebris.h"

#include "BSCore/BSList.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout Shared/TempEffects/BSTempEffectSimpleDecal.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

NiRTTI BSTempEffectDebris::ms_RTTI("BSTempEffectDebris", &BSTempEffect::ms_RTTI);
BSList<BSTempEffectDebris*> BSTempEffectDebris::DebrisLimitList;

BSTempEffectDebris::BSTempEffectDebris(
    TESObjectCELL* apCell,
    float afLifetime,
    const char* apFilename,
    TESObjectREFR* apSourceRef,
    const NiPoint3& aPosition,
    NiMatrix3& aRotation,
    NiPoint3& aStartLinearVelocity,
    NiPoint3& aStartAngularVelocity,
    float afScale,
    bool bUseCache,
    bool abAddDebrisCount
)
  : BSTempEffect(apCell, afLifetime), sp3D(nullptr), pFilename(apFilename), bUseDebrisCounter(abAddDebrisCount),
    bForceDelete(false) {
    Init(apSourceRef, aPosition, aRotation, aStartLinearVelocity, aStartAngularVelocity, afScale, bUseCache);
}

BSTempEffectDebris::BSTempEffectDebris(
    TESObjectCELL* apCell,
    float afLifetime,
    const char* apFilename,
    TESObjectREFR* apSourceRef,
    const NiPoint3& aPosition,
    NiPoint3& aDirection,
    NiPoint3& aStartLinearVelocity,
    NiPoint3& aStartAngularVelocity,
    float afScale,
    bool bUseCache,
    bool abAddDebrisCount
)
  : BSTempEffect(apCell, afLifetime), sp3D(nullptr), pFilename(apFilename), bUseDebrisCounter(abAddDebrisCount),
    bForceDelete(false) {
    NiMatrix3 particleRot;
    NiPoint3 dir = aDirection;
    float fz = dir.z;
    dir.z = 0.0f;
    float fxy = dir.Length();
    float fXAngle = -NiATan2(fz, fxy);
    float fZAngle = NiATan2(dir.x, dir.y);
    particleRot.FromEulerAnglesZYX(fZAngle, 0.0f, fXAngle);
    Init(apSourceRef, aPosition, particleRot, aStartLinearVelocity, aStartAngularVelocity, afScale, bUseCache);
}

BSTempEffectDebris::~BSTempEffectDebris() {
    if (pFilename) {
        pModelLoader->ReleaseModel(pFilename, false, 1u);
    }
    if ((sp3D.m_pObject) && (sp3D.m_pObject->m_pkParent)) {
        NiNode* pParent = sp3D.m_pObject->m_pkParent;
        pParent->DetachChild(sp3D.m_pObject);
    }

    if (iNumHWThreads == 1) {
        bhkWorld::Activate(sp3D.m_pObject, true, true, false);
        bhkWorld::RemoveObjects(sp3D.m_pObject, true, false);
    }
    else {
        NiAVObject* pObject = sp3D.m_pObject;
        TaskQueueInterface* pInstance = TaskQueueInterface::GetInstance();
        pInstance->QueueHavokRemove(pObject);
    }
    if (bUseDebrisCounter) {
        BSTempEffectDebris::RemoveFromDebrisCounter(this);
    }
    sp3D = static_cast<NiAVObject*>(nullptr);
}

const NiRTTI* BSTempEffectDebris::GetRTTI() const {
    return &BSTempEffectDebris::ms_RTTI;
}

bool BSTempEffectDebris::Update(float afTimeDelta) {
    bool bIsAlive = BSTempEffect::Update(afTimeDelta);
    if (bForceDelete) {
        bIsAlive = false;
    }
    if (bIsAlive) {
        double Time = GetLifetime();
        float v8 = Time - GetAge();
        if (v8 >= 0.0 && fDebrisFadeTime.Float() >= static_cast<double>(v8)) {
            NiNode* v3;
            NiAVObject* v7 = Get3D();
            if (v7) {
                v3 = v7->IsNode();
            }
            else {
                v3 = nullptr;
            }
            if (v3) {
                float v6 = v8 / fDebrisFadeTime.Float();
                BSShaderUtil::RecursiveSetPropertyFadeAlpha(v3, v6);
            }
        }
    }
    return bIsAlive;
}

NiAVObject* BSTempEffectDebris::Get3D() {
    return (&sp3D)->operator->();
}

TEMP_EFFECT_TYPE BSTempEffectDebris::GetType() {
    return TEMP_EFFECT_PARTICLE;
}

bool BSTempEffectDebris::IsFirstPerson() {
    return b1stPerson;
}

void BSTempEffectDebris::SetForceDelete(bool abValue) {
    bForceDelete = (bool)((uint8_t)abValue);
}

void BSTempEffectDebris::SetIs1stPerson(bool abValue) {
    b1stPerson = (bool)((uint8_t)abValue);
}

void BSTempEffectDebris::Init(
    TESObjectREFR* apSourceRef,
    const NiPoint3& aPosition,
    NiMatrix3& aRotation,
    NiPoint3& aStartLinearVelocity,
    NiPoint3& aStartAngularVelocity,
    float afScale,
    bool bUseCache
) {
    SetIs1stPerson(false);
    NiAVObject* pmodel = pModelLoader->LoadFile(pFilename, LOD_MULT_NONE, true, false, false, false);
    if (bUseCache) {
        if (TES::pInstance) {
            TES::pInstance->GetUnusedCachedParticleObject(pmodel, sp3D);
        }
        else {
            sp3D = static_cast<NiAVObject*>(nullptr);
        }
    }
    else {
        BSCloningProcess cloneProcess(1.0f);
        sp3D = static_cast<NiAVObject*>(pmodel->Clone(cloneProcess));
    }
    if ((sp3D.m_pObject) && (pCell)) {
        NiNode* psceneParent = static_cast<NiNode*>(pCell->GetDynamicNode());
        if (psceneParent) {
            NiAVObject* v8 = sp3D.m_pObject;
            psceneParent->AttachChild(v8, 1);
        }
        sp3D.m_pObject->SetTranslate(aPosition);
        sp3D.m_pObject->SetRotate(aRotation);
        sp3D.m_pObject->SetScale(afScale);
        BSShaderManager::SetObjectTypeRecurse(sp3D.m_pObject, BSShaderManager::BSSM_OBJ_MISC);
        InitHavok(apSourceRef);
        sp3D.m_pObject->UpdateProperties();
        NiUpdateData UpdateData(fCellAppTime, true, false);
        sp3D.m_pObject->Update(UpdateData);
        TESHavokUtilities::AddVelocity(sp3D.m_pObject, aStartLinearVelocity, true);
        TESHavokUtilities::AddAngularVelocity(sp3D.m_pObject, aStartAngularVelocity, true);
        ReferenceProcessLists.AddTempEffect(this);
        if (bUseDebrisCounter) {
            BSTempEffectDebris::AddtoDebrisCounter(this);
            BSTempEffectDebris::LimitDebrisObjects();
        }
    }
}

void BSTempEffectDebris::InitHavok(TESObjectREFR* apSourceRef) {
    NiAVObject* proot = Get3D();
    NiAVObject* pav = proot;
    if (proot) {
        NiNode* pn = static_cast<NiNode*>(NiObject::DynamicCastStatic(&BSFadeNode::ms_RTTI, proot));
        if (pn) {
            pav = pn->GetAtChecked(0);
        }
        else {
            pn = proot->IsNode();
            if ((!bhkNiCollisionObject::GetbhkNiCollisionObject(pav)) && (pn) && (pn->GetArrayCount()) &&
                (pn->GetAt(0))) {
                pav = pn->GetAt(0);
                bhkNiCollisionObject* pcol = bhkNiCollisionObject::GetbhkNiCollisionObject(pav);
                if ((!pcol && pn->GetArrayCount() > 1) && (pn->GetAt(1u))) {
                    pav = pn->GetAt(1u);
                }
            }
        }
        if (pav) {
            TESObjectCELL* pcell = pCell;
            if (pcell) {
                bhkWorld* pw = pcell->GetbhkWorld();
                if (pw) {
                    if ((apSourceRef) && (apSourceRef->IsActor())) {
                        // GetCollisionFilter fills kFilter and returns its address, which GetGroup is called on.
                        CFilter kFilter;
                        static_cast<MobileObject*>(apSourceRef)->MobileObject::GetCollisionFilter(&kFilter.iFilter);
                        uint32_t Group = kFilter.GetGroup();
                        bhkWorld::SetGroup(pav, Group, true, true);
                    }

                    pw->AddObjectsAndMoveController(pav, proot);
                }
            }
        }
    }
}

void BSTempEffectDebris::AddtoDebrisCounter(BSTempEffectDebris* abDebrisObject) {
    if (abDebrisObject) {
        BSTempEffectDebris::DebrisLimitList.AddHead(abDebrisObject);
    }
}

void BSTempEffectDebris::RemoveFromDebrisCounter(BSTempEffectDebris* abDebrisObject) {
    if ((abDebrisObject) && (!DebrisLimitList.IsEmpty())) {
        bool bFound = false;
        void* iter = DebrisLimitList.m_pkTail;
        while (iter && !bFound) {
            if (DebrisLimitList.Get(iter) == abDebrisObject) {
                DebrisLimitList.RemovePos(iter);
                bFound = true;
            }
            else {
                DebrisLimitList.GetPrev(iter);
            }
        }
    }
}

void BSTempEffectDebris::LimitDebrisObjects() {
    if ((!DebrisLimitList.IsEmpty()) && (DebrisLimitList.m_uiCount > iDebrisMaxCount.Int())) {
        int32_t ikill = DebrisLimitList.m_uiCount - iDebrisMaxCount.Int();
        for (void* iter = DebrisLimitList.m_pkTail; iter && ikill > 0; iter = DebrisLimitList.GetPrevPos(iter)) {
            BSTempEffectDebris* pdebris = DebrisLimitList.Get(iter);
            if (pdebris) {
                pdebris->SetForceDelete(true);
                --ikill;
            }
        }
    }
}
