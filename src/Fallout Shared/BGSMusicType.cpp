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
        uint32_t v1;
        if (GetSoundFileLength() > 2) {
            const char* SoundFile = GetSoundFile();
            if (SoundFile[(GetSoundFileLength() - 1)] == 92) {
                BuildFileList();
            }
            else {
                char apDest[268];
                const char* v4 = GetSoundFile();
                BSsprintf(apDest, 260, "%s\\%s", "Data\\Music", v4);
                SetSoundFile(apDest);
            }
        }
        if (pMusicFileList.IsEmpty()) {
            v1 = 0;
        }
        else {
            v1 = RandomInt(0, pMusicFileList.ItemsInList() - 1);
        }
        iLastSelection = v1;
        SetInitialized(true);
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
        return true;
    }
    if (v4->fAttenuation == fAttenuation) {
        return (((int8_t)TESForm::CompareAllComponents(v4)) != 0);
    }
    return true;
}

bool BGSMusicType::GetLooping() {
    return (bool)((uint8_t)(fAttenuation > 0.0));
}

float BGSMusicType::GetAttenuation() {
    if (!(fAttenuation > 0.0)) {
        return fAttenuation;
    }

    return (-fAttenuation);
}

void BGSMusicType::BuildFileList() {
    char apDest[264];
    char apString1[264];
    BSSystemDir FindDir;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_AUDIO, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\BGSMusicType.cpp", 352
    );
    const char* SoundFile = GetSoundFile();
    BSsprintf(apDest, 260, "%s\\%s", "Data\\Music", SoundFile);
    const char* TestExt = ".mp3";
    bool bDone = false;
    FindDir.~BSSystemDir();
    ::new (&FindDir) BSSystemDir(apDest);
    for (BSSystemFile::ErrorCode Err = FindDir.NextEntry(bDone); !bDone && Err == BSSystemFile::EC_NONE;
         Err = FindDir.NextEntry(bDone)) {
        if (!static_cast<uint8_t>(FindDir.IsDirectory())) {
            // This address (&FindDir.FindData_.cFileName) is never null.
            char apExt[264];
            BSsplitpath(FindDir.FindData_.cFileName, nullptr, 0, nullptr, 0, nullptr, 0, apExt, 260);
            if (!BSstricmp(apExt, TestExt)) {
                BGSMusicType::MusicFileEntry* v2;
                BSsprintf(apString1, 260, "%s%s", apDest, &FindDir.FindData_.cFileName);
                BGSMusicType::MusicFileEntry* v4 =
                    static_cast<BGSMusicType::MusicFileEntry*>(ourMalloc(sizeof(BGSMusicType::MusicFileEntry)));
                if (v4) {
                    v2 = ::new (v4) BGSMusicType::MusicFileEntry(apString1);
                }
                else {
                    v2 = nullptr;
                }
                char* a2 = v2->cFilename;
                pMusicFileList.AddHead(a2);
            }
        }
    }
}

// GetRandomEntry returns char* while TESSoundFile::GetSoundFile returns const char*, so the const is cast away.
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
    }
    else {
        if (iLastSelection || iLastSelection != iwhich) {
            iLastSelection = iwhich;
        }
        else {
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
    }

    return const_cast<char*>(GetSoundFile());
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
        return true;
    }
    if (!GetSoundFileLength() && !pcompare->GetSoundFileLength()) {
        return false;
    }
    uint32_t SoundFileLength = GetSoundFileLength();
    if (SoundFileLength != pcompare->GetSoundFileLength()) {
        return true;
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

const char* TESSoundFile::GetSoundFile() {
    if (cSoundFile.pString) {
        return cSoundFile.pString;
    }

    return "";
}

uint32_t TESSoundFile::GetSoundFileLength() {
    return cSoundFile.GetLength();
}
