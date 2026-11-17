#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/IOManager.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/GarbageCollector.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSReconstructFormsInAllFilesMap.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadChangesMap.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormIDMap.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGlobalData.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadHistory.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadInitialData.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadReferencesMap.h"
#include "Fallout/Misc/SaveLoad/BGSUnloadedFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Reconstruction.hpp"

namespace {} // namespace

SaveGameFormInfo::SaveGameFormInfo()
	: iFormIDMapOffset(0), iHistoryOffset(0), iInitialGlobalDataOffset(0), iFormDataOffset(0),
	  iFinalGlobalDataOffset(0), iInitialGlobalFormCount(0), iFinalGlobalFormCount(0), iFormCount(0),
	  iSecondaryDataSize(0), ReservedSpace{} {}

BGSSaveLoadGame::BGSSaveLoadGame()
	: pOldChangesMap(nullptr), FormBufferArray(), QueuedInitPackageLocationsActorMap(37u), uiGlobalFlags(0),
	  cCurrentMinorVersion(0xFF) {
	SetGlobalAllowChanges(false);
	pChangesMap = new BGSSaveLoadChangesMap();
	pFormIDMap = new BGSSaveLoadFormIDMap(0x13AFu);
	pWorldspaceFormIDMap = new BGSSaveLoadFormIDMap(37u);
	pReferencesMap = new BGSSaveLoadReferencesMap();
	pQueuedSubBuffersMap = new BGSSaveLoadQueuedSubBufferMap();
	pChangedFormIDMap = new NiTMap<uint32_t, uint32_t>(37u);
	pHistory = new BGSSaveLoadHistory();
	pReconstructForms = new BGSReconstructFormsInAllFilesMap();
	ourMemset(cFileIndexArray, 255, 255u);
	ourMemset(cReverseFileIndexArray, 255, 255u);
}

BGSSaveLoadGame::~BGSSaveLoadGame() {
	delete pChangesMap;
	delete pFormIDMap;
	delete pWorldspaceFormIDMap;
	delete pReferencesMap;
	delete pQueuedSubBuffersMap;
	delete pChangedFormIDMap;
	delete pHistory;
	delete pReconstructForms;
}

void BGSSaveLoadGame::QueueInitPackageLocations(Actor* apActor) {
	ActorValueOwner* v2 = static_cast<ActorValueOwner*>(apActor->GetActorBase());
	if (v2->GetActorFloatValue(ActorValue::HEALTH) > 0.0) {
		QueuedInitPackageLocationsActorMap.SetAt(apActor->iFormID, apActor);
	}
}

void BGSSaveLoadGame::DeleteForm(TESForm* apForm) {
	if (apForm) {
		delete apForm;
	}
}

void BGSSaveLoadGame::CancelInitPackageLocations(TESObjectREFR* apRef) {
	QueuedInitPackageLocationsActorMap.RemoveAt(apRef->iFormID);
}

void BGSSaveLoadGame::AddChange(TESForm* apForm, BGSChangeFlags aiChanges, bool abForceAdd) {
	if (!ShouldAddChanges(apForm, abForceAdd)) {
		return;
	}
	pChangesMap->AddChangeFlags(apForm->iFormID, aiChanges);
}

void BGSSaveLoadGame::RemoveChange(TESForm* apForm, BGSChangeFlags aiChanges) {
	if (!ShouldRemoveChanges(apForm)) {
		return;
	}
	pChangesMap->RemoveChangeFlags(apForm->iFormID, aiChanges);
}

// Drops the form's changes unless they still hold an unloaded form buffer.
void BGSSaveLoadGame::RemoveChanges(TESForm* apForm) {
	uint32_t refID = apForm->GetFormID();
	BGSFormChanges* pChanges = pChangesMap->GetChanges(refID);
	if (pChanges && !static_cast<char*>(pChanges->UnloadedFormBuffer)) {
		pChangesMap->RemoveChanges(refID);
	}
}

bool BGSSaveLoadGame::GetOldChange(TESForm* apForm, BGSChangeFlags aiChanges) {
	if (!pOldChangesMap) {
		return false;
	}
	return pOldChangesMap->CheckChangeFlags(apForm->GetFormID(), aiChanges);
}

void BGSSaveLoadGame::QueueSubBuffer(QUEUED_SUB_BUFFER_TYPE aeType, BGSLoadFormBuffer* apLoadFormBuffer,
	TESForm* apParentForm) {
	pQueuedSubBuffersMap->QueueSubBuffer(aeType, apLoadFormBuffer, apParentForm);
}

void BGSSaveLoadGame::RevertAllChanges() {
	SetSaveGameLoading(true);
	BGSSaveLoadGlobalData::RevertGlobalData(true);
	RevertCurrentChanges(nullptr, nullptr);
	PerformQueuedInitPackageLocations();
	SetSaveGameLoading(false);
}

void BGSSaveLoadGame::AddChangedFormID(uint32_t aiOldFormID, uint32_t aiNewFormID) {
	uint32_t uiCurrentFormID = 0;
	if (pChangedFormIDMap->GetAt(aiOldFormID, uiCurrentFormID)) {
		// The format is the exe's literal at 0x107F338.
		SaveGameWarning("FORMS: Trying to set new form ID %08X for old form ID %08X, but already in map with value "
						"%08X",
			aiNewFormID, aiOldFormID, uiCurrentFormID);
	}
	pChangedFormIDMap->SetAt(aiOldFormID, aiNewFormID);
}

bool BGSSaveLoadGame::ShouldAddChanges(TESForm* apForm, bool abForceAdd) {
	if (!GetAllowChanges() && !abForceAdd) {
		return false;
	}
	if (apForm->GetTemporary()) {
		return false;
	}
	uint32_t iFormID = apForm->iFormID;
	if (apForm->GetDelete() && pDataHandler->IsFormIDCreated(iFormID)) {
		return false;
	}
	return true;
}

bool BGSSaveLoadGame::GetChange(TESForm* apForm, BGSChangeFlags aiChanges) {
	return pChangesMap->CheckChangeFlags(apForm->iFormID, aiChanges);
}

bool BGSSaveLoadGame::IsFormIDInUse(uint32_t aiFormID) {
	return pChangesMap->GetChanges(aiFormID) != nullptr;
}

bool BGSSaveLoadGame::HasReferenceChangedCells(uint32_t aiFormID) {
	return pReferencesMap->GetMovedReference(aiFormID);
}

bool BGSSaveLoadGame::GetSaveGameLoading() const {
	return (uiGlobalFlags & GLOBAL_SAVEGAME_LOADING) != 0;
}

bool BGSSaveLoadGame::GetSaveGameSaving() const {
	return (uiGlobalFlags & GLOBAL_SAVEGAME_SAVING) != 0;
}

bool BGSSaveLoadGame::GetDeferInitForms() const {
	return (uiGlobalFlags & GLOBAL_DEFER_INIT_FORMS) != 0;
}

bool BGSSaveLoadGame::GetGlobalAllowChanges() const {
	return (uiGlobalFlags & GLOBAL_DISALLOW_CHANGES) == 0;
}

bool BGSSaveLoadGame::GetInitingForms() const {
	return ((((int32_t)uiGlobalFlags) & 8) != 0);
}

void BGSSaveLoadGame::SetInitingForms(bool abInitingForms) {
	if (abInitingForms) {
		uiGlobalFlags |= GLOBAL_INITING_FORMS;
	} else {
		uiGlobalFlags &= ~static_cast<uint32_t>(GLOBAL_INITING_FORMS);
	}
}

void BGSSaveLoadGame::SetSaveLoadFailed(bool abFailed) {
	if (abFailed) {
		uiGlobalFlags |= GLOBAL_SAVELOAD_FAILED;
	} else {
		uiGlobalFlags &= ~static_cast<uint32_t>(GLOBAL_SAVELOAD_FAILED);
	}
}

bool BGSSaveLoadGame::SetGlobalAllowChanges(bool abValue) {
	bool bWasAllowed = GetGlobalAllowChanges();
	if (!abValue) {
		uiGlobalFlags |= GLOBAL_DISALLOW_CHANGES;
	} else {
		uiGlobalFlags &= ~static_cast<uint32_t>(GLOBAL_DISALLOW_CHANGES);
	}
	return bWasAllowed;
}

bool BGSSaveLoadGame::GetAllowChanges() {
	return (bool)((uint8_t)(GetThreadAllowChanges() && GetGlobalAllowChanges()));
}

// Changes are tracked only while changes are allowed, and never for temporary forms.
bool BGSSaveLoadGame::ShouldRemoveChanges(TESForm* apForm) {
	return GetAllowChanges() && !apForm->GetTemporary();
}

// The exe reloads the TLS address for each read and write.
void BGSSaveLoadGame::SetLoadingMovedRefs(bool abValue) {
	if (abValue) {
		uint32_t uiFlags = GetThreadSpecificFlags();
		GetThreadSpecificFlags() = uiFlags | 2u;
	} else {
		uint32_t uiFlags = GetThreadSpecificFlags();
		GetThreadSpecificFlags() = uiFlags & ~2u;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void BGSSaveLoadGame::UnloadForm(TESForm* apForm, bool abUnloadWhileLoadingSaveGame) {
	BGSSaveFormBuffer saveFormBuffer;
	BGSChangeFlags iremovedFlags;
	FNVD_INLINED_LOCAL(AutoMemContext, tempMemContext, std::nullopt, std::nullopt);
	::new (&tempMemContext) AutoMemContext(MEM_CONTEXT_SAVELOAD, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Misc\\SaveLoad\\BGSSaveLoadGame.cpp", 894);
	if ((GetSaveGameLoading() && !abUnloadWhileLoadingSaveGame) ||
		(pDataHandler->GetClearingData() || !ShouldUnloadForm(apForm))) {
		tempMemContext.~AutoMemContext();
		return;
	}
	uint32_t iformID = apForm->iFormID;
	BGSFormChanges* pchanges = pChangesMap->GetChanges(iformID);
	if (pchanges) {
		if (pchanges->UnloadedFormBuffer.pBuffer) {
			SaveGameWarning("FORMS: Form '%s' (%08X) is unloading, but it already has a buffer.  This unload will be "
							"skipped.",
				apForm->GetFormEditorID(), iformID);
			tempMemContext.~AutoMemContext();
			return;
		}
		saveFormBuffer.~BGSSaveFormBuffer();
		::new (&saveFormBuffer) BGSSaveFormBuffer;
		uint8_t MinorVersion = pSaveLoadManager->GetMinorVersion();
		ENUM_FORM_ID FormType = apForm->GetFormType();
		saveFormBuffer.SetHeader(iformID, pchanges->iChangeFlags, FormType, MinorVersion);
		saveFormBuffer.pForm = apForm;
		apForm->CheckSaveGame(&saveFormBuffer);
		int32_t v8 = saveFormBuffer.GetChangeFlags().iFlags;
		pchanges->iChangeFlags.GetDifference(&iremovedFlags, v8);
		if (iremovedFlags.iFlags && pChangesMap->RemoveChangeFlags(iformID, iremovedFlags)) {
			tempMemContext.~AutoMemContext();
			return;
		}
		INITIAL_DATA_TYPE einitialDataType =
			BGSSaveLoadInitialData::SaveInitialData(&saveFormBuffer, apForm, pchanges->iChangeFlags);
		apForm->SaveGame(&saveFormBuffer);
		pchanges->UnloadedFormBuffer.Initialize(&saveFormBuffer);
		if (einitialDataType == INITIAL_DATA_CREATED_REFERENCE || einitialDataType == INITIAL_DATA_MOVED_REFERENCE) {
			TESObjectREFR* pref = static_cast<TESObjectREFR*>(apForm);
			TESObjectCELL* pcell = pref->GetParentCell();
			TESWorldSpace* pworld = pref->GetWorldSpace();
			if (pcell && pcell->GetInterior()) {
				uint32_t CastingType = pcell->iFormID;
				pReferencesMap->AddReference(CastingType, iformID);
				if (einitialDataType == INITIAL_DATA_MOVED_REFERENCE) {
					pReferencesMap->AddMovedReference(iformID);
				}
			} else {
				if (pworld) {
					const NiPoint3* v10 = &pref->GetLocationOnReference();
					uint32_t v5 = pworld->iFormID;
					pReferencesMap->AddReference(v5, iformID, *v10);
					if (einitialDataType == INITIAL_DATA_MOVED_REFERENCE) {
						pReferencesMap->AddMovedReference(iformID);
					}
				} else {
					SaveGameWarning("FORMS: No Worldspace or Cell could not be found while unloading reference '%s' "
									"(%08X)",
						pref->GetFormEditorID(), iformID);
				}
			}
		}
	}
	tempMemContext.~AutoMemContext();
}

void BGSSaveLoadGame::LoadForm(TESForm* apForm) {
	if (!GetSaveGameLoading()) {
		uint32_t iformID = apForm->iFormID;
		BGSFormChanges* pchanges = pChangesMap->GetChanges(iformID);
		if (pchanges) {
			if (pchanges->UnloadedFormBuffer.pBuffer) {
				ENUM_FORM_ID eformType = pchanges->UnloadedFormBuffer.GetFormType();
				if (apForm->GetFormType() == eformType) {
					bool brestoreAllowChanges = SetThreadAllowChanges(0);
					BGSLoadFormBuffer* ploadFormBuffer = pchanges->UnloadedFormBuffer.FillLoadFormBuffer();
					// The load buffer owns the data now: the exe's destructor copy 0x6665A0 only clears the pointer.
					BGSUnloadedFormBuffer::DestructKeepBuffer(&pchanges->UnloadedFormBuffer);
					ploadFormBuffer->pForm = apForm;
					ploadFormBuffer->SetChangeFlags(pchanges->iChangeFlags);
					apForm->SetStillLoading(1);
					BGSSaveLoadInitialData::LoadInitialData(ploadFormBuffer, apForm, pchanges->iChangeFlags);
					apForm->LoadGame(ploadFormBuffer);
					ploadFormBuffer->ClearBuffer();
					FormBufferArray.Add(ploadFormBuffer);
					SetThreadAllowChanges(brestoreAllowChanges);
				} else {
					const char* FormTypeString = apForm->GetFormTypeString();
					const char* FormTypeString_0 = TESForm::GetFormTypeString(eformType);
					SaveGameWarning("SAVELOAD: (Load Error) Form '%s' (%08X) was saved with form type %s, but "
									"currently has form type %s.  Its loading will be skipped.",
						apForm->GetFormEditorID(), iformID, FormTypeString_0, FormTypeString);
					pChangesMap->RemoveChanges(iformID);
				}
			}
		}
	}
}

// The exe's cleanup-only C++ EH (unwind) frame for the form buffer is left out; its destructor is called where the
// exe calls it. The two strings are pushed straight to the cdecl note and warning functions.
void BGSSaveLoadGame::SaveGame(BGSSaveLoadFile* apFile) {
	SetSaveGameSaving(true);
	char acFullFileName[0x108];
	pSaveLoadManager->GetFullFileName(apFile->GetSaveFileName(), acFullFileName);
	char acVersionInfo[0x104];
	pSaveLoadManager->GetVersionInfo(acVersionInfo, 0x104);
	pHistory->AddNote("***Saving game: %s (%s)", acFullFileName, acVersionInfo);
	uint8_t ucMinorVersion = (uint8_t)pSaveLoadManager->GetMinorVersion();
	SaveGameDataToFileFunction(apFile, &ucMinorVersion, 1);
	SavePluginList(apFile);
	uint32_t uiHeaderPosition = apFile->GetFilePosition();
	SaveGameFormInfo kHeader;
	kHeader.~SaveGameFormInfo();
	::new (&kHeader) SaveGameFormInfo;
	apFile->Write(&kHeader, 0x6E);
	kHeader.iInitialGlobalDataOffset = apFile->GetFilePosition();
	kHeader.iInitialGlobalFormCount = BGSSaveLoadFile::SaveGlobalDataRange0_C(apFile);
	kHeader.iFormDataOffset = apFile->GetFilePosition();
	// Built and destroyed by the exe's out-of-line constructor and destructor.
	alignas(BGSSaveFormBuffer) uint8_t kBufferMemory[sizeof(BGSSaveFormBuffer)];
	BGSSaveFormBuffer* pBuffer = ::new (kBufferMemory) BGSSaveFormBuffer;
	NiTMapItem<uint32_t, BGSFormChanges*>* pPos = pChangesMap->GetFirstPos();
	while (pPos) {
		uint32_t uiFormID = 0;
		BGSFormChanges* pChanges = nullptr;
		pChangesMap->GetNext(pPos, uiFormID, pChanges);
		if ((char*)pChanges->UnloadedFormBuffer) {
			pChanges->UnloadedFormBuffer.Save(apFile, uiFormID, pChanges->iChangeFlags);
			++kHeader.iFormCount;
			continue;
		}
		TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
		if (!pForm) {
			SaveGameWarning("SAVELOAD: Could not find form %08X with change flags %08X during "
							"BGSSaveLoadGame::SaveGame()",
				uiFormID, (int32_t)pChanges->iChangeFlags);
			continue;
		}
		pBuffer->ClearPosition();
		// The exe evaluates the version, then the form type, then the flags.
		uint8_t ucVersion = (uint8_t)pSaveLoadManager->GetMinorVersion();
		ENUM_FORM_ID eFormType = (ENUM_FORM_ID)pForm->GetFormType();
		pBuffer->SetHeader(uiFormID, pChanges->iChangeFlags, eFormType, ucVersion);
		pBuffer->SetForm(pForm);
		pForm->CheckSaveGame(pBuffer);
		// The flags the buffer gained over the recorded ones are recorded.
		BGSChangeFlags kRecorded;
		kRecorded.iFlags = (int32_t)pChanges->iChangeFlags;
		BGSChangeFlags kAdded;
		pBuffer->GetChangeFlags().GetDifference(&kAdded, kRecorded.iFlags);
		if ((int32_t)kAdded) {
			pChangesMap->AddChangeFlags(uiFormID, kAdded);
		}
		// The recorded flags the buffer lost are removed; when that empties the entry, the form is not saved.
		BGSChangeFlags kBufferFlags;
		kBufferFlags.iFlags = (int32_t)pBuffer->GetChangeFlags();
		BGSChangeFlags kRemoved;
		pChanges->iChangeFlags.GetDifference(&kRemoved, kBufferFlags.iFlags);
		if ((int32_t)kRemoved && pChangesMap->RemoveChangeFlags(uiFormID, kRemoved)) {
			continue;
		}
		BGSSaveLoadInitialData::SaveInitialData(pBuffer, pForm, pChanges->iChangeFlags);
		pForm->SaveGame(pBuffer);
		pBuffer->Save(apFile);
		++kHeader.iFormCount;
	}
	kHeader.iFinalGlobalDataOffset = apFile->GetFilePosition();
	kHeader.iFinalGlobalFormCount = BGSSaveLoadFile::SaveGlobalDataRange3E8_3E9(apFile);
	kHeader.iFormIDMapOffset = apFile->GetFilePosition();
	pFormIDMap->Save(apFile);
	pWorldspaceFormIDMap->Save(apFile);
	kHeader.iHistoryOffset = apFile->GetFilePosition();
	pHistory->AddNote("---Finished saving game: %s", acFullFileName);
	pHistory->Save(apFile);
	apFile->SeekSet(uiHeaderPosition);
	apFile->Write(&kHeader, 0x6E);
	SetSaveGameSaving(false);
	pBuffer->~BGSSaveFormBuffer();
}

void BGSSaveLoadGame::SavePluginList(BGSSaveLoadFile* apFile) {
	const char* a2;
	TESFile* file;
	uint32_t index;
	// Raw storage: the exe runs only BGSSaveGameBuffer's own constructor on this stack buffer.
	alignas(BGSSaveGameBuffer) char aBufferStorage[sizeof(BGSSaveGameBuffer)];
	BGSSaveGameBuffer* p_apMemory = ::new (aBufferStorage) BGSSaveGameBuffer;
	int32_t v8 = 0;
	int8_t apData = (int8_t)pDataHandler->TESDataHandler::CountCompiled();
	p_apMemory->SaveDataEndian(static_cast<char*>(static_cast<void*>(&apData)), 1u, 0);
	for (index = 0; index < ((uint32_t)((int32_t)((uint8_t)apData))); ++index) {
		file = pDataHandler->GetCompiledFile(index);
		a2 = file->GetFilename();
		p_apMemory->SaveString(a2, 0);
	}
	p_apMemory->Save(apFile);
	v8 = (-1);
	p_apMemory->~BGSSaveGameBuffer();
}

// Reverts every changed form to its loaded state: created forms are cleared, the others revert through a revert-only
// load buffer (their unrevertible changes handled or the form queued for reconstruction), and an unloaded moved
// reference whose original cell is loaded is queued for reconstruction. apChangesMap (a new map when null) replaces
// the changes map, which is deleted. The exe's cleanup-only C++ EH frame is left out.
void BGSSaveLoadGame::RevertCurrentChanges(BGSSaveLoadChangesMap* apNewChangesMap,
	BGSSaveLoadFormIDMap* apOldFormIDMap) {
	if (!apNewChangesMap) {
		BGSSaveLoadChangesMap* pMemory = static_cast<BGSSaveLoadChangesMap*>(ourMalloc(sizeof(BGSSaveLoadChangesMap)));
		BGSSaveLoadChangesMap* pNewMap;
		if (pMemory) {
			pNewMap = ::new (pMemory) BGSSaveLoadChangesMap;
		} else {
			pNewMap = nullptr;
		}
		apNewChangesMap = pNewMap;
	}
	NiTMapItem<uint32_t, BGSFormChanges*>* pPos = pChangesMap->GetFirstPos();
	while (pPos) {
		uint32_t iformID = 0;
		BGSFormChanges* pchanges = nullptr;
		pChangesMap->GetNext(pPos, iformID, pchanges);
		TESForm* pform = TESForm::GetFormByNumericID(iformID);
		if (pform) {
			if (pDataHandler->IsFormIDCreated(iformID)) {
				ClearForm(pform);
				apNewChangesMap->RemoveChanges(iformID);
			} else {
				FNVD_CONSTRUCTED_LOCAL(BGSLoadFormBuffer, kLoadFormBuffer, &BGSLoadFormBuffer::Destruct);
				kLoadFormBuffer.pForm = pform;
				kLoadFormBuffer.SetOldChangeFlags(pchanges->iChangeFlags);
				kLoadFormBuffer.SetRevertOnly(1);
				pform->Revert(&kLoadFormBuffer);
				uint32_t eFormType = pform->GetFormType();
				// The new change flags argument is built in its stack slot by the exe's constructor 0x8C71B0.
				BGSChangeFlags kNoChanges;
				kNoChanges.~BGSChangeFlags();
				::new (&kNoChanges) BGSChangeFlags(0);
				BGSChangeFlags kUnrevertible;
				BGSChangeFlags::GetUnrevertibleChanges(&kUnrevertible, pchanges->iChangeFlags, kNoChanges, eFormType,
					false);
				if (kUnrevertible.iFlags) {
					if (!HandleUnrevertibleChanges(pform, kUnrevertible, true)) {
						if (pform) {
							pReconstructForms->AddForm(pform, 8u);
						}
					}
				}
				if (pform) {
					if (pchanges->iChangeFlags.ReduceToForcedChanges(pform)) {
						apNewChangesMap->AddChangeFlags(iformID, pchanges->iChangeFlags);
					}
				}
			}
		} else if (apOldFormIDMap) {
			BGSUnloadedFormBuffer unloadedFormBuffer;
			unloadedFormBuffer.pBuffer = pchanges->UnloadedFormBuffer.pBuffer;
			if (unloadedFormBuffer.pBuffer) {
				ENUM_FORM_ID eformType = unloadedFormBuffer.GetFormType();
				INITIAL_DATA_TYPE einitialDataType =
					BGSSaveLoadInitialData::GetInitialDataType(iformID, eformType, pchanges->iChangeFlags);
				if (einitialDataType == INITIAL_DATA_MOVED_REFERENCE) {
					BGSSaveLoadFormIDMap* pcurrentFormIDMap = pFormIDMap;
					pFormIDMap = apOldFormIDMap;
					BGSUnloadedFormBuffer changeBuffer;
					pChangesMap->GetChangeBuffer(&changeBuffer, iformID);
					ReferenceInitialData refData;
					refData.~ReferenceInitialData();
					::new (&refData) ReferenceInitialData;
					BGSSaveLoadInitialData::LoadInitialDataStruct(changeBuffer.GetAdvancedBuffer(), einitialDataType,
						&refData);
					TESObjectCELL* pcell = nullptr;
					TESWorldSpace* pworld = nullptr;
					refData.GetOriginalLocationCellAndWorld(pcell, pworld);
					if (pcell) {
						if (TES::pInstance->IsCellLoaded(pcell, 0)) {
							pReconstructForms->AddReference(pcell, iformID, 8u);
						}
					}
					pFormIDMap = pcurrentFormIDMap;
				}
			}
		}
	}
	pReconstructForms->ReconstructForms();
	pReconstructForms->Clear();
	// The exe calls the old map's deleting destructor through vtable slot 0 (flag 1).
	delete pChangesMap;
	pChangesMap = apNewChangesMap;
}

void BGSSaveLoadGame::PerformQueuedInitPackageLocations() {
	NiTMapItem<uint32_t, Actor*>* it = QueuedInitPackageLocationsActorMap.GetFirstPos();
	while (it) {
		uint32_t iformID = 0;
		Actor* pactor = 0;
		QueuedInitPackageLocationsActorMap.GetNext(it, iformID, pactor);
		if (pactor->pCurrentProcess) {
			pactor->InitPackageLocations(1);
		}
	}
	QueuedInitPackageLocationsActorMap.RemoveAll();
}

void BGSSaveLoadGame::ClearForm(TESForm* apForm) {
	if (apForm) {
		uint32_t FormType = apForm->GetFormType();
		switch (FormType) {
		case 4u:
		case 17u:
		case 18u:
		case 42u:
		case 43u:
		case 57u:
		case 66u:
		case 67u: {
			uint32_t NextID = pDataHandler->GetNextID();
			apForm->SetFormID(NextID, 1);
			break;
		}
		case 58u:
		case 59u:
		case 60u:
		case 61u:
		case 62u:
		case 63u:
		case 64u:
		case 73u:
		case 105u: {
			DeleteForm(apForm);
			break;
		}
		default: {
			const char* FormTypeString_0 = TESForm::GetFormTypeString(FormType);
			int32_t CastingType = apForm->GetFormID();
			SaveGameWarning("FORMS: Form %08X with form type %i %s was re-numbered by BGSSaveLoadGame::ClearForm()",
				CastingType, FormType, FormTypeString_0);
			uint32_t v4 = pDataHandler->GetNextID();
			apForm->SetFormID(v4, 1);
			break;
		}
		}
	}
}

// Outside a save load, loads the cell's saved changes, then its saved references (with moved references flagged),
// rebuilds the forms they need and loads each. The exe's cleanup-only EH frame (for the local map) is left out; the
// map is constructed and destructed through the exe's calls.
void BGSSaveLoadGame::LoadCell(TESObjectCELL* apCell) {
	if (GetSaveGameLoading())
		return;
	LoadForm(apCell);
	alignas(BGSReconstructFormsInAllFilesMap) uint8_t aMapStorage[sizeof(BGSReconstructFormsInAllFilesMap)];
	BGSReconstructFormsInAllFilesMap& rReconstructForms = *::new (aMapStorage) BGSReconstructFormsInAllFilesMap;
	pReferencesMap->LoadReferencesForCell(apCell, &rReconstructForms);
	SetLoadingMovedRefs(true);
	rReconstructForms.ReconstructForms();
	SetLoadingMovedRefs(false);
	uint32_t uiCount = rReconstructForms.GetReconstructedCount();
	for (uint32_t i = 0; i < uiCount; ++i) {
		TESForm* pForm = rReconstructForms.GetReconstructedForm(i);
		LoadForm(pForm);
	}
	rReconstructForms.~BGSReconstructFormsInAllFilesMap();
}

void BGSSaveLoadGame::InitForms(bool abLoadModels) {
	SetInitingForms(1);
	if (abLoadModels) {
		pIOManager->ForceHighestPriority();
	}
	for (uint32_t i = 0; i < this->FormBufferArray.iSize; ++i) {
		BGSLoadFormBuffer* ploadFormBuffer = this->FormBufferArray.operator[](i);
		if (ploadFormBuffer) {
			TESForm* pform = ploadFormBuffer->GetForm();
			if (pform) {
				pform->InitLoadGame(ploadFormBuffer);
			}
		}
	}
	if (abLoadModels) {
		bhkWorld* pworld;
		TES::pInstance->QueueReferencesForAttachedCells();
		pIOManager->Resume();
		pIOManager->LoadAllQueuedPriorities();
		TESObjectCELL* pcell = pPlayer->GetParentCell();
		if (pcell) {
			pworld = pcell->GetbhkWorld();
		} else {
			pworld = 0;
		}
		if (pworld) {
			pworld->DeactivateAllIslands();
		}
		TES::pInstance->ActivateAllSubmergedReferencesIslands();
	}
	this->pQueuedSubBuffersMap->LoadAllQueuedSubBuffers();
	for (uint32_t i_2 = 0; i_2 < this->FormBufferArray.iSize; ++i_2) {
		BGSLoadFormBuffer* ploadFormBuffer_2 = this->FormBufferArray.operator[](i_2);
		if (ploadFormBuffer_2) {
			TESForm* pform_2 = ploadFormBuffer_2->GetForm();
			if (pform_2) {
				pform_2->FinishLoadGame(ploadFormBuffer_2);
				if (ploadFormBuffer_2->GetCellChanged()) {
					Actor* pactor = static_cast<Actor*>(ploadFormBuffer_2->GetReference());
					// The change flags argument is built in its stack slot by the exe's constructor 0x8C71B0.
					BGSChangeFlags kMoved;
					kMoved.~BGSChangeFlags();
					::new (&kMoved) BGSChangeFlags(8);
					AddChange(pactor, kMoved, true);
					if (pactor->IsActor()) {
						if (pactor->pCurrentProcess) {
							pactor->MovetoLow();
						}
					}
					this->UnloadForm(pactor, 1);
					TESObjectCELL* pparentcell = pactor->GetParentCell();
					if (pparentcell) {
						pparentcell->RemoveReference(pactor);
					}
					GarbageCollector::Add(pactor);
				}
				pform_2->SetStillLoading(0);
			}
			delete ploadFormBuffer_2;
		}
	}
	this->FormBufferArray.Clear(1);
	this->pChangedFormIDMap->RemoveAll();
	this->SetInitingForms(0);
}

bool BGSSaveLoadGame::GetPositioningPlayerCharacter() {
	return ((((int32_t)uiGlobalFlags) & 32) != 0);
}

void BGSSaveLoadGame::SetDeferInitForms(bool abValue) {
	uint32_t v2;
	if (abValue) {
		v2 = uiGlobalFlags | 16;
	} else {
		v2 = uiGlobalFlags & 0xFFFFFFEFu;
	}
	uiGlobalFlags = v2;
}

bool BGSSaveLoadGame::GetPlayerLocationInvalid() {
	return ((((int32_t)uiGlobalFlags) & 64) != 0);
}

uint8_t BGSSaveLoadGame::GetCurrentMinorVersion() {
	return cCurrentMinorVersion;
}

void BGSSaveLoadGame::SetSaveGameSaving(bool abValue) {
	uint32_t v2;
	if (abValue) {
		v2 = uiGlobalFlags | 4;
	} else {
		v2 = uiGlobalFlags & (~4u);
	}
	uiGlobalFlags = v2;
}

void BGSSaveLoadGame::SetSaveGameLoading(bool abValue) {
	uint32_t v2;
	if (abValue) {
		v2 = uiGlobalFlags | 2;
	} else {
		v2 = uiGlobalFlags & 0xFFFFFFFDu;
	}
	uiGlobalFlags = v2;
}

void BGSSaveLoadGame::SetPlayerLocationInvalid(bool abValue) {
	uint32_t v2;
	if (abValue) {
		v2 = uiGlobalFlags | 64;
	} else {
		v2 = uiGlobalFlags & 0xFFFFFFBFu;
	}
	uiGlobalFlags = v2;
}

uint8_t BGSSaveLoadGame::GetSaveMod(uint8_t a2) {
	return cFileIndexArray[a2];
}

bool BGSSaveLoadGame::GetSaveLoadFailed() {
	return ((((int32_t)uiGlobalFlags) & 128) != 0);
}

void BGSSaveLoadGame::TogglePositioningPlayer(char a2) {
	uint32_t v2;
	if (a2) {
		v2 = uiGlobalFlags | 32;
	} else {
		v2 = uiGlobalFlags & 0xFFFFFFDFu;
	}
	uiGlobalFlags = v2;
}

void BGSSaveLoadGame::ClearFormID(uint32_t aiFormID) {
	TESForm* formByNumericID = TESForm::GetFormByNumericID(aiFormID);
	ClearForm(formByNumericID);
}

void BGSSaveLoadGame::SaveGameWarningFunction(const char* apText, va_list aList) {
	pHistory->AddNoteVarArgs(apText, aList);
}

uint32_t BGSSaveLoadGame::GetChangedFormID(uint32_t aiOldFormID) {
	uint32_t a2 = 0;
	bool vCondition1;
	vCondition1 =
		pChangedFormIDMap->NiTMapBase<DFALL<NiTMapItem<uint32_t, uint32_t>>, uint32_t, uint32_t>::GetAt(aiOldFormID,
			a2);
	if (vCondition1) {
		return a2;
	} else {
		return (uint32_t)0;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void BGSSaveLoadGame::CleanupExpiredData() {
	ReferenceInitialData refData_2;
	ReferenceInitialData refData;
	CellInitialData cellData;
	using ExpiredCells_t = NiTMap<int64_t, int32_t>;
	FNVD_INLINED_LOCAL(ExpiredCells_t, ExpiredCells, std::nullopt, std::nullopt);
	NiTMapItem<uint32_t, BGSFormChanges*>* it;
	bool brestore = SetThreadAllowChanges(0);
	::new (&ExpiredCells) NiTMap<int64_t, int32_t>(37u);
	uint32_t icurrentTime = appCalendar.GetHoursPassed();
	uint32_t iexpiredCellCount = 0;
	it = pChangesMap->GetFirstPos();
	while (it) {
		uint32_t iformID = 0;
		BGSFormChanges* pchanges = 0;
		pChangesMap->GetNext(it, iformID, pchanges);
		uint32_t ilocationID = iformID;
		if (pchanges->iChangeFlags.CheckFlags(CHANGE_CELL_DETACHTIME)) {
			int32_t idetachTime = 0;
			int32_t icellX = 0x7FFFFFFF;
			int32_t icellY = 0x7FFFFFFF;
			if (pchanges->UnloadedFormBuffer.pBuffer) {
				if (pchanges->UnloadedFormBuffer.GetFormType() == CELL_ID) {
					INITIAL_DATA_TYPE einitialDataType =
						BGSSaveLoadInitialData::GetInitialDataType(iformID, CELL_ID, pchanges->iChangeFlags);
					if (einitialDataType) {
						INITIAL_DATA_TYPE v6 = einitialDataType;
						char* Data = pchanges->UnloadedFormBuffer.GetAdvancedBuffer();
						BGSSaveLoadInitialData::LoadInitialDataStruct(Data, v6, &cellData);
						idetachTime = cellData.iDetachTime;
						if (einitialDataType != INITIAL_DATA_INTERIOR_CELL) {
							ilocationID = cellData.iWorldSpaceID;
							icellX = cellData.iCellX;
							icellY = cellData.iCellY;
						}
					}
				}
			} else {
				TESObjectCELL* pcell = static_cast<TESObjectCELL*>(TESForm::GetFormByNumericID(iformID));
				if (pcell) {
					if (pcell->GetFormType() == CELL_ID) {
						TESObjectCELL* pcell_2 = pcell;
						idetachTime = pcell->GetDetachTime();
						if (!pcell->GetInterior()) {
							TESWorldSpace* WorldSpace = pcell_2->GetWorldSpace();
							ilocationID = WorldSpace->iFormID;
							icellX = pcell_2->GetDataX();
							icellY = pcell_2->GetDataY();
						}
					}
				}
			}
			if (idetachTime) {
				uint32_t v3 = icurrentTime - idetachTime;
				if (v3 > TESObjectCELL::GetHoursToRespawnCell()) {
					int64_t ilocationKey = ilocationID;
					if (icellX != 0x7FFFFFFF) {
						int64_t icellKey =
							TESObjectCELL::GetCoord(static_cast<uint16_t>(icellX), static_cast<uint16_t>(icellY));
						ilocationKey |= icellKey << 32;
					}
					ExpiredCells.SetAt(ilocationKey, idetachTime);
					++iexpiredCellCount;
				}
			}
		}
	}
	if (iexpiredCellCount) {
		uint32_t iremoveHavokCount = 0;
		uint32_t iremoveHavokFullCount = 0;
		uint32_t iremoveHavokUnloadedPartialCount = 0;
		it = pChangesMap->GetFirstPos();
		while (it) {
			uint32_t iformID_2 = 0;
			BGSFormChanges* pchanges_2 = 0;
			pChangesMap->GetNext(it, iformID_2, pchanges_2);
			if (!pDataHandler->IsFormIDCreated(iformID_2)) {
				uint32_t ilocationID_2 = 0;
				int32_t icellX_2 = 0;
				int32_t icellY_2 = 0;
				TESObjectREFR* pref = 0;
				ENUM_FORM_ID eformType_2 = NO_FORM;
				if (pchanges_2->UnloadedFormBuffer.pBuffer) {
					eformType_2 = pchanges_2->UnloadedFormBuffer.GetFormType();
				} else {
					TESForm* pform = TESForm::GetFormByNumericID(iformID_2);
					if (pform) {
						eformType_2 = pform->GetFormType();
						if (pform->IsReference()) {
							pref = static_cast<TESObjectREFR*>(pform);
						}
					} else {
						int32_t someFloat = pchanges_2->iChangeFlags.iFlags;
						SaveGameWarning("During cleanup, form %08X with changeflags %08X and no unloaded form buffer "
										"could not be found",
							iformID_2, someFloat);
					}
				}
				if (eformType_2 == REFR_ID) {
					if (pchanges_2->UnloadedFormBuffer.pBuffer) {
						INITIAL_DATA_TYPE einitialDataType_2 = BGSSaveLoadInitialData::GetInitialDataType(iformID_2,
							eformType_2, pchanges_2->iChangeFlags);
						if (einitialDataType_2) {
							refData.~ReferenceInitialData();
							::new (&refData) ReferenceInitialData;
							INITIAL_DATA_TYPE v7 = einitialDataType_2;
							char* v4 = pchanges_2->UnloadedFormBuffer.GetAdvancedBuffer();
							BGSSaveLoadInitialData::LoadInitialDataStruct(v4, v7, &refData);
							ilocationID_2 = refData.iLocationID;
							icellX_2 = FloatToInt(refData.Location.x) >> 12;
							icellY_2 = FloatToInt(refData.Location.y) >> 12;
						}
					} else {
						if (pref) {
							TESObjectCELL* pparentCell = pref->GetParentCell();
							TESWorldSpace* pworld = pref->GetWorldSpace();
							if (pparentCell && pparentCell->GetInterior()) {
								ilocationID_2 = pparentCell->iFormID;
							} else {
								if (pworld) {
									ilocationID_2 = pworld->iFormID;
									NiPoint3 v26 = pref->GetLocationOnReference();
									icellX_2 = FloatToInt(v26.x) >> 12;
									icellY_2 = FloatToInt(v26.y) >> 12;
								}
							}
						}
					}
				}
				if (ilocationID_2) {
					int64_t ilocationKey_2 = ilocationID_2;
					if (icellX_2 != 0x7FFFFFFF) {
						int64_t icellKey_2 =
							TESObjectCELL::GetCoord(static_cast<uint16_t>(icellX_2), static_cast<uint16_t>(icellY_2));
						ilocationKey_2 |= icellKey_2 << 32;
					}
					int32_t idetachTime_2 = 0;
					if (ExpiredCells.GetAt(ilocationKey_2, idetachTime_2) &&
						pchanges_2->iChangeFlags.CheckFlags(CHANGE_REFR_HAVOK_MOVE) &&
						!pchanges_2->iChangeFlags.CheckFlags(CHANGE_REFR_MOVE)) {
						++iremoveHavokCount;
						INITIAL_DATA_TYPE einitialDataType_3 = BGSSaveLoadInitialData::GetInitialDataType(iformID_2,
							eformType_2, pchanges_2->iChangeFlags);
						refData_2.~ReferenceInitialData();
						::new (&refData_2) ReferenceInitialData;
						if (pchanges_2->UnloadedFormBuffer.pBuffer) {
							if (einitialDataType_3 == INITIAL_DATA_MOVED_REFERENCE) {
								char* v5 = pchanges_2->UnloadedFormBuffer.GetAdvancedBuffer();
								BGSSaveLoadInitialData::LoadInitialDataStruct(v5, INITIAL_DATA_MOVED_REFERENCE,
									&refData_2);
								pReferencesMap->RemoveMovedReference(iformID_2);
								TESForm* pform_2 = TESForm::GetFormByNumericID(refData_2.iLocationID);
								if (pform_2 && pform_2->GetFormType() == WRLD_ID) {
									pReferencesMap->RemoveReference(refData_2.iLocationID, iformID_2,
										refData_2.Location);
								} else {
									pReferencesMap->RemoveReference_0(refData_2.iLocationID, iformID_2);
								}
							}
						}
						if (pchanges_2->iChangeFlags.Equals(CHANGE_REFR_HAVOK_MOVE)) {
							pChangesMap->RemoveChanges(iformID_2);
							++iremoveHavokFullCount;
						} else {
							if (pchanges_2->UnloadedFormBuffer.pBuffer) {
								if (einitialDataType_3) {
									char* apBuffer = pchanges_2->UnloadedFormBuffer.GetAdvancedBuffer();
									uint32_t iinitialDataSize =
										BGSSaveLoadInitialData::GetInitialDataTypeDataSize(einitialDataType_3);
									char* pbuffer = &apBuffer[iinitialDataSize];
									uint32_t ihavokDataSize = 0;
									uint32_t ihavokDataSizeTypeSize =
										LoadVariableSizedValueFromBuffer(pbuffer, ihavokDataSize);
									char* pbuffer_2 = &pbuffer[ihavokDataSize + ihavokDataSizeTypeSize];
									char* poriginalBuffer = pchanges_2->UnloadedFormBuffer.pBuffer;
									uint8_t cversion = pchanges_2->UnloadedFormBuffer.GetVersion();
									uint32_t idataSize = pchanges_2->UnloadedFormBuffer.GetSize();
									uint32_t inewDataSize =
										idataSize - iinitialDataSize - ihavokDataSizeTypeSize - ihavokDataSize;
									// The exe's destructor copy 0x6665A0 only clears the pointer: the old buffer is freed
									// below.
									BGSUnloadedFormBuffer::DestructKeepBuffer(&pchanges_2->UnloadedFormBuffer);
									pchanges_2->UnloadedFormBuffer.Initialize(pbuffer_2, inewDataSize, eformType_2,
										cversion);
									ourFree(poriginalBuffer);
									++iremoveHavokUnloadedPartialCount;
								}
							}
							pchanges_2->iChangeFlags.RemoveFlags(12);
						}
						if (pref) {
							pref->MoveToEditorLocation(0);
						}
					}
				}
			}
		}
		if (iremoveHavokCount) {
			pHistory->AddNote("CleanupExpiredData removed havok from %i refs: %i removed all changes and %i unloaded "
							  "refs removed partial changes",
				iremoveHavokCount, iremoveHavokFullCount, iremoveHavokUnloadedPartialCount);
		}
	}
	ExpiredCells.RemoveAll_1();
	SetThreadAllowChanges(brestore);
	ExpiredCells.~NiTMap<int64_t, int32_t>();
}

// The exe's cleanup-only EH frame (for the new'd objects whose constructors may throw) is left out.
bool BGSSaveLoadGame::LoadGame(BGSSaveLoadFile* apFile, bool abIgnoreMissingContent) {
	char cFullFileName[260];
	char cVersionInfo[260];
	char cSaveName[260];

	BGSSaveLoadHistory* pOldHistory = pHistory;
	void* pMemory = ourMalloc(sizeof(BGSSaveLoadHistory));
	pHistory = pMemory ? ::new (pMemory) BGSSaveLoadHistory : nullptr;
	pSaveLoadManager->GetFullFileName(apFile->GetSaveFileName(), cFullFileName);
	pSaveLoadManager->GetVersionInfo(cVersionInfo, 0x104);
	pHistory->AddNote("***Loading game: %s (%s)", cFullFileName, cVersionInfo);
	LoadGameDataFromFileFunction(apFile, &cCurrentMinorVersion, 1);
	bool bPluginsLoaded = LoadPluginList(apFile);
	if (!bPluginsLoaded && !abIgnoreMissingContent) {
		BGSSaveLoadHistory* pNewHistory = pHistory;
		if (pNewHistory) {
			delete pNewHistory;
		}
		pHistory = pOldHistory;
		cCurrentMinorVersion = 0xFF;
		StartMenu::SetFlag20000000(true);
		return false;
	}
	if (pOldHistory) {
		delete pOldHistory;
	}
	pReferencesMap->Clear();
	pSaveLoadManager->StartLoadGame();
	pIOManager->Pause();
	SetSaveGameLoading(true);
	pPlayer->ClearTeammates();

	SaveGameFormInfo kHeader;
	kHeader.~SaveGameFormInfo();
	::new (&kHeader) SaveGameFormInfo;
	apFile->Read(&kHeader, 0x6E);
	uint32_t iHeaderEnd = apFile->GetFilePosition();
	apFile->SeekSet(kHeader.iFormIDMapOffset);

	BGSSaveLoadFormIDMap* pOldFormIDMap = pFormIDMap;
	pMemory = ourMalloc(sizeof(BGSSaveLoadFormIDMap));
	pFormIDMap = pMemory ? ::new (pMemory) BGSSaveLoadFormIDMap(0x13AF) : nullptr;
	pFormIDMap->Load(apFile);
	pWorldspaceFormIDMap->Load(apFile);

	BGSSaveLoadHistory* pLoadingHistory = pHistory;
	pMemory = ourMalloc(sizeof(BGSSaveLoadHistory));
	pHistory = pMemory ? ::new (pMemory) BGSSaveLoadHistory : nullptr;
	pHistory->Load(apFile);
	pHistory->AddHistory(pLoadingHistory);
	if (pLoadingHistory) {
		delete pLoadingHistory;
	}
	apFile->SeekSet(iHeaderEnd);

	// The initial global data loads into a fresh changes map, which then holds the save's changes; the current
	// map goes back in place.
	pOldChangesMap = pChangesMap;
	pMemory = ourMalloc(sizeof(BGSSaveLoadChangesMap));
	pChangesMap = pMemory ? ::new (pMemory) BGSSaveLoadChangesMap : nullptr;
	BGSSaveLoadGlobalData::RevertGlobalData(false);
	pDataHandler->SetIsLoading(true);
	BGSSaveLoadGlobalData::LoadGlobalData(apFile, kHeader.iInitialGlobalFormCount);
	BGSSaveLoadChangesMap* pLoadedChangesMap = pChangesMap;
	pChangesMap = pOldChangesMap;
	pOldChangesMap = nullptr;
	uint32_t iFormDataPosition = apFile->GetFilePosition();
	FormBufferArray.Clear(false);
	FormBufferArray.SetReservedSize(kHeader.iFormCount);
	SetGlobalAllowChanges(false);

	for (uint32_t i = 0; i < kHeader.iFormCount; ++i) {
		pMemory = ourMalloc(sizeof(BGSLoadFormBuffer));
		BGSLoadFormBuffer* pLoadFormBuffer = pMemory ? ::new (pMemory) BGSLoadFormBuffer : nullptr;
		pLoadFormBuffer->LoadHeader(apFile);
		ENUM_FORM_ID eFormType = pLoadFormBuffer->GetFormType();
		uint32_t iFormID = pLoadFormBuffer->GetFormID();
		BGSChangeFlags kNewChangeFlags = pLoadFormBuffer->GetChangeFlags();
		BGSChangeFlags kOldChangeFlags;
		pChangesMap->GetChangeFlags(&kOldChangeFlags, iFormID);
		BGSChangeFlags kLoadedChangeFlags;
		pLoadedChangesMap->GetChangeFlags(&kLoadedChangeFlags, iFormID);
		if (static_cast<int32_t>(kLoadedChangeFlags)) {
			kOldChangeFlags.AddFlags(static_cast<int32_t>(kLoadedChangeFlags));
			pLoadedChangesMap->RemoveChanges(iFormID);
		}
		pLoadedChangesMap->SetChangeFlags(iFormID, kNewChangeFlags);
		pLoadFormBuffer->SetOldChangeFlags(kOldChangeFlags);
		TESForm* pForm = TESForm::GetFormByNumericID(iFormID);
		pLoadFormBuffer->SetForm(pForm);
		int32_t iLoaded = CheckInitialData(apFile, pLoadFormBuffer, pOldFormIDMap);
		pLoadFormBuffer->SkipData(apFile, iLoaded);
		pForm = pLoadFormBuffer->GetForm();
		kOldChangeFlags = pLoadFormBuffer->GetOldChangeFlags();
		bool bRemoveChanges = false;
		if (pForm) {
			bool bSkip = false;
			if (pForm->GetFormType() != eFormType) {
				bSkip = true;
			} else if (pForm->IsReference()) {
				if (!static_cast<TESObjectREFR*>(pForm)->GetObjectReference()) {
					bSkip = true;
				}
			}
			if (bSkip) {
				if (pDataHandler->IsFormIDCreated(iFormID)) {
					ClearForm(pForm);
					pLoadFormBuffer->SetForm(nullptr);
					BGSChangeFlags kNoChanges;
					kNoChanges.~BGSChangeFlags();
					::new (&kNoChanges) BGSChangeFlags(0);
					pLoadFormBuffer->SetOldChangeFlags(kNoChanges);
					bRemoveChanges = true;
				} else {
					if (pForm->GetFormType() != eFormType) {
						// The exe evaluates the arguments right to left.
						const char* pCurrentType = pForm->GetFormTypeString();
						const char* pSavedType = TESForm::GetFormTypeString(static_cast<uint8_t>(eFormType));
						const char* pEditorID = pForm->GetFormEditorID();
						SaveGameWarning("SAVELOAD: Form '%s' (%08X) was saved with form type %s, but currently has "
										"form type %s.  Its loading will be skipped.",
							pEditorID, iFormID, pSavedType, pCurrentType);
					}
					pLoadFormBuffer->SetSkipForm(true);
				}
			} else {
				pForm->Revert(pLoadFormBuffer);
				BGSChangeFlags kUnrevertible;
				BGSChangeFlags::GetUnrevertibleChanges(&kUnrevertible, kOldChangeFlags, kNewChangeFlags, eFormType,
					true);
				if (static_cast<int32_t>(kUnrevertible)) {
					if (!HandleUnrevertibleChanges(pForm, kUnrevertible, false)) {
						pReconstructForms->AddForm(pForm, 0);
						pLoadFormBuffer->SetReconstructedForm(true);
					}
				}
				bRemoveChanges = true;
			}
		} else {
			bRemoveChanges = true;
		}
		if (bRemoveChanges) {
			pChangesMap->RemoveChanges(iFormID);
		}
		FormBufferArray.Add(pLoadFormBuffer);
	}

	RevertCurrentChanges(pLoadedChangesMap, pOldFormIDMap);
	BSShaderManager::ProcessAllQueuedLights();
	if (pOldFormIDMap) {
		delete pOldFormIDMap;
	}
	pDataHandler->SetIsLoading(false);
	PerformQueuedInitPackageLocations();
	apFile->SeekSet(iFormDataPosition);

	for (uint32_t i = 0; i < kHeader.iFormCount; ++i) {
		BGSLoadFormBuffer* pLoadFormBuffer = FormBufferArray[i];
		if (pLoadFormBuffer->GetSkipForm()) {
			pChangesMap->RemoveChanges(pLoadFormBuffer->GetFormID());
			pLoadFormBuffer->LoadHeader(apFile);
			pLoadFormBuffer->SkipData(apFile, 0);
			if (pLoadFormBuffer) {
				delete pLoadFormBuffer;
			}
			BGSLoadFormBuffer* pNone = nullptr;
			FormBufferArray.SetAt(i, pNone);
			continue;
		}
		uint32_t iFormID = pLoadFormBuffer->GetFormID();
		TESForm* pForm = pLoadFormBuffer->GetForm();
		if (pForm) {
			pLoadFormBuffer->LoadHeader(apFile);
			ENUM_FORM_ID eFormType = pLoadFormBuffer->GetFormType();
			bool bSkip = false;
			if (pForm->GetFormType() != eFormType) {
				// The exe evaluates the arguments right to left.
				const char* pCurrentType = pForm->GetFormTypeString();
				const char* pSavedType = TESForm::GetFormTypeString(static_cast<uint8_t>(eFormType));
				const char* pEditorID = pForm->GetFormEditorID();
				SaveGameWarning("SAVELOAD: Form '%s' (%08X) was saved with form type %s, but currently has form "
								"type %s.  Its loading will be skipped.",
					pEditorID, iFormID, pSavedType, pCurrentType);
				bSkip = true;
			} else if (pForm->IsReference()) {
				if (!static_cast<TESObjectREFR*>(pForm)->GetObjectReference()) {
					bSkip = true;
				}
			}
			if (bSkip) {
				pChangesMap->RemoveChanges(pLoadFormBuffer->GetFormID());
				pLoadFormBuffer->SkipData(apFile, 0);
				if (pLoadFormBuffer) {
					delete pLoadFormBuffer;
				}
				BGSLoadFormBuffer* pNone = nullptr;
				FormBufferArray.SetAt(i, pNone);
				continue;
			}
			// The exe tests the byte count as signed (jl): -1 is a short read.
			if (static_cast<int32_t>(pLoadFormBuffer->LoadDataFromFile(apFile)) < 0 || GetSaveLoadFailed()) {
				pLoadFormBuffer->SkipData(apFile, 0);
				if (pLoadFormBuffer) {
					delete pLoadFormBuffer;
				}
				BGSLoadFormBuffer* pNone = nullptr;
				FormBufferArray.SetAt(i, pNone);
				FormBufferArray.Clear(true);
				pPlayer->SetParentCell(nullptr);
				SetGlobalAllowChanges(true);
				SetSaveGameLoading(false);
				return false;
			}
			BGSSaveLoadInitialData::LoadInitialData(pLoadFormBuffer, pForm, pLoadFormBuffer->GetChangeFlags());
			if (pLoadFormBuffer->GetForm()) {
				if (!GetSaveLoadFailed()) {
					pForm->SetStillLoading(true);
					pForm->LoadGame(pLoadFormBuffer);
				}
			}
		} else {
			BGSUnloadedFormBuffer kUnloadedBuffer;
			kUnloadedBuffer.~BGSUnloadedFormBuffer();
			::new (&kUnloadedBuffer) BGSUnloadedFormBuffer;
			kUnloadedBuffer.Load(apFile);
			pChangesMap->SetChanges(iFormID, pLoadFormBuffer->GetChangeFlags(), kUnloadedBuffer);
		}
		pLoadFormBuffer->ClearBuffer();
	}

	apFile->SeekCurrent(kHeader.iSecondaryDataSize);
	BGSSaveLoadGlobalData::InitInitialGlobalData();
	InitForms(true);
	BGSSaveLoadGlobalData::FinishLoadInitialGlobalData();
	BGSSaveLoadGlobalData::LoadGlobalData(apFile, kHeader.iFinalGlobalFormCount);
	cCurrentMinorVersion = 0xFF;
	BGSSaveLoadManager::CreateSystemSaveName(cSaveName);
	pSaveLoadManager->pSaveName.Set(cSaveName, 0);
	pHistory->AddNote("---Finished loading game: %s", cFullFileName);
	SetGlobalAllowChanges(true);
	appCalendar.SetGameLoaded();
	SetSaveGameLoading(false);
	return true;
}

// The exe's cleanup-only C++ EH frame is left out.
bool BGSSaveLoadGame::LoadPluginList(BGSSaveLoadFile* apFile) {
	uint8_t ccount;
	uint8_t ccurrentCount = pDataHandler->CountCompiled();
	FNVD_CONSTRUCTED_LOCAL(BGSLoadGameBuffer, loadBuffer, &BGSLoadGameBuffer::Destruct);
	loadBuffer.Load(apFile);
	BGSLoadGameBuffer* apLoadGameBuffer = &loadBuffer;
	bool bpluginsComplete = 1;
	ourMemset(cFileIndexArray, 255, 255u);
	ourMemset(cReverseFileIndexArray, 255, 255u);
	ccount = 0;
	loadBuffer.LoadData(&ccount, 1u);
	for (uint32_t i = 0; i < ccount; ++i) {
		char apString1[264];
		apLoadGameBuffer->LoadString(apString1);
		bool bfound = 0;
		for (uint32_t ii = 0; ii < ccurrentCount; ++ii) {
			TESFile* pfile = pDataHandler->GetCompiledFile(static_cast<int32_t>(ii));
			if (!BSstricmp(apString1, pfile->m_Filename)) {
				cFileIndexArray[i] = ii;
				cReverseFileIndexArray[ii] = i;
				bfound = 1;
				break;
			}
		}
		if (!bfound) {
			bpluginsComplete = 0;
			cFileIndexArray[i] = -1;
			SaveGameWarning("SAVELOAD: Cannot find file %s referenced in the save game.  Errors may result.",
				apString1);
		}
	}
	return bpluginsComplete;
}

// Created and moved references carry their initial data in the save. A reference whose cell is loaded is recreated
// (created) or queued for reconstruction (moved); otherwise it is recorded in the references map and the loaded form,
// if any, is deleted. A reference whose changes no longer say "moved" is deleted when its editor cell is not loaded,
// or queued for reconstruction from its old buffer when it has no form. Each zero change-flags argument is built in
// its stack slot by the exe's constructor 0x8C71B0.
uint32_t BGSSaveLoadGame::CheckInitialData(BGSSaveLoadFile* apFile, BGSLoadFormBuffer* apLoadGameBuffer,
	BGSSaveLoadFormIDMap* apOldFormIDMap) {
	uint32_t iLoaded = 0;
	uint32_t eFormType = apLoadGameBuffer->GetFormType();
	BGSChangeFlags kChangeFlags = apLoadGameBuffer->GetChangeFlags();
	BGSChangeFlags kOldChangeFlags = apLoadGameBuffer->GetOldChangeFlags();
	uint32_t iFormID = apLoadGameBuffer->GetFormID();
	TESForm* pForm = apLoadGameBuffer->GetForm();
	// The exe pushes GetFormType's whole dword.
	int32_t eType = BGSSaveLoadInitialData::GetInitialDataType(iFormID, static_cast<int32_t>(eFormType), kChangeFlags);
	if (eType == INITIAL_DATA_CREATED_REFERENCE || eType == INITIAL_DATA_MOVED_REFERENCE) {
		ReferenceInitialData kData;
		kData.~ReferenceInitialData();
		::new (&kData) ReferenceInitialData;
		iLoaded = BGSSaveLoadInitialData::LoadInitialDataStruct(apFile, static_cast<INITIAL_DATA_TYPE>(eType), &kData);
		TESObjectREFR* pRef = nullptr;
		if (pForm) {
			bool bClear = false;
			if (!pForm->IsReference()) {
				if (eType == INITIAL_DATA_CREATED_REFERENCE)
					bClear = true;
			} else {
				pRef = static_cast<TESObjectREFR*>(pForm);
				if (eType == INITIAL_DATA_CREATED_REFERENCE &&
					pRef->GetObjectReference()->GetFormID() != kData.iBoundID)
					bClear = true;
			}
			if (bClear) {
				ClearForm(pForm);
				apLoadGameBuffer->SetForm(nullptr);
				BGSChangeFlags kNoChanges;
				kNoChanges.~BGSChangeFlags();
				::new (&kNoChanges) BGSChangeFlags(0);
				apLoadGameBuffer->SetOldChangeFlags(kNoChanges);
				pForm = nullptr;
				pRef = nullptr;
			}
		}
		TESObjectCELL* pCell = nullptr;
		TESWorldSpace* pWorld = nullptr;
		kData.GetLocationCellAndWorld(pCell, pWorld);
		if ((pCell && TES::pInstance->IsCellLoaded(pCell, false)) || (kData.cCreatedReferenceFlags & 2) != 0) {
			if (eType == INITIAL_DATA_CREATED_REFERENCE) {
				if (pForm) {
					BGSChangeFlags kUnrevertible;
					BGSChangeFlags::GetUnrevertibleChanges(&kUnrevertible, kOldChangeFlags, kChangeFlags, eFormType,
						true);
					if (static_cast<int32_t>(kUnrevertible)) {
						DeleteForm(pForm);
						pForm = nullptr;
						BGSChangeFlags kNoChanges;
						kNoChanges.~BGSChangeFlags();
						::new (&kNoChanges) BGSChangeFlags(0);
						apLoadGameBuffer->SetOldChangeFlags(kNoChanges);
					}
				}
				pForm = BGSSaveLoadInitialData::CreateReference(iFormID, &kData, pForm);
				if (pForm)
					apLoadGameBuffer->SetForm(pForm);
				else
					apLoadGameBuffer->SetSkipForm(true);
			} else if (eType == INITIAL_DATA_MOVED_REFERENCE && !pForm) {
				kData.GetOriginalLocationCellAndWorld(pCell, pWorld);
				if (pCell)
					pReconstructForms->AddReference(pCell, iFormID, 0);
				else
					pReconstructForms->AddReference(pWorld, kData.iOriginalCellX, kData.iOriginalCellY, iFormID, 0);
				apLoadGameBuffer->SetReconstructedForm(true);
				apLoadGameBuffer->SetFormPending(true);
			}
		} else if (!pRef || (!pRef->GetRefPersists() && pRef != pPlayer)) {
			if (pCell && pCell->GetInterior()) {
				pReferencesMap->AddReference(kData.iLocationID, iFormID);
				if (eType == INITIAL_DATA_MOVED_REFERENCE)
					pReferencesMap->AddMovedReference(iFormID);
			} else if (pWorld) {
				pReferencesMap->AddReference(kData.iLocationID, iFormID, kData.Location);
				if (eType == INITIAL_DATA_MOVED_REFERENCE)
					pReferencesMap->AddMovedReference(iFormID);
			} else {
				SaveGameWarning("CELLS: Worldspace or Cell %08X could not be found while loading a %s reference.  Its "
								"loading will be skipped.",
					kData.iLocationID, eType == INITIAL_DATA_CREATED_REFERENCE ? "created" : "moved");
				apLoadGameBuffer->SetSkipForm(true);
			}
			if (pRef) {
				if (pRef->GetParentCell())
					pRef->GetParentCell()->RemoveReference(pRef);
				DeleteForm(pForm);
				pForm = nullptr;
				apLoadGameBuffer->SetForm(nullptr);
			}
		}
	} else {
		int32_t eOldType =
			BGSSaveLoadInitialData::GetInitialDataType(iFormID, static_cast<int32_t>(eFormType), kOldChangeFlags);
		if (eOldType == INITIAL_DATA_MOVED_REFERENCE) {
			if (!pForm) {
				BGSUnloadedFormBuffer kBuffer;
				pChangesMap->GetChangeBuffer(&kBuffer, iFormID);
				if (static_cast<char*>(kBuffer)) {
					BGSSaveLoadFormIDMap* pSavedFormIDMap = pFormIDMap;
					pFormIDMap = apOldFormIDMap;
					ReferenceInitialData kData;
					kData.~ReferenceInitialData();
					::new (&kData) ReferenceInitialData;
					BGSSaveLoadInitialData::LoadInitialDataStruct(kBuffer.GetAdvancedBuffer(),
						static_cast<INITIAL_DATA_TYPE>(eOldType), &kData);
					TESObjectCELL* pCell = nullptr;
					TESWorldSpace* pWorld = nullptr;
					kData.GetOriginalLocationCellAndWorld(pCell, pWorld);
					if (pCell && TES::pInstance->IsCellLoaded(pCell, false)) {
						pReconstructForms->AddReference(pCell, iFormID, 0);
						apLoadGameBuffer->SetReconstructedForm(true);
						apLoadGameBuffer->SetFormPending(true);
					}
					pFormIDMap = pSavedFormIDMap;
				}
			} else if (pForm->IsReference()) {
				TESObjectREFR* pRef = static_cast<TESObjectREFR*>(pForm);
				if (!pRef->GetRefPersists()) {
					TESForm* pLocation = nullptr;
					NiPoint3 kLocation = NiPoint3_ZERO;
					NiPoint3 kAngle = NiPoint3_ZERO;
					bool bDelete = false;
					if (!pRef->GetEditorLocation(kLocation, kAngle, pLocation, nullptr)) {
						bDelete = true;
					} else {
						TESObjectCELL* pCell = nullptr;
						if (pLocation->GetFormType() == WRLD_ID)
							pCell = static_cast<TESWorldSpace*>(pLocation)->GetCellFromWorldCoord(kLocation);
						else
							pCell = static_cast<TESObjectCELL*>(pLocation);
						if (!pCell || !TES::pInstance->IsCellLoaded(pCell, false))
							bDelete = true;
					}
					if (bDelete) {
						DeleteForm(pForm);
						apLoadGameBuffer->SetForm(nullptr);
						BGSChangeFlags kNoChanges;
						kNoChanges.~BGSChangeFlags();
						::new (&kNoChanges) BGSChangeFlags(0);
						apLoadGameBuffer->SetOldChangeFlags(kNoChanges);
						pForm = nullptr;
					}
				}
			}
		}
	}
	return iLoaded;
}

bool BGSSaveLoadGame::HandleUnrevertibleChanges(TESForm*& arpForm, BGSChangeFlags aiUnrevertibleChanges,
	bool abRevertOnly) {
	if (arpForm == pPlayer) {
		return 1;
	}
	if (arpForm->IsReference()) {
		Actor* pactor = static_cast<Actor*>(arpForm);
		if (aiUnrevertibleChanges.CheckFlags(14)) {
			uint32_t CastingType = arpForm->iFormID;
			if (pDataHandler->IsFormIDCreated(CastingType) || pactor->MoveToEditorLocation(0)) {
				aiUnrevertibleChanges.RemoveFlags(14);
				if (!aiUnrevertibleChanges.iFlags && pactor->IsActor()) {
					QueueInitPackageLocations(pactor);
				}
			} else {
				if (!abRevertOnly || pactor->GetRefPersists()) {
					const char* FullName;
					if (pactor->GetFormEditorIDLength()) {
						FullName = pactor->GetFormEditorID();
					} else {
						FullName = pactor->GetFullName();
					}
					int32_t v7 = aiUnrevertibleChanges.iFlags;
					const char* FormTypeString = pactor->GetObjectReference()->GetFormTypeString();
					SaveGameWarning("Reference %s %08X with type %s and with changes %08X could not revert its changed "
									"location",
						FullName, pactor->iFormID, FormTypeString, v7);
				} else {
					DeleteForm(pactor);
					arpForm = 0;
				}
			}
		}
	}
	return aiUnrevertibleChanges.iFlags == 0;
}

// A projectile is unloaded only when it is permanent.
bool BGSSaveLoadGame::ShouldUnloadForm(TESForm* apForm) {
	if (apForm->GetTemporary()) {
		return false;
	}
	if (apForm->IsReference()) {
		TESObjectREFR* pRef = static_cast<TESObjectREFR*>(apForm);
		if (pRef->IsProjectile()) {
			Projectile* pProjectile = static_cast<Projectile*>(pRef);
			if (!pProjectile->IsPermanent()) {
				return false;
			}
		}
	}
	return true;
}

// Allocates the singleton, installs its constructor result, then initializes form types.
void BGSSaveLoadGame::Create() {
	void* pMemory = ourMalloc(sizeof(BGSSaveLoadGame));
	BGSSaveLoadGame* pNewGame = pMemory ? ::new (pMemory) BGSSaveLoadGame : nullptr;
	pSaveLoadGame = pNewGame;
	BGSSaveLoadFormInfo::InitSaveGameFormTypes();
}

void BGSSaveLoadGame::Destroy() {
	if (pSaveLoadGame) {
		delete pSaveLoadGame;
	}
	pSaveLoadGame = nullptr;
}

bool BGSLoadFormBuffer::GetSkipForm() {
	return (iFlags & LOAD_FORM_SKIP_FORM) != 0;
}

void BGSLoadFormBuffer::SetSkipForm(bool abValue) {
	uint32_t v2;
	if (abValue) {
		v2 = iFlags | 1;
	} else {
		v2 = iFlags & 0xFFFFFFFEu;
	}
	iFlags = v2;
}

uint32_t BGSLoadFormBuffer::GetFormID() {
	return iFormID;
}

void BGSSaveGameBuffer::ClearPosition() {
	iBufferPosition = 0;
	iDebugOverhead = 0;
}

char* BGSSaveLoadFile::GetSaveFileName() {
	return pSaveFileName;
}

ReferenceInitialData::ReferenceInitialData() {}

void BGSSaveLoadReferencesMap::AddMovedReference(uint32_t aiFormID) {
	MovedReferencesMap.SetAt(aiFormID, aiFormID);
}

void BGSSaveLoadReferencesMap::AddReference(uint32_t aiCellID, uint32_t aiReferenceID) {
	InteriorReferencesMap.AddReference(aiCellID, aiReferenceID);
}

bool BGSSaveLoadReferencesMap::RemoveMovedReference(uint32_t aiFormID) {
	return MovedReferencesMap.RemoveAt(aiFormID);
}
