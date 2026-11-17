#include "Fallout Shared/Pathfinding/PathingLocation.h"

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/Havok/CameraCaster.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshBounds.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoMap.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

PathingLocation::PathingLocation()
  : Location(3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f), pNavMesh(nullptr), pNavMeshes(nullptr), pCell(nullptr),
    pWorldSpace(nullptr), iCellKey(0xDEADDEAD), usTriangle(0xFFFF), uiFlags(0) {}

PathingLocation::PathingLocation(const PathingLocation& aLoc)
  : Location(aLoc.Location.x, aLoc.Location.y, aLoc.Location.z), pNavMesh(aLoc.pNavMesh), pNavMeshes(aLoc.pNavMeshes),
    pCell(aLoc.pCell), pWorldSpace(aLoc.pWorldSpace), iCellKey(aLoc.iCellKey), usTriangle(aLoc.usTriangle),
    uiFlags(aLoc.uiFlags) {}

PathingLocation::PathingLocation(const NiPoint3& akLocation, TESObjectREFR* apRef) {
    TESWorldSpace* pWorld = apRef->GetWorldSpace();
    TESObjectCELL* pParentCell = apRef->GetParentCell();
    SetupData(akLocation, pParentCell, pWorld);
}

PathingLocation::PathingLocation(const NiPoint3& aLocation, const PathingLocation& aHintLoc)
  : Location(aLocation.x, aLocation.y, aLocation.z), usTriangle(0xFFFF), uiFlags(0) {
    NavMeshPtr spMesh;
    if (!aHintLoc.GetNavMesh(spMesh) ||
        (usTriangle = spMesh.m_pObject->FindTriangleForLocation(aLocation, 0), usTriangle == 0xFFFF)) {
        TESWorldSpace* pWorld = aHintLoc.pWorldSpace;
        SetupData(aLocation, aHintLoc.GetCell(), pWorld);
    }
    else {
        pNavMesh = aHintLoc.pNavMesh;
        pNavMeshes = nullptr;
        pCell = aHintLoc.pCell;
        pWorldSpace = aHintLoc.pWorldSpace;
        iCellKey = aHintLoc.iCellKey;
    }
}

PathingLocation::PathingLocation(const NiPoint3* arLocation, TESObjectCELL* apCell, TESWorldSpace* apWorld) {
    SetupData(*arLocation, apCell, apWorld);
}

PathingLocation::PathingLocation(const NiPoint3* arLocation, const NavMeshInfo* apNavMeshInfo)
  : Location(arLocation->x, arLocation->y, arLocation->z), pNavMesh(apNavMeshInfo), pNavMeshes(nullptr),
    pWorldSpace(apNavMeshInfo->GetWorldSpace()), usTriangle(0xFFFF), uiFlags(0) {
    if (pWorldSpace) {
        pCell = nullptr;
        iCellKey = apNavMeshInfo->iCellKey;
    }
    else {
        pCell = apNavMeshInfo->GetInteriorCell();
        iCellKey = 0xDEADDEAD;
    }
    SetAllMeshesReachable(1);
}

PathingLocation::PathingLocation(const NiPoint3& arLocation, const NavMeshInfo* apNavMeshInfo, uint16_t ausTriangle)
  : Location(arLocation.x, arLocation.y, arLocation.z), pNavMesh(apNavMeshInfo), pNavMeshes(nullptr),
    pWorldSpace(apNavMeshInfo->GetWorldSpace()), usTriangle(ausTriangle), uiFlags(0) {
    if (pWorldSpace) {
        pCell = nullptr;
        iCellKey = apNavMeshInfo->iCellKey;
    }
    else {
        pCell = apNavMeshInfo->GetInteriorCell();
        iCellKey = 0xDEADDEAD;
    }
    SetAllMeshesReachable(1);
}

PathingLocation::PathingLocation(TESObjectREFR* apTargetRef) {
    TESObjectCELL* pTargetCell = apTargetRef->GetParentCell();
    TESWorldSpace* pWorld = apTargetRef->GetWorldSpace();
    if (!pTargetCell && !pWorld && apTargetRef == pPlayer) {
        pTargetCell = TES::pInstance->GetInterior();
        if (!pTargetCell) {
            pWorld = TES::pInstance->GetWorldSpace();
        }
    }
    SetupData(apTargetRef->GetLocationOnReference(), pTargetCell, pWorld);
}

PathingLocation::PathingLocation(const NiPoint3* a1, TESObjectCELL* a2) {
    SetupData(*a1, a2, nullptr);
}

PathingLocation::PathingLocation(const NiPoint3* a1, TESWorldSpace* a2) {
    SetupData(*a1, nullptr, a2);
}

PathingLocation::PathingLocation(BGSWorldLocation* ecx0) {
    TESObjectCELL* pCellOut = nullptr;
    TESWorldSpace* pWorldOut = nullptr;
    ecx0->GetCellOrWorld(pCellOut, pWorldOut);
    SetupData(*ecx0->GetPoint(), pCellOut, pWorldOut);
}

void PathingLocation::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    uint32_t aiFormID;
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&Location)), 0xCu, 0);
    if (pNavMesh) {
        aiFormID = pNavMesh->GetNavMeshID();
    }
    else {
        aiFormID = 0;
    }
    apSaveGameBuffer->SaveFormID(aiFormID, (uint32_t)0);
    apSaveGameBuffer->SaveFormID(pCell, (uint32_t)0);
    apSaveGameBuffer->SaveFormID(pWorldSpace, (uint32_t)0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&iCellKey)), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&usTriangle)), 2u, 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&uiFlags)), 1u, 0);
    int8_t apData = (int8_t)(pNavMeshes != 0);
    apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&apData)), 1u, 0);
}

void PathingLocation::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    bool bhadNavMeshesArray;
    apLoadGameBuffer->LoadData(&Location, 12u);
    uint32_t inavmeshID = 0;
    apLoadGameBuffer->LoadFormID(inavmeshID);
    uint32_t v7 = inavmeshID;
    NavMeshInfoMap* NavMeshInfoMap_ = TES::pInstance->GetNavMeshInfoMap();
    pNavMesh = NavMeshInfoMap_->GetNavMeshInfo(v7);
    uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    pCell = dynamic_cast<TESObjectCELL*>(FormByNumericID);
    uint32_t v5 = apLoadGameBuffer->LoadFormID();
    TESForm* v6 = TESForm::GetFormByNumericID(v5);
    pWorldSpace = dynamic_cast<TESWorldSpace*>(v6);
    apLoadGameBuffer->LoadData(&iCellKey, 4u);
    apLoadGameBuffer->LoadData(&usTriangle, 2u);
    usTriangle = -1;
    apLoadGameBuffer->LoadData(&uiFlags, 1u);
    bhadNavMeshesArray = 0;
    apLoadGameBuffer->LoadData(&bhadNavMeshesArray, 1u);
    if (bhadNavMeshesArray) {
        if (pWorldSpace) {
            pNavMeshes = Pathing::GetPotentialNavMeshInfoForLocation(pWorldSpace, iCellKey);
        }
        else {
            if (pCell) {
                pNavMeshes = Pathing::GetPotentialNavMeshInfoForLocation(pCell);
            }
        }
    }
}

PathingLocation::~PathingLocation() {}

bool PathingLocation::IsValidLocation(float afRadius) {
    NiPoint3 v4;
    NiPoint3 aTarget;
    NiPoint3 v6;
    NiPoint3 v7;
    NiPoint3 result;
    ResolveNavMeshInfo(0);
    if (!HasNavMeshData()) {
        return 0;
    }
    if (afRadius <= 0.0) {
        return 1;
    }
    result = GetLocation();
    aTarget.y = result.y;
    aTarget.z = result.z;
    aTarget.x = result.x - afRadius;
    if (!Pathing::CheckLineOfSight(
            *this, aTarget, static_cast<PathingLocation*>(0), static_cast<Pathing::CheckLineOfSightFilter*>(0)
        )) {
        return 0;
    }
    v7.y = result.y;
    v7.z = result.z;
    v7.x = result.x + afRadius;
    if (!Pathing::CheckLineOfSight(
            *this, v7, static_cast<PathingLocation*>(0), static_cast<Pathing::CheckLineOfSightFilter*>(0)
        )) {
        return 0;
    }
    v4.x = result.x;
    v4.z = result.z;
    v4.y = result.y - afRadius;
    if (!Pathing::CheckLineOfSight(
            *this, v4, static_cast<PathingLocation*>(0), static_cast<Pathing::CheckLineOfSightFilter*>(0)
        )) {
        return 0;
    }
    v6.x = result.x;
    v6.z = result.z;
    v6.y = result.y + afRadius;
    return Pathing::CheckLineOfSight(
        *this, v6, static_cast<PathingLocation*>(0), static_cast<Pathing::CheckLineOfSightFilter*>(0)
    );
}

NiPoint3 PathingLocation::GetLocation() const {
    NiPoint3 kResult;
    kResult.x = Location.x;
    kResult.y = Location.y;
    kResult.z = Location.z;
    return kResult;
}

TESObjectCELL* PathingLocation::GetCell() const {
    if (pCell != nullptr) {
        return pCell;
    }

    if (pWorldSpace == nullptr) {
        return nullptr;
    }

    TESObjectCELL* pCellFromKey = pWorldSpace->GetCellFromKey(iCellKey);
    if (pCellFromKey != nullptr) {
        return pCellFromKey;
    }

    return nullptr;
}

void PathingLocation::SetUnused() {
    uiFlags |= kFlagUnused;
}

PathingLocation& PathingLocation::operator=(const PathingLocation& aLoc) {
    Location = aLoc.Location;
    pNavMesh = aLoc.pNavMesh;
    pNavMeshes = aLoc.pNavMeshes;
    pCell = aLoc.pCell;
    pWorldSpace = aLoc.pWorldSpace;
    iCellKey = aLoc.iCellKey;
    usTriangle = aLoc.usTriangle;
    uiFlags = aLoc.uiFlags;
    return *this;
}

bool PathingLocation::QUnused() const {
    return (uiFlags & kFlagUnused) != 0;
}

bool PathingLocation::QAllMeshesReachable() {
    return (uiFlags & kFlagAllMeshesReachable) != 0;
}

void PathingLocation::SetAllMeshesReachable(int32_t aiVal) {
    uiFlags = static_cast<uint8_t>((uiFlags & ~kFlagAllMeshesReachable) | aiVal);
}

bool PathingLocation::HasNavMeshData() {
    if (pNavMesh == nullptr) {
        return false;
    }

    if (!pNavMesh->HasNavMesh()) {
        return false;
    }

    return usTriangle != 0xFFFF;
}

bool PathingLocation::GetNavMeshAndTriangle(NavMeshPtr& aRetNavMesh, uint16_t& aRetTriangle) const {
    if (pNavMesh == nullptr) {
        return false;
    }

    if (usTriangle == 0xFFFF) {
        return false;
    }

    if (!pNavMesh->GetNavMesh(aRetNavMesh)) {
        return false;
    }

    aRetTriangle = usTriangle;
    return true;
}

bool PathingLocation::ResolveNavMeshInfo(FindTriangleForLocationFilter* apFilter) {
    if (QUnused()) {
        return false;
    }
    if (pNavMesh == nullptr && pNavMeshes == nullptr) {
        return false;
    }

    if (pNavMesh != nullptr) {
        if (pNavMesh->GetDisabled()) {
            return false;
        }
        if (usTriangle != 0xFFFF) {
            return true;
        }
        alignas(NavMeshPtr) uint8_t aNavMeshStorage[sizeof(NavMeshPtr)];
        NavMeshPtr& spNavMesh = *reinterpret_cast<NavMeshPtr*>(aNavMeshStorage);
        ::new (aNavMeshStorage) NavMeshPtr;
        if (pNavMesh->GetNavMesh(spNavMesh)) {
            usTriangle = spNavMesh->FindTriangleForLocation(Location, apFilter);
        }
        spNavMesh.~NavMeshPtr();
        return true;
    }

    if (GetCell() != nullptr && GetCell()->GetAttached()) {
        // The cell is loaded: take the first enabled navmesh with a triangle under the point.
        for (uint32_t i = 0; i < pNavMeshes->GetSize(); ++i) {
            alignas(NavMeshPtr) uint8_t aNavMeshStorage[sizeof(NavMeshPtr)];
            NavMeshPtr& spNavMesh = *reinterpret_cast<NavMeshPtr*>(aNavMeshStorage);
            ::new (aNavMeshStorage) NavMeshPtr;
            if (const_cast<NavMeshInfo*>((*pNavMeshes)[i])->GetNavMesh(spNavMesh) && !spNavMesh->GetDisabled()) {
                const uint16_t usFoundTriangle = spNavMesh->FindTriangleForLocation(Location, apFilter);
                if (usFoundTriangle != 0xFFFF) {
                    pNavMesh = const_cast<NavMeshInfo*>((*pNavMeshes)[i]);
                    pNavMeshes = nullptr;
                    usTriangle = usFoundTriangle;
                    SetAllMeshesReachable(kFlagAllMeshesReachable);
                    spNavMesh.~NavMeshPtr();
                    return true;
                }
            }
            spNavMesh.~NavMeshPtr();
        }
        // Otherwise move onto the closest triangle, keeping the point itself.
        const NiPoint3 kLocation = Location;
        const bool bResolved = ResolveToClosestNavmeshAndTriangle();
        usTriangle = 0xFFFF;
        Location = kLocation;
        if (bResolved) {
            SetAllMeshesReachable(kFlagAllMeshesReachable);
        }
        else {
            SetAllMeshesReachable(QAllMeshesReachable());
        }
        return bResolved;
    }

    // The cell is not loaded: take the first enabled navmesh whose bounds hold the point.
    for (uint32_t i = 0; i < pNavMeshes->GetSize(); ++i) {
        const NavMeshInfo* pInfo = (*pNavMeshes)[i];
        if (pInfo->GetDisabled()) {
            continue;
        }
        NavMeshBounds* pBounds = pInfo->GetBounds();
        if (pBounds != nullptr && pBounds->IsPointInBounds(Location)) {
            pNavMesh = const_cast<NavMeshInfo*>((*pNavMeshes)[i]);
            pNavMeshes = nullptr;
            alignas(NavMeshPtr) uint8_t aNavMeshStorage[sizeof(NavMeshPtr)];
            NavMeshPtr& spNavMesh = *reinterpret_cast<NavMeshPtr*>(aNavMeshStorage);
            ::new (aNavMeshStorage) NavMeshPtr;
            if (pNavMesh->GetNavMesh(spNavMesh)) {
                usTriangle = spNavMesh->FindTriangleForLocation(Location, apFilter);
            }
            SetAllMeshesReachable(kFlagAllMeshesReachable);
            spNavMesh.~NavMeshPtr();
            return true;
        }
    }

    // Else the enabled navmesh whose bounds lie nearest, or the last one without bounds when none is within 128 units.
    // The squared distance is rounded to float before it is compared, as the game does.
    int32_t iNoBounds = -1;
    float fBestSqrDistance = 3.4028235e+38f;
    int32_t iBest = -1;
    for (uint32_t i = 0; i < pNavMeshes->GetSize(); ++i) {
        const NavMeshInfo* pInfo = (*pNavMeshes)[i];
        if (pInfo->GetDisabled()) {
            continue;
        }
        NavMeshBounds* pBounds = pInfo->GetBounds();
        if (pBounds != nullptr) {
            NiPoint3 kDistance = pBounds->DistanceFromPointToBounds(Location);
            const float fSqrDistance = kDistance.SqrLength();
            if (fBestSqrDistance > fSqrDistance) {
                fBestSqrDistance = fSqrDistance;
                iBest = i;
            }
        }
        else {
            iNoBounds = i;
        }
    }
    // A NaN distance takes the no-bounds branch.
    if (fBestSqrDistance < 16384.0) {
        pNavMesh = const_cast<NavMeshInfo*>((*pNavMeshes)[iBest]);
        pNavMeshes = nullptr;
        usTriangle = 0xFFFF;
        SetAllMeshesReachable(kFlagAllMeshesReachable);
        return true;
    }
    pNavMesh = const_cast<NavMeshInfo*>((*pNavMeshes)[iNoBounds]);
    pNavMeshes = nullptr;
    SetAllMeshesReachable(AreAllMeshesReachable());
    return QAllMeshesReachable();
}

void PathingLocation::SetTriangleIndex(uint16_t aiTriIndex) {
    usTriangle = aiTriIndex;
}

void PathingLocation::ClearTriangleData() {
    usTriangle = (uint16_t)((-1));
}

uint16_t PathingLocation::GetTriangle() const {
    return usTriangle;
}

const NavMeshInfo* PathingLocation::GetBestNavMeshInfo() {
    if (pNavMesh) {
        return pNavMesh;
    }
    if (!pNavMeshes) {
        return 0;
    }
    if (QAllMeshesReachable()) {
        return pNavMeshes->operator[](0);
    }
    return 0;
}

bool PathingLocation::HasNavMesh() {
    return (bool)((uint8_t)(pNavMesh || pNavMeshes));
}

bool PathingLocation::GetNavMesh(NavMeshPtr& aRetNavMesh) const {
    return (bool)((uint8_t)(pNavMesh && pNavMesh->GetNavMesh(aRetNavMesh)));
}

bool PathingLocation::IsCloseTo(const PathingLocation& aOtherLoc, float afTolerance) {
    NiPoint3 v5;
    if ((&aOtherLoc)->pWorldSpace != pWorldSpace) {
        return (bool)0;
    }
    if (pWorldSpace) {
        if ((&aOtherLoc)->iCellKey != iCellKey) {
            return (bool)0;
        }
    }
    else {
        if ((&aOtherLoc)->pCell != pCell) {
            return (bool)0;
        }
    }
    NiPoint3* v4 = &(v5 = (&(&aOtherLoc)->Location)->operator-(Location));
    return (bool)((uint8_t)(!((afTolerance * afTolerance) < v4->SqrLength())));
}

// The point is copied one component at a time, so a source overlapping Location reads already-copied components, as the
// game does.
void PathingLocation::SetLocation(const NiPoint3& aLocation) {
    Location.x = aLocation.x;
    Location.y = aLocation.y;
    Location.z = aLocation.z;
    usTriangle = 0xFFFF;
    if (pWorldSpace) {
        int32_t iKey = TESWorldSpace::GetKeyForWorldCoord(Location);
        if (static_cast<uint32_t>(iKey) != iCellKey) {
            SetupData(aLocation, nullptr, pWorldSpace);
        }
    }
}

bool PathingLocation::ResolveNavMeshInfoOnly() {
    const NavMeshInfo* pmesh;
    using allMeshes_t = BSSimpleArray<const NavMeshInfo*, 1024>;
    if (QUnused()) {
        return 0;
    }
    if (!pNavMesh && !pNavMeshes) {
        return 0;
    }
    if (pNavMesh) {
        return !pNavMesh->GetDisabled();
    }
    allMeshes_t allMeshes;
    // CopyTo<1024> takes its own array type, while the local holds const NavMeshInfo pointers.
    pNavMeshes->CopyTo(reinterpret_cast<BSSimpleArray<NavMeshInfo*, 1024>*>(&allMeshes));
    for (uint32_t i = 0;; ++i) {
        uint32_t FormFlags = allMeshes.iSize;
        if (i >= FormFlags) {
            break;
        }
        pmesh = allMeshes.GetAt(static_cast<int32_t>(i));
        NavMeshBounds* pbounds = pmesh->GetBounds();
        if (pbounds) {
            if (pbounds->IsPointInBounds(Location)) {
                allMeshes.Clear(0);
                allMeshes.Add(pmesh);
                break;
            }
            allMeshes.RemoveFast(i--, 1u);
        }
        else {
            NavMeshPtr spmesh;
            if (pmesh->GetNavMesh(spmesh) && spmesh.m_pObject->GetDelete()) {
                allMeshes.RemoveFast(i--, 1u);
            }
        }
    }
    if (allMeshes.iSize == 1) {
        pNavMesh = allMeshes.GetFirst();
        pNavMeshes = 0;
        SetAllMeshesReachable(1);
        return 1;
    }
    else {
        bool v3 = AreAllMeshesReachable();
        SetAllMeshesReachable(v3);
        bool v5 = QAllMeshesReachable();
        return v5;
    }
}

bool PathingLocation::AreAllMeshesReachable() {
    // The array is constructed once and destroyed before each return.
    BSScrapArray<const NavMeshInfo*> kReachable;
    if (pNavMesh != nullptr) {
        return true;
    }
    if (pNavMeshes == nullptr || pNavMeshes->IsEmpty()) {
        return false;
    }

    // Start from the first navmesh and the candidates adjacent to it. Passes operator[]'s result, the entry itself.
    kReachable.Add(const_cast<const NavMeshInfo* const&>((*pNavMeshes)[0]));
    uint32_t uiAdjacentCount = (*pNavMeshes)[0]->AdjacentNavMeshes.GetSize();
    uint32_t uiPreferredCount = (*pNavMeshes)[0]->PreferredAdjacentNavMeshes.GetSize();
    for (uint32_t i = 0; i < uiAdjacentCount + uiPreferredCount; ++i) {
        const NavMeshInfo* pAdjacent;
        if (i < uiAdjacentCount) {
            pAdjacent = const_cast<NavMeshInfo*>((*pNavMeshes)[0])->AdjacentNavMeshes.GetAt(i);
        }
        else {
            pAdjacent =
                const_cast<NavMeshInfo*>((*pNavMeshes)[0])->PreferredAdjacentNavMeshes.GetAt(i - uiAdjacentCount);
        }
        if (pNavMeshes->Find(
                const_cast<NavMeshInfo* const&>(pAdjacent), 0,
                &BSSimpleArray<NavMeshInfo*>::DefaultEqualFunctionByAddress
            ) != -1) {
            kReachable.Add(pAdjacent);
        }
    }
    uint32_t uiReachableCount = kReachable.GetSize();
    if (uiReachableCount == pNavMeshes->GetSize()) {
        return true;
    }

    // Grow the set until a pass adds nothing. Starts at the second entry and walks only as far as each navmesh's
    // adjacent count, so the preferred adjacent navmeshes are deliberately never reached here, as the game does.
    bool bAdded;
    do {
        bAdded = false;
        for (uint32_t i = 1; i < kReachable.GetSize(); ++i) {
            const NavMeshInfo* pInfo = kReachable.GetAt(i);
            uiAdjacentCount = pInfo->AdjacentNavMeshes.GetSize();
            uiPreferredCount = pInfo->PreferredAdjacentNavMeshes.GetSize();
            for (uint32_t j = 0; j < pInfo->AdjacentNavMeshes.GetSize(); ++j) {
                const NavMeshInfo* pAdjacent;
                if (j < uiAdjacentCount) {
                    pAdjacent = const_cast<NavMeshInfo*>(pInfo)->AdjacentNavMeshes.GetAt(j);
                }
                else {
                    pAdjacent = const_cast<NavMeshInfo*>(pInfo)->PreferredAdjacentNavMeshes.GetAt(j - uiAdjacentCount);
                }
                if (pNavMeshes->Find(
                        const_cast<NavMeshInfo* const&>(pAdjacent), 0,
                        &BSSimpleArray<NavMeshInfo*>::DefaultEqualFunctionByAddress
                    ) != -1 &&
                    kReachable.Find(pAdjacent, 0, &BSSimpleArray<const NavMeshInfo*>::DefaultEqualFunctionByAddress) ==
                        -1) {
                    kReachable.Add(pAdjacent);
                    bAdded = true;
                }
            }
        }
    }
    while (bAdded);

    uiReachableCount = kReachable.GetSize();
    if (uiReachableCount == pNavMeshes->GetSize()) {
        return true;
    }
    return false;
}

bool PathingLocation::ResolveToClosestNavmeshAndTriangle() {
    NiPoint3 v10;
    NiPoint3 v15;
    NavMesh::FIEResult res_2;
    bool bpointInside_2;
    NavMesh::FIEResult res;
    bool bpointInside;
    SetupData(Location, pCell, pWorldSpace);
    if (!pNavMesh && !pNavMeshes) {
        return 0;
    }
    if (pNavMesh && usTriangle != 0xFFFF) {
        return 1;
    }
    if (pNavMesh) {
        bpointInside = 0;
        NavMeshPtr spmesh;
        if (pNavMesh->GetNavMesh(spmesh)) {
            if (spmesh.m_pObject->FindClosestTriangleForLocation(Location, &usTriangle, &bpointInside)) {
                if (!bpointInside) {
                    NiPoint3 center = spmesh.m_pObject->GetCenter(usTriangle);
                    spmesh.m_pObject->FindInterectingEdge2(&res, usTriangle, -1, center, Location);
                    if (res.eType) {
                        NiPoint3* fScalar = ::new (&v15) NiPoint3(center * 0.1f);
                        Location = res.Intersection * 0.9f + *fScalar;
                    }
                }
                return 1;
            }
            else {
                return 0;
            }
        }
        else {
            return 0;
        }
    }
    else {
        float fminDistToPointSqr = 3.4028235e+38f;
        uint32_t ibestNavMesh = -1;
        uint16_t usbestTri = -1;
        bool bbestTriWithinZRange = 0;
        uint32_t isize = pNavMeshes->iSize;
        bool bunderWater = Location.z < TES::pInstance->GetWaterHeight(Location, pCell) + 25.0;
        for (uint32_t i = 0; i < isize; ++i) {
            uint16_t a3[3];
            a3[0] = -1;
            bpointInside_2 = 0;
            NavMeshPtr spmesh_2;
            const NavMeshInfo* pinfo = pNavMeshes->operator[](i);
            if (pinfo->GetDisabled() ||
                (!pinfo->GetNavMesh(spmesh_2) ||
                 spmesh_2.m_pObject->FindClosestTriangleForLocation(Location, a3, &bpointInside_2))) {
                if (bpointInside_2) {
                    pNavMesh = pNavMeshes->operator[](i);
                    pNavMeshes = 0;
                    usTriangle = a3[0];
                    bool v13 = 1;
                    return v13;
                }
                if (spmesh_2.operator NavMesh*()) {
                    NiPoint3 delta = Location - spmesh_2.m_pObject->GetCenter(a3[0]);
                    float fdistToPointSqr = delta.SqrXYLength();
                    float fzdist = delta.z;
                    bool bthisTriInZRange = 0;
                    if ((bunderWater || fzdist < 180.0) && fzdist > -64.0) {
                        bthisTriInZRange = 1;
                    }
                    if (bbestTriWithinZRange) {
                        if (bthisTriInZRange) {
                            if (fminDistToPointSqr > fdistToPointSqr) {
                                fminDistToPointSqr = fdistToPointSqr;
                                usbestTri = a3[0];
                                ibestNavMesh = i;
                            }
                        }
                    }
                    else {
                        if (bthisTriInZRange) {
                            fminDistToPointSqr = fdistToPointSqr;
                            usbestTri = a3[0];
                            ibestNavMesh = i;
                            bbestTriWithinZRange = 1;
                        }
                        else {
                            if (fminDistToPointSqr > fdistToPointSqr) {
                                fminDistToPointSqr = fdistToPointSqr;
                                usbestTri = a3[0];
                                ibestNavMesh = i;
                            }
                        }
                    }
                }
            }
        }
        if (ibestNavMesh == -1) {
            return 0;
        }
        else {
            pNavMesh = pNavMeshes->operator[](ibestNavMesh);
            pNavMeshes = 0;
            usTriangle = usbestTri;
            NavMeshPtr spmesh_3;
            if (pNavMesh->GetNavMesh(spmesh_3)) {
                NiPoint3 center_2 = spmesh_3.m_pObject->GetCenter(usbestTri);
                spmesh_3.m_pObject->FindInterectingEdge2(&res_2, usbestTri, -1, center_2, Location);
                if (res_2.eType) {
                    NiPoint3* fScalara = ::new (&v10) NiPoint3(res_2.Intersection * 0.9f);
                    Location = center_2 * 0.1f + *fScalara;
                }
                return 1;
            }
            else {
                return 0;
            }
        }
    }
}

void PathingLocation::SetupData(const NiPoint3& aLocation, TESObjectCELL* apCell, TESWorldSpace* apWorldSpace) {
    // The point is copied one component at a time, so a source overlapping Location reads already-copied components, as
    // the game does.
    Location.x = aLocation.x;
    Location.y = aLocation.y;
    Location.z = aLocation.z;
    uiFlags = 0;
    if (apWorldSpace != nullptr) {
        pWorldSpace = apWorldSpace;
        iCellKey = TESWorldSpace::GetKeyForWorldCoord(Location);
        pCell = nullptr;
        pNavMeshes = Pathing::GetPotentialNavMeshInfoForLocation(pWorldSpace, iCellKey);
    }
    else {
        pNavMeshes = Pathing::GetPotentialNavMeshInfoForLocation(apCell);
        if (apCell == nullptr || apCell->GetInterior()) {
            pCell = apCell;
            pWorldSpace = nullptr;
            iCellKey = 0xDEADDEAD;
        }
        else {
            pWorldSpace = apCell->GetWorldSpace();
            int32_t iY = apCell->GetDataY();
            int32_t iX = apCell->GetDataX();
            iCellKey = TESObjectCELL::GetCoord(iX, iY);
            pCell = nullptr;
        }
    }
    if (pNavMeshes != nullptr && pNavMeshes->GetSize() == 1) {
        SetAllMeshesReachable(kFlagAllMeshesReachable);
        pNavMesh = const_cast<NavMeshInfo*>((*pNavMeshes)[0]);
        pNavMeshes = nullptr;
    }
    else {
        SetAllMeshesReachable(0);
        pNavMesh = nullptr;
    }
    usTriangle = 0xFFFF;
}

void PathingLocation::SetNavMeshInfo(const NavMeshInfo* apNavMeshInfo) {
    pNavMeshes = 0;
    pNavMesh = apNavMeshInfo;
    pWorldSpace = apNavMeshInfo->GetWorldSpace();
    if (pWorldSpace) {
        pCell = 0;
        iCellKey = apNavMeshInfo->iCellKey;
    }
    else {
        pCell = apNavMeshInfo->GetInteriorCell();
        iCellKey = -0x21522153;
    }
    SetAllMeshesReachable(1);
    usTriangle = -1;
}

bool PathingLocation::GetNavMeshInfos(BSSimpleArray<const NavMeshInfo*, 1024>& apRetInfos) {
    if (pNavMesh) {
        apRetInfos.Add(pNavMesh);
        return 1;
    }
    else {
        if (pNavMeshes) {
            // CopyTo<1024> takes its own array type, while the caller's array holds const NavMeshInfo pointers, hence
            // the cast.
            pNavMeshes->CopyTo(reinterpret_cast<BSSimpleArray<NavMeshInfo*, 1024>*>(&apRetInfos));
            return 1;
        }
        else {
            return 0;
        }
    }
}

void PathingLocation::PrintDebugText(char* apBuffer, int32_t aiMaxSize) {
    if (pNavMesh && (!pNavMeshes || pNavMeshes->iSize)) {
        if (pNavMesh) {
            uint8_t navmeshinfo[256];
            if (usTriangle == 0xFFFF) {
                BSsprintf(apBuffer, aiMaxSize, "%s, %.2f %.2f %.2f", navmeshinfo, Location.x, Location.y, Location.z);
            }
            else {
                BSsprintf(
                    apBuffer, aiMaxSize, "%s, Tri %d, %.2f %.2f %.2f", navmeshinfo, usTriangle, Location.x, Location.y,
                    Location.z
                );
            }
        }
        else {
            char navmeshBuffer[256];
            BSsprintf(navmeshBuffer, aiMaxSize, "[");
            for (uint32_t i = 0;; ++i) {
                char Src[68];
                uint32_t FormFlags = pNavMeshes->iSize;
                if (i >= FormFlags) {
                    break;
                }
                BSsprintf(Src, 64, "%08x", pNavMeshes->operator[](i)->NavMeshID);
                BSstrcat(navmeshBuffer, 256, Src);
                uint32_t v4 = pNavMeshes->iSize;
                if (i < v4 - 1) {
                    BSstrcat(navmeshBuffer, 256, ", ");
                }
            }
            BSstrcat(navmeshBuffer, 256, "]");
            TESWorldSpace* pspace = pNavMeshes->operator[](0)->GetWorldSpace();
            TESObjectCELL* pcell_2 = pNavMeshes->operator[](0)->GetCell();
            if (pspace) {
                const char* v5;
                int32_t v17 = static_cast<int32_t>(Location.x) >> 12;
                int32_t v16 = static_cast<int32_t>(Location.y) >> 12;
                if (pcell_2 && (v5 = pcell_2->GetFormEditorID(), BSstrcmp(v5, ""))) {
                    double z = Location.z;
                    double y = Location.y;
                    double x = Location.x;
                    const char* v7 = pcell_2->GetFormEditorID();
                    BSsprintf(
                        apBuffer, aiMaxSize, "%s Cell %s[%d, %d](%s), %.2f %.2f %.2f", navmeshBuffer,
                        pspace->GetFormEditorID(), v17, v16, v7, x, y, z
                    );
                }
                else {
                    BSsprintf(
                        apBuffer, aiMaxSize, "%s Cell %s[%d, %d], %.2f %.2f %.2f", navmeshBuffer,
                        pspace->GetFormEditorID(), v17, v16, Location.x, Location.y, Location.z
                    );
                }
            }
            else {
                BSsprintf(
                    apBuffer, aiMaxSize, "%s Cell %s, %.2f %.2f %.2f", navmeshBuffer, pcell_2->GetFormEditorID(),
                    Location.x, Location.y, Location.z
                );
            }
        }
    }
    else {
        if (pWorldSpace) {
            int32_t cellx = static_cast<int32_t>(Location.x) >> 12;
            int32_t celly = static_cast<int32_t>(Location.y) >> 12;
            TESObjectCELL* pcell = pWorldSpace->GetCellFromWorldCoord(Location);
            if (pcell) {
                double v12 = Location.z;
                double v10 = Location.y;
                double v8 = Location.x;
                const char* v6 = pcell->GetFormEditorID();
                BSsprintf(
                    apBuffer, aiMaxSize, "Cell %s[%d,%d] %s,  %.2f %.2f %.2f", pWorldSpace->GetFormEditorID(), cellx,
                    celly, v6, v8, v10, v12
                );
            }
            else {
                BSsprintf(
                    apBuffer, aiMaxSize, "Cell %s[%d,%d], %.2f %.2f %.2f", pWorldSpace->GetFormEditorID(), cellx, celly,
                    Location.x, Location.y, Location.z
                );
            }
        }
        else {
            const char* v14;
            if (pCell) {
                v14 = pCell->GetFormEditorID();
            }
            else {
                v14 = "NONE";
            }
            BSsprintf(apBuffer, aiMaxSize, "Cell %s, %.2f %.2f %.2f", v14, Location.x, Location.y, Location.z);
        }
    }
}

void PathingLocation::operator delete(void* apMemory) {
    ourFree(apMemory);
}

NiPoint3* BGSWorldLocation::GetPoint() {
    return &Point;
}

bool NavMeshInfo::HasNavMesh() const {
    return pNavMesh != nullptr;
}
