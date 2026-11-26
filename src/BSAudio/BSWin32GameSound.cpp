#include "BSAudio/BSWin32GameSound.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioListener.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSWin32Audio.h"
#include "BSAudio/DirectSound.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
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
#include <limits>
#include <vorbis/vorbisfile.h>

SettingT<INISettingCollection> gs_bEnableEnviroEffectsOnPC("bEnableEnviroEffectsOnPC:Audio", false);
static SettingT<INISettingCollection> gs_iVoiceDecodeBufferSize_Audio("iVoiceDecodeBufferSize:Audio", 0x8000);
SettingT<INISettingCollection> BSWin32GameSound::iMaxFX("iMaxFX:Audio", 128);
SettingT<INISettingCollection> g_fFilterDistortionGain_Audio("fFilterDistortionGain:Audio", -18.2f);
static SettingT<INISettingCollection> gs_fFilterPEQGain_Audio("fFilterPEQGain:Audio", -15.0f);
static SettingT<INISettingCollection> fFilterdBAttenuation("fFilterdBAttenuation:Audio", 1.0f);

int32_t SeekSoundFileCallback(std::FILE* Stream, int32_t Offset, int32_t OffsetHigh, int32_t Origin) {
    if (Stream) {
        return std::fseek(Stream, Offset, Origin);
    }

    return (-1);
}

intptr_t CALLBACK mmio_IOProc(_MMIOINFO* lpmmioinfo, uint32_t uMsg, intptr_t lParam1, intptr_t lParam2) {
    int32_t result;
    uint32_t uiComponentSize;
    int32_t iReadResult = 0;
    switch (uMsg) {
        case 0u: {
            BSFile* pFile = static_cast<BSFile*>(lpmmioinfo->GetProcData());
            if (pFile) {
                uiComponentSize = 1;
                // MMIOM_READ: lParam1 is the buffer, lParam2 its size.
                uint32_t uiBytesRead = pFile->m_pfnRead(
                    pFile,
                    reinterpret_cast<char*>(lParam1),
                    static_cast<uint32_t>(lParam2),
                    &uiComponentSize,
                    1
                );
                pFile->m_uiAbsoluteCurrentPos += uiBytesRead;
                iReadResult = uiBytesRead;
                lpmmioinfo->lDiskOffset += uiBytesRead;
                result = iReadResult;
            }
            else {
                result = 261;
            }
            break;
        }
        case 2u: {
            int32_t iWhence = NiFile::ms_iSeekCur;
            if (lParam2) {
                if (lParam2 == 2) {
                    iWhence = NiFile::ms_iSeekEnd;
                }
            }
            else {
                iWhence = NiFile::ms_iSeekSet;
            }
            BSFile* pFile = static_cast<BSFile*>(lpmmioinfo->GetProcData());
            if (pFile) {
                int32_t iTrueFilePos;
                // MMIOM_SEEK: lParam1 is the offset.
                pFile->Seek(static_cast<int32_t>(lParam1), iWhence);
                if (pFile->iAuxTrueFilePos == -1) {
                    iTrueFilePos = pFile->iTrueFilePos;
                }
                else {
                    iTrueFilePos = pFile->iAuxTrueFilePos;
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
            BSFile* pFile = static_cast<BSFile*>(lpmmioinfo->GetProcData());
            if (pFile) {
                delete pFile;
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

int32_t SeekStreamCallback(std::FILE* Stream, int32_t Offset, int32_t OffsetHigh, int32_t Origin) {
    if (Stream) {
        return std::fseek(Stream, Offset, Origin);
    }

    return -1;
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

int32_t CloseFileCallback(BSFile* datasource) {
    if (!datasource) {
        return -1;
    }
    datasource->Close();
    delete datasource;
    return 0;
}

long tell_func(void* datasource) {
    if (!datasource) {
        return 0;
    }
    // The datasource is the BSFile OggVorbisFile hands ov_open_callbacks.
    BSFile* pFile = static_cast<BSFile*>(datasource);
    if (pFile->iAuxTrueFilePos == (-1)) {
        return static_cast<int32_t>(pFile->iTrueFilePos);
    }

    return pFile->iAuxTrueFilePos;
}

// The library's decoder state starts zeroed.
BSWin32GameSound::OggVorbisFile::OggVorbisFile()
  : OggVorbis_File{}, pFile(nullptr), iSampleCount(0), pInfo(nullptr), bLoaded(false) {}

// Lowercases the whole path into a 0x108-byte buffer, which strcpy fills unbounded; takes the
// extension as .ogg, or renames a .mp3 or .wav one to .ogg, then opens the file and hands it to libvorbisfile. A file
// that will not open, or is not a Vorbis stream, is closed and deleted; pFile is left null in both cases.
bool BSWin32GameSound::OggVorbisFile::OpenFile(const char* apPath) {
    char cPath[0x108];
    std::strcpy(cPath, apPath);
    for (int32_t i = 0; i < 0x103; ++i) {
        cPath[i] = static_cast<char>(std::tolower(cPath[i]));
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
            this,
            static_cast<char*>(static_cast<void*>(apBuffer)),
            auiSize - uiTotalReadBytes,
            0,
            2,
            1,
            &bitstream
        ));
        uiTotalReadBytes += uiReadBytes;
        apBuffer += uiReadBytes;
        if (!uiReadBytes) {
            auiSize = 0;
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
    kPosition(NiPoint3::ZERO), bPositionSet(false), pEnvelope(nullptr), pOggFile(nullptr) {
    kBufferDesc.lpwfxFormat = static_cast<tWAVEFORMATEX*>(MemoryManager::Instance().Allocate(sizeof(tWAVEFORMATEX)));
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
        MemoryManager::Instance().Deallocate(kBufferDesc.lpwfxFormat);
        kBufferDesc.lpwfxFormat = nullptr;
    }
    if (pRawBuffer) {
        MemoryManager::Instance().Deallocate(pRawBuffer);
        pRawBuffer = nullptr;
    }
    if (pEnvelope) {
        if (pEnvelope->pHeadBuf) {
            MemoryManager::Instance().Deallocate(pEnvelope->pHeadBuf);
        }
        if (pEnvelope->pTailBuf) {
            MemoryManager::Instance().Deallocate(pEnvelope->pTailBuf);
        }
        if (pEnvelope->pMidBuf) {
            MemoryManager::Instance().Deallocate(pEnvelope->pMidBuf);
        }
        MemoryManager::Instance().Deallocate(pEnvelope);
    }
    if (pOggFile) {
        BSWin32GameSound::OggVorbisFile* pFile = pOggFile;
        pFile->Destroy();
        MemoryManager::Instance().Deallocate(pFile);
        pOggFile = nullptr;
    }
}

void BSWin32GameSound::SetInaudible(bool abIn) {
    if (abIn) {
        iStateFlags |= BSGameSound::STATE_INAUDIBLE;
    }
    else {
        iStateFlags &= ~BSGameSound::STATE_INAUDIBLE;
    }
}

// Opens the sound: as an Ogg Vorbis stream for the region, underwater, voice, radio and music types when that works,
// otherwise as a PCM wave file (an .mp3 or .ogg name is turned into .wav) read whole into pRawBuffer. A missing radio,
// voice or looping file is replaced by a stand-in; a one-shot sound of 150 ms or less is padded to 250 ms, and every
// non-looping one gets a tail of 500 to 1000 ms (a fifth of its length between those). Envelope sounds take their
// loop points from the smpl chunk. The result goes through OpenResult, except for a bad fmt chunk, which returns
// false directly.
bool BSWin32GameSound::Open() {
    iStateFlags |= BSGameSound::STATE_OPENING;
    if (!SetDirectSound()) {
        return OpenResult(false);
    }
    if (((iSoundTypeFlags & SOUND_FLAG_REGION) ||
         (iSoundTypeFlags & SOUND_FLAG_MAYBE_UNDERWATER) ||
         (iSoundTypeFlags & SOUND_FLAG_VOICE) ||
         (iSoundTypeFlags & SOUND_FLAG_UNKBIT22) ||
         (iSoundTypeFlags & SOUND_FLAG_RADIO) ||
         (iSoundTypeFlags & SOUND_FLAG_MUSIC)) &&
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
            bool bLooping = IsLoopingSound();
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
        bool bShortLooping = IsLoopingSound();
        if (!bShortLooping) {
            // Not DSBCAPS_CTRLFX: the padded buffer gets no effects.
            kBufferDesc.dwFlags &= ~0x200u;
            uiPadBytes = (250 - iDurationInMS) * uiBytesPerSample * uiSamplesPerMS;
        }
    }
    bool bLooping = IsLoopingSound();
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
    pRawBuffer = static_cast<uint8_t*>(
        MemoryManager::Instance().Allocate(kChunkInfo.cksize + uiPadBytes + uiDataStart)
    );
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
    DSEFFECTDESC kEffectDesc;
    uint32_t dwResultCode;
    void* pvAudioPtr;
    uint32_t uiAudioBytes;
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
            size_t stTailPadBytes = 1024;
            pEnvelope = static_cast<BSWin32GameSound::EnvelopeBuffer*>(
                MemoryManager::Instance().Allocate(sizeof(BSWin32GameSound::EnvelopeBuffer))
            );
            pEnvelope->uiHeadBytes = uiBytesPerSample * iLoopStart;
            pEnvelope->uiTailSize = uiBufferBytes - uiBytesPerSample * iLoopEnd;
            pEnvelope->uiMiddleSize = uiBufferBytes - (pEnvelope->uiTailSize + pEnvelope->uiHeadBytes);
            uiBufferBytes = kBufferDesc.lpwfxFormat->nAvgBytesPerSec >> 1;
            kBufferDesc.dwBufferBytes = uiBufferBytes;
            pEnvelope->uiPlayCursor = 0;
            pEnvelope->uiState = 0;
            pEnvelope->uiSize = 0;
            pEnvelope->pHeadBuf = static_cast<uint8_t*>(MemoryManager::Instance().Allocate(pEnvelope->uiHeadBytes));
            pEnvelope->pMidBuf = static_cast<uint8_t*>(MemoryManager::Instance().Allocate(pEnvelope->uiMiddleSize));
            pEnvelope->pTailBuf = static_cast<uint8_t*>(
                MemoryManager::Instance().Allocate(stTailPadBytes + pEnvelope->uiTailSize)
            );
            void* pvSrc = pRawBuffer;
            rt::memcpy(pEnvelope->pHeadBuf, pvSrc, pEnvelope->uiHeadBytes);
            pvSrc = static_cast<char*>(pvSrc) + pEnvelope->uiHeadBytes;
            rt::memcpy(pEnvelope->pMidBuf, pvSrc, pEnvelope->uiMiddleSize);
            pvSrc = static_cast<char*>(pvSrc) + pEnvelope->uiMiddleSize;
            rt::memcpy(pEnvelope->pTailBuf, pvSrc, pEnvelope->uiTailSize);
            std::memset(&pEnvelope->pTailBuf[pEnvelope->uiTailSize], 0, stTailPadBytes);
            pEnvelope->uiTailSize += stTailPadBytes;
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
                    kBufferDesc.lpwfxFormat->nChannels,
                    pFilename
                );
            }
            else {
                if (hResult == DSERR_BUFFERTOOSMALL) {
                    Warning("Audio: Wav file too small for DirectSound to process\"%s\" ", pFilename);
                }
            }
            if (pRawBuffer) {
                MemoryManager::Instance().Deallocate(pRawBuffer);
                pRawBuffer = nullptr;
            }
            return OpenResult(false);
        }
        hResult = pTempSoundBuffer->QueryInterface(CLSID_IDirectSoundBuffer8, reinterpret_cast<void**>(&pSoundBuffer));
        if (pSoundBuffer && ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_3D) != 0)) {
            hResult = pSoundBuffer->QueryInterface(
                CLSID_IDirectSound3DBuffer,
                reinterpret_cast<void**>(&p3DSoundBuffer)
            );
            p3DSoundBuffer->SetMinDistance(1e+09f, 1);
            SetEmitterPosition(-3.4028235e+37f, -3.4028235e+37f, -3.4028235e+37f);
            fDistanceToListener = std::numeric_limits<float>::max();
            pTempSoundBuffer->SetVolume(DSBVOLUME_MIN);
        }

        pvAudioPtr = nullptr;
        uiAudioBytes = 0;
        hResult = pSoundBuffer->Lock(0, uiBufferBytes, &pvAudioPtr, &uiAudioBytes, nullptr, nullptr, 0);
        if (hResult < 0) {
            return OpenResult(false);
        }
        void* pvDest = pvAudioPtr;
        bool bEnvelope = (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
            (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0;
        if (bEnvelope && (pEnvelope && pEnvelope->pHeadBuf)) {
            if (pEnvelope->uiHeadBytes <= uiBufferBytes) {
                rt::memcpy(pvDest, pEnvelope->pHeadBuf, pEnvelope->uiHeadBytes);
                pEnvelope->uiSize = 0;
                pEnvelope->uiPlayCursor = pEnvelope->uiHeadBytes;
                pEnvelope->uiState = 2;
            }
            else {
                rt::memcpy(pvDest, pEnvelope->pHeadBuf, uiBufferBytes);
                pEnvelope->uiSize = uiBufferBytes;
                pEnvelope->uiState = 1;
            }
        }
        else {
            rt::memcpy(pvDest, pRawBuffer, uiBufferBytes);
        }
        pSoundBuffer->Unlock(pvAudioPtr, uiAudioBytes, nullptr, 0);
        if (pRawBuffer) {
            MemoryManager::Instance().Deallocate(pRawBuffer);
            pRawBuffer = nullptr;
        }
        SetVolume(fVolume);
    }
    // The duration is compared signed, as iDurationInMS is an int.
    if (static_cast<int32_t>(iDurationInMS) >= 150) {
        PrepareReverb();
    }
    if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_MODULATED) != 0) {
        kEffectDesc.dwSize = sizeof(DSEFFECTDESC);
        kEffectDesc.dwFlags = 0;
        kEffectDesc.guidDSFXClass = CLSID_GUID_DSFX_STANDARD_DISTORTION;
        kEffectDesc.dwReserved1 = 0;
        kEffectDesc.dwReserved2 = 0;
        pSoundBuffer->SetFX(1, &kEffectDesc, &dwResultCode);
        if (pSoundBuffer->GetObjectInPath(
                &CLSID_GUID_DSFX_STANDARD_DISTORTION,
                0,
                &CLSID_IDirectSoundFXDistortion,
                reinterpret_cast<void**>(&pDistortionEffect)
            ) >= 0) {
            Setting::Info* pGainValue;
            pGainValue = &g_fFilterDistortionGain_Audio.uValue;
            kDistortionData.fGain = pGainValue->f;
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
    _DSCAPS kCaps;
    uint32_t uiDestBytes2;
    void* pvDestPtr1;
    void* pvDestPtr2;
    uint32_t uiDestBytes1;
    void* pvSrcPtr1;
    uint32_t uiSrcBytes2;
    void* pvSrcPtr2;
    uint32_t uiSrcBytes1;
    BSWin32GameSound* pSource = static_cast<BSWin32GameSound*>(apToCopy);
    if (pSource->pRawBuffer) {
        pSource->Prepare();
    }
    BSGameSound::Copy(apToCopy, true);
    pReverbBuffer = nullptr;
    pTempReverbBuffer = nullptr;
    p3DReverbBuffer = nullptr;
    iDirectoryHash = apToCopy->iDirectoryHash;
    uiBufferBytes = pSource->uiBufferBytes;
    iSoundSize = uiBufferBytes;
    iSamplesPerSecond = pSource->iSamplesPerSecond;
    iModSamplesPerSecond = pSource->iModSamplesPerSecond;
    pSoundBuffer = nullptr;
    p3DSoundBuffer = nullptr;
    pDirectSound8 = pSource->pDirectSound8;
    if (pSource && pSource->pSoundBuffer) {
        char pcDest[256];
        if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_2D) == 0 &&
            (iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_3D) == 0) {
            iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(iSoundTypeFlags | BSGameSound::SOUND_FLAG_IS_2D);
        }
        pvDestPtr1 = nullptr;
        pvDestPtr2 = nullptr;
        uiDestBytes1 = 0;
        uiDestBytes2 = 0;
        pvSrcPtr1 = nullptr;
        pvSrcPtr2 = nullptr;
        uiSrcBytes1 = 0;
        uiSrcBytes2 = 0;
        kBufferDesc.dwSize = sizeof(DSBUFFERDESC);
        kBufferDesc.dwBufferBytes = pSource->kBufferDesc.dwBufferBytes;
        pSource->pSoundBuffer->GetFormat(kBufferDesc.lpwfxFormat, sizeof(WAVEFORMATEX), &uiDestBytes1);
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
        uint32_t uiBytesPerSample = kBufferDesc.lpwfxFormat->wBitsPerSample / 8;
        uint32_t uiPadBytes = 0;
        int32_t iExtraBytes = 0;
        uint32_t uiSamplesPerMS = iSamplesPerSecond / 1000;
        // The duration compare is signed.
        if (static_cast<int32_t>(iDurationInMS) <= 150) {
            kBufferDesc.dwFlags &= ~512u;
            if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_LOOP) == 0 &&
                ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) == 0 &&
                 (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) == 0)) {
                uiPadBytes = uiSamplesPerMS * (uiBytesPerSample * (250 - iDurationInMS));
            }
        }
        uiBufferBytes += iExtraBytes + uiPadBytes;
        kBufferDesc.dwBufferBytes = uiBufferBytes;
        iSoundSize = kBufferDesc.dwBufferBytes;
        iDurationInMS = uiBufferBytes / uiBytesPerSample / uiSamplesPerMS;
        if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_2D) != 0) {
            kBufferDesc.guid3DAlgorithm = GUID_NULL;
        }
        else {
            kCaps = static_cast<BSWin32Audio*>(BSAudio::pAudioInstance)->kBufferCaps;
            if (kCaps.dwFreeHwMixingAllBuffers) {
                kBufferDesc.guid3DAlgorithm = CLSID_DS3DALG_HRTF_LIGHT;
            }
            else {
                kBufferDesc.guid3DAlgorithm = GUID_NULL;
            }
        }
        float fFrequencyRatio = static_cast<double>(iModSamplesPerSecond) / static_cast<double>(iSamplesPerSecond);
        iSamplesPerSecond = kBufferDesc.lpwfxFormat->nSamplesPerSec;
        iModSamplesPerSecond = static_cast<int32_t>(static_cast<double>(iSamplesPerSecond) * fFrequencyRatio);
        iDurationInMS = uiBufferBytes / (static_cast<int32_t>(kBufferDesc.lpwfxFormat->wBitsPerSample) >> 3) /
            (iSamplesPerSecond / 1000);
        int32_t eError = pDirectSound8->CreateSoundBuffer(&kBufferDesc, &pTempSoundBuffer, nullptr);
        if (eError >= 0) {
            if (pTempSoundBuffer->Lock(0, 0, &pvDestPtr1, &uiDestBytes1, &pvDestPtr2, &uiDestBytes2, 2) >= 0) {
                if (pSource->pSoundBuffer->Lock(0, 0, &pvSrcPtr1, &uiSrcBytes1, &pvSrcPtr2, &uiSrcBytes2, 2) >= 0) {
                    // The padded size (up to 250 ms more) is larger than the source buffer; the padding is the silence
                    // (zeros) Prepare pads a sound with.
                    uint32_t uiCopyBytes = uiSrcBytes1 < kBufferDesc.dwBufferBytes ? uiSrcBytes1
                                                                                   : kBufferDesc.dwBufferBytes;
                    rt::memcpy(pvDestPtr1, pvSrcPtr1, uiCopyBytes);
                    std::memset(
                        static_cast<uint8_t*>(pvDestPtr1) + uiCopyBytes,
                        0,
                        kBufferDesc.dwBufferBytes - uiCopyBytes
                    );
                    pSource->pSoundBuffer->Unlock(pvSrcPtr1, uiSrcBytes1, pvSrcPtr2, uiSrcBytes2);
                }
                pTempSoundBuffer->Unlock(pvDestPtr1, uiDestBytes1, pvDestPtr2, uiDestBytes2);
                pTempSoundBuffer->QueryInterface(CLSID_IDirectSoundBuffer8, reinterpret_cast<void**>(&pSoundBuffer));
                if (pSoundBuffer && ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_3D) != 0)) {
                    pSoundBuffer->QueryInterface(CLSID_IDirectSound3DBuffer, reinterpret_cast<void**>(&p3DSoundBuffer));
                    if (p3DSoundBuffer) {
                        p3DSoundBuffer->SetMinDistance(1e+09f, 1);
                    }
                }

                if (((pSource->iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
                     (pSource->iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0) &&
                    pSource->pEnvelope) {
                    pEnvelope = static_cast<BSWin32GameSound::EnvelopeBuffer*>(
                        MemoryManager::Instance().Allocate(sizeof(BSWin32GameSound::EnvelopeBuffer))
                    );
                    pEnvelope->uiHeadBytes = pSource->pEnvelope->uiHeadBytes;
                    pEnvelope->uiTailSize = pSource->pEnvelope->uiTailSize;
                    pEnvelope->uiMiddleSize = pSource->pEnvelope->uiMiddleSize;
                    pEnvelope->uiPlayCursor = 0;
                    pEnvelope->uiState = 0;
                    pEnvelope->uiSize = 0;
                    pEnvelope->pHeadBuf = static_cast<uint8_t*>(
                        MemoryManager::Instance().Allocate(pEnvelope->uiHeadBytes)
                    );
                    pEnvelope->pMidBuf = static_cast<uint8_t*>(
                        MemoryManager::Instance().Allocate(pEnvelope->uiMiddleSize)
                    );
                    pEnvelope->pTailBuf = static_cast<uint8_t*>(
                        MemoryManager::Instance().Allocate(pEnvelope->uiTailSize)
                    );
                    rt::memcpy(pEnvelope->pHeadBuf, pSource->pEnvelope->pHeadBuf, pEnvelope->uiHeadBytes);
                    rt::memcpy(pEnvelope->pMidBuf, pSource->pEnvelope->pMidBuf, pEnvelope->uiMiddleSize);
                    rt::memcpy(pEnvelope->pTailBuf, pSource->pEnvelope->pTailBuf, pEnvelope->uiTailSize);
                }

                iStateFlags = 0;
                OpenResult(true);
                if (abPlayable) {
                    Prepare();
                }
            }
            return;
        }
        switch (eError) {
            case static_cast<int32_t>(0x8878000Au): {
                strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_ALLOCATED\n");
                break;
            }
            case static_cast<int32_t>(0x8878001Eu): {
                strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_CONTROLUNAVAIL\n");
                break;
            }
            case static_cast<int32_t>(0x80070057u): {
                strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_INVALIDPARAM\n");
                break;
            }
            case static_cast<int32_t>(0x88780032u): {
                strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_INVALIDCALL\n");
                break;
            }
            case static_cast<int32_t>(0x88780064u): {
                strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_BADFORMAT\n");
                break;
            }
            case static_cast<int32_t>(0x80004001u): {
                strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_UNSUPPORTED\n");
                break;
            }
            case static_cast<int32_t>(0x8007000Eu): {
                strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_OUTOFMEMORY\n");
                break;
            }
            case static_cast<int32_t>(0x887800AAu): {
                strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_UNINITIALIZED\n");
                break;
            }
            case static_cast<int32_t>(0x80040110u): {
                strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_NOAGGREGATION\n");
                break;
            }
            case static_cast<int32_t>(0x887800B4u): {
                strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_BUFFERTOOSMALL\n");
                break;
            }
            case static_cast<int32_t>(0x887800BEu): {
                strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer: DSERR_DS8_REQUIRED\n");
                break;
            }
            default: {
                strcpy_s(pcDest, 255, "Audio: Cound not create DSound buffer - unknown warning\n");
                break;
            }
        }
        Warning(pcDest);
    }
}

bool BSWin32GameSound::Play(bool abLoopPlayback) {
    int32_t eResult = E_FAIL;
    if (pSoundBuffer && pDirectSound8) {
        if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_LOOP) != 0) {
            abLoopPlayback = true;
            uint32_t iSoundSize = this->iSoundSize;
            uint32_t uiRandom = rt::rand();
            pSoundBuffer->SetCurrentPosition(uiRandom % (iSoundSize - 1));
        }
        else {
            if (((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
                 (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0) &&
                (!IsPaused() || (!pEnvelope || pEnvelope->uiState != 5))) {
                abLoopPlayback = true;
                if (((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
                     (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0) &&
                    pEnvelope &&
                    pEnvelope->pHeadBuf &&
                    (pEnvelope->uiState != 1 && pEnvelope->uiState != 2)) {
                    pSoundBuffer->SetCurrentPosition(0);
                    if (pEnvelope->uiHeadBytes <= uiBufferBytes) {
                        uint32_t dwWriteBytes = uiBufferBytes - pEnvelope->uiHeadBytes;
                        Write(0, 0, pEnvelope->uiHeadBytes, reinterpret_cast<char*>(pEnvelope->pHeadBuf));
                        Write(0, pEnvelope->uiHeadBytes, dwWriteBytes, reinterpret_cast<char*>(pEnvelope->pMidBuf));
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

    if (eResult >= 0) {
        iStateFlags |= BSGameSound::STATE_PLAYING;
        iStateFlags &= ~BSGameSound::STATE_PAUSED;
    }
    SetVolume(fVolume);
    return eResult >= 0;
}

bool BSWin32GameSound::Pause() {
    HRESULT eResult = E_FAIL;
    if (pSoundBuffer && pDirectSound8) {
        eResult = pSoundBuffer->Stop();
        if (pReverbBuffer) {
            pReverbBuffer->Stop();
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
    if (pSoundBuffer && pDirectSound8) {
        bool bEnvelope = (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
            (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0;
        if (!bEnvelope || (!pEnvelope || (pEnvelope->uiState == 5 || pEnvelope->uiState == 6))) {
            bool bEnvelopeSound = (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
                (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0;
            if (!bEnvelopeSound || !pEnvelope) {
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

    return eResult >= 0;
}

bool BSWin32GameSound::SetVolume(float afNewVolume) {
    int32_t iReverbVolume;
    int32_t iClampedReverbVolume;
    int32_t iNormalVolume;
    int32_t iClampedVolume;
    float fNonzeroVolume;
    float fClampedVolume;
    float fCappedVolume;
    fVolume = afNewVolume;
    float fRawReverbAttenuation = static_cast<float>(iReverbAttenuation);
    float fReverbAttenuation = fRawReverbAttenuation / 100.0;
    if (!pSoundBuffer) {
        return false;
    }
    if (!(afNewVolume > 1.0)) {
        fCappedVolume = afNewVolume;
    }
    else {
        fCappedVolume = 1.0f;
    }
    if (!(fCappedVolume < 0.0)) {
        fClampedVolume = fCappedVolume;
    }
    else {
        fClampedVolume = 0.0f;
    }
    BSGameSound::SoundFlags eSoundFlags = iSoundTypeFlags;
    float afVolume = BSAudioManager::QInstance().GetCorrectedVolume(fClampedVolume, eSoundFlags);
    if ((iStateFlags & BSGameSound::STATE_FADING) == 0 &&
        (iStateFlags & BSGameSound::STATE_PLAY_LATER) != 0 &&
        (iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) == 0) {
        afVolume = 1e-05f;
    }
    if (!(afVolume < 9.999999747378752e-06)) {
        fNonzeroVolume = afVolume;
    }
    else {
        fNonzeroVolume = 1e-05f;
    }
    float fVolumeLog = std::log10(static_cast<double>(fNonzeroVolume));
    int32_t iFinalVolume = static_cast<int32_t>(fVolumeLog * 2000.0) -
        (iFaderAttenuation + (iSystemAttenuation + iStaticAttenuation));
    if (Is3DSound() || Is2DRadiusSound()) {
        iFinalVolume -= iDistanceAttenuation;
    }
    if (iFinalVolume <= DSBVOLUME_MIN) {
        iClampedVolume = DSBVOLUME_MIN;
    }
    else {
        iClampedVolume = iFinalVolume;
    }
    if (iClampedVolume >= -1) {
        iNormalVolume = -1;
    }
    else {
        iNormalVolume = iClampedVolume;
    }
    if (static_cast<int32_t>(static_cast<int64_t>(fReverbAttenuation * -10000.0) + iNormalVolume) <= DSBVOLUME_MIN) {
        iClampedReverbVolume = DSBVOLUME_MIN;
    }
    else {
        iClampedReverbVolume = static_cast<int64_t>(fReverbAttenuation * -10000.0) + iNormalVolume;
    }
    if (iClampedReverbVolume >= -1) {
        iReverbVolume = -1;
    }
    else {
        iReverbVolume = iClampedReverbVolume;
    }
    if ((iStateFlags & BSGameSound::STATE_MUTED) != 0) {
        if (pReverbBuffer) {
            pReverbBuffer->SetVolume(DSBVOLUME_MIN);
        }
        return pSoundBuffer->SetVolume(DSBVOLUME_MIN) >= 0;
    }

    int32_t eResult = E_FAIL;
    if (pReverbBuffer) {
        pReverbBuffer->SetVolume(iReverbVolume);
    }
    if (pSoundBuffer) {
        eResult = pSoundBuffer->SetVolume(iNormalVolume);
    }
    return eResult >= 0;
}

void BSWin32GameSound::UpdateEmitterPosition() {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    NiPoint3 kPos = NiPoint3::ZERO;
    if ((Is3DSound() || Is2DRadiusSound()) && BSAudio::pAudioInstance->pListener) {
        if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_FIRST_PERSON) != 0) {
            BSAudioListener* pListener = BSAudio::pAudioInstance->pListener;
            NiPoint3 kListenerPos = pListener->GetPosition();
            x = kListenerPos.x;
            y = kListenerPos.y;
            z = kListenerPos.z;
            // DirectSound's y is Gamebryo's z, so the position is passed as x, z, y.
            if (p3DSoundBuffer) {
                float fSoundZ = y * 0.014287755824625492;
                float fSoundY = z * 0.014287755824625492;
                float fSoundX = x * 0.014287755824625492;
                p3DSoundBuffer->SetPosition(fSoundX, fSoundY, fSoundZ, 1);
                if (p3DReverbBuffer) {
                    float fReverbZ = y * 0.014287755824625492;
                    float fReverbY = z * 0.014287755824625492;
                    float fReverbX = x * 0.014287755824625492;
                    p3DReverbBuffer->SetPosition(fReverbX, fReverbY, fReverbZ, 1);
                }
            }
        }
        else {
            if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_BEAM_EMITTER) != 0 &&
                (iStateFlags & BSGameSound::STATE_IS_VALID_BEAM) != 0) {
                float fProjection = 0.0;
                float fLengthSquared = 0.0;
                float fRatio = 0.0;
                NiPoint3 kListenerPos = BSAudio::pAudioInstance->pListener->GetPosition();
                NiPoint3 pt = vBeamEnd - kPosition;
                NiPoint3 kToListener = kListenerPos - kPosition;
                fProjection = kToListener.Dot(pt);
                if (!(fProjection <= 0.0)) {
                    fLengthSquared = pt.Dot(pt);
                    if (!(fProjection >= static_cast<double>(fLengthSquared))) {
                        fRatio = fProjection / fLengthSquared;
                        kPos = kPosition + fRatio * pt;
                    }
                    else {
                        kPos = vBeamEnd;
                    }
                }
                else {
                    kPos = kPosition;
                }
                if (p3DSoundBuffer) {
                    float fSoundZ = kPos.y * 0.014287755824625492;
                    float fSoundY = kPos.z * 0.014287755824625492;
                    float fSoundX = kPos.x * 0.014287755824625492;
                    p3DSoundBuffer->SetPosition(fSoundX, fSoundY, fSoundZ, 1);
                    if (p3DReverbBuffer) {
                        float fReverbZ = kPos.y * 0.014287755824625492;
                        float fReverbY = kPos.z * 0.014287755824625492;
                        float fReverbX = kPos.x * 0.014287755824625492;
                        p3DReverbBuffer->SetPosition(fReverbX, fReverbY, fReverbZ, 1);
                    }
                }
            }
        }
        float fListenerDistance = fDistanceToListener;
        if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_BEAM_EMITTER) != 0 &&
            ((iStateFlags & BSGameSound::STATE_IS_VALID_BEAM) != 0 && BSAudio::pAudioInstance->pListener)) {
            NiPoint3* pListenerPos = &BSAudio::pAudioInstance->pListener->GetPosition();
            fListenerDistance = (kPos - *pListenerPos).Length();
            fDistanceToListener = fListenerDistance;
        }
        else {
            if (BSAudio::pAudioInstance->pListener) {
                NiPoint3* pListenerPos = &BSAudio::pAudioInstance->pListener->GetPosition();
                fListenerDistance = (kPosition - *pListenerPos).Length();
                fDistanceToListener = fListenerDistance;
            }
        }
        float fRange = fMaxDist - fMinDist;
        float fAbsRange = std::fabs(static_cast<double>(fRange));
        if (!(fAbsRange > 10.0) || !(fListenerDistance < fMaxDist * 1.100000023841858)) {
            if (!(fMaxDist <= static_cast<double>(fListenerDistance))) {
                iDistanceAttenuation = 0;
            }
            else {
                iDistanceAttenuation = 0x2710;
            }
            SetVolume(fVolume);
        }
        else {
            float fQuarterDist = (fMaxDist - fMinDist) * 0.25 + fMinDist;
            float fHalfDist = (fMaxDist - fMinDist) * 0.5 + fMinDist;
            float fThreeQuarterDist = (fMaxDist - fMinDist) * 0.75 + fMinDist;
            if (!(fMaxDist <= static_cast<double>(fListenerDistance))) {
                if (!(fThreeQuarterDist < static_cast<double>(fListenerDistance)) ||
                    !(fMaxDist - fThreeQuarterDist > 0.0)) {
                    if (!(fHalfDist < static_cast<double>(fListenerDistance)) ||
                        !(fThreeQuarterDist - fHalfDist > 0.0)) {
                        if (!(fQuarterDist < static_cast<double>(fListenerDistance)) ||
                            !(fHalfDist - fQuarterDist > 0.0)) {
                            if (!(fMinDist < static_cast<double>(fListenerDistance)) ||
                                !(fQuarterDist - fMinDist > 0.0)) {
                                iDistanceAttenuation = 0;
                                SetVolume(fVolume);
                            }
                            else {
                                iDistanceAttenuation = static_cast<int32_t>(
                                    (fListenerDistance - fMinDist) / (fQuarterDist - fMinDist) *
                                        static_cast<double>(usAttenuationCurve[1] - usAttenuationCurve[0]) +
                                    static_cast<double>(usAttenuationCurve[0])
                                );
                                SetVolume(fVolume);
                            }
                        }
                        else {
                            iDistanceAttenuation = static_cast<int32_t>(
                                (fListenerDistance - fQuarterDist) / (fHalfDist - fQuarterDist) *
                                    static_cast<double>(usAttenuationCurve[2] - usAttenuationCurve[1]) +
                                static_cast<double>(usAttenuationCurve[1])
                            );
                            SetVolume(fVolume);
                        }
                    }
                    else {
                        iDistanceAttenuation = static_cast<int32_t>(
                            (fListenerDistance - fHalfDist) / (fThreeQuarterDist - fHalfDist) *
                                static_cast<double>(usAttenuationCurve[3] - usAttenuationCurve[2]) +
                            static_cast<double>(usAttenuationCurve[2])
                        );
                        SetVolume(fVolume);
                    }
                }
                else {
                    iDistanceAttenuation = static_cast<int32_t>(
                        (fListenerDistance - fThreeQuarterDist) / (fMaxDist - fThreeQuarterDist) *
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

bool BSWin32GameSound::Update(uint32_t adwTimeDelta) {
    uint32_t uiCurrPos;
    WAVEFORMATEX kFormat;
    uint32_t dwWriteBytes;
    uint32_t uiWriteCursor;
    char bResetPlayCursor;
    int32_t uiPlayCursor;
    int32_t eStatus;
    if (pSoundBuffer && pDirectSound8) {
        pSoundBuffer->GetStatus(reinterpret_cast<uint32_t*>(&eStatus));
        if ((eStatus & DSBSTATUS_PLAYING) != 0) {
            iStateFlags |= BSGameSound::STATE_PLAYING;
        }
        else {
            iStateFlags &= ~BSGameSound::STATE_PLAYING;
        }
        if (!IsPlaying() ||
            !((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
              (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0) ||
            !pEnvelope) {
            if (IsPlaying() && (iSoundTypeFlags & BSGameSound::SOUND_FLAG_EXTERNAL_SYNC) != 0) {
                if ((iStateFlags & BSGameSound::STATE_EXPLICIT_PAUSE) != 0 ||
                    (BSAudio::pAudioInstance->SynchTimer.iFlags & AudioTick::FLAG_PAUSED) == 0) {
                    if ((iStateFlags & BSGameSound::STATE_EXPLICIT_PAUSE) != 0 &&
                        (BSAudio::pAudioInstance->SynchTimer.iFlags & AudioTick::FLAG_PAUSED) == 0) {
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
                    bool bStopped = false;
                    pSoundBuffer->GetCurrentPosition(&uiCurrPos, nullptr);
                    pSoundBuffer->GetFormat(&kFormat, sizeof(WAVEFORMATEX), nullptr);
                    int32_t uiOffset = static_cast<int32_t>(static_cast<double>(kFormat.nAvgBytesPerSec) / 1000.0);
                    uint32_t uiPositionMS = static_cast<int64_t>(
                        static_cast<double>(uiCurrPos) / (static_cast<double>(kFormat.nAvgBytesPerSec) / 1000.0)
                    );
                    uint32_t uiDelta = BSAudio::pAudioInstance->SynchTimer.iMilliseconds - iSynchStartTime;
                    int32_t iTimeDiff = 0;
                    if (uiPositionMS <= uiDelta) {
                        if (uiPositionMS < uiDelta) {
                            iTimeDiff = uiDelta - uiPositionMS;
                        }
                    }
                    else {
                        iTimeDiff = uiDelta - uiPositionMS;
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
                            bStopped = true;
                        }
                        if (!bStopped) {
                            pSoundBuffer->SetCurrentPosition(iNewTime);
                        }
                    }
                }
            }
        }
        else {
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
                        pEnvelope->uiSize,
                        pEnvelope->uiPlayCursor,
                        dwWriteBytes,
                        reinterpret_cast<char*>(pEnvelope->pHeadBuf)
                    );
                    pEnvelope->uiSize = 0;
                    pEnvelope->uiState = 2;
                }
                else {
                    Write(
                        pEnvelope->uiSize,
                        pEnvelope->uiPlayCursor,
                        dwWriteBytes,
                        reinterpret_cast<char*>(pEnvelope->pHeadBuf)
                    );
                    pEnvelope->uiSize += dwWriteBytes;
                }
            }
            else if (pEnvelope->uiState == 2 && pEnvelope->uiMiddleSize) {
                bool bWrapped = false;
                int32_t auiSize = 0;
                if (dwWriteBytes > pEnvelope->uiMiddleSize - pEnvelope->uiSize) {
                    auiSize = dwWriteBytes - (pEnvelope->uiMiddleSize - pEnvelope->uiSize);
                    dwWriteBytes = pEnvelope->uiMiddleSize - pEnvelope->uiSize;
                    bWrapped = true;
                }
                Write(
                    pEnvelope->uiSize,
                    pEnvelope->uiPlayCursor,
                    dwWriteBytes,
                    reinterpret_cast<char*>(pEnvelope->pMidBuf)
                );
                pEnvelope->uiSize = (dwWriteBytes + pEnvelope->uiSize) % pEnvelope->uiMiddleSize;
                if (bWrapped) {
                    pEnvelope->uiPlayCursor += dwWriteBytes;
                    Write(
                        pEnvelope->uiSize,
                        pEnvelope->uiPlayCursor,
                        auiSize,
                        reinterpret_cast<char*>(pEnvelope->pMidBuf)
                    );
                    pEnvelope->uiSize = auiSize;
                }
            }
            else if (pEnvelope->uiState != 4) {
                if (pEnvelope->uiState == 5) {
                    if (dwWriteBytes >= pEnvelope->uiTailSize - pEnvelope->uiSize) {
                        pSoundBuffer->Stop();
                        dwWriteBytes = pEnvelope->uiTailSize - pEnvelope->uiSize;
                        Write(
                            pEnvelope->uiSize,
                            pEnvelope->uiPlayCursor,
                            dwWriteBytes,
                            reinterpret_cast<char*>(pEnvelope->pTailBuf)
                        );
                        pSoundBuffer->Play(0, 0, 0);
                        pEnvelope->uiState = 6;
                        iStateFlags &= ~BSGameSound::STATE_PLAYING;
                        iStateFlags &= ~BSGameSound::STATE_PAUSED;
                    }
                    else {
                        Write(
                            pEnvelope->uiSize,
                            pEnvelope->uiPlayCursor,
                            dwWriteBytes,
                            reinterpret_cast<char*>(pEnvelope->pTailBuf)
                        );
                        pEnvelope->uiSize += dwWriteBytes;
                    }
                }
            }
            else {
                if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) == 0) {
                    if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0) {
                        pEnvelope->uiState = 3;
                        pSoundBuffer->Stop();
                        pSoundBuffer->Play(0, 0, 0);
                    }
                }
                else {
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
            }
            if (bResetPlayCursor) {
                pEnvelope->uiPlayCursor = 0;
            }
            else {
                pEnvelope->uiPlayCursor = uiPlayCursor;
            }
        }
    }
    UpdateEmitterPosition();
    return true;
}

bool BSWin32GameSound::ProcessDataRequest(int32_t aiRequestNum) {
    return false;
}

bool BSWin32GameSound::SetEmitterPosition(NiPoint3 vin) {
    return SetEmitterPosition(vin.x, vin.y, vin.z);
}

bool BSWin32GameSound::SetEmitterPosition(float afX, float afY, float afZ) {
    NiPoint3 kPos = NiPoint3::ZERO;
    NiPoint3 kListenerPos = NiPoint3::ZERO;
    bool bPositionChanged = false;
    bool bPositional = Is3DSound();
    if ((bPositional || (bPositional = Is2DRadiusSound())) && pDirectSound8) {
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
        bPositionSet = true;
        if (p3DSoundBuffer) {
            if (((iSoundTypeFlags & BSGameSound::SOUND_FLAG_BEAM_EMITTER) != 0 &&
                 (iStateFlags & BSGameSound::STATE_IS_VALID_BEAM) != 0) ||
                !bPositionChanged) {
                if (BSAudio::pAudioInstance->pListener &&
                    ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_BEAM_EMITTER) != 0 &&
                     (iStateFlags & BSGameSound::STATE_IS_VALID_BEAM) != 0)) {
                    float fProjection = 0.0;
                    float fLengthSquared = 0.0;
                    float fRatio = 0.0;
                    NiPoint3 kCurrentListenerPos = BSAudio::pAudioInstance->pListener->GetPosition();
                    NiPoint3 pt = vBeamEnd - kPosition;
                    NiPoint3 kToListener = kCurrentListenerPos - kPosition;
                    fProjection = kToListener.Dot(pt);
                    if (!(fProjection <= 0.0)) {
                        fLengthSquared = pt.Dot(pt);
                        if (!(fProjection >= static_cast<double>(fLengthSquared))) {
                            fRatio = fProjection / fLengthSquared;
                            kPos = kPosition + fRatio * pt;
                        }
                        else {
                            kPos = vBeamEnd;
                        }
                    }
                    else {
                        kPos = kPosition;
                    }
                    // DirectSound's y is the game's z: the position is passed as x, z, y.
                    float fSoundZ = kPos.y * 0.014287755824625492;
                    float fSoundY = kPos.z * 0.014287755824625492;
                    float fSoundX = kPos.x * 0.014287755824625492;
                    p3DSoundBuffer->SetPosition(fSoundX, fSoundY, fSoundZ, 1);
                    if (p3DReverbBuffer) {
                        float fReverbZ = kPos.y * 0.014287755824625492;
                        float fReverbY = kPos.z * 0.014287755824625492;
                        float fReverbX = kPos.x * 0.014287755824625492;
                        p3DReverbBuffer->SetPosition(fReverbX, fReverbY, fReverbZ, 1);
                    }
                }
            }
            else {
                float fSoundZ = afY * 0.014287755824625492;
                float fSoundY = afZ * 0.014287755824625492;
                float fSoundX = afX * 0.014287755824625492;
                p3DSoundBuffer->SetPosition(fSoundX, fSoundY, fSoundZ, 1);
                if (p3DReverbBuffer) {
                    float fReverbZ = afY * 0.014287755824625492;
                    float fReverbY = afZ * 0.014287755824625492;
                    float fReverbX = afX * 0.014287755824625492;
                    p3DReverbBuffer->SetPosition(fReverbX, fReverbY, fReverbZ, 1);
                }
            }
        }
        float fDistance = (kListenerPos - kPosition).Length();
        this->fDistanceToListener = fDistance;
        float fDistanceToListener = this->fDistanceToListener;
        if (((iSoundTypeFlags & BSGameSound::SOUND_FLAG_BEAM_EMITTER) != 0 &&
             (iStateFlags & BSGameSound::STATE_IS_VALID_BEAM) != 0) &&
            BSAudio::pAudioInstance->pListener) {
            NiPoint3* pListenerPos = &BSAudio::pAudioInstance->pListener->GetPosition();
            fDistanceToListener = (kPos - *pListenerPos).Length();
        }

        float fRange = fMaxDist - fMinDist;
        float fAbsRange = std::fabs(static_cast<double>(fRange));
        if (!(fAbsRange > 10.0)) {
            if (!(fMaxDist <= static_cast<double>(fDistanceToListener))) {
                iDistanceAttenuation = 0;
            }
            else {
                iDistanceAttenuation = 10000;
            }
            SetVolume(fVolume);
        }
        else {
            float fQuarterDist = (fMaxDist - fMinDist) * 0.25 + fMinDist;
            float fHalfDist = (fMaxDist - fMinDist) * 0.5 + fMinDist;
            float fThreeQuarterDist = (fMaxDist - fMinDist) * 0.75 + fMinDist;
            if (!(fMaxDist <= static_cast<double>(fDistanceToListener))) {
                if (!(fThreeQuarterDist < static_cast<double>(fDistanceToListener)) ||
                    !(fMaxDist - fThreeQuarterDist > 0.0)) {
                    if (!(fHalfDist < static_cast<double>(fDistanceToListener)) ||
                        !(fThreeQuarterDist - fHalfDist > 0.0)) {
                        if (!(fQuarterDist < static_cast<double>(fDistanceToListener)) ||
                            !(fHalfDist - fQuarterDist > 0.0)) {
                            if (!(fMinDist < static_cast<double>(fDistanceToListener)) ||
                                !(fQuarterDist - fMinDist > 0.0)) {
                                iDistanceAttenuation = 0;
                                SetVolume(fVolume);
                            }
                            else {
                                iDistanceAttenuation = static_cast<int32_t>(
                                    (fDistanceToListener - fMinDist) / (fQuarterDist - fMinDist) *
                                        static_cast<double>(usAttenuationCurve[1] - usAttenuationCurve[0]) +
                                    static_cast<double>(usAttenuationCurve[0])
                                );
                                SetVolume(fVolume);
                            }
                        }
                        else {
                            iDistanceAttenuation = static_cast<int32_t>(
                                (fDistanceToListener - fQuarterDist) / (fHalfDist - fQuarterDist) *
                                    static_cast<double>(usAttenuationCurve[2] - usAttenuationCurve[1]) +
                                static_cast<double>(usAttenuationCurve[1])
                            );
                            SetVolume(fVolume);
                        }
                    }
                    else {
                        iDistanceAttenuation = static_cast<int32_t>(
                            (fDistanceToListener - fHalfDist) / (fThreeQuarterDist - fHalfDist) *
                                static_cast<double>(usAttenuationCurve[3] - usAttenuationCurve[2]) +
                            static_cast<double>(usAttenuationCurve[2])
                        );
                        SetVolume(fVolume);
                    }
                }
                else {
                    iDistanceAttenuation = static_cast<int32_t>(
                        (fDistanceToListener - fThreeQuarterDist) / (fMaxDist - fThreeQuarterDist) *
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

    return bPositional;
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
        if (bPositionSet) {
            SetEmitterPosition(kPosition);
        }
    }
    if (Is3DSound() && p3DSoundBuffer && pDirectSound8) {
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

void BSWin32GameSound::SetAttenuationCurve(
    uint16_t aiPt1,
    uint16_t aiPt2,
    uint16_t aiPt3,
    uint16_t aiPt4,
    uint16_t aiPt5
) {
    double dAttenuation5;
    double dAttenuation4;
    double dAttenuation3;
    double dAttenuation2;
    double dAttenuation1;
    if (aiPt1) {
        float fRatio = static_cast<double>(aiPt1) / 100.0;
        float fLog = std::log10(static_cast<double>(fRatio));
        dAttenuation1 = fLog * -2000.0;
    }
    else {
        dAttenuation1 = 5000.0;
    }
    usAttenuationCurve[0] = static_cast<int32_t>(dAttenuation1);
    if (aiPt2) {
        float fRatio = static_cast<double>(aiPt2) / 100.0;
        float fLog = std::log10(static_cast<double>(fRatio));
        dAttenuation2 = fLog * -2000.0;
    }
    else {
        dAttenuation2 = 5000.0;
    }
    usAttenuationCurve[1] = static_cast<int32_t>(dAttenuation2);
    if (aiPt3) {
        float fRatio = static_cast<double>(aiPt3) / 100.0;
        float fLog = std::log10(static_cast<double>(fRatio));
        dAttenuation3 = fLog * -2000.0;
    }
    else {
        dAttenuation3 = 5000.0;
    }
    usAttenuationCurve[2] = static_cast<int32_t>(dAttenuation3);
    if (aiPt4) {
        float fRatio = static_cast<double>(aiPt4) / 100.0;
        float fLog = std::log10(static_cast<double>(fRatio));
        dAttenuation4 = fLog * -2000.0;
    }
    else {
        dAttenuation4 = 5000.0;
    }
    usAttenuationCurve[3] = static_cast<int32_t>(dAttenuation4);
    if (aiPt5) {
        float fRatio = static_cast<double>(aiPt5) / 100.0;
        float fLog = std::log10(static_cast<double>(fRatio));
        dAttenuation5 = fLog * -2000.0;
    }
    else {
        dAttenuation5 = 5000.0;
    }
    usAttenuationCurve[4] = static_cast<int32_t>(dAttenuation5);
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
    if (pSoundBuffer && pDirectSound8) {
        eResult = pSoundBuffer->SetFrequency(uiFreq);
    }

    return eResult >= 0;
}

float BSWin32GameSound::GetFrequency() {
    uint32_t uiFrequency;
    HRESULT eResult = E_FAIL;
    uiFrequency = 0;
    float fResult = 1.0f;
    if (pSoundBuffer && pDirectSound8) {
        eResult = pSoundBuffer->GetFrequency(&uiFrequency);
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
    pOggFile = new BSWin32GameSound::OggVorbisFile;
    if (pOggFile->OpenFile(pFilename)) {
        uint32_t uiChannels;
        int32_t uiChannelCount;
        int32_t iChannels;
        uint16_t usChannels;
        uint32_t rate;
        BSWin32GameSound::OggVorbisFile* pOggForFlags;
        uint32_t Size;
        if (((iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_SLOW) != 0 ||
             (iSoundTypeFlags & BSGameSound::SOUND_FLAG_ENVELOPE_FAST) != 0) &&
            ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_MAYBE_UNDERWATER) != 0 ||
             (iSoundTypeFlags & BSGameSound::SOUND_FLAG_REGION) != 0)) {
            iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(iSoundTypeFlags | BSGameSound::SOUND_FLAG_LOOP);
        }
        iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(
            iSoundTypeFlags & ~BSGameSound::SOUND_FLAG_ENVELOPE_FAST
        );
        iSoundTypeFlags = static_cast<BSGameSound::SoundFlags>(
            iSoundTypeFlags & ~BSGameSound::SOUND_FLAG_ENVELOPE_SLOW
        );
        bool bRadio = (iSoundTypeFlags & BSGameSound::SOUND_FLAG_UNKBIT22) != 0 ||
            (iSoundTypeFlags & BSGameSound::SOUND_FLAG_RADIO) != 0;
        kBufferDesc.dwSize = sizeof(DSBUFFERDESC);
        kBufferDesc.lpwfxFormat->cbSize = 0;
        kBufferDesc.lpwfxFormat->wFormatTag = 1;
        BSWin32GameSound::OggVorbisFile* pOggForRate = pOggFile;
        if (pOggForRate->pInfo) {
            // vorbis_info's rate is a C long, which libvorbis reads from the header's 32-bit unsigned field and
            // accepts only from 1 up: uint32_t holds it.
            rate = static_cast<uint32_t>(pOggForRate->pInfo->rate);
        }
        else {
            rate = 0;
        }
        kBufferDesc.lpwfxFormat->nSamplesPerSec = rate;
        kBufferDesc.lpwfxFormat->wBitsPerSample = 16;
        BSWin32GameSound::OggVorbisFile* pOggForChannels = pOggFile;
        // Only the channel count's low word is stored (nChannels is a WORD).
        if (pOggForChannels->pInfo) {
            usChannels = pOggForChannels->pInfo->channels;
        }
        else {
            usChannels = 1;
        }
        kBufferDesc.lpwfxFormat->nChannels = usChannels;
        kBufferDesc.lpwfxFormat->nBlockAlign = kBufferDesc.lpwfxFormat->nChannels *
            (kBufferDesc.lpwfxFormat->wBitsPerSample / 8);
        kBufferDesc.lpwfxFormat->nAvgBytesPerSec = kBufferDesc.lpwfxFormat->nSamplesPerSec *
            kBufferDesc.lpwfxFormat->nBlockAlign;
        BSWin32GameSound::OggVorbisFile* pOGGFile = pOggFile;
        if (pOGGFile->pInfo) {
            iChannels = pOGGFile->pInfo->channels;
        }
        else {
            iChannels = 1;
        }
        kBufferDesc.dwBufferBytes = iChannels * (2 * pOGGFile->iSampleCount);
        uiBufferBytes = kBufferDesc.dwBufferBytes;
        if (bRadio) {
            Size = kBufferDesc.dwBufferBytes + (kBufferDesc.lpwfxFormat->nAvgBytesPerSec >> 1);
        }
        else {
            Size = kBufferDesc.dwBufferBytes;
        }
        pRawBuffer = static_cast<uint8_t*>(MemoryManager::Instance().Allocate(Size));
        std::memset(pRawBuffer, 0, Size);
        iSamplesPerSecond = kBufferDesc.lpwfxFormat->nSamplesPerSec;
        iModSamplesPerSecond = iSamplesPerSecond;
        BSWin32GameSound::OggVorbisFile* pOggForDuration = pOggFile;
        if (pOggForDuration->pInfo) {
            uiChannelCount = pOggForDuration->pInfo->channels;
        }
        else {
            uiChannelCount = 1;
        }
        iDurationInMS = uiBufferBytes / (2 * uiChannelCount) / (iSamplesPerSecond / 1000);
        pOggFile->Read(pRawBuffer, kBufferDesc.dwBufferBytes);
        if ((iSoundTypeFlags & BSGameSound::SOUND_FLAG_IS_2D) != 0 ||
            ((pOggForFlags = pOggFile, !pOggForFlags->pInfo) ? uiChannels = 1
                                                             : uiChannels = pOggForFlags->pInfo->channels,
             uiChannels > 1)) {
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

    BSWin32GameSound::OggVorbisFile* Block = pOggFile;
    if (Block) {
        Block->Destroy();
        MemoryManager::Instance().Deallocate(Block);
    }
    pOggFile = nullptr;
    return false;
}

bool BSWin32GameSound::PrepareReverb() {
    Setting::Info* iMaxFX;
    Setting::Info* bEnableEnviroEffectsOnPC;
    uint32_t uiDestBytes2;
    void* pvDestPtr1;
    void* pvDestPtr2;
    DSEFFECTDESC kEffectDesc;
    uint32_t uiDestBytes1;
    DSBUFFERDESC kBufferDesc;
    void* pvSrcPtr1;
    uint32_t uiSrcBytes2;
    void* pvSrcPtr2;
    uint32_t uiSrcBytes1;
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
    pvDestPtr1 = nullptr;
    pvDestPtr2 = nullptr;
    uiDestBytes1 = 0;
    uiDestBytes2 = 0;
    pvSrcPtr1 = nullptr;
    pvSrcPtr2 = nullptr;
    uiSrcBytes1 = 0;
    uiSrcBytes2 = 0;
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
    kBufferDesc.lpwfxFormat = static_cast<tWAVEFORMATEX*>(MemoryManager::Instance().Allocate(sizeof(tWAVEFORMATEX)));
    pSoundBuffer->GetFormat(kBufferDesc.lpwfxFormat, sizeof(WAVEFORMATEX), &uiDestBytes1);
    ++BSWin32GameSound::uiEffectCount;
    if (pDirectSound8->CreateSoundBuffer(&kBufferDesc, &pTempReverbBuffer, nullptr) >= 0 &&
        (pTempReverbBuffer->QueryInterface(CLSID_IDirectSoundBuffer8, reinterpret_cast<void**>(&pReverbBuffer)),
         pReverbBuffer)) {
        pReverbBuffer->QueryInterface(CLSID_IDirectSound3DBuffer, reinterpret_cast<void**>(&p3DReverbBuffer));
        kEffectDesc.dwSize = sizeof(DSEFFECTDESC);
        kEffectDesc.dwFlags = 0;
        kEffectDesc.guidDSFXClass = CLSID_GUID_DSFX_STANDARD_I3DL2REVERB;
        kEffectDesc.dwReserved1 = 0;
        kEffectDesc.dwReserved2 = 0;
        pReverbBuffer->SetFX(1, &kEffectDesc, nullptr);
        int32_t eResult = pReverbBuffer->GetObjectInPath(
            &CLSID_GUID_DSFX_STANDARD_I3DL2REVERB,
            0,
            &CLSID_IDirectSoundFXI3DL2Reverb,
            reinterpret_cast<void**>(&pReverbEffect)
        );
        if (eResult >= 0) {
            uint32_t eEnvironmentType = this->iEnvironmentType;
            if (BSAudio::pAudioInstance->pListener) {
                eEnvironmentType = BSAudio::pAudioInstance->pListener->GetEnvironmentType();
            }
            if (eEnvironmentType) {
                pReverbEffect->SetPreset(eEnvironmentType - 1);
            }
        }
        uiDestBytes1 = 0;
        if (pReverbBuffer->Lock(0, 0, &pvDestPtr1, &uiDestBytes1, &pvDestPtr2, &uiDestBytes2, 2) >= 0) {
            if (pSoundBuffer->Lock(0, 0, &pvSrcPtr1, &uiSrcBytes1, &pvSrcPtr2, &uiSrcBytes2, 2) >= 0) {
                std::memset(pvDestPtr1, 0, kBufferDesc.dwBufferBytes);
                rt::memcpy(pvDestPtr1, pvSrcPtr1, this->kBufferDesc.dwBufferBytes);
                pSoundBuffer->Unlock(pvSrcPtr1, uiSrcBytes1, pvSrcPtr2, uiSrcBytes2);
            }
            pReverbBuffer->Unlock(pvDestPtr1, uiDestBytes1, pvDestPtr2, uiDestBytes2);
            MemoryManager::Instance().Deallocate(kBufferDesc.lpwfxFormat);
            return true;
        }

        MemoryManager::Instance().Deallocate(kBufferDesc.lpwfxFormat);
        return false;
    }

    MemoryManager::Instance().Deallocate(kBufferDesc.lpwfxFormat);
    return false;
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
