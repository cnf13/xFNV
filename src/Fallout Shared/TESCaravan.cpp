#include "Fallout Shared/TESCaravan.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

void CARAVANDECKDATA::Endian() {
	ENDIAN(uiValue, false);
}

TESCaravanCard::TESCaravanCard() {
	SetFormType(CCRD_ID);
	TESCaravanCard::InitializeData();
}

TESCaravanCard::~TESCaravanCard() {
	TESCaravanCard::ClearData();
}

void TESCaravanCard::InitializeData() {
	(&faceTexture)->InitializeDataComponent();
	(&backTexture)->InitializeDataComponent();
	ourMemset(&data, 0, sizeof(data));
}

void TESCaravanCard::ClearData() {
	(&faceTexture)->ClearDataComponent();
	(&backTexture)->ClearDataComponent();
	ourMemset(&data, 0, sizeof(data));
}

bool TESCaravanCard::Load(TESFile* apFile) {
	uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
	bool bReadValue = false;
	if (cFormType != CCRD_ID) {
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
			static_cast<TESBoundObject*>(this)->LoadObjectBound(apFile);
			break;
		case FULL_ID:
			TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile);
			break;
		case MODL_ID:
		case MODD_ID:
		case MOSD_ID:
		case MODT_ID:
			TESModel::LoadModelChunk(static_cast<TESModelTextureSwap*>(this), apFile);
			break;
		case MODS_ID: {
			uint32_t uiChunkSize = apFile->GetChunkSize();
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
			apFile->GetChunkData(pBuffer, uiChunkSize);
			uint16_t usFormVersion = apFile->GetFormVersion();
			static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiChunkSize, usFormVersion);
			break;
		}
		case ICON_ID:
			TESTexture::LoadTextureChunk(static_cast<TESIcon*>(this), apFile);
			break;
		case MICO_ID:
			BGSMessageIcon::LoadMessageIcon(static_cast<BGSMessageIcon*>(this), apFile);
			break;
		case SCRI_ID: {
			uint32_t uiScriptID = 0;
			apFile->GetChunkData<uint32_t>(uiScriptID);
			TESScriptableForm* pScriptableForm = static_cast<TESScriptableForm*>(this);
			// The chunk holds a form ID that stands in for the script until the item is initialised.
			pScriptableForm->SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiScriptID)));
			pScriptableForm->InitItem(this);
			break;
		}
		case YNAM_ID: {
			uint32_t uiSoundID = 0;
			apFile->GetChunkData<uint32_t>(uiSoundID);
			// The chunk holds a form ID that stands in for the sound until the item is initialised.
			static_cast<BGSPickupPutdownSounds*>(this)
				->SetPickupSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID)));
			break;
		}
		case ZNAM_ID: {
			uint32_t uiSoundID = 0;
			apFile->GetChunkData<uint32_t>(uiSoundID);
			// The chunk holds a form ID that stands in for the sound until the item is initialised.
			static_cast<BGSPickupPutdownSounds*>(this)
				->SetPutdownSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID)));
			break;
		}
		case INTV_ID:
			// The first INTV chunk of a card fills the suit, the second the face value.
			if (!bReadValue) {
				apFile->GetChunkData<uint32_t>(data.iCardSuit);
				bReadValue = true;
			} else {
				apFile->GetChunkData<int32_t>(data.iFaceValue);
			}
			break;
		case TX00_ID:
			TESTexture::LoadTextureChunk(&faceTexture, apFile);
			break;
		case TX01_ID:
			TESTexture::LoadTextureChunk(&backTexture, apFile);
			break;
		case DATA_ID:
			LoadData(apFile, nullptr, 0);
			break;
		}
	} while (apFile->NextChunk());
	return true;
}

void TESCaravanCard::Save() {
	StartForm();
	TESFullName::Save();
	TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
	TESTexture::Save(ICON_ID);
	BGSMessageIcon::Save(MICO_ID);
	TESScriptableForm::Save();
	BGSPickupPutdownSounds::Save();
	TESTexture* Children = &faceTexture;
	Children->Save(TX00_ID);
	TESTexture* EyeColorList = &backTexture;
	EyeColorList->Save(TX01_ID);
	TESForm::AddChunk(INTV_ID, data.iCardSuit);
	TESForm::AddChunk(INTV_ID, data.iFaceValue);
	SaveData();
	CloseForm();
}

uint16_t TESCaravanCard::GetSaveSize(uint32_t aiFlags) {
	return TESForm::GetSaveSize(aiFlags);
}

void TESCaravanCard::SaveGame(uint32_t aiFlags) {
	TESForm::SaveGame(aiFlags);
}

void TESCaravanCard::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	TESForm::SaveGame(apSaveGameBuffer);
}

void TESCaravanCard::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	TESForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESCaravanCard::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESForm::LoadGame(apLoadGameBuffer);
}

void TESCaravanCard::InitItem() {
	if (!TESForm::GetInitialized()) {
		static_cast<TESModelTextureSwap*>(this)->InitItem(this);
		static_cast<TESScriptableForm*>(this)->InitItem(this);
		static_cast<BGSPickupPutdownSounds*>(this)->InitItem(this);
		TESForm::SetInitialized(1);
	}
}

void TESCaravanCard::Copy(TESForm* apCopy) {
	TESCaravanCard* pCopy = dynamic_cast<TESCaravanCard*>(apCopy);
	if (pCopy) {
		CopyAllComponents(pCopy);
		faceTexture.SetTextureName((&pCopy->faceTexture)->GetTextureName());
		backTexture.SetTextureName((&pCopy->backTexture)->GetTextureName());
		CARAVANCARDDATA* MuzzleFlashModel = &pCopy->data;
		uint32_t iFaceValue = MuzzleFlashModel->iFaceValue;
		uint32_t iCardSuit = MuzzleFlashModel->iCardSuit;
		data.iFaceValue = iFaceValue;
		data.iCardSuit = iCardSuit;
		cFormType = apCopy->GetFormType();
	}
}

bool TESCaravanCard::Compare(TESForm* apCompare) {
	TESCaravanCard* pComp = dynamic_cast<TESCaravanCard*>(apCompare);
	if (!pComp) {
		return 1;
	}
	if (CompareAllComponents(pComp)) {
		return 1;
	}
	if (BSstrcmp(faceTexture.GetTextureName(), (&pComp->faceTexture)->GetTextureName())) {
		return 1;
	}
	if (BSstrcmp(backTexture.GetTextureName(), (&pComp->backTexture)->GetTextureName())) {
		return 1;
	}
	return std::memcmp(&data, &pComp->data, sizeof(data)) != 0;
}

NiAVObject* TESCaravanCard::Clone3D(TESObjectREFR* pRequester, bool deepCopy) {
	return TESBoundObject::Clone3D(pRequester, deepCopy);
}

// qsort comparator over TESCaravanCard* entries: orders the cards by face value (signed compare).
int32_t TESCaravanCard::SortCardsFunc(const void* t1, const void* t2) {
	int32_t v4 = (*static_cast<TESCaravanCard* const*>(t1))->GetData()->iFaceValue;
	int32_t v3 = (*static_cast<TESCaravanCard* const*>(t2))->GetData()->iFaceValue;
	if (v4 > v3) {
		return 1;
	}
	if (v4 >= v3) {
		return 0;
	}
	return -1;
}

TESCaravanMoney::TESCaravanMoney() {
	SetFormType(CMNY_ID);
	TESCaravanMoney::InitializeData();
}

TESCaravanMoney::~TESCaravanMoney() {
	TESCaravanMoney::ClearData();
}

void TESCaravanMoney::InitializeData() {
	(&anteModels[0])->InitializeDataComponent();
	(&anteModels[1])->InitializeDataComponent();
}

void TESCaravanMoney::ClearData() {
	anteModels->ClearDataComponent();
	(&anteModels[1])->ClearDataComponent();
}

bool TESCaravanMoney::Load(TESFile* apFile) {
	uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
	if (cFormType != CMNY_ID) {
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
			static_cast<TESBoundObject*>(this)->LoadObjectBound(apFile);
			break;
		case FULL_ID:
			TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile);
			break;
		case MODL_ID:
		case MODD_ID:
		case MOSD_ID:
		case MODT_ID:
			TESModel::LoadModelChunk(static_cast<TESModelTextureSwap*>(this), apFile);
			break;
		case MODS_ID: {
			uint32_t uiChunkSize = apFile->GetChunkSize();
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
			apFile->GetChunkData(pBuffer, uiChunkSize);
			uint16_t usFormVersion = apFile->GetFormVersion();
			static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiChunkSize, usFormVersion);
			break;
		}
		case MOD2_ID: {
			uint32_t uiChunkSize = apFile->GetChunkSize();
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
			apFile->GetChunkData(pBuffer, 0);
			static_cast<TESModel*>(&anteModels[0])->SetModel(pBuffer);
			TESModel::LoadModelTextureChunk(&anteModels[0], apFile);
			break;
		}
		case MOD3_ID: {
			uint32_t uiChunkSize = apFile->GetChunkSize();
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
			apFile->GetChunkData(pBuffer, 0);
			static_cast<TESModel*>(&anteModels[1])->SetModel(pBuffer);
			TESModel::LoadModelTextureChunk(&anteModels[1], apFile);
			break;
		}
		case ICON_ID:
			TESTexture::LoadTextureChunk(static_cast<TESIcon*>(this), apFile);
			break;
		case MICO_ID:
			BGSMessageIcon::LoadMessageIcon(static_cast<BGSMessageIcon*>(this), apFile);
			break;
		case YNAM_ID: {
			uint32_t uiSoundID = 0;
			apFile->GetChunkData<uint32_t>(uiSoundID);
			// The chunk holds a form ID that stands in for the sound until the item is initialised.
			static_cast<BGSPickupPutdownSounds*>(this)
				->SetPickupSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID)));
			break;
		}
		case ZNAM_ID: {
			uint32_t uiSoundID = 0;
			apFile->GetChunkData<uint32_t>(uiSoundID);
			// The chunk holds a form ID that stands in for the sound until the item is initialised.
			static_cast<BGSPickupPutdownSounds*>(this)
				->SetPutdownSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID)));
			break;
		}
		case DATA_ID:
			LoadData(apFile, nullptr, 0);
			break;
		}
	} while (apFile->NextChunk());
	return true;
}

// IDA name SaveAlt; the vtable's Save slot. The components, then the form data, then the two ante models.
void TESCaravanMoney::Save() {
	StartForm();
	TESFullName::Save();
	TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
	TESIcon::Save(ICON_ID);
	BGSMessageIcon::Save(MICO_ID);
	BGSPickupPutdownSounds::Save();
	SaveData();
	GetAnteModel(0)->Save(MOD2_ID, MODT_ID, MODS_ID);
	GetAnteModel(1)->Save(MOD3_ID, MODT_ID, MODS_ID);
	CloseForm();
}

uint16_t TESCaravanMoney::GetSaveSize(uint32_t aiFlags) {
	uint16_t saveSize = TESForm::GetSaveSize(aiFlags);
	return (uint16_t)(static_cast<TESValueForm*>(this)->TESValueForm::GetSaveSize(aiFlags) + saveSize);
}

void TESCaravanMoney::SaveGame(uint32_t aiFlags) {
	TESForm::SaveGame(aiFlags);
	TESValueForm::SaveGame(aiFlags);
}

void TESCaravanMoney::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	TESForm::SaveGame(apSaveGameBuffer);
	static_cast<TESValueForm*>(this)->SaveGame(apSaveGameBuffer);
}

void TESCaravanMoney::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	TESForm::LoadGame(aiFlags, aiCurrentFlags);
	static_cast<TESValueForm*>(this)->TESValueForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESCaravanMoney::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESForm::LoadGame(apLoadGameBuffer);
	static_cast<TESValueForm*>(this)->LoadGame(apLoadGameBuffer);
}

void TESCaravanMoney::InitItem() {
	if (!TESForm::GetInitialized()) {
		static_cast<TESModelTextureSwap*>(this)->InitItem(this);
		static_cast<BGSPickupPutdownSounds*>(this)->InitItem(this);
		TESForm::SetInitialized(1);
	}
}

void TESCaravanMoney::Copy(TESForm* apCopy) {
	const char* v2;
	const char* v3;
	uint8_t formType;
	TESModelTextureSwap* v5;
	TESModelTextureSwap* anteModel;
	TESCaravanMoney* v8 = dynamic_cast<TESCaravanMoney*>(apCopy);
	if (v8) {
		TESForm::CopyAllComponents(v8);
		anteModel = v8->GetAnteModel(0);
		v2 = anteModel->GetModel();
		(&anteModels[0])->SetModel(v2);
		v5 = v8->GetAnteModel(1);
		v3 = v5->GetModel();
		(&anteModels[1])->SetModel(v3);
		formType = apCopy->GetFormType();
		TESForm::SetFormType(formType);
	}
}

bool TESCaravanMoney::Compare(TESForm* apCompare) {
	TESCaravanMoney* v10 = dynamic_cast<TESCaravanMoney*>(apCompare);
	if (!v10) {
		return (bool)1;
	}
	if (TESForm::CompareAllComponents(v10)) {
		return (bool)1;
	}
	TESModelTextureSwap* anteModel = v10->GetAnteModel(0);
	const char* v5 = anteModel->GetModel();
	const char* v3 = anteModels->GetModel();
	if (BSstrcmp(v3, v5)) {
		return (bool)1;
	}
	TESModelTextureSwap* v7 = v10->GetAnteModel(1);
	const char* v6 = v7->GetModel();
	const char* v4 = (&anteModels[1])->GetModel();
	return (BSstrcmp(v4, v6) != 0);
}

NiAVObject* TESCaravanMoney::Clone3D(TESObjectREFR* pRequester, bool deepCopy) {
	return TESBoundObject::Clone3D(pRequester, deepCopy);
}

TESModelTextureSwap* TESCaravanMoney::GetAnteModel(int32_t index) {
	return &anteModels[index];
}

TESCaravanDeck::TESCaravanDeck() {
	SetFormType(CDCK_ID);
	TESCaravanDeck::InitializeData();
}

TESCaravanDeck::~TESCaravanDeck() {
	TESCaravanDeck::ClearData();
}

// The exe's cleanup-only C++ EH frame is left out.
void TESCaravanDeck::InitializeData() {
	BSSimpleList<TESCaravanCard*>* v1;
	BSSimpleList<TESCaravanCard*>* v3 =
		static_cast<BSSimpleList<TESCaravanCard*>*>(ourMalloc(sizeof(BSSimpleList<TESCaravanCard*>)));
	if (v3) {
		v1 = ::new (v3) BSSimpleList<TESCaravanCard*>;
	} else {
		v1 = 0;
	}
	pDeck = v1;
	data.uiValue = 0;
}

void TESCaravanDeck::ClearData() {
	pDeck->RemoveAll();
	ourMemset(&data, 0, sizeof(data));
}

// An empty EDID chunk leaves the alloca'd buffer unwritten (malformed data), so src reserves one byte and terminates
// it. The strict check reports a DIFF on that buffer only: the exe's sits at a different place in its frame, so the
// bytes GetChunkData (0x472890) is handed differ (fix queue).
bool TESCaravanDeck::Load(TESFile* apFile) {
	uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
	if (cFormType != CDCK_ID) {
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
			uint32_t uiChunkSize = apFile->GetChunkSize();
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
			// Not in the exe: an empty chunk leaves the buffer unwritten.
			pBuffer[0] = '\0';
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
			// LoadData takes the chunk's raw bytes, which here are the deck's data dword.
			LoadData(apFile, reinterpret_cast<char*>(&data), 4);
			break;
		case CARD_ID: {
			uint32_t uiFormID = 0;
			apFile->GetChunkData<uint32_t>(uiFormID);
			TESForm::AddCompileIndex(uiFormID, apFile);
			TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
			// The exe adds the form as a card without checking its type.
			TESCaravanCard* pItem = static_cast<TESCaravanCard*>(pForm);
			if (!pDeck || pDeck->IsEmpty()) {
				pDeck->SetItem(pItem);
			} else {
				pDeck->AddHead(pItem);
			}
			break;
		}
		}
		if (uiChunk == DATA_ID && apFile->QEndian()) {
			data.Endian();
		}
	} while (apFile->NextChunk());
	return true;
}

void TESCaravanDeck::Save() {
	StartForm();
	TESFullName::Save();
	for (TESCaravanCard*& rpItem : this->pDeck->ItemsUntilNull()) {
		TESForm::AddChunk(CARD_ID, rpItem->iFormID);
	}
	if (TESForm::QEndianSwapOnSave()) {
		data.Endian();
	}
	__SaveData(&data, 4u);
	if (TESForm::QEndianSwapOnSave()) {
		data.Endian();
	}
	CloseForm();
}

void TESCaravanDeck::Copy(TESForm* apCopy) {
	TESCaravanDeck* pCopy = dynamic_cast<TESCaravanDeck*>(apCopy);
	if (pCopy) {
		CopyAllComponents(pCopy);
		while (pDeck && !pDeck->IsEmpty()) {
			pDeck->RemoveHead();
		}
		for (TESCaravanCard* cpyCard : pCopy->pDeck->ItemsUntilNull()) {
			if (pDeck && !pDeck->IsEmpty()) {
				pDeck->AddHead(cpyCard);
			} else {
				pDeck->SetItem(cpyCard);
			}
		}
		data.uiValue = (&pCopy->data)->uiValue;
		cFormType = pCopy->GetFormType();
	}
}

bool TESCaravanDeck::Compare(TESForm* apCompare) {
	TESCaravanDeck* pComp = dynamic_cast<TESCaravanDeck*>(apCompare);
	if (!pComp) {
		return 1;
	}
	if (CompareAllComponents(pComp)) {
		return 1;
	}
	int32_t v3 = pDeck->ItemsInList();
	if (v3 != pComp->pDeck->ItemsInList()) {
		return 1;
	}
	for (BSSimpleList<TESCaravanCard*>* cardList = pComp->pDeck; cardList && cardList->m_item;
		cardList = cardList->m_pkNext) {
		if (!pDeck->IsInList(cardList->m_item)) {
			return 1;
		}
	}
	void* Extra = &pComp->data;
	return std::memcmp(&data, Extra, sizeof(data)) != 0;
}
