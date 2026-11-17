#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESSoundFile.h"

// A sound form (SOUN).
class TESSound : public TESBoundAnimObject, public TESSoundFile {
public:
    // The sound's playback data (DATA).
    struct SOUND_DATA {
        // Swaps the multi-byte fields read from a big-endian file.
        void Endian();

        uint8_t iMin;
        uint8_t iMax;
        char iPitch;
        uint8_t iPadding1;
        uint32_t iFlags;
        uint16_t iAttenuation;
        uint16_t iTimeConstraints;
        uint16_t pAttenuationCurve[5];
        uint16_t iReverbAttenuation;
        uint32_t iPriority;
        uint32_t iLoopBegin;
        uint32_t iLoopEnd;
    };

    // The playback data as older files store it, without the attenuation curve, priority and loop points.
    struct SOUND_DATA2 {
        // Swaps the flags, attenuation and time constraints read from a big-endian file.
        void Endian();

        uint8_t iMin;
        uint8_t iMax;
        char iPitch;
        uint8_t iPadding1;
        uint32_t iFlags;
        uint16_t iAttenuation;
        uint16_t iTimeConstraints;
    };

    using randomNameHashesLastPick_t = char[10];
    // Guessed bound: the database gives none; the next data item starts 0x28 bytes on.
    using randomNameHashes_t = int32_t[10];

    TESSound();
    ~TESSound() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    const char* GetFormEditorID() override;
    bool SetFormEditorID(const char* apEditorID) override;
    void UnClone3D(TESObjectREFR*) override {}
    NiAVObject* Clone3D(TESObjectREFR*) override {
        return nullptr;
    }

    using TESBoundObject::Clone3D;
    using TESForm::Save;

    // A copy of the playback data.
    SOUND_DATA GetData();
    // The sound file's path.
    const char* GetFilename();
    char GetRandomPercentChance();
    uint8_t GetMaxDistance();
    uint8_t GetMinDistance();
    uint16_t GetAttenuationCurveValue(int32_t aiIndex);
    uint16_t GetReverbAttenuation();
    uint8_t GetStartTime();
    uint8_t GetEndTime();
    uint8_t GetPriority();

    // The audio flags for the form's flags aiTESSoundFlags: loop, else fast envelope, else slow envelope.
    static uint32_t ConvertLoopFlag(uint32_t aiTESSoundFlags);

    // Creates the built-in sound forms under the default-objects heap context.
    static void GenerateDefaultObjects();
    // The sound form whose file is apFilename, or null.
    static TESSound* FindSoundByFilename(const char* apFilename);
    static bool PickRandomFilename(char* pPath);

    BSStringT<char> strEditorID;
    SOUND_DATA data;
    char cRandomPercentChance;
    uint8_t padding69[3];

    static inline randomNameHashesLastPick_t randomNameHashesLastPick{};
    static inline char lastSoundHash;
    static inline randomNameHashes_t randomNameHashes{};
};

// The bush sound name for aiIndex (0..9), else null.
const char* GetBushSoundName(int32_t aiIndex);
// The BRUSH<bush><Sneak|Run|Walk> sound for the player's movement (Walk without a player).
TESSound* GetNthBrushPassthroughSound(int32_t aiIndex);

// Ten names: GetBushSoundName indexes it with 0..9.
using BushSoundNameArray = char* [10];
inline BushSoundNameArray g_bushSoundNameArray{const_cast<char*>("BushA"), const_cast<char*>("BushB"),
                                               const_cast<char*>("BushC"), const_cast<char*>("BushD"),
                                               const_cast<char*>("BushE"), const_cast<char*>("BushF"),
                                               const_cast<char*>("BushG"), const_cast<char*>("BushH"),
                                               const_cast<char*>("BushI"), const_cast<char*>("BushJ")};
