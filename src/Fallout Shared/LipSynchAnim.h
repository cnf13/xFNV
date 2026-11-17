#pragma once

#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"

class BSFaceGenAnimationData;
class BSFile;

// The lip-sync animation of a voice file: for every frame (1/30 s) the 16 phoneme and 17 modifier values. Not
// polymorphic.
class LipSynchAnim {
public:
    // The weight of each of the 16 phonemes when the strongest is picked.
    using PhonemeCoefficientArray = float[16];

    LipSynchAnim(uint32_t aiNumFrames, int32_t aiStartingFrame);
    ~LipSynchAnim();

    // The seconds before the first frame: zero unless the starting frame is negative; the double product is rounded to
    // a float before it is returned.
    float GetStartingDelay() const;
    float GetPuppetValue(int32_t aiFrame);
    bool ApplyToNode(BSFaceGenAnimationData* apData, float afEmptyAtEnd);

    static void operator delete(void* apMemory);
    // Finds and opens the voice file of that name and loads it from there; null when there is
    // none.
    static LipSynchAnim* LoadFromFile(const char* apName);
    // The same through the name string's pString.
    static LipSynchAnim* LoadFromFile(const BSStringT<char>& aFilename);
    // Reads the animation from the opened file: a 12-byte header (version, data size, compressed)
    // when the version is 1; otherwise a version byte and run-length data whose unpacked size is counted first. The
    // file is deleted before returning.
    static LipSynchAnim* LoadFromFile(BSFile* apFile);
    // Builds the animation from the unpacked data.
    static LipSynchAnim* Scatter(const char* pBuffer, uint32_t nBufferSize);
    // Unpacks aiBufferSize bytes of the run-length data read from apFile into apDest,
    // aiDecompSize bytes.
    static bool Decompress(BSFile* apFile, char* apDest, uint32_t aiBufferSize, uint32_t aiDecompSize);
    static bool ChangeFilenameToLipSynch(BSStringT<char>& aFilename);

    uint32_t iNumFrames;
    int32_t iStartingFrame;
    float** ppPhonemes;
    float** ppModifiers;

    static inline PhonemeCoefficientArray fPhonemeCoefficients{0.7f, 0.95f, 0.0f, 0.0f, 0.0f,  0.4f, 0.4f, 0.0f,
                                                               0.6f, 0.0f,  0.0f, 0.6f, 0.95f, 0.2f, 0.2f, 0.4f};
};

extern SettingT<INISettingCollection> fDialogueHeadYawExaggeration;
extern SettingT<INISettingCollection> fDialogueHeadPitchExaggeration;
extern SettingT<INISettingCollection> fDialogueHeadRollExaggeration;
