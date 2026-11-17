#include "BSAudio/BSWin32Audio.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSWin32AudioListener.h"
#include "BSAudio/BSWin32GameSound.h"
#include "BSAudio/DirectSound.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
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

int32_t SeekFileCallback(std::FILE* Stream, int32_t Offset, int32_t a3, int32_t Origin) {
    if (Stream) {
        return std::fseek(Stream, Offset, Origin);
    }
    else {
        return -1;
    }
}

BSWin32Audio::BSWin32Audio() : pDirectSound8(nullptr), pPrimarySoundBuffer(nullptr), kBufferCaps{}, hWindow(nullptr) {}

BSWin32Audio::~BSWin32Audio() {}

void BSWin32Audio::Init(void* pHandle) {
    IDirectSound3DListener* pDSListener;
    _DSBUFFERDESC v11;
    hWindow = static_cast<HWND__*>(pHandle);
    if (!bInitialized) {
        Setting::Info* bEnableAudio;
        int32_t aiResult;
        // Tests the setting's address through a null-safe accessor, which is never null.
        bEnableAudio = &BSAudio::bEnableAudio.uValue;
        if (!bEnableAudio->b) {
            goto LABEL_13;
        }
        CoInitialize(nullptr);
        aiResult = DirectSoundCreate8(nullptr, &pDirectSound8, nullptr);
        if (Error(aiResult, "Unable to create DirectSound Interface. Audio will be disabled.")) {
            aiResult = pDirectSound8->SetCooperativeLevel(hWindow, 2);
            Error(aiResult, "Unable to set DirectSound Cooperative Level.");
            kBufferCaps.dwSize = sizeof(kBufferCaps);
            aiResult = pDirectSound8->GetCaps(&kBufferCaps);
            Error(
                aiResult,
                "Unable to determine DirectSound interface capabilities. Some audio functionality may not be "
                "available."
            );
            std::memset(&v11, 0, sizeof(v11));
            // A fixed 36 would be the 32-bit sizeof; x64 DirectSound fails the create with it and audio stays off.
            v11.dwSize = sizeof(DSBUFFERDESC);
            v11.dwFlags = 145;
            aiResult = pDirectSound8->CreateSoundBuffer(&v11, &pPrimarySoundBuffer, nullptr);
            if (Error(aiResult, "Unable to create Primary Sound Buffer. Audio will be disabled.")) {
                aiResult = pPrimarySoundBuffer->QueryInterface(
                    CLSID_IDirectSound3DListener, reinterpret_cast<void**>(&pDSListener)
                );
                if (Error(aiResult, "Unable to obtain Listener Interface. Audio will be disabled.")) {
                    BSWin32AudioListener* v3;
                    BSWin32AudioListener* pListener;
                    pListener = static_cast<BSWin32AudioListener*>(
                        MemoryManager_Singleton.Allocate(sizeof(BSWin32AudioListener))
                    );
                    if (pListener) {
                        v3 = ::new (pListener) BSWin32AudioListener(pDSListener);
                    }
                    else {
                        v3 = nullptr;
                    }
                    this->pListener = v3;
                LABEL_13:;
                    if (this->pListener) {
                        NiPoint3 v6;
                        v6.x = 3.4028235e+38f;
                        v6.y = 3.4028235e+38f;
                        v6.z = 3.4028235e+38f;
                        this->pListener->SetPosition(v6);
                        this->pListener->SetRolloffFactor(1.0f);
                    }
                    bInitialized = true;
                    bAudioEnabled = true;
                    BSAudioManager* v2 = &BSAudioManager::QInstance();
                    v2->Initialize();
                    BSAudio::Init(nullptr);
                }
            }
        }
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
    BSWin32GameSound* apMemory =
        static_cast<BSWin32GameSound*>(MemoryManager_Singleton.Allocate(sizeof(BSWin32GameSound)));
    if (apMemory) {
        return ::new (apMemory) BSWin32GameSound(apcFileName);
    }
    else {
        return nullptr;
    }
}

void BSWin32Audio::FixSoundPath(char* FileName) {
    if (FileName) {
        if ((*FileName == 102 && FileName[1] == 120) ||
            (*FileName == 115 && (FileName[1] == 111 && (FileName[2] == 110 && FileName[3] == 103)))) {
            char pcDest[268];
            strcpy_s(pcDest, 260, FileName);
            strcpy_s(FileName, 260, "data\\sound\\");
            strcat_s(FileName, 260, pcDest);
        }
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

void BSWin32Audio::PrecacheSoundFromID(int32_t auiFormID, int32_t a2) {
    int32_t eMemContext = GetThreadMemContext();
    GetThreadMemContext() = 11;
    BSAudioManager* singleton = &BSAudioManager::QInstance();
    singleton->PrecacheSoundFromID(auiFormID, a2);
    GetThreadMemContext() = eMemContext;
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

void BSWin32Audio::SetAudioCacheSize(uint32_t a1) {
    MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_AUDIO;
    BSAudioManager* v2 = &BSAudioManager::QInstance();
    v2->SetAudioCacheSizeInK(a1);
    GetThreadTLSData()->eMemContext = eMemContext;
}

void BSWin32Audio::SetCacheEnabled(bool a1) {
    MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_AUDIO;
    BSAudioManager* v2 = &BSAudioManager::QInstance();
    v2->SetCacheEnabled(a1);
    GetThreadTLSData()->eMemContext = eMemContext;
}

void BSWin32Audio::SetFootstepVolumeMult(float a1) {
    BSAudioManager* v1 = &BSAudioManager::QInstance();
    v1->fVolumes[7] = a1;
    v1->VolumeChanged();
}

void BSWin32Audio::SetVoiceVolumeMult(float a1) {
    BSAudioManager* v1 = &BSAudioManager::QInstance();
    v1->fVolumes[8] = a1;
    v1->VolumeChanged();
}

void BSWin32Audio::SetEffectsVolumeMult(float a1) {
    BSAudioManager* v1 = &BSAudioManager::QInstance();
    v1->fVolumes[9] = a1;
    v1->VolumeChanged();
}

void BSWin32Audio::SetMusicVolumeMult(float a1) {
    BSAudioManager* v1 = &BSAudioManager::QInstance();
    v1->fVolumes[10] = a1;
    v1->VolumeChanged();
}

void BSWin32Audio::SetRadioVolumeMult(float a1) {
    BSAudioManager* v1 = &BSAudioManager::QInstance();
    v1->fVolumes[11] = a1;
    v1->VolumeChanged();
}

void BSWin32Audio::Create() {
    static BSWin32Audio _sInstance;
    BSAudio::pAudioInstance = &_sInstance;
}
