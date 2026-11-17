#include "Fallout Shared/BGSIdleCollection.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

void BGSIdleCollection::Load(TESFile* apFile) {
	CHUNK_ID chunkid = apFile->GetTESChunk();
	if (BGSIdleCollection::IsValidChunk(chunkid)) {
		if (chunkid > IDLF_ID) {
			if (chunkid == IDLT_ID) {
				apFile->GetChunkData<uint32_t>(*reinterpret_cast<uint32_t*>(&fTimerCheckForIdle));
			}
		} else {
			switch (chunkid) {
			case IDLF_ID: {
				apFile->GetChunkData(&cIdleFlags, 1u);
				break;
			}
			case IDLA_ID: {
				if (pIdleArray) {
					uint32_t isize = 4 * cIdleCount;
					if (apFile->GetChunkSize() == isize) {
						apFile->GetChunkData(pIdleArray, isize);
#if FNVD_64BIT
						// The file keeps a 4-byte form ID for each idle, resolved by InitItem.
						TESFile::WidenFormIDRecords(pIdleArray, cIdleCount, 4, sizeof(TESIdleForm*), {{0, 0}});
#endif
						if (apFile->QEndian()) {
							for (int32_t ii = 0; ii < cIdleCount; ++ii) {
								ENDIAN<TESIdleForm*>(pIdleArray[ii], 0);
							}
						}
					}
				}
				break;
			}
			case IDLC_ID: {
				if (apFile->GetChunkSize() == 1) {
					apFile->GetChunkData(&cIdleCount, 1u);
				} else {
					uint32_t icount = 0;
					apFile->GetChunkData<uint32_t>(icount);
					cIdleCount = icount;
				}
				if (cIdleCount) {
					if (pIdleArray) {
						ourFree(pIdleArray);
					}
					pIdleArray = static_cast<TESIdleForm**>(ourMalloc(cIdleCount <= SIZE_MAX / sizeof(TESIdleForm*)
							? sizeof(TESIdleForm*) * cIdleCount
							: SIZE_MAX));
					ourMemset(pIdleArray, 0, sizeof(TESIdleForm*) * cIdleCount);
				}
				break;
			}
			}
		}
	}
}

BGSIdleCollection::BGSIdleCollection() {
	BGSIdleCollection::InitializeDataComponent();
}

BGSIdleCollection::~BGSIdleCollection() {
	BGSIdleCollection::ClearDataComponent();
}

void BGSIdleCollection::InitializeDataComponent() {
	cIdleFlags = 0;
	cIdleCount = 0;
	pIdleArray = nullptr;
	fTimerCheckForIdle = 0.0f;
}

// Frees the idle array and empties the collection. Slot 1 of BGSIdleCollection's vtable; BGSIdleMarker's
// BGSIdleCollection vtable (0x1046638) holds the same entry. The exe leaves this in eax (void here, as the base
// declares it).
void BGSIdleCollection::ClearDataComponent() {
	ourFree(pIdleArray);
	cIdleCount = 0;
	pIdleArray = nullptr;
}

void BGSIdleCollection::CopyComponent(BaseFormComponent* apCopyFC) {
	BGSIdleCollection* pcopy = dynamic_cast<BGSIdleCollection*>(apCopyFC);
	if (pcopy) {
		cIdleFlags = pcopy->cIdleFlags;
		ourFree(pIdleArray);
		pIdleArray = 0;
		cIdleCount = 0;
		if (pcopy->cIdleCount) {
			pIdleArray = static_cast<TESIdleForm**>(ourMalloc(pcopy->cIdleCount <= SIZE_MAX / sizeof(TESIdleForm*)
					? sizeof(TESIdleForm*) * pcopy->cIdleCount
					: SIZE_MAX));
			ourMemcpy(pIdleArray, pcopy->pIdleArray, sizeof(TESIdleForm*) * pcopy->cIdleCount);
			cIdleCount = pcopy->cIdleCount;
		}
		fTimerCheckForIdle = pcopy->fTimerCheckForIdle;
	}
}

bool BGSIdleCollection::CompareComponent(BaseFormComponent* apCompareFC) {
	BGSIdleCollection* pcompare = dynamic_cast<BGSIdleCollection*>(apCompareFC);
	if (!pcompare) {
		return 1;
	}
	if (cIdleFlags != pcompare->cIdleFlags) {
		return 1;
	}
	if (cIdleCount != pcompare->cIdleCount) {
		return 1;
	}
	if (pcompare->fTimerCheckForIdle != fTimerCheckForIdle) {
		return 1;
	}
	return pIdleArray &&
		(pcompare->pIdleArray && std::memcmp(pIdleArray, pcompare->pIdleArray, sizeof(TESIdleForm*) * cIdleCount));
}

void BGSIdleCollection::InitItem(TESForm* apOwner) {
	TESIdleForm* v11 = 0;
	for (uint32_t i = 0; i < cIdleCount; ++i) {
		uint32_t iid = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pIdleArray[i]));
		TESFile* File = apOwner->GetFile(-1);
		TESForm::AddCompileIndex(iid, File);
		TESForm* FormByNumericID = TESForm::GetFormByNumericID(iid);
		TESIdleForm* pidle = dynamic_cast<TESIdleForm*>(FormByNumericID);
		if (pidle) {
			if (pidle->CheckIdleFile()) {
				pIdleArray[i] = pidle;
			} else {
				int32_t CastingType = apOwner->iFormID;
				const char* v4 = apOwner->GetFormEditorID();
				Warning("MASTERFILE: MBGSIdleCollection has an idle '%s' without an animation file on form '%s' "
						"(%08X).",
					pidle->GetFormEditorID(), v4, CastingType);
				RemoveIdleAtIndex(i--);
			}
		} else {
			int32_t v7 = apOwner->iFormID;
			const char* v5 = apOwner->GetFormEditorID();
			Warning("MASTERFILE: Unable to find Idle (%08X) on form '%s' (%08X).", iid, v5, v7);
			RemoveIdleAtIndex(i--);
		}
	}
}

// The record's idle chunks: the flags, the count, the check timer, then one chunk of the idles' form IDs.
void BGSIdleCollection::Save() {
	if (!cIdleCount) {
		return;
	}
	TESForm::AddChunk(IDLF_ID, cIdleFlags);
	TESForm::AddChunk(IDLC_ID, cIdleCount);
	TESForm::AddChunk(IDLT_ID, fTimerCheckForIdle);
	// the exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of one dword per idle
	uint32_t* pIdleIDs = static_cast<uint32_t*>(__builtin_alloca_with_align(4 * cIdleCount, 128));
	for (uint32_t i = 0; i < cIdleCount; ++i) {
		pIdleIDs[i] = pIdleArray[i] ? pIdleArray[i]->GetFormID() : 0;
	}
	TESForm::AddChunkArray(IDLA_ID, pIdleIDs, cIdleCount);
}

int32_t BGSIdleCollection::GetIdleCount() const {
	return cIdleCount;
}

bool BGSIdleCollection::GetDoOnce() {
	return ((((int8_t)cIdleFlags) & 4) != 0);
}

BGSIdleCollection::PICK_TYPE BGSIdleCollection::GetPickType() {
	return static_cast<BGSIdleCollection::PICK_TYPE>((uint8_t)((cIdleFlags & 1) != 0));
}

void BGSIdleCollection::RemoveIdleAtIndex(uint32_t aiIndex) {
	if (aiIndex < cIdleCount) {
		if (cIdleCount == 1) {
			cIdleCount = 0;
			ourFree(pIdleArray);
			pIdleArray = 0;
		} else {
			--cIdleCount;
			TESIdleForm** parray = static_cast<TESIdleForm**>(ourMalloc(cIdleCount <= SIZE_MAX / sizeof(TESIdleForm*)
					? sizeof(TESIdleForm*) * cIdleCount
					: SIZE_MAX));
			// One signed count local ([ebp-8], jle) serves both copies: the index, then the entries after it.
			int32_t icopycount = aiIndex;
			if (icopycount > 0) {
				ourMemcpy(parray, pIdleArray, sizeof(TESIdleForm*) * icopycount);
			}
			icopycount = cIdleCount - aiIndex;
			if (icopycount > 0) {
				ourMemcpy(&parray[aiIndex], &pIdleArray[aiIndex + 1], sizeof(TESIdleForm*) * icopycount);
			}
			ourFree(pIdleArray);
			pIdleArray = parray;
		}
	}
}

bool BGSIdleCollection::HasIdleForActor(Actor* apActor, TESObjectREFR* apTargetRef) {
	if (apActor) {
		for (int32_t i = 0; i < cIdleCount; ++i) {
			TESIdleForm* pidle = pIdleArray[i];
			if (pidle && (apActor->CanUseIdle(pidle) && pidle->CheckConditions(apActor, apTargetRef, 1))) {
				return 1;
			}
		}
	}
	return 0;
}

uint32_t BGSIdleCollection::GetIdlesForActor(Actor* apActor, TESObjectREFR* apTargetRef,
	BSSimpleArray<TESIdleForm*, 1024>* apOutIdleArray) {
	uint32_t iidlecount = 0;
	if (apActor) {
		uint32_t iarraysize = 0;
		if (apOutIdleArray) {
			iarraysize = apOutIdleArray->iReservedSize;
		}
		for (int32_t i = 0; i < cIdleCount; ++i) {
			TESIdleForm* pidle = pIdleArray[i];
			if (pidle) {
				if (apActor->CanUseIdle(pidle) && pidle->CheckConditions(apActor, apTargetRef, 1)) {
					if (apOutIdleArray) {
						if (iidlecount >= iarraysize) {
							apOutIdleArray->Add(pidle);
						} else {
							apOutIdleArray->SetAt(iidlecount, pidle);
						}
					}
					++iidlecount;
				}
			}
		}
	}
	return iidlecount;
}

bool BGSIdleCollection::IsValidChunk(CHUNK_ID aeChunk) {
	if (((int32_t)aeChunk) > 1179403337) {
		return (bool)((uint8_t)((((int32_t)aeChunk) == 1313621065) || (((int32_t)aeChunk) == 1414284361)));
	}
	return (bool)((uint8_t)((((int32_t)aeChunk) == 1179403337) ||
		((((int32_t)aeChunk) == 1095517257) ||
			((((int32_t)aeChunk) == 1112294473) || (((int32_t)aeChunk) == 1129071689)))));
}
