#include "Fallout Shared/TESFaction.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESReactionForm.h"
#include "Fallout Shared/TESReputation.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

// Reads the faction's chunks. RNAM starts a new rank; the MNAM, FNAM and INAM chunks after it fill that rank's
// titles and insignia. The exe's cleanup-only EH frame (over the rank allocation) is left out.
// The exe reads the EDID, MNAM, FNAM and INAM buffers and the old-format flags byte uninitialised when the chunk is
// empty (malformed data); src initialises them to "" and 0 (allocated even for an empty chunk).
bool TESFaction::Load(TESFile* apFile) {
	RANK_DATA* pRankData = nullptr;
	LoadForm(apFile);
	do {
		switch (apFile->GetTESChunk()) {
		case EDID_ID: {
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			uint32_t uiChunkSize = apFile->GetChunkSize();
			char* pEditorID = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
			// Not in the exe: an empty chunk leaves the buffer unwritten (see above).
			pEditorID[0] = '\0';
			apFile->GetChunkData(pEditorID, 0x200);
			static_cast<TESForm*>(this)->SetFormEditorID(pEditorID);
			break;
		}
		case OBND_ID:
			static_cast<TESForm*>(this)->LoadObjectBound(apFile);
			break;
		case FULL_ID:
			// The exe null-checks the form before taking the full name subobject's address.
			TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile);
			break;
		case XNAM_ID: {
			GROUP_REACTION* pReaction = static_cast<GROUP_REACTION*>(ourMalloc(sizeof(GROUP_REACTION)));
			apFile->GetChunkData(pReaction, 0xC);
#if FNVD_64BIT
			// The file's record is 0xC bytes, the form's 4-byte ID first.
			TESFile::WidenFormIDRecords(pReaction, 1, 0xC, sizeof(GROUP_REACTION),
				{{0, offsetof(GROUP_REACTION, pForm)}});
#endif
			if (apFile->QEndian())
				pReaction->Endian();
			static_cast<TESReactionForm*>(this)->QReactionList()->AddTail(pReaction);
			break;
		}
		case DATA_ID:
			if (apFile->GetFormVersion() >= 9) {
				LoadData(apFile, reinterpret_cast<char*>(&data), 4);
				if (apFile->QEndian())
					data.Endian();
			} else {
				// Older files store the flags in one signed byte, which the exe sign-extends. An empty chunk leaves
				// the slot unwritten (malformed data); src initialises it to 0.
				char cFlags = 0;
				LoadData(apFile, &cFlags, 1);
				data.iFlags = cFlags;
			}
			break;
		case RNAM_ID: {
			void* pMemory = ourMalloc(sizeof(RANK_DATA));
			pRankData = pMemory ? ::new (pMemory) RANK_DATA : nullptr;
			QRankDataList()->AddTail(pRankData);
			break;
		}
		case MNAM_ID:
			if (pRankData) {
				// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
				uint32_t uiChunkSize = apFile->GetChunkSize();
				char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
				// Not in the exe: an empty chunk leaves the buffer unwritten (see above).
				pBuffer[0] = '\0';
				apFile->GetChunkData(pBuffer, 0);
				pRankData->cMaleRankTitle.Set(pBuffer, 0);
			}
			break;
		case FNAM_ID:
			if (pRankData) {
				// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
				uint32_t uiChunkSize = apFile->GetChunkSize();
				char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
				// Not in the exe: an empty chunk leaves the buffer unwritten (see above).
				pBuffer[0] = '\0';
				apFile->GetChunkData(pBuffer, 0);
				pRankData->cFemaleRankTitle.Set(pBuffer, 0);
			}
			break;
		case INAM_ID:
			if (pRankData) {
				// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
				uint32_t uiChunkSize = apFile->GetChunkSize();
				char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
				// Not in the exe: an empty chunk leaves the buffer unwritten (see above).
				pBuffer[0] = '\0';
				apFile->GetChunkData(pBuffer, 0);
				// The exe calls the setter's body directly (0x489100).
				pRankData->TextureInsignia.TESTexture::SetTextureName(pBuffer);
			}
			break;
		case WMI1_ID: {
			uint32_t uiReputation = 0;
			apFile->GetChunkData(uiReputation);
			// The exe stores the chunk's dword in the pointer field unchanged.
			if (uiReputation)
				pReputation = reinterpret_cast<TESReputation*>(static_cast<uintptr_t>(uiReputation));
			break;
		}
		default:
			break;
		}
	} while (apFile->NextChunk());
	return true;
}

bool Cmd_GetPCFactionAttack(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
	afReturnValue = 0.0;
	if (apParam1) {
		if (static_cast<TESFaction*>(apParam1)->GetPCHasAttackedMember()) {
			afReturnValue = 1.0;
		}
	}
	if (Script::GetThreadSystemOutput()) {
		if (0.0 == (afReturnValue)) {
			Interface::PrintLine("PC did not attack a faction member.");
		} else {
			Interface::PrintLine("PC attacked a faction member.");
		}
	}
	return 1;
}

bool Cmd_GetPCEnemyofFaction(TESObjectREFR* apRefObject, void* apParam1, void* apParam2, double& afReturnValue) {
	afReturnValue = 0.0;
	if (apParam1) {
		if (static_cast<TESFaction*>(apParam1)->GetPCAnEnemy()) {
			afReturnValue = 1.0;
		}
	}
	if (GetThreadTLSData()->bSystemOutput) {
		if (0.0 == afReturnValue) {
			Interface::PrintLine("PC is not an enemy of this faction.");
		} else {
			Interface::PrintLine("PC Is an Enemy of the faction.");
		}
	}
	return 1;
}

RANK_DATA::RANK_DATA() : cMaleRankTitle(), cFemaleRankTitle(), TextureInsignia() {}

RANK_DATA::~RANK_DATA() {}

void RANK_DATA::operator delete(void* apMemory) {
	ourFree(apMemory);
}

void FACTION_DATA::Endian() {
	ENDIAN(iFlags, false);
}

TESFaction::TESFaction() : TESForm(), TESFullName(), TESReactionForm(), m_rankDataList() {
	TESFaction::InitializeData();
	SetFormType(FACT_ID);
}

TESFaction::~TESFaction() {
	TESFaction::ClearData();
}

void TESFaction::InitializeData() {
	iMajorCrime = (uint32_t)0;
	iMinorCrime = (uint32_t)0;
	pReputation = nullptr;
	ourMemset(&data.iFlags, 0, sizeof(data.iFlags));
}

void TESFaction::ClearData() {
	ClearRankDataList();
}

void TESFaction::Save() {
	StartForm();
	TESFullName::Save();
	TESReactionForm::Save();
	if (TESForm::QEndianSwapOnSave()) {
		data.Endian();
	}
	__SaveData(&data, 4u);
	if (TESForm::QEndianSwapOnSave()) {
		data.Endian();
	}
	int32_t iRank = 0;
	for (BSSimpleList<RANK_DATA*>* pRankList = &m_rankDataList; pRankList && !pRankList->IsEmpty();
		pRankList = pRankList->m_pkNext) {
		RANK_DATA* pdata = pRankList->m_item;
		TESForm::AddChunk(RNAM_ID, static_cast<uint32_t>(iRank));
		if (pdata->cMaleRankTitle.GetLength()) {
			uint32_t Length = pdata->cMaleRankTitle.GetLength();
			TESForm::AddChunkArray(MNAM_ID, pdata->cMaleRankTitle.pString, Length + 1);
		}
		if (pdata->cFemaleRankTitle.GetLength()) {
			uint32_t v2 = pdata->cFemaleRankTitle.GetLength();
			TESForm::AddChunkArray(FNAM_ID, pdata->cFemaleRankTitle.pString, v2 + 1);
		}
		if (pdata->TextureInsignia.GetTextureNameLength()) {
			TESForm::AddChunkArray(INAM_ID, pdata->TextureInsignia.GetTextureName(),
				pdata->TextureInsignia.GetTextureNameLength() + 1);
		}
		++iRank;
	}
	if (pReputation) {
		TESForm::AddChunk(WMI1_ID, pReputation->iFormID);
	}
	CloseForm();
}

uint16_t TESFaction::GetSaveSize(uint32_t aiFlags) {
	uint16_t saveSize = TESForm::GetSaveSize(aiFlags);
	uint16_t v5 = (uint16_t)(static_cast<TESReactionForm*>(this)->GetSaveSize(aiFlags) + saveSize);
	if ((aiFlags & 2) != 0) {
		v5 += 4;
	}
	return v5;
}

// Saves the form's changes into the save buffer: the base form's and the reactions', then the faction flags and the
// two crime counts when they changed.
void TESFaction::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	TESForm::SaveGame(apSaveGameBuffer);
	TESReactionForm::SaveGame(apSaveGameBuffer);
	if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_FACTION_FLAGS)) {
		apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&data), 4, 0);
	}
	if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_FACTION_CRIME_COUNTS)) {
		apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iMajorCrime), 4, 0);
		apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iMinorCrime), 4, 0);
	}
}

void TESFaction::SaveGame(uint32_t aiFlags) {
	TESForm::SaveGame(aiFlags);
	SaveGameTES(aiFlags);
	if ((aiFlags & 2) != 0) {
		SaveGameDataOLD(&data, 4);
	}
}

// Loads the same changes back from the load buffer.
void TESFaction::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESForm::LoadGame(apLoadGameBuffer);
	TESReactionForm::LoadGame(apLoadGameBuffer);
	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_FACTION_FLAGS)) {
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&data), 4);
	}
	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_FACTION_CRIME_COUNTS)) {
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iMajorCrime), 4);
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iMinorCrime), 4);
	}
}

void TESFaction::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	TESForm::LoadGame(aiFlags, aiCurrentFlags);
	LoadGameTES(aiFlags, aiCurrentFlags);
	if ((aiFlags & 2) != 0) {
		LoadGameDataOLD(&data, 4);
	}
}

void TESFaction::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESReactionForm::InitLoadGame(apLoadGameBuffer);
}

void TESFaction::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
	TESForm::InitLoadGame(aiFlags, aiOldFlags);
	TESReactionForm::InitLoadGame(aiFlags, aiOldFlags);
}

void TESFaction::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
	if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_FACTION_CRIME_COUNTS)) {
		iMajorCrime = 0;
		iMinorCrime = 0;
	}
}

void TESFaction::InitItem() {
	if (!GetInitialized()) {
		TESReactionForm::InitItem(this);
		SetInitialized(1);
		if (pReputation) {
			uint32_t itempint = 0;
			itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pReputation));
			TESFile* File = GetFile(-1);
			TESForm::AddCompileIndex(itempint, File);
			pReputation = pDataHandler->GetReputation(itempint);
		}
	}
}

// Copies apCopy's components, flags, reputation and ranks. The exe's cleanup-only C++ EH frame (over the rank
// allocation) is left out.
void TESFaction::Copy(TESForm* apCopy) {
	TESFaction* pFaction = dynamic_cast<TESFaction*>(apCopy);
	if (!pFaction)
		return;
	CopyAllComponents(apCopy);
	ourMemcpy(&data, &pFaction->data, sizeof(FACTION_DATA));
	pReputation = pFaction->pReputation;
	ClearRankDataList();
	BSSimpleList<RANK_DATA*>* pRankList = QRankDataList();
	for (BSSimpleList<RANK_DATA*>* pNode = pFaction->QRankDataList(); pNode && !pNode->IsEmpty();
		pNode = pNode->GetNext()) {
		void* pMemory = ourMalloc(sizeof(RANK_DATA));
		RANK_DATA* pRank = pMemory ? ::new (pMemory) RANK_DATA : nullptr;
		pRank->cMaleRankTitle.Set(pNode->GetItem()->cMaleRankTitle.pString, 0);
		pRank->cFemaleRankTitle.Set(pNode->GetItem()->cFemaleRankTitle.pString, 0);
		// The exe copies the insignia through CopyComponent (slot 2) of its vtable; a call on the member itself would
		// be devirtualised.
		BaseFormComponent* pInsignia = &pRank->TextureInsignia;
		pInsignia->CopyComponent(&pNode->GetItem()->TextureInsignia);
		pRankList->AddTail(pRank);
		if (pRankList->GetNext())
			pRankList = pRankList->GetNext();
	}
}

bool TESFaction::Compare(TESForm* apCompare) {
	TESFaction* pComp = dynamic_cast<TESFaction*>(apCompare);
	if (!pComp) {
		return 1;
	}
	if (CompareAllComponents(pComp)) {
		return 1;
	}
	void* LocalTransform = &pComp->data;
	if (std::memcmp(&data, LocalTransform, sizeof(data))) {
		return 1;
	}
	if (pReputation != pComp->pReputation) {
		return 1;
	}
	BSSimpleList<RANK_DATA*>* pRankList = &m_rankDataList;
	BSSimpleList<RANK_DATA*>* pCompRankList = &pComp->m_rankDataList;
	while (pRankList && !pRankList->IsEmpty()) {
		if (!pCompRankList || !pCompRankList->m_item) {
			return 1;
		}
		RANK_DATA* pdata = pRankList->m_item;
		RANK_DATA* pcompdata = pCompRankList->m_item;
		if (pdata->cMaleRankTitle.GetLength() || pcompdata->cMaleRankTitle.GetLength()) {
			uint32_t Length = pdata->cMaleRankTitle.GetLength();
			if (Length != pcompdata->cMaleRankTitle.GetLength()) {
				return 1;
			}
			if (pdata->cMaleRankTitle.StrCmp(static_cast<const char*>(pcompdata->cMaleRankTitle.pString),
					static_cast<bool>(0))) {
				return 1;
			}
		}
		if (pdata->cFemaleRankTitle.GetLength() || pcompdata->cFemaleRankTitle.GetLength()) {
			uint32_t v5 = pdata->cFemaleRankTitle.GetLength();
			if (v5 != pcompdata->cFemaleRankTitle.GetLength()) {
				return 1;
			}
			if (pdata->cFemaleRankTitle.StrCmp(static_cast<const char*>(pcompdata->cFemaleRankTitle.pString),
					static_cast<bool>(0))) {
				return 1;
			}
		}
		if (pdata->TextureInsignia.CompareComponent(&pcompdata->TextureInsignia)) {
			return 1;
		}
		pRankList = pRankList->m_pkNext;
		pCompRankList = pCompRankList->m_pkNext;
	}
	return pCompRankList && !pCompRankList->IsEmpty();
}

bool TESFaction::GetIsEvil() {
	return ((data.iFlags & 2) != 0);
}

// Appends a new, empty rank data to the faction's rank list and returns its index: the count of the entries the walk
// passes before it. The exe's try/finally only frees the memory when the constructor throws, so it is left out.
int32_t TESFaction::AddRankData() {
	RANK_DATA* pRankData = nullptr;
	void* pMemory = ourMalloc(sizeof(RANK_DATA));
	if (pMemory) {
		pRankData = ::new (pMemory) RANK_DATA;
	}

	int32_t iRank = 0;
	BSSimpleList<RANK_DATA*>* pNode = QRankDataList();
	BSSimpleList<RANK_DATA*>* pLast = nullptr;
	while (pNode && !pNode->IsEmpty()) { // SWEEP-INLINE 0x8256D0
		++iRank;
		pLast = pNode;
		pNode = pNode->GetNext(); // SWEEP-INLINE 0x726070
	}

	if (pLast) {
		pLast->AddTail(pRankData); // SWEEP-INLINE 0x905820
	} else {
		QRankDataList()->AddHead(pRankData); // SWEEP-INLINE 0x5AE3D0
	}

	return iRank;
}

bool TESFaction::GetPCAnEnemy() {
	return ((data.iFlags & 16) != 0);
}

bool TESFaction::GetAllowSell() {
	return ((data.iFlags & 512) != 0);
}

int32_t TESFaction::GetFactionReaction(TESFaction* pFaction) {
	return TESReactionForm::GetReaction(pFaction);
}

// Change flag 4 is the faction's reactions; it has no name in the PDB or IDA.
void TESFaction::ModFactionReaction(TESFaction* pFaction, int32_t reaction) {
	TESReactionForm::ModReaction(pFaction, reaction);
	AddChange(4);
}

// Change flag 4 is the faction's reactions; it has no name in the PDB or IDA.
void TESFaction::SetFactionReaction(TESFaction* pFaction, int32_t reaction) {
	TESReactionForm::SetFightReaction(pFaction, reaction);
	AddChange(4);
}

bool TESFaction::GetPCExpelled() {
	return ((data.iFlags & 8) != 0);
}

bool TESFaction::GetPCHasMurdereredMember() {
	return ((data.iFlags & 64) != 0);
}

bool TESFaction::GetPCHasAttackedMember() {
	return ((data.iFlags & 32) != 0);
}

bool TESFaction::IsTrackCrime() {
	return ((data.iFlags & 256) != 0);
}

void TESFaction::SetPCMurder(bool abVal) {
	SetFlag(0x40u, abVal);
}

void TESFaction::SetPCAttack(bool abVal) {
	SetFlag(0x20u, abVal);
}

bool TESFaction::GetSpecialCombat() {
	return (((int32_t)(data.iFlags & 4)) != 0);
}

void TESFaction::SetPCExpelled(bool abVal) {
	SetFlag(8u, abVal);
}

void TESFaction::AddMinorCrime(int32_t a2, uint8_t a3) {
	iMinorCrime += a2;
	if (pReputation) {
		if (a3) {
			pReputation->RemoveReputationMinorOrMajorCrime(0);
		}
	}
	AddChange(0x80000000u);
}

void TESFaction::ClearRankDataList() {
	while (!(&m_rankDataList)->IsEmpty()) {
		RANK_DATA* rankData = (&m_rankDataList)->m_item;
		if (rankData) {
			delete rankData;
		}
		(&m_rankDataList)->RemoveHead();
	}
}

void TESFaction::SetFlag(uint32_t aiFlag, bool abVal) {
	if (abVal) {
		data.iFlags |= aiFlag;
		uint32_t iFlags = data.iFlags;
	} else {
		data.iFlags &= ~aiFlag;
		uint32_t v4 = data.iFlags;
	}
	uint32_t v5 = data.iFlags;
	AddChange(2);
}

void TESFaction::HandleMajorCrimeReputation(int32_t a2, char a3) {
	iMajorCrime += a2;
	if (pReputation) {
		if (a3) {
			pReputation->RemoveReputationMinorOrMajorCrime(1);
		}
	}
	AddChange(CHANGE_FACTION_CRIME_COUNTS);
}

void TESFaction::SetFactionReaction_0(TESFaction* pFaction, int32_t reaction) {
	SetReaction(pFaction, reaction);
	AddChange(4);
}
