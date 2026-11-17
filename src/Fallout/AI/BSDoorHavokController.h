#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Reconstruction.hpp"

class TESObjectREFR;

// A controller on a door's scene graph that keeps the door's physics off while it moves, and the list of doors to
// switch back on. The PC exe keeps only the statics and the two static functions below: no vtable, constructor or
// virtual of the class survives in it (the PDB's GetRTTI, CreateClone, Update, TargetIsRequiredType, the
// destructor and CopyMembers are not in the exe), so nothing virtual is declared here.
class BSDoorHavokController : public NiTimeController {
public:
	// Calls the virtual at slot 113 (+0x1C4) of each listed door and empties the list.
	static void ReenableDoors();
	// Takes apDoor off the list of doors to switch back on, when it is not null.
	static void ReleaseDoor(TESObjectREFR* apDoor);

	// The PDB's offset (the Xbox build); the PC exe has no instance of the class to check it against.
	bool bFirst;

	static NiRTTI ms_RTTI;
	// The doors waiting to be switched back on (the PDB's DoorsToReenable; IDA: kDoorsToReenable).
	static BSSimpleList<TESObjectREFR*> DoorsToReenable;
	// The lock around DoorsToReenable (the PDB's ListCritSection).
	static BSCriticalSection ListCritSection;
};
