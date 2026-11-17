#include "BSAudio/BSWin32GameSound.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioListener.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSWin32Audio.h"
#include "BSAudio/DirectSound.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <vorbis/vorbisfile.h>

SettingT<INISettingCollection> gs_bEnableEnviroEffectsOnPC("bEnableEnviroEffectsOnPC:Audio", false);
static SettingT<INISettingCollection> gs_iVoiceDecodeBufferSize_Audio("iVoiceDecodeBufferSize:Audio", 0x8000);
SettingT<INISettingCollection> BSWin32GameSound::iMaxFX("iMaxFX:Audio", 128);
SettingT<INISettingCollection> g_fFilterDistortionGain_Audio("fFilterDistortionGain:Audio", -18.2f);
static SettingT<INISettingCollection> gs_fFilterPEQGain_Audio("fFilterPEQGain:Audio", -15.0f);
static SettingT<INISettingCollection> fFilterdBAttenuation("fFilterdBAttenuation:Audio", 1.0f);

int32_t SeekSoundFileCallback(std::FILE* Stream, int32_t Offset, int32_t a3, int32_t Origin) {
    if (Stream) {
        return std::fseek(Stream, Offset, Origin);
    }
    else {
        return (-1);
    }
}

intptr_t CALLBACK mmio_IOProc(_MMIOINFO* lpmmioinfo, uint32_t uMsg, intptr_t lParam1, intptr_t lParam2) {
    int32_t result;
    uint32_t v7;
    int32_t v19 = 0;
    switch (uMsg) {
        case 0u: {
            BSFile* v15 = static_cast<BSFile*>(lpmmioinfo->GetProcData());
            if (v15) {
                v7 = 1;
                // MMIOM_READ: lParam1 is the buffer, lParam2 its size.
                uint32_t v5 =
                    v15->m_pfnRead(v15, reinterpret_cast<char*>(lParam1), static_cast<uint32_t>(lParam2), &v7, 1);
                v15->m_uiAbsoluteCurrentPos += v5;
                v19 = v5;
                lpmmioinfo->lDiskOffset += v5;
                result = v19;
            }
            else {
                result = 261;
            }
            break;
        }
        case 2u: {
            int32_t v13 = NiFile::ms_iSeekCur;
            if (lParam2) {
                if (lParam2 == 2) {
                    v13 = NiFile::ms_iSeekEnd;
                }
            }
            else {
                v13 = NiFile::ms_iSeekSet;
            }
            BSFile* v12 = static_cast<BSFile*>(lpmmioinfo->GetProcData());
            if (v12) {
                int32_t iTrueFilePos;
                // MMIOM_SEEK: lParam1 is the offset.
                v12->Seek(static_cast<int32_t>(lParam1), v13);
                if (v12->iAuxTrueFilePos == -1) {
                    iTrueFilePos = v12->iTrueFilePos;
                }
                else {
                    iTrueFilePos = v12->iAuxTrueFilePos;
                }
                lpmmioinfo->lDiskOffset = iTrueFilePos;
                result = 0;
            }
            else {
                result = 263;
            }
            break;
        }
        case 3u: {
            bool m_bGood;
            // MMIOM_OPEN: lParam1 is the file name.
            BSFile* File = static_cast<BSFile*>(
                FileFinder::GetFile(reinterpret_cast<char*>(lParam1), NiFile::READ_ONLY, 0x4000u, 8u)
            );
            if (File && (m_bGood = File->m_bGood)) {
                lpmmioinfo->lDiskOffset = 0;
                lpmmioinfo->SetProcData(File);
                result = 0;
            }
            else {
                if (File) {
                    delete File;
                }
                result = 257;
            }
            break;
        }
        case 4u: {
            BSFile* v16 = static_cast<BSFile*>(lpmmioinfo->GetProcData());
            if (v16) {
                delete v16;
                result = 0;
            }
            else {
                Warning("Audio: Could not close file.\n");
                result = 260;
            }
            break;
        }
        default: {
            result = 268;
            break;
        }
    }
    return result;
}

int32_t SeekStreamCallback(std::FILE* Stream, int32_t Offset, int32_t a3, int32_t Origin) {
    if (Stream) {
        return std::fseek(Stream, Offset, Origin);
    }
    else {
        return -1;
    }
}

size_t read_func(void* ptr, size_t size, size_t nmemb, void* datasource) {
    if (!datasource) {
        return 0;
    }
    // The datasource is the BSFile OggVorbisFile hands ov_open_callbacks.
    BSFile* pFile = static_cast<BSFile*>(datasource);
    uint32_t uiComponentSize = 1;
    // BinaryRead counts in 32 bits. vorbisfile asks for a few KB at a time; a larger request (a 64-bit size_t only)
    // reads what 32 bits hold, a short read as fread's may be.
    size_t uiBytes = size * nmemb;
    uint32_t uiReadBytes = uiBytes > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(uiBytes);
    return pFile->BinaryRead(ptr, uiReadBytes, &uiComponentSize, 1);
}

int32_t seek_func(void* datasource, int64_t offset, int32_t whence) {
    if (!datasource) {
        return -1;
    }
    // The datasource is the BSFile OggVorbisFile hands ov_open_callbacks; the seek uses only the low 32 bits of the
    // offset.
    BSFile* pFile = static_cast<BSFile*>(datasource);
    pFile->Seek(static_cast<int32_t>(offset), whence);
    return 0;
}

int32_t CloseFileCallback(BSFile* a1) {
    if (!a1) {
        return -1;
    }
    a1->Close();
    delete a1;
    return 0;
}

long tell_func(void* datasource) {
    if (!datasource) {
        return 0;
    }
    // The datasource is the BSFile OggVorbisFile hands ov_open_callbacks.
    BSFile* pFile = static_cast<BSFile*>(datasource);
    if (pFile->iAuxTrueFilePos == (-1)) {
        return (int32_t)pFile->iTrueFilePos;
    }
    else {
        return pFile->iAuxTrueFilePos;
    }
}

// The library's decoder state starts zeroed.
BSWin32GameSound::OggVorbisFile::OggVorbisFile()
  : OggVorbis_File{}, pFile(nullptr), iSampleCount(0), pInfo(nullptr), bLoaded(false) {}

// Lowercases the whole path into a 0x108-byte buffer, which strcpy fills unbounded, as the game does; takes the
// extension as .ogg, or renames a .mp3 or .wav one to .ogg, then opens the file and hands it to libvorbisfile. A file
// that will not open, or is not a Vorbis stream, is closed and deleted; pFile is left null in both cases.
bool BSWin32GameSound::OggVorbisFile::OpenFile(const char* apPath) {
    char cPath[0x108];
    std::strcpy(cPath, apPath);
    for (int32_t i = 0; i < 0x103; ++i) {
        cPath[i] = (char)std::tolower(cPath[i]);
    }
    char* pcExtension = std::strstr(cPath, ".ogg");
    if (!pcExtension) {
        pcExtension = std::strstr(cPath, ".mp3");
        if (!pcExtension) {
            pcExtension = std::strstr(cPath, ".wav");
        }
        if (!pcExtension) {
            return false;
        }
        pcExtension[1] = 'o';
        pcExtension[2] = 'g';
        pcExtension[3] = 'g';
    }
    pFile = static_cast<BSFile*>(FileFinder::GetFile(cPath, NiFile::READ_ONLY, 0x4000, ARCHIVE_TYPE_VOICES));
    if (pFile && pFile->m_bGood) {
        // The stream callbacks read the BSFile. There is no close callback.
        ov_callbacks kCallbacks;
        kCallbacks.read_func = read_func;
        kCallbacks.seek_func = seek_func;
        kCallbacks.close_func = nullptr;
        kCallbacks.tell_func = tell_func;
        if (ov_open_callbacks(pFile, this, nullptr, 0, kCallbacks) != 0) {
            pFile->Close();
            BSFile* pUnopenedFile = pFile;
            if (pUnopenedFile) {
                delete pUnopenedFile;
            }
            pFile = nullptr;
            return false;
        }
        bLoaded = true;
        pInfo = ov_info(this, -1);
        if (ov_seekable(this)) {
            iSampleCount = static_cast<int32_t>(ov_pcm_total(this, 0));
        }
        return true;
    }
    if (!pFile) {
        return false;
    }
    BSFile* pBadFile = pFile;
    if (pBadFile) {
        delete pBadFile;
    }
    pFile = nullptr;
    return false;
}

uint32_t BSWin32GameSound::OggVorbisFile::Read(uint8_t* apBuffer, uint32_t auiSize) {
    int32_t uiReadBytes;
    uint32_t uiTotalReadBytes = 0;
    int32_t bitstream = 0;
    while (uiTotalReadBytes <= auiSize) {
        // ov_read returns at most the int length it is given, or a negative error code: an int32_t holds it.
        uiReadBytes = static_cast<int32_t>(ov_read(
            this, static_cast<char*>(static_cast<void*>(apBuffer)), auiSize - uiTotalReadBytes, 0, 2, 1, &bitstream
        ));
        uiTotalReadBytes += uiReadBytes;
        apBuffer += uiReadBytes;
        if (!uiReadBytes) {
            auiSize = (uint32_t)0;
        }
    }
    return uiTotalReadBytes;
}

void BSWin32GameSound::OggVorbisFile::Destroy() {
    if (bLoaded) {
        ov_clear(this);
    }
    if (this->pFile) {
        BSFile* pFile = this->pFile;
        if (pFile) {
            delete pFile;
        }
        this->pFile = nullptr;
    }
}

BSWin32GameSound::BSWin32GameSound()
  : pDirectSound8(nullptr), pSoundBuffer(nullptr), pReverbBuffer(nullptr), kBufferDesc{}, pRawBuffer(nullptr),
    pTempReverbBuffer(nullptr), p3DSoundBuffer(nullptr), p3DReverbBuffer(nullptr), uiBufferBytes(0),
    pEqualizerEffect(nullptr), pDistortionEffect(nullptr), usAttenuationCurve{0, 600, 1400, 2600, 5000},
    kPosition(NiPoint3_ZERO), padding224(0), pEnvelope(nullptr), pOggFile(nullptr) {
    kBufferDesc.lpwfxFormat = static_cast<tWAVEFORMATEX*>(MemoryManager_Singleton.Allocate(sizeof(tWAVEFORMATEX)));
}

BSWin32GameSound::BSWin32GameSound(const char* apPath) : BSWin32GameSound() {
    if (apPath) {
        int32_t stDestSize = std::strlen(apPath) + 1;
        strcpy_s(pFilename, stDestSize, apPath);
    }
    else {
        std::memset(pFilename, 0, sizeof(pFilename));
    }
}

BSWin32GameSound::~BSWin32GameSound() {
    if (pSoundBuffer) {
        while (pSoundBuffer->Release()) {
            ;
        }
        pSoundBuffer = nullptr;
    }
    if (p3DSoundBuffer) {
        while (p3DSoundBuffer->Release()) {
            ;
        }
        p3DSoundBuffer = nullptr;
    }
    if (pReverbBuffer) {
        --BSWin32GameSound::uiEffectCount;
        while (pReverbBuffer->Release()) {
            ;
        }
        pReverbBuffer = nullptr;
    }
    if (p3DReverbBuffer) {
        while (p3DReverbBuffer->Release()) {
            ;
        }
        p3DReverbBuffer = nullptr;
    }
    if (kBufferDesc.lpwfxFormat) {
        MemoryManager_Singleton.Deallocate(kBufferDesc.lpwfxFormat);
        kBufferDesc.lpwfxFormat = nullptr;
    }
    if (pRawBuffer) {
        MemoryManager_Singleton.Deallocate(pRawBuffer);
        pRawBuffer = nullptr;
    }
    if (pEnvelope) {
        if (pEnvelope->pHeadBuf) {
            MemoryManager_Singleton.Deallocate(pEnvelope->pHeadBuf);
        }
        if (pEnvelope->pTailBuf) {
            MemoryManager_Singleton.Deallocate(pEnvelope->pTailBuf);
        }
        if (pEnvelope->pMidBuf) {
            MemoryManager_Singleton.Deallocate(pEnvelope->pMidBuf);
        }
        MemoryManager_Singleton.Deallocate(pEnvelope);
    }
    if (pOggFile) {
        BSWin32GameSound::OggVorbisFile* pFile = pOggFile;
        pFile->Destroy();
        MemoryManager_Singleton.Deallocate(pFile);
        pOggFile = nullptr;
    }
}

// Sets or clears the inaudible bit (0x20000) of iStateFlags; BSGameSound's StateFlags enum is not defined here.
void BSWin32GameSound::SetInaudible(bool abIn) {
    uint32_t uiNewFlags;
    if (abIn) {
        uiNewFlags = iStateFlags | 0x20000;
    }
    else {
        uiNewFlags = iStateFlags & ~0x20000u;
    }
    iStateFlags = uiNewFlags;
}

// Opens the sound: as an Ogg Vorbis stream for the region, underwater, voice, radio and music types when that works,
// otherwise as a PCM wave file (an .mp3 or .ogg name is turned into .wav) read whole into pRawBuffer. A missing radio,
// voice or looping file is replaced by a stand-in; a one-shot sound of 150 ms or less is padded to 250 ms, and every
// non-looping one gets a tail of 500 to 1000 ms (a fifth of its length between those). Envelope sounds take their
// loop points from the smpl chunk. The result goes through OpenResult, except for a bad fmt chunk, which returns
// false directly.
bool BSWin32GameSound::Open() {
    // The opening bit (0x1000) of uiStateFlags; BSGameSound's StateFlags enum is not declared.
    iStateFlags |= 0x1000;
    if (!SetDirectSound()) {
        return OpenResult(false);
    }
    if (((iSoundTypeFlags & SOUND_FLAG_REGION) || (iSoundTypeFlags & SOUND_FLAG_MAYBE_UNDERWATER) ||
         (iSoundTypeFlags & SOUND_FLAG_VOICE) || (iSoundTypeFlags & SOUND_FLAG_UNKBIT22) ||
         (iSoundTypeFlags & SOUND_FLAG_RADIO) || (iSoundTypeFlags & SOUND_FLAG_MUSIC)) &&
        OpenOgg()) {
        return OpenResult(true);
    }
    char* pExtension = std::strstr(pFilename, ".mp3");
    if (!pExtension) {
        pExtension = std::strstr(pFilename, ".ogg");
    }
    if (pExtension) {
        pExtension[1] = 'w';
        pExtension[2] = 'a';
        pExtension[3] = 'v';
    }
    pTempSoundBuffer = nullptr;
    _MMIOINFO kMMIOInfo;
    std::memset(&kMMIOInfo, 0, sizeof(kMMIOInfo));
    // The FOURCC "WAV" with a zero last byte.
    kMMIOInfo.fccIOProc = 0x564157;
    kMMIOInfo.pIOProc = &mmio_IOProc;
    // MMIO_ALLOCBUF.
    HMMIO pFile = mmioOpenA(pFilename, &kMMIOInfo, 0x10000);
    if (!pFile) {
        Warning("Audio: Could not open file %s\n", pFilename);
        if ((iSoundTypeFlags & SOUND_FLAG_UNKBIT22) || (iSoundTypeFlags & SOUND_FLAG_RADIO)) {
            strcpy_s(pFilename, 64, "Data\\Sound\\FX\\UI\\RADIO\\TEMP_radioassetmissing.wav");
            SetupPath(true);
            pFile = mmioOpenA(pFilename, &kMMIOInfo, 0x10000);
            if (!pFile) {
                return OpenResult(false);
            }
        }
        else if (iSoundTypeFlags & SOUND_FLAG_VOICE) {
            strcpy_s(pFilename, 64, "Data\\Sound\\FX\\TEMP_Missing_Voice_Asset.wav");
            SetupPath(true);
            pFile = mmioOpenA(pFilename, &kMMIOInfo, 0x10000);
            if (!pFile) {
                return OpenResult(false);
            }
        }
        else {
            // EXE-INLINED none: the looping-sound flag test (LOOP, ENVELOPE_SLOW, ENVELOPE_FAST)
            bool bLooping = (iSoundTypeFlags & SOUND_FLAG_LOOP) || (iSoundTypeFlags & SOUND_FLAG_ENVELOPE_SLOW) ||
                (iSoundTypeFlags & SOUND_FLAG_ENVELOPE_FAST);
            if (!bLooping) {
                return OpenResult(false);
            }
            iSoundTypeFlags = static_cast<SoundFlags>(iSoundTypeFlags & ~SOUND_FLAG_ENVELOPE_FAST);
            iSoundTypeFlags = static_cast<SoundFlags>(iSoundTypeFlags & ~SOUND_FLAG_ENVELOPE_SLOW);
            strcpy_s(pFilename, 64, "Data\\Sound\\FX\\silent_lp.wav");
            SetupPath(true);
            pFile = mmioOpenA(pFilename, &kMMIOInfo, 0x10000);
            if (!pFile) {
                return OpenResult(false);
            }
        }
    }

    // 0x109 is MMIOERR_CHUNKNOTFOUND, 0x10 MMIO_FINDCHUNK.
    _MMCKINFO kStartInfo;
    uint32_t eResult = mmioDescend(pFile, &kStartInfo, nullptr, 0);
    if (eResult == 0x109) {
        mmioClose(pFile, 0);
        return OpenResult(false);
    }
    _MMCKINFO kChunkInfo;
    // 'fmt '.
    kChunkInfo.ckid = 0x20746D66;
    eResult = mmioDescend(pFile, &kChunkInfo, &kStartInfo, 0x10);
    if (eResult == 0x109) {
        mmioClose(pFile, 0);
        return false;
    }
    if (kChunkInfo.cksize < sizeof(pcmwaveformat_tag)) {
        mmioClose(pFile, 0);
        return false;
    }
    pcmwaveformat_tag kWAVFormat;
    // mmioRead fills a raw byte buffer (HPSTR).
    mmioRead(pFile, reinterpret_cast<char*>(&kWAVFormat), sizeof(kWAVFormat));
    // WAVE_FORMAT_PCM.
    if (kWAVFormat.wf.wFormatTag != 1) {
        mmioClose(pFile, 0);
        return false;
    }
    mmioAscend(pFile, &kChunkInfo, 0);
    // 'data'.
    kChunkInfo.ckid = 0x61746164;
    mmioDescend(pFile, &kChunkInfo, &kStartInfo, 0x10);
    uiBufferBytes = kChunkInfo.cksize;
    iSoundSize = kChunkInfo.cksize;
    kBufferDesc.dwSize = sizeof(DSBUFFERDESC);
    if (kWAVFormat.wf.nChannels > 1 &&
        (!(iSoundTypeFlags & SOUND_FLAG_IS_2D) || (iSoundTypeFlags & SOUND_FLAG_IS_3D))) {
        iSoundTypeFlags = static_cast<SoundFlags>(iSoundTypeFlags | SOUND_FLAG_IS_2D);
        iSoundTypeFlags = static_cast<SoundFlags>(iSoundTypeFlags & ~SOUND_FLAG_IS_3D);
        if (!(iSoundTypeFlags & SOUND_FLAG_IS_2D_RADIUS)) {
            Warning("Audio: Attempted to play multi-channel sound (%s) in 3D. Forcing 2D playback.", pFilename);
        }
    }
    // DSBCAPS_CTRLPAN for a 2D sound; DSBCAPS_CTRL3D | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_MUTE3DATMAXDISTANCE for a
    // 3D one, with the light HRTF algorithm when the device has free hardware mixing buffers.
    if (iSoundTypeFlags & SOUND_FLAG_IS_2D) {
        kBufferDesc.dwFlags = 0x40;
        kBufferDesc.guid3DAlgorithm = GUID_NULL;
    }
    else {
        kBufferDesc.dwFlags = 0x30010;
        DSCAPS kCaps = static_cast<BSWin32Audio*>(BSAudio::pAudioInstance)->kBufferCaps;
        if (kCaps.dwFreeHwMixingAllBuffers) {
            kBufferDesc.guid3DAlgorithm = CLSID_DS3DALG_HRTF_LIGHT;
        }
        else {
            kBufferDesc.guid3DAlgorithm = GUID_NULL;
        }
    }
    // DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFX.
    kBufferDesc.dwFlags |= 0x2A0;
    kBufferDesc.dwBufferBytes = uiBufferBytes;
    rt::memcpy(kBufferDesc.lpwfxFormat, &kWAVFormat, sizeof(kWAVFormat));
    kBufferDesc.lpwfxFormat->cbSize = 0;
    uint32_t uiSamplesPerMS = 0;
    uint32_t uiBytesPerSample = 0;
    iSamplesPerSecond = static_cast<uint16_t>(kBufferDesc.lpwfxFormat->nSamplesPerSec);
    iModSamplesPerSecond = iSamplesPerSecond;
    uiSamplesPerMS = iSamplesPerSecond / 1000;
    uiBytesPerSample = kBufferDesc.lpwfxFormat->wBitsPerSample / 8;
    iDurationInMS = uiBufferBytes / uiBytesPerSample / uiSamplesPerMS;

    uint32_t uiPadBytes = 0;
    // The first byte the data is read to; always 0.
    uint32_t uiDataStart = 0;
    // The asm compares the duration signed.
    if (static_cast<int32_t>(iDurationInMS) <= 150) {
        // EXE-INLINED none: the looping-sound flag test (LOOP, ENVELOPE_SLOW, ENVELOPE_FAST)
        bool bShortLooping = (iSoundTypeFlags & SOUND_FLAG_LOOP) || (iSoundTypeFlags & SOUND_FLAG_ENVELOPE_SLOW) ||
            (iSoundTypeFlags & SOUND_FLAG_ENVELOPE_FAST);
        if (!bShortLooping) {
            // Not DSBCAPS_CTRLFX: the padded buffer gets no effects.
            kBufferDesc.dwFlags &= ~0x200u;
            uiPadBytes = (250 - iDurationInMS) * uiBytesPerSample * uiSamplesPerMS;
        }
    }
    // EXE-INLINED none: the looping-sound flag test (LOOP, ENVELOPE_SLOW, ENVELOPE_FAST)
    bool bLooping = (iSoundTypeFlags & SOUND_FLAG_LOOP) || (iSoundTypeFlags & SOUND_FLAG_ENVELOPE_SLOW) ||
        (iSoundTypeFlags & SOUND_FLAG_ENVELOPE_FAST);
    if (!bLooping) {
        // Compares doubles computed from the unsigned byte counts and the constant 0.20000000298023224 (0.2f widened),
        // then truncates the result to an integer.
        double dFifth = static_cast<double>(uiBufferBytes) * 0.20000000298023224;
        double dMinTail = static_cast<double>(uiBytesPerSample * 500 * uiSamplesPerMS);
        double dMaxTail = static_cast<double>(uiBytesPerSample * 1000 * uiSamplesPerMS);
        // The asm keeps the 500 ms size unless it is below the fifth (an unordered compare keeps it too).
        double dTail = dMinTail < dFifth ? dFifth : dMinTail;
        // It keeps the 1000 ms size unless that is above the result (unordered keeps it).
        dTail = dMaxTail > dTail ? dTail : dMaxTail;
        uiPadBytes += static_cast<uint32_t>(static_cast<int64_t>(dTail));
    }
    pRawBuffer = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(kChunkInfo.cksize + uiPadBytes + uiDataStart));
    if (!pRawBuffer) {
        mmioClose(pFile, 0);
        return OpenResult(false);
    }
    std::memset(pRawBuffer, 0, kChunkInfo.cksize + uiPadBytes + uiDataStart);
    _MMIOINFO kInfo;
    mmioGetInfo(pFile, &kInfo, 0);
    for (uint32_t i = uiDataStart; i < kChunkInfo.cksize; ++i) {
        // MMIO_READ.
        if (kInfo.pchNext == kInfo.pchEndRead && mmioAdvance(pFile, &kInfo, 0)) {
            mmioClose(pFile, 0);
            return OpenResult(false);
        }
        pRawBuffer[i] = *kInfo.pchNext;
        ++kInfo.pchNext;
    }
    uiBufferBytes = uiPadBytes + uiDataStart + uiBufferBytes;
    kBufferDesc.dwBufferBytes = uiBufferBytes;
    iSoundSize = kBufferDesc.dwBufferBytes;
    iDurationInMS = uiBufferBytes / uiBytesPerSample / uiSamplesPerMS;
    if ((iSoundTypeFlags & SOUND_FLAG_ENVELOPE_FAST) || (iSoundTypeFlags & SOUND_FLAG_ENVELOPE_SLOW)) {
        mmioAscend(pFile, &kChunkInfo, 0);
        // 'smpl'.
        kChunkInfo.ckid = 0x6C706D73;
        eResult = mmioDescend(pFile, &kChunkInfo, &kStartInfo, 0x10);
        if (eResult == 0x109) {
            mmioClose(pFile, 0);
            Warning(
                "Audio: Loop envelope sound (%s) missing smpl chunk. Use different file or change sound definition",
                pFilename
            );
            return OpenResult(false);
        }
        SamplerChunk kSampler;
        // mmioRead fills a raw byte buffer (HPSTR).
        mmioRead(pFile, reinterpret_cast<char*>(&kSampler), sizeof(kSampler));
        iLoopStart = kSampler.kLoop[0].uiStart;
        iLoopEnd = kSampler.kLoop[0].uiEnd;
    }
    mmioClose(pFile, 0);
    return OpenResult(true);
}

bool BSWin32GameSound::Prepare() {
    DSEFFECTDESC v7;
    uint32_t dwResultCode;
    void* v11;
    uint32_t v14;
    if (!pDirectSound8) {
        return OpenResult(false);
    }
    int32_t uiBytesPerSample = kBufferDesc.lpwfxFormat->wBitsPerSample / 8;
    if (Is3DSound() || Is2DRadiusSound()) {
        iDistanceAttenuation = 0x2710;
    }
    if (pRawBuffer) {
        if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
            (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0) {
            size_t v9 = 1024;
            pEnvelope = static_cast<BSWin32GameSound::EnvelopeBuffer*>(
                MemoryManager_Singleton.Allocate(sizeof(BSWin32GameSound::EnvelopeBuffer))
            );
            pEnvelope->uiHeadBytes = uiBytesPerSample * iLoopStart;
            pEnvelope->uiTailSize = uiBufferBytes - uiBytesPerSample * iLoopEnd;
            pEnvelope->uiMiddleSize = uiBufferBytes - (pEnvelope->uiTailSize + pEnvelope->uiHeadBytes);
            uiBufferBytes = kBufferDesc.lpwfxFormat->nAvgBytesPerSec >> 1;
            kBufferDesc.dwBufferBytes = uiBufferBytes;
            pEnvelope->uiPlayCursor = 0;
            pEnvelope->uiState = 0;
            pEnvelope->uiSize = 0;
            pEnvelope->pHeadBuf = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(pEnvelope->uiHeadBytes));
            pEnvelope->pMidBuf = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(pEnvelope->uiMiddleSize));
            pEnvelope->pTailBuf = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(v9 + pEnvelope->uiTailSize));
            void* pvSrc = (void*)pRawBuffer;
            rt::memcpy(pEnvelope->pHeadBuf, pvSrc, pEnvelope->uiHeadBytes);
            pvSrc = (void*)(static_cast<char*>(pvSrc) + pEnvelope->uiHeadBytes);
            rt::memcpy(pEnvelope->pMidBuf, pvSrc, pEnvelope->uiMiddleSize);
            pvSrc = (void*)(static_cast<char*>(pvSrc) + pEnvelope->uiMiddleSize);
            rt::memcpy(pEnvelope->pTailBuf, pvSrc, pEnvelope->uiTailSize);
            std::memset(&pEnvelope->pTailBuf[pEnvelope->uiTailSize], 0, v9);
            pEnvelope->uiTailSize += v9;
        }
        // The duration compare is signed.
        if (static_cast<int32_t>(iDurationInMS) < 150 &&
            ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_LOOP) != 0 ||
             ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
              (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0))) {
            kBufferDesc.dwFlags &= ~512u;
        }
        HRESULT hResult = pDirectSound8->CreateSoundBuffer(&kBufferDesc, &pTempSoundBuffer, nullptr);
        if (hResult < 0) {
            if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_3D) != 0 && kBufferDesc.lpwfxFormat->nChannels > 1u) {
                Warning(
                    "Audio: Attempting to play %i channel sound \"%s\" in 3D (must be mono)",
                    kBufferDesc.lpwfxFormat->nChannels, pFilename
                );
            }
            else {
                if (hResult == DSERR_BUFFERTOOSMALL) {
                    Warning("Audio: Wav file too small for DirectSound to process\"%s\" ", pFilename);
                }
            }
            if (pRawBuffer) {
                MemoryManager_Singleton.Deallocate(pRawBuffer);
                pRawBuffer = nullptr;
            }
            return OpenResult(false);
        }
        hResult = pTempSoundBuffer->QueryInterface(CLSID_IDirectSoundBuffer8, reinterpret_cast<void**>(&pSoundBuffer));
        if (pSoundBuffer) {
            if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_3D) != 0) {
                hResult =
                    pSoundBuffer->QueryInterface(CLSID_IDirectSound3DBuffer, reinterpret_cast<void**>(&p3DSoundBuffer));
                p3DSoundBuffer->SetMinDistance(1e+09f, 1);
                SetEmitterPosition(-3.4028235e+37f, -3.4028235e+37f, -3.4028235e+37f);
                fDistanceToListener = 3.4028235e+38f;
                pTempSoundBuffer->SetVolume(DSBVOLUME_MIN);
            }
        }
        v11 = nullptr;
        v14 = 0;
        hResult = pSoundBuffer->Lock(0, uiBufferBytes, &v11, &v14, nullptr, nullptr, 0);
        if (hResult < 0) {
            return OpenResult(false);
        }
        void* v12 = v11;
        bool bEnvelope = (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
            (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0;
        if (bEnvelope && (pEnvelope && pEnvelope->pHeadBuf)) {
            if (pEnvelope->uiHeadBytes <= uiBufferBytes) {
                rt::memcpy(v12, pEnvelope->pHeadBuf, pEnvelope->uiHeadBytes);
                pEnvelope->uiSize = 0;
                pEnvelope->uiPlayCursor = pEnvelope->uiHeadBytes;
                pEnvelope->uiState = 2;
            }
            else {
                rt::memcpy(v12, pEnvelope->pHeadBuf, uiBufferBytes);
                pEnvelope->uiSize = uiBufferBytes;
                pEnvelope->uiState = 1;
            }
        }
        else {
            rt::memcpy(v12, pRawBuffer, uiBufferBytes);
        }
        pSoundBuffer->Unlock(v11, v14, nullptr, 0);
        if (pRawBuffer) {
            MemoryManager_Singleton.Deallocate(pRawBuffer);
            pRawBuffer = nullptr;
        }
        SetVolume(fVolume);
    }
    // The duration is compared signed, as iDurationInMS is an int.
    if (static_cast<int32_t>(iDurationInMS) >= 150) {
        PrepareReverb();
    }
    if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_MODULATED) != 0) {
        v7.dwSize = sizeof(DSEFFECTDESC);
        v7.dwFlags = 0;
        v7.guidDSFXClass = CLSID_GUID_DSFX_STANDARD_DISTORTION;
        v7.dwReserved1 = 0;
        v7.dwReserved2 = 0;
        pSoundBuffer->SetFX(1, &v7, &dwResultCode);
        if (pSoundBuffer->GetObjectInPath(
                &CLSID_GUID_DSFX_STANDARD_DISTORTION, 0, &CLSID_IDirectSoundFXDistortion,
                reinterpret_cast<void**>(&pDistortionEffect)
            ) >= 0) {
            Setting::Info* p_uValue;
            p_uValue = &g_fFilterDistortionGain_Audio.uValue;
            kDistortionData.fGain = p_uValue->f;
            kDistortionData.fEdge = 0.0f;
            kDistortionData.fPostEQCenterFrequency = 5000.0f;
            kDistortionData.fPostEQBandwidth = 1135.0f;
            kDistortionData.fPreLowpassCutoff = 5000.0f;
            pDistortionEffect->SetAllParameters(&kDistortionData);
        }
    }
    return BSGameSound::Prepare();
}

void BSWin32GameSound::Copy(BSGameSound* apToCopy, bool abPlayable) {
    _DSCAPS v6;
    uint32_t v9;
    void* v10;
    void* v13;
    uint32_t v14;
    void* v15;
    uint32_t v16;
    void* v17;
    uint32_t v22;
    BSWin32GameSound* v8 = static_cast<BSWin32GameSound*>(apToCopy);
    if (v8->pRawBuffer) {
        v8->Prepare();
    }
    BSGameSound::Copy(apToCopy, true);
    pReverbBuffer = nullptr;
    pTempReverbBuffer = nullptr;
    p3DReverbBuffer = nullptr;
    iDirectoryHash = apToCopy->iDirectoryHash;
    uiBufferBytes = v8->uiBufferBytes;
    iSoundSize = uiBufferBytes;
    iSamplesPerSecond = v8->iSamplesPerSecond;
    iModSamplesPerSecond = v8->iModSamplesPerSecond;
    pSoundBuffer = nullptr;
    p3DSoundBuffer = nullptr;
    pDirectSound8 = v8->pDirectSound8;
    if (v8 && v8->pSoundBuffer) {
        char pcDest[256];
        if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_2D) == 0 &&
            (iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_3D) == 0) {
            iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(iSoundTypeFlags | BSGameSound::SOUND_FLAG_IS_2D);
        }
        v10 = nullptr;
        v13 = nullptr;
        v14 = 0;
        v9 = 0;
        v15 = nullptr;
        v17 = nullptr;
        v22 = 0;
        v16 = 0;
        kBufferDesc.dwSize = sizeof(DSBUFFERDESC);
        kBufferDesc.dwBufferBytes = v8->kBufferDesc.dwBufferBytes;
        v8->pSoundBuffer->GetFormat(kBufferDesc.lpwfxFormat, sizeof(WAVEFORMATEX), &v14);
        if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_2D) != 0 ||
            (kBufferDesc.lpwfxFormat && kBufferDesc.lpwfxFormat->nChannels > 1u)) {
            kBufferDesc.dwFlags = 64;
        }
        else {
            if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_3D) != 0) {
                // DSBCAPS_CTRL3D | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_MUTE3DATMAXDISTANCE.
                kBufferDesc.dwFlags = 0x30010;
            }
        }
        kBufferDesc.dwFlags |= 672u;
        uint32_t v12 = kBufferDesc.lpwfxFormat->wBitsPerSample / 8;
        uint32_t v19 = 0;
        int32_t v18 = 0;
        uint32_t v11 = iSamplesPerSecond / 1000;
        // The duration compare is signed.
        if (static_cast<int32_t>(iDurationInMS) <= 150) {
            kBufferDesc.dwFlags &= ~512u;
            if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_LOOP) == 0 &&
                ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) == 0 &&
                 (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) == 0)) {
                v19 = v11 * (v12 * (250 - iDurationInMS));
            }
        }
        uiBufferBytes += v18 + v19;
        kBufferDesc.dwBufferBytes = uiBufferBytes;
        iSoundSize = kBufferDesc.dwBufferBytes;
        iDurationInMS = uiBufferBytes / v12 / v11;
        if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_2D) != 0) {
            kBufferDesc.guid3DAlgorithm = GUID_NULL;
        }
        else {
            v6 = static_cast<BSWin32Audio*>(BSAudio::pAudioInstance)->kBufferCaps;
            if (v6.dwFreeHwMixingAllBuffers) {
                kBufferDesc.guid3DAlgorithm = CLSID_DS3DALG_HRTF_LIGHT;
            }
            else {
                kBufferDesc.guid3DAlgorithm = GUID_NULL;
            }
        }
        float v20 = static_cast<double>(iModSamplesPerSecond) / static_cast<double>(iSamplesPerSecond);
        iSamplesPerSecond = kBufferDesc.lpwfxFormat->nSamplesPerSec;
        iModSamplesPerSecond = static_cast<int32_t>(static_cast<double>(iSamplesPerSecond) * v20);
        iDurationInMS = uiBufferBytes / (static_cast<int32_t>(kBufferDesc.lpwfxFormat->wBitsPerSample) >> 3) /
            (iSamplesPerSecond / 1000);
        int32_t eError = pDirectSound8->CreateSoundBuffer(&kBufferDesc, &pTempSoundBuffer, nullptr);
        if (eError >= 0) {
            if (pTempSoundBuffer->Lock(0, 0, &v10, &v14, &v13, &v9, 2) >= 0) {
                if (v8->pSoundBuffer->Lock(0, 0, &v15, &v22, &v17, &v16, 2) >= 0) {
                    // The padded size (up to 250 ms more) is larger than the source buffer; the padding is the silence
                    // (zeros) Prepare pads a sound with.
                    uint32_t uiCopyBytes = v22 < kBufferDesc.dwBufferBytes ? v22 : kBufferDesc.dwBufferBytes;
                    rt::memcpy(v10, v15, uiCopyBytes);
                    std::memset(static_cast<uint8_t*>(v10) + uiCopyBytes, 0, kBufferDesc.dwBufferBytes - uiCopyBytes);
                    v8->pSoundBuffer->Unlock(v15, v22, v17, v16);
                }
                pTempSoundBuffer->Unlock(v10, v14, v13, v9);
                pTempSoundBuffer->QueryInterface(CLSID_IDirectSoundBuffer8, reinterpret_cast<void**>(&pSoundBuffer));
                if (pSoundBuffer) {
                    if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_3D) != 0) {
                        pSoundBuffer->QueryInterface(
                            CLSID_IDirectSound3DBuffer, reinterpret_cast<void**>(&p3DSoundBuffer)
                        );
                        if (p3DSoundBuffer) {
                            p3DSoundBuffer->SetMinDistance(1e+09f, 1);
                        }
                    }
                }
                if ((v8->iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
                    (v8->iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0) {
                    if (v8->pEnvelope) {
                        pEnvelope = static_cast<BSWin32GameSound::EnvelopeBuffer*>(
                            MemoryManager_Singleton.Allocate(sizeof(BSWin32GameSound::EnvelopeBuffer))
                        );
                        pEnvelope->uiHeadBytes = v8->pEnvelope->uiHeadBytes;
                        pEnvelope->uiTailSize = v8->pEnvelope->uiTailSize;
                        pEnvelope->uiMiddleSize = v8->pEnvelope->uiMiddleSize;
                        pEnvelope->uiPlayCursor = 0;
                        pEnvelope->uiState = 0;
                        pEnvelope->uiSize = 0;
                        pEnvelope->pHeadBuf =
                            static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(pEnvelope->uiHeadBytes));
                        pEnvelope->pMidBuf =
                            static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(pEnvelope->uiMiddleSize));
                        pEnvelope->pTailBuf =
                            static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(pEnvelope->uiTailSize));
                        rt::memcpy(pEnvelope->pHeadBuf, v8->pEnvelope->pHeadBuf, pEnvelope->uiHeadBytes);
                        rt::memcpy(pEnvelope->pMidBuf, v8->pEnvelope->pMidBuf, pEnvelope->uiMiddleSize);
                        rt::memcpy(pEnvelope->pTailBuf, v8->pEnvelope->pTailBuf, pEnvelope->uiTailSize);
                    }
                }
                iStateFlags = 0;
                OpenResult(true);
                if (abPlayable) {
                    Prepare();
                }
            }
            return;
        }
        // The DirectSound HRESULTs, compared signed: 0x8878001E is DSERR_CONTROLUNAVAIL.
        if (eError > static_cast<int32_t>(0x8878001Eu)) {
            switch (eError) {
                case -0x7787FFCE: {
                    strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_INVALIDCALL\n");
                    break;
                }
                case -0x7787FF9C: {
                    strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_BADFORMAT\n");
                    break;
                }
                case -0x7787FF56: {
                    strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_UNINITIALIZED\n");
                    break;
                }
                case -0x7787FF4C: {
                    strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_BUFFERTOOSMALL\n");
                    break;
                }
                case -0x7787FF42: {
                    strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_DS8_REQUIRED\n");
                    break;
                }
                default: {
                    goto LABEL_51;
                }
            }
            goto LABEL_52;
        }
        if (eError == static_cast<int32_t>(0x8878001Eu)) {
            strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_CONTROLUNAVAIL\n");
        }
        else {
            if (eError > -0x7FF8FFF2) {
                if (eError == static_cast<int32_t>(0x80070057u)) {
                    strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_INVALIDPARAM\n");
                }
                else {
                    if (eError != static_cast<int32_t>(0x8878000Au)) {
                        goto LABEL_51;
                    }
                    strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_ALLOCATED\n");
                }
            }
            else {
                switch (eError) {
                    case -0x7FF8FFF2: {
                        strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_OUTOFMEMORY\n");
                        break;
                    }
                    case -0x7FFFBFFF: {
                        strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_UNSUPPORTED\n");
                        break;
                    }
                    case -0x7FFBFEF0: {
                        strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_NOAGGREGATION\n");
                        break;
                    }
                    default: {
                    LABEL_51:;
                        strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer - unknown warning\n");
                        break;
                    }
                }
            }
        }
    LABEL_52:;
        Warning(pcDest);
    }
}

bool BSWin32GameSound::Play(bool abLoopPlayback) {
    int32_t eResult = E_FAIL;
    if (pSoundBuffer) {
        if (pDirectSound8) {
            if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_LOOP) != 0) {
                abLoopPlayback = true;
                uint32_t iSoundSize = this->iSoundSize;
                uint32_t v2 = rt::rand();
                pSoundBuffer->SetCurrentPosition(v2 % (iSoundSize - 1));
            }
            else {
                if (((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
                     (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0) &&
                    (!IsPaused() || (!pEnvelope || pEnvelope->uiState != 5))) {
                    abLoopPlayback = true;
                    if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
                        (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0) {
                        if (pEnvelope) {
                            if (pEnvelope->pHeadBuf) {
                                if (pEnvelope->uiState != 1 && pEnvelope->uiState != 2) {
                                    pSoundBuffer->SetCurrentPosition(0);
                                    if (pEnvelope->uiHeadBytes <= uiBufferBytes) {
                                        uint32_t dwWriteBytes = uiBufferBytes - pEnvelope->uiHeadBytes;
                                        Write(
                                            0, 0, pEnvelope->uiHeadBytes, reinterpret_cast<char*>(pEnvelope->pHeadBuf)
                                        );
                                        Write(
                                            0, pEnvelope->uiHeadBytes, dwWriteBytes,
                                            reinterpret_cast<char*>(pEnvelope->pMidBuf)
                                        );
                                        pEnvelope->uiSize = dwWriteBytes + pEnvelope->uiHeadBytes;
                                        pEnvelope->uiState = 2;
                                    }
                                    else {
                                        Write(0, 0, uiBufferBytes, reinterpret_cast<char*>(pEnvelope->pHeadBuf));
                                        pEnvelope->uiSize = uiBufferBytes;
                                        pEnvelope->uiState = 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (abLoopPlayback) {
                eResult = pSoundBuffer->Play(0, 0, 1);
                if (pReverbBuffer) {
                    pReverbBuffer->Play(0, 0, 1);
                }
            }
            else {
                eResult = pSoundBuffer->Play(0, 0, 0);
                if (pReverbBuffer) {
                    pReverbBuffer->Play(0, 0, 0);
                }
            }
            HandlePlayCallback();
        }
    }
    if (eResult >= 0) {
        iStateFlags |= BSGameSound::STATE_PLAYING;
        iStateFlags &= ~BSGameSound::STATE_PAUSED;
    }
    SetVolume(fVolume);
    return eResult >= 0;
}

bool BSWin32GameSound::Pause() {
    HRESULT eResult = E_FAIL;
    if (pSoundBuffer) {
        if (pDirectSound8) {
            eResult = pSoundBuffer->Stop();
            if (pReverbBuffer) {
                pReverbBuffer->Stop();
            }
        }
    }
    if (eResult >= 0) {
        iStateFlags |= BSGameSound::STATE_PAUSED;
        iStateFlags &= ~BSGameSound::STATE_PLAYING;
    }
    return eResult >= 0;
}

bool BSWin32GameSound::Stop() {
    int32_t eResult = E_FAIL;
    if (pSoundBuffer) {
        if (pDirectSound8) {
            bool v4 = (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
                (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0;
            if (!v4 || (!pEnvelope || (pEnvelope->uiState == 5 || pEnvelope->uiState == 6))) {
                bool v3 = (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
                    (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0;
                if (!v3 || !pEnvelope) {
                    eResult = pSoundBuffer->Stop();
                    pSoundBuffer->SetCurrentPosition(0);
                    if (pReverbBuffer) {
                        pReverbBuffer->Stop();
                        pReverbBuffer->SetCurrentPosition(0);
                    }
                    if (eResult >= 0) {
                        iStateFlags &= ~BSGameSound::STATE_PLAYING;
                    }
                }
            }
            else {
                pEnvelope->uiState = 4;
            }
            if ((iStateFlags & BSGameSound::STATE_PAUSED) != 0) {
                iStateFlags &= ~BSGameSound::STATE_PAUSED;
            }
        }
    }
    return eResult >= 0;
}

bool BSWin32GameSound::SetVolume(float afNewVolume) {
    int32_t iReverbVolume;
    int32_t v6;
    int32_t iNormalVolume;
    int32_t v8;
    float v10;
    float v11;
    float v12;
    fVolume = afNewVolume;
    float fReverbAttenuationa = static_cast<float>(iReverbAttenuation);
    float fReverbAttenuation = fReverbAttenuationa / 100.0;
    if (!pSoundBuffer) {
        return false;
    }
    if (!(afNewVolume > 1.0)) {
        v12 = afNewVolume;
    }
    else {
        v12 = 1.0f;
    }
    if (!(v12 < 0.0)) {
        v11 = v12;
    }
    else {
        v11 = 0.0f;
    }
    BSGameSound::SoundFlags flags_4 = iSoundTypeFlags;
    BSAudioManager* v2 = &BSAudioManager::QInstance();
    float afVolume = v2->GetCorrectedVolume(v11, flags_4);
    if ((iStateFlags & BSGameSound::STATE_FADING) == 0 && (iStateFlags & BSGameSound::STATE_PLAY_LATER) != 0 &&
        (iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) == 0) {
        afVolume = 1e-05f;
    }
    if (!(afVolume < 9.999999747378752e-06)) {
        v10 = afVolume;
    }
    else {
        v10 = 1e-05f;
    }
    float v9 = std::log10(static_cast<double>(v10));
    int32_t iFinalVolume =
        static_cast<int32_t>(v9 * 2000.0) - (iFaderAttenuation + (iSystemAttenuation + iStaticAttenuation));
    if (Is3DSound() || Is2DRadiusSound()) {
        iFinalVolume -= iDistanceAttenuation;
    }
    if (iFinalVolume <= DSBVOLUME_MIN) {
        v8 = DSBVOLUME_MIN;
    }
    else {
        v8 = iFinalVolume;
    }
    if (v8 >= -1) {
        iNormalVolume = -1;
    }
    else {
        iNormalVolume = v8;
    }
    if (static_cast<int32_t>(static_cast<int64_t>(fReverbAttenuation * -10000.0) + iNormalVolume) <= DSBVOLUME_MIN) {
        v6 = DSBVOLUME_MIN;
    }
    else {
        v6 = static_cast<int64_t>(fReverbAttenuation * -10000.0) + iNormalVolume;
    }
    if (v6 >= -1) {
        iReverbVolume = -1;
    }
    else {
        iReverbVolume = v6;
    }
    if ((iStateFlags & BSGameSound::STATE_MUTED) != 0) {
        if (pReverbBuffer) {
            pReverbBuffer->SetVolume(DSBVOLUME_MIN);
        }
        return pSoundBuffer->SetVolume(DSBVOLUME_MIN) >= 0;
    }
    else {
        int32_t v14 = E_FAIL;
        if (pReverbBuffer) {
            pReverbBuffer->SetVolume(iReverbVolume);
        }
        if (pSoundBuffer) {
            v14 = pSoundBuffer->SetVolume(iNormalVolume);
        }
        return v14 >= 0;
    }
}

void BSWin32GameSound::UpdateEmitterPosition() {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    NiPoint3 kPos = NiPoint3_ZERO;
    if (Is3DSound() || Is2DRadiusSound()) {
        if (BSAudio::pAudioInstance->pListener) {
            if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_FIRST_PERSON) != 0) {
                BSAudioListener* pListener = BSAudio::pAudioInstance->pListener;
                NiPoint3 kListenerPos = pListener->GetPosition();
                x = kListenerPos.x;
                y = kListenerPos.y;
                z = kListenerPos.z;
                // DirectSound's y is Gamebryo's z, so the position is passed as x, z, y.
                if (p3DSoundBuffer) {
                    float v18 = y * 0.014287755824625492;
                    float v17 = z * 0.014287755824625492;
                    float v16 = x * 0.014287755824625492;
                    p3DSoundBuffer->SetPosition(v16, v17, v18, 1);
                    if (p3DReverbBuffer) {
                        float v15 = y * 0.014287755824625492;
                        float v14 = z * 0.014287755824625492;
                        float v13 = x * 0.014287755824625492;
                        p3DReverbBuffer->SetPosition(v13, v14, v15, 1);
                    }
                }
            }
            else {
                if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_BEAM_EMITTER) != 0 &&
                    (iStateFlags & BSGameSound::STATE_IS_VALID_BEAM) != 0) {
                    float v33 = 0.0;
                    float v32 = 0.0;
                    float v31 = 0.0;
                    NiPoint3 v35 = BSAudio::pAudioInstance->pListener->GetPosition();
                    NiPoint3 pt = vBeamEnd - kPosition;
                    NiPoint3 v30 = v35 - kPosition;
                    v33 = v30.Dot(pt);
                    if (!(v33 <= 0.0)) {
                        v32 = pt.Dot(pt);
                        if (!(v33 >= static_cast<double>(v32))) {
                            v31 = v33 / v32;
                            kPos = kPosition + v31 * pt;
                        }
                        else {
                            kPos = vBeamEnd;
                        }
                    }
                    else {
                        kPos = kPosition;
                    }
                    if (p3DSoundBuffer) {
                        float v12 = kPos.y * 0.014287755824625492;
                        float v11 = kPos.z * 0.014287755824625492;
                        float v10 = kPos.x * 0.014287755824625492;
                        p3DSoundBuffer->SetPosition(v10, v11, v12, 1);
                        if (p3DReverbBuffer) {
                            float v9 = kPos.y * 0.014287755824625492;
                            float v8 = kPos.z * 0.014287755824625492;
                            float v7 = kPos.x * 0.014287755824625492;
                            p3DReverbBuffer->SetPosition(v7, v8, v9, 1);
                        }
                    }
                }
            }
            float fListenerDistance = fDistanceToListener;
            if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_BEAM_EMITTER) != 0 &&
                ((iStateFlags & BSGameSound::STATE_IS_VALID_BEAM) != 0 && BSAudio::pAudioInstance->pListener)) {
                NiPoint3* v2 = &BSAudio::pAudioInstance->pListener->GetPosition();
                fListenerDistance = (kPos - *v2).Length();
                fDistanceToListener = fListenerDistance;
            }
            else {
                if (BSAudio::pAudioInstance->pListener) {
                    NiPoint3* v4 = &BSAudio::pAudioInstance->pListener->GetPosition();
                    fListenerDistance = (kPosition - *v4).Length();
                    fDistanceToListener = fListenerDistance;
                }
            }
            float v20 = fMaxDist - fMinDist;
            float v6 = std::fabs(static_cast<double>(v20));
            if (!(v6 > 10.0) || !(fListenerDistance < fMaxDist * 1.100000023841858)) {
                if (!(fMaxDist <= static_cast<double>(fListenerDistance))) {
                    iDistanceAttenuation = 0;
                }
                else {
                    iDistanceAttenuation = 0x2710;
                }
                SetVolume(fVolume);
            }
            else {
                float fAttenStart = (fMaxDist - fMinDist) * 0.25 + fMinDist;
                float fAttenMid = (fMaxDist - fMinDist) * 0.5 + fMinDist;
                float fAttenEnd = (fMaxDist - fMinDist) * 0.75 + fMinDist;
                if (!(fMaxDist <= static_cast<double>(fListenerDistance))) {
                    if (!(fAttenEnd < static_cast<double>(fListenerDistance)) || !(fMaxDist - fAttenEnd > 0.0)) {
                        if (!(fAttenMid < static_cast<double>(fListenerDistance)) || !(fAttenEnd - fAttenMid > 0.0)) {
                            if (!(fAttenStart < static_cast<double>(fListenerDistance)) ||
                                !(fAttenMid - fAttenStart > 0.0)) {
                                if (!(fMinDist < static_cast<double>(fListenerDistance)) ||
                                    !(fAttenStart - fMinDist > 0.0)) {
                                    iDistanceAttenuation = 0;
                                    SetVolume(fVolume);
                                }
                                else {
                                    iDistanceAttenuation = static_cast<int32_t>(
                                        (fListenerDistance - fMinDist) / (fAttenStart - fMinDist) *
                                            static_cast<double>(usAttenuationCurve[1] - usAttenuationCurve[0]) +
                                        static_cast<double>(usAttenuationCurve[0])
                                    );
                                    SetVolume(fVolume);
                                }
                            }
                            else {
                                iDistanceAttenuation = static_cast<int32_t>(
                                    (fListenerDistance - fAttenStart) / (fAttenMid - fAttenStart) *
                                        static_cast<double>(usAttenuationCurve[2] - usAttenuationCurve[1]) +
                                    static_cast<double>(usAttenuationCurve[1])
                                );
                                SetVolume(fVolume);
                            }
                        }
                        else {
                            iDistanceAttenuation = static_cast<int32_t>(
                                (fListenerDistance - fAttenMid) / (fAttenEnd - fAttenMid) *
                                    static_cast<double>(usAttenuationCurve[3] - usAttenuationCurve[2]) +
                                static_cast<double>(usAttenuationCurve[2])
                            );
                            SetVolume(fVolume);
                        }
                    }
                    else {
                        iDistanceAttenuation = static_cast<int32_t>(
                            (fListenerDistance - fAttenEnd) / (fMaxDist - fAttenEnd) *
                                static_cast<double>(usAttenuationCurve[4] - usAttenuationCurve[3]) +
                            static_cast<double>(usAttenuationCurve[3])
                        );
                        SetVolume(fVolume);
                    }
                }
                else {
                    iDistanceAttenuation = 0x2710;
                    SetVolume(fVolume);
                }
            }
            SetVolume(fVolume);
        }
    }
}

bool BSWin32GameSound::Update(uint32_t adwTimeDelta) {
    bool v3;
    uint32_t eStateFlags;
    uint32_t uiCurrPos;
    WAVEFORMATEX kFormat;
    uint32_t dwWriteBytes;
    uint32_t uiWriteCursor;
    char bResetPlayCursor;
    int32_t uiPlayCursor;
    int32_t eStatus;
    if (!pSoundBuffer || !pDirectSound8) {
        goto LABEL_68;
    }
    pSoundBuffer->GetStatus(reinterpret_cast<uint32_t*>(&eStatus));
    if ((eStatus & 1) != 0) {
        eStateFlags = iStateFlags | BSGameSound::STATE_PLAYING;
    }
    else {
        eStateFlags = iStateFlags & ~BSGameSound::STATE_PLAYING;
    }
    iStateFlags = eStateFlags;
    if (!IsPlaying() ||
        ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
                 (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0
             ? v3 = true
             : v3 = false,
         !v3 || !pEnvelope)) {
        if (IsPlaying() && (iSoundTypeFlags & BSGameSound::SOUND_FLAG_EXTERNAL_SYNC) != 0) {
            if ((iStateFlags & BSGameSound::STATE_EXPLICIT_PAUSE) != 0 ||
                (BSAudio::pAudioInstance->SynchTimer.iFlags & 1) == 0) {
                if ((iStateFlags & BSGameSound::STATE_EXPLICIT_PAUSE) != 0 &&
                    (BSAudio::pAudioInstance->SynchTimer.iFlags & 1) == 0) {
                    iStateFlags &= ~BSGameSound::STATE_EXPLICIT_PAUSE;
                    Play(false);
                }
            }
            else {
                iStateFlags |= BSGameSound::STATE_EXPLICIT_PAUSE;
                Pause();
            }
            if ((iStateFlags & BSGameSound::STATE_EXPLICIT_PAUSE) == 0) {
                std::memset(&kFormat, 0, sizeof(kFormat));
                uiCurrPos = 0;
                bool v11 = false;
                pSoundBuffer->GetCurrentPosition(&uiCurrPos, nullptr);
                pSoundBuffer->GetFormat(&kFormat, sizeof(WAVEFORMATEX), nullptr);
                int32_t uiOffset = static_cast<int32_t>(static_cast<double>(kFormat.nAvgBytesPerSec) / 1000.0);
                uint32_t v12 = static_cast<int64_t>(
                    static_cast<double>(uiCurrPos) / (static_cast<double>(kFormat.nAvgBytesPerSec) / 1000.0)
                );
                uint32_t uiDelta = BSAudio::pAudioInstance->SynchTimer.iMilliseconds - iSynchStartTime;
                int32_t iTimeDiff = 0;
                if (v12 <= uiDelta) {
                    if (v12 < uiDelta) {
                        iTimeDiff = uiDelta - v12;
                    }
                }
                else {
                    iTimeDiff = uiDelta - v12;
                }
                if (iTimeDiff > 50 || -iTimeDiff > 50) {
                    int32_t iNewTime = uiCurrPos + uiOffset * iTimeDiff;
                    // The end position is treated as signed.
                    if (!(static_cast<double>(static_cast<int32_t>(uiOffset * iDurationInMS)) * 1.100000023841858 <
                          static_cast<double>(iNewTime))) {
                        if (iNewTime <= static_cast<int32_t>(uiOffset * iDurationInMS)) {
                            if (iNewTime < 0) {
                                iNewTime = 0;
                            }
                        }
                        else {
                            iNewTime = uiOffset * iDurationInMS;
                        }
                    }
                    else {
                        iCleanupDelayInMS = 100;
                        iSynchStartTime = 0;
                        iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(
                            iSoundTypeFlags & ~BSGameSound::SOUND_FLAG_EXTERNAL_SYNC
                        );
                        Stop();
                        v11 = true;
                    }
                    if (!v11) {
                        pSoundBuffer->SetCurrentPosition(iNewTime);
                    }
                }
            }
        }
        goto LABEL_68;
    }
    uiPlayCursor = 0;
    uiWriteCursor = 0;
    bResetPlayCursor = 0;
    dwWriteBytes = 0;
    pSoundBuffer->GetCurrentPosition(reinterpret_cast<uint32_t*>(&uiPlayCursor), &uiWriteCursor);
    if (pEnvelope->uiPlayCursor <= uiPlayCursor) {
        if (pEnvelope->uiPlayCursor >= uiPlayCursor) {
            return false;
        }
        dwWriteBytes = uiPlayCursor - pEnvelope->uiPlayCursor;
    }
    else {
        bResetPlayCursor = 1;
        dwWriteBytes = uiBufferBytes - pEnvelope->uiPlayCursor;
    }
    if (pEnvelope->uiState == 1) {
        if (dwWriteBytes >= pEnvelope->uiHeadBytes - pEnvelope->uiSize) {
            dwWriteBytes = pEnvelope->uiHeadBytes - pEnvelope->uiSize;
            Write(
                pEnvelope->uiSize, pEnvelope->uiPlayCursor, dwWriteBytes, reinterpret_cast<char*>(pEnvelope->pHeadBuf)
            );
            pEnvelope->uiSize = 0;
            pEnvelope->uiState = 2;
        }
        else {
            Write(
                pEnvelope->uiSize, pEnvelope->uiPlayCursor, dwWriteBytes, reinterpret_cast<char*>(pEnvelope->pHeadBuf)
            );
            pEnvelope->uiSize += dwWriteBytes;
        }
    LABEL_42:;
        if (bResetPlayCursor) {
            pEnvelope->uiPlayCursor = 0;
        }
        else {
            pEnvelope->uiPlayCursor = uiPlayCursor;
        }
    LABEL_68:;
        UpdateEmitterPosition();
        return true;
    }
    if (pEnvelope->uiState == 2 && pEnvelope->uiMiddleSize) {
        bool v15 = false;
        int32_t auiSize = 0;
        if (dwWriteBytes > pEnvelope->uiMiddleSize - pEnvelope->uiSize) {
            auiSize = dwWriteBytes - (pEnvelope->uiMiddleSize - pEnvelope->uiSize);
            dwWriteBytes = pEnvelope->uiMiddleSize - pEnvelope->uiSize;
            v15 = true;
        }
        Write(pEnvelope->uiSize, pEnvelope->uiPlayCursor, dwWriteBytes, reinterpret_cast<char*>(pEnvelope->pMidBuf));
        pEnvelope->uiSize = (dwWriteBytes + pEnvelope->uiSize) % pEnvelope->uiMiddleSize;
        if (v15) {
            pEnvelope->uiPlayCursor += dwWriteBytes;
            Write(pEnvelope->uiSize, pEnvelope->uiPlayCursor, auiSize, reinterpret_cast<char*>(pEnvelope->pMidBuf));
            pEnvelope->uiSize = auiSize;
        }
        goto LABEL_42;
    }
    if (pEnvelope->uiState != 4) {
        if (pEnvelope->uiState == 5) {
            if (dwWriteBytes >= pEnvelope->uiTailSize - pEnvelope->uiSize) {
                pSoundBuffer->Stop();
                dwWriteBytes = pEnvelope->uiTailSize - pEnvelope->uiSize;
                Write(
                    pEnvelope->uiSize, pEnvelope->uiPlayCursor, dwWriteBytes,
                    reinterpret_cast<char*>(pEnvelope->pTailBuf)
                );
                pSoundBuffer->Play(0, 0, 0);
                pEnvelope->uiState = 6;
                iStateFlags &= ~BSGameSound::STATE_PLAYING;
                iStateFlags &= ~BSGameSound::STATE_PAUSED;
            }
            else {
                Write(
                    pEnvelope->uiSize, pEnvelope->uiPlayCursor, dwWriteBytes,
                    reinterpret_cast<char*>(pEnvelope->pTailBuf)
                );
                pEnvelope->uiSize += dwWriteBytes;
            }
        }
        goto LABEL_42;
    }
    bool v14 = false;
    if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) == 0) {
        if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0) {
            pEnvelope->uiState = 3;
            pSoundBuffer->Stop();
            pSoundBuffer->Play(0, 0, 0);
        }
        goto LABEL_42;
    }
    pEnvelope->uiSize = 0;
    bResetPlayCursor = 1;
    if (uiBufferBytes <= pEnvelope->uiTailSize) {
        pSoundBuffer->Stop();
        pSoundBuffer->SetCurrentPosition(0);
        dwWriteBytes = uiBufferBytes;
        Write(0, 0, dwWriteBytes, reinterpret_cast<char*>(pEnvelope->pTailBuf));
        pSoundBuffer->Play(0, 0, 1);
        pEnvelope->uiState = 5;
        pEnvelope->uiSize += dwWriteBytes;
    }
    else {
        pSoundBuffer->Stop();
        pSoundBuffer->SetCurrentPosition(0);
        dwWriteBytes = pEnvelope->uiTailSize;
        Write(0, 0, dwWriteBytes, reinterpret_cast<char*>(pEnvelope->pTailBuf));
        Write(0, dwWriteBytes, uiBufferBytes - dwWriteBytes, nullptr);
        pEnvelope->uiState = 6;
        pSoundBuffer->Play(0, 0, 0);
    }
    pEnvelope->uiPlayCursor = 0;
    return false;
}

bool BSWin32GameSound::ProcessDataRequest(int32_t aiRequestNum) {
    return false;
}

bool BSWin32GameSound::SetEmitterPosition(NiPoint3 vin) {
    return SetEmitterPosition(vin.x, vin.y, vin.z);
}

bool BSWin32GameSound::SetEmitterPosition(float afX, float afY, float afZ) {
    NiPoint3 kPos = NiPoint3_ZERO;
    NiPoint3 kListenerPos = NiPoint3_ZERO;
    bool bPositionChanged = false;
    bool v4 = Is3DSound();
    if (v4 || (v4 = Is2DRadiusSound())) {
        if (pDirectSound8) {
            if (BSAudio::pAudioInstance->pListener) {
                BSAudioListener* pListener = BSAudio::pAudioInstance->pListener;
                kListenerPos = pListener->GetPosition();
                if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_FIRST_PERSON) != 0) {
                    afX = kListenerPos.x;
                    afY = kListenerPos.y;
                    afZ = kListenerPos.z;
                }
                if (afX != kPosition.x || (afY != kPosition.y || afZ != kPosition.z)) {
                    kPosition.x = afX;
                    kPosition.y = afY;
                    kPosition.z = afZ;
                    bPositionChanged = true;
                }
            }
            padding224 = 1;
            if (p3DSoundBuffer) {
                if (((iSoundTypeFlags & BSGameSound::SOUND_FLAG_BEAM_EMITTER) != 0 &&
                     (iStateFlags & BSGameSound::STATE_IS_VALID_BEAM) != 0) ||
                    !bPositionChanged) {
                    if (BSAudio::pAudioInstance->pListener) {
                        if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_BEAM_EMITTER) != 0 &&
                            (iStateFlags & BSGameSound::STATE_IS_VALID_BEAM) != 0) {
                            float v37 = 0.0;
                            float v36 = 0.0;
                            float v35 = 0.0;
                            NiPoint3 v39 = BSAudio::pAudioInstance->pListener->GetPosition();
                            NiPoint3 pt = vBeamEnd - kPosition;
                            NiPoint3 v34 = v39 - kPosition;
                            v37 = v34.Dot(pt);
                            if (!(v37 <= 0.0)) {
                                v36 = pt.Dot(pt);
                                if (!(v37 >= static_cast<double>(v36))) {
                                    v35 = v37 / v36;
                                    kPos = kPosition + v35 * pt;
                                }
                                else {
                                    kPos = vBeamEnd;
                                }
                            }
                            else {
                                kPos = kPosition;
                            }
                            // DirectSound's y is the game's z: the position is passed as x, z, y.
                            float v15 = kPos.y * 0.014287755824625492;
                            float v14 = kPos.z * 0.014287755824625492;
                            float v13 = kPos.x * 0.014287755824625492;
                            p3DSoundBuffer->SetPosition(v13, v14, v15, 1);
                            if (p3DReverbBuffer) {
                                float v12 = kPos.y * 0.014287755824625492;
                                float v11 = kPos.z * 0.014287755824625492;
                                float v10 = kPos.x * 0.014287755824625492;
                                p3DReverbBuffer->SetPosition(v10, v11, v12, 1);
                            }
                        }
                    }
                }
                else {
                    float v21 = afY * 0.014287755824625492;
                    float v20 = afZ * 0.014287755824625492;
                    float v19 = afX * 0.014287755824625492;
                    p3DSoundBuffer->SetPosition(v19, v20, v21, 1);
                    if (p3DReverbBuffer) {
                        float v18 = afY * 0.014287755824625492;
                        float v17 = afZ * 0.014287755824625492;
                        float v16 = afX * 0.014287755824625492;
                        p3DReverbBuffer->SetPosition(v16, v17, v18, 1);
                    }
                }
            }
            float v24 = (kListenerPos - kPosition).Length();
            this->fDistanceToListener = v24;
            float fDistanceToListener = this->fDistanceToListener;
            if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_BEAM_EMITTER) != 0 &&
                (iStateFlags & BSGameSound::STATE_IS_VALID_BEAM) != 0) {
                if (BSAudio::pAudioInstance->pListener) {
                    NiPoint3* v7 = &BSAudio::pAudioInstance->pListener->GetPosition();
                    fDistanceToListener = (kPos - *v7).Length();
                }
            }
            float v23 = fMaxDist - fMinDist;
            float v9 = std::fabs(static_cast<double>(v23));
            if (!(v9 > 10.0)) {
                if (!(fMaxDist <= static_cast<double>(fDistanceToListener))) {
                    iDistanceAttenuation = 0;
                }
                else {
                    iDistanceAttenuation = 10000;
                }
                SetVolume(fVolume);
            }
            else {
                float v33 = (fMaxDist - fMinDist) * 0.25 + fMinDist;
                float v32 = (fMaxDist - fMinDist) * 0.5 + fMinDist;
                float v31 = (fMaxDist - fMinDist) * 0.75 + fMinDist;
                if (!(fMaxDist <= static_cast<double>(fDistanceToListener))) {
                    if (!(v31 < static_cast<double>(fDistanceToListener)) || !(fMaxDist - v31 > 0.0)) {
                        if (!(v32 < static_cast<double>(fDistanceToListener)) || !(v31 - v32 > 0.0)) {
                            if (!(v33 < static_cast<double>(fDistanceToListener)) || !(v32 - v33 > 0.0)) {
                                if (!(fMinDist < static_cast<double>(fDistanceToListener)) || !(v33 - fMinDist > 0.0)) {
                                    iDistanceAttenuation = 0;
                                    SetVolume(fVolume);
                                }
                                else {
                                    iDistanceAttenuation = static_cast<int32_t>(
                                        (fDistanceToListener - fMinDist) / (v33 - fMinDist) *
                                            static_cast<double>(usAttenuationCurve[1] - usAttenuationCurve[0]) +
                                        static_cast<double>(usAttenuationCurve[0])
                                    );
                                    SetVolume(fVolume);
                                }
                            }
                            else {
                                iDistanceAttenuation = static_cast<int32_t>(
                                    (fDistanceToListener - v33) / (v32 - v33) *
                                        static_cast<double>(usAttenuationCurve[2] - usAttenuationCurve[1]) +
                                    static_cast<double>(usAttenuationCurve[1])
                                );
                                SetVolume(fVolume);
                            }
                        }
                        else {
                            iDistanceAttenuation = static_cast<int32_t>(
                                (fDistanceToListener - v32) / (v31 - v32) *
                                    static_cast<double>(usAttenuationCurve[3] - usAttenuationCurve[2]) +
                                static_cast<double>(usAttenuationCurve[2])
                            );
                            SetVolume(fVolume);
                        }
                    }
                    else {
                        iDistanceAttenuation = static_cast<int32_t>(
                            (fDistanceToListener - v31) / (fMaxDist - v31) *
                                static_cast<double>(usAttenuationCurve[4] - usAttenuationCurve[3]) +
                            static_cast<double>(usAttenuationCurve[3])
                        );
                        SetVolume(fVolume);
                    }
                }
                else {
                    iDistanceAttenuation = 10000;
                    SetVolume(fVolume);
                }
            }
            return SetVolume(fVolume);
        }
    }
    return v4;
}

// A 3D or 2D-radius sound reports its emitter position; any other sound the origin.
NiPoint3 BSWin32GameSound::GetEmitterPosition() {
    NiPoint3 kResult;
    if (Is3DSound() || Is2DRadiusSound()) {
        kResult = kPosition;
    }
    else {
        kResult.x = 0.0f;
        kResult.y = 0.0f;
        kResult.z = 0.0f;
    }
    return kResult;
}

// DirectSound sounds have no emitter orientation, so this does nothing.
void BSWin32GameSound::SetEmitterOrientation(float afX, float afY, float afZ) {}

void BSWin32GameSound::SetMinMax(float afMin, float afMax) {
    if (fMaxDist != afMax || fMinDist != afMin) {
        fMaxDist = afMax;
        fMinDist = afMin;
        if (padding224) {
            SetEmitterPosition(kPosition);
        }
    }
    if (Is3DSound()) {
        if (p3DSoundBuffer) {
            if (pDirectSound8) {
                p3DSoundBuffer->SetMinDistance(1e+09f, 0);
                if (!(afMin > 0.0)) {
                    fMinDist = 69.99f;
                }
                else {
                    fMinDist = afMin;
                }
                p3DSoundBuffer->SetMaxDistance(1e+09f, 0);
                if (!(afMax > 0.0)) {
                    fMaxDist = 1e+09f;
                }
                else {
                    fMaxDist = afMax;
                }
            }
        }
    }
}

void BSWin32GameSound::SetAttenuationCurve(
    uint16_t aiPt1,
    uint16_t aiPt2,
    uint16_t aiPt3,
    uint16_t aiPt4,
    uint16_t aiPt5
) {
    double v6;
    double v9;
    double v12;
    double v15;
    double v18;
    if (aiPt1) {
        float v20 = static_cast<double>(aiPt1) / 100.0;
        float v19 = std::log10(static_cast<double>(v20));
        v18 = v19 * -2000.0;
    }
    else {
        v18 = 5000.0;
    }
    usAttenuationCurve[0] = static_cast<int32_t>(v18);
    if (aiPt2) {
        float v17 = static_cast<double>(aiPt2) / 100.0;
        float v16 = std::log10(static_cast<double>(v17));
        v15 = v16 * -2000.0;
    }
    else {
        v15 = 5000.0;
    }
    usAttenuationCurve[1] = static_cast<int32_t>(v15);
    if (aiPt3) {
        float v14 = static_cast<double>(aiPt3) / 100.0;
        float v13 = std::log10(static_cast<double>(v14));
        v12 = v13 * -2000.0;
    }
    else {
        v12 = 5000.0;
    }
    usAttenuationCurve[2] = static_cast<int32_t>(v12);
    if (aiPt4) {
        float v11 = static_cast<double>(aiPt4) / 100.0;
        float v10 = std::log10(static_cast<double>(v11));
        v9 = v10 * -2000.0;
    }
    else {
        v9 = 5000.0;
    }
    usAttenuationCurve[3] = static_cast<int32_t>(v9);
    if (aiPt5) {
        float v8 = static_cast<double>(aiPt5) / 100.0;
        float v7 = std::log10(static_cast<double>(v8));
        v6 = v7 * -2000.0;
    }
    else {
        v6 = 5000.0;
    }
    usAttenuationCurve[4] = static_cast<int32_t>(v6);
}

bool BSWin32GameSound::SetFrequency(float afNewFrequency) {
    int32_t eResult = E_FAIL;
    uint32_t uiFreq = static_cast<int64_t>(static_cast<double>(iModSamplesPerSecond) * afNewFrequency);
    if (uiFreq >= 100) {
        if (uiFreq > DSBFREQUENCY_MAX) {
            uiFreq = DSBFREQUENCY_MAX;
        }
    }
    else {
        uiFreq = 100;
    }
    if (pSoundBuffer) {
        if (pDirectSound8) {
            eResult = pSoundBuffer->SetFrequency(uiFreq);
        }
    }
    return eResult >= 0;
}

float BSWin32GameSound::GetFrequency() {
    uint32_t uiFrequency;
    HRESULT eResult = E_FAIL;
    uiFrequency = 0;
    float fResult = 1.0f;
    if (pSoundBuffer) {
        if (pDirectSound8) {
            eResult = pSoundBuffer->GetFrequency(&uiFrequency);
        }
    }
    if (eResult >= 0) {
        return static_cast<double>(uiFrequency) / static_cast<double>(iModSamplesPerSecond);
    }
    return fResult;
}

// The duration is read as a signed 32-bit value and nAvgBytesPerSec as unsigned, and the bytes-per-millisecond quotient
// is truncated.
void BSWin32GameSound::Seek(uint32_t uiMilliseconds) {
    tWAVEFORMATEX kFormat;
    if (pSoundBuffer && uiMilliseconds < iDurationInMS) {
        std::memset(&kFormat, 0, sizeof(kFormat));
        pSoundBuffer->GetFormat(&kFormat, sizeof(WAVEFORMATEX), nullptr);
        pSoundBuffer->SetCurrentPosition(
            static_cast<int32_t>(static_cast<double>(kFormat.nAvgBytesPerSec) / 1000.0) * uiMilliseconds
        );
    }
    else {
        Warning(
            "Attempted to seek to %.2fs in a sound of only %.2fs duration.",
            static_cast<double>(uiMilliseconds) / 1000.0,
            static_cast<double>(static_cast<int32_t>(iDurationInMS)) / 1000.0
        );
    }
}

bool BSWin32GameSound::SetDirectSound() {
    if (pDirectSound8) {
        return true;
    }
    if (!static_cast<BSWin32Audio*>(BSAudio::pAudioInstance)->pDirectSound8) {
        return false;
    }
    pDirectSound8 = static_cast<BSWin32Audio*>(BSAudio::pAudioInstance)->pDirectSound8;
    return true;
}

bool BSWin32GameSound::OpenOgg() {
    BSWin32GameSound::OggVorbisFile* pOVFile;
    BSWin32GameSound::OggVorbisFile* apMemory = static_cast<BSWin32GameSound::OggVorbisFile*>(
        MemoryManager_Singleton.Allocate(sizeof(BSWin32GameSound::OggVorbisFile))
    );
    if (apMemory) {
        pOVFile = ::new (apMemory) BSWin32GameSound::OggVorbisFile;
    }
    else {
        pOVFile = nullptr;
    }
    pOggFile = pOVFile;
    if (pOggFile->OpenFile(pFilename)) {
        uint32_t v2;
        int32_t uiChannelCount;
        int32_t v4;
        uint16_t usChannels;
        uint32_t rate;
        BSWin32GameSound::OggVorbisFile* v11;
        uint32_t Size;
        if (((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
             (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0) &&
            ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER) != 0 ||
             (iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) != 0)) {
            iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(iSoundTypeFlags | BSGameSound::SOUND_FLAG_LOOP);
        }
        iSoundTypeFlags =
            static_cast<BSGameSound::SoundFlags>(iSoundTypeFlags & ~BSGameSound::SOUND_FLAG_ENVELOPE_FAST);
        iSoundTypeFlags =
            static_cast<BSGameSound::SoundFlags>(iSoundTypeFlags & ~BSGameSound::SOUND_FLAG_ENVELOPE_SLOW);
        bool bRadio = (iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT22) != 0 ||
            (iSoundTypeFlags & BSGameSound::SOUND_FLAG_RADIO) != 0;
        kBufferDesc.dwSize = sizeof(DSBUFFERDESC);
        kBufferDesc.lpwfxFormat->cbSize = 0;
        kBufferDesc.lpwfxFormat->wFormatTag = 1;
        BSWin32GameSound::OggVorbisFile* v15 = pOggFile;
        if (v15->pInfo) {
            // vorbis_info's rate is a C long, which libvorbis reads from the header's 32-bit unsigned field and
            // accepts only from 1 up: uint32_t holds it.
            rate = static_cast<uint32_t>(v15->pInfo->rate);
        }
        else {
            rate = 0;
        }
        kBufferDesc.lpwfxFormat->nSamplesPerSec = rate;
        kBufferDesc.lpwfxFormat->wBitsPerSample = 16;
        BSWin32GameSound::OggVorbisFile* v14 = pOggFile;
        // Only the channel count's low word is stored (nChannels is a WORD).
        if (v14->pInfo) {
            usChannels = v14->pInfo->channels;
        }
        else {
            usChannels = 1;
        }
        kBufferDesc.lpwfxFormat->nChannels = usChannels;
        kBufferDesc.lpwfxFormat->nBlockAlign =
            kBufferDesc.lpwfxFormat->nChannels * (kBufferDesc.lpwfxFormat->wBitsPerSample / 8);
        kBufferDesc.lpwfxFormat->nAvgBytesPerSec =
            kBufferDesc.lpwfxFormat->nSamplesPerSec * kBufferDesc.lpwfxFormat->nBlockAlign;
        BSWin32GameSound::OggVorbisFile* pOGGFile = pOggFile;
        if (pOGGFile->pInfo) {
            v4 = pOGGFile->pInfo->channels;
        }
        else {
            v4 = 1;
        }
        kBufferDesc.dwBufferBytes = v4 * (2 * pOGGFile->iSampleCount);
        uiBufferBytes = kBufferDesc.dwBufferBytes;
        if (bRadio) {
            Size = kBufferDesc.dwBufferBytes + (kBufferDesc.lpwfxFormat->nAvgBytesPerSec >> 1);
        }
        else {
            Size = kBufferDesc.dwBufferBytes;
        }
        pRawBuffer = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(Size));
        std::memset(pRawBuffer, 0, Size);
        iSamplesPerSecond = kBufferDesc.lpwfxFormat->nSamplesPerSec;
        iModSamplesPerSecond = iSamplesPerSecond;
        BSWin32GameSound::OggVorbisFile* v12 = pOggFile;
        if (v12->pInfo) {
            uiChannelCount = v12->pInfo->channels;
        }
        else {
            uiChannelCount = 1;
        }
        iDurationInMS = uiBufferBytes / (2 * uiChannelCount) / (iSamplesPerSecond / 1000);
        pOggFile->Read(pRawBuffer, kBufferDesc.dwBufferBytes);
        if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_2D) != 0 ||
            ((v11 = pOggFile, !v11->pInfo) ? v2 = 1 : v2 = v11->pInfo->channels, v2 > 1)) {
            kBufferDesc.dwFlags = 64;
            iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(iSoundTypeFlags & ~BSGameSound::SOUND_FLAG_IS_3D);
            iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(iSoundTypeFlags | BSGameSound::SOUND_FLAG_IS_2D);
        }
        else {
            kBufferDesc.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_MUTE3DATMAXDISTANCE;
        }
        kBufferDesc.dwFlags |= 672u;
        if (bRadio) {
            kBufferDesc.dwBufferBytes += kBufferDesc.lpwfxFormat->nAvgBytesPerSec >> 1;
            uiBufferBytes = kBufferDesc.dwBufferBytes;
            iDurationInMS += 500;
        }
        return OpenResult(true);
    }
    else {
        BSWin32GameSound::OggVorbisFile* Block = pOggFile;
        if (Block) {
            Block->Destroy();
            MemoryManager_Singleton.Deallocate(Block);
        }
        pOggFile = nullptr;
        return false;
    }
}

bool BSWin32GameSound::PrepareReverb() {
    Setting::Info* iMaxFX;
    Setting::Info* bEnableEnviroEffectsOnPC;
    uint32_t v8;
    void* v9;
    void* v11;
    DSEFFECTDESC v12;
    uint32_t v13;
    DSBUFFERDESC kBufferDesc;
    void* v15;
    uint32_t v16;
    void* v17;
    uint32_t v18;
    if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_SYSTEM_SOUND) != 0) {
        return false;
    }
    bEnableEnviroEffectsOnPC = &gs_bEnableEnviroEffectsOnPC.uValue;
    if (!bEnableEnviroEffectsOnPC->b) {
        return false;
    }
    if (!pDirectSound8) {
        return false;
    }
    if (!pSoundBuffer) {
        return false;
    }
    if (!BSAudio::pAudioInstance->pListener) {
        return false;
    }
    if (!BSAudio::pAudioInstance->pListener->GetEnvironmentType()) {
        return false;
    }
    if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
        (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0) {
        return false;
    }
    iMaxFX = &BSWin32GameSound::iMaxFX.uValue;
    if (iMaxFX->uint <= BSWin32GameSound::uiEffectCount) {
        return false;
    }
    v9 = nullptr;
    v11 = nullptr;
    v13 = 0;
    v8 = 0;
    v15 = nullptr;
    v17 = nullptr;
    v18 = 0;
    v16 = 0;
    std::memset(&kBufferDesc, 0, sizeof(kBufferDesc));
    kBufferDesc.dwSize = sizeof(DSBUFFERDESC);
    if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_2D) != 0) {
        kBufferDesc.dwFlags = 64;
    }
    else {
        kBufferDesc.dwFlags = 0x30010;
    }
    kBufferDesc.dwFlags |= 672u;
    kBufferDesc.dwBufferBytes = uiBufferBytes + static_cast<int64_t>(static_cast<double>(uiBufferBytes) * 0.25);
    // GUID_NULL here is DS3DALG_DEFAULT.
    kBufferDesc.guid3DAlgorithm = GUID_NULL;
    kBufferDesc.lpwfxFormat = static_cast<tWAVEFORMATEX*>(MemoryManager_Singleton.Allocate(sizeof(tWAVEFORMATEX)));
    pSoundBuffer->GetFormat(kBufferDesc.lpwfxFormat, sizeof(WAVEFORMATEX), &v13);
    ++BSWin32GameSound::uiEffectCount;
    if (pDirectSound8->CreateSoundBuffer(&kBufferDesc, &pTempReverbBuffer, nullptr) >= 0 &&
        (pTempReverbBuffer->QueryInterface(CLSID_IDirectSoundBuffer8, reinterpret_cast<void**>(&pReverbBuffer)),
         pReverbBuffer)) {
        pReverbBuffer->QueryInterface(CLSID_IDirectSound3DBuffer, reinterpret_cast<void**>(&p3DReverbBuffer));
        v12.dwSize = sizeof(DSEFFECTDESC);
        v12.dwFlags = 0;
        v12.guidDSFXClass = CLSID_GUID_DSFX_STANDARD_I3DL2REVERB;
        v12.dwReserved1 = 0;
        v12.dwReserved2 = 0;
        int32_t v10 = pReverbBuffer->SetFX(1, &v12, nullptr);
        int32_t v2 = pReverbBuffer->GetObjectInPath(
            &CLSID_GUID_DSFX_STANDARD_I3DL2REVERB, 0, &CLSID_IDirectSoundFXI3DL2Reverb,
            reinterpret_cast<void**>(&pReverbEffect)
        );
        v10 = v2;
        if (v2 >= 0) {
            uint32_t eEnvironmentType = this->iEnvironmentType;
            if (BSAudio::pAudioInstance->pListener) {
                eEnvironmentType = BSAudio::pAudioInstance->pListener->GetEnvironmentType();
            }
            if (eEnvironmentType) {
                v10 = pReverbEffect->SetPreset(eEnvironmentType - 1);
            }
        }
        v13 = 0;
        if (pReverbBuffer->Lock(0, 0, &v9, &v13, &v11, &v8, 2) >= 0) {
            if (pSoundBuffer->Lock(0, 0, &v15, &v18, &v17, &v16, 2) >= 0) {
                std::memset(v9, 0, kBufferDesc.dwBufferBytes);
                rt::memcpy(v9, v15, this->kBufferDesc.dwBufferBytes);
                pSoundBuffer->Unlock(v15, v18, v17, v16);
            }
            pReverbBuffer->Unlock(v9, v13, v11, v8);
            MemoryManager_Singleton.Deallocate(kBufferDesc.lpwfxFormat);
            return true;
        }
        else {
            MemoryManager_Singleton.Deallocate(kBufferDesc.lpwfxFormat);
            return false;
        }
    }
    else {
        MemoryManager_Singleton.Deallocate(kBufferDesc.lpwfxFormat);
        return false;
    }
}

void BSWin32GameSound::RemoveEffects() {
    if (pReverbBuffer) {
        --BSWin32GameSound::uiEffectCount;
        pReverbBuffer->Stop();
        while (pReverbBuffer->Release()) {
            ;
        }
        pReverbBuffer = nullptr;
    }
    if (p3DReverbBuffer) {
        while (p3DReverbBuffer->Release()) {
            ;
        }
        p3DReverbBuffer = nullptr;
    }
}

bool BSWin32GameSound::Write(uint32_t auiPosition, uint32_t auiOffset, uint32_t auiSize, char* apData) {
    void* pData = nullptr;
    uint32_t uiSize = 0;
    if (pSoundBuffer->Lock(auiOffset, auiSize, &pData, &uiSize, nullptr, nullptr, 0) < 0) {
        return false;
    }
    void* pvDest = pData;
    if (apData) {
        rt::memcpy(pvDest, &apData[auiPosition], auiSize);
    }
    else {
        std::memset(pvDest, 0, auiSize);
    }
    pSoundBuffer->Unlock(pData, uiSize, nullptr, 0);
    return true;
}
