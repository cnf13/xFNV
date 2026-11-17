#include "Fallout Shared/Magic/EffectSettingCollection.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "SDK/MSVC/rtti.h"

EffectSettingCollection EffectSettingCollection::Instance;

EffectSettingCollection::~EffectSettingCollection() {
	ClearData();
}

void EffectSettingCollection::ClearData() {
	for (BSSimpleList<EffectSetting*>* pnode = this; pnode; pnode = pnode->m_pkNext) {
		EffectSetting* psetting = pnode->m_item;
		if (!psetting) {
			break;
		}
		delete psetting;
	}
	RemoveAll();
}

void EffectSettingCollection::AddEffect(EffectSetting* apSetting) {
	if (apSetting) {
		(&EffectSettingCollection::Instance)->AddHead(apSetting);
	}
}

void EffectSettingCollection::InitItems() {
	for (EffectSetting* v1 : EffectSettingCollection::Instance.ItemsUntilNull()) {
		v1->InitItem();
	}
}

EffectSettingCollection* EffectSettingCollection::Construct(void* apMemory) {
	EffectSettingCollection* pThis = static_cast<EffectSettingCollection*>(apMemory);
	::new (pThis) BSSimpleList<EffectSetting*>;
	return pThis;
}

// The exe's cleanup-only C++ EH frame is left out.
EffectSetting* EffectSettingCollection::CreateDefaultEffect(uint32_t aiFormID, const char* acEditorID,
	const char* apEffectName, ActorValue::Index aiAssociatedSkill, float afBaseCost,
	ActorValue::Index aeAssociatedActorVaue, uint32_t aiFlags, ActorValue::Index aiResistVariable,
	int32_t aiEffectArchetype) {
	EffectSetting* psettings;
	if (TESForm::GetFormByNumericID(aiFormID)) {
		return 0;
	}
	EffectSetting* apMemory = static_cast<EffectSetting*>(ourMalloc(sizeof(EffectSetting)));
	if (apMemory) {
		psettings = ::new (apMemory) EffectSetting;
	} else {
		psettings = 0;
	}
	psettings->SetFormID(aiFormID, 1);
	psettings->SetFormEditorID(acEditorID);
	psettings->data.iFlags = aiFlags;
	psettings->SetEffectName(apEffectName);
	psettings->SetBaseCost(afBaseCost);
	psettings->data.eAssociatedActorValue = aeAssociatedActorVaue;
	psettings->data.iAssociatedSkill = aiAssociatedSkill;
	psettings->data.iResistVariable = aiResistVariable;
	psettings->SetEffectArchetype(aiEffectArchetype);
	EffectSettingCollection::AddEffect(psettings);
	return psettings;
}

// The flags and the fire damage's resist variable (0x34) are the exe's pushed immediates.
void EffectSettingCollection::CreateDefaultEffects() {
	CreateDefaultEffect(0x14A, "ScriptEffect", "Script Effect", static_cast<ActorValue::Index>(-1), 0.0f,
		static_cast<ActorValue::Index>(-1), 0x170, static_cast<ActorValue::Index>(-1), 1);
	CreateDefaultEffect(0x14B, "FireDamage", "Fire Damage", static_cast<ActorValue::Index>(-1), 0.0f,
		static_cast<ActorValue::Index>(0x10), 0x20000475, static_cast<ActorValue::Index>(0x34), 0);
	CreateDefaultEffect(0x14C, "FrostDamage", "Frost Damage", static_cast<ActorValue::Index>(-1), 0.0f,
		static_cast<ActorValue::Index>(0x10), 0x20000475, static_cast<ActorValue::Index>(-1), 0);
	CreateDefaultEffect(0x14D, "ShockDamage", "Shock Damage", static_cast<ActorValue::Index>(-1), 0.0f,
		static_cast<ActorValue::Index>(0x10), 0x20000475, static_cast<ActorValue::Index>(-1), 0);
	CreateDefaultEffect(0x14E, "RestoreHealth", "Restore Health", static_cast<ActorValue::Index>(-1), 0.0f,
		static_cast<ActorValue::Index>(0x10), 0x70, static_cast<ActorValue::Index>(-1), 0);
	pUsageMonitorEffectSetting =
		CreateDefaultEffect(0x14F, "UMON", "Usage Monitor Effect", static_cast<ActorValue::Index>(-1), 0.0f,
			static_cast<ActorValue::Index>(-1), 0x12, static_cast<ActorValue::Index>(-1), 1);
}

EffectSetting* EffectSettingCollection::GetUsageMonitorEffectSetting() {
	return pUsageMonitorEffectSetting;
}

void EffectSettingCollection::Reset() {
	static_cast<EffectSettingCollection*>(&EffectSettingCollection::Instance)->ClearData();
}

int32_t EffectSettingCollection::GetKnownEffectsSaveSize() {
	int32_t v5 = 0;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		v5 = 6;
	}
	int32_t isize = v5 + 4;
	for (EffectSetting* psetting : EffectSettingCollection::Instance.ItemsUntilNull()) {
		if (psetting->IsFlagSet(EffectSetting::PLAYER_KNOWS)) {
			isize += 4;
		}
	}
	if (bDebugSaveBuffer.Bool()) {
		SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pheader) {
			TESForm* FormByNumericID = TESForm::GetFormByNumericID(pheader->iFormID);
			InitializationFlow("GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				isize, pheader->iFormID, FormByNumericID->GetFormEditorID(), pheader->iFlags, 119,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Magic\\EffectSettingCollection.cpp");
		} else {
			InitializationFlow("GetSaveSize(): %-5i ending at line %i in file %s", isize, 119,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Magic\\EffectSettingCollection.cpp");
		}
	}
	return isize;
}

const char* TESFullName::GetFullName() {
	if (cFullName.pString) {
		return cFullName.pString;
	} else {
		return "";
	}
}

const char* TESFullName::GetFullName(TESForm* apObj) {
	TESFullName* pFullName = dynamic_cast<TESFullName*>(apObj);
	if (pFullName) {
		return pFullName->GetFullName();
	} else {
		return "";
	}
}
