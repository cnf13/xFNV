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
#include "xfnv/Compiler.h"

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
        char* pFileName = pMusicFileList.m_item;
        pMusicFileList.RemoveHead();
        ourFree(pFileName);
    }
}

// Reads the music type's chunks. FNAM holds the sound file name and ANAM the attenuation, which is read as a 32-bit
// integer and reinterpreted as a float.
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
                // ANAM is read as an untyped 32-bit value and then the same four bytes are used as a float.
                float fValue = 0.0f;
                apFile->GetChunkData<uint32_t>(reinterpret_cast<uint32_t&>(fValue));
                fAttenuation = fValue;
                break;
            }
            case EDID_ID: {
                // A 16-byte aligned buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200u);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case FNAM_ID: {
                // A 16-byte aligned buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0);
                static_cast<TESSoundFile*>(this)->SetSoundFile(pBuffer);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
        }
    }
    while (apFile->NextChunk());
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
        if (GetSoundFileLength() > 2) {
            const char* pSoundFile = GetSoundFile();
            if (pSoundFile[GetSoundFileLength() - 1] == '\\') {
                BuildFileList();
            }
            else {
                char cPath[268];
                BSsprintf(cPath, 260, "%s\\%s", "Data\\Music", GetSoundFile());
                SetSoundFile(cPath);
            }
        }
        uint32_t uiSelection;
        if (pMusicFileList.IsEmpty()) {
            uiSelection = 0;
        }
        else {
            uiSelection = RandomInt(0, pMusicFileList.ItemsInList() - 1);
        }
        iLastSelection = uiSelection;
        SetInitialized(true);
    }
}

void BGSMusicType::Copy(TESForm* apCopy) {
    BGSMusicType* pCopy = dynamic_cast<BGSMusicType*>(apCopy);
    if (pCopy) {
        fAttenuation = pCopy->fAttenuation;
        TESForm::CopyAllComponents(pCopy);
    }
}

bool BGSMusicType::Compare(TESForm* apCompare) {
    BGSMusicType* pCompare = dynamic_cast<BGSMusicType*>(apCompare);
    if (!pCompare) {
        return true;
    }
    if (pCompare->fAttenuation == fAttenuation) {
        return TESForm::CompareAllComponents(pCompare);
    }
    return true;
}

bool BGSMusicType::GetLooping() {
    return fAttenuation > 0.0;
}

float BGSMusicType::GetAttenuation() {
    if (fAttenuation > 0.0) {
        return -fAttenuation;
    }

    return fAttenuation;
}

void BGSMusicType::BuildFileList() {
    char cPath[264];
    char cFilePath[264];
    AutoMemContext tempMemContext(
        MEM_CONTEXT_AUDIO,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BGSMusicType.cpp",
        352
    );
    BSsprintf(cPath, 260, "%s\\%s", "Data\\Music", GetSoundFile());
    const char* TestExt = ".mp3";
    bool bDone = false;
    BSSystemDir FindDir(cPath);
    for (BSSystemFile::ErrorCode Err = FindDir.NextEntry(bDone); !bDone && Err == BSSystemFile::EC_NONE;
         Err = FindDir.NextEntry(bDone)) {
        if (!static_cast<uint8_t>(FindDir.IsDirectory())) {
            // This address (&FindDir.FindData_.cFileName) is never null.
            char cExt[264];
            BSsplitpath(FindDir.FindData_.cFileName, nullptr, 0, nullptr, 0, nullptr, 0, cExt, 260);
            if (!BSstricmp(cExt, TestExt)) {
                BSsprintf(cFilePath, 260, "%s%s", cPath, &FindDir.FindData_.cFileName);
                BGSMusicType::MusicFileEntry* pEntry = new BGSMusicType::MusicFileEntry(cFilePath);
                char* pFilename = pEntry->cFilename;
                pMusicFileList.AddHead(pFilename);
            }
        }
    }
}

// GetRandomEntry returns char* while TESSoundFile::GetSoundFile returns const char*, so the const is cast away.
char* BGSMusicType::GetRandomEntry() {
    uint32_t uiItemCount = pMusicFileList.ItemsInList();
    if (pMusicFileList.IsEmpty() && !uiItemCount) {
        return const_cast<char*>(GetSoundFile());
    }
    if (uiItemCount == 1) {
        return pMusicFileList.m_item;
    }
    BSSimpleList<char*>* pEntry = &pMusicFileList;
    uint32_t uiWhich = RandomInt(0, pMusicFileList.ItemsInList());
    if (iLastSelection && iLastSelection == uiWhich) {
        uiWhich = 0;
        iLastSelection = 0;
    }
    else if (iLastSelection || iLastSelection != uiWhich) {
        iLastSelection = uiWhich;
    }
    else {
        uiWhich = 1;
        iLastSelection = 1;
    }
    while (pEntry) {
        if (!uiWhich--) {
            break;
        }
        pEntry = pEntry->m_pkNext;
    }
    if (pEntry && pEntry->m_item) {
        return pEntry->m_item;
    }

    return const_cast<char*>(GetSoundFile());
}

TESSoundFile::TESSoundFile() : BaseFormComponent(), cSoundFile() {}

TESSoundFile::~TESSoundFile() {}

void TESSoundFile::InitializeDataComponent() {
    cSoundFile.Set(nullptr, 0);
}

void TESSoundFile::CopyComponent(BaseFormComponent* apCopyFC) {
    TESSoundFile* pCopy = dynamic_cast<TESSoundFile*>(apCopyFC);
    if (pCopy) {
        SetSoundFile(pCopy->GetSoundFile());
    }
}

bool TESSoundFile::CompareComponent(BaseFormComponent* apCompareFC) {
    TESSoundFile* pCompare = dynamic_cast<TESSoundFile*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    if (!GetSoundFileLength() && !pCompare->GetSoundFileLength()) {
        return false;
    }
    uint32_t uiSoundFileLength = GetSoundFileLength();
    if (uiSoundFileLength != pCompare->GetSoundFileLength()) {
        return true;
    }
    return BSstricmp(GetSoundFile(), pCompare->GetSoundFile()) != 0;
}

void TESSoundFile::SetSoundFile(const char* apSound) {
    cSoundFile.Set(apSound, 0);
}

void TESSoundFile::Save() {
    if (GetSoundFileLength()) {
        TESForm::AddChunkArray(FNAM_ID, GetSoundFile(), GetSoundFileLength() + 1);
    }
}

const char* TESSoundFile::GetSoundFile() {
    if (cSoundFile.pString) {
        return cSoundFile.pString;
    }

    return "";
}

uint32_t TESSoundFile::GetSoundFileLength() {
    return cSoundFile.GetLength();
}
