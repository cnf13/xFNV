#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout/Audio/FalloutAudioMedia.h"

class MediaLocationController;
class TESSound;

// A media set form (MSET).
class MediaSet : public TESForm, public TESFullName {
public:
    // One music layer of a media set: the sound's name, how much it is attenuated (decibels) and the percent of the
    // time it plays.
    struct MediaLayer {
        MediaLayer();
        // Copies the name, then the attenuation and percent.
        MediaLayer(const MediaLayer& arSource);
        ~MediaLayer();

        // Empties the name and resets the attenuation to -6 and the percent to 100.
        void Clear();
        // Copies aLayer into this layer; aLayer is passed by value and destroyed when the call returns.
        void Copy(MediaLayer l);
        // Whether aLayer differs from this layer; aLayer is passed by value.
        bool Compare(MediaLayer l);

        BSStringT<char> Name;
        float Attenuation;
        float Percent;
    };

    // The kinds of set a media location controller picks from.
    enum MEDIASETTYPE : int32_t {
        // No type yet: the constructor stores -1 in Type.
        SETTYPE_NONE = -1,
        BATTLE = 0,
        LAYER_LOCATION = 1,
        LAYER_DUNGEON = 2,
        INCIDENTAL = 3,
        SETTYPE_COUNT = 4,
    };

    MediaSet();
    ~MediaSet() override;

    void InitializeData() override;
    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    using TESForm::Save;

    void Reset();
    bool UpdateLocation(uint32_t ticks, bool abInCombat, MediaLocationController* pController);
    bool Update(uint32_t ticks, bool abInCombat, MediaLocationController* pController);
    bool UpdateDungeon(uint32_t ticks, bool abInCombat, MediaLocationController* pController);
    bool UpdateBattle(bool bInCombat, uint32_t ticks, MEDIATYPE mType);
    bool UpdateIncidentals(uint32_t ticks);

    uint32_t uiMinLayerUpdate;
    uint32_t uiSeekTime;
    uint32_t uiLoopUpdate;
    bool bStarted;
    bool bInCombat;
    bool bInTension;
    bool bIsDay;
    int8_t cCurrentLayer;
    TESFullName MediaSetName;
    MEDIASETTYPE Type;
    MediaLayer pMLOne;
    MediaLayer pMLTwo;
    MediaLayer pMLThree;
    MediaLayer pMLFour;
    MediaLayer pMLFive;
    MediaLayer pMLSix;
    uint8_t cEnableFlags;
    float fOne;
    float fTwo;
    float fThree;
    float fFour;
    TESSound* pSoundOne;
    TESSound* pSoundTwo;

    static inline uint32_t uiLastUpdate;
};
