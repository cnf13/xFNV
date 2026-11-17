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

PathingNode::PathingNode(const PathingLocation* a1, const NiPoint3* a2)
  : iFlags(0), Location(*a1), Tangent(*a2), pDoorRef(nullptr) {}

PathingNode::~PathingNode() {}

bool PathingNode::ActivateNode(Actor* apActor) {
    return (bool)((uint8_t)(IsTeleportDoorNode() && pDoorRef->Activate(apActor, false, nullptr, 1)));
}

PathingNode* PathingNode::Copy(PathingNode* apSource) {
    iFlags = (uint32_t)((int32_t)apSource->iFlags);
    &(&Location)->operator=(apSource->Location);
    Tangent = apSource->Tangent;
    pDoorRef = apSource->pDoorRef;
    return this;
}

bool PathingNode::IsTransitionNode() {
    return (bool)((uint8_t)((((int32_t)iFlags) & 4) == 4));
}

void PathingNode::SetTransitionFlag(bool abFlag) {
    int32_t v2;
    if (abFlag) {
        v2 = ((int32_t)iFlags) | 4;
    }
    else {
        v2 = (int32_t)(((int32_t)iFlags) & (~4u));
    }
    iFlags = (uint32_t)v2;
}

bool PathingNode::IsTeleportDoorNode() const {
    return (bool)((uint8_t)((((int32_t)iFlags) & 3) == 3));
}

void PathingNode::SetTeleportDoor(TESObjectREFR* a1) {
    pDoorRef = a1;
    const NiPoint3* LinkedDoorTeleportPosition = &pDoorRef->GetLinkedDoorTeleportPosition();
    Location.SetLocation(*LinkedDoorTeleportPosition);
    iFlags |= 3u;
}

void PathingNode::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(this, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&Tangent, 12u, 0);
    apSaveGameBuffer->SaveFormID(pDoorRef, 0);
    Location.SaveGame(apSaveGameBuffer);
}

void PathingNode::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(this, 4u);
    apLoadGameBuffer->LoadData(&Tangent, 12u);
    uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    pDoorRef = dynamic_cast<TESObjectREFR*>(FormByNumericID);
    Location.LoadGame(apLoadGameBuffer);
}

void PathingNode::operator delete(void* apMemory) {
    ourFree(apMemory);
}
