#include "Fallout Shared/LipSynchAnim.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/Archive.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "SDK/MSVC/string.h"

// The game settings this file defines, with their IDs and values, in their initialization order.
SettingT<INISettingCollection> fDialogueHeadPitchExaggeration("fDialogueHeadPitchExaggeration:Audio", 2.0f);
SettingT<INISettingCollection> fDialogueHeadRollExaggeration("fDialogueHeadRollExaggeration:Audio", 2.0f);
SettingT<INISettingCollection> fDialogueHeadYawExaggeration("fDialogueHeadYawExaggeration:Audio", 2.0f);

// One zeroed block holds every frame's 16 phoneme and 17 modifier values; the two row tables point into it.
LipSynchAnim::LipSynchAnim(uint32_t aiNumFrames, int32_t aiStartingFrame)
  : iNumFrames(aiNumFrames), iStartingFrame(aiStartingFrame),
    ppPhonemes(static_cast<float**>(ourMalloc(sizeof(float*) * aiNumFrames))),
    ppModifiers(static_cast<float**>(ourMalloc(sizeof(float*) * aiNumFrames))) {
    float* pfloatBuffer = static_cast<float*>(ourMalloc(sizeof(float) * 33 * iNumFrames));
    ourMemset(pfloatBuffer, 0, sizeof(float) * 33 * iNumFrames);
    for (uint32_t iframe = 0; iframe < iNumFrames; ++iframe) {
        ppPhonemes[iframe] = pfloatBuffer;
        ppModifiers[iframe] = pfloatBuffer + 16;
        pfloatBuffer += 33;
    }
}

// The first phoneme row is the start of the value block.
LipSynchAnim::~LipSynchAnim() {
    ourFree(ppPhonemes[0]);
    ourFree(ppPhonemes);
    ourFree(ppModifiers);
}

float LipSynchAnim::GetStartingDelay() const {
    if (iStartingFrame >= 0) {
        return 0.0f;
    }

    return static_cast<float>(-iStartingFrame * 0.03333333507180214);
}

float LipSynchAnim::GetPuppetValue(int32_t aiFrame) {
    int32_t iFrame = aiFrame - iStartingFrame;
    if (iFrame >= 0 && iFrame < iNumFrames) {
        float fSmoothed;
        int32_t iPhoneme = 0;
        float fMaxWeight = *ppPhonemes[iFrame] * fPhonemeCoefficients[0];
        for (int32_t i = 1; i < 16; ++i) {
            if (fMaxWeight < ppPhonemes[iFrame][i] * fPhonemeCoefficients[i]) {
                iPhoneme = i;
                fMaxWeight = ppPhonemes[iFrame][i] * fPhonemeCoefficients[i];
            }
        }
        if (iFrame <= 1 || iFrame >= iNumFrames - 2) {
            if (iFrame <= 0) {
                if (iFrame >= iNumFrames - 1) {
                    fSmoothed = ppPhonemes[iFrame][iPhoneme];
                }
                else {
                    fSmoothed = (ppPhonemes[iFrame][iPhoneme] + ppPhonemes[iFrame + 1][iPhoneme]) / 2.0;
                }
            }
            else {
                fSmoothed = (ppPhonemes[iFrame][iPhoneme] + ppPhonemes[iFrame - 1][iPhoneme]) / 2.0;
            }
        }
        else {
            fSmoothed = (ppPhonemes[iFrame][iPhoneme] + ppPhonemes[iFrame - 1][iPhoneme] +
                         ppPhonemes[iFrame + 1][iPhoneme] + ppPhonemes[iFrame - 2][iPhoneme] +
                         ppPhonemes[iFrame + 2][iPhoneme]) /
                5.0;
        }
        return fSmoothed * fPhonemeCoefficients[iPhoneme];
    }

    return 0.0f;
}

bool LipSynchAnim::ApplyToNode(BSFaceGenAnimationData* apData, float afEmptyAtEnd) {
    if (!apData) {
        return false;
    }
    float fDelay = GetStartingDelay();
    float fBlendTime = BSMIN(0.2f, fDelay);
    apData->ResetSpeechCurrentMorphs(fBlendTime, false, true, true, true, false);
    for (uint32_t i = 0; i < iNumFrames; ++i) {
        float fModifiers[18];
        float fPhonemes[16];
        ourMemcpy(fPhonemes, ppPhonemes[i], sizeof(fPhonemes));
        ourMemcpy(fModifiers, ppModifiers[i], sizeof(float) * 17);
        apData->AddPhonemeKeyframe(fPhonemes, 0.033333335f);
        apData->AddModifiersKeyframe(fModifiers, 0.033333335f);
    }
    if (afEmptyAtEnd > 0.0) {
        apData->AddSpeechEmptyKeyframe(BS_FG_MORPH_PHONEME, afEmptyAtEnd, true);
        apData->AddSpeechEmptyKeyframe(BS_FG_MORPH_MODIFIER, afEmptyAtEnd, true);
    }
    return true;
}

void LipSynchAnim::operator delete(void* apMemory) {
    ourFree(apMemory);
}

LipSynchAnim* LipSynchAnim::LoadFromFile(const char* apName) {
    if (!apName || !FileFinder::Locate(apName, nullptr, FileFinder::SKIP_NONE, -1)) {
        return nullptr;
    }
    ArchiveFile* pFile = static_cast<ArchiveFile*>(FileFinder::GetFile(apName, NiFile::READ_ONLY, 16384, 16));
    return LipSynchAnim::LoadFromFile(pFile);
}

LipSynchAnim* LipSynchAnim::LoadFromFile(const BSStringT<char>& aFilename) {
    return LipSynchAnim::LoadFromFile(static_cast<const char*>(aFilename.pString));
}

// The header is read whole into three locals: version, unpacked size, compressed flag. A file of another version is one
// version byte and run-length data (a nonzero byte is one byte; a zero byte is followed by a 16-bit run length, read
// from the raw, untyped buffer), scanned once to count its unpacked size.
LipSynchAnim* LipSynchAnim::LoadFromFile(BSFile* apFile) {
    if (!apFile || !apFile->Open(true, false)) {
        delete apFile;
        return nullptr;
    }
    uint32_t uiFileSize = apFile->GetSize();
    uint32_t auiHeader[3];
    apFile->NiBinaryStream::Read(auiHeader, 12);
    char* pData = nullptr;
    if (auiHeader[0] != 1) {
        uiFileSize -= 1;
        apFile->Seek(1, NiFile::ms_iSeekSet);
        char* pPacked = static_cast<char*>(ourMalloc(uiFileSize));
        apFile->NiBinaryStream::Read(pPacked, uiFileSize);
        char* pEnd = pPacked + uiFileSize;
        uint32_t uiSize = 0;
        uint32_t uiPos = 0;
        while (pPacked + uiPos < pEnd) {
            if (pPacked[uiPos]) {
                ++uiSize;
                ++uiPos;
            }
            else {
                ++uiPos;
                uiSize += *reinterpret_cast<uint16_t*>(pPacked + uiPos);
                uiPos += 2;
            }
        }
        auiHeader[1] = uiSize;
        auiHeader[0] = 1;
        auiHeader[2] = 1;
        apFile->Seek(1, NiFile::ms_iSeekSet);
        ourFree(pPacked);
    }
    else {
        uiFileSize -= 12;
    }
    pData = static_cast<char*>(ourMalloc(auiHeader[1] + 16));
    if (auiHeader[2] & 1) {
        Decompress(apFile, pData, uiFileSize, auiHeader[1]);
    }
    else {
        apFile->NiBinaryStream::Read(pData, auiHeader[1]);
    }
    LipSynchAnim* pAnim = Scatter(pData, auiHeader[1]);
    delete apFile;
    ourFree(pData);
    return pAnim;
}

// The buffer is the unpacked.lip data: the frame count (at most 1000000), the starting frame, then per frame 16 phoneme
// and 17 modifier floats, read through casts of the raw buffer. A buffer that ends early deletes the animation and
// returns null.
LipSynchAnim* LipSynchAnim::Scatter(const char* pBuffer, uint32_t nBufferSize) {
    LipSynchAnim* pRet;
    if (!pBuffer || !nBufferSize) {
        return nullptr;
    }
    if (nBufferSize < 4) {
        return nullptr;
    }
    uint32_t nNumFrames = *reinterpret_cast<const uint32_t*>(pBuffer);
    if (*reinterpret_cast<const uint32_t*>(pBuffer) > 1000000u) {
        return nullptr;
    }
    if (nBufferSize < 8) {
        return nullptr;
    }
    int32_t nStartingFrame = reinterpret_cast<const int32_t*>(pBuffer)[1];
    uint32_t nIndex = 8;
    pRet = new LipSynchAnim(nNumFrames, nStartingFrame);
    for (uint32_t nFrame = 0; nFrame < nNumFrames; ++nFrame) {
        for (uint32_t nPhon = 0; nPhon < 16; ++nPhon) {
            if (nIndex + 4 > nBufferSize) {
                if (pRet) {
                    delete pRet;
                }
                return nullptr;
            }
            pRet->ppPhonemes[nFrame][nPhon] = *reinterpret_cast<const float*>(&pBuffer[nIndex]);
            nIndex += 4;
        }
        for (uint32_t nMod = 0; nMod < 17; ++nMod) {
            if (nIndex + 4 > nBufferSize) {
                if (pRet) {
                    delete pRet;
                }
                return nullptr;
            }
            pRet->ppModifiers[nFrame][nMod] = *reinterpret_cast<const float*>(&pBuffer[nIndex]);
            nIndex += 4;
        }
        float* pModifiers = pRet->ppModifiers[nFrame];
        pModifiers[14] = pModifiers[14] * fDialogueHeadPitchExaggeration.Float();
        pModifiers = pRet->ppModifiers[nFrame];
        pModifiers[15] = pModifiers[15] * fDialogueHeadRollExaggeration.Float();
        pModifiers = pRet->ppModifiers[nFrame];
        pModifiers[16] = pModifiers[16] * fDialogueHeadYawExaggeration.Float();
    }
    return pRet;
}

bool LipSynchAnim::Decompress(BSFile* apFile, char* apDest, uint32_t aiBufferSize, uint32_t aiDecompSize) {
    char c;
    bool bok = true;
    uint32_t iprocessed = aiBufferSize;
    while (iprocessed) {
        c = 0;
        if (!apFile->NiBinaryStream::Read(&c, 1u)) {
            iprocessed = 0;
            return false;
        }
        if (c) {
            *apDest++ = c;
            --iprocessed;
        }
        else {
            uint16_t usRunLength[3];
            usRunLength[0] = 0;
            apFile->NiBinaryStream::Read(usRunLength, 2u);
            ourMemset(apDest, 0, usRunLength[0]);
            apDest += usRunLength[0];
            iprocessed -= 3;
        }
    }
    return bok;
}

bool LipSynchAnim::ChangeFilenameToLipSynch(BSStringT<char>& aFilename) {
    char cfilename[268];
    BSstrcpy(cfilename, 260, aFilename.pString);
    char* pext = std::strrchr(cfilename, '.');
    if (!pext) {
        return false;
    }
    *pext = 0;
    aFilename.SPrintF("%s.lip", cfilename);
    return true;
}
