#include "Fallout Shared/TESAmmoEffect.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
SettingT<GameSettingCollection> sAmmoEffectDAM("sAmmoEffectDAM", "DAM");
SettingT<GameSettingCollection> sAmmoEffectDR("sAmmoEffectDR", "Target DR");
SettingT<GameSettingCollection> sAmmoEffectDT("sAmmoEffectDT", "Target DT");
SettingT<GameSettingCollection> sAmmoEffectSpread("sAmmoEffectSpread", "Gun Spread");
SettingT<GameSettingCollection> sAmmoEffectCondition("sAmmoEffectCondition", "Gun CND");
SettingT<GameSettingCollection> sAmmoEffectFatigue("sAmmoEffectFatigue", "Target Fatigue");

void AMMO_EFFECT_DATA::Endian() {
	ENDIAN(iAmmoEffectType, false);
	ENDIAN(iOperation, false);
	ENDIAN(fValue, false);
}

TESAmmoEffect::TESAmmoEffect() : data{} {
	SetFormType(AMEF_ID);
}

TESAmmoEffect::~TESAmmoEffect() {}

bool TESAmmoEffect::Load(TESFile* apFile) {
	uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
	if (cFormType != AMEF_ID) {
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
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0x200);
			static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
			break;
		}
		case OBND_ID:
			static_cast<TESForm*>(this)->LoadObjectBound(apFile);
			break;
		case FULL_ID:
			TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile);
			break;
		case DATA_ID:
			// LoadData takes the raw bytes of the chunk; the DATA chunk is the AMMO_EFFECT_DATA struct.
			LoadData(apFile, reinterpret_cast<char*>(&data), 0xC);
			if (apFile->QEndian()) {
				data.Endian();
			}
			break;
		}
	} while (apFile->NextChunk());
	return true;
}

void TESAmmoEffect::Save() {
	TESForm::StartForm();
	static_cast<TESFullName*>(this)->Save();
	if (TESForm::QEndianSwapOnSave()) {
		(&data)->Endian();
	}
	TESForm::__SaveData(&data, 0xCu);
	if (TESForm::QEndianSwapOnSave()) {
		(&data)->Endian();
	}
	TESForm::CloseForm();
}

void TESAmmoEffect::Copy(TESForm* apCopy) {
	TESAmmoEffect* v3 = dynamic_cast<TESAmmoEffect*>(apCopy);
	if (v3) {
		ourMemcpy(&data.iAmmoEffectType, &v3->data,
			sizeof(AMMO_EFFECT_DATA) - offsetof(AMMO_EFFECT_DATA, iAmmoEffectType));
		TESForm::CopyAllComponents(apCopy);
	}
}

bool TESAmmoEffect::Compare(TESForm* apCompare) {
	TESForm* v4 =
		static_cast<TESForm*>(FNVD_DYNAMIC_CAST_VOID(TESAmmoEffect)(apCompare, RTTI_TESForm, RTTI_TESAmmoEffect));
	if (!v4) {
		return 1;
	}
	if (!std::memcmp(&data, &v4[1].iFormID, sizeof(data))) {
		return CompareAllComponents(v4);
	}
	return 1;
}

// Unregistered (KEEP, uninitialised read): an effect type past AMMO_EFFECT_FATIGUE takes the switch's default, and
// the exe's BSsprintf (0x59A338) then reads Dst, which nothing wrote on that path.
void TESAmmoEffect::BuildMenuString(char* astrResult, uint32_t auiResultLen) {
	char Dst[64];
	char apString1[4];
	apString1[0] = 0;
	int32_t ProcessLevel = data.iOperation;
	if (ProcessLevel) {
		if (ProcessLevel == 1) {
			BSstrcpy(apString1, 2, "x");
		} else {
			if (ProcessLevel == 2) {
				BSstrcpy(apString1, 2, "-");
			}
		}
	} else {
		BSstrcpy(apString1, 2, "+");
	}
	switch (data.iAmmoEffectType) {
	case AMMO_EFFECT_DAM: {
		const char* v3 = sAmmoEffectDAM.String();
		BSstrcpy(Dst, 64, v3);
		break;
	}
	case AMMO_EFFECT_DR: {
		const char* v4 = sAmmoEffectDR.String();
		BSstrcpy(Dst, 64, v4);
		break;
	}
	case AMMO_EFFECT_DT: {
		const char* v5 = sAmmoEffectDT.String();
		BSstrcpy(Dst, 64, v5);
		break;
	}
	case AMMO_EFFECT_SPREAD: {
		const char* v6 = sAmmoEffectSpread.String();
		BSstrcpy(Dst, 64, v6);
		break;
	}
	case AMMO_EFFECT_CONDITION: {
		const char* v7 = sAmmoEffectCondition.String();
		BSstrcpy(Dst, 64, v7);
		break;
	}
	case AMMO_EFFECT_FATIGUE: {
		const char* v8 = sAmmoEffectFatigue.String();
		BSstrcpy(Dst, 64, v8);
		break;
	}
	default: {
		break;
	}
	}
	BSsprintf(astrResult, static_cast<int32_t>(auiResultLen), "%s %s %.2f", Dst, apString1, data.fValue);
}

// The exe's cleanup-only C++ EH frame is left out.
float TESAmmoEffect::ApplyAmmoEffect(AMMO_EFFECT_TYPE aiEffectType, BSSimpleList<TESAmmoEffect*>* pEffectList,
	float afOriginalValue) {
	float fReturnValue = afOriginalValue;
	if (pEffectList) {
		BSSimpleList<TESAmmoEffect*>* pEffectsLocal;
		BSSimpleList<TESAmmoEffect*>* v6 =
			static_cast<BSSimpleList<TESAmmoEffect*>*>(ourMalloc(sizeof(BSSimpleList<TESAmmoEffect*>)));
		if (v6) {
			pEffectsLocal = ::new (v6) BSSimpleList<TESAmmoEffect*>;
		} else {
			pEffectsLocal = 0;
		}
		BSSimpleList<TESAmmoEffect*>* pEffectsLocal_2 = pEffectsLocal;
		while (pEffectList && pEffectList->m_item) {
			if (pEffectList->m_item->data.iOperation == AMMO_OPERATION_TIMES) {
				pEffectsLocal->AddHead(pEffectList->m_item);
			} else {
				pEffectsLocal->AddTail(pEffectList->m_item);
			}
			pEffectList = pEffectList->m_pkNext;
		}
		while (pEffectsLocal_2 && pEffectsLocal_2->m_item) {
			TESAmmoEffect* pEffect = pEffectsLocal_2->m_item;
			pEffectsLocal_2 = pEffectsLocal_2->m_pkNext;
			AMMO_EFFECT_TYPE iEffect = pEffect->data.iAmmoEffectType;
			AMMO_OPERATION_TYPE iOperation = pEffect->data.iOperation;
			float fEffectModifier = pEffect->data.fValue;
			if (iEffect == aiEffectType) {
				if (iOperation) {
					if (iOperation == AMMO_OPERATION_TIMES) {
						fReturnValue = fReturnValue * fEffectModifier;
					} else {
						if (iOperation == AMMO_OPERATION_MINUS) {
							fReturnValue = fReturnValue - fEffectModifier;
						}
					}
				} else {
					fReturnValue = fReturnValue + fEffectModifier;
				}
			}
		}
		// The exe deletes the walking pointer, not the list it allocated: null, or the node whose item ended the walk.
		if (pEffectsLocal_2) {
			delete pEffectsLocal_2;
		}
		return fReturnValue;
	} else {
		return afOriginalValue;
	}
}
