#include "Fallout Shared/BGSEquipType.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/BGSMenuIcon.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectCLOT.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESTexture.h"
#include "SDK/MSVC/rtti.h"

BGSEquipType::BGSEquipType() : BaseFormComponent() {
	BGSEquipType::InitializeDataComponent();
}

void BGSEquipType::InitializeDataComponent() {
	eEquipType = ET_BIG_GUN;
}

void BGSEquipType::CopyComponent(BaseFormComponent* apCopyFC) {
	BGSEquipType* pcopy = dynamic_cast<BGSEquipType*>(apCopyFC);
	if (pcopy) {
		eEquipType = pcopy->eEquipType;
	}
}

bool BGSEquipType::CompareComponent(BaseFormComponent* apCompareFC) {
	BGSEquipType* pcompare = dynamic_cast<BGSEquipType*>(apCompareFC);
	if (!pcompare) {
		return 1;
	}
	uint32_t FormHealth_0 = eEquipType;
	return FormHealth_0 != pcompare->eEquipType;
}

void BGSEquipType::Save() {
	TESForm::AddChunk(static_cast<CHUNK_ID>(1348031557), static_cast<uint32_t>(eEquipType));
}

void BGSEquipType::GenerateDefaultMenuIcons() {
	static int32_t uiEquipTypePrefixLen = BSstrlen("MenuIconEquipType");
	for (int32_t i = 0; i < 14; ++i) {
		uint32_t iid = i + 600;
		TESForm* FormByNumericID = TESForm::GetFormByNumericID(i + 600);
		BGSMenuIcon* picon = dynamic_cast<BGSMenuIcon*>(FormByNumericID);
		if (!picon) {
			BGSMenuIcon* picon_2;
			char Dst[264];
			BGSMenuIcon* apMemory = static_cast<BGSMenuIcon*>(ourMalloc(sizeof(BGSMenuIcon)));
			if (apMemory) {
				picon_2 = ::new (apMemory) BGSMenuIcon;
			} else {
				picon_2 = 0;
			}
			picon = picon_2;
			picon_2->SetFormID(iid, 1);
			BSstrcpy(Dst, 260, "MenuIconEquipType");
			BSstrcat(Dst, 260, BGSEquipType::pEquipTypeNames[i]);
			picon->SetFormEditorID(Dst);
			BSstrcpy(Dst, 260, "TypeIcons\\hot_keys_icon_");
			BSstrcat(Dst, 260, BGSEquipType::pEquipTypeNames[i]);
			BSstrcat(Dst, 260, ".dds");
			picon->SetTextureName(Dst);
			pDataHandler->AddFormToDataHandler(picon);
		}
		BGSEquipType::pEquipTypeMenuIcons[i] = picon;
	}
}

BGSEquipType::EQUIP_TYPE BGSEquipType::GetEquipType(TESForm* apForm) {
	BGSEquipType::EQUIP_TYPE erettype = BGSEquipType::ET_NONE;
	if (apForm) {
		switch (apForm->GetFormType()) {
		case ARMO_ID:
			erettype = static_cast<BGSEquipType*>(static_cast<TESObjectARMO*>(apForm))->eEquipType;
			break;
		case CLOT_ID:
			erettype = static_cast<BGSEquipType*>(static_cast<TESObjectCLOT*>(apForm))->eEquipType;
			break;
		case INGR_ID:
			erettype = static_cast<BGSEquipType*>(static_cast<IngredientItem*>(apForm))->eEquipType;
			break;
		case WEAP_ID:
			erettype = static_cast<BGSEquipType*>(static_cast<TESObjectWEAP*>(apForm))->eEquipType;
			break;
		case ALCH_ID:
			erettype = static_cast<BGSEquipType*>(static_cast<AlchemyItem*>(apForm))->eEquipType;
			break;
		default:
			break;
		}
	}
	return erettype;
}

BGSMenuIcon* BGSEquipType::GetEquipTypeIcon(BGSEquipType::EQUIP_TYPE aeType) {
	BGSMenuIcon* picon = nullptr;
	if (aeType > ET_NONE && aeType < ET_COUNT) {
		picon = BGSEquipType::pEquipTypeMenuIcons[aeType];
	}
	return picon;
}

const char* BGSEquipType::GetEquipTypeIconFilename(EQUIP_TYPE aeType) {
	BGSMenuIcon* pIcon = GetEquipTypeIcon(aeType);
	if (pIcon)
		return pIcon->GetTextureName();
	return "";
}
