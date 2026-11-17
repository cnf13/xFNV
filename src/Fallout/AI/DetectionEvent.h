#pragma once

#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class BGSLoadFormBuffer;
class BGSLoadGameBuffer;
class BGSSaveFormBuffer;
class TESObjectREFR;

// What made a detection event.
enum DETECTION_EVENT_TYPE : int32_t {
    DETECTION_EVENT_NONE = -1,
    DETECTION_EVENT_BULLET = 0,
    DETECTION_EVENT_BEAM = 1,
    DETECTION_EVENT_EXPLOSION = 2,
    DETECTION_EVENT_GENERIC = 3,
    DETECTION_EVENT_BODY = 4,
    DETECTION_EVENT_COUNT = 5,
};

// A noise or sight an actor registered: how loud it was, where it came from, when, what caused it and which reference.
class DetectionEvent {
public:
    DetectionEvent();

    // Saves the event; the reference as its form ID.
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    void LoadGame(BGSLoadGameBuffer* arg0);
    // Resolves pRef's saved form ID to the reference after a load; the buffer is unread.
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

    int32_t iActionValue;
    NiPoint3 Location;
    CombatTimeStamp fTimeStamp;
    DETECTION_EVENT_TYPE eEventType;
    TESObjectREFR* pRef;
};
