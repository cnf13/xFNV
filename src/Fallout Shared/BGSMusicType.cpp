#include "Fallout Shared/BGSMusicType.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSSystem/BSSystemDir.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESSoundFile.h"
#include "SDK/MSVC/rtti.h"

BGSMusicType::MusicFileEntry::MusicFileEntry(char* apFilename) {
	ourStrcpy(cFilename, apFilename);
}

BGSMusicType::BGSMusicType() : fAttenuation(-6.0f) {
	SetFormType(102u);
}

BGSMusicType::~BGSMusicType() {
	BGSMusicType::ClearData();
}

void BGSMusicType::ClearData() {
	while (!pMusicFileList.IsEmpty()) {
		void* v2 = pMusicFileList.m_item;
		pMusicFileList.RemoveHead();
		ourFree(v2);
	}
}

// Reads the music type's chunks. FNAM holds the sound file name and ANAM the attenuation, which the exe reads as a
// dword and reinterprets as a float.
bool BGSMusicType::Load(TESFile* apFile) {
	this->LoadForm(apFile);
	this->SetInitialized(false);
	do {
		uint32_t eChunk = apFile->GetTESChunk();
		if (eChunk == NO_CHUNK) {
			break;
		}
		switch (eChunk) {
		case ANAM_ID: {
			// The exe reads ANAM as an untyped dword and then loads the same four bytes as a float.
			float fValue = 0.0f;
			apFile->GetChunkData<uint32_t>(reinterpret_cast<uint32_t&>(fValue));
			fAttenuation = fValue;
			break;
		}
		case EDID_ID: {
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0x200u);
			static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
			break;
		}
		case FNAM_ID: {
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0);
			static_cast<TESSoundFile*>(this)->SetSoundFile(pBuffer);
			break;
		}
		case OBND_ID:
			static_cast<TESForm*>(this)->LoadObjectBound(apFile);
			break;
		}
	} while (apFile->NextChunk());
	return true;
}

void BGSMusicType::Save() {
	TESForm::StartForm();
	static_cast<TESSoundFile*>(this)->Save();
	TESForm::AddChunk(ANAM_ID, fAttenuation);
	TESForm::CloseForm();
}

void BGSMusicType::InitItem() {
	if (!GetInitialized()) {
		uint32_t v1;
		if (GetSoundFileLength() > 2) {
			const char* SoundFile = GetSoundFile();
			if (SoundFile[(GetSoundFileLength() - 1)] == 92) {
				BuildFileList();
			} else {
				char apDest[268];
				const char* v4 = GetSoundFile();
				BSsprintf(apDest, 260, "%s\\%s", "Data\\Music", v4);
				SetSoundFile(apDest);
			}
		}
		if (pMusicFileList.IsEmpty()) {
			v1 = 0;
		} else {
			v1 = RandomInt(0, pMusicFileList.ItemsInList() - 1);
		}
		iLastSelection = v1;
		SetInitialized(1);
	}
}

void BGSMusicType::Copy(TESForm* apCopy) {
	BGSMusicType* v3 = dynamic_cast<BGSMusicType*>(apCopy);
	if (v3) {
		fAttenuation = v3->fAttenuation;
		TESForm::CopyAllComponents(v3);
	}
}

bool BGSMusicType::Compare(TESForm* apCompare) {
	BGSMusicType* v4 = dynamic_cast<BGSMusicType*>(apCompare);
	if (!v4) {
		return (bool)1;
	}
	if (v4->fAttenuation == fAttenuation) {
		return (((int8_t)TESForm::CompareAllComponents(v4)) != 0);
	}
	return (bool)1;
}

bool BGSMusicType::GetLooping() {
	return (bool)((uint8_t)(fAttenuation > 0.0));
}

float BGSMusicType::GetAttenuation() {
	if (!(fAttenuation > 0.0)) {
		return fAttenuation;
	} else {
		return (float)(-fAttenuation);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void BGSMusicType::BuildFileList() {
	char apDest[264];
	char apString1[264];
	BSSystemDir FindDir;
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, tempMemContext, &AutoMemContext::Destruct, MEM_CONTEXT_AUDIO, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BGSMusicType.cpp", 352);
	const char* SoundFile = GetSoundFile();
	BSsprintf(apDest, 260, "%s\\%s", "Data\\Music", SoundFile);
	const char* TestExt = ".mp3";
	bool bDone = 0;
	FindDir.~BSSystemDir();
	::new (&FindDir) BSSystemDir(apDest);
	for (BSSystemFile::ErrorCode Err = FindDir.NextEntry(bDone); !bDone && Err == BSSystemFile::EC_NONE;
		Err = FindDir.NextEntry(bDone)) {
		if (!static_cast<uint8_t>(FindDir.IsDirectory())) {
			// The exe tests this address (&FindDir.FindData_.cFileName), which is never null.
			char apExt[264];
			void* MBRoomAccumList = &FindDir.FindData_.cFileName;
			BSsplitpath(static_cast<char*>(MBRoomAccumList), 0, 0, 0, 0, 0, 0, apExt, 260);
			if (!BSstricmp(apExt, TestExt)) {
				BGSMusicType::MusicFileEntry* v2;
				BSsprintf(apString1, 260, "%s%s", apDest, &FindDir.FindData_.cFileName);
				BGSMusicType::MusicFileEntry* v4 =
					static_cast<BGSMusicType::MusicFileEntry*>(ourMalloc(sizeof(BGSMusicType::MusicFileEntry)));
				if (v4) {
					v2 = ::new (v4) BGSMusicType::MusicFileEntry(apString1);
				} else {
					v2 = 0;
				}
				char* a2 = v2->cFilename;
				pMusicFileList.AddHead(a2);
			}
		}
	}
}

// The PDB has GetRandomEntry return char* and TESSoundFile::GetSoundFile const char*: the source casts the const away.
char* BGSMusicType::GetRandomEntry() {
	uint32_t iitemCount = pMusicFileList.ItemsInList();
	if (pMusicFileList.IsEmpty() && !iitemCount) {
		return const_cast<char*>(GetSoundFile());
	}
	if (iitemCount == 1) {
		return (&pMusicFileList)->m_item;
	}
	BSSimpleList<char*>* pentry = &pMusicFileList;
	uint32_t iwhich = RandomInt(0, pMusicFileList.ItemsInList());
	if (iLastSelection && iLastSelection == iwhich) {
		iwhich = 0;
		iLastSelection = 0;
	} else {
		if (iLastSelection || iLastSelection != iwhich) {
			iLastSelection = iwhich;
		} else {
			iwhich = 1;
			iLastSelection = 1;
		}
	}
	while (pentry) {
		if (!iwhich--) {
			break;
		}
		pentry = pentry->m_pkNext;
	}
	if (pentry && pentry->m_item) {
		return pentry->m_item;
	} else {
		return const_cast<char*>(GetSoundFile());
	}
}

TESSoundFile::TESSoundFile() : BaseFormComponent(), cSoundFile() {}

TESSoundFile::~TESSoundFile() {}

void TESSoundFile::InitializeDataComponent() {
	cSoundFile.Set(nullptr, (uint32_t)0);
}

void TESSoundFile::CopyComponent(BaseFormComponent* apCopyFC) {
	TESSoundFile* pcopy = dynamic_cast<TESSoundFile*>(apCopyFC);
	if (pcopy) {
		SetSoundFile(pcopy->GetSoundFile());
	}
}

bool TESSoundFile::CompareComponent(BaseFormComponent* apCompareFC) {
	TESSoundFile* pcompare = dynamic_cast<TESSoundFile*>(apCompareFC);
	if (!pcompare) {
		return 1;
	}
	if (!GetSoundFileLength() && !pcompare->GetSoundFileLength()) {
		return 0;
	}
	uint32_t SoundFileLength = GetSoundFileLength();
	if (SoundFileLength != pcompare->GetSoundFileLength()) {
		return 1;
	}
	return BSstricmp(GetSoundFile(), pcompare->GetSoundFile()) != 0;
}

void TESSoundFile::SetSoundFile(const char* apSound) {
	cSoundFile.Set(apSound, (uint32_t)0);
}

void TESSoundFile::Save() {
	if (GetSoundFileLength()) {
		TESForm::AddChunkArray(FNAM_ID, GetSoundFile(), GetSoundFileLength() + 1);
	}
}

// The exe reads cSoundFile's pointer through the one-field getter 0x559450 (a trivial accessor), twice.
const char* TESSoundFile::GetSoundFile() {
	if (cSoundFile.pString) {
		return cSoundFile.pString;
	} else {
		return "";
	}
}

uint32_t TESSoundFile::GetSoundFileLength() {
	return cSoundFile.GetLength();
}
