#include "Fallout Shared/Pathfinding/TeleportPath.h"

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/TESObjectREFR.h"

TeleportLink::TeleportLink() : DoorTeleportLocation() {}

TeleportPath::TeleportPath() : Spaces(), Doors() {}

TeleportPath::~TeleportPath() {}

void TeleportPath::Clear() {
    Spaces.Clear(true);
    Doors.Clear(true);
}

// The running length is a float local that is rounded to float after every addition.
float TeleportPath::ComputeLength() {
    float fLength = 0.0f;
    if (Spaces.IsEmpty()) {
        return -1.0f;
    }
    if (Doors.IsEmpty()) {
        return (End - Start).Length();
    }
    TESObjectREFR* pFirstDoor = Doors.GetFirst().pDoor;
    fLength = (pFirstDoor->GetLocationOnReference() - Start).Length() + fLength;
    for (uint32_t i = 1; i < Doors.GetSize(); ++i) {
        TESObjectREFR* pDoor = Doors.GetAt(i).pDoor;
        const NiPoint3& rPrevious = Doors.GetAt(i - 1).DoorTeleportLocation;
        fLength = (pDoor->GetLocationOnReference() - rPrevious).Length() + fLength;
    }
    fLength = (End - Doors.GetLast().DoorTeleportLocation).Length() + fLength;
    return fLength;
}
