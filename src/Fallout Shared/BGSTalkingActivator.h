#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "Fallout Shared/TESObjectACTI.h"

class Actor;
class BGSVoiceType;
class MobileObject;

// How far a radio station is heard; the values are the cases of GetRadioStationsInRangeOfRef's switch.
enum RADIO_RANGE_TYPE : uint32_t {
    RADIO_RANGE_RADIUS = 0,
    RADIO_RANGE_EVERYWHERE = 1,
    RADIO_RANGE_WORLDSPACE = 2,
    RADIO_RANGE_LINKEDINT = 3,
    RADIO_RANGE_CURRENT_CELL = 4,
    RADIO_RANGE_COUNT = 5,
};

// A talking activator base form (TACT): an activator that speaks, such as a radio station.
class BGSTalkingActivator : public TESObjectACTI {
public:
    BGSTalkingActivator();
    ~BGSTalkingActivator() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    bool Activate(
        TESObjectREFR* apItemActivated,
        TESObjectREFR* apActionRef,
        bool abIDFlag,
        TESBoundObject* apObjectToGet,
        int32_t aiCount
    ) override;

    // A new virtual function, const because TESForm::GetDangerous is non-const; with the using-declaration below, a
    // non-const receiver picks TESForm's GetDangerous, so call this one through a const BGSTalkingActivator.
    virtual bool GetDangerous() const {
        return false;
    }
    virtual bool GetReflectedByAutoWater() {
        return false;
    }
    using TESForm::GetDangerous;
    using TESForm::InitLoadGame;
    using TESForm::Revert;
    using TESObjectACTI::LoadGame;
    using TESObjectACTI::Save;
    using TESObjectACTI::SaveGame;

    // Whether this talking activator is a radio station.
    bool IsRadioStation();
    // Whether this radio station is not one of the Pip-Boy's.
    bool IsNonPipboyRadioStation();
    void SetVoiceType(BGSVoiceType* apVoiceType);
    // Returns pVoiceType.
    BGSVoiceType* GetVoiceType();
    // Adds the talking activator's extra data to apRef.
    MobileObject* AddMobileObjectExtra(TESObjectREFR* apRef);
    // Sets pTempRef, the speaker, when apRef is null or an actor.
    void SetTempRef(TESObjectREFR* pref);
    // Returns pTempRef.
    Actor* GetTempRef();

    // Whether apForm is a talking activator radio station that the Pip-Boy does not list; aiData is unused.
    static bool IsNonPipboyRadioStation(TESForm* apForm, int32_t aiData);
    // Empties RadioStationList.
    static void ClearRadioStationList();
    // Adds to apOutList the radio stations apListenRef hears and, when apDistancesList is given, their distances to
    // it.
    static void GetRadioStationsInRangeOfRef(
        TESObjectREFR* apListenRef,
        BSSimpleList<TESObjectREFR*>* apOutList,
        BSSimpleList<float>* apDistancesList
    );
    // Whether apForm is a talking activator that is a radio station; aiData is unused.
    static bool IsRadioStation(TESForm* apForm, int32_t aiData);
    // Adds apRef to RadioStationList when its base object is a radio station.
    static void AddRadioStation(TESObjectREFR* apRef);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    Actor* pTempRef;
    BGSVoiceType* pVoiceType;

    // The radio stations placed in the loaded world.
    static BSSimpleList<TESObjectREFR*> RadioStationList;
};
