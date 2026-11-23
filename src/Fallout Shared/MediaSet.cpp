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
#include "xfnv/Compiler.h"

#include <bit>

MediaSet::MediaLayer::MediaLayer() : Name(), Attenuation(-12.0f), Percent(100.0f) {
    Name.Set(nullptr, 0);
}

MediaSet::MediaLayer::MediaLayer(const MediaSet::MediaLayer& arSource)
  : Name(arSource.Name), Attenuation(arSource.Attenuation), Percent(arSource.Percent) {}

MediaSet::MediaLayer::~MediaLayer() {}

void MediaSet::MediaLayer::Clear() {
    Name.Set(nullptr, 0);
    Attenuation = (-6.0f);
    Percent = 100.0f;
}

void MediaSet::MediaLayer::Copy(MediaSet::MediaLayer l) {
    Name.Set(l.Name.pString, 0);
    Attenuation = l.Attenuation;
    Percent = l.Percent;
}

bool MediaSet::MediaLayer::Compare(MediaSet::MediaLayer l) {
    return !(l.Attenuation == Attenuation && (l.Percent == Percent && Name.StrCmp(l.Name.pString, false)));
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
    pMLOne.Clear();
    pMLTwo.Clear();
    pMLThree.Clear();
    pMLFour.Clear();
    pMLFive.Clear();
    pMLSix.Clear();
    fOne = 0.0f;
    fTwo = 0.0f;
    fThree = 0.0f;
    fFour = 0.0f;
    pSoundOne = nullptr;
    pSoundTwo = nullptr;
}

void MediaSet::ClearData() {
    pMLOne.Clear();
    pMLTwo.Clear();
    pMLThree.Clear();
    pMLFour.Clear();
    pMLFive.Clear();
    pMLSix.Clear();
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
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
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
                    char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
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
                LoadData(apFile, reinterpret_cast<char*>(&cEnableFlags), sizeof(cEnableFlags));
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
        uint32_t uiLength = pMLOne.Name.GetLength();
        TESForm::AddChunkArray(NAM2_ID, pMLOne.Name.pString, uiLength + 1);
    }
    if (pMLTwo.Name.GetLength()) {
        uint32_t uiLength = pMLTwo.Name.GetLength();
        TESForm::AddChunkArray(NAM3_ID, pMLTwo.Name.pString, uiLength + 1);
    }
    if (pMLThree.Name.GetLength()) {
        uint32_t uiLength = pMLThree.Name.GetLength();
        TESForm::AddChunkArray(NAM4_ID, pMLThree.Name.pString, uiLength + 1);
    }
    if (pMLFour.Name.GetLength()) {
        uint32_t uiLength = pMLFour.Name.GetLength();
        TESForm::AddChunkArray(NAM5_ID, pMLFour.Name.pString, uiLength + 1);
    }
    if (pMLFive.Name.GetLength()) {
        uint32_t uiLength = pMLFive.Name.GetLength();
        TESForm::AddChunkArray(NAM6_ID, pMLFive.Name.pString, uiLength + 1);
    }
    if (pMLSix.Name.GetLength()) {
        uint32_t uiLength = pMLSix.Name.GetLength();
        TESForm::AddChunkArray(NAM7_ID, pMLSix.Name.pString, uiLength + 1);
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
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            pSoundOne = dynamic_cast<TESSound*>(pForm);
            if (!pSoundOne) {
                int32_t iMediaSetID = iFormID;
                const char* pcEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find open sound (%08X) for MediaSet '%s' (%08X).",
                    itempInt,
                    pcEditorID,
                    iMediaSetID
                );
            }
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSoundTwo));
        if (itempInt) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempInt, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempInt);
            pSoundTwo = dynamic_cast<TESSound*>(pForm);
            if (!pSoundTwo) {
                int32_t iMediaSetID = iFormID;
                const char* pcEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Could not find open sound (%08X) for MediaSet '%s' (%08X).",
                    itempInt,
                    pcEditorID,
                    iMediaSetID
                );
            }
        }
        SetInitialized(true);
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
    bStarted = false;
    uiSeekTime = 0;
    bInCombat = false;
    bInTension = false;
    bIsDay = false;
    cCurrentLayer = 0;
}

bool MediaSet::UpdateLocation(uint32_t ticks, bool abInCombat, MediaLocationController* pController) {
    bool bIsPlaying = FalloutAudioMedia::MediaIsPlaying(MEDIA_LOCATION, false);
    bool bReturn = true;
    int8_t cLayer = 0;
    if (abInCombat) {
        return false;
    }
    uint32_t subDuration = FalloutAudioMedia::MediaGetSubDuration(MEDIA_LOCATION);
    int8_t cLoop = 0;
    if (uiLoopUpdate && subDuration && subDuration + uiLoopUpdate - fThree * 1000.0 < ticks) {
        FalloutAudioMedia::MediaFadeOut(static_cast<int32_t>(fThree * 1000.0), MEDIA_LOCATION);
        uiLoopUpdate = ticks;
        if (pController->GetLoop()) {
            return false;
        }
        cLoop = 1;
    }
    float fPlayerRadius = static_cast<float>(pController->fCurrentPlayerRadius * 100.0);
    float fMinPercent = 1000.0f;
    if (pController->GetDay()) {
        if ((cEnableFlags & ENABLE_DAY_OUTER) != 0 &&
            fPlayerRadius < static_cast<double>(pMLOne.Percent) &&
            fMinPercent > static_cast<double>(pMLOne.Percent)) {
            cLayer = 1;
            fMinPercent = fMinPercent < static_cast<double>(pMLOne.Percent) ? 1000.0f : pMLOne.Percent;
        }
        if ((cEnableFlags & ENABLE_DAY_MIDDLE) != 0 &&
            fPlayerRadius < static_cast<double>(pMLTwo.Percent) &&
            fMinPercent > static_cast<double>(pMLTwo.Percent)) {
            cLayer = 2;
            fMinPercent = fMinPercent < static_cast<double>(pMLTwo.Percent) ? fMinPercent : pMLTwo.Percent;
        }
        if ((cEnableFlags & ENABLE_DAY_INNER) != 0 &&
            fPlayerRadius < static_cast<double>(pMLThree.Percent) &&
            fMinPercent > static_cast<double>(pMLThree.Percent)) {
            cLayer = 3;
        }
    }
    else {
        if ((cEnableFlags & ENABLE_NIGHT_OUTER) != 0 &&
            fPlayerRadius < static_cast<double>(pMLFour.Percent) &&
            fMinPercent > static_cast<double>(pMLFour.Percent)) {
            cLayer = 1;
            fMinPercent = fMinPercent < static_cast<double>(pMLFour.Percent) ? 1000.0f : pMLFour.Percent;
        }
        if ((cEnableFlags & ENABLE_NIGHT_MIDDLE) != 0 &&
            fPlayerRadius < static_cast<double>(pMLFive.Percent) &&
            fMinPercent > static_cast<double>(pMLFive.Percent)) {
            cLayer = 2;
            fMinPercent = fMinPercent < static_cast<double>(pMLFive.Percent) ? fMinPercent : pMLFive.Percent;
        }
        if ((cEnableFlags & ENABLE_NIGHT_INNER) != 0 &&
            fPlayerRadius < static_cast<double>(pMLSix.Percent) &&
            fMinPercent > static_cast<double>(pMLSix.Percent)) {
            cLayer = 3;
        }
    }
    if (((!bIsPlaying || bInCombat || bIsDay != pController->GetDay() || cCurrentLayer != cLayer) &&
         FalloutAudioMedia::uiMediaLocationControllerDelay < ticks) ||
        cLoop) {
        MediaSet::MediaLayer* pLayer = nullptr;
        if (pController->GetDay()) {
            switch (cLayer) {
                case 1: {
                    pLayer = &pMLOne;
                    break;
                }
                case 2: {
                    pLayer = &pMLTwo;
                    break;
                }
                case 3: {
                    pLayer = &pMLThree;
                    break;
                }
            }
        }
        else {
            switch (cLayer) {
                case 1: {
                    pLayer = &pMLFour;
                    break;
                }
                case 2: {
                    pLayer = &pMLFive;
                    break;
                }
                case 3: {
                    pLayer = &pMLSix;
                    break;
                }
            }
        }
        if (pLayer) {
            FalloutAudioMedia::uiMediaLocationControllerDelay = static_cast<uint32_t>(
                static_cast<int64_t>(ticks + fOne * 1000.0)
            );
            uint32_t uiSynchTime = 0;
            if (FalloutAudioMedia::MediaGetPosition(MEDIA_LOCATION) && !cLoop) {
                uiSynchTime = ticks - FalloutAudioMedia::MediaGetPosition(MEDIA_LOCATION);
            }

            float fAttenuation = pLayer->Attenuation;
            bool bLoop = pController->GetLoop() == MediaLocationController::LOOP_LOOP;
            uint32_t uiFadeTime = static_cast<uint32_t>(static_cast<int64_t>(fThree * 1000.0));
            void* pName = pLayer->Name.pString;
            FalloutAudioMedia::MediaOpen(
                MEDIA_LOCATION,
                static_cast<const char*>(pName),
                uiFadeTime,
                bLoop,
                true,
                fAttenuation,
                uiSynchTime
            );
            FalloutAudioMedia::MediaPlay();
            uiLoopUpdate = ticks;
        }
        else {
            if (bIsPlaying) {
                FalloutAudioMedia::MediaFadeOut(static_cast<int32_t>(fThree * 1000.0), MEDIA_LOCATION);
            }
            bReturn = false;
        }
        bInCombat = false;
        this->bIsDay = pController->GetDay();
        cCurrentLayer = cLayer;
    }
    return bReturn;
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
            return false;
        }
        return UpdateBattle(abInCombat, ticks, MEDIA_BATTLE);
    }
    if (this->Type == MediaSet::INCIDENTAL) {
        if (MediaSet::uiLastUpdate > ticks) {
            return true;
        }
        UpdateIncidentals(ticks);
        return false;
    }
    if (this->Type == MediaSet::BATTLE) {
        return UpdateBattle(abInCombat, ticks, MEDIA_BATTLE);
    }
    return false;
}

bool MediaSet::UpdateDungeon(uint32_t ticks, bool abInCombat, MediaLocationController* pController) {
    bool bMusicPlaying = FalloutAudioMedia::MediaIsPlaying(MEDIA_DUNGEON, true);
    bool bReturn = true;
    char cLayer = pController->fCurrentPlayerRadius < 100.0;
    if (!bInCombat && abInCombat) {
        FalloutAudioMedia::uiMediaLocationControllerDelay = 0;
    }
    uint32_t dur = FalloutAudioMedia::MediaGetDuration(MEDIA_LOCATION);
    bool bLoop = false;
    if (uiLoopUpdate &&
        (dur && static_cast<double>(dur + uiLoopUpdate) - fThree * 1000.0 < static_cast<double>(ticks))) {
        FalloutAudioMedia::MediaFadeOut(static_cast<int32_t>(fThree * 1000.0), MEDIA_LOCATION);
        uiLoopUpdate = ticks;
        if (pController->GetLoop()) {
            return false;
        }
        bLoop = true;
    }
    if (((bMusicPlaying &&
          bInCombat == abInCombat &&
          bInTension == pController->bInTension &&
          (cCurrentLayer == cLayer || cCurrentLayer)) ||
         FalloutAudioMedia::uiMediaLocationControllerDelay >= ticks) &&
        !bLoop) {
        if (!cLayer && ticks > MediaSet::uiLastUpdate && !bStarted) {
            if (bMusicPlaying) {
                FalloutAudioMedia::MediaFadeOut(static_cast<int32_t>(fThree * 1000.0), MEDIA_DUNGEON);
            }
            return false;
        }
    }
    else {
        MediaSet::MediaLayer* newLayer = nullptr;
        if (abInCombat) {
            if (pSoundOne && !bStarted) {
                uint32_t uiSoundID = pSoundOne->iFormID;
                BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 2305u).Play(false);
            }

            newLayer = &pMLOne;
            bStarted = true;
        }
        else if (pController->bInTension) {
            if (bStarted && pSoundTwo) {
                uint32_t uiSoundID = pSoundTwo->iFormID;
                BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 2305u).Play(false);
            }

            newLayer = &pMLThree;
            bStarted = false;
        }
        else {
            if (bStarted && pSoundTwo) {
                uint32_t uiSoundID = pSoundTwo->iFormID;
                BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 2305u).Play(false);
            }

            newLayer = &pMLTwo;
            bStarted = false;
        }
        if (newLayer) {
            FalloutAudioMedia::uiMediaLocationControllerDelay = ticks;
            if (!abInCombat) {
                FalloutAudioMedia::uiMediaLocationControllerDelay = static_cast<int64_t>(
                    fOne * 1000.0 + static_cast<double>(FalloutAudioMedia::uiMediaLocationControllerDelay)
                );
            }
            uint32_t uiSeekTime = 0;
            if (FalloutAudioMedia::MediaGetPosition(MEDIA_DUNGEON) && !bLoop) {
                uiSeekTime = ticks - FalloutAudioMedia::MediaGetPosition(MEDIA_DUNGEON);
            }

            if (newLayer == &pMLOne) {
                FalloutAudioMedia::MediaOpen(
                    MEDIA_DUNGEON,
                    newLayer->Name.pString,
                    1000u,
                    true,
                    true,
                    newLayer->Attenuation,
                    uiSeekTime
                );
            }
            else {
                FalloutAudioMedia::MediaOpen(
                    MEDIA_DUNGEON,
                    newLayer->Name.pString,
                    static_cast<int64_t>(fThree * 1000.0),
                    true,
                    true,
                    newLayer->Attenuation,
                    uiSeekTime
                );
            }
            FalloutAudioMedia::MediaPlay();
        }
        else {
            FalloutAudioMedia::MediaFadeOut(static_cast<int32_t>(fThree * 1000.0), MEDIA_DUNGEON);
            bReturn = false;
        }
        bInCombat = abInCombat;
        this->bInTension = pController->bInTension;
        cCurrentLayer = cLayer;
    }
    return bReturn;
}

bool MediaSet::UpdateBattle(bool bInCombat, uint32_t ticks, MEDIATYPE mType) {
    if (bInCombat) {
        bool bIsLoaded = FalloutAudioMedia::MediaIsPlaying(mType, false);
        if (!bIsLoaded && !bStarted) {
            if (pSoundOne && !bStarted) {
                uint32_t uiSoundID = pSoundOne->iFormID;
                BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 2305u).Play(false);
            }

            MediaSet::uiLastUpdate = ticks - 1;
            bStarted = true;
            FalloutAudioMedia::MediaOpen(
                mType,
                pMLOne.Name.pString,
                static_cast<int64_t>(fTwo * 1000.0),
                true,
                true,
                pMLOne.Attenuation,
                0
            );
            FalloutAudioMedia::MediaPlay();
            FalloutAudioMedia::MediaPause();
        }
        if (static_cast<double>(MediaSet::uiLastUpdate) + fOne < static_cast<double>(ticks)) {
            FalloutAudioMedia::MediaUnPause();
            bStarted = true;
        }
        uint32_t dur = FalloutAudioMedia::MediaGetDuration(mType);
        if (dur && ticks > MediaSet::uiLastUpdate + dur - 1000) {
            FalloutAudioMedia::MediaOpen(
                mType,
                pMLOne.Name.pString,
                static_cast<int64_t>(fTwo * 1000.0),
                true,
                true,
                pMLOne.Attenuation,
                0
            );
            FalloutAudioMedia::MediaPlay();
            MediaSet::uiLastUpdate = ticks;
        }
    }
    else {
        bool bShouldPlayOutro = false;
        if (FalloutAudioMedia::MediaIsPlaying(mType, false) && bStarted) {
            bShouldPlayOutro = true;
            FalloutAudioMedia::MediaFadeOut(static_cast<int32_t>(fTwo * 1000.0), mType);
        }
        if (bShouldPlayOutro) {
            if (pSoundTwo) {
                uint32_t uiSoundID = pSoundTwo->iFormID;
                BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 2305u).Play(false);
            }
            MediaSet::uiLastUpdate = static_cast<int64_t>(static_cast<double>(ticks) + fThree * 1000.0);
            if (FalloutAudioMedia::pCurrentMediaController) {
                FalloutAudioMedia::pCurrentMediaController->uiLocationDelay = MediaSet::uiLastUpdate;
            }
            bStarted = false;
        }
    }
    return bInCombat;
}

bool MediaSet::UpdateIncidentals(uint32_t ticks) {
    char cUseDaySound;
    if (ticks <= MediaSet::uiLastUpdate) {
        return true;
    }
    Sky* psky = TES::pInstance->pSky;
    if (psky) {
        double dSunriseBegin = psky->GetSunriseBegin();
        bool bInDaylight = false;
        if (psky->GetCurrentGameHour() >= dSunriseBegin) {
            double dCurrentGameHour = psky->GetCurrentGameHour();
            if (psky->GetSunsetEnd() >= dCurrentGameHour) {
                bInDaylight = true;
            }
        }
        cUseDaySound = bInDaylight;
    }
    else {
        cUseDaySound = 1;
    }
    if (cUseDaySound) {
        uint32_t uiSoundID = pSoundOne->iFormID;
        BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 2305u).Play(false);
        MediaSet::uiLastUpdate = ticks +
            RandomInt(static_cast<int32_t>(fOne * 1000.0), static_cast<int32_t>(fThree * 1000.0));
    }
    else {
        uint32_t uiSoundID = pSoundTwo->iFormID;
        BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 2305u).Play(false);
        MediaSet::uiLastUpdate = ticks +
            RandomInt(static_cast<int32_t>(fTwo * 1000.0), static_cast<int32_t>(fFour * 1000.0));
    }
    return true;
}
