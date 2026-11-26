#include "Fallout Shared/Pathfinding/VirtualPathingNode.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/rtti.h"

VirtualPathingNode::VirtualPathingNode() : iFlags(0), Location(), pDoorRef(nullptr) {}

VirtualPathingNode::VirtualPathingNode(const PathingLocation& aLocation)
  : iFlags(kFlagHasActualLocation), Location(), pDoorRef(nullptr) {
    Location = aLocation;
    if (Location.ResolveNavMeshInfo(nullptr)) {
        iFlags |= kFlagHasNavMesh;
    }
}

VirtualPathingNode::VirtualPathingNode(const NavMeshInfo* apInfo)
  : iFlags(kFlagHasNavMesh), Location(), pDoorRef(nullptr) {
    Location.SetNavMeshInfo(apInfo);
    Location.SetLocationSimple(apInfo->ApproxLocation);
}

VirtualPathingNode::VirtualPathingNode(const NiPoint3* apPoint, const NavMeshInfo* apNavmeshInfo)
  : iFlags(kFlagHasNavMesh | kFlagHasActualLocation), Location(), pDoorRef(nullptr) {
    Location.SetNavMeshInfo(apNavmeshInfo);
    Location.SetLocationSimple(*apPoint);
}

VirtualPathingNode::VirtualPathingNode(
    const NavMeshInfo* apNavMeshInfo,
    const NiPoint3* apLocation,
    TESObjectREFR* apDoor
)
  : iFlags(kFlagHasNavMesh | kFlagHasActualLocation | kFlagTransition), Location(), pDoorRef(apDoor) {
    Location.SetNavMeshInfo(apNavMeshInfo);
    Location.SetLocationSimple(*apLocation);
}

VirtualPathingNode::VirtualPathingNode(const PathingLocation* apLocation, TESObjectREFR* apDoor)
  : iFlags(kFlagHasActualLocation | kFlagTransition), Location(), pDoorRef(apDoor) {
    Location = *apLocation;
    if (Location.ResolveNavMeshInfo(nullptr)) {
        iFlags |= kFlagHasNavMesh;
    }
}

VirtualPathingNode::~VirtualPathingNode() {}

VirtualPathingNode* VirtualPathingNode::Copy(VirtualPathingNode* apSource) {
    iFlags = apSource->iFlags;
    Location = apSource->Location;
    pDoorRef = apSource->pDoorRef;
    return this;
}

void VirtualPathingNode::UpdateNode() {
    if (!(iFlags & kFlagHasNavMesh) && Location.ResolveNavMeshInfo(nullptr)) {
        iFlags |= kFlagHasNavMesh;
    }
}

void VirtualPathingNode::SetLocation(PathingLocation& arLocation) {
    Location = arLocation;
    iFlags = kFlagHasActualLocation;
    if (Location.ResolveNavMeshInfo(nullptr)) {
        iFlags |= kFlagHasNavMesh;
    }
}

bool VirtualPathingNode::HasNavMesh() const {
    return (iFlags & kFlagHasNavMesh) != 0;
}

bool VirtualPathingNode::HasActualLocation() const {
    return (iFlags & kFlagHasActualLocation) != 0;
}

bool VirtualPathingNode::IsTransitionNode() {
    return (iFlags & kFlagTransition) != 0;
}

void VirtualPathingNode::Save(BGSSaveGameBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&iFlags, sizeof(iFlags), 0);
    apSaveGameBuffer->SaveFormID(pDoorRef, 0);
    Location.SaveGame(apSaveGameBuffer);
}

void VirtualPathingNode::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(&iFlags, sizeof(iFlags));
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
    pDoorRef = dynamic_cast<TESObjectREFR*>(pForm);

    Location.LoadGame(apLoadGameBuffer);
}

void VirtualPathingNode::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// Copies the point component by component, so a source overlapping Location reads already-copied components.
void PathingLocation::SetLocationSimple(const NiPoint3& aLocation) {
    Location.x = aLocation.x;
    Location.y = aLocation.y;
    Location.z = aLocation.z;
}
