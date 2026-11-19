#include "Fallout Shared/TESModel.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSArchive.h"
#include "BSSystem/BSHash.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

// The game settings this file defines, with their IDs and default values, in initialization order.
SettingT<INISettingCollection> bWarnOnMissingFileEntry("bWarnOnMissingFileEntry:General", false);
SettingT<INISettingCollection> bUseOptimizedTextureLoading("bUseOptimizedTextureLoading:General", true);

TESTextureList::TESTextureList() : cTextureCount(0), pTextureOffsetArray(nullptr) {}

TESTextureList::~TESTextureList() {
    ClearData();
}

void TESTextureList::Copy(TESTextureList* apTextureList) {
    ClearData();
    cTextureCount = apTextureList->cTextureCount;
    pTextureOffsetArray = static_cast<BSFileEntry**>(
        ourMalloc(cTextureCount <= SIZE_MAX / sizeof(BSFileEntry*) ? sizeof(BSFileEntry*) * cTextureCount : SIZE_MAX)
    );
    ourMemcpy(pTextureOffsetArray, apTextureList->pTextureOffsetArray, sizeof(BSFileEntry*) * cTextureCount);
}

// The chunk is a run of 24-byte texture hash records; a trailing partial record is ignored.
void TESTextureList::Load(TESFile* apFile, TESForm* apParentForm, const char* apModelName) {
    uint32_t uiChunkSize = apFile->GetChunkSize();
    // The division is unsigned.
    if ((uiChunkSize % 0x18) != 0) {
        return;
    }
    uint32_t uiCount = uiChunkSize / 0x18;
    if (uiCount == 0) {
        return;
    }
    uint32_t uiSize = uiCount * 0x18;
    // The records are built in an alloca'd buffer that is zeroed first.
    BSHashData* pBuffer = static_cast<BSHashData*>(__builtin_alloca(uiSize));
    ourMemset(pBuffer, 0, uiSize);
    apFile->GetChunkData(pBuffer, uiSize);
    LoadFromBuffer(pBuffer, uiCount, apParentForm, apModelName);
}

void TESTextureList::LoadFromBuffer(
    BSHashData* apBuffer,
    uint32_t aiCount,
    TESForm* apParentForm,
    const char* apModelName
) {
    Archive* pmasterTextureArchive = ArchiveManager::GetMasterArchive(ARCHIVE_TYPE_INDEX_TEXTURES);
    if ((pmasterTextureArchive) && (static_cast<uint8_t>(pmasterTextureArchive->HasEmbeddedFileNames())) &&
        (bUseOptimizedTextureLoading.Bool())) {
        cTextureCount = aiCount;
        // The allocation size is 0xFFFFFFFF when 4 * aiCount would overflow 32 bits, as the game does.
        pTextureOffsetArray = static_cast<BSFileEntry**>(
            ourMalloc((aiCount >> 30) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(sizeof(BSFileEntry*) * aiCount))
        );
        for (uint32_t i = 0; i < aiCount; ++i) {
            BSHash filehash2(
                apBuffer[i].cLast, apBuffer[i].cLast2, apBuffer[i].cLength, apBuffer[i].cFirst, apBuffer[i].iCRC
            );
            BSHash dirhash(
                apBuffer[i].cDirLast, apBuffer[i].cDirLast2, apBuffer[i].cDirLength, apBuffer[i].cDirFirst,
                apBuffer[i].iDirCRC
            );
            BSFileEntry* FileEntryForFileFromAllArchives = ArchiveManager::GetFileEntryForFileFromAllArchives(
                ARCHIVE_TYPE_INDEX_TEXTURES, dirhash, filehash2, nullptr
            );
            pTextureOffsetArray[i] = FileEntryForFileFromAllArchives;
            if (bWarnOnMissingFileEntry.Bool() && ArchiveManager::ArchivesEnabled() && !pTextureOffsetArray[i]) {
                if (apParentForm && apModelName) {
                    int32_t CastingType = apParentForm->iFormID;
                    Warning(
                        "MASTERFILE: Failed to find archive file entry for texture on model '%s' for form "
                        "%s (%08X).",
                        apModelName, apParentForm->GetFormEditorID(), CastingType
                    );
                }
                else {
                    if (apModelName) {
                        Warning(
                            "MASTERFILE: Failed to find archive file entry for texture on model '%s' on "
                            "UNKNOWN form.",
                            apModelName
                        );
                    }
                    else {
                        Warning("MASTERFILE: Failed to find archive file entry for texture on UNKNOWN Model");
                    }
                }
            }
        }
    }
}

bool TESTextureList::ClearData() {
    uint8_t v3 = 0;
    if (pTextureOffsetArray) {
        ourFree(pTextureOffsetArray);
        pTextureOffsetArray = (BSFileEntry**)nullptr;
        v3 = 1;
    }
    cTextureCount = (uint8_t)0;
    return (bool)v3;
}

void TESTextureList::operator delete(void* apMemory) {
    ourFree(apMemory);
}

TESModel::TESModel() : BaseFormComponent(), cModel(), TextureList() {
    TESModel::InitializeDataComponent();
}

TESModel::~TESModel() {
    TESModel::ClearDataComponent();
}

// Clear the model path when set, then reset the model flags.
void TESModel::InitializeDataComponent() {
    if (cModel.String()) {
        cModel.Set(nullptr, 0);
    }
    cFlags = 0;
}

void TESModel::ClearDataComponent() {
    TextureList.ClearData();
}

// Copy the model path when apCopyFC is another TESModel.
void TESModel::CopyComponent(BaseFormComponent* apCopyFC) {
    auto* model = dynamic_cast<TESModel*>(apCopyFC);
    if (model) {
        const char* modelName = model->GetModel();
        SetModel(modelName);
    }
}

// Compare model paths, treating missing models as equal only when both are empty.
bool TESModel::CompareComponent(BaseFormComponent* apCompareFC) {
    auto* model = dynamic_cast<TESModel*>(apCompareFC);
    if (!model) {
        return true;
    }

    const char* thisModel = GetModel();
    const char* otherModel = model->GetModel();
    if (thisModel && otherModel) {
        return BSstricmp(thisModel, otherModel) != 0;
    }
    if (thisModel && static_cast<int8_t>(*thisModel) != 0) {
        return true;
    }
    return otherModel && static_cast<int8_t>(*otherModel) != 0;
}

const char* TESModel::GetModel() {
    if (cModel.pString) {
        return cModel.pString;
    }

    return "";
}

void TESModel::SetModel(const char* apModel) {
    (&cModel)->Set(apModel, (uint32_t)0);
}

void TESModel::GetDistantModelName(char* apModelName) {
    const char* v2 = GetModel();
    BSstrcpy(apModelName, 260, v2);
    char* Dst = std::strrchr(apModelName, static_cast<int32_t>(46));
    if (Dst) {
        BSstrcpy(Dst, 260 - (Dst - apModelName), "_far.NIF");
    }
}

void TESModel::Save(CHUNK_ID aeModelID, CHUNK_ID aeTextureID) {
    if (GetModelLength()) {
        if (aeModelID) {
            uint32_t v4 = GetModelLength() + 1;
            const char* v3 = GetModel();
            TESForm::AddChunkArray(aeModelID, v3, v4);
        }
        if (aeTextureID) {
            const char* v5 = GetModel();
        }
    }
}

uint32_t TESModel::GetSkinFlags() const {
    return cFlags & 0xE;
}

void TESModel::SetFlags(char acFlags) {
    cFlags = acFlags;
}

void TESModel::SaveFlags(CHUNK_ID aeFlagsID) {
    uint8_t cFlagsToSave = QFlags();
    if (cFlagsToSave) {
        TESForm::AddChunk(aeFlagsID, cFlagsToSave);
    }
}

uint32_t TESModel::GetModelLength() {
    return (&cModel)->GetLength();
}

bool TESModel::QHasFacegenData() {
    return (bool)(cFlags & 1);
}

// Writes apForm's distant model name into apModelName when apForm is a TESModel.
void TESModel::GetDistantModelName_(char* apModelName, TESForm* apForm) {
    if (apForm) {
        TESModel* pModel = dynamic_cast<TESModel*>(apForm);
        if (pModel) {
            pModel->GetDistantModelName(apModelName);
        }
    }
}

// Record codes 'MODL', 'MODD', 'MODS' and 'MODT' as little-endian integers.
bool TESModel::IsModelChunk(CHUNK_ID aChunk) {
    switch (aChunk) {
        case 0x4C444F4D:
        case 0x44444F4D:
        case 0x44534F4D:
        case 0x54444F4D: return true;
        default: return false;
    }
}

// Read the model, texture, or flag data represented by the current chunk.
void TESModel::LoadModelChunk(TESModel* apModel, TESFile* apFile) {
    if (!apFile || !apModel) {
        return;
    }

    int32_t chunk = static_cast<int32_t>(apFile->GetTESChunk());
    // PNAM is read as the model path, the same as MODL.
    if (chunk == static_cast<int32_t>(CHUNK_ID::MODL_ID) || chunk == static_cast<int32_t>(CHUNK_ID::PNAM_ID)) {
        void* data = __builtin_alloca(apFile->GetChunkSize());
        apFile->GetChunkData(data, 0);
        apModel->SetModel(static_cast<const char*>(data));
    }
    else if (chunk == static_cast<int32_t>(CHUNK_ID::MODT_ID)) {
        LoadModelTextureChunk(apModel, apFile);
    }
    else if (chunk == static_cast<int32_t>(CHUNK_ID::MODD_ID) || chunk == static_cast<int32_t>(CHUNK_ID::MOSD_ID)) {
        uint8_t flags;
        apFile->GetChunkData(&flags, 1);
        apModel->SetFlags(flags);
    }
}

// Resolve the model's name and owning form before loading its texture list.
void TESModel::LoadModelTextureChunk(TESModel* apModel, TESFile* apFile) {
    if (apModel && apFile) {
        const char* modelName = apModel->GetModel();
        auto* parentForm = dynamic_cast<TESForm*>(apModel);
        apModel->TextureList.Load(apFile, parentForm, const_cast<char*>(modelName));
    }
}

// Return apObj's direct model, its reference world model, or the shared empty string.
const char* TESModel::GetModel(TESForm* apObj) {
    auto* model = dynamic_cast<TESModel*>(apObj);
    if (!model) {
        auto* objectRefr = dynamic_cast<TESObjectREFR*>(apObj);
        model = TESBipedModelForm::GetWorldTESModel(apObj, objectRefr);
    }
    return model ? model->GetModel() : "";
}

uint32_t TESModel::GetModelLength_00489540(TESForm* apObj) {
    TESModel* pmodel = dynamic_cast<TESModel*>(apObj);
    if (!pmodel) {
        TESObjectREFR* v1 = dynamic_cast<TESObjectREFR*>(apObj);
        pmodel = TESBipedModelForm::GetWorldTESModel(apObj, v1);
    }
    if (pmodel) {
        return pmodel->GetModelLength();
    }

    return 0;
}

// The group's master archive from the first set, else from the second.
Archive* ArchiveManager::GetMasterArchive(ARCHIVE_TYPE_INDEX aeTypeIndex) {
    NiPointer<Archive>& spMaster =
        spMasterArchives[0][aeTypeIndex] ? spMasterArchives[0][aeTypeIndex] : spMasterArchives[1][aeTypeIndex];
    return spMaster;
}
