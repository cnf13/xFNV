#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"

void PatrolRefData::Copy(PatrolRefData* apCopy) {
    fTimeAtRef = apCopy->fTimeAtRef;
    OnPatrol.Copy(&apCopy->OnPatrol);
}

// The patrol action's topic.
TESTopic* PatrolRefData::GetOnPatrolTopic() {
    return OnPatrol.QTopic();
}

bool PatrolRefData::Compare(PatrolRefData* apCompare) {
    return OnPatrol.Compare(&apCompare->OnPatrol) || apCompare->fTimeAtRef != fTimeAtRef;
}

void TESPatrolPackageData::Copy(TESPackageData* apCopy) {
    TESPatrolPackageData* v3 = dynamic_cast<TESPatrolPackageData*>(apCopy);
    if (v3) {
        bRepeatable = (bool)((int8_t)v3->bRepeatable);
        bStartingLocationAtLinkedRef = (bool)((int8_t)v3->bStartingLocationAtLinkedRef);
    }
}

bool TESPatrolPackageData::Compare(TESPackageData* apCompare) {
    TESPatrolPackageData* v4 = dynamic_cast<TESPatrolPackageData*>(apCompare);
    if (!v4) {
        return (bool)1;
    }
    if (((uint32_t)((uint8_t)bRepeatable)) == ((uint32_t)((int8_t)v4->bRepeatable))) {
        return (bool)((uint8_t)(((uint32_t)((uint8_t)bStartingLocationAtLinkedRef)) !=
                                ((uint32_t)((int8_t)v4->bStartingLocationAtLinkedRef))));
    }
    return (bool)1;
}

void TESPatrolPackageData::Save() {
    bool th_s[4];
    ourMemset(th_s, 0, 2u);
    th_s[0] = bRepeatable;
    if (TESForm::QEndianSwapOnSave()) {}
    TESForm::__AddChunkData(PKPT_ID, th_s, 2u);
    if (TESForm::QEndianSwapOnSave()) {}
}

void TESPatrolPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bRepeatable), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bStartingLocationAtLinkedRef), 1, 0);
}

void TESPatrolPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bRepeatable), 1);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bStartingLocationAtLinkedRef), 1);
}
