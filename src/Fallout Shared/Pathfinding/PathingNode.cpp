#include "Fallout Shared/Pathfinding/PathingNode.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/rtti.h"

PathingNode::PathingNode() : iFlags(0), Location(), pDoorRef(nullptr) {}

PathingNode::PathingNode(const PathingLocation* apLocation, const NiPoint3* apTangent)
  : iFlags(0), Location(*apLocation), Tangent(*apTangent), pDoorRef(nullptr) {}

PathingNode::~PathingNode() {}

bool PathingNode::ActivateNode(Actor* apActor) {
    return IsTeleportDoorNode() && pDoorRef->Activate(apActor, false, nullptr, 1);
}

PathingNode* PathingNode::Copy(PathingNode* apSource) {
    iFlags = apSource->iFlags;
    Location = apSource->Location;
    Tangent = apSource->Tangent;
    pDoorRef = apSource->pDoorRef;
    return this;
}

bool PathingNode::IsTransitionNode() {
    return (iFlags & kFlagTransition) != 0;
}

void PathingNode::SetTransitionFlag(bool abFlag) {
    if (abFlag) {
        iFlags |= kFlagTransition;
    }
    else {
        iFlags &= ~kFlagTransition;
    }
}

bool PathingNode::IsTeleportDoorNode() const {
    return (iFlags & kFlagTeleportDoor) == kFlagTeleportDoor;
}

void PathingNode::SetTeleportDoor(TESObjectREFR* apDoorRef) {
    pDoorRef = apDoorRef;
    Location.SetLocation(pDoorRef->GetLinkedDoorTeleportPosition());
    iFlags |= kFlagTeleportDoor;
}

void PathingNode::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&iFlags, sizeof(iFlags), 0);
    apSaveGameBuffer->SaveDataEndian(&Tangent, sizeof(Tangent), 0);
    apSaveGameBuffer->SaveFormID(pDoorRef, 0);
    Location.SaveGame(apSaveGameBuffer);
}

void PathingNode::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&iFlags, sizeof(iFlags));
    apLoadGameBuffer->LoadData(&Tangent, sizeof(Tangent));
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
    pDoorRef = dynamic_cast<TESObjectREFR*>(pForm);
    Location.LoadGame(apLoadGameBuffer);
}

void PathingNode::operator delete(void* apMemory) {
    ourFree(apMemory);
}
