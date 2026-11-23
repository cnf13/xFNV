#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/Region System/TESRegionData.h"
#include "SDK/MSVC/ehvec.h"

class BGSMusicType;
class MediaSet;
class TESFile;
struct TESRegionSound;
class TESSound;

// One sound of a region: the sound form, its flags and how likely it plays (also the RDSD chunk's 12-byte record).
struct TESRegionSound {
    // The weather classifications the sound plays in.
    enum REGION_SOUND_FLAGS : uint32_t {
        FLAG_PLEASANT = 0x1,
        FLAG_CLOUDY = 0x2,
        FLAG_RAINY = 0x4,
        FLAG_SNOWY = 0x8,
        // The bits compared with the current weather's classification flags.
        WEATHER_MASK = 0xFFFF,
    };

    TESRegionSound();

    // Swaps the three dwords.
    void Endian();
    // The sound: the form of the ID uiSound holds.
    TESSound* GetSound() const;

    // The file record's form ID, kept after the load since a TESSound* does not fit (the exe stores the pointer here);
    // GetSound looks it up.
    uint32_t uiSound;
    uint32_t uiFlags;
    uint32_t uiChance;
};

// A region's sound data: its music type, its sounds and the incidental and battle media sets (form IDs).
struct TESRegionSounds {
    TESRegionSounds();
    ~TESRegionSounds();

    // Swaps the byte order of pType. LoadRegionDataSound calls it on its sounds after reading an RDMO form ID.
    void Endian();

    BGSMusicType* pType;
    BSSimpleList<TESRegionSound*> soundList;
    // A form ID despite the name: GetIncidentalMediaSet looks it up with GetFormByNumericID, SetIncidentalMediaSet
    // stores the form's ID.
    uint32_t pMediaSetIncidentals;
    BSSimpleList<uint32_t> MediaSetBattleList;
};

// A region's sound data.
class TESRegionDataSound : public TESRegionData {
public:
    TESRegionDataSound();
    // The copy constructor.
    TESRegionDataSound(TESRegionDataSound* arData);
    ~TESRegionDataSound() override;

    void Save() override;
    REGION_DATA_ID GetID() const override;
    TESRegionData* Copy() override;
    TESRegionData* Blend(const TESRegionData*) override;
    void BlendInto(const TESRegionData*, uint32_t) override;
    bool Validate() override;
    virtual bool LoadRegionDataSound(TESFile*);
    virtual BGSMusicType* GetMusicType() const {
        return sounds.pType;
    }
    virtual bool SetMusicType(BGSMusicType*);
    virtual MediaSet* GetMediaSetIncidentals();
    virtual MediaSet* GetMediaSetBattle();
    virtual void SetMediaSetIncidentals(MediaSet*);

    bool ClearSoundList();

    TESRegionSounds sounds;
};
