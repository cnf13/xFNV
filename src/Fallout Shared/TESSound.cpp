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
    data.iFlags = (uint32_t)0;
    data.iMax = (uint8_t)0;
    data.iMin = (uint8_t)0;
    data.iPitch = 0;
    data.iAttenuation = (uint16_t)0;
    data.iPadding1 = (uint8_t)0;
    data.iTimeConstraints = (uint16_t)0;
    data.pAttenuationCurve[0] = (uint16_t)100;
    data.pAttenuationCurve[1] = (uint16_t)50;
    data.pAttenuationCurve[2] = (uint16_t)20;
    data.pAttenuationCurve[3] = (uint16_t)5;
    data.pAttenuationCurve[4] = (uint16_t)0;
    data.iReverbAttenuation = (uint16_t)80;
    data.iLoopBegin = (uint32_t)0;
    data.iLoopEnd = (uint32_t)0;
    data.iPriority = (uint32_t)128;
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
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
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
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
                // The buffer is cleared first, unlike the game, because an empty chunk leaves it unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0);
                TESSoundFile* pSoundFile = this;
                pSoundFile->SetSoundFile(pBuffer);
                break;
            }
            case RNAM_ID: LoadData(apFile, &cRandomPercentChance, 1); break;
            case SNDD_ID:
                LoadData(apFile, static_cast<char*>(static_cast<void*>(&data)), 0x24);
                if (apFile->QEndian()) {
                    data.Endian();
                }
                break;
            case SNDX_ID: {
                SOUND_DATA2 kData = {};
                LoadData(apFile, static_cast<char*>(static_cast<void*>(&kData)), 0xC);
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
                if (data.pAttenuationCurve[0] == 100 && data.pAttenuationCurve[1] == 75 &&
                    data.pAttenuationCurve[2] == 50 && data.pAttenuationCurve[3] == 25 &&
                    data.pAttenuationCurve[4] == 0) {
                    data.pAttenuationCurve[0] = 100;
                    data.pAttenuationCurve[1] = 50;
                    data.pAttenuationCurve[2] = 20;
                    data.pAttenuationCurve[3] = 5;
                    data.pAttenuationCurve[4] = 0;
                }
                break;
            case GNAM_ID: LoadData(apFile, static_cast<char*>(static_cast<void*>(&data.iReverbAttenuation)), 2); break;
            case HNAM_ID: LoadData(apFile, static_cast<char*>(static_cast<void*>(&data.iPriority)), 4); break;
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESSound::Save() {
    TESForm::StartForm();
    static_cast<TESSoundFile*>(this)->Save();
    TESForm::AddChunk(RNAM_ID, (uint8_t)cRandomPercentChance);
    if (TESForm::QEndianSwapOnSave()) {
        (&data)->Endian();
    }
    TESForm::__AddChunkData(static_cast<CHUNK_ID>(1145327187), &data, 0x24u);
    if (TESForm::QEndianSwapOnSave()) {
        (&data)->Endian();
    }
    TESForm::CloseForm();
}

void TESSound::Copy(TESForm* apCopy) {
    TESSound::SOUND_DATA result;
    TESSound* v4 = dynamic_cast<TESSound*>(apCopy);
    if (v4) {
        cRandomPercentChance = v4->cRandomPercentChance;
        CopyAllComponents(apCopy);
        data = (result = v4->GetData());
    }
}

bool TESSound::Compare(TESForm* apCompare) {
    int32_t i;
    TESSound* v5 = dynamic_cast<TESSound*>(apCompare);
    if (!v5) {
        return (bool)1;
    }
    if (((uint8_t)v5->cRandomPercentChance) != ((uint8_t)cRandomPercentChance)) {
        return (bool)1;
    }
    for (i = 0; i < 5; ++i) {
        if (v5->data.pAttenuationCurve[i] != data.pAttenuationCurve[i]) {
            return (bool)1;
        }
    }
    return (
        ((((int32_t)v5->data.iFlags) != ((int32_t)data.iFlags)) ||
         ((v5->data.iMax != data.iMax) ||
          ((v5->data.iMin != data.iMin) ||
           ((((uint8_t)v5->data.iPitch) != ((uint8_t)data.iPitch)) ||
            ((v5->data.iAttenuation != data.iAttenuation) ||
             ((v5->data.iTimeConstraints != data.iTimeConstraints) ||
              ((v5->data.iReverbAttenuation != data.iReverbAttenuation) ||
               ((((int32_t)v5->data.iPriority) != ((int32_t)data.iPriority)) ||
                ((((int32_t)v5->data.iLoopBegin) != ((int32_t)data.iLoopBegin)) ||
                 (((int32_t)v5->data.iLoopEnd) != ((int32_t)data.iLoopEnd))))))))))) ||
        (((int8_t)TESForm::CompareAllComponents(v5)) != 0)
    );
}

const char* TESSound::GetFormEditorID() {
    if (strEditorID.pString) {
        return strEditorID.pString;
    }
    else {
        return "";
    }
}

bool TESSound::SetFormEditorID(const char* apEditorID) {
    strEditorID.Set(apEditorID, 0u);
    return 1;
}

char TESSound::GetRandomPercentChance() {
    if ((((int32_t)data.iFlags) & 2) != 0) {
        return (int8_t)((uint8_t)cRandomPercentChance);
    }
    else {
        return 0;
    }
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
    return (uint8_t)((data.iTimeConstraints & 65280) >> 8);
}

uint8_t TESSound::GetEndTime() {
    return (uint8_t)data.iTimeConstraints;
}

uint8_t TESSound::GetPriority() {
    return (uint8_t)data.iPriority;
}

uint32_t TESSound::ConvertLoopFlag(uint32_t aiTESSoundFlags) {
    uint32_t iFlags = 0;
    if (aiTESSoundFlags & 0x10) {
        iFlags = BSGameSound::SOUND_FLAG_LOOP;
    }
    else if (aiTESSoundFlags & 0x200) {
        iFlags = BSGameSound::SOUND_FLAG_ENVELOPE_FAST;
    }
    else if (aiTESSoundFlags & 0x400) {
        iFlags = BSGameSound::SOUND_FLAG_ENVELOPE_SLOW;
    }
    return iFlags;
}

void TESSound::GenerateDefaultObjects() {
    // The context object: raw storage treated as untyped until the constructor fills it.
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = static_cast<AutoMemContext*>(static_cast<void*>(aucContext));
    ::new (pContext) AutoMemContext(
        MEM_CONTEXT_TES_DEFAULT_OBJECTS, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESSound.cpp",
        1369
    );
    pContext->~AutoMemContext();
}

TESSound* TESSound::FindSoundByFilename(const char* apFilename) {
    TESSound* pretsound = 0;
    if (apFilename) {
        for (BSSimpleList<TESSound*>* plist = pDataHandler->GetSoundList(); plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            TESSound* psound = plist->m_item;
            if (psound->GetSoundFile() && !BSstricmp(psound->GetSoundFile(), apFilename)) {
                return psound;
            }
        }
    }
    return pretsound;
}

// The recent-pick ring ends with `lastSoundHash %= 10` and is never advanced, as the game does.
bool TESSound::PickRandomFilename(char* pPath) {
    char wavPath[264];
    char Dst[264];
    char oggPath[268];
    BSstrcpy(wavPath, 260, pPath);
    BSstrcpy(Dst, 260, pPath);
    BSstrcat(wavPath, 260, "*.wav");
    BSstrcpy(oggPath, 260, pPath);
    BSstrcat(oggPath, 260, "*.ogg");
    if (!ArchiveManager::QInvalidateOlderFiles()) {
        bool bretValue = ArchiveManager::GetRandomFileNameForDirectory(wavPath, Dst, ARCHIVE_TYPE_SOUNDS);
        if (bretValue) {
            BSstrcpy(pPath, 260, Dst);
        }
        return bretValue;
    }
    BSSimpleList<const char*>* pfilenameList = FileFinder::BuildFileList(oggPath, Dst, 8u, nullptr);
    if (!pfilenameList) {
        pfilenameList = FileFinder::BuildFileList(wavPath, Dst, 8u, nullptr);
    }
    if (!pfilenameList) {
        return 0;
    }
    uint32_t icount = pfilenameList->ItemsInList();
    if (!icount) {
        delete pfilenameList;
        return 0;
    }
    int32_t irandVal = Random() % icount;
    if (icount > 1) {
        uint32_t hash = BSGameSound::GetNameHash(pPath);
        for (int32_t i = 0; i < 10; ++i) {
            if (TESSound::randomNameHashes[i] == hash) {
                while (irandVal == TESSound::randomNameHashesLastPick[i]) {
                    irandVal = RandomUnsignedInt(icount);
                }
                break;
            }
        }
        TESSound::randomNameHashes[TESSound::lastSoundHash] = hash;
        TESSound::randomNameHashesLastPick[TESSound::lastSoundHash] = irandVal;
        TESSound::lastSoundHash %= 10;
    }
    for (BSSimpleList<const char*>* plist = pfilenameList; plist; plist = plist->GetNext()) {
        if (plist->GetItem()) {
            if (!irandVal) {
                BSstrcpy(pPath, 260, plist->GetItem());
            }
            ourFree(const_cast<char*>(plist->GetItem()));
        }
        --irandVal;
    }
    pfilenameList->RemoveAll();
    delete pfilenameList;
    return 1;
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
