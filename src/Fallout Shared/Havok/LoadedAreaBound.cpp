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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
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

hkAabb* hkAabb_operator_assign(hkAabb* apThis, const hkAabb& arThat) {
	apThis->m_min.operator=(arThat.m_min);
	apThis->m_max.operator=(arThat.m_max);
	return apThis;
}

LoadedAreaBound::LoadedAreaBound(float a1, float a2, float a3, float a4)
	: pCurrentCell(nullptr), HitMap(37u), fBoundOffset(a1), fBoundDepth(a2), fSideWallHeightOffset(a3),
	  fVelocityDampen(a4) {}

// The phantoms go before the hit map is emptied.
LoadedAreaBound::~LoadedAreaBound() {
	for (int32_t i = 0; i < 6; ++i) {
		spPhantomA[i] = nullptr;
	}
	HitMap.RemoveAll();
}

void LoadedAreaBound::Clear() {
	pCurrentCell = nullptr;

	for (int32_t i = 0; i < 6; ++i) {
		spPhantomA[i] = nullptr;
	}

	HitMap.RemoveAll();
}

// The world comes from the cell, the dimension and the cells from apCellGrid.
bool LoadedAreaBound::GetBoundFromCell(TESObjectCELL* apCell, GridCellArray* apCellGrid, hkAabb& arBound) {
	bool bResult = false;
	bhkWorld* pbhkWorld = apCell->GetbhkWorld();
	ahkpWorld* pWorld = pbhkWorld ? pbhkWorld->operator*() : nullptr;
	if (pWorld) {
		// The local's declaration runs the empty hkAabb default constructor, which stores nothing; a further
		// hkAabb::Construct would construct the box twice.
		alignas(16) hkAabb kAabb;
		alignas(16) hkVector4 kMin;
		kMin.~hkVector4();
		::new (&kMin) hkVector4;
		alignas(16) hkVector4 kMax;
		kMax.~hkVector4();
		::new (&kMax) hkVector4;
		kMax.setAll(-3.40282e+38f);
		kMin.setAll(3.40282e+38f);
		if (apCell->GetInterior()) {
			const hkArray<hkpEntity*>& rEntities = pWorld->getFixedIsland()->getEntities();
			for (int32_t i = 0; i < rEntities.getSize(); ++i) {
				const hkpCollidable* pCollidable = rEntities[i]->getCollidable();
				if (pCollidable->hkpCdBody::getShape()) {
					hkpBroadPhase* pBroadPhase = pWorld->getBroadPhase();
					pBroadPhase->getAabb(const_cast<hkpCollidable*>(pCollidable)->getBroadPhaseHandle(), kAabb);
					kMax.setMax4(kMax, kAabb.m_max);
					kMin.setMin4(kMin, kAabb.m_min);
				}
			}
		} else if (apCellGrid) {
			uint32_t uiDimension = apCellGrid->GetDimension();
			for (uint32_t x = 0; x < uiDimension; ++x) {
				for (uint32_t y = 0; y < uiDimension; ++y) {
					GridCell* pGridCell = apCellGrid->Get(x, y);
					if (!pGridCell->pCell) {
						continue;
					}
					NiTObjectArray<NiPointer<bhkRigidBody>>& rBodies = pGridCell->pCell->GetCellMopp()->GetRigidBodyA();
					for (uint32_t j = 0; j < rBodies.GetSize(); ++j) {
						const hkpCollidable* pCollidable = rBodies[j].operator->()->GetCollidable();
						if (!pCollidable->hkpCdBody::getShape()) {
							continue;
						}
						bhkRigidBody* pBody = rBodies[j].operator->();
						if (pBody->GethkWorld()) {
							hkpBroadPhase* pBroadPhase = pWorld->getBroadPhase();
							pBroadPhase->getAabb(const_cast<hkpCollidable*>(pCollidable)->getBroadPhaseHandle(), kAabb);
						} else {
							alignas(16) hkTransform kTransform;
							kTransform.~hkTransform();
							::new (&kTransform) hkTransform;
							rBodies[j].operator->()->GetTransform(kTransform);
							hkpShape* pShape = rBodies[j].operator->()->GetShape()->Gethk();
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
	NiPoint3 ecx0;
	const NiPoint3* v2 = &apActor->GetLocationOnReference();
	FindWarpLocation(apActor, *v2, ecx0);
	apActor->SetPosition(ecx0);
	if (apActor->GetCharController()) {
		bhkCharacterController* CharController = apActor->GetCharController();
		CharController->SetStuckQuadruped(0);
	}
}

// Finds where to warp apRef back into the loaded area: the navmesh height under arCurLoc; else, for an actor, its
// editor location when that lies in the current cell; else, for any other reference, its editor location when that lies
// in an attached cell. A location found must lie in the box spanned by the bound's phantoms 2 and 5; without one the
// current cell's COC placement is taken.
void LoadedAreaBound::FindWarpLocation(TESObjectREFR* apRef, const NiPoint3& arCurLoc, NiPoint3& arWarpLoc) {
	TESForm* plocForm = nullptr;
	TESObjectCELL* pcell = pCurrentCell;
	Actor* pact;
	if (apRef && apRef->IsActor()) {
		pact = static_cast<Actor*>(apRef);
	} else {
		pact = nullptr;
	}
	NiPoint3 rot;
	bool blocFound = false;
	NiPoint3 tmpLoc = arCurLoc;
	float fheightNavMesh;
	if (apRef && Pathing::GetNavMeshZForLocation(tmpLoc, pcell, pcell->GetWorldSpace(), fheightNavMesh)) {
		tmpLoc.z = fheightNavMesh;
		arWarpLoc = tmpLoc;
		blocFound = true;
	} else if (pact && pact->GetEditorLocationInteriorCell() == pCurrentCell) {
		arWarpLoc = pact->GetEditorLocationCoord();
		blocFound = true;
	} else if (!pact && apRef && apRef->GetEditorLocation(tmpLoc, rot, plocForm, 0)) {
		TESObjectCELL* pcellLoc = TES::pInstance->GetCellForPoint(tmpLoc);
		if (pcellLoc && pcellLoc->GetState() == TESObjectCELL::CS_ATTACHED) {
			arWarpLoc = tmpLoc;
			blocFound = true;
		}
	}
	if (blocFound) {
		hkVector4 warpLoc;
		NI2HK(warpLoc, arWarpLoc);
		hkAabb boundAabb;
		boundAabb.setEmpty();
		boundAabb.includePoint(spPhantomA[2]->Gethk()->m_aabb.m_max);
		boundAabb.includePoint(spPhantomA[5]->Gethk()->m_aabb.m_min);
		blocFound = boundAabb.containsPoint(warpLoc) != 0;
	}
	if (!blocFound) {
		pcell->GetCOCPlacementInfo(arWarpLoc, rot);
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
	uint16_t uscurrentFlag = 1;
	using curhitMap_t = NiTMap<bhkRigidBody*, uint16_t>;
	curhitMap_t curhitMap(37u);
	for (uint32_t ui = 0; ui < 6; ++ui, uscurrentFlag <<= 1) {
		bool buseWarp = pCurrentCell->GetInterior() || ui == 0;
		hkpAabbPhantom* pphantom = spPhantomA[ui]->Gethk();
		// The overlap array's getter is a trivial accessor.
		hkArray<hkpCollidable*, hkContainerHeapAllocator>* rcols = &pphantom->m_overlappingCollidables;
		if (rcols->isEmpty()) {
			continue;
		}
		int32_t isize = rcols->getSize();
		using PhantomArray = hkArray<hkpPhantom*, hkContainerHeapAllocator>;
		using RigidBodyArray = hkArray<hkpRigidBody*, hkContainerHeapAllocator>;
		PhantomArray phantomA;
		RigidBodyArray bodyA;
		bodyA.reserve(rcols->getSize());
		for (int32_t i = 0; i < isize; ++i) {
			alignas(16) hkpRigidBody* prb = hkpGetRigidBody(rcols->operator[]<hkpCollidable*>(i));
			if (prb) {
				bodyA.pushBack(prb);
			} else {
				hkpPhantom* pp = hkpGetPhantom(rcols->operator[]<hkpCollidable*>(i));
				if (pp) {
					phantomA.pushBack(pp);
				}
			}
		}
		for (int32_t i = 0; i < phantomA.getSize(); ++i) {
			if (!buseWarp) {
				continue;
			}
			CFilter filter;
			filter.iFilter = phantomA.operator[]<hkpPhantom*>(i)->getCollidable()->getCollisionFilterInfo();
			TESObjectREFR* pref =
				TESHavokUtilities::FindCollidableRef(phantomA.operator[]<hkpPhantom*>(i)->getCollidable());
			if (filter.GetLayer() == L_CHARCONTROLLER && pref && pref->IsActor()) {
				Actor* pactor = static_cast<Actor*>(pref);
				WarpActor(pactor);
			}
		}
		alignas(16) RigidBodyArray warpedChildBodies;
		for (int32_t i = 0; i < bodyA.getSize(); ++i) {
			hkpRigidBody* prb = bodyA.operator[]<hkpRigidBody*>(i);
			if (warpedChildBodies.indexOf(prb, 0, -1) != -1) {
				continue;
			}
			// Looks the body's reference up before its null test and never uses it, as the game does.
			TESHavokUtilities::FindCollidableRef(prb->getCollidable());
			if (!prb || prb->getMotionType() == hkpMotion::MOTION_KEYFRAMED ||
				prb->getMotionType() == hkpMotion::MOTION_FIXED ||
				prb->getMotionType() == hkpMotion::MOTION_CHARACTER) {
				continue;
			}
			bhkRigidBody* pbhkRb = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(prb));
			if (buseWarp) {
				RigidBodyArray warpedBodies;
				WarpBody(pbhkRb, warpedBodies);
				for (int32_t j = 0; j < warpedBodies.getSize(); ++j) {
					warpedChildBodies.pushBack(warpedBodies.operator[]<hkpRigidBody*>(j));
				}
				continue;
			}
			uint16_t usborderFlags;
			bool bcontains = HitMap.GetAt(pbhkRb, usborderFlags);
			if (!bcontains || (usborderFlags & uscurrentFlag) == 0) {
				hkVector4 curVel(pbhkRb->GetLinearVelocity());
				// -1.0 is a double constant; the product is stored as a double, then as a float.
				float fdampMult = (ui == 1) ? fVelocityDampen : fVelocityDampen * -1.0;
				hkSimdReal kDampMult(fdampMult);
				curVel.mul4(kDampMult);
				float& rfZ = curVel(2);
				rfZ = rfZ * -1.0;
				pbhkRb->SetLinearVelocity(curVel);
			}
			uint16_t usbodyCurrFlags = uscurrentFlag;
			if (curhitMap.GetAt(pbhkRb, usbodyCurrFlags)) {
				usbodyCurrFlags |= uscurrentFlag;
			}
			curhitMap.SetAt(pbhkRb, usbodyCurrFlags);
		}
	}
	bhkRigidBody* pbody;
	alignas(16) uint16_t usborderFlags;
	NiTMapItem<bhkRigidBody*, uint16_t>* it = curhitMap.GetFirstPos();
	while (it) {
		curhitMap.GetNext(it, pbody, usborderFlags);
		HitMap.SetAt(pbody, usborderFlags);
	}
	using removeBodyA_t = BSSimpleArray<bhkRigidBody*, 1024>;
	removeBodyA_t removeBodyA;
	it = HitMap.GetFirstPos();
	while (it) {
		HitMap.GetNext(it, pbody, usborderFlags);
		if (!curhitMap.GetAt(pbody, usborderFlags)) {
			removeBodyA.Add(pbody);
		}
	}
	for (uint32_t k = 0; k < removeBodyA.iSize; ++k) {
		HitMap.RemoveAt(removeBodyA.GetAt(static_cast<int32_t>(k)));
	}
}

// It realigns its frame to 16: the filter, the creation info, the box and both extents sit at 16-aligned slots. Each
// corner component goes through hkVector4::operator() and is rounded to a float before hkVector4::set takes it.
void LoadedAreaBound::Create(TESObjectCELL* apCell, const hkAabb& arboundAABB) {
	alignas(16) CFilter kFilter;
	bhkWorld* pworld = apCell->GetbhkWorld();
	for (uint32_t ui = 0; ui < 6; ++ui) {
		float fheightOffset;
		alignas(16) hkVector4 minExtent;
		alignas(16) hkVector4 maxExtent;
		this->spPhantomA[ui].operator=(static_cast<bhkAabbPhantom*>(0));
		hkAabbPhantomCinfo ci;
		hkAabb bound;
		if (apCell->GetInterior()) {
			fheightOffset = 0.0f;
		} else {
			fheightOffset = this->fSideWallHeightOffset;
		}
		float fHeight = fheightOffset;
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
		bound.m_max.operator=(maxExtent);
		bound.m_min.operator=(minExtent);
		hkAabb_operator_assign(&ci.aabb, bound);
		uint32_t uiSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
		ci.m_collisionFilterInfo = FNVD_RECONSTRUCT(CFilter)(&kFilter, L_UNIDENTIFIED, uiSystemGroup, P_OTHER)->iFilter;
		void* pMemory = NiMemObject::operator new(sizeof(bhkAabbPhantom));
		bhkAabbPhantom* pPhantom = pMemory ? ::new (pMemory) bhkAabbPhantom(ci) : nullptr;
		this->spPhantomA[ui].operator=(pPhantom);
		pworld->Add(this->spPhantomA[ui].m_pObject);
	}
}

void LoadedAreaBound::CheckForDynamicBodiesOutsideBound() {
	hkAabb boundAABB;
	alignas(16) hkVector4 minExtent;
	alignas(16) hkVector4 maxExtent;
	maxExtent.setAll(-3.40282e+38f);
	minExtent.setAll(3.40282e+38f);
	for (uint32_t ui = 0; ui < 6; ++ui) {
		hkAabb phantomAABB;
		this->spPhantomA[ui]->GetAabb(phantomAABB);
		maxExtent.setMax4(maxExtent, phantomAABB.m_max);
		minExtent.setMin4(minExtent, phantomAABB.m_min);
	}
	boundAABB.m_min.operator=(minExtent);
	boundAABB.m_max.operator=(maxExtent);
	bhkWorld* pbhkWorld = this->pCurrentCell->GetbhkWorld();
	hkpWorld* pworld = pbhkWorld ? pbhkWorld->operator*() : nullptr;
	if (pworld) {
		const hkArray<hkpPhantom*, hkContainerHeapAllocator>& rPhantoms = pworld->getPhantoms();
		this->CheckPhantomsOutsideAABB(boundAABB, rPhantoms, pworld);
		this->SetBodiesMotionOutsideAABB(boundAABB, pworld->m_activeSimulationIslands);
		this->SetBodiesMotionOutsideAABB(boundAABB, pworld->m_inactiveSimulationIslands);
	}
}

// Collects the moving bodies of arSimIslandA's islands whose broad-phase box is not inside boundAABB, or does not
// overlap it, and warps each that no warp of this pass moved yet, with the bodies constrained to it.
void LoadedAreaBound::SetBodiesMotionOutsideAABB(hkAabb& boundAABB,
	const hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>& arSimIslandA) {
	hkAabb aabb;
	hkpWorld* pworld = nullptr;
	alignas(16) hkArray<hkpRigidBody*, hkContainerHeapAllocator> bodiesToBeWarped;
	bodiesToBeWarped.~hkArray<hkpRigidBody*, hkContainerHeapAllocator>();
	::new (&bodiesToBeWarped) hkArray<hkpRigidBody*, hkContainerHeapAllocator>;
	for (int32_t i = 0; i < arSimIslandA.getSize(); ++i) {
		pworld = arSimIslandA.operator[]<hkpSimulationIsland*>(i)->getWorld();
		const hkArray<hkpEntity*>& entitiesA = arSimIslandA.operator[]<hkpSimulationIsland*>(i)->getEntities();
		for (int32_t j = 0; j < entitiesA.getSize(); ++j) {
			const hkpCollidable* pcol = entitiesA.operator[]<hkpEntity*>(j)->getCollidable();
			if (pcol->hkpCdBody::getShape()) {
				hkpBroadPhase* pBroadPhase = pworld->getBroadPhase();
				pBroadPhase->getAabb(&pcol->m_broadPhaseHandle, aabb);
				hkBool bContains = boundAABB.contains(aabb);
				if (!bContains.operator bool() || !boundAABB.overlaps(aabb)) {
					hkpRigidBody* prb = hkpGetRigidBody(pcol);
					if (prb && prb->getMotionType() != hkpMotion::MOTION_KEYFRAMED &&
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
	for (int32_t i = 0; i < bodiesToBeWarped.getSize(); ++i) {
		if (warpedChildBodies.indexOf(bodiesToBeWarped.operator[]<hkpRigidBody*>(i), 0, -1) != -1) {
			continue;
		}
		bhkRigidBody* pbhkRb =
			static_cast<bhkRigidBody*>(bhkEntity::Getbhk(bodiesToBeWarped.operator[]<hkpRigidBody*>(i)));
		hkArray<hkpRigidBody*, hkContainerHeapAllocator> warpedBodies;
		warpedBodies.~hkArray<hkpRigidBody*, hkContainerHeapAllocator>();
		::new (&warpedBodies) hkArray<hkpRigidBody*, hkContainerHeapAllocator>;
		WarpBody(pbhkRb, warpedBodies);
		for (int32_t j = 0; j < warpedBodies.getSize(); ++j) {
			warpedChildBodies.pushBack(warpedBodies.operator[]<hkpRigidBody*>(j));
		}
	}
}

// Warps each character controller among arPhantomA whose broad-phase box is not inside boundAABB, or does not overlap
// it, back into the loaded area.
void LoadedAreaBound::CheckPhantomsOutsideAABB(hkAabb& boundAABB,
	const hkArray<hkpPhantom*, hkContainerHeapAllocator>& arPhantomA, hkpWorld* apWorld) {
	hkAabb aabb;
	for (int32_t i = 0; i < arPhantomA.getSize(); ++i) {
		hkpCollidable* pcol = &arPhantomA.operator[]<hkpPhantom*>(i)->m_collidable;
		if (pcol->m_shape) {
			hkpBroadPhase* pBroadPhase = apWorld->getBroadPhase();
			pBroadPhase->getAabb(&pcol->m_broadPhaseHandle, aabb);
			hkBool bContains = boundAABB.contains(aabb);
			if (!bContains.operator bool() || !boundAABB.overlaps(aabb)) {
				CFilter kFilter{pcol->getCollisionFilterInfo()};
				if (kFilter.GetLayer() == L_CHARCONTROLLER) {
					TESObjectREFR* pRef = TESHavokUtilities::FindCollidableRef(pcol);
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
	TESObjectREFR* pref = TESHavokUtilities::FindCollidableRef(apBody->GetCollidable());
	NiPoint3 warpLoc;
	hkTransform hktransf;
	apBody->GetTransform(hktransf);
	NiTransform nitransf;
	HK2NI(nitransf, hktransf);
	FindWarpLocation(pref, nitransf.m_Translate, warpLoc);
	bool bconstrained = false;
	NiPoint3 warpDiff = warpLoc - nitransf.m_Translate;
	hkpRigidBody* prb = apBody->Gethk();
	for (int32_t i = 0; i < prb->getNumConstraints() && !bconstrained; ++i) {
		hkpConstraintData* pdata = prb->getConstraint(i)->m_data;
		bconstrained = pdata->getType() != hkpConstraintData::CONSTRAINT_TYPE_CONTACT;
	}
	if (bconstrained) {
		hkArray<const hkpRigidBody*, hkContainerHeapAllocator> pbodyA;
		pbodyA.~hkArray<const hkpRigidBody*, hkContainerHeapAllocator>();
		::new (&pbodyA) hkArray<const hkpRigidBody*, hkContainerHeapAllocator>;
		bhkUtilFunctions::GatherConstrainedBodies(prb, pbodyA);
		hkAabb constrainedSystemAabb;
		constrainedSystemAabb.setEmpty();
		const hkpWorld* pworld = apBody->GethkWorld();
		for (int32_t i = 0; i < pbodyA.getSize(); ++i) {
			hkAabb aabb;
			const hkpBroadPhase* pBroadPhase = pworld->getBroadPhase();
			pBroadPhase->getAabb(&pbodyA.operator[]<const hkpRigidBody*>(i)->getCollidable()->m_broadPhaseHandle, aabb);
			constrainedSystemAabb.includePoint(aabb.m_max);
			constrainedSystemAabb.includePoint(aabb.m_min);
		}
		float& rfDiffZ = warpDiff[2];
		const float& rfMaxZ = constrainedSystemAabb.m_max.operator()(2);
		rfDiffZ = rfMaxZ - constrainedSystemAabb.m_min.operator()(2) + rfDiffZ;
		for (int32_t j = 0; j < pbodyA.getSize(); ++j) {
			// Takes apBody's transform for every body of the system, as the game does.
			hkTransform hkchildTransf;
			apBody->GetTransform(hkchildTransf);
			NiTransform nichildTransf;
			HK2NI(nichildTransf, hkchildTransf);
			NiPoint3 childWarpLoc = nichildTransf.m_Translate + warpDiff;
			bhkRigidBody* pbody =
				static_cast<bhkRigidBody*>(bhkEntity::Getbhk(pbodyA.operator[]<const hkpRigidBody*>(j)));
			if (apBody != pbody) {
				// Passes the element's own address on.
				arWarpedBodies.pushBack(const_cast<hkpRigidBody* const&>(pbodyA.operator[]<const hkpRigidBody*>(j)));
			}
			pbody->SetPosition(childWarpLoc);
			hkVector4 upVel(0.0f, 0.0f, 0.01f, 0.0f);
			hkVector4 zero(0.0f, 0.0f, 0.0f, 0.0f);
			pbody->SetLinearVelocity(upVel);
			pbody->SetAngularVelocity(zero);
		}
		if (pref && pref->IsActor()) {
			Actor* pact = static_cast<Actor*>(pref);
			if (pact->pPenetrationDetection) {
				uint32_t uiFilterResult;
				uint32_t uiFilter = *pact->GetCollisionFilter(&uiFilterResult);
				NiNode* p3D = pref->Get3D();
				pact->pPenetrationDetection->Initialize(p3D, uiFilter);
			}
		}
	}
	if (!bconstrained) {
		hkAabb bodyAabb;
		apBody->GetAabbLocal(bodyAabb);
		float fmaxZ = HK2NI(bodyAabb.m_max.operator()(2));
		float fminZ = HK2NI(bodyAabb.m_min.operator()(2));
		float fheight = std::fabs(fminZ) + fmaxZ;
		float& rfZ = warpLoc[2];
		rfZ = rfZ + fheight;
		apBody->SetPosition(warpLoc);
		hkVector4 upVel(0.0f, 0.0f, 0.01f, 0.0f);
		apBody->SetLinearVelocity(upVel);
	}
}

void bhkAabbPhantom::GetAabb(hkAabb& arB) {
	hkpAabbPhantom* pPhantom = operator*();
	if (pPhantom) {
		MarkRead();
		hkAabb_operator_assign(&arB, pPhantom->m_aabb);
		UnMarkRead();
	} else {
		const hkVector4& rMax = hkVector4::getZero();
		const hkVector4& rMin = hkVector4::getZero();
		hkAabb kZeroAabb(rMin, rMax);
		hkAabb_operator_assign(&arB, kZeroAabb);
	}
}

hkpAabbPhantom* bhkAabbPhantom::Gethk() {
	return static_cast<hkpAabbPhantom*>(phkObject);
}

void bhkWorldObject::SetFlags(uint32_t aiFlags, bool abTF) {
	if (abTF) {
		iFlags |= aiFlags;
	} else {
		ClearFlags(aiFlags);
	}
}

NiTObjectArray<NiPointer<bhkRigidBody>>& CellMopp::GetRigidBodyA() {
	return spRigidBodyA;
}
