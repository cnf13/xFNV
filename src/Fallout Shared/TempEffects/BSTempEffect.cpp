#include "Fallout Shared/TempEffects/BSTempEffect.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "SDK/MSVC/rtti.h"

NiRTTI BSTempEffect::ms_RTTI("BSTempEffect", nullptr);
BSTempEffect::DecalList BSTempEffect::kDecals;

BSTempEffect::BSTempEffect(TESObjectCELL* apCell, float afLifetime)
  : fLifetime(afLifetime), pCell(apCell), fAge(0.0f), bInitialized(false) {}

BSTempEffect::~BSTempEffect() {
    pCell = nullptr;
    fLifetime = 0.0f;
    fAge = 0.0f;
    bInitialized = false;
}

bool BSTempEffect::Update(float afTimeDelta) {
    fAge = fAge + afTimeDelta;
    return !(fLifetime < fAge);
}

uint16_t BSTempEffect::GetSaveSize() {
    uint16_t sSize = 0;
    sSize += 4;
    sSize += 4;
    sSize += 4;
    return sSize;
}

void BSTempEffect::SaveGame() {
    pSaveLoadGameOLD->SaveGameDataOLD(&fLifetime, sizeof(fLifetime));
    pSaveLoadGameOLD->SaveGameDataOLD(&fAge, sizeof(fAge));
    int32_t iCellID = 0;
    if (pCell) {
        iCellID = pCell->GetFormID();
    }
    pSaveLoadGameOLD->SaveNumericID(reinterpret_cast<uint32_t*>(&iCellID), 4u);
}

void BSTempEffect::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fLifetime), sizeof(fLifetime), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAge), sizeof(fAge), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bInitialized), sizeof(bInitialized), 0);
}

bool BSTempEffect::LoadGame() {
    uint32_t uiFormID;
    pSaveLoadGameOLD->LoadGameDataOLD(&fLifetime, sizeof(fLifetime));
    pSaveLoadGameOLD->LoadGameDataOLD(&fAge, sizeof(fAge));
    pSaveLoadGameOLD->LoadNumericID(&uiFormID, 4u);
    TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
    pCell = dynamic_cast<TESObjectCELL*>(pForm);
    if (!pCell || !pCell->Get3D()) {
        return false;
    }
    return true;
}

void BSTempEffect::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fLifetime), sizeof(fLifetime));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAge), sizeof(fAge));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bInitialized), sizeof(bInitialized));
}
