#include "Fallout Shared/TESObjectCLOT.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSAddonNode.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"

TESObjectCLOT::TESObjectCLOT() {
	SetFormType(26u);
	TESObjectCLOT::InitializeData();
}

TESObjectCLOT::~TESObjectCLOT() {}

void TESObjectCLOT::InitializeData() {
	static_cast<TESEnchantableForm*>(this)->TESEnchantableForm::SetCastingType(MagicSystem::CONSTANT_EFFECT);
}

// Reads the clothing's chunks: its name, script, enchantment and biped model components, the destructible data and the
// equip type. A DATA chunk's contents are not handled here. The editor ID buffer is emptied before the chunk is read:
// GetChunkData writes at most the chunk's bytes, and a zero-size chunk leaves the buffer unwritten, so uninitialised
// data would otherwise be read as the editor ID.
bool TESObjectCLOT::Load(TESFile* apFile) {
	if ((uint8_t)apFile->GetTESForm() != 26) {
		return false;
	}
	LoadForm(apFile);
	SetInitialized(false);
	do {
		uint32_t uiChunk = apFile->GetTESChunk();
		if (uiChunk == NO_CHUNK) {
			break;
		}
		switch (uiChunk) {
		case EDID_ID: {
			// A 16-byte aligned buffer of the chunk's size.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			// the empty string, for a chunk that writes nothing
			pBuffer[0] = '\0';
			apFile->GetChunkData(pBuffer, 0x200);
			SetFormEditorID(pBuffer);
			break;
		}
		case OBND_ID:
			LoadObjectBound(apFile);
			break;
		case FULL_ID:
			TESFullName::LoadFullNameChunk(this, apFile);
			break;
		case SCRI_ID: {
			uint32_t uiScriptID = 0;
			apFile->GetChunkData(uiScriptID);
			TESScriptableForm* pScriptableForm = static_cast<TESScriptableForm*>(this);
			pScriptableForm->SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiScriptID)));
			pScriptableForm->InitItem(this);
			break;
		}
		case ENAM_ID:
		case EITM_ID: {
			uint32_t uiEnchantmentID = 0;
			apFile->GetChunkData(uiEnchantmentID);
			static_cast<TESEnchantableForm*>(this)
				->SetFormEnchanting(reinterpret_cast<EnchantmentItem*>(static_cast<uintptr_t>(uiEnchantmentID)));
			break;
		}
		case ANAM_ID:
		case EAMT_ID: {
			int16_t sAmount = 0;
			apFile->LoadWord(&sAmount);
			static_cast<TESEnchantableForm*>(this)->SetFormEnchantmentAmount((uint16_t)sAmount);
			break;
		}
		case BMDT_ID:
			if (apFile->GetFormVersion() >= 0xBu) {
				apFile->GetChunkData(&bipedModelData, 8u);
				if (apFile->QEndian()) {
					bipedModelData.Endian();
				}
			} else {
				ADDON_DATA oldData;
				apFile->GetChunkData(&oldData, 4u);
				if (apFile->QEndian()) {
					oldData.Endian();
				}
				bipedModelData.sFlags = oldData.cFlags;
				bipedModelData.iBipedObjectSlots = oldData.usMasterParticleCap;
			}
			bipedModelData.sFlags &= ~0x10u;
			break;
		case MODL_ID: {
			// A 16-byte aligned buffer of the chunk's size.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0);
			TESModel* pModel = static_cast<TESBipedModelForm*>(this)->bipedModel;
			pModel->SetModel(pBuffer);
			break;
		}
		case MOD2_ID: {
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0);
			TESModel* pModel = static_cast<TESBipedModelForm*>(this)->worldModel;
			pModel->SetModel(pBuffer);
			break;
		}
		case MOD3_ID: {
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0);
			TESModel* pModel = static_cast<TESBipedModelForm*>(this)->bipedModel + 1;
			pModel->SetModel(pBuffer);
			break;
		}
		case MOD4_ID: {
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0);
			TESModel* pModel = static_cast<TESBipedModelForm*>(this)->worldModel + 1;
			pModel->SetModel(pBuffer);
			break;
		}
		case BMCT_ID: {
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0);
			TESModel* pModel = &static_cast<TESBipedModelForm*>(this)->ConstraintTemplate;
			pModel->SetModel(pBuffer);
			break;
		}
		case MODT_ID:
			TESModel::LoadModelTextureChunk(static_cast<TESBipedModelForm*>(this)->bipedModel, apFile);
			break;
		case MO2T_ID:
			TESModel::LoadModelTextureChunk(static_cast<TESBipedModelForm*>(this)->worldModel, apFile);
			break;
		case MO3T_ID:
			TESModel::LoadModelTextureChunk(static_cast<TESBipedModelForm*>(this)->bipedModel + 1, apFile);
			break;
		case MO4T_ID:
			TESModel::LoadModelTextureChunk(static_cast<TESBipedModelForm*>(this)->worldModel + 1, apFile);
			break;
		case MODD_ID:
			TESModel::LoadModelChunk(static_cast<TESBipedModelForm*>(this)->bipedModel, apFile);
			break;
		case MOSD_ID:
			TESModel::LoadModelChunk(static_cast<TESBipedModelForm*>(this)->bipedModel + 1, apFile);
			break;
		case MODS_ID:
			TESModelTextureSwap::LoadModelTextureSwapChunk(static_cast<TESBipedModelForm*>(this)->bipedModel, apFile);
			break;
		case MO2S_ID:
			TESModelTextureSwap::LoadModelTextureSwapChunk(static_cast<TESBipedModelForm*>(this)->worldModel, apFile);
			break;
		case MO3S_ID:
			TESModelTextureSwap::LoadModelTextureSwapChunk(static_cast<TESBipedModelForm*>(this)->bipedModel + 1,
				apFile);
			break;
		case MO4S_ID:
			TESModelTextureSwap::LoadModelTextureSwapChunk(static_cast<TESBipedModelForm*>(this)->worldModel + 1,
				apFile);
			break;
		case ICON_ID: {
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0);
			static_cast<TESBipedModelForm*>(this)->inventoryIcon[0].SetTextureName(pBuffer);
			break;
		}
		case ICO2_ID: {
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0);
			static_cast<TESBipedModelForm*>(this)->inventoryIcon[1].SetTextureName(pBuffer);
			break;
		}
		case MICO_ID: {
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0);
			static_cast<TESBipedModelForm*>(this)->messageIcon[0].SetMessageIconTextureName(pBuffer);
			break;
		}
		case MIC2_ID: {
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0);
			static_cast<TESBipedModelForm*>(this)->messageIcon[1].SetMessageIconTextureName(pBuffer);
			break;
		}
		case DSTD_ID:
		case DEST_ID:
			BGSDestructibleObjectForm::LoadChunk(this, apFile);
			break;
		case ETYP_ID: {
			uint32_t uiEquipType = 0;
			apFile->GetChunkData(uiEquipType);
			static_cast<BGSEquipType*>(this)->SetEquipType((BGSEquipType::EQUIP_TYPE)uiEquipType);
			break;
		}
		case DATA_ID:
			LoadData(apFile, nullptr, 0);
			break;
		default:
			break;
		}
	} while (apFile->NextChunk());
	return true;
}

void TESObjectCLOT::Save() {
	StartForm();
	TESFullName::Save();
	TESScriptableForm::Save();
	TESEnchantableForm::SaveSubRecord();
	TESBipedModelForm::Save();
	BGSDestructibleObjectForm::Save();
	BGSEquipType::Save();
	SaveData();
	CloseForm();
}

uint16_t TESObjectCLOT::GetSaveSize(uint32_t aiFlags) {
	uint16_t saveSize = TESForm::GetSaveSize(aiFlags);
	return (uint16_t)(static_cast<TESValueForm*>(this)->TESValueForm::GetSaveSize(aiFlags) + saveSize);
}

void TESObjectCLOT::SaveGame(uint32_t aiFlags) {
	TESForm::SaveGame(aiFlags);
	static_cast<TESValueForm*>(this)->TESValueForm::SaveGame(aiFlags);
}

void TESObjectCLOT::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	TESForm::SaveGame(apSaveGameBuffer);
	static_cast<TESValueForm*>(this)->TESValueForm::SaveGame(apSaveGameBuffer);
}

void TESObjectCLOT::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	TESForm::LoadGame(aiFlags, aiCurrentFlags);
	static_cast<TESValueForm*>(this)->TESValueForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESObjectCLOT::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESForm::LoadGame(apLoadGameBuffer);
	static_cast<TESValueForm*>(this)->TESValueForm::LoadGame(apLoadGameBuffer);
}

void TESObjectCLOT::InitItem() {
	const char* v1;
	if (!TESForm::GetInitialized()) {
		if (!bipedModelData.iBipedObjectSlots) {
			v1 = GetFormEditorID();
			Warning("MASTERFILE: Clothing '%s' needs to have biped slots selected in the editor.", v1);
		}
		static_cast<TESScriptableForm*>(this)->InitItem(this);
		static_cast<TESEnchantableForm*>(this)->InitItem(this);
		static_cast<BGSDestructibleObjectForm*>(this)->InitItem(this);
		static_cast<TESBipedModelForm*>(this)->InitItem(this);
		TESForm::SetInitialized(1);
	}
}

void TESObjectCLOT::Copy(TESForm* apCopy) {
	if (dynamic_cast<TESObjectCLOT*>(apCopy)) {
		TESForm::CopyAllComponents(apCopy);
	}
}

bool TESObjectCLOT::Compare(TESForm* apCompare) {
	TESObjectCLOT* v4 = dynamic_cast<TESObjectCLOT*>(apCompare);
	return ((!v4) || (((int8_t)TESForm::CompareAllComponents(v4)) != 0));
}
