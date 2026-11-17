#include "Fallout Shared/CreatureSounds.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout/Magic/ActiveEffect.h"

CreatureSounds::CreatureSounds() : pSoundListArray{} {}

CreatureSounds::~CreatureSounds() {
	FreeAllCreatureSoundLists();
}

void CreatureSounds::FreeCreatureSoundList(CREATURE_SOUND_TYPE aeType) {
	for (BSSimpleList<CREATURE_SOUND*>* plist = GetCreatureSoundList(aeType); plist && !plist->IsEmpty();
		plist->RemoveHead()) {
		ourFree(plist->m_item);
	}
	void* v3 = pSoundListArray[aeType];
	if (v3) {
		delete static_cast<BSSimpleList<ActiveEffect*>*>(v3);
	}
	pSoundListArray[aeType] = 0;
}

void CreatureSounds::Copy(CreatureSounds* apCopy) {
	FreeAllCreatureSoundLists();
	if (apCopy) {
		for (char i = 0; i < 22; ++i) {
			for (BSSimpleList<CREATURE_SOUND*>* ecx0 =
					 apCopy->GetCreatureSoundList(static_cast<CREATURE_SOUND_TYPE>(i));
				ecx0 && !ecx0->IsEmpty(); ecx0 = ecx0->m_pkNext) {
				CREATURE_SOUND* apCopyFrom = ecx0->m_item;
				CREATURE_SOUND* apCopyTo = static_cast<CREATURE_SOUND*>(ourMalloc(sizeof(CREATURE_SOUND)));
				CreatureSounds::CopyCreatureSound(apCopyTo, apCopyFrom);
				AddCreatureSound(apCopyTo, static_cast<CREATURE_SOUND_TYPE>(i));
			}
		}
	}
}

// Walks the type's list until an entry with a sound wins its percent roll; null when none does. The exe calls the
// pointer BSSimpleList's IsEmpty (0x8256D0), GetItem (0x6815C0) and GetNext (0x726070) out of line; src's inline
// copies have the same effects.
TESSound* CreatureSounds::PickCreatureSound(CREATURE_SOUND_TYPE aeType) {
	TESSound* pSound = nullptr;
	for (BSSimpleList<CREATURE_SOUND*>* pIter = GetCreatureSoundList(aeType); pIter && !pIter->IsEmpty() && !pSound;
		pIter = pIter->GetNext()) {
		CREATURE_SOUND* pEntry = pIter->GetItem();
		if (pEntry->pSound && Random() % 100 < pEntry->iChance)
			pSound = pEntry->pSound;
	}
	return pSound;
}

void CreatureSounds::FreeAllCreatureSoundLists() {
	int8_t i;
	for (i = 0; ((int32_t)i) < 22; ++i) {
		FreeCreatureSoundList(static_cast<CREATURE_SOUND_TYPE>(i));
	}
}

// Chunks 0x49445343 'CSDI' (sound form ID), 0x46445343 'CSDF' (sound file name, 0x104 bytes) and 0x43445343 'CSDC'
// (one chance byte). The 8-byte entry is freed again when no CSDC follows.
bool CreatureSounds::LoadCreatureSound(TESFile* apFile, CREATURE_SOUND_TYPE aeType, TESCreature* apOwner) {
	bool bLoaded = false;
	if (!apFile) {
		return bLoaded;
	}
	CREATURE_SOUND* pEntry = nullptr;
	if (apFile->GetTESChunk() == 0x49445343) {
		uint32_t uiID = 0;
		apFile->GetChunkData<uint32_t>(uiID);
		pEntry = static_cast<CREATURE_SOUND*>(ourMalloc(sizeof(CREATURE_SOUND)));
		if (uiID) {
			TESForm::AddCompileIndex(uiID, apFile);
			pEntry->pSound = dynamic_cast<TESSound*>(TESForm::GetFormByNumericID(uiID));
			if (!pEntry->pSound) {
				uint32_t uiOwnerID = apOwner->GetFormID();
				const char* pOwnerName = apOwner->GetFormEditorID();
				Warning("AUDIO: Unable to find CreatureSound ID (%08X) for creature '%s' (%08X).", uiID, pOwnerName,
					uiOwnerID);
			}
		} else {
			pEntry->pSound = nullptr;
		}
		apFile->NextChunk();
	} else if (apFile->GetTESChunk() == 0x46445343) {
		char acFilename[0x104];
		apFile->GetChunkData(acFilename, 0x104);
		pEntry = static_cast<CREATURE_SOUND*>(ourMalloc(sizeof(CREATURE_SOUND)));
		pEntry->pSound = TESSound::FindSoundByFilename(acFilename);
		apFile->NextChunk();
	}
	if (pEntry) {
		if (apFile->GetTESChunk() == 0x43445343) {
			apFile->GetChunkData(&pEntry->iChance, 1);
			AddCreatureSound(pEntry, aeType);
			bLoaded = true;
		} else {
			ourFree(pEntry);
		}
	}
	return bLoaded;
}

// The exe's cleanup-only C++ EH frame is left out.
void CreatureSounds::AddCreatureSound(CREATURE_SOUND* apSound, CREATURE_SOUND_TYPE aeType) {
	if (apSound) {
		if (aeType >= CREA_SOUND_LEFT && aeType < CREA_SOUND_COUNT) {
			if (!pSoundListArray[aeType]) {
				BSSimpleList<CREATURE_SOUND*>* v4;
				BSSimpleList<CREATURE_SOUND*>* v6 =
					static_cast<BSSimpleList<CREATURE_SOUND*>*>(ourMalloc(sizeof(BSSimpleList<CREATURE_SOUND*>)));
				if (v6) {
					v4 = ::new (v6) BSSimpleList<CREATURE_SOUND*>;
				} else {
					v4 = 0;
				}
				pSoundListArray[aeType] = v4;
			}
			BSSimpleList<CREATURE_SOUND*>* plist = GetCreatureSoundList(aeType);
			BSSimpleList<CREATURE_SOUND*>* plastlist = 0;
			while (plist) {
				if (plist->IsEmpty()) {
					break;
				}
				int32_t iChance = plist->m_item->iChance;
				if (iChance > apSound->iChance) {
					break;
				}
				plastlist = plist;
				plist = plist->m_pkNext;
			}
			if (plist) {
				plist->AddHead(apSound);
			} else {
				if (plastlist) {
					plastlist->AddTail(apSound);
				}
			}
		}
	}
}

BSSimpleList<CREATURE_SOUND*>* CreatureSounds::GetCreatureSoundList(CREATURE_SOUND_TYPE aeType) {
	BSSimpleList<CREATURE_SOUND*>* v3 = 0;
	if (aeType >= CREA_SOUND_LEFT && aeType < CREA_SOUND_COUNT) {
		v3 = pSoundListArray[aeType];
	}
	return v3;
}

// The sound types are walked as an int, as the exe's counter, and each is passed as its sound type.
void CreatureSounds::Save() {
	for (int32_t i = 0; i < CREA_SOUND_COUNT; ++i) {
		BSSimpleList<CREATURE_SOUND*>* plist = GetCreatureSoundList(static_cast<CREATURE_SOUND_TYPE>(i));
		if (plist) {
			if (!plist->IsEmpty()) {
				TESForm::AddChunk(CSDT_ID, static_cast<uint32_t>(i));
				while (plist && !plist->IsEmpty()) {
					CREATURE_SOUND* pdata = plist->m_item;
					uint32_t tempint = 0;
					if (pdata->pSound) {
						tempint = pdata->pSound->iFormID;
					}
					TESForm::AddChunk(CSDI_ID, tempint);
					TESForm::AddChunk(CSDC_ID, pdata->iChance);
					plist = plist->m_pkNext;
				}
			}
		}
	}
}

bool CreatureSounds::Compare(CreatureSounds* apCompare) {
	if (!apCompare) {
		return 1;
	}
	for (char i = 0; i < 22; ++i) {
		BSSimpleList<CREATURE_SOUND*>* pcomplist;
		BSSimpleList<CREATURE_SOUND*>* plist = GetCreatureSoundList(static_cast<CREATURE_SOUND_TYPE>(i));
		for (pcomplist = apCompare->GetCreatureSoundList(static_cast<CREATURE_SOUND_TYPE>(i));
			plist && (!plist->IsEmpty() && (pcomplist && !pcomplist->IsEmpty())); pcomplist = pcomplist->m_pkNext) {
			CREATURE_SOUND* m_item = plist->m_item;
			CREATURE_SOUND* v5 = pcomplist->m_item;
			if (m_item->iChance != v5->iChance) {
				return 1;
			}
			if (m_item->pSound != v5->pSound) {
				return 1;
			}
			plist = plist->m_pkNext;
		}
		if (plist || pcomplist) {
			return 1;
		}
	}
	return 0;
}

void CreatureSounds::operator delete(void* apMemory) {
	ourFree(apMemory);
}

// The exe copies the two fields, not the padding after iChance.
void CreatureSounds::CopyCreatureSound(CREATURE_SOUND* apCopyTo, CREATURE_SOUND* apCopyFrom) {
	if (apCopyTo) {
		if (apCopyFrom) {
			apCopyTo->pSound = apCopyFrom->pSound;
			apCopyTo->iChance = apCopyFrom->iChance;
		}
	}
}
