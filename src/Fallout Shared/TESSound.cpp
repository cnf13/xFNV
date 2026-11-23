#include "Fallout Shared/TESSound.h"

#include "BSAudio/BSGameSound.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESSoundFile.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

static SettingT<GameSettingCollection> fCombatSoundWeaponPct("fCombatSoundWeaponPct", 0.85f);
static SettingT<GameSettingCollection> fCombatSoundArmorPct("fCombatSoundArmorPct", 0.9f);
static SettingT<GameSettingCollection> fCombatSoundFleshPct("fCombatSoundFleshPct", 0.8f);

// The volume bytes and the pitch need no swap; the rest is swapped in field order.
void TESSound::SOUND_DATA::Endian() {
    ENDIAN(iFlags, false);
    ENDIAN(iAttenuation, false);
    ENDIAN(iTimeConstraints, false);
    ENDIAN(pAttenuationCurve[0], false);
    ENDIAN(pAttenuationCurve[1], false);
    ENDIAN(pAttenuationCurve[2], false);
    ENDIAN(pAttenuationCurve[3], false);
    ENDIAN(pAttenuationCurve[4], false);
    ENDIAN(iReverbAttenuation, false);
    ENDIAN(iPriority, false);
    ENDIAN(iLoopBegin, false);
    ENDIAN(iLoopEnd, false);
}

// The three attenuation bytes and the pitch need no swap.
void TESSound::SOUND_DATA2::Endian() {
    ENDIAN(iFlags, false);
    ENDIAN(iAttenuation, false);
    ENDIAN(iTimeConstraints, false);
}

TESSound::TESSound() : TESBoundAnimObject(), TESSoundFile(), strEditorID(), cRandomPercentChance(0) {
    cFormType = 13;
    TESSound::InitializeData();
}

TESSound::~TESSound() {}

void TESSound::InitializeData() {
    data.iFlags = 0;
    data.iMax = 0;
    data.iMin = 0;
    data.iPitch = 0;
    data.iAttenuation = 0;
    data.iPadding1 = 0;
    data.iTimeConstraints = 0;
    data.pAttenuationCurve[0] = 100;
    data.pAttenuationCurve[1] = 50;
    data.pAttenuationCurve[2] = 20;
    data.pAttenuationCurve[3] = 5;
    data.pAttenuationCurve[4] = 0;
    data.iReverbAttenuation = 80;
    data.iLoopBegin = 0;
    data.iLoopEnd = 0;
    data.iPriority = 128;
}

// SOUN. SNDX is the older playback data (SOUND_DATA2), read into a temporary and copied field by field; an ANAM curve
// of 100/75/50/25/0 (the old default) is rewritten to 100/50/20/5/0. TESForm::LoadData copies only the chunk's bytes,
// so the SNDX temporary is initialised to 0 for a chunk shorter than 0xC, and the EDID and FNAM buffers to "" (one
// zeroed byte, allocated only for an empty chunk) for malformed data.
bool TESSound::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != SOUN_ID) {
        return false;
    }
    LoadForm(apFile);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // The buffer is cleared first, unlike the game, because an empty chunk leaves it unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case FNAM_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // The buffer is cleared first, unlike the game, because an empty chunk leaves it unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                TESSoundFile* pSoundFile = this;
                pSoundFile->SetSoundFile(pBuffer);
                break;
            }
            case RNAM_ID: LoadData(apFile, &cRandomPercentChance, sizeof(cRandomPercentChance)); break;
            case SNDD_ID:
                LoadData(apFile, static_cast<char*>(static_cast<void*>(&data)), sizeof(data));
                if (apFile->QEndian()) {
                    data.Endian();
                }
                break;
            case SNDX_ID: {
                SOUND_DATA2 kData = {};
                LoadData(apFile, static_cast<char*>(static_cast<void*>(&kData)), sizeof(kData));
                if (apFile->QEndian()) {
                    kData.Endian();
                }
                data.iMin = kData.iMin;
                data.iMax = kData.iMax;
                data.iPitch = kData.iPitch;
                data.iFlags = kData.iFlags;
                data.iAttenuation = kData.iAttenuation;
                data.iTimeConstraints = kData.iTimeConstraints;
                break;
            }
            case ANAM_ID:
                LoadData(apFile, static_cast<char*>(static_cast<void*>(data.pAttenuationCurve)), 0xA);
                if (apFile->QEndian()) {
                    ENDIAN(data.pAttenuationCurve[0], false);
                    ENDIAN(data.pAttenuationCurve[1], false);
                    ENDIAN(data.pAttenuationCurve[2], false);
                    ENDIAN(data.pAttenuationCurve[3], false);
                    ENDIAN(data.pAttenuationCurve[4], false);
                }
                if (data.pAttenuationCurve[0] == 100 &&
                    data.pAttenuationCurve[1] == 75 &&
                    data.pAttenuationCurve[2] == 50 &&
                    data.pAttenuationCurve[3] == 25 &&
                    data.pAttenuationCurve[4] == 0) {
                    data.pAttenuationCurve[0] = 100;
                    data.pAttenuationCurve[1] = 50;
                    data.pAttenuationCurve[2] = 20;
                    data.pAttenuationCurve[3] = 5;
                    data.pAttenuationCurve[4] = 0;
                }
                break;
            case GNAM_ID:
                LoadData(
                    apFile,
                    static_cast<char*>(static_cast<void*>(&data.iReverbAttenuation)),
                    sizeof(data.iReverbAttenuation)
                );
                break;
            case HNAM_ID:
                LoadData(apFile, static_cast<char*>(static_cast<void*>(&data.iPriority)), sizeof(data.iPriority));
                break;
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESSound::Save() {
    TESForm::StartForm();
    static_cast<TESSoundFile*>(this)->Save();
    TESForm::AddChunk(RNAM_ID, static_cast<uint8_t>(cRandomPercentChance));
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::__AddChunkData(SNDD_ID, &data, 0x24u);
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::CloseForm();
}

void TESSound::Copy(TESForm* apCopy) {
    TESSound* pSound = dynamic_cast<TESSound*>(apCopy);
    if (pSound) {
        cRandomPercentChance = pSound->cRandomPercentChance;
        CopyAllComponents(apCopy);
        data = pSound->GetData();
    }
}

bool TESSound::Compare(TESForm* apCompare) {
    TESSound* pSound = dynamic_cast<TESSound*>(apCompare);
    if (!pSound) {
        return true;
    }
    if (pSound->cRandomPercentChance != cRandomPercentChance) {
        return true;
    }
    for (int32_t i = 0; i < 5; ++i) {
        if (pSound->data.pAttenuationCurve[i] != data.pAttenuationCurve[i]) {
            return true;
        }
    }
    return pSound->data.iFlags != data.iFlags ||
        pSound->data.iMax != data.iMax ||
        pSound->data.iMin != data.iMin ||
        pSound->data.iPitch != data.iPitch ||
        pSound->data.iAttenuation != data.iAttenuation ||
        pSound->data.iTimeConstraints != data.iTimeConstraints ||
        pSound->data.iReverbAttenuation != data.iReverbAttenuation ||
        pSound->data.iPriority != data.iPriority ||
        pSound->data.iLoopBegin != data.iLoopBegin ||
        pSound->data.iLoopEnd != data.iLoopEnd ||
        TESForm::CompareAllComponents(pSound);
}

const char* TESSound::GetFormEditorID() {
    if (strEditorID.pString) {
        return strEditorID.pString;
    }

    return "";
}

bool TESSound::SetFormEditorID(const char* apEditorID) {
    strEditorID.Set(apEditorID, 0u);
    return true;
}

char TESSound::GetRandomPercentChance() {
    if ((data.iFlags & FLAG_PLAY_AT_RANDOM) != 0) {
        return cRandomPercentChance;
    }

    return 0;
}

uint8_t TESSound::GetMaxDistance() {
    return data.iMax;
}

uint8_t TESSound::GetMinDistance() {
    return data.iMin;
}

uint16_t TESSound::GetAttenuationCurveValue(int32_t aiIndex) {
    return data.pAttenuationCurve[aiIndex];
}

uint16_t TESSound::GetReverbAttenuation() {
    return data.iReverbAttenuation;
}

uint8_t TESSound::GetStartTime() {
    return static_cast<uint8_t>((data.iTimeConstraints & 65280) >> 8);
}

uint8_t TESSound::GetEndTime() {
    return static_cast<uint8_t>(data.iTimeConstraints);
}

uint8_t TESSound::GetPriority() {
    return static_cast<uint8_t>(data.iPriority);
}

uint32_t TESSound::ConvertLoopFlag(uint32_t aiTESSoundFlags) {
    uint32_t iFlags = 0;
    if (aiTESSoundFlags & FLAG_LOOP) {
        iFlags = BSGameSound::SOUND_FLAG_LOOP;
    }
    else if (aiTESSoundFlags & FLAG_ENVELOPE_FAST) {
        iFlags = BSGameSound::SOUND_FLAG_ENVELOPE_FAST;
    }
    else if (aiTESSoundFlags & FLAG_ENVELOPE_SLOW) {
        iFlags = BSGameSound::SOUND_FLAG_ENVELOPE_SLOW;
    }
    return iFlags;
}

void TESSound::GenerateDefaultObjects() {
    // The context object: raw storage treated as untyped until the constructor fills it.
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = static_cast<AutoMemContext*>(static_cast<void*>(aucContext));
    ::new (pContext) AutoMemContext(
        MEM_CONTEXT_TES_DEFAULT_OBJECTS,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSound.cpp",
        1369
    );
    pContext->~AutoMemContext();
}

TESSound* TESSound::FindSoundByFilename(const char* apFilename) {
    TESSound* pRetSound = nullptr;
    if (apFilename) {
        for (BSSimpleList<TESSound*>* pList = pDataHandler->GetSoundList(); pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            TESSound* pSound = pList->m_item;
            if (pSound->GetSoundFile() && !BSstricmp(pSound->GetSoundFile(), apFilename)) {
                return pSound;
            }
        }
    }
    return pRetSound;
}

// The recent-pick ring ends with `lastSoundHash %= 10` and is never advanced.
bool TESSound::PickRandomFilename(char* pPath) {
    char wavPath[264];
    char basePath[264];
    char oggPath[268];
    BSstrcpy(wavPath, 260, pPath);
    BSstrcpy(basePath, 260, pPath);
    BSstrcat(wavPath, 260, "*.wav");
    BSstrcpy(oggPath, 260, pPath);
    BSstrcat(oggPath, 260, "*.ogg");
    if (!ArchiveManager::QInvalidateOlderFiles()) {
        bool bRetValue = ArchiveManager::GetRandomFileNameForDirectory(wavPath, basePath, ARCHIVE_TYPE_SOUNDS);
        if (bRetValue) {
            BSstrcpy(pPath, 260, basePath);
        }
        return bRetValue;
    }
    BSSimpleList<const char*>* pFilenameList = FileFinder::BuildFileList(oggPath, basePath, 8u, nullptr);
    if (!pFilenameList) {
        pFilenameList = FileFinder::BuildFileList(wavPath, basePath, 8u, nullptr);
    }
    if (!pFilenameList) {
        return false;
    }
    uint32_t uiCount = pFilenameList->ItemsInList();
    if (!uiCount) {
        delete pFilenameList;
        return false;
    }
    int32_t iRandVal = Random() % uiCount;
    if (uiCount > 1) {
        uint32_t uiHash = BSGameSound::GetNameHash(pPath);
        for (int32_t i = 0; i < 10; ++i) {
            if (TESSound::randomNameHashes[i] == uiHash) {
                while (iRandVal == TESSound::randomNameHashesLastPick[i]) {
                    iRandVal = RandomUnsignedInt(uiCount);
                }
                break;
            }
        }
        TESSound::randomNameHashes[TESSound::lastSoundHash] = uiHash;
        TESSound::randomNameHashesLastPick[TESSound::lastSoundHash] = iRandVal;
        TESSound::lastSoundHash %= 10;
    }
    for (BSSimpleList<const char*>* pList = pFilenameList; pList; pList = pList->GetNext()) {
        if (pList->GetItem()) {
            if (!iRandVal) {
                BSstrcpy(pPath, 260, pList->GetItem());
            }
            ourFree(const_cast<char*>(pList->GetItem()));
        }
        --iRandVal;
    }
    pFilenameList->RemoveAll();
    delete pFilenameList;
    return true;
}

// Both bounds are tested signed.
const char* GetBushSoundName(int32_t aiIndex) {
    if (aiIndex < 0 || aiIndex >= 10) {
        return nullptr;
    }
    return g_bushSoundNameArray[aiIndex];
}

TESSound* GetNthBrushPassthroughSound(int32_t aiIndex) {
    if (pPlayer) {
        const char* pMove;
        if (pPlayer->IsSneaking()) {
            pMove = "Sneak";
        }
        else {
            pMove = pPlayer->IsRunning() ? "Run" : "Walk";
        }
        char acName[40];
        BSsprintf(acName, 0x28, "BRUSH%s%s", GetBushSoundName(aiIndex), pMove);
        return pDataHandler->GetSound(acName);
    }
    char acWalkName[40];
    BSsprintf(acWalkName, 0x28, "BRUSH%sWalk", GetBushSoundName(aiIndex));
    return pDataHandler->GetSound(acWalkName);
}
