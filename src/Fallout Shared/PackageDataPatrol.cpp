#include "BSCore/NewOverloads.h"
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
    TESPatrolPackageData* pCopy = dynamic_cast<TESPatrolPackageData*>(apCopy);
    if (pCopy) {
        bRepeatable = pCopy->bRepeatable;
        bStartingLocationAtLinkedRef = pCopy->bStartingLocationAtLinkedRef;
    }
}

bool TESPatrolPackageData::Compare(TESPackageData* apCompare) {
    TESPatrolPackageData* pCompare = dynamic_cast<TESPatrolPackageData*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (bRepeatable == pCompare->bRepeatable) {
        return bStartingLocationAtLinkedRef != pCompare->bStartingLocationAtLinkedRef;
    }
    return true;
}

void TESPatrolPackageData::Save() {
    bool abData[4];
    ourMemset(abData, 0, 2u);
    abData[0] = bRepeatable;
    if (TESForm::QEndianSwapOnSave()) {}
    TESForm::__AddChunkData(PKPT_ID, abData, 2u);
    if (TESForm::QEndianSwapOnSave()) {}
}

void TESPatrolPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bRepeatable), sizeof(bRepeatable), 0);
    apSaveGameBuffer->SaveDataEndian(
        reinterpret_cast<char*>(&bStartingLocationAtLinkedRef),
        sizeof(bStartingLocationAtLinkedRef),
        0
    );
}

void TESPatrolPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bRepeatable), sizeof(bRepeatable));
    apLoadGameBuffer->LoadData(
        reinterpret_cast<char*>(&bStartingLocationAtLinkedRef),
        sizeof(bStartingLocationAtLinkedRef)
    );
}
