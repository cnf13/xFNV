#include "Fallout Shared/BGSTextureSet.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BSShaderTextureSet.h"
#include "BSSystem/Archive.h"
#include "Fallout Shared/DecalData.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"

BGSTextureSet::BGSTextureSet() {
    BGSTextureSet::InitializeData();
    cFormType = 4;
}

BGSTextureSet::~BGSTextureSet() {
    BGSTextureSet::ClearData();
}

void BGSTextureSet::InitializeData() {
    int32_t i;
    for (i = 0; i < 6; ++i) {
        (&pTextures[i])->InitializeDataComponent();
    }
    pDecalData = nullptr;
    iFlags = (uint16_t)0;
    ourMemset(pTextureFileEntries, 0, sizeof(pTextureFileEntries));
}

void BGSTextureSet::ClearData() {
    int32_t i;
    for (i = 0; i < 6; ++i) {
        (&pTextures[i])->ClearDataComponent();
    }
    if (pDecalData) {
        delete pDecalData;
    }
    iFlags = (uint16_t)0;
}

// Reads the six texture chunks, the decal data, the flags, the editor ID and the object bound, then looks every texture
// name up in the archives: each becomes a "Data\Textures\"-prefixed path, standardized, hashed and searched. The two
// path buffers are 260 bytes. When the EDID chunk is empty (malformed data), its buffer is a single zeroed byte, so the
// editor ID reads as "".
bool BGSTextureSet::Load(TESFile* apFile) {
    LoadForm(apFile);
    SetInitialized(false);
    for (;;) {
        CHUNK_ID eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
        if (eChunk == NO_CHUNK) {
            break;
        }

        switch (eChunk) {
            case TX00_ID: TESTexture::LoadTextureChunk(&pTextures[0], apFile); break;
            case TX01_ID: TESTexture::LoadTextureChunk(&pTextures[1], apFile); break;
            case TX02_ID: TESTexture::LoadTextureChunk(&pTextures[2], apFile); break;
            case TX03_ID: TESTexture::LoadTextureChunk(&pTextures[3], apFile); break;
            case TX04_ID: TESTexture::LoadTextureChunk(&pTextures[4], apFile); break;
            case TX05_ID: TESTexture::LoadTextureChunk(&pTextures[5], apFile); break;
            case DNAM_ID:
                // LoadWord takes the flags as a signed short; the field is unsigned.
                apFile->LoadWord(reinterpret_cast<int16_t*>(&iFlags));
                break;
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pEditorID = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // An empty chunk leaves the buffer unwritten (see above).
                pEditorID[0] = '\0';
                apFile->GetChunkData(pEditorID, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pEditorID);
                break;
            }
            case OBND_ID: static_cast<TESBoundObject*>(this)->LoadObjectBound(apFile); break;
            case DODT_ID:
                if (!pDecalData) {
                    void* pMemory = ourMalloc(sizeof(DecalData));
                    pDecalData = pMemory ? ::new (pMemory) DecalData : nullptr;
                }
                pDecalData->Load(apFile);
                break;
        }

        if (!apFile->NextChunk()) {
            break;
        }
    }

    for (int32_t i = 0; i < 6; ++i) {
        const char* pTextureName = pTextures[i].GetTextureName();
        if (pTextureName && *pTextureName) {
            char aPath[260];
            char aStandardizedPath[260];
            BSstrcpy(aPath, 260, "Data\\Textures\\");
            BSstrcat(aPath, 260, pTextureName);
            StandardizeTexturePath(aPath, aStandardizedPath, 260);
            BSHash kDirHash;
            BSHash kFileHash;
            BSHash::MakeDirAndFile(aStandardizedPath, kDirHash, kFileHash);
            pTextureFileEntries[i] = ArchiveManager::GetFileEntryForFileFromAllArchives(
                ARCHIVE_TYPE_INDEX_TEXTURES, kDirHash, kFileHash, aStandardizedPath
            );
        }
    }
    return true;
}

void BGSTextureSet::Save() {
    StartForm();
    for (int32_t i = 0; i < 6; ++i) {
        pTextures[i].Save(aeTextureID[i]);
    }
    if (pDecalData) {
        pDecalData->Save(DODT_ID);
    }
    TESForm::AddChunk(DNAM_ID, iFlags);
    CloseForm();
}

void BGSTextureSet::Copy(TESForm* apCopy) {
    BGSTextureSet* pcopy = dynamic_cast<BGSTextureSet*>(apCopy);
    if (pcopy) {
        for (int32_t i = 0; i < 6; ++i) {
            pTextures[i].CopyComponent(&pcopy->pTextures[i]);
        }
        if (pcopy->pDecalData) {
            if (!pDecalData) {
                DecalData* v2;
                DecalData* apMemory = static_cast<DecalData*>(ourMalloc(sizeof(DecalData)));
                if (apMemory) {
                    v2 = ::new (apMemory) DecalData;
                }
                else {
                    v2 = 0;
                }
                pDecalData = v2;
            }
            pDecalData->Copy(pcopy->pDecalData);
        }
        else {
            if (pDecalData) {
                delete pDecalData;
                pDecalData = 0;
            }
        }
        iFlags = pcopy->iFlags;
        CopyAllComponents(pcopy);
    }
}

bool BGSTextureSet::Compare(TESForm* apCompare) {
    BGSTextureSet* pcompare = dynamic_cast<BGSTextureSet*>(apCompare);
    if (!pcompare) {
        return 1;
    }
    for (int32_t i = 0; i < 6; ++i) {
        if (pTextures[i].CompareComponent(&pcompare->pTextures[i])) {
            return 1;
        }
    }
    if (pcompare->pDecalData) {
        if (!pDecalData) {
            return 1;
        }
        if (pDecalData->Compare(pcompare->pDecalData)) {
            return 1;
        }
    }
    else {
        if (pDecalData) {
            return 1;
        }
    }
    return iFlags != pcompare->iFlags || CompareAllComponents(pcompare);
}

// A fade node named after the form's editor ID.
NiAVObject* BGSTextureSet::Clone3D(TESObjectREFR* apRequester) {
    NiFixedString kName;
    void* pMemory = NiMemObject::operator new(sizeof(BSFadeNode));
    BSFadeNode* pNode = nullptr;
    if (pMemory) {
        pNode = ::new (pMemory) BSFadeNode;
    }
    const char* pEditorID = GetFormEditorID();
    NiFixedString* pName = ::new (static_cast<void*>(&kName)) NiFixedString(const_cast<char*>(pEditorID));
    pNode->SetName(*pName);
    return pNode;
}

// The texture type order differs from the field order: type 2 is the fourth texture, 5 the third. An empty name is
// returned as null.
const char* BGSTextureSet::GetTextureFilename(BSShaderProperty::TextureTypeEnum aiType) {
    const char* pName = nullptr;
    switch (aiType) {
        case BSShaderProperty::TEXTYPE_DIFFUSE: pName = pTextures[0].GetTextureName(); break;
        case BSShaderProperty::TEXTYPE_NORMAL: pName = pTextures[1].GetTextureName(); break;
        case BSShaderProperty::TEXTYPE_GLOW: pName = pTextures[3].GetTextureName(); break;
        case BSShaderProperty::TEXTYPE_HEIGHT: pName = pTextures[4].GetTextureName(); break;
        case BSShaderProperty::TEXTYPE_ENV: pName = pTextures[5].GetTextureName(); break;
        case BSShaderProperty::TEXTYPE_ENV_MASK: pName = pTextures[2].GetTextureName(); break;
        default: break;
    }
    if (pName && !*pName) {
        pName = nullptr;
    }
    return pName;
}

void BGSTextureSet::GetTexture(BSShaderProperty::TextureTypeEnum aiType, NiPointer<NiTexture>& aspTexture) {
    NiPointer<NiTexture> sptexture(static_cast<NiTexture*>(0));
    const char* ptexfile = static_cast<BSTextureSet*>(this)->GetTextureFilename(aiType);
    if (ptexfile) {
        if (*ptexfile) {
            char Src[264];
            BSstrcpy(Src, 260, "Data\\Textures\\");
            BSstrcat(Src, 260, ptexfile);
            BSShaderManager::GetTexture(Src, 1, sptexture, 1, 0);
        }
    }
    aspTexture.operator=(sptexture);
}

// Implements the BSTextureSet function, using the same slot order as GetTextureFilename.
void BGSTextureSet::SetTextureFilename(BSShaderProperty::TextureTypeEnum aiType, const char* aFilename) {
    switch (aiType) {
        case BSShaderProperty::TEXTYPE_DIFFUSE: pTextures[0].SetTextureName(aFilename); break;
        case BSShaderProperty::TEXTYPE_NORMAL: pTextures[1].SetTextureName(aFilename); break;
        case BSShaderProperty::TEXTYPE_GLOW: pTextures[3].SetTextureName(aFilename); break;
        case BSShaderProperty::TEXTYPE_HEIGHT: pTextures[4].SetTextureName(aFilename); break;
        case BSShaderProperty::TEXTYPE_ENV: pTextures[5].SetTextureName(aFilename); break;
        case BSShaderProperty::TEXTYPE_ENV_MASK: pTextures[2].SetTextureName(aFilename); break;
        default: break;
    }
}

bool BGSTextureSet::HasDecalData() {
    return (pDecalData != 0);
}

DecalData* BGSTextureSet::GetDecalData() {
    if (pDecalData) {
        return pDecalData;
    }
    else {
        return DecalData::GetDefaultDecalData();
    }
}

BSTextureSet* BGSTextureSet::GetAsShaderTextureSet() {
    return this;
}

void BGSTextureSet::SetHasSpecular(bool abValue) {
    if (abValue) {
        iFlags &= (~1u);
    }
    else {
        iFlags |= 1u;
    }
}

void BGSTextureSet::QueueTextureSet(IO_TASK_PRIORITY aePriority, QueuedFile* apParent, bool abNoSpecular) {
    int32_t i;
    for (i = 0; i < 6; ++i) {
        if (pTextureFileEntries[i]) {
            pModelLoader->QueueTexture(pTextureFileEntries[i], aePriority, apParent);
        }
    }
}

BSShaderTextureSet* BGSTextureSet::CreateShaderTextureSet() {
    BSShaderTextureSet* ptexset;
    BSShaderTextureSet* apMemory =
        static_cast<BSShaderTextureSet*>(NiMemObject::operator new(sizeof(BSShaderTextureSet)));
    if (apMemory) {
        ptexset = ::new (apMemory) BSShaderTextureSet;
    }
    else {
        ptexset = 0;
    }
    for (BSShaderProperty::TextureTypeEnum i = BSShaderProperty::TEXTYPE_DIFFUSE;
         i < BSShaderProperty::TEXTYPE_SPECULAR; i = static_cast<BSShaderProperty::TextureTypeEnum>(i + 1)) {
        const char* pfilename = GetTextureFilename(i);
        if (pfilename) {
            if (*pfilename) {
                char Dst[264];
                BSstrcpy(Dst, 260, "Data\\Textures\\");
                BSstrcat(Dst, 260, pfilename);
                ptexset->SetTextureFilename(i, Dst);
            }
        }
    }
    return ptexset;
}

void BGSTextureSet::SetBGSTextureFilename(BGSTextureSet::TexType aeType, const char* apFilename) {
    if (aeType < BGSTextureSet::TEXTYPE_COUNT) {
        pTextures[aeType].SetTextureName(apFilename);
    }
}

void BGSTextureSet::CreateNullTextureSet() {
    BGSTextureSet* v0;
    void* v1;
    if (!BGSTextureSet::pNullTextureSet) {
        v1 = NiMemObject::operator new(sizeof(BGSTextureSet));
        if (v1) {
            v0 = ::new (v1) BGSTextureSet;
        }
        else {
            v0 = nullptr;
        }
        BGSTextureSet::pNullTextureSet = v0;
        v0->SetFormID(0x28u, (bool)1);
        BGSTextureSet::pNullTextureSet->SetFormEditorID("NullTextureSet");
        pDataHandler->AddFormToDataHandler(BGSTextureSet::pNullTextureSet);
    }
}

void BGSTextureSet::ClearNullTextureSet() {
    BGSTextureSet::pNullTextureSet = nullptr;
}

BGSTextureSet* BGSTextureSet::GetNullTextureSet() {
    return BGSTextureSet::pNullTextureSet;
}
