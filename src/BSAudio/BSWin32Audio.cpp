#include "BSAudio/BSWin32Audio.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSWin32AudioListener.h"
#include "BSAudio/BSWin32GameSound.h"
#include "BSAudio/DirectSound.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEffectShader.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout Shared/TempEffects/BSTempEffectParticle.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Magic/MagicHitEffect.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"

#include <cstdint>
#include <limits>

int32_t SeekFileCallback(std::FILE* Stream, int32_t Offset, int32_t OffsetHigh, int32_t Origin) {
    if (Stream) {
        return std::fseek(Stream, Offset, Origin);
    }

    return -1;
}

BSWin32Audio::BSWin32Audio() : pDirectSound8(nullptr), pPrimarySoundBuffer(nullptr), kBufferCaps{}, hWindow(nullptr) {}

BSWin32Audio::~BSWin32Audio() {}

void BSWin32Audio::Init(void* pHandle) {
    IDirectSound3DListener* pDSListener;
    _DSBUFFERDESC kBufferDesc;
    hWindow = static_cast<HWND__*>(pHandle);
    if (!bInitialized) {
        Setting::Info* bEnableAudio;
        int32_t aiResult;
        // Tests the setting's address through a null-safe accessor, which is never null.
        bEnableAudio = &BSAudio::bEnableAudio.uValue;
        if (bEnableAudio->b) {
            CoInitialize(nullptr);
            aiResult = DirectSoundCreate8(nullptr, &pDirectSound8, nullptr);
            if (!Error(aiResult, "Unable to create DirectSound Interface. Audio will be disabled.")) {
                return;
            }
            aiResult = pDirectSound8->SetCooperativeLevel(hWindow, 2);
            Error(aiResult, "Unable to set DirectSound Cooperative Level.");
            kBufferCaps.dwSize = sizeof(kBufferCaps);
            aiResult = pDirectSound8->GetCaps(&kBufferCaps);
            Error(
                aiResult,
                "Unable to determine DirectSound interface capabilities. Some audio functionality may not be "
                "available."
            );
            std::memset(&kBufferDesc, 0, sizeof(kBufferDesc));
            // A fixed 36 (the exe's sizeof) would make DirectSound fail the create, and audio would stay off.
            kBufferDesc.dwSize = sizeof(DSBUFFERDESC);
            kBufferDesc.dwFlags = 145;
            aiResult = pDirectSound8->CreateSoundBuffer(&kBufferDesc, &pPrimarySoundBuffer, nullptr);
            if (!Error(aiResult, "Unable to create Primary Sound Buffer. Audio will be disabled.")) {
                return;
            }
            aiResult = pPrimarySoundBuffer->QueryInterface(
                CLSID_IDirectSound3DListener,
                reinterpret_cast<void**>(&pDSListener)
            );
            if (!Error(aiResult, "Unable to obtain Listener Interface. Audio will be disabled.")) {
                return;
            }
            this->pListener = new BSWin32AudioListener(pDSListener);
        }
        if (this->pListener) {
            NiPoint3 kPosition;
            kPosition.x = std::numeric_limits<float>::max();
            kPosition.y = std::numeric_limits<float>::max();
            kPosition.z = std::numeric_limits<float>::max();
            this->pListener->SetPosition(kPosition);
            this->pListener->SetRolloffFactor(1.0f);
        }
        bInitialized = true;
        bAudioEnabled = true;
        BSAudioManager::QInstance().Initialize();
        BSAudio::Init(nullptr);
    }
}

void BSWin32Audio::Shutdown() {
    bInitialized = false;
    BSAudio::Shutdown();
    CoUninitialize();
    if (pListener) {
        delete pListener;
        pListener = nullptr;
    }
    if (pPrimarySoundBuffer) {
        pPrimarySoundBuffer->Release();
        pPrimarySoundBuffer = nullptr;
    }
    if (pDirectSound8) {
        pDirectSound8->Release();
        pDirectSound8 = nullptr;
    }
}

BSGameSound* BSWin32Audio::CreateNewGameSound(const char* apcFileName) {
    return new BSWin32GameSound(apcFileName);
}

void BSWin32Audio::FixSoundPath(char* FileName) {
    if (FileName &&
        ((*FileName == 102 && FileName[1] == 120) ||
         (*FileName == 115 && (FileName[1] == 111 && (FileName[2] == 110 && FileName[3] == 103))))) {
        char pcDest[268];
        strcpy_s(pcDest, 260, FileName);
        strcpy_s(FileName, 260, "data\\sound\\");
        strcat_s(FileName, 260, pcDest);
    }
}

float BSWin32Audio::GetMasterVolume() {
    return BSAudioManager::QInstance().fVolumes[0];
}

float BSWin32Audio::GetFootVolume() {
    return BSAudioManager::QInstance().fVolumes[1];
}

float BSWin32Audio::GetVoiceVolume() {
    return BSAudioManager::QInstance().fVolumes[2];
}

float BSWin32Audio::GetEffectsVolume() {
    return BSAudioManager::QInstance().fVolumes[3];
}

float BSWin32Audio::GetMusicVolume() {
    return BSAudioManager::QInstance().fVolumes[4];
}

float BSWin32Audio::GetRadioVolume() {
    return BSAudioManager::QInstance().fVolumes[5];
}

void BSWin32Audio::StopMovingSounds(void* apObj, float afFadeSeconds, uint32_t aiFlags) {
    BSAudioManager::QInstance().StopMovingSounds(static_cast<NiAVObject*>(apObj), afFadeSeconds, aiFlags);
}

void BSWin32Audio::SetOrientation(NiPoint3 aFront, NiPoint3 aTop) {
    if (pListener != nullptr) {
        pListener->SetOrientation(aFront, aTop);
    }
}

void BSWin32Audio::PrecacheSoundFromID(int32_t auiFormID, int32_t aiFlags) {
    AutoMemContext kMemContext(MEM_CONTEXT_AUDIO);
    BSAudioManager::QInstance().PrecacheSoundFromID(auiFormID, aiFlags);
}

bool BSWin32Audio::IsEnableAudioCache() {
    AutoMemContext kMemContext(MEM_CONTEXT_AUDIO);
    bool bEnabled = BSAudioManager::QInstance().IsEnableAudioCache();
    return bEnabled;
}

// The failure is passed to the empty report with a fixed message. apMessage, the caller's own message (each caller
// passes a string literal), is not read.
bool BSWin32Audio::Error(int32_t aiResult, [[maybe_unused]] const char* apMessage) {
    if (aiResult >= 0) {
        return true;
    }
    InitializationFlow("Audio system could not be initialized.");
    return false;
}

void BSWin32Audio::SetAudioCacheSize(uint32_t auiNewSize) {
    AutoMemContext kMemContext(MEM_CONTEXT_AUDIO);
    BSAudioManager::QInstance().SetAudioCacheSizeInK(auiNewSize);
}

void BSWin32Audio::SetCacheEnabled(bool abEnableCache) {
    AutoMemContext kMemContext(MEM_CONTEXT_AUDIO);
    BSAudioManager::QInstance().SetCacheEnabled(abEnableCache);
}

void BSWin32Audio::SetFootstepVolumeMult(float afMult) {
    BSAudioManager* pAudioManager = &BSAudioManager::QInstance();
    pAudioManager->fVolumes[7] = afMult;
    pAudioManager->VolumeChanged();
}

void BSWin32Audio::SetVoiceVolumeMult(float afMult) {
    BSAudioManager* pAudioManager = &BSAudioManager::QInstance();
    pAudioManager->fVolumes[8] = afMult;
    pAudioManager->VolumeChanged();
}

void BSWin32Audio::SetEffectsVolumeMult(float afMult) {
    BSAudioManager* pAudioManager = &BSAudioManager::QInstance();
    pAudioManager->fVolumes[9] = afMult;
    pAudioManager->VolumeChanged();
}

void BSWin32Audio::SetMusicVolumeMult(float afMult) {
    BSAudioManager* pAudioManager = &BSAudioManager::QInstance();
    pAudioManager->fVolumes[10] = afMult;
    pAudioManager->VolumeChanged();
}

void BSWin32Audio::SetRadioVolumeMult(float afMult) {
    BSAudioManager* pAudioManager = &BSAudioManager::QInstance();
    pAudioManager->fVolumes[11] = afMult;
    pAudioManager->VolumeChanged();
}

void BSWin32Audio::Create() {
    static BSWin32Audio _sInstance;
    BSAudio::pAudioInstance = &_sInstance;
}
