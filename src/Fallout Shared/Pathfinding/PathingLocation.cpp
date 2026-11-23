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
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

#include <limits>

PathingLocation::PathingLocation()
  : Location(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()),
    pNavMesh(nullptr), pNavMeshes(nullptr), pCell(nullptr), pWorldSpace(nullptr), iCellKey(0xDEADDEAD),
    usTriangle(0xFFFF), uiFlags(0) {}

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
    if (!aHintLoc.GetNavMesh(spMesh) || (usTriangle = spMesh->FindTriangleForLocation(aLocation, nullptr)) == 0xFFFF) {
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

PathingLocation::PathingLocation(const NiPoint3* apLocation, TESObjectCELL* apCell, TESWorldSpace* apWorld) {
    SetupData(*apLocation, apCell, apWorld);
}

PathingLocation::PathingLocation(const NiPoint3* apLocation, const NavMeshInfo* apNavMeshInfo)
  : Location(apLocation->x, apLocation->y, apLocation->z), pNavMesh(apNavMeshInfo), pNavMeshes(nullptr),
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

PathingLocation::PathingLocation(const NiPoint3* apLocation, TESObjectCELL* apCell) {
    SetupData(*apLocation, apCell, nullptr);
}

PathingLocation::PathingLocation(const NiPoint3* apLocation, TESWorldSpace* apWorld) {
    SetupData(*apLocation, nullptr, apWorld);
}

PathingLocation::PathingLocation(BGSWorldLocation* apWorldLocation) {
    TESObjectCELL* pCellOut = nullptr;
    TESWorldSpace* pWorldOut = nullptr;
    apWorldLocation->GetCellOrWorld(pCellOut, pWorldOut);
    SetupData(*apWorldLocation->GetPoint(), pCellOut, pWorldOut);
}

void PathingLocation::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
    uint32_t uiFormID;
    apSaveGameBuffer->SaveDataEndian(&Location, sizeof(Location), 0);
    if (pNavMesh) {
        uiFormID = pNavMesh->GetNavMeshID();
    }
    else {
        uiFormID = 0;
    }
    apSaveGameBuffer->SaveFormID(uiFormID, 0);
    apSaveGameBuffer->SaveFormID(pCell, 0);
    apSaveGameBuffer->SaveFormID(pWorldSpace, 0);
    apSaveGameBuffer->SaveDataEndian(&iCellKey, sizeof(iCellKey), 0);
    apSaveGameBuffer->SaveDataEndian(&usTriangle, sizeof(usTriangle), 0);
    apSaveGameBuffer->SaveDataEndian(&uiFlags, sizeof(uiFlags), 0);
    int8_t cHasNavMeshes = pNavMeshes != nullptr;
    apSaveGameBuffer->SaveDataEndian(&cHasNavMeshes, sizeof(cHasNavMeshes), 0);
}

void PathingLocation::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
    bool bHadNavMeshesArray;
    apLoadGameBuffer->LoadData(&Location, sizeof(Location));
    uint32_t uiNavmeshID = 0;
    apLoadGameBuffer->LoadFormID(uiNavmeshID);
    uint32_t uiInfoID = uiNavmeshID;
    NavMeshInfoMap* pInfoMap = TES::pInstance->GetNavMeshInfoMap();
    pNavMesh = pInfoMap->GetNavMeshInfo(uiInfoID);
    uint32_t uiCellID = apLoadGameBuffer->LoadFormID();
    TESForm* pForm = TESForm::GetFormByNumericID(uiCellID);
    pCell = dynamic_cast<TESObjectCELL*>(pForm);
    uint32_t uiWorldSpaceID = apLoadGameBuffer->LoadFormID();
    pForm = TESForm::GetFormByNumericID(uiWorldSpaceID);
    pWorldSpace = dynamic_cast<TESWorldSpace*>(pForm);
    apLoadGameBuffer->LoadData(&iCellKey, sizeof(iCellKey));
    apLoadGameBuffer->LoadData(&usTriangle, sizeof(usTriangle));
    usTriangle = -1;
    apLoadGameBuffer->LoadData(&uiFlags, sizeof(uiFlags));
    bHadNavMeshesArray = false;
    apLoadGameBuffer->LoadData(&bHadNavMeshesArray, sizeof(bHadNavMeshesArray));
    if (bHadNavMeshesArray) {
        if (pWorldSpace) {
            pNavMeshes = Pathing::GetPotentialNavMeshInfoForLocation(pWorldSpace, iCellKey);
        }
        else if (pCell) {
            pNavMeshes = Pathing::GetPotentialNavMeshInfoForLocation(pCell);
        }
    }
}

PathingLocation::~PathingLocation() {}

bool PathingLocation::IsValidLocation(float afRadius) {
    NiPoint3 kMinY;
    NiPoint3 kMinX;
    NiPoint3 kMaxY;
    NiPoint3 kMaxX;
    NiPoint3 kLocation;
    ResolveNavMeshInfo(nullptr);
    if (!HasNavMeshData()) {
        return false;
    }
    if (afRadius <= 0.0) {
        return true;
    }
    kLocation = GetLocation();
    kMinX.y = kLocation.y;
    kMinX.z = kLocation.z;
    kMinX.x = kLocation.x - afRadius;
    if (!Pathing::CheckLineOfSight(
            *this,
            kMinX,
            static_cast<PathingLocation*>(nullptr),
            static_cast<Pathing::CheckLineOfSightFilter*>(nullptr)
        )) {
        return false;
    }
    kMaxX.y = kLocation.y;
    kMaxX.z = kLocation.z;
    kMaxX.x = kLocation.x + afRadius;
    if (!Pathing::CheckLineOfSight(
            *this,
            kMaxX,
            static_cast<PathingLocation*>(nullptr),
            static_cast<Pathing::CheckLineOfSightFilter*>(nullptr)
        )) {
        return false;
    }
    kMinY.x = kLocation.x;
    kMinY.z = kLocation.z;
    kMinY.y = kLocation.y - afRadius;
    if (!Pathing::CheckLineOfSight(
            *this,
            kMinY,
            static_cast<PathingLocation*>(nullptr),
            static_cast<Pathing::CheckLineOfSightFilter*>(nullptr)
        )) {
        return false;
    }
    kMaxY.x = kLocation.x;
    kMaxY.z = kLocation.z;
    kMaxY.y = kLocation.y + afRadius;
    return Pathing::CheckLineOfSight(
        *this,
        kMaxY,
        static_cast<PathingLocation*>(nullptr),
        static_cast<Pathing::CheckLineOfSightFilter*>(nullptr)
    );
}

NiPoint3 PathingLocation::GetLocation() const {
    return Location;
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
        spNavMesh = nullptr;
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
                    spNavMesh = nullptr;
                    return true;
                }
            }
            spNavMesh = nullptr;
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
            spNavMesh = nullptr;
            return true;
        }
    }

    // Else the enabled navmesh whose bounds lie nearest, or the last one without bounds when none is within 128 units.
    // The squared distance is rounded to float before it is compared.
    int32_t iNoBounds = -1;
    float fBestSqrDistance = std::numeric_limits<float>::max();
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
    usTriangle = static_cast<uint16_t>(-1);
}

uint16_t PathingLocation::GetTriangle() const {
    return usTriangle;
}

const NavMeshInfo* PathingLocation::GetBestNavMeshInfo() {
    if (pNavMesh) {
        return pNavMesh;
    }
    if (!pNavMeshes) {
        return nullptr;
    }
    if (QAllMeshesReachable()) {
        return (*pNavMeshes)[0];
    }
    return nullptr;
}

bool PathingLocation::HasNavMesh() {
    return pNavMesh || pNavMeshes;
}

bool PathingLocation::GetNavMesh(NavMeshPtr& aRetNavMesh) const {
    return pNavMesh && pNavMesh->GetNavMesh(aRetNavMesh);
}

bool PathingLocation::IsCloseTo(const PathingLocation& aOtherLoc, float afTolerance) {
    NiPoint3 kDelta;
    if (aOtherLoc.pWorldSpace != pWorldSpace) {
        return false;
    }
    if (pWorldSpace) {
        if (aOtherLoc.iCellKey != iCellKey) {
            return false;
        }
    }
    else if (aOtherLoc.pCell != pCell) {
        return false;
    }
    kDelta = aOtherLoc.Location - Location;
    return !(afTolerance * afTolerance < kDelta.SqrLength());
}

// The point is copied one component at a time, so a source overlapping Location reads already-copied components.
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
    const NavMeshInfo* pMesh;
    using allMeshes_t = BSSimpleArray<const NavMeshInfo*, 1024>;
    if (QUnused()) {
        return false;
    }
    if (!pNavMesh && !pNavMeshes) {
        return false;
    }
    if (pNavMesh) {
        return !pNavMesh->GetDisabled();
    }
    allMeshes_t allMeshes;
    // CopyTo<1024> takes its own array type, while the local holds const NavMeshInfo pointers.
    pNavMeshes->CopyTo(reinterpret_cast<BSSimpleArray<NavMeshInfo*, 1024>*>(&allMeshes));
    for (uint32_t i = 0; i < allMeshes.iSize; ++i) {
        pMesh = allMeshes.GetAt(static_cast<int32_t>(i));
        NavMeshBounds* pBounds = pMesh->GetBounds();
        if (pBounds) {
            if (pBounds->IsPointInBounds(Location)) {
                allMeshes.Clear(false);
                allMeshes.Add(pMesh);
                break;
            }
            allMeshes.RemoveFast(i--, 1u);
        }
        else {
            NavMeshPtr spMesh;
            if (pMesh->GetNavMesh(spMesh) && spMesh->GetDelete()) {
                allMeshes.RemoveFast(i--, 1u);
            }
        }
    }
    if (allMeshes.iSize == 1) {
        pNavMesh = allMeshes.GetFirst();
        pNavMeshes = nullptr;
        SetAllMeshesReachable(1);
        return true;
    }

    bool bReachable = AreAllMeshesReachable();
    SetAllMeshesReachable(bReachable);
    return QAllMeshesReachable();
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
            pAdjacent = const_cast<NavMeshInfo*>((*pNavMeshes)[0])
                            ->PreferredAdjacentNavMeshes.GetAt(i - uiAdjacentCount);
        }
        if (pNavMeshes->Find(
                const_cast<NavMeshInfo* const&>(pAdjacent),
                0,
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
    // adjacent count, so the preferred adjacent navmeshes are deliberately never reached here.
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
                        const_cast<NavMeshInfo* const&>(pAdjacent),
                        0,
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
    return uiReachableCount == pNavMeshes->GetSize();
}

bool PathingLocation::ResolveToClosestNavmeshAndTriangle() {
    NiPoint3 kScaled;
    NavMesh::FIEResult res;
    bool bPointInside;
    SetupData(Location, pCell, pWorldSpace);
    if (!pNavMesh && !pNavMeshes) {
        return false;
    }
    if (pNavMesh && usTriangle != 0xFFFF) {
        return true;
    }
    if (pNavMesh) {
        bPointInside = false;
        NavMeshPtr spMesh;
        if (pNavMesh->GetNavMesh(spMesh)) {
            if (spMesh->FindClosestTriangleForLocation(Location, &usTriangle, &bPointInside)) {
                if (!bPointInside) {
                    NiPoint3 kCenter = spMesh->GetCenter(usTriangle);
                    spMesh->FindInterectingEdge2(&res, usTriangle, -1, kCenter, Location);
                    if (res.eType) {
                        NiPoint3* pScaledCenter = ::new (&kScaled) NiPoint3(kCenter * 0.1f);
                        Location = res.Intersection * 0.9f + *pScaledCenter;
                    }
                }
                return true;
            }

            return false;
        }

        return false;
    }

    float fMinDistToPointSqr = std::numeric_limits<float>::max();
    uint32_t uiBestNavMesh = -1;
    uint16_t usBestTri = -1;
    bool bBestTriWithinZRange = false;
    uint32_t uiSize = pNavMeshes->iSize;
    bool bUnderWater = Location.z < TES::pInstance->GetWaterHeight(Location, pCell) + 25.0;
    for (uint32_t i = 0; i < uiSize; ++i) {
        uint16_t usTriangles[3];
        usTriangles[0] = -1;
        bPointInside = false;
        NavMeshPtr spMesh;
        const NavMeshInfo* pInfo = (*pNavMeshes)[i];
        if (pInfo->GetDisabled() ||
            !pInfo->GetNavMesh(spMesh) ||
            spMesh->FindClosestTriangleForLocation(Location, usTriangles, &bPointInside)) {
            if (bPointInside) {
                pNavMesh = (*pNavMeshes)[i];
                pNavMeshes = nullptr;
                usTriangle = usTriangles[0];
                return true;
            }
            if (spMesh) {
                NiPoint3 kDelta = Location - spMesh->GetCenter(usTriangles[0]);
                float fDistToPointSqr = kDelta.SqrXYLength();
                float fZDist = kDelta.z;
                bool bThisTriInZRange = false;
                if ((bUnderWater || fZDist < 180.0) && fZDist > -64.0) {
                    bThisTriInZRange = true;
                }
                if (bBestTriWithinZRange) {
                    if (bThisTriInZRange && fMinDistToPointSqr > fDistToPointSqr) {
                        fMinDistToPointSqr = fDistToPointSqr;
                        usBestTri = usTriangles[0];
                        uiBestNavMesh = i;
                    }
                }
                else if (bThisTriInZRange) {
                    fMinDistToPointSqr = fDistToPointSqr;
                    usBestTri = usTriangles[0];
                    uiBestNavMesh = i;
                    bBestTriWithinZRange = true;
                }
                else if (fMinDistToPointSqr > fDistToPointSqr) {
                    fMinDistToPointSqr = fDistToPointSqr;
                    usBestTri = usTriangles[0];
                    uiBestNavMesh = i;
                }
            }
        }
    }
    if (uiBestNavMesh == -1) {
        return false;
    }

    pNavMesh = (*pNavMeshes)[uiBestNavMesh];
    pNavMeshes = nullptr;
    usTriangle = usBestTri;
    NavMeshPtr spMesh;
    if (pNavMesh->GetNavMesh(spMesh)) {
        NiPoint3 kCenter = spMesh->GetCenter(usBestTri);
        spMesh->FindInterectingEdge2(&res, usBestTri, -1, kCenter, Location);
        if (res.eType) {
            NiPoint3* pScaledIntersection = ::new (&kScaled) NiPoint3(res.Intersection * 0.9f);
            Location = kCenter * 0.1f + *pScaledIntersection;
        }
        return true;
    }

    return false;
}

void PathingLocation::SetupData(const NiPoint3& aLocation, TESObjectCELL* apCell, TESWorldSpace* apWorldSpace) {
    // The point is copied one component at a time, so a source overlapping Location reads already-copied components.
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
    pNavMeshes = nullptr;
    pNavMesh = apNavMeshInfo;
    pWorldSpace = apNavMeshInfo->GetWorldSpace();
    if (pWorldSpace) {
        pCell = nullptr;
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
        return true;
    }

    if (pNavMeshes) {
        // CopyTo<1024> takes its own array type, while the caller's array holds const NavMeshInfo pointers, hence
        // the cast.
        pNavMeshes->CopyTo(reinterpret_cast<BSSimpleArray<NavMeshInfo*, 1024>*>(&apRetInfos));
        return true;
    }

    return false;
}

void PathingLocation::PrintDebugText(char* apBuffer, int32_t aiMaxSize) {
    if (pNavMesh && (!pNavMeshes || pNavMeshes->iSize)) {
        if (pNavMesh) {
            uint8_t aucNavmeshInfo[256];
            if (usTriangle == 0xFFFF) {
                BSsprintf(
                    apBuffer,
                    aiMaxSize,
                    "%s, %.2f %.2f %.2f",
                    aucNavmeshInfo,
                    Location.x,
                    Location.y,
                    Location.z
                );
            }
            else {
                BSsprintf(
                    apBuffer,
                    aiMaxSize,
                    "%s, Tri %d, %.2f %.2f %.2f",
                    aucNavmeshInfo,
                    usTriangle,
                    Location.x,
                    Location.y,
                    Location.z
                );
            }
        }
        else {
            char acNavmeshBuffer[256];
            BSsprintf(acNavmeshBuffer, aiMaxSize, "[");
            for (uint32_t i = 0; i < pNavMeshes->iSize; ++i) {
                char acID[68];
                BSsprintf(acID, 64, "%08x", (*pNavMeshes)[i]->NavMeshID);
                BSstrcat(acNavmeshBuffer, 256, acID);
                if (i < pNavMeshes->iSize - 1) {
                    BSstrcat(acNavmeshBuffer, 256, ", ");
                }
            }
            BSstrcat(acNavmeshBuffer, 256, "]");
            TESWorldSpace* pSpace = (*pNavMeshes)[0]->GetWorldSpace();
            TESObjectCELL* pNavMeshCell = (*pNavMeshes)[0]->GetCell();
            if (pSpace) {
                int32_t iCellX = static_cast<int32_t>(Location.x) >> 12;
                int32_t iCellY = static_cast<int32_t>(Location.y) >> 12;
                if (pNavMeshCell && BSstrcmp(pNavMeshCell->GetFormEditorID(), "")) {
                    double dZ = Location.z;
                    double dY = Location.y;
                    double dX = Location.x;
                    const char* pcCellEditorID = pNavMeshCell->GetFormEditorID();
                    BSsprintf(
                        apBuffer,
                        aiMaxSize,
                        "%s Cell %s[%d, %d](%s), %.2f %.2f %.2f",
                        acNavmeshBuffer,
                        pSpace->GetFormEditorID(),
                        iCellX,
                        iCellY,
                        pcCellEditorID,
                        dX,
                        dY,
                        dZ
                    );
                }
                else {
                    BSsprintf(
                        apBuffer,
                        aiMaxSize,
                        "%s Cell %s[%d, %d], %.2f %.2f %.2f",
                        acNavmeshBuffer,
                        pSpace->GetFormEditorID(),
                        iCellX,
                        iCellY,
                        Location.x,
                        Location.y,
                        Location.z
                    );
                }
            }
            else {
                BSsprintf(
                    apBuffer,
                    aiMaxSize,
                    "%s Cell %s, %.2f %.2f %.2f",
                    acNavmeshBuffer,
                    pNavMeshCell->GetFormEditorID(),
                    Location.x,
                    Location.y,
                    Location.z
                );
            }
        }
    }
    else if (pWorldSpace) {
        int32_t iCellX = static_cast<int32_t>(Location.x) >> 12;
        int32_t iCellY = static_cast<int32_t>(Location.y) >> 12;
        TESObjectCELL* pWorldCell = pWorldSpace->GetCellFromWorldCoord(Location);
        if (pWorldCell) {
            double dZ = Location.z;
            double dY = Location.y;
            double dX = Location.x;
            const char* pcCellEditorID = pWorldCell->GetFormEditorID();
            BSsprintf(
                apBuffer,
                aiMaxSize,
                "Cell %s[%d,%d] %s,  %.2f %.2f %.2f",
                pWorldSpace->GetFormEditorID(),
                iCellX,
                iCellY,
                pcCellEditorID,
                dX,
                dY,
                dZ
            );
        }
        else {
            BSsprintf(
                apBuffer,
                aiMaxSize,
                "Cell %s[%d,%d], %.2f %.2f %.2f",
                pWorldSpace->GetFormEditorID(),
                iCellX,
                iCellY,
                Location.x,
                Location.y,
                Location.z
            );
        }
    }
    else {
        const char* pcCellName;
        if (pCell) {
            pcCellName = pCell->GetFormEditorID();
        }
        else {
            pcCellName = "NONE";
        }
        BSsprintf(apBuffer, aiMaxSize, "Cell %s, %.2f %.2f %.2f", pcCellName, Location.x, Location.y, Location.z);
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
