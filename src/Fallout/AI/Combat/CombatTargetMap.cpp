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

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
SettingT<INISettingCollection> bDebugCombatTargets("bDebugCombatTargets:Combat", false);

// The exe's cleanup-only C++ EH frame is left out.
CombatTargetData::CombatTargetData() : Attackers(5u, 0u), spDebugGeometry(nullptr) {}

// The exe's cleanup-only C++ EH frame is left out.
CombatTargetData::~CombatTargetData() {
	if (spDebugGeometry.m_pObject && spDebugGeometry.m_pObject->m_pkParent) {
		if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
			TaskQueueInterface::GetInstance()->QueueNodeDetach(spDebugGeometry.m_pObject);
		} else {
			spDebugGeometry.m_pObject->m_pkParent->DetachChild(spDebugGeometry.m_pObject);
		}
	}
}

CombatTargetMap::CombatTargetMap() : NiTMap<Actor*, CombatTargetData*>(0x25u) {}

// The exe's cleanup-only C++ EH frame is left out.
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
	CombatTargetData* a2 = 0;
	if (GetAt(apTarget, a2)) {
		RemoveAt(apTarget);
		if (a2) {
			delete a2;
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void CombatTargetMap::AddTargetAttacker(Actor* apTarget, Actor* apAttacker) {
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, tempMemContext, &AutoMemContext::Destruct, MEM_CONTEXT_COMBAT_AI, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\Combat\\CombatTargetMap.cpp", 124);
	CombatTargetData* ptargetData = 0;
	if (!GetAt(apTarget, ptargetData)) {
		CombatTargetData* ptargetData_2;
		CombatTargetData* v5 = static_cast<CombatTargetData*>(ourMalloc(sizeof(CombatTargetData)));
		if (v5) {
			ptargetData_2 = ::new (v5) CombatTargetData;
		} else {
			ptargetData_2 = 0;
		}
		ptargetData = ptargetData_2;
		SetAt(apTarget, ptargetData_2);
	}
	ptargetData->Attackers.Add(apAttacker);
}

void CombatTargetMap::RemoveTargetAttacker(Actor* apTarget, Actor* apAttacker) {
	CombatTargetData* ptargetData = 0;
	if (GetAt(apTarget, ptargetData)) {
		for (uint32_t i = 0; i < ptargetData->Attackers.iSize; ++i) {
			if (ptargetData->Attackers.operator[](i) == apAttacker) {
				if (ptargetData->Attackers.iSize == 1) {
					if (ptargetData) {
						delete ptargetData;
					}
					RemoveAt(apTarget);
				} else {
					ptargetData->Attackers.RemoveFast(i, 1u);
				}
				return;
			}
		}
	}
}

CombatTargetData* CombatTargetMap::GetTargetAttackerArray(Actor* apTarget) {
	CombatTargetData* ptargetData = 0;
	if (GetAt(apTarget, ptargetData)) {
		return ptargetData;
	}
	return 0;
}
