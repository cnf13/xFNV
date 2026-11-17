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
	fAge = (float)(fAge + afTimeDelta);
	return (bool)((uint8_t)(!(fLifetime < ((double)fAge))));
}

uint16_t BSTempEffect::GetSaveSize() {
	uint16_t sSize = 0;
	sSize += 4;
	sSize += 4;
	sSize += 4;
	return sSize;
}

void BSTempEffect::SaveGame() {
	int32_t a2;
	pSaveLoadGameOLD->SaveGameDataOLD(&fLifetime, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fAge, 4u);
	a2 = 0;
	if (pCell) {
		a2 = pCell->GetFormID();
	}
	pSaveLoadGameOLD->SaveNumericID(reinterpret_cast<uint32_t*>(&a2), 4u);
}

void BSTempEffect::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fLifetime), 4, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fAge), 4, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bInitialized), 1, 0);
}

bool BSTempEffect::LoadGame() {
	uint32_t apFormID;
	bool v4 = 1;
	pSaveLoadGameOLD->LoadGameDataOLD(&fLifetime, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fAge, 4u);
	pSaveLoadGameOLD->LoadNumericID(&apFormID, 4u);
	TESForm* FormByNumericID = TESForm::GetFormByNumericID(apFormID);
	pCell = dynamic_cast<TESObjectCELL*>(FormByNumericID);
	if (!pCell || !pCell->Get3D()) {
		return 0;
	}
	return v4;
}

void BSTempEffect::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fLifetime), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fAge), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bInitialized), 1);
}
