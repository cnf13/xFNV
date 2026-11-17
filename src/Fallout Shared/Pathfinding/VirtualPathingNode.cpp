#include "Fallout Shared/Pathfinding/VirtualPathingNode.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
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

VirtualPathingNode::VirtualPathingNode(const PathingLocation& aLocation) : iFlags(2), Location(), pDoorRef(nullptr) {
    Location = aLocation;
    if (Location.ResolveNavMeshInfo(nullptr)) {
        iFlags |= 1u;
    }
}

VirtualPathingNode::VirtualPathingNode(const NavMeshInfo* apInfo) : iFlags(1), Location(), pDoorRef(nullptr) {
    Location.SetNavMeshInfo(apInfo);
    Location.SetLocationSimple(apInfo->ApproxLocation);
}

VirtualPathingNode::VirtualPathingNode(const NiPoint3* apPoint, const NavMeshInfo* apNavmeshInfo)
  : iFlags(3), Location(), pDoorRef(nullptr) {
    Location.SetNavMeshInfo(apNavmeshInfo);
    Location.SetLocationSimple(*apPoint);
}

VirtualPathingNode::VirtualPathingNode(
    const NavMeshInfo* apNavMeshInfo,
    const NiPoint3* apLocation,
    TESObjectREFR* apDoor
)
  : iFlags(7), Location(), pDoorRef(apDoor) {
    Location.SetNavMeshInfo(apNavMeshInfo);
    Location.SetLocationSimple(*apLocation);
}

VirtualPathingNode::VirtualPathingNode(const PathingLocation* a1, TESObjectREFR* a2)
  : iFlags(6), Location(), pDoorRef(a2) {
    Location = *a1;
    if (Location.ResolveNavMeshInfo(nullptr)) {
        iFlags |= 1u;
    }
}

VirtualPathingNode::~VirtualPathingNode() {}

VirtualPathingNode* VirtualPathingNode::Copy(VirtualPathingNode* apSource) {
    iFlags = apSource->iFlags;
    &(&Location)->operator=(apSource->Location);
    pDoorRef = apSource->pDoorRef;
    return this;
}

void VirtualPathingNode::UpdateNode() {
    if (!(iFlags & 1) && Location.ResolveNavMeshInfo(nullptr)) {
        uint32_t& ruFlags = iFlags;
        ruFlags = ruFlags | 1;
    }
}

void VirtualPathingNode::SetLocation(PathingLocation& arLocation) {
    Location = arLocation;
    iFlags = 2;
    if (Location.ResolveNavMeshInfo(nullptr)) {
        uint32_t& ruFlags = iFlags;
        ruFlags = ruFlags | 1;
    }
}

bool VirtualPathingNode::HasNavMesh() const {
    return (bool)((uint8_t)(((int32_t)(iFlags & 1)) == 1));
}

bool VirtualPathingNode::HasActualLocation() const {
    return (bool)((uint8_t)(((int32_t)(iFlags & 2)) == 2));
}

bool VirtualPathingNode::IsTransitionNode() {
    return (iFlags & 4) == 4;
}

void VirtualPathingNode::Save(BGSSaveGameBuffer* a2) {
    a2->SaveDataEndian(this, 4u, 0);
    a2->SaveFormID(pDoorRef, 0);
    Location.SaveGame(a2);
}

void VirtualPathingNode::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(this, 4u);
    uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    pDoorRef = dynamic_cast<TESObjectREFR*>(FormByNumericID);
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
