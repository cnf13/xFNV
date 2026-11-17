#include "Fallout Shared/MediaSet.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTaskManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MediaLocationController.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
#include "SDK/MSVC/rtti.h"

#include <bit>

MediaSet::MediaLayer::MediaLayer() : Name(), Attenuation(-12.0f), Percent(100.0f) {
    Name.Set(nullptr, 0);
}

MediaSet::MediaLayer::MediaLayer(const MediaSet::MediaLayer& arSource)
  : Name(arSource.Name), Attenuation(arSource.Attenuation), Percent(arSource.Percent) {}

MediaSet::MediaLayer::~MediaLayer() {}

void MediaSet::MediaLayer::Clear() {
    (&Name)->Set(nullptr, (uint32_t)0);
    Attenuation = (-6.0f);
    Percent = 100.0f;
}

void MediaSet::MediaLayer::Copy(MediaSet::MediaLayer l) {
    Name.Set(l.Name.pString, 0);
    Attenuation = l.Attenuation;
    Percent = l.Percent;
}

bool MediaSet::MediaLayer::Compare(MediaSet::MediaLayer l) {
    if (l.Attenuation == Attenuation && (l.Percent == Percent && Name.StrCmp(l.Name.pString, static_cast<bool>(0)))) {
        return 0;
    }
    else {
        return 1;
    }
}

MediaSet::MediaSet()
  : TESForm(), TESFullName(), uiSeekTime(0), uiLoopUpdate(0), bStarted(false), bInCombat(false), bIsDay(false),
    cCurrentLayer(0), MediaSetName(), Type(SETTYPE_NONE), fOne(0.0f), fTwo(0.0f), fThree(0.0f), fFour(0.0f),
    pSoundOne(nullptr), pSoundTwo(nullptr) {
    cFormType = 111;
    pMLOne.Clear();
    pMLTwo.Clear();
    pMLThree.Clear();
    pMLFour.Clear();
    pMLFive.Clear();
    pMLSix.Clear();
    MediaSet::uiLastUpdate = 0;
}

MediaSet::~MediaSet() {}

void MediaSet::InitializeData() {
    (&pMLOne)->Clear();
    (&pMLTwo)->Clear();
    (&pMLThree)->Clear();
    (&pMLFour)->Clear();
    (&pMLFive)->Clear();
    (&pMLSix)->Clear();
    fOne = (float)0.0;
    fTwo = (float)0.0;
    fThree = (float)0.0;
    fFour = (float)0.0;
    pSoundOne = nullptr;
    pSoundTwo = nullptr;
}

void MediaSet::ClearData() {
    (&pMLOne)->Clear();
    (&pMLTwo)->Clear();
    (&pMLThree)->Clear();
    (&pMLFour)->Clear();
    (&pMLFive)->Clear();
    (&pMLSix)->Clear();
    pSoundOne = nullptr;
    pSoundTwo = nullptr;
}

// When the EDID chunk is empty (malformed data), the EDID string is initialised to "" instead of being read
// uninitialised as in the game.
bool MediaSet::Load(TESFile* apFile) {
    uint32_t uiValue = 0;
    LoadForm(apFile);
    SetInitialized(false);
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
                // An empty chunk leaves the buffer unwritten, so it is terminated first.
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(static_cast<TESFullName*>(this), apFile); break;
            case NAM1_ID:
                apFile->GetChunkData<uint32_t>(uiValue);
                Type = static_cast<MEDIASETTYPE>(uiValue);
                break;
            case NAM2_ID:
            case NAM3_ID:
            case NAM4_ID:
            case NAM5_ID:
            case NAM6_ID:
            case NAM7_ID: {
                uint32_t uiChunkSize = apFile->GetChunkSize();
                if (uiChunkSize != 0) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
                    apFile->GetChunkData(pBuffer, 0);
                    // The music path is built in untyped storage, so no constructor runs on it first.
                    alignas(BSStringT<char>) char cPathStorage[sizeof(BSStringT<char>)];
                    BSStringT<char>* pPath = reinterpret_cast<BSStringT<char>*>(cPathStorage);
                    ::new (cPathStorage) BSStringT<char>("Data\\Music\\");
                    *pPath += pBuffer;
                    switch (uiChunk) {
                        case NAM2_ID: pMLOne.Name.Set(*pPath); break;
                        case NAM3_ID: pMLTwo.Name.Set(*pPath); break;
                        case NAM4_ID: pMLThree.Name.Set(*pPath); break;
                        case NAM5_ID: pMLFour.Name.Set(*pPath); break;
                        case NAM6_ID: pMLFive.Name.Set(*pPath); break;
                        case NAM7_ID: pMLSix.Name.Set(*pPath); break;
                    }
                    // SWEEP-INLINE 0x4037D0
                    pPath->~BSStringT<char>();
                }
                break;
            }
            // The float chunks hold the field's four bytes, which are read straight into the field.
            case NAM0_ID:
            case ANAM_ID:
            case BNAM_ID:
            case CNAM_ID:
            case NAM8_ID:
            case NAM9_ID: {
                float fValue = 0.0f;
                apFile->GetChunkData<uint32_t>(*reinterpret_cast<uint32_t*>(&fValue));
                switch (uiChunk) {
                    case NAM8_ID: pMLOne.Attenuation = fValue; break;
                    case NAM9_ID: pMLTwo.Attenuation = fValue; break;
                    case NAM0_ID: pMLThree.Attenuation = fValue; break;
                    case ANAM_ID: pMLFour.Attenuation = fValue; break;
                    case BNAM_ID: pMLFive.Attenuation = fValue; break;
                    case CNAM_ID: pMLSix.Attenuation = fValue; break;
                }
                break;
            }
            case JNAM_ID:
            case KNAM_ID:
            case LNAM_ID:
            case MNAM_ID:
            case NNAM_ID:
            case ONAM_ID: {
                float fValue = 0.0f;
                apFile->GetChunkData<uint32_t>(*reinterpret_cast<uint32_t*>(&fValue));
                switch (uiChunk) {
                    case JNAM_ID: pMLOne.Percent = fValue; break;
                    case KNAM_ID: pMLTwo.Percent = fValue; break;
                    case LNAM_ID: pMLThree.Percent = fValue; break;
                    case MNAM_ID: pMLFour.Percent = fValue; break;
                    case NNAM_ID: pMLFive.Percent = fValue; break;
                    case ONAM_ID: pMLSix.Percent = fValue; break;
                }
                break;
            }
            case DNAM_ID:
            case ENAM_ID:
            case FNAM_ID:
            case GNAM_ID: {
                float fValue = 0.0f;
                apFile->GetChunkData<uint32_t>(*reinterpret_cast<uint32_t*>(&fValue));
                switch (uiChunk) {
                    case DNAM_ID: fOne = fValue; break;
                    case ENAM_ID: fTwo = fValue; break;
                    case FNAM_ID: fThree = fValue; break;
                    case GNAM_ID: fFour = fValue; break;
                }
                break;
            }
            case HNAM_ID:
            case INAM_ID: {
                uint32_t uiSoundID = 0;
                apFile->GetChunkData<uint32_t>(uiSoundID);
                // The chunk holds a form ID that stands in for the sound until it is resolved.
                if (uiChunk == HNAM_ID) {
                    pSoundOne = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID));
                }
                else {
                    pSoundTwo = reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID));
                }
                break;
            }
            case PNAM_ID:
                // LoadData takes the chunk's raw bytes, which here are the enable flags.
                LoadData(apFile, reinterpret_cast<char*>(&cEnableFlags), 1);
                break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void MediaSet::Save() {
    StartForm();
    TESFullName::Save();
    TESForm::AddChunk(NAM1_ID, Type);
    if (pMLOne.Name.GetLength()) {
        uint32_t Length = pMLOne.Name.GetLength();
        TESForm::AddChunkArray(NAM2_ID, pMLOne.Name.pString, Length + 1);
    }
    if (pMLTwo.Name.GetLength()) {
        uint32_t v2 = pMLTwo.Name.GetLength();
        TESForm::AddChunkArray(NAM3_ID, pMLTwo.Name.pString, v2 + 1);
    }
    if (pMLThree.Name.GetLength()) {
        uint32_t v3 = pMLThree.Name.GetLength();
        TESForm::AddChunkArray(NAM4_ID, pMLThree.Name.pString, v3 + 1);
    }
    if (pMLFour.Name.GetLength()) {
        uint32_t v4 = pMLFour.Name.GetLength();
        TESForm::AddChunkArray(NAM5_ID, pMLFour.Name.pString, v4 + 1);
    }
    if (pMLFive.Name.GetLength()) {
        uint32_t v5 = pMLFive.Name.GetLength();
        TESForm::AddChunkArray(NAM6_ID, pMLFive.Name.pString, v5 + 1);
    }
    if (pMLSix.Name.GetLength()) {
        uint32_t v6 = pMLSix.Name.GetLength();
        TESForm::AddChunkArray(NAM7_ID, pMLSix.Name.pString, v6 + 1);
    }
    TESForm::AddChunk(NAM8_ID, std::bit_cast<uint32_t>(pMLOne.Attenuation));
    TESForm::AddChunk(NAM9_ID, std::bit_cast<uint32_t>(pMLTwo.Attenuation));
    TESForm::AddChunk(NAM0_ID, std::bit_cast<uint32_t>(pMLThree.Attenuation));
    TESForm::AddChunk(ANAM_ID, std::bit_cast<uint32_t>(pMLFour.Attenuation));
    TESForm::AddChunk(BNAM_ID, std::bit_cast<uint32_t>(pMLFive.Attenuation));
    TESForm::AddChunk(CNAM_ID, std::bit_cast<uint32_t>(pMLSix.Attenuation));
    TESForm::AddChunk(JNAM_ID, std::bit_cast<uint32_t>(pMLOne.Percent));
    TESForm::AddChunk(KNAM_ID, std::bit_cast<uint32_t>(pMLTwo.Percent));
    TESForm::AddChunk(LNAM_ID, std::bit_cast<uint32_t>(pMLThree.Percent));
    TESForm::AddChunk(MNAM_ID, std::bit_cast<uint32_t>(pMLFour.Percent));
    TESForm::AddChunk(NNAM_ID, std::bit_cast<uint32_t>(pMLFive.Percent));
    TESForm::AddChunk(ONAM_ID, std::bit_cast<uint32_t>(pMLSix.Percent));
    TESForm::AddChunk(PNAM_ID, cEnableFlags);
    TESForm::AddChunk(DNAM_ID, std::bit_cast<uint32_t>(fOne));
    TESForm::AddChunk(ENAM_ID, std::bit_cast<uint32_t>(fTwo));
    TESForm::AddChunk(FNAM_ID, std::bit_cast<uint32_t>(fThree));
    TESForm::AddChunk(GNAM_ID, std::bit_cast<uint32_t>(fFour));
    if (pSoundOne) {
        TESForm::AddChunk(HNAM_ID, pSoundOne->iFormID);
    }
    if (pSoundTwo) {
        TESForm::AddChunk(INAM_ID, pSoundTwo->iFormID);
    }
    SaveData();
    CloseForm();
}

void MediaSet::InitItem() {
    if (!GetInitialized()) {
        uint32_t itempInt = 0;
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSoundOne));
        if (itempInt) {
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempInt);
            pSoundOne = dynamic_cast<TESSound*>(FormByNumericID);
            if (!pSoundOne) {
                int32_t CastingType = iFormID;
                const char* v5 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find open sound (%08X) for MediaSet '%s' (%08X).", itempInt, v5, CastingType
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSoundTwo));
        if (itempInt) {
            TESFile* v3 = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, v3);
            TESForm* v4 = TESForm::GetFormByNumericID(itempInt);
            pSoundTwo = dynamic_cast<TESSound*>(v4);
            if (!pSoundTwo) {
                int32_t v8 = iFormID;
                const char* v6 = GetFormEditorID();
                Warning("MASTERFILE: Could not find open sound (%08X) for MediaSet '%s' (%08X).", itempInt, v6, v8);
            }
        }
        SetInitialized(1);
    }
}

// Each layer is passed to MediaLayer::Copy by value: it is copy-constructed in the argument and the callee destroys it.
void MediaSet::Copy(TESForm* apCopy) {
    MediaSet* pCopy = dynamic_cast<MediaSet*>(apCopy);
    if (!pCopy) {
        return;
    }
    Type = pCopy->Type;
    pMLOne.Copy(pCopy->pMLOne);
    pMLTwo.Copy(pCopy->pMLTwo);
    pMLThree.Copy(pCopy->pMLThree);
    pMLFour.Copy(pCopy->pMLFour);
    pMLFive.Copy(pCopy->pMLFive);
    pMLSix.Copy(pCopy->pMLSix);
    cEnableFlags = pCopy->cEnableFlags;
    fOne = pCopy->fOne;
    fTwo = pCopy->fTwo;
    fThree = pCopy->fThree;
    fFour = pCopy->fFour;
    pSoundOne = pCopy->pSoundOne;
    pSoundTwo = pCopy->pSoundTwo;
    CopyAllComponents(pCopy);
}

// True when the forms differ. The layers go to MediaLayer::Compare by value as in Copy above. A float that is NaN
// on either side differs.
bool MediaSet::Compare(TESForm* apCompare) {
    MediaSet* pCopy = dynamic_cast<MediaSet*>(apCompare);
    if (!pCopy) {
        return true;
    }
    if (pCopy->Type != Type) {
        return true;
    }
    if (pMLOne.Compare(pCopy->pMLOne)) {
        return true;
    }
    if (pMLTwo.Compare(pCopy->pMLTwo)) {
        return true;
    }
    if (pMLThree.Compare(pCopy->pMLThree)) {
        return true;
    }
    if (pMLFour.Compare(pCopy->pMLFour)) {
        return true;
    }
    if (pMLFive.Compare(pCopy->pMLFive)) {
        return true;
    }
    if (pMLSix.Compare(pCopy->pMLSix)) {
        return true;
    }
    if (cEnableFlags != pCopy->cEnableFlags) {
        return true;
    }
    if (fOne != pCopy->fOne) {
        return true;
    }
    if (fTwo != pCopy->fTwo) {
        return true;
    }
    if (fThree != pCopy->fThree) {
        return true;
    }
    if (fFour != pCopy->fFour) {
        return true;
    }
    if (pSoundOne != pCopy->pSoundOne) {
        return true;
    }
    if (pSoundTwo != pCopy->pSoundTwo) {
        return true;
    }
    return CompareAllComponents(pCopy);
}

void MediaSet::Reset() {
    bStarted = (bool)0;
    uiSeekTime = (uint32_t)0;
    bInCombat = (bool)0;
    bInTension = (bool)0;
    bIsDay = (bool)0;
    cCurrentLayer = 0;
}

bool MediaSet::UpdateLocation(uint32_t ticks, bool abInCombat, MediaLocationController* pController) {
    int32_t bIsDay;
    void* v6;
    uint32_t v7;
    uint8_t v8;
    float afAttenuation;
    float v10;
    float v11;
    float v12;
    float v13;
    float v14;
    float percent;
    uint32_t auiSynchTime;
    MediaSet::MediaLayer* p_pMLOne;
    uint8_t isPlaying = (uint8_t)FalloutAudioMedia::MediaIsPlaying(MEDIA_LOCATION, 0);
    uint8_t v25 = 1;
    int8_t v23 = 0;
    if (abInCombat) {
        return (bool)0;
    }
    uint32_t subDuration = FalloutAudioMedia::MediaGetSubDuration(MEDIA_LOCATION);
    int8_t v21 = 0;
    if (uiLoopUpdate &&
        (subDuration && ((((double)(subDuration + uiLoopUpdate)) - (fThree * 1000.0)) < ((double)ticks)))) {
        FalloutAudioMedia::MediaFadeOut((int32_t)(fThree * 1000.0), MEDIA_LOCATION);
        uiLoopUpdate = ticks;
        if (pController->GetLoop()) {
            return (bool)0;
        }
        v21 = 1;
    }
    float v19 = (float)(pController->fCurrentPlayerRadius * 100.0);
    float v22 = 1000.0f;
    if (pController->GetDay()) {
        if ((((cEnableFlags & 1) != 0) && (v19 < ((double)pMLOne.Percent))) && (v22 > ((double)pMLOne.Percent))) {
            v23 = 1;
            if (!(v22 < ((double)pMLOne.Percent))) {
                percent = pMLOne.Percent;
            }
            else {
                percent = 1000.0f;
            }
            v22 = percent;
        }
        if ((((cEnableFlags & 2) != 0) && (v19 < ((double)pMLTwo.Percent))) && (v22 > ((double)pMLTwo.Percent))) {
            v23 = 2;
            if (!(v22 < ((double)pMLTwo.Percent))) {
                v14 = pMLTwo.Percent;
            }
            else {
                v14 = v22;
            }
            v22 = v14;
        }
        if ((((cEnableFlags & 4) != 0) && (v19 < ((double)pMLThree.Percent))) && (v22 > ((double)pMLThree.Percent))) {
            v23 = 3;
            if (!(v22 < ((double)pMLThree.Percent))) {
                v13 = pMLThree.Percent;
            }
        }
    }
    else {
        if ((((cEnableFlags & 8) != 0) && (v19 < ((double)pMLFour.Percent))) && (v22 > ((double)pMLFour.Percent))) {
            v23 = 1;
            if (!(v22 < ((double)pMLFour.Percent))) {
                v12 = pMLFour.Percent;
            }
            else {
                v12 = 1000.0f;
            }
            v22 = v12;
        }
        if ((((cEnableFlags & 16) != 0) && (v19 < ((double)pMLFive.Percent))) && (v22 > ((double)pMLFive.Percent))) {
            v23 = 2;
            if (!(v22 < ((double)pMLFive.Percent))) {
                v11 = pMLFive.Percent;
            }
            else {
                v11 = v22;
            }
            v22 = v11;
        }
        if ((((cEnableFlags & 32) != 0) && (v19 < ((double)pMLSix.Percent))) && (v22 > ((double)pMLSix.Percent))) {
            v23 = 3;
            if (!(v22 < ((double)pMLSix.Percent))) {
                v10 = pMLSix.Percent;
            }
        }
    }
    if ((((!isPlaying) ||
          (bInCombat ||
           (((bIsDay = (int32_t)this->bIsDay), (((uint32_t)bIsDay) != ((uint32_t)((uint8_t)pController->GetDay())))) ||
            (cCurrentLayer != v23)))) &&
         (((uint32_t)FalloutAudioMedia::uiMediaLocationControllerDelay) < ticks)) ||
        v21) {
        p_pMLOne = nullptr;
        if (pController->GetDay()) {
            switch (v23) {
                case 1: {
                    p_pMLOne = &pMLOne;
                    break;
                }
                case 2: {
                    p_pMLOne = &pMLTwo;
                    break;
                }
                case 3: {
                    p_pMLOne = &pMLThree;
                    break;
                }
            }
        }
        else {
            switch (v23) {
                case 1: {
                    p_pMLOne = &pMLFour;
                    break;
                }
                case 2: {
                    p_pMLOne = &pMLFive;
                    break;
                }
                case 3: {
                    p_pMLOne = &pMLSix;
                    break;
                }
            }
        }
        if (p_pMLOne) {
            FalloutAudioMedia::uiMediaLocationControllerDelay =
                (uint32_t)((int64_t)(((double)ticks) + (fOne * 1000.0)));
            auiSynchTime = 0;
            if (FalloutAudioMedia::MediaGetPosition(MEDIA_LOCATION)) {
                if (!v21) {
                    auiSynchTime = ticks - FalloutAudioMedia::MediaGetPosition(MEDIA_LOCATION);
                }
            }
            afAttenuation = p_pMLOne->Attenuation;
            v8 = (uint8_t)(((int32_t)pController->GetLoop()) == 0);
            v7 = (uint32_t)((int64_t)(fThree * 1000.0));
            v6 = p_pMLOne->Name.pString;
            FalloutAudioMedia::MediaOpen(
                MEDIA_LOCATION, static_cast<const char*>(v6), v7, (bool)v8, (bool)1, afAttenuation, auiSynchTime
            );
            FalloutAudioMedia::MediaPlay();
            uiLoopUpdate = ticks;
        }
        else {
            if (isPlaying) {
                FalloutAudioMedia::MediaFadeOut((int32_t)(fThree * 1000.0), MEDIA_LOCATION);
            }
            v25 = 0;
        }
        bInCombat = (bool)0;
        this->bIsDay = (bool)((uint8_t)pController->GetDay());
        cCurrentLayer = v23;
    }
    return (bool)v25;
}

bool MediaSet::Update(uint32_t ticks, bool abInCombat, MediaLocationController* pController) {
    if (pController) {
        MediaSet::MEDIASETTYPE Type = this->Type;
        if (Type) {
            if (Type == MediaSet::LAYER_LOCATION) {
                return UpdateLocation(ticks, abInCombat, pController);
            }
            if (Type == MediaSet::LAYER_DUNGEON) {
                return UpdateDungeon(ticks, abInCombat, pController);
            }
            return 0;
        }
        return UpdateBattle(abInCombat, ticks, MEDIA_BATTLE);
    }
    if (this->Type == MediaSet::INCIDENTAL) {
        if (MediaSet::uiLastUpdate > ticks) {
            return 1;
        }
        UpdateIncidentals(ticks);
        return 0;
    }
    if (this->Type == MediaSet::BATTLE) {
        return UpdateBattle(abInCombat, ticks, MEDIA_BATTLE);
    }
    return 0;
}

bool MediaSet::UpdateDungeon(uint32_t ticks, bool abInCombat, MediaLocationController* pController) {
    int32_t bInTension;
    FNVD_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, apHandle, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    bool bMusicPlaying = FalloutAudioMedia::MediaIsPlaying(MEDIA_DUNGEON, 1);
    bool bReturn = 1;
    char cLayer = pController->fCurrentPlayerRadius < 100.0;
    if (!bInCombat && abInCombat) {
        FalloutAudioMedia::uiMediaLocationControllerDelay = 0;
    }
    uint32_t dur = FalloutAudioMedia::MediaGetDuration(MEDIA_LOCATION);
    bool bLoop = 0;
    if (uiLoopUpdate &&
        (dur && static_cast<double>(dur + uiLoopUpdate) - fThree * 1000.0 < static_cast<double>(ticks))) {
        FalloutAudioMedia::MediaFadeOut(static_cast<int32_t>(fThree * 1000.0), MEDIA_LOCATION);
        uiLoopUpdate = ticks;
        if (pController->GetLoop()) {
            return 0;
        }
        bLoop = 1;
    }
    if (((bMusicPlaying &&
          (bInCombat == abInCombat &&
           ((bInTension = this->bInTension, bInTension == pController->bInTension) &&
            (cCurrentLayer == cLayer || cCurrentLayer)))) ||
         FalloutAudioMedia::uiMediaLocationControllerDelay >= ticks) &&
        !bLoop) {
        if (!cLayer && ticks > MediaSet::uiLastUpdate && !bStarted) {
            if (bMusicPlaying) {
                FalloutAudioMedia::MediaFadeOut(static_cast<int32_t>(fThree * 1000.0), MEDIA_DUNGEON);
            }
            return 0;
        }
    }
    else {
        MediaSet::MediaLayer* newLayer = 0;
        if (abInCombat) {
            if (pSoundOne) {
                if (!bStarted) {
                    uint32_t CastingType = pSoundOne->iFormID;
                    BSAudio* v6 = &BSAudio::QInstance();
                    BSSoundHandle* SoundHandleByNumericID = v6->GetSoundHandleByNumericID(&a1, CastingType, 2305u);
                    SoundHandleByNumericID->Play(0);
                }
            }
            newLayer = &pMLOne;
            bStarted = 1;
        }
        else {
            if (pController->bInTension) {
                if (bStarted) {
                    if (pSoundTwo) {
                        uint32_t v10 = pSoundTwo->iFormID;
                        BSAudio* v7 = &BSAudio::QInstance();
                        BSSoundHandle* v13 = v7->GetSoundHandleByNumericID(&apHandle, v10, 2305u);
                        v13->Play(0);
                    }
                }
                newLayer = &pMLThree;
                bStarted = 0;
            }
            else {
                if (bStarted) {
                    if (pSoundTwo) {
                        uint32_t v11 = pSoundTwo->iFormID;
                        BSAudio* v8 = &BSAudio::QInstance();
                        BSSoundHandle* v12 = v8->GetSoundHandleByNumericID(&th_s, v11, 2305u);
                        v12->Play(0);
                    }
                }
                newLayer = &pMLTwo;
                bStarted = 0;
            }
        }
        if (newLayer) {
            FalloutAudioMedia::uiMediaLocationControllerDelay = ticks;
            if (!abInCombat) {
                FalloutAudioMedia::uiMediaLocationControllerDelay = static_cast<int64_t>(
                    fOne * 1000.0 + static_cast<double>(FalloutAudioMedia::uiMediaLocationControllerDelay)
                );
            }
            uint32_t uiSeekTime = 0;
            if (FalloutAudioMedia::MediaGetPosition(MEDIA_DUNGEON)) {
                if (!bLoop) {
                    uiSeekTime = ticks - FalloutAudioMedia::MediaGetPosition(MEDIA_DUNGEON);
                }
            }
            if (newLayer == &pMLOne) {
                FalloutAudioMedia::MediaOpen(
                    MEDIA_DUNGEON, newLayer->Name.pString, 1000u, 1, 1, newLayer->Attenuation, uiSeekTime
                );
            }
            else {
                FalloutAudioMedia::MediaOpen(
                    MEDIA_DUNGEON, newLayer->Name.pString, static_cast<int64_t>(fThree * 1000.0), 1, 1,
                    newLayer->Attenuation, uiSeekTime
                );
            }
            FalloutAudioMedia::MediaPlay();
        }
        else {
            FalloutAudioMedia::MediaFadeOut(static_cast<int32_t>(fThree * 1000.0), MEDIA_DUNGEON);
            bReturn = 0;
        }
        bInCombat = abInCombat;
        this->bInTension = pController->bInTension;
        cCurrentLayer = cLayer;
    }
    return bReturn;
}

bool MediaSet::UpdateBattle(bool bInCombat, uint32_t ticks, MEDIATYPE mType) {
    FNVD_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, apHandle, std::nullopt, std::nullopt);
    if (bInCombat) {
        bool bShouldPlayIntro = 0;
        bool bIsLoaded = FalloutAudioMedia::MediaIsPlaying(mType, 0);
        if (!bIsLoaded && !bStarted) {
            if (pSoundOne) {
                if (!bStarted) {
                    uint32_t CastingType = pSoundOne->iFormID;
                    BSAudio* v4 = &BSAudio::QInstance();
                    BSSoundHandle* SoundHandleByNumericID =
                        v4->GetSoundHandleByNumericID(&apHandle, CastingType, 2305u);
                    SoundHandleByNumericID->Play(0);
                }
            }
            MediaSet::uiLastUpdate = ticks - 1;
            bStarted = 1;
            FalloutAudioMedia::MediaOpen(
                mType, pMLOne.Name.pString, static_cast<int64_t>(fTwo * 1000.0), 1, 1, pMLOne.Attenuation, 0
            );
            FalloutAudioMedia::MediaPlay();
            FalloutAudioMedia::MediaPause();
        }
        if (static_cast<double>(MediaSet::uiLastUpdate) + fOne < static_cast<double>(ticks)) {
            FalloutAudioMedia::MediaUnPause();
            bStarted = 1;
        }
        uint32_t dur = FalloutAudioMedia::MediaGetDuration(mType);
        if (dur) {
            if (ticks > MediaSet::uiLastUpdate + dur - 1000) {
                FalloutAudioMedia::MediaOpen(
                    mType, pMLOne.Name.pString, static_cast<int64_t>(fTwo * 1000.0), 1, 1, pMLOne.Attenuation, 0
                );
                FalloutAudioMedia::MediaPlay();
                MediaSet::uiLastUpdate = ticks;
            }
        }
    }
    else {
        bool bShouldPlayOutro = 0;
        if (FalloutAudioMedia::MediaIsPlaying(mType, 0) && bStarted) {
            bShouldPlayOutro = 1;
            FalloutAudioMedia::MediaFadeOut(static_cast<int32_t>(fTwo * 1000.0), mType);
        }
        if (bShouldPlayOutro) {
            if (pSoundTwo) {
                uint32_t v8 = pSoundTwo->iFormID;
                BSAudio* v5 = &BSAudio::QInstance();
                BSSoundHandle* v9 = v5->GetSoundHandleByNumericID(&th_s, v8, 2305u);
                v9->Play(0);
            }
            MediaSet::uiLastUpdate = static_cast<int64_t>(static_cast<double>(ticks) + fThree * 1000.0);
            if (FalloutAudioMedia::pCurrentMediaController) {
                FalloutAudioMedia::pCurrentMediaController->uiLocationDelay = MediaSet::uiLastUpdate;
            }
            bStarted = 0;
        }
    }
    return bInCombat;
}

bool MediaSet::UpdateIncidentals(uint32_t ticks) {
    char v9;
    FNVD_INLINED_LOCAL(BSSoundHandle, apHandle, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    if (ticks <= MediaSet::uiLastUpdate) {
        return 1;
    }
    Sky* psky = TES::pInstance->pSky;
    if (psky) {
        double SunriseBegin = psky->GetSunriseBegin();
        bool v10 = false;
        if (psky->GetCurrentGameHour() >= SunriseBegin) {
            double CurrentGameHour = psky->GetCurrentGameHour();
            if (psky->GetSunsetEnd() >= CurrentGameHour) {
                v10 = true;
            }
        }
        v9 = v10;
    }
    else {
        v9 = 1;
    }
    if (v9) {
        uint32_t CastingType = pSoundOne->iFormID;
        BSAudio* v3 = &BSAudio::QInstance();
        BSSoundHandle* SoundHandleByNumericID = v3->GetSoundHandleByNumericID(&a1, CastingType, 2305u);
        SoundHandleByNumericID->Play(0);
        MediaSet::uiLastUpdate =
            ticks + RandomInt(static_cast<int32_t>(fOne * 1000.0), static_cast<int32_t>(fThree * 1000.0));
    }
    else {
        uint32_t v6 = pSoundTwo->iFormID;
        BSAudio* v4 = &BSAudio::QInstance();
        BSSoundHandle* v7 = v4->GetSoundHandleByNumericID(&apHandle, v6, 2305u);
        v7->Play(0);
        MediaSet::uiLastUpdate =
            ticks + RandomInt(static_cast<int32_t>(fTwo * 1000.0), static_cast<int32_t>(fFour * 1000.0));
    }
    return 1;
}
