#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"

class TESObjectREFR;

// A controller on a door's scene graph that keeps the door's physics off while it moves, and the list of doors to
// switch back on. Only the statics and the two static functions below are declared; nothing virtual is declared here.
class BSDoorHavokController : public NiTimeController {
public:
    // Calls a virtual function on each listed door and empties the list.
    static void ReenableDoors();
    // Takes apDoor off the list of doors to switch back on, when it is not null.
    static void ReleaseDoor(TESObjectREFR* apDoor);

    bool bFirst;

    static NiRTTI ms_RTTI;
    // The doors waiting to be switched back on.
    static BSSimpleList<TESObjectREFR*> DoorsToReenable;
    // The lock around DoorsToReenable.
    static BSCriticalSection ListCritSection;
};
