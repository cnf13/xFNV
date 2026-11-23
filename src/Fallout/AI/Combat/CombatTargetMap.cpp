#include "Fallout/AI/Combat/CombatTargetMap.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bDebugCombatTargets("bDebugCombatTargets:Combat", false);

CombatTargetData::CombatTargetData() : Attackers(5u, 0u), spDebugGeometry(nullptr) {}

CombatTargetData::~CombatTargetData() {
    if (spDebugGeometry.m_pObject && spDebugGeometry->m_pkParent) {
        if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
            TaskQueueInterface::GetInstance()->QueueNodeDetach(spDebugGeometry.m_pObject);
        }
        else {
            spDebugGeometry->m_pkParent->DetachChild(spDebugGeometry.m_pObject);
        }
    }
}

CombatTargetMap::CombatTargetMap() : NiTMap<Actor*, CombatTargetData*>(0x25u) {}

CombatTargetMap::~CombatTargetMap() {
    NiTMapItem<Actor*, CombatTargetData*>* it = GetFirstPos();
    while (it) {
        Actor* ptarget = nullptr;
        CombatTargetData* ptargetData = nullptr;
        GetNext(it, ptarget, ptargetData);
        delete ptargetData;
    }
    RemoveAll();
}

void CombatTargetMap::RemoveTarget(Actor* apTarget) {
    CombatTargetData* ptargetData = nullptr;
    if (GetAt(apTarget, ptargetData)) {
        RemoveAt(apTarget);
        if (ptargetData) {
            delete ptargetData;
        }
    }
}

void CombatTargetMap::AddTargetAttacker(Actor* apTarget, Actor* apAttacker) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_COMBAT_AI,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\CombatTargetMap.cpp",
        124
    );
    CombatTargetData* ptargetData = nullptr;
    if (!GetAt(apTarget, ptargetData)) {
        CombatTargetData* pNewData;
        pNewData = new CombatTargetData;
        ptargetData = pNewData;
        SetAt(apTarget, pNewData);
    }
    ptargetData->Attackers.Add(apAttacker);
}

void CombatTargetMap::RemoveTargetAttacker(Actor* apTarget, Actor* apAttacker) {
    CombatTargetData* ptargetData = nullptr;
    if (GetAt(apTarget, ptargetData)) {
        for (uint32_t i = 0; i < ptargetData->Attackers.iSize; ++i) {
            if (ptargetData->Attackers[i] == apAttacker) {
                if (ptargetData->Attackers.iSize == 1) {
                    if (ptargetData) {
                        delete ptargetData;
                    }
                    RemoveAt(apTarget);
                }
                else {
                    ptargetData->Attackers.RemoveFast(i, 1u);
                }
                return;
            }
        }
    }
}

CombatTargetData* CombatTargetMap::GetTargetAttackerArray(Actor* apTarget) {
    CombatTargetData* ptargetData = nullptr;
    if (GetAt(apTarget, ptargetData)) {
        return ptargetData;
    }
    return nullptr;
}
