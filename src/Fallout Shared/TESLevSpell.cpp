#include "Fallout Shared/TESLevSpell.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTaskManager.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSTextureUseMap.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/LEVELED_OBJECT_FILE.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Interface/Interface.h"
#include "SDK/MSVC/rtti.h"

int32_t retn_minusOne() {
	return (-1);
}

// The entry the last LVLO chunk created; the COED chunk that follows gives it its extra data.
TESLevSpell::TESLevSpell() : TESBoundObject(), TESLeveledList() {
	SetFormType(0x4Cu);
	SetInitialized(1);
}

TESLevSpell::~TESLevSpell() {
	TESForm::InitializeDataComponent();
}

// Reads the LVSP record: the leveled list chunks and the object bounds. The EDID buffer is initialised to "" (one
// zeroed byte, allocated even for an empty chunk), so an empty chunk (malformed data) is not read uninitialised.
bool TESLevSpell::Load(TESFile* apFile) {
	static LEVELED_OBJECT* pLastLeveledObject;
	const uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
	if (cFormType != LVSP_ID) {
		return false;
	}
	LoadForm(apFile);
	do {
		switch (apFile->GetTESChunk()) {
		case EDID_ID: {
			// A 16-byte aligned stack buffer of the chunk's size.
			uint32_t uiChunkSize = apFile->GetChunkSize();
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
			// Clears the buffer first because an empty chunk leaves it unwritten (see above); the game does not do
			// this.
			pBuffer[0] = '\0';
			apFile->GetChunkData(pBuffer, 0x200);
			static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
			break;
		}
		case OBND_ID:
			static_cast<TESBoundObject*>(this)->LoadObjectBound(apFile);
			break;
		case LVLF_ID:
			apFile->GetChunkData(&static_cast<TESLeveledList*>(this)->cLLFlags, 0);
			break;
		case LVLD_ID: {
			uint8_t cChanceNone = 0;
			apFile->GetChunkData(&cChanceNone, 0);
			static_cast<TESLeveledList*>(this)->SetChanceNone(cChanceNone);
			break;
		}
		case LVLG_ID: {
			uint32_t uiChanceGlobal = 0;
			apFile->GetChunkData<uint32_t>(uiChanceGlobal);
			// Stores the chunk's value in the pointer field unchanged.
			static_cast<TESLeveledList*>(this)->pChanceGlobal = reinterpret_cast<TESGlobal*>(uiChanceGlobal);
			break;
		}
		case COED_ID:
		case LVLO_ID: {
			// Re-reads the chunk: the COED chunk belongs to the entry the last LVLO chunk added.
			const uint32_t uiSubChunk = apFile->GetTESChunk();
			if (uiSubChunk == COED_ID) {
				if (pLastLeveledObject) {
					void* pMemory = ourMalloc(sizeof(ContainerItemExtra));
					ContainerItemExtra* pExtra = nullptr;
					if (pMemory) {
						pExtra = ::new (pMemory) ContainerItemExtra;
					}
					pExtra->Load(apFile);
					pLastLeveledObject->pItemExtra = pExtra;
					pLastLeveledObject = nullptr;
				}
			} else if (uiSubChunk == LVLO_ID) {
				LEVELED_OBJECT_FILE kObject;
				ourMemset(&kObject, 0, sizeof(kObject));
				apFile->GetChunkData(&kObject, 0xC);
				if (apFile->QEndian()) {
					kObject.Endian();
				}
				// The entry's form ID is deliberately passed as the pointer argument, as the game does.
				pLastLeveledObject = static_cast<TESLeveledList*>(this)->AddLeveledObject(kObject.sLevel,
					kObject.sCount, reinterpret_cast<TESForm*>(kObject.iFormID), nullptr);
			}
			break;
		}
		default:
			break;
		}
	} while (apFile->NextChunk());
	SetInitialized(false);
	return true;
}

void TESLevSpell::Save() {
	TESForm::StartForm();
	TESForm::InitializeDataComponent();
	static_cast<TESLeveledList*>(this)->Save();
	TESForm::CloseForm();
}

void TESLevSpell::InitItem() {
	if (!TESForm::GetInitialized()) {
		static_cast<TESLeveledList*>(this)->InitItem(this);
		TESForm::SetInitialized(1);
	}
}

void TESLevSpell::Copy(TESForm* apCopy) {
	if (dynamic_cast<TESLevSpell*>(apCopy)) {
		TESForm::CopyAllComponents(apCopy);
	}
}

bool TESLevSpell::Compare(TESForm* apCompare) {
	TESLevSpell* v4 = dynamic_cast<TESLevSpell*>(apCompare);
	return ((!v4) || (((int8_t)TESForm::CompareAllComponents(v4)) != 0));
}

void TESLevSpell::CalculateCurrentForm(MagicSystem::School aeSchool, uint16_t asLevel, TESForm*& apOutForm,
	uint16_t& asOutCount, bool abRecurse) {
	TESForm* psubform;
	uint16_t ssubcount;
	int32_t rnone;
	apOutForm = nullptr;
	asOutCount = 0;
	uint8_t chancenone = GetChanceNone();
	if (!chancenone || (rnone = Random() % 100, rnone >= chancenone)) {
		BSSimpleList<LEVELED_OBJECT*>* plist;
		int32_t ivalidcount = 1;
		int32_t ilastvalid = 0;
		BSSimpleList<LEVELED_OBJECT*>* pfirstvalid = 0;
		for (plist = &leveledList; plist; plist = plist->m_pkNext) {
			if (!plist->m_item) {
				break;
			}
			LEVELED_OBJECT* pdata = plist->m_item;
			MagicItem* pmagicItem = dynamic_cast<MagicItem*>(pdata->pForm);
			int32_t ithislevel = pdata->sLevel;
			if (ithislevel > asLevel) {
				break;
			}
			if (pmagicItem) {
				ActorValue::Index AssociatedSkill = pmagicItem->GetAssociatedSkill();
				if (MagicSystem::MagicSkillToSchool(AssociatedSkill) != aeSchool) {
					break;
				}
			}
			if (ithislevel <= ilastvalid || (ilastvalid && GetCalcAllBelow())) {
				++ivalidcount;
			} else {
				ivalidcount = 1;
				pfirstvalid = plist;
				ilastvalid = ithislevel;
			}
		}
		if (pfirstvalid) {
			int32_t ishift = Random() % ivalidcount;
			plist = pfirstvalid;
			while (ishift) {
				plist = plist->m_pkNext;
				--ishift;
				if (!plist) {
					return;
				}
			}
			LEVELED_OBJECT* pdata_2 = plist->m_item;
			TESForm* poutform = pdata_2->pForm;
			uint16_t soutcount = pdata_2->sCount;
			TESLevSpell* plevlist = dynamic_cast<TESLevSpell*>(pdata_2->pForm);
			if (plevlist) {
				if (abRecurse) {
					psubform = 0;
					ssubcount = 0;
					plevlist->CalculateCurrentForm(aeSchool, asLevel, psubform, ssubcount, abRecurse);
					poutform = psubform;
					soutcount *= ssubcount;
				}
			}
			apOutForm = poutform;
			asOutCount = soutcount;
		}
	}
}

SpellItem* TESLevSpell::GetLeveledSpell(MagicSystem::School aeSchool, int16_t asLevel) {
	SpellItem* pSpell = nullptr;
	TESForm* pForm = nullptr;
	uint16_t sCount = 0;
	CalculateCurrentForm(aeSchool, asLevel, pForm, sCount, true);
	if (pForm)
		pSpell = dynamic_cast<SpellItem*>(pForm);
	return pSpell;
}

BSSimpleList<SpellItem*>* TESLevSpell::GetLeveledSpells(MagicSystem::School aeSchool, int16_t asLevel) {
	BSSimpleList<SpellItem*>* plist_2 = 0;
	if (GetUseAll()) {
		for (LEVELED_OBJECT* pobject : *&leveledList) {
			if (pobject) {
				TESLevSpell* plevSpell;
				if (!plist_2) {
					BSSimpleList<SpellItem*>* plist;
					BSSimpleList<SpellItem*>* v8 =
						static_cast<BSSimpleList<SpellItem*>*>(ourMalloc(sizeof(BSSimpleList<SpellItem*>)));
					if (v8) {
						plist = ::new (v8) BSSimpleList<SpellItem*>;
					} else {
						plist = 0;
					}
					plist_2 = plist;
				}
				SpellItem* pspell = dynamic_cast<SpellItem*>(pobject->pForm);
				if (pspell ||
					((plevSpell = dynamic_cast<TESLevSpell*>(pobject->pForm)) != 0 &&
						(pspell = plevSpell->GetLeveledSpell(aeSchool, asLevel)) != 0)) {
					plist_2->AddHead(pspell);
				}
			}
		}
	} else {
		SpellItem* pspell_2 = GetLeveledSpell(aeSchool, asLevel);
		if (pspell_2) {
			BSSimpleList<SpellItem*>* plist_3;
			BSSimpleList<SpellItem*>* v7 =
				static_cast<BSSimpleList<SpellItem*>*>(ourMalloc(sizeof(BSSimpleList<SpellItem*>)));
			if (v7) {
				plist_3 = ::new (v7) BSSimpleList<SpellItem*>;
			} else {
				plist_3 = 0;
			}
			plist_2 = plist_3;
			plist_3->AddHead(pspell_2);
		}
	}
	return plist_2;
}

BSSimpleList<SpellItem*>* TESLevSpell::GetLeveledSpells_(Actor* apActor) {
	BSSimpleList<SpellItem*>* presultingList_2 = 0;
	for (MagicSystem::School emagicSchool = MagicSystem::ALTERATION; emagicSchool < MagicSystem::SCHOOL_COUNT;
		emagicSchool = static_cast<MagicSystem::School>(emagicSchool + 1)) {
		ActorValue::Index v2 = static_cast<ActorValue::Index>(retn_minusOne());
		int16_t v3 = apActor->GetActorValue(v2);
		BSSimpleList<SpellItem*>* plist = GetLeveledSpells(emagicSchool, v3);
		BSSimpleList<SpellItem*>* phead = plist;
		while (plist) {
			SpellItem* pitem = plist->m_item;
			if (pitem) {
				bool baddIt = 1;
				if (presultingList_2) {
					baddIt = !presultingList_2->IsInList(pitem);
				} else {
					BSSimpleList<SpellItem*>* presultingList;
					BSSimpleList<SpellItem*>* v7 =
						static_cast<BSSimpleList<SpellItem*>*>(ourMalloc(sizeof(BSSimpleList<SpellItem*>)));
					if (v7) {
						presultingList = ::new (v7) BSSimpleList<SpellItem*>;
					} else {
						presultingList = 0;
					}
					presultingList_2 = presultingList;
				}
				if (baddIt) {
					presultingList_2->AddHead(pitem);
				}
			}
			plist = plist->m_pkNext;
		}
		if (phead) {
			phead->RemoveAll();
		}
		if (phead) {
			delete phead;
		}
	}
	return presultingList_2;
}
