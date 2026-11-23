#include "Fallout/AI/BSDoorHavokController.h"

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"

BSSimpleList<TESObjectREFR*> BSDoorHavokController::DoorsToReenable;
BSCriticalSection BSDoorHavokController::ListCritSection;
NiRTTI BSDoorHavokController::ms_RTTI("BSDoorHavokController", &NiTimeController::ms_RTTI);

void BSDoorHavokController::ReenableDoors() {
    while (!BSDoorHavokController::DoorsToReenable.IsEmpty()) {
        TESObjectREFR* m_item = BSDoorHavokController::DoorsToReenable.m_item;
        m_item->InitHavok();
        BSDoorHavokController::DoorsToReenable.RemoveHead();
    }
}

void BSDoorHavokController::ReleaseDoor(TESObjectREFR* apDoor) {
    if (apDoor) {
        BSDoorHavokController::DoorsToReenable.Remove(apDoor);
    }
}
