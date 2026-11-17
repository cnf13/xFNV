#include "Fallout/AI/bhkCharacterListenerArrow.h"

#include "BSCore/RTTI.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkEntity.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/ConstraintSolver/Simplex/hkpSimplexSolver.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "SDK/MSVC/rtti.h"

bhkCharacterListenerArrow::bhkCharacterListenerArrow(bhkCharacterControllerCinfo& arInfo, ArrowProjectile* apProjectile)
  : bhkCharacterController(arInfo), pProj(apProjectile) {}

// The arrow has nothing of its own to release.
bhkCharacterListenerArrow::~bhkCharacterListenerArrow() {}

void bhkCharacterListenerArrow::processConstraintsCallback(
    const hkArray<hkpRootCdPoint, hkContainerHeapAllocator>& arManifold,
    hkpSimplexSolverInput& arInput
) {
    int32_t iNumConstraints = arInput.m_numConstraints;
    for (int32_t i = 0; i < iNumConstraints; ++i) {
        hkpSurfaceConstraintInfo* rsurface = &arInput.m_constraints[i];
        const hkpRootCdPoint* rpnt = &arManifold.operator[]<hkpRootCdPoint>(i);
        hkpRigidBody* pbody = hkpGetRigidBody(rpnt->m_rootCollidableB);
        alignas(16) CFilter ifilter;
        ifilter.iFilter = rpnt->m_rootCollidableB->getCollisionFilterInfo();
        COL_LAYER elayer = ifilter.GetLayer();
        if (pbody) {
            TESObjectREFR* pref = TESHavokUtilities::FindCollidableRef_009C69E0(rpnt->m_rootCollidableB);
            if (pref && pref == pPlayer->GetGrabObject()) {
                rsurface->m_velocity = GethkLinearVelocity();
                hkVector4 kPlane(0.0f, 0.0f, 0.0f, 1.0f);
                rsurface->m_plane = kPlane;
                return;
            }
        }
        else {
            if (elayer != L_CHARCONTROLLER) {
                rsurface->m_velocity = GethkLinearVelocity();
                hkVector4 kPlane(0.0f, 0.0f, 0.0f, 1.0f);
                rsurface->m_plane = kPlane;
            }
        }
    }
}

void bhkCharacterListenerArrow::contactPointAddedCallback(const hkpRootCdPoint& point) {
    TESForm* pprojectile = pProj;
    if (pprojectile) {
        if (!static_cast<TESObjectCONT*>(pprojectile)->TESObjectCONT::GetOpenSound() && !pprojectile->GetDelete()) {
            bhkCharacterListenerArrow::ProcessCollision(
                static_cast<ArrowProjectile*>(pprojectile), point.m_rootCollidableB, (&point.m_contact)->m_position,
                point.m_contact.getNormal()
            );
        }
    }
}

void bhkCharacterListenerArrow::ProcessCollision(
    ArrowProjectile* apProjectile,
    const hkpCollidable* apCollidee,
    const hkVector4& aContactPoint,
    const hkVector4& aContactNormal
) {
    BaseProcess* v8;
    BaseProcess* CurrentAIProcess;
    TESObjectREFR* ptarget = TESHavokUtilities::FindCollidableRef_009C69E0(apCollidee);
    Actor* pactor = dynamic_cast<Actor*>(ptarget);
    hkpPhantom* pph = hkpGetPhantom(apCollidee);
    if (!pph ||
        (pactor &&
         (!pactor->IsDead(0) &&
          ((!pactor->pCurrentProcess ||
            ((CurrentAIProcess = pactor->pCurrentProcess, CurrentAIProcess->GetKnockState() == KNOCK_STATE_NORMAL) ||
             (v8 = pactor->pCurrentProcess, v8->GetKnockState() == KNOCK_STATE_GET_UP))) &&
           (pactor->GetSitSleepState() != SIT_SLEEP_IS_SITTING &&
            pactor->GetSitSleepState() != SIT_SLEEP_IS_SLEEPING))))) {
        CFilter colfilter;
        uint32_t CollisionFilterInfo = apCollidee->getCollisionFilterInfo();
        colfilter.iFilter = CollisionFilterInfo;
        uint32_t Group = colfilter.GetGroup();
        if (Group != ArrowProjectile::GetArrowGroup()) {
            TESObjectREFR* GrabObject;
            if (!ptarget || (GrabObject = pPlayer->GetGrabObject(), ptarget != GrabObject)) {
                NiPoint3 normal;
                NiPoint3 contact;
                HK2NI(contact, aContactPoint);
                HK2NINS(normal, aContactNormal);
                if (colfilter.GetGroup() == 1) {
                    apProjectile->CollideTerrain(contact, normal);
                }
                else {
                    if (ptarget) {
                        if (!pactor || pactor->GetGhost()) {
                            if (!pactor) {
                                hkpRigidBody* RigidBody = hkpGetRigidBody(apCollidee);
                                bhkEntity* pbody = bhkEntity::Getbhk(RigidBody);
                            }
                        }
                        else {
                            apProjectile->CollideActor(contact, normal, pactor);
                        }
                    }
                }
            }
        }
    }
}
