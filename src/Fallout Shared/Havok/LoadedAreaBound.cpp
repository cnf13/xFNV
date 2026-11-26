#include "Fallout Shared/Havok/LoadedAreaBound.h"

#include "BSCore/BSSimpleArray.h"
#include "BSHavok/bhkAabbPhantom.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkRagdollPenetrationUtil.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/Havok/CellMopp.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "SDK/MSVC/ehvec.h"

#include <cmath>

LoadedAreaBound::LoadedAreaBound(
    float afBoundOffset,
    float afBoundDepth,
    float afSideWallHeightOffset,
    float afVelocityDampen
)
  : pCurrentCell(nullptr), HitMap(37u), fBoundOffset(afBoundOffset), fBoundDepth(afBoundDepth),
    fSideWallHeightOffset(afSideWallHeightOffset), fVelocityDampen(afVelocityDampen) {}

// The phantoms go before the hit map is emptied.
LoadedAreaBound::~LoadedAreaBound() {
    for (NiPointer<bhkAabbPhantom>& spPhantom : spPhantomA) {
        spPhantom = nullptr;
    }
    HitMap.RemoveAll();
}

void LoadedAreaBound::Clear() {
    pCurrentCell = nullptr;

    for (NiPointer<bhkAabbPhantom>& spPhantom : spPhantomA) {
        spPhantom = nullptr;
    }

    HitMap.RemoveAll();
}

// The world comes from the cell, the dimension and the cells from apCellGrid.
bool LoadedAreaBound::GetBoundFromCell(TESObjectCELL* apCell, GridCellArray* apCellGrid, hkAabb& arBound) {
    bool bResult = false;
    bhkWorld* pbhkWorld = apCell->GetbhkWorld();
    ahkpWorld* pWorld = pbhkWorld ? **pbhkWorld : nullptr;
    if (pWorld) {
        // The local's declaration runs the empty hkAabb default constructor, which stores nothing; a further
        // hkAabb::Construct would construct the box twice.
        alignas(16) hkAabb kAabb;
        alignas(16) hkVector4 kMin;
        alignas(16) hkVector4 kMax;
        kMax.setAll(-HK_REAL_MAX);
        kMin.setAll(HK_REAL_MAX);
        if (apCell->GetInterior()) {
            const hkArray<hkpEntity*>& rEntities = pWorld->getFixedIsland()->getEntities();
            for (hkpEntity* pEntity : rEntities) {
                const hkpCollidable* pCollidable = pEntity->getCollidable();
                if (pCollidable->hkpCdBody::getShape()) {
                    hkpBroadPhase* pBroadPhase = pWorld->getBroadPhase();
                    pBroadPhase->getAabb(const_cast<hkpCollidable*>(pCollidable)->getBroadPhaseHandle(), kAabb);
                    kMax.setMax4(kMax, kAabb.m_max);
                    kMin.setMin4(kMin, kAabb.m_min);
                }
            }
        }
        else if (apCellGrid) {
            uint32_t uiDimension = apCellGrid->GetDimension();
            for (uint32_t x = 0; x < uiDimension; ++x) {
                for (uint32_t y = 0; y < uiDimension; ++y) {
                    GridCell* pGridCell = apCellGrid->Get(x, y);
                    if (!pGridCell->pCell) {
                        continue;
                    }
                    NiTObjectArray<NiPointer<bhkRigidBody>>& rBodies = pGridCell->pCell->GetCellMopp()->GetRigidBodyA();
                    for (const NiPointer<bhkRigidBody>& spBody : rBodies) {
                        const hkpCollidable* pCollidable = spBody->GetCollidable();
                        if (!pCollidable->hkpCdBody::getShape()) {
                            continue;
                        }
                        bhkRigidBody* pBody = spBody;
                        if (pBody->GethkWorld()) {
                            hkpBroadPhase* pBroadPhase = pWorld->getBroadPhase();
                            pBroadPhase->getAabb(const_cast<hkpCollidable*>(pCollidable)->getBroadPhaseHandle(), kAabb);
                        }
                        else {
                            alignas(16) hkTransform kTransform;
                            spBody->GetTransform(kTransform);
                            hkpShape* pShape = spBody->GetShape()->Gethk();
                            pShape->getAabb(kTransform, 0.1f, kAabb);
                        }
                        kMax.setMax4(kMax, kAabb.m_max);
                        kMin.setMin4(kMin, kAabb.m_min);
                    }
                }
            }
        }
        arBound.m_min = kMin;
        arBound.m_max = kMax;
        // Each float corner is compared with the double FLT_MAX constants; a NaN counts as different.
        bResult = static_cast<double>(kMax(0)) != -3.402820018375656e+38 &&
            static_cast<double>(kMin(0)) != 3.402820018375656e+38;
    }
    return bResult;
}

void LoadedAreaBound::WarpActor(Actor* apActor) {
    NiPoint3 warpLoc;
    FindWarpLocation(apActor, apActor->GetLocationOnReference(), warpLoc);
    apActor->SetPosition(warpLoc);
    if (apActor->GetCharController()) {
        apActor->GetCharController()->SetStuckQuadruped(false);
    }
}

// Finds where to warp apRef back into the loaded area: the navmesh height under arCurLoc; else, for an actor, its
// editor location when that lies in the current cell; else, for any other reference, its editor location when that lies
// in an attached cell. A location found must lie in the box spanned by the bound's phantoms 2 and 5; without one the
// current cell's COC placement is taken.
void LoadedAreaBound::FindWarpLocation(TESObjectREFR* apRef, const NiPoint3& arCurLoc, NiPoint3& arWarpLoc) {
    TESForm* pLocForm = nullptr;
    TESObjectCELL* pCell = pCurrentCell;
    Actor* pAct = nullptr;
    if (apRef && apRef->IsActor()) {
        pAct = static_cast<Actor*>(apRef);
    }
    NiPoint3 rot;
    bool bLocFound = false;
    NiPoint3 tmpLoc = arCurLoc;
    float fHeightNavMesh;
    if (apRef && Pathing::GetNavMeshZForLocation(tmpLoc, pCell, pCell->GetWorldSpace(), fHeightNavMesh)) {
        tmpLoc.z = fHeightNavMesh;
        arWarpLoc = tmpLoc;
        bLocFound = true;
    }
    else if (pAct && pAct->GetEditorLocationInteriorCell() == pCurrentCell) {
        arWarpLoc = pAct->GetEditorLocationCoord();
        bLocFound = true;
    }
    else if (!pAct && apRef && apRef->GetEditorLocation(tmpLoc, rot, pLocForm, nullptr)) {
        TESObjectCELL* pCellLoc = TES::pInstance->GetCellForPoint(tmpLoc);
        if (pCellLoc && pCellLoc->GetState() == TESObjectCELL::CS_ATTACHED) {
            arWarpLoc = tmpLoc;
            bLocFound = true;
        }
    }
    if (bLocFound) {
        hkVector4 warpLoc;
        NI2HK(warpLoc, arWarpLoc);
        hkAabb boundAabb;
        boundAabb.setEmpty();
        boundAabb.includePoint(spPhantomA[2]->Gethk()->m_aabb.m_max);
        boundAabb.includePoint(spPhantomA[5]->Gethk()->m_aabb.m_min);
        bLocFound = boundAabb.containsPoint(warpLoc) != 0;
    }
    if (!bLocFound) {
        pCell->GetCOCPlacementInfo(arWarpLoc, rot);
    }
}

// The phantom of each of the six sides sends its overlapping actors and bodies back into the loaded area: the floor's
// (ui 0), and every side's in an interior, warps them; the walls' dampen and reverse the velocity of each body that has
// not hit that side yet. The sides each body hits are collected in a map that then replaces HitMap.
void LoadedAreaBound::Update(TESObjectCELL* apCell, GridCellArray* apCellGrid) {
    if (apCell != pCurrentCell && apCell) {
        hkAabb loadedAreaAABB;
        if (!GetBoundFromCell(apCell, apCellGrid, loadedAreaAABB)) {
            return;
        }
        Create(apCell, loadedAreaAABB);
        pCurrentCell = apCell;
        CheckForDynamicBodiesOutsideBound();
    }
    if (!apCell) {
        pCurrentCell = apCell;
        Clear();
        return;
    }
    uint16_t usCurrentFlag = 1;
    using curhitMap_t = NiTMap<bhkRigidBody*, uint16_t>;
    curhitMap_t curHitMap(37u);
    for (uint32_t ui = 0; ui < 6; ++ui, usCurrentFlag <<= 1) {
        bool bUseWarp = pCurrentCell->GetInterior() || ui == 0;
        hkpAabbPhantom* pphantom = spPhantomA[ui]->Gethk();
        // The overlap array's getter is a trivial accessor.
        hkArray<hkpCollidable*, hkContainerHeapAllocator>* pCols = &pphantom->m_overlappingCollidables;
        if (pCols->isEmpty()) {
            continue;
        }
        int32_t iSize = pCols->getSize();
        using PhantomArray = hkArray<hkpPhantom*, hkContainerHeapAllocator>;
        using RigidBodyArray = hkArray<hkpRigidBody*, hkContainerHeapAllocator>;
        PhantomArray phantomA;
        RigidBodyArray bodyA;
        bodyA.reserve(pCols->getSize());
        for (int32_t i = 0; i < iSize; ++i) {
            alignas(16) hkpRigidBody* prb = hkpGetRigidBody((*pCols)[i]);
            if (prb) {
                bodyA.pushBack(prb);
            }
            else {
                hkpPhantom* pp = hkpGetPhantom((*pCols)[i]);
                if (pp) {
                    phantomA.pushBack(pp);
                }
            }
        }
        for (hkpPhantom* pPhantom : phantomA) {
            if (!bUseWarp) {
                continue;
            }
            CFilter filter;
            filter.iFilter = pPhantom->getCollidable()->getCollisionFilterInfo();
            TESObjectREFR* pRef = TESHavokUtilities::FindCollidableRef(pPhantom->getCollidable());
            if (filter.GetLayer() == L_CHARCONTROLLER && pRef && pRef->IsActor()) {
                WarpActor(static_cast<Actor*>(pRef));
            }
        }
        alignas(16) RigidBodyArray warpedChildBodies;
        for (hkpRigidBody* prb : bodyA) {
            if (warpedChildBodies.indexOf(prb, 0, -1) != -1) {
                continue;
            }
            // Looks the body's reference up before its null test and never uses it.
            TESHavokUtilities::FindCollidableRef(prb->getCollidable());
            if (!prb ||
                prb->getMotionType() == hkpMotion::MOTION_KEYFRAMED ||
                prb->getMotionType() == hkpMotion::MOTION_FIXED ||
                prb->getMotionType() == hkpMotion::MOTION_CHARACTER) {
                continue;
            }
            bhkRigidBody* pbhkRb = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(prb));
            if (bUseWarp) {
                RigidBodyArray warpedBodies;
                WarpBody(pbhkRb, warpedBodies);
                for (hkpRigidBody* pWarpedBody : warpedBodies) {
                    warpedChildBodies.pushBack(pWarpedBody);
                }
                continue;
            }
            uint16_t usBorderFlags;
            bool bContains = HitMap.GetAt(pbhkRb, usBorderFlags);
            if (!bContains || (usBorderFlags & usCurrentFlag) == 0) {
                hkVector4 curVel(pbhkRb->GetLinearVelocity());
                // -1.0 is a double constant; the product is stored as a double, then as a float.
                float fDampMult = ui == 1 ? fVelocityDampen : fVelocityDampen * -1.0;
                hkSimdReal kDampMult(fDampMult);
                curVel.mul4(kDampMult);
                float& rfZ = curVel(2);
                rfZ = rfZ * -1.0;
                pbhkRb->SetLinearVelocity(curVel);
            }
            uint16_t usBodyCurrFlags = usCurrentFlag;
            if (curHitMap.GetAt(pbhkRb, usBodyCurrFlags)) {
                usBodyCurrFlags |= usCurrentFlag;
            }
            curHitMap.SetAt(pbhkRb, usBodyCurrFlags);
        }
    }
    bhkRigidBody* pbody;
    alignas(16) uint16_t usBorderFlags;
    NiTMapItem<bhkRigidBody*, uint16_t>* it = curHitMap.GetFirstPos();
    while (it) {
        curHitMap.GetNext(it, pbody, usBorderFlags);
        HitMap.SetAt(pbody, usBorderFlags);
    }
    using removeBodyA_t = BSSimpleArray<bhkRigidBody*, 1024>;
    removeBodyA_t removeBodyA;
    it = HitMap.GetFirstPos();
    while (it) {
        HitMap.GetNext(it, pbody, usBorderFlags);
        if (!curHitMap.GetAt(pbody, usBorderFlags)) {
            removeBodyA.Add(pbody);
        }
    }
    for (bhkRigidBody* pBody : removeBodyA) {
        HitMap.RemoveAt(pBody);
    }
}

// It realigns its frame to 16: the filter, the creation info, the box and both extents sit at 16-aligned slots. Each
// corner component goes through hkVector4::operator() and is rounded to a float before hkVector4::set takes it.
void LoadedAreaBound::Create(TESObjectCELL* apCell, const hkAabb& arboundAABB) {
    bhkWorld* pWorld = apCell->GetbhkWorld();
    for (uint32_t ui = 0; ui < 6; ++ui) {
        alignas(16) hkVector4 minExtent;
        alignas(16) hkVector4 maxExtent;
        this->spPhantomA[ui] = static_cast<bhkAabbPhantom*>(nullptr);
        hkAabbPhantomCinfo ci;
        hkAabb bound;
        float fHeight = apCell->GetInterior() ? 0.0f : this->fSideWallHeightOffset;
        switch (ui) {
            case 0u: {
                float fZ = arboundAABB.m_min(2) - this->fBoundOffset;
                float fY = arboundAABB.m_max(1) + this->fBoundDepth;
                float fX = arboundAABB.m_max(0) + this->fBoundDepth;
                maxExtent.set(fX, fY, fZ, 0.0f);
                float fZ2 = arboundAABB.m_min(2) - this->fBoundDepth;
                float fY2 = arboundAABB.m_min(1) - this->fBoundDepth;
                float fX2 = arboundAABB.m_min(0) - this->fBoundDepth;
                minExtent.set(fX2, fY2, fZ2, 0.0f);
                break;
            }
            case 1u: {
                float fZ = arboundAABB.m_max(2) + this->fBoundDepth + fHeight;
                float fY = arboundAABB.m_max(1) + this->fBoundDepth;
                float fX = arboundAABB.m_max(0) + this->fBoundDepth;
                maxExtent.set(fX, fY, fZ, 0.0f);
                float fZ2 = arboundAABB.m_max(2) + fHeight + this->fBoundOffset;
                float fY2 = arboundAABB.m_min(1) - this->fBoundDepth;
                float fX2 = arboundAABB.m_min(0) - this->fBoundDepth;
                minExtent.set(fX2, fY2, fZ2, 0.0f);
                break;
            }
            case 2u: {
                float fZ = arboundAABB.m_max(2) + fHeight + this->fBoundOffset;
                float fY = arboundAABB.m_min(1) - this->fBoundOffset;
                float fX = arboundAABB.m_max(0) + this->fBoundOffset;
                maxExtent.set(fX, fY, fZ, 0.0f);
                float fZ2 = arboundAABB.m_min(2) - this->fBoundOffset;
                float fY2 = arboundAABB.m_min(1) - this->fBoundDepth;
                float fX2 = arboundAABB.m_min(0) - this->fBoundOffset;
                minExtent.set(fX2, fY2, fZ2, 0.0f);
                break;
            }
            case 3u: {
                float fZ = arboundAABB.m_max(2) + fHeight + this->fBoundOffset;
                float fY = arboundAABB.m_max(1) + this->fBoundDepth;
                float fX = arboundAABB.m_min(0) - this->fBoundOffset;
                maxExtent.set(fX, fY, fZ, 0.0f);
                float fZ2 = arboundAABB.m_min(2) - this->fBoundOffset;
                float fY2 = arboundAABB.m_min(1) - this->fBoundDepth;
                float fX2 = arboundAABB.m_min(0) - this->fBoundDepth;
                minExtent.set(fX2, fY2, fZ2, 0.0f);
                break;
            }
            case 4u: {
                float fZ = arboundAABB.m_max(2) + fHeight + this->fBoundOffset;
                float fY = arboundAABB.m_max(1) + this->fBoundDepth;
                float fX = arboundAABB.m_max(0) + this->fBoundDepth;
                maxExtent.set(fX, fY, fZ, 0.0f);
                float fZ2 = arboundAABB.m_min(2) - this->fBoundOffset;
                float fY2 = arboundAABB.m_min(1) - this->fBoundDepth;
                float fX2 = arboundAABB.m_max(0) + this->fBoundOffset;
                minExtent.set(fX2, fY2, fZ2, 0.0f);
                break;
            }
            case 5u: {
                float fZ = arboundAABB.m_max(2) + fHeight + this->fBoundOffset;
                float fY = arboundAABB.m_max(1) + this->fBoundDepth;
                float fX = arboundAABB.m_max(0) + this->fBoundOffset;
                maxExtent.set(fX, fY, fZ, 0.0f);
                float fZ2 = arboundAABB.m_min(2) - this->fBoundOffset;
                float fY2 = arboundAABB.m_max(1) + this->fBoundOffset;
                float fX2 = arboundAABB.m_min(0) - this->fBoundOffset;
                minExtent.set(fX2, fY2, fZ2, 0.0f);
                break;
            }
            default: {
                break;
            }
        }
        bound.m_max = maxExtent;
        bound.m_min = minExtent;
        ci.aabb = bound;
        uint32_t uiSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
        ci.m_collisionFilterInfo = CFilter(L_UNIDENTIFIED, uiSystemGroup, P_OTHER).iFilter;
        bhkAabbPhantom* pPhantom = new bhkAabbPhantom(ci);
        this->spPhantomA[ui] = pPhantom;
        pWorld->Add(this->spPhantomA[ui].m_pObject);
    }
}

void LoadedAreaBound::CheckForDynamicBodiesOutsideBound() {
    hkAabb boundAABB;
    alignas(16) hkVector4 minExtent;
    alignas(16) hkVector4 maxExtent;
    maxExtent.setAll(-HK_REAL_MAX);
    minExtent.setAll(HK_REAL_MAX);
    for (const NiPointer<bhkAabbPhantom>& spPhantom : this->spPhantomA) {
        hkAabb phantomAABB;
        spPhantom->GetAabb(phantomAABB);
        maxExtent.setMax4(maxExtent, phantomAABB.m_max);
        minExtent.setMin4(minExtent, phantomAABB.m_min);
    }
    boundAABB.m_min = minExtent;
    boundAABB.m_max = maxExtent;
    bhkWorld* pbhkWorld = this->pCurrentCell->GetbhkWorld();
    hkpWorld* pWorld = pbhkWorld ? **pbhkWorld : nullptr;
    if (pWorld) {
        const hkArray<hkpPhantom*, hkContainerHeapAllocator>& rPhantoms = pWorld->getPhantoms();
        this->CheckPhantomsOutsideAABB(boundAABB, rPhantoms, pWorld);
        this->SetBodiesMotionOutsideAABB(boundAABB, pWorld->m_activeSimulationIslands);
        this->SetBodiesMotionOutsideAABB(boundAABB, pWorld->m_inactiveSimulationIslands);
    }
}

// Collects the moving bodies of arSimIslandA's islands whose broad-phase box is not inside boundAABB, or does not
// overlap it, and warps each that no warp of this pass moved yet, with the bodies constrained to it.
void LoadedAreaBound::SetBodiesMotionOutsideAABB(
    hkAabb& boundAABB,
    const hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>& arSimIslandA
) {
    hkAabb aabb;
    hkpWorld* pWorld = nullptr;
    alignas(16) hkArray<hkpRigidBody*, hkContainerHeapAllocator> bodiesToBeWarped;
    bodiesToBeWarped.~hkArray<hkpRigidBody*, hkContainerHeapAllocator>();
    ::new (&bodiesToBeWarped) hkArray<hkpRigidBody*, hkContainerHeapAllocator>;
    for (hkpSimulationIsland* pIsland : arSimIslandA) {
        pWorld = pIsland->getWorld();
        const hkArray<hkpEntity*>& entitiesA = pIsland->getEntities();
        for (hkpEntity* pEntity : entitiesA) {
            const hkpCollidable* pCol = pEntity->getCollidable();
            if (pCol->hkpCdBody::getShape()) {
                hkpBroadPhase* pBroadPhase = pWorld->getBroadPhase();
                pBroadPhase->getAabb(&pCol->m_broadPhaseHandle, aabb);
                hkBool bContains = boundAABB.contains(aabb);
                if (!bContains || !boundAABB.overlaps(aabb)) {
                    hkpRigidBody* prb = hkpGetRigidBody(pCol);
                    if (prb &&
                        prb->getMotionType() != hkpMotion::MOTION_KEYFRAMED &&
                        prb->getMotionType() != hkpMotion::MOTION_FIXED &&
                        prb->getMotionType() != hkpMotion::MOTION_CHARACTER) {
                        bodiesToBeWarped.pushBack(prb);
                    }
                }
            }
        }
    }
    hkArray<hkpRigidBody*, hkContainerHeapAllocator> warpedChildBodies;
    warpedChildBodies.~hkArray<hkpRigidBody*, hkContainerHeapAllocator>();
    ::new (&warpedChildBodies) hkArray<hkpRigidBody*, hkContainerHeapAllocator>;
    for (hkpRigidBody* prb : bodiesToBeWarped) {
        if (warpedChildBodies.indexOf(prb, 0, -1) != -1) {
            continue;
        }
        bhkRigidBody* pbhkRb = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(prb));
        hkArray<hkpRigidBody*, hkContainerHeapAllocator> warpedBodies;
        warpedBodies.~hkArray<hkpRigidBody*, hkContainerHeapAllocator>();
        ::new (&warpedBodies) hkArray<hkpRigidBody*, hkContainerHeapAllocator>;
        WarpBody(pbhkRb, warpedBodies);
        for (hkpRigidBody* pWarpedBody : warpedBodies) {
            warpedChildBodies.pushBack(pWarpedBody);
        }
    }
}

// Warps each character controller among arPhantomA whose broad-phase box is not inside boundAABB, or does not overlap
// it, back into the loaded area.
void LoadedAreaBound::CheckPhantomsOutsideAABB(
    hkAabb& boundAABB,
    const hkArray<hkpPhantom*, hkContainerHeapAllocator>& arPhantomA,
    hkpWorld* apWorld
) {
    hkAabb aabb;
    for (int32_t i = 0; i < arPhantomA.getSize(); ++i) {
        hkpCollidable* pCol = &arPhantomA[i]->m_collidable;
        if (pCol->m_shape) {
            hkpBroadPhase* pBroadPhase = apWorld->getBroadPhase();
            pBroadPhase->getAabb(&pCol->m_broadPhaseHandle, aabb);
            hkBool bContains = boundAABB.contains(aabb);
            if (!bContains || !boundAABB.overlaps(aabb)) {
                CFilter kFilter{pCol->getCollisionFilterInfo()};
                if (kFilter.GetLayer() == L_CHARCONTROLLER) {
                    TESObjectREFR* pRef = TESHavokUtilities::FindCollidableRef(pCol);
                    if (pRef && pRef->IsActor()) {
                        WarpActor(static_cast<Actor*>(pRef));
                    }
                }
            }
        }
    }
}

// Warps apBody back into the loaded area, to FindWarpLocation's point. A body with a constraint other than a contact
// moves with every body constrained to it, raised by the height of their joint box (each of those bodies but apBody
// goes on arWarpedBodies); any other body is raised by its own box's height. Both get a small upward velocity.
void LoadedAreaBound::WarpBody(bhkRigidBody* apBody, hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arWarpedBodies) {
    if (!apBody || static_cast<uint8_t>(apBody->IsFlagSet(bhkWorldObject::BATCH_REMOVE))) {
        return;
    }
    TESObjectREFR* pRef = TESHavokUtilities::FindCollidableRef(apBody->GetCollidable());
    NiPoint3 warpLoc;
    hkTransform hktransf;
    apBody->GetTransform(hktransf);
    NiTransform nitransf;
    HK2NI(nitransf, hktransf);
    FindWarpLocation(pRef, nitransf.m_Translate, warpLoc);
    bool bConstrained = false;
    NiPoint3 warpDiff = warpLoc - nitransf.m_Translate;
    hkpRigidBody* prb = apBody->Gethk();
    for (int32_t i = 0; i < prb->getNumConstraints() && !bConstrained; ++i) {
        hkpConstraintData* pData = prb->getConstraint(i)->m_data;
        bConstrained = pData->getType() != hkpConstraintData::CONSTRAINT_TYPE_CONTACT;
    }
    if (bConstrained) {
        hkArray<const hkpRigidBody*, hkContainerHeapAllocator> pbodyA;
        pbodyA.~hkArray<const hkpRigidBody*, hkContainerHeapAllocator>();
        ::new (&pbodyA) hkArray<const hkpRigidBody*, hkContainerHeapAllocator>;
        bhkUtilFunctions::GatherConstrainedBodies(prb, pbodyA);
        hkAabb constrainedSystemAabb;
        constrainedSystemAabb.setEmpty();
        const hkpWorld* pWorld = apBody->GethkWorld();
        for (const hkpRigidBody* phkBody : pbodyA) {
            hkAabb aabb;
            const hkpBroadPhase* pBroadPhase = pWorld->getBroadPhase();
            pBroadPhase->getAabb(&phkBody->getCollidable()->m_broadPhaseHandle, aabb);
            constrainedSystemAabb.includePoint(aabb.m_max);
            constrainedSystemAabb.includePoint(aabb.m_min);
        }
        float& rfDiffZ = warpDiff[2];
        const float& rfMaxZ = constrainedSystemAabb.m_max(2);
        rfDiffZ = rfMaxZ - constrainedSystemAabb.m_min(2) + rfDiffZ;
        for (const hkpRigidBody*& rphkBody : pbodyA) {
            // Takes apBody's transform for every body of the system.
            hkTransform hkchildTransf;
            apBody->GetTransform(hkchildTransf);
            NiTransform nichildTransf;
            HK2NI(nichildTransf, hkchildTransf);
            NiPoint3 childWarpLoc = nichildTransf.m_Translate + warpDiff;
            bhkRigidBody* pBody = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(rphkBody));
            if (apBody != pBody) {
                // Passes the element's own address on.
                arWarpedBodies.pushBack(const_cast<hkpRigidBody* const&>(rphkBody));
            }
            pBody->SetPosition(childWarpLoc);
            hkVector4 upVel(0.0f, 0.0f, 0.01f, 0.0f);
            hkVector4 zero(0.0f, 0.0f, 0.0f, 0.0f);
            pBody->SetLinearVelocity(upVel);
            pBody->SetAngularVelocity(zero);
        }
        if (pRef && pRef->IsActor()) {
            Actor* pAct = static_cast<Actor*>(pRef);
            if (pAct->pPenetrationDetection) {
                uint32_t uiFilterResult;
                uint32_t uiFilter = *pAct->GetCollisionFilter(&uiFilterResult);
                NiNode* p3D = pRef->Get3D();
                pAct->pPenetrationDetection->Initialize(p3D, uiFilter);
            }
        }
    }
    if (!bConstrained) {
        hkAabb bodyAabb;
        apBody->GetAabbLocal(bodyAabb);
        float fMaxZ = HK2NI(bodyAabb.m_max(2));
        float fMinZ = HK2NI(bodyAabb.m_min(2));
        float fHeight = std::fabs(fMinZ) + fMaxZ;
        float& rfZ = warpLoc[2];
        rfZ = rfZ + fHeight;
        apBody->SetPosition(warpLoc);
        hkVector4 upVel(0.0f, 0.0f, 0.01f, 0.0f);
        apBody->SetLinearVelocity(upVel);
    }
}

NiTObjectArray<NiPointer<bhkRigidBody>>& CellMopp::GetRigidBodyA() {
    return spRigidBodyA;
}
