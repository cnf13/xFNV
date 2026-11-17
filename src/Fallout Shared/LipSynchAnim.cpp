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
        return (float)0.0;
    }
    else {
        return (float)(((double)(-iStartingFrame)) * 0.03333333507180214);
    }
}

float LipSynchAnim::GetPuppetValue(int32_t aiFrame) {
    int32_t v2;
    int32_t v10 = aiFrame - iStartingFrame;
    if (v10 >= 0 && (v2 = aiFrame - iStartingFrame, v10 < iNumFrames)) {
        float v9;
        int32_t v11 = 0;
        int32_t v4 = aiFrame - iStartingFrame;
        float v12 = *ppPhonemes[v10] * fPhonemeCoefficients[0];
        for (int32_t i = 1; i < 16; ++i) {
            if (v12 < ppPhonemes[v10][i] * fPhonemeCoefficients[i]) {
                v11 = i;
                v12 = ppPhonemes[v10][i] * fPhonemeCoefficients[i];
            }
        }
        float v8 = ppPhonemes[v10][v11];
        if (v10 <= 1 || v10 >= iNumFrames - 2) {
            if (v10 <= 0) {
                if (v10 >= iNumFrames - 1) {
                    v9 = ppPhonemes[v10][v11];
                }
                else {
                    v9 = (ppPhonemes[v10][v11] + ppPhonemes[v10 + 1][v11]) / 2.0;
                }
            }
            else {
                v9 = (ppPhonemes[v10][v11] + ppPhonemes[v10 - 1][v11]) / 2.0;
            }
        }
        else {
            v9 = (ppPhonemes[v10][v11] + ppPhonemes[v10 - 1][v11] + ppPhonemes[v10 + 1][v11] +
                  ppPhonemes[v10 - 2][v11] + ppPhonemes[v10 + 2][v11]) /
                5.0;
        }
        return v9 * fPhonemeCoefficients[v11];
    }
    else {
        return 0.0f;
    }
}

bool LipSynchAnim::ApplyToNode(BSFaceGenAnimationData* apData, float afEmptyAtEnd) {
    if (!apData) {
        return 0;
    }
    float afB = GetStartingDelay();
    float afBa = BSMIN(0.2f, afB);
    apData->ResetSpeechCurrentMorphs(afBa, 0, 1, 1, 1, 0);
    for (uint32_t i = 0; i < iNumFrames; ++i) {
        float v8[18];
        float apDest[16];
        ourMemcpy(apDest, ppPhonemes[i], sizeof(apDest));
        ourMemcpy(v8, ppModifiers[i], sizeof(float) * 17);
        apData->AddPhonemeKeyframe(apDest, 0.033333335f);
        apData->AddModifiersKeyframe(v8, 0.033333335f);
    }
    if (afEmptyAtEnd > 0.0) {
        apData->AddSpeechEmptyKeyframe(BS_FG_MORPH_PHONEME, afEmptyAtEnd, 1);
        apData->AddSpeechEmptyKeyframe(BS_FG_MORPH_MODIFIER, afEmptyAtEnd, 1);
    }
    return 1;
}

void LipSynchAnim::operator delete(void* apMemory) {
    ourFree(apMemory);
}

LipSynchAnim* LipSynchAnim::LoadFromFile(const char* apName) {
    if ((!apName) || (!FileFinder::Locate(apName, nullptr, FileFinder::SKIP_NONE, (-1)))) {
        return nullptr;
    }
    ArchiveFile* file = static_cast<ArchiveFile*>(FileFinder::GetFile(apName, NiFile::READ_ONLY, 16384, 16));
    return LipSynchAnim::LoadFromFile(file);
}

LipSynchAnim* LipSynchAnim::LoadFromFile(const BSStringT<char>& aFilename) {
    return LipSynchAnim::LoadFromFile(static_cast<const char*>(aFilename.pString));
}

// The header is read whole into three locals: version, unpacked size, compressed flag. A file of another version is one
// version byte and run-length data (a nonzero byte is one byte; a zero byte is followed by a 16-bit run length, read
// from the raw, untyped buffer), scanned once to count its unpacked size.
LipSynchAnim* LipSynchAnim::LoadFromFile(BSFile* apFile) {
    if (!apFile || !apFile->Open(1, false)) {
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
        return 0;
    }
    if (nBufferSize < 4) {
        return 0;
    }
    uint32_t nNumFrames = *reinterpret_cast<const uint32_t*>(pBuffer);
    if (*reinterpret_cast<const uint32_t*>(pBuffer) > 1000000u) {
        return 0;
    }
    if (nBufferSize < 8) {
        return 0;
    }
    int32_t nStartingFrame = reinterpret_cast<const int32_t*>(pBuffer)[1];
    uint32_t nIndex = 8;
    LipSynchAnim* apMemory = static_cast<LipSynchAnim*>(ourMalloc(sizeof(LipSynchAnim)));
    if (apMemory) {
        pRet = ::new (apMemory) LipSynchAnim(nNumFrames, nStartingFrame);
    }
    else {
        pRet = 0;
    }
    for (uint32_t nFrame = 0; nFrame < nNumFrames; ++nFrame) {
        for (uint32_t nPhon = 0; nPhon < 16; ++nPhon) {
            if (nIndex + 4 > nBufferSize) {
                if (pRet) {
                    delete pRet;
                }
                return 0;
            }
            pRet->ppPhonemes[nFrame][nPhon] = *reinterpret_cast<const float*>(&pBuffer[nIndex]);
            nIndex += 4;
        }
        for (uint32_t nMod = 0; nMod < 17; ++nMod) {
            if (nIndex + 4 > nBufferSize) {
                if (pRet) {
                    delete pRet;
                }
                return 0;
            }
            pRet->ppModifiers[nFrame][nMod] = *reinterpret_cast<const float*>(&pBuffer[nIndex]);
            nIndex += 4;
        }
        float* v3 = pRet->ppModifiers[nFrame];
        v3[14] = v3[14] * fDialogueHeadPitchExaggeration.Float();
        float* v4 = pRet->ppModifiers[nFrame];
        v4[15] = v4[15] * fDialogueHeadRollExaggeration.Float();
        float* v5 = pRet->ppModifiers[nFrame];
        v5[16] = v5[16] * fDialogueHeadYawExaggeration.Float();
    }
    return pRet;
}

bool LipSynchAnim::Decompress(BSFile* apFile, char* apDest, uint32_t aiBufferSize, uint32_t aiDecompSize) {
    char c;
    bool bok = 1;
    uint32_t iprocessed = aiBufferSize;
    while (iprocessed) {
        c = 0;
        if (!apFile->NiBinaryStream::Read(&c, 1u)) {
            iprocessed = 0;
            return 0;
        }
        if (c) {
            *apDest++ = c;
            --iprocessed;
        }
        else {
            uint16_t v5[3];
            v5[0] = 0;
            apFile->NiBinaryStream::Read(v5, 2u);
            ourMemset(apDest, 0, v5[0]);
            apDest += v5[0];
            iprocessed -= 3;
        }
    }
    return bok;
}

bool LipSynchAnim::ChangeFilenameToLipSynch(BSStringT<char>& aFilename) {
    char Dst[268];
    BSstrcpy(Dst, 260, aFilename.pString);
    char* pext = std::strrchr(Dst, 46);
    if (!pext) {
        return 0;
    }
    *pext = 0;
    aFilename.SPrintF("%s.lip", Dst);
    return 1;
}
