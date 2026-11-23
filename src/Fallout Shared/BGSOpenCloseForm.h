#pragma once

#include "BSMain/Setting.h"

class NiControllerSequence;
class TESForm;
class TESObjectREFR;

// The open/close animation hooks of activators, containers and doors (a base with its own vtable, no fields).
class BGSOpenCloseForm {
public:
    enum OPEN_STATE {
        OBJECT_NONE = 0,
        OBJECT_OPEN = 1,
        OBJECT_OPENING = 2,
        OBJECT_CLOSED = 3,
        OBJECT_CLOSING = 4,
    };

    BGSOpenCloseForm();

    virtual void HandleOpenFinish(TESObjectREFR*, TESObjectREFR*) {}
    virtual void HandleCloseFinish(TESObjectREFR*, TESObjectREFR*) {}
    virtual bool GetBlockActivatesWhileOpening() {
        return false;
    }

    // Whether apForm is an open/close form (a door, a container or an activator with open and
    // close animations).
    static bool IsOpenCloseForm(TESForm* apForm);
    // Opens or closes apRef, snapping to the end state with abSnap.
    static bool SetOpenState(TESObjectREFR* apRef, bool abOpen, bool abSnap);
    // Animation end callback of an open or close sequence.
    static void HandleAnimEnd(TESObjectREFR* apAnimRef, NiControllerSequence* apEndSeq);
    // Handles the end of the open or close sequence apEndSeq of apAnimRef: hands the reference's Havok bodies back
    // to its cell's world when door physics is off. Returns whether the sequence was "Open" or "Close".
    static bool HandleAnimEndHavok(TESObjectREFR* apAnimRef, NiControllerSequence* apEndSeq);
    // Snaps apRef's Havok bodies to its 3D.
    static void SnapHavokTo3D(TESObjectREFR* apRef);
    // The open state of apRef's open/close animation; OBJECT_NONE when apRef is null or
    // its base form has no open/close animations.
    static OPEN_STATE GetOpenState(TESObjectREFR* apRef);
    // Plays apItemActivated's open or close animation for apActionRef.
    static void
    HandleActivate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, BGSOpenCloseForm* apOpenCloseForm);
    static BGSOpenCloseForm* GetAsOpenCloseForm(TESForm* apForm);
};

// The game settings BGSOpenCloseForm.cpp defines.
extern SettingT<INISettingCollection> bAnimateDoorPhysics;
