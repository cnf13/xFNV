#include "Fallout Shared/Magic/IngredientItem.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicItemObject.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout/AI/Actor.h"
#include "SDK/MSVC/rtti.h"

void IngredientItemData::Endian() {
	ENDIAN(iCostOverride, false);
}

IngredientItem::IngredientItem()
	: MagicItemObject(), TESModelTextureSwap(), TESIcon(), TESScriptableForm(), TESWeightForm(), BGSEquipType(),
	  BGSDestructibleObjectForm() {
	SetFormType(INGR_ID);
	IngredientItem::InitializeData();
}

IngredientItem::~IngredientItem() {
	IngredientItem::ClearData();
}

void IngredientItem::InitializeData() {
	data.iCostOverride = (-1);
	data.iFlags = (uint8_t)0;
	TESForm::InitializeDataComponent();
}

void IngredientItem::ClearData() {
	static_cast<EffectItemList*>(this)->RemoveAllEffects();
	TESForm::InitializeDataComponent();
}

// Resolves the loaded component forms once, unless the form is already initialised.
void IngredientItem::InitItem() {
	if (!GetInitialized()) {
		TESScriptableForm::InitItem(this);
		BGSDestructibleObjectForm::InitItem(this);
		TESModelTextureSwap::InitItem(this);
		MagicItemObject::InitItem();
	}
}

bool IngredientItem::IsAutoCalc() {
	return (bool)((uint8_t)(!IsFlagSet(COST_OVERRIDE)));
}

void IngredientItem::SetAutoCalc(bool abValue) {
	SetFlag(1, !abValue);
}

float IngredientItem::GetCost(Actor* apActor) {
	if (IsFlagSet(IngredientItem::COST_OVERRIDE)) {
		return static_cast<float>(data.iCostOverride);
	} else {
		return static_cast<EffectItemList*>(this)->EffectItemList::GetCost(0);
	}
}

int32_t IngredientItem::GetMaxEffectCount() {
	return 4;
}

void IngredientItem::Endian() {
	(&data)->Endian();
}

CHUNK_ID IngredientItem::GetChunkID() {
	return ENIT_ID;
}

uint32_t IngredientItem::GetDataSize() {
	return sizeof(IngredientItemData);
}

bool IngredientItem::CompareMagicItemData(MagicItem* apCompare) {
	IngredientItem* palchemy = dynamic_cast<IngredientItem*>(apCompare);
	if (!palchemy) {
		return 1;
	}
	int32_t MusicType = palchemy->data.iCostOverride;
	return MusicType != data.iCostOverride || palchemy->data.iFlags != data.iFlags;
}

void IngredientItem::CopyMagicItemData(MagicItem* apCopy) {
	IngredientItem* palchemy = dynamic_cast<IngredientItem*>(apCopy);
	if (palchemy) {
		data.iCostOverride = palchemy->data.iCostOverride;
		data.iFlags = palchemy->data.iFlags;
	}
}

void IngredientItem::SaveMagicItemComponents() {
	TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
	TESTexture::Save(ICON_ID);
	TESScriptableForm::Save();
	BGSDestructibleObjectForm::Save();
	BGSEquipType::Save();
	SaveData();
}

// Writes the data chunk, byte swapped around the write when saves are swapped.
void IngredientItem::SaveMagicItemData() {
	if (TESForm::QEndianSwapOnSave()) {
		data.Endian();
	}
	TESForm::__AddChunkData(GetChunkID(), &data, sizeof(data));
	if (TESForm::QEndianSwapOnSave()) {
		data.Endian();
	}
}

// Reads one chunk of the ingredient's record: the model and texture-swap chunks, the icon, the script, the equip type,
// the destruction chunks and the item's DATA (through the form's LoadData, with no size limit).
void IngredientItem::LoadMagicItemChunk(TESFile* apFile, CHUNK_ID apChunkID) {
	switch (apChunkID) {
	case MODL_ID:
	case MODD_ID:
	case MOSD_ID:
	case MODT_ID:
		TESModel::LoadModelChunk(static_cast<TESModelTextureSwap*>(this), apFile);
		break;
	case MODS_ID: {
		uint32_t uiSize = apFile->GetChunkSize();
		// A 16-byte aligned stack buffer of the chunk's size.
		char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiSize, 128));
		apFile->GetChunkData(pBuffer, uiSize);
		uint16_t usFormVersion = apFile->GetFormVersion();
		static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiSize, usFormVersion);
		break;
	}
	case ICON_ID:
		TESTexture::LoadTextureChunk(static_cast<TESIcon*>(this), apFile);
		break;
	case SCRI_ID: {
		uint32_t uiScript = 0;
		apFile->GetChunkData<uint32_t>(uiScript);
		TESScriptableForm* pScriptableForm = static_cast<TESScriptableForm*>(this);
		// Stores the chunk's value in the pointer field unchanged.
		pScriptableForm->SetFormScript(reinterpret_cast<Script*>(uiScript));
		pScriptableForm->InitItem(static_cast<TESForm*>(this));
		break;
	}
	case ETYP_ID: {
		uint32_t uiEquipType = 0;
		apFile->GetChunkData<uint32_t>(uiEquipType);
		static_cast<BGSEquipType*>(this)->SetEquipType(static_cast<BGSEquipType::EQUIP_TYPE>(uiEquipType));
		break;
	}
	case DSTD_ID:
	case DEST_ID:
		BGSDestructibleObjectForm::LoadChunk(static_cast<BGSDestructibleObjectForm*>(this), apFile);
		break;
	case DATA_ID:
		static_cast<TESForm*>(this)->LoadData(apFile, nullptr, 0);
		break;
	}
}

bool IngredientItem::IsFlagSet(IngredientFlag aiFlag) {
	return (aiFlag & data.iFlags) != 0;
}

void IngredientItem::SetFlag(char a2, char a3) {
	uint8_t v3;
	if (a3) {
		v3 = (uint8_t)(a2 | data.iFlags);
	} else {
		v3 = (uint8_t)((~a2) & data.iFlags);
	}
	data.iFlags = v3;
	uint8_t ucUnkA0 = data.iFlags;
}
