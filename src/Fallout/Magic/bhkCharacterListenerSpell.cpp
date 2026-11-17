#include "Fallout/Magic/bhkCharacterListenerSpell.h"

#include "BSCore/RTTI.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/ConstraintSolver/Simplex/hkpSimplexSolver.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpSurfaceConstraintInfo.h"
#include "SDK/MSVC/rtti.h"

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bDrawSpellContact("bDrawSpellContact:General", false);

bhkCharacterListenerSpell::bhkCharacterListenerSpell(bhkCharacterControllerCinfo& arInfo, MagicProjectile* apProjectile)
  : bhkCharacterController(arInfo), pProj(apProjectile) {}

void bhkCharacterListenerSpell::processConstraintsCallback(
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
            TESObjectREFR* ptarget = GetObjectREFR(rpnt->m_rootCollidableB);
            if (ptarget && ptarget == pPlayer->GetGrabObject()) {
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

void bhkCharacterListenerSpell::contactPointAddedCallback(const hkpRootCdPoint& point) {
    MagicProjectile* pprojectile = dynamic_cast<MagicProjectile*>(GetObjectREFR(point.m_rootCollidableA));
    if (!pprojectile) {
        pprojectile = pProj;
    }
    if (!pprojectile->IsActive()) {
        return;
    }
    TESObjectREFR* ptarget = GetObjectREFR(point.m_rootCollidableB);
    Actor* pactor = (ptarget && ptarget->IsActor()) ? static_cast<Actor*>(ptarget) : nullptr;
    hkpPhantom* pph = hkpGetPhantom(point.m_rootCollidableB);
    if (pph) {
        if (!pactor || pactor->IsDead(0)) {
            return;
        }
        if (pactor->pCurrentProcess) {
            BaseProcess* pProcess = pactor->pCurrentProcess;
            if (pProcess->GetKnockState() != KNOCK_STATE_NORMAL) {
                pProcess = pactor->pCurrentProcess;
                if (pProcess->GetKnockState() != KNOCK_STATE_GET_UP) {
                    return;
                }
            }
        }
        if (pactor->GetSitSleepState() == SIT_SLEEP_IS_SITTING || pactor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
            return;
        }
    }
    if (ptarget && ptarget == pPlayer->GetGrabObject()) {
        return;
    }
    if (pactor && pactor->GetGhost()) {
        return;
    }
    if (bDrawSpellContact.Bool()) {
        hkpRootCdPoint kPoint(point);
        bhkShape* pShapeA = bhkShape::Getbhk(kPoint.m_rootCollidableA->m_shape);
        if (pShapeA) {
            NiNode* pMemory = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
            NiNode* pNode = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
            alignas(16) NiColor kColor(1.0f, 1.0f, 1.0f);
            pShapeA->BuildDisplayGeometry(pNode, kColor);
            NiMatrix3 kRot;
            HK2NI(kRot, kPoint.m_rootCollidableA->m_motion.pState->m_transform.m_rotation);
            pNode->SetRotate(kRot);
            alignas(16) NiPoint3 kPos;
            HK2NI(kPos, kPoint.m_rootCollidableA->m_motion.pState->m_transform.m_translation);
            pNode->SetTranslate(kPos);
        }
        bhkShape* pShapeB = bhkShape::Getbhk(kPoint.m_rootCollidableB->m_shape);
        if (pShapeB) {
            NiNode* pMemory = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
            NiNode* pNode = pMemory ? ::new (pMemory) NiNode(0) : nullptr;
            alignas(16) NiColor kColor(1.0f, 1.0f, 1.0f);
            pShapeB->BuildDisplayGeometry(pNode, kColor);
            NiMatrix3 kRot;
            HK2NI(kRot, kPoint.m_rootCollidableB->m_motion.pState->m_transform.m_rotation);
            pNode->SetRotate(kRot);
            alignas(16) NiPoint3 kPos;
            HK2NI(kPos, kPoint.m_rootCollidableB->m_motion.pState->m_transform.m_translation);
            pNode->SetTranslate(kPos);
        }
    }
    NiPoint3 kContact;
    HK2NI(kContact, point.m_contact.m_position);
    if (bDrawSpellContact.Bool()) {
        NiColorA kColor(1.0f, 0.0f, 0.0f, 1.0f);
        NiTriShape* pTriPoint = MakeTriPoint(5.0f, kColor, true);
        pTriPoint->SetTranslate(kContact);
        TES::pInstance->AddTempDebugObject(pTriPoint, 50.0f);
    }
    MagicCaster* pmagicCaster = pprojectile->GetCaster();
    // Computes the target as an actor and the caster's actor and deliberately does not use them, as the game does.
    [[maybe_unused]] bool bTargetIsActor = false;
    [[maybe_unused]] Actor* pactortarg = nullptr;
    if (ptarget && ptarget->IsActor()) {
        bTargetIsActor = true;
        pactortarg = static_cast<Actor*>(ptarget);
    }
    [[maybe_unused]] Actor* pcaster = pmagicCaster ? pmagicCaster->GetCasterAsActor() : nullptr;
    hkpRigidBody* pRigidBody = hkpGetRigidBody(point.m_rootCollidableB);
    bhkRigidBody* pbody = pRigidBody ? static_cast<bhkRigidBody*>(bhkEntity::Getbhk(pRigidBody)) : nullptr;
    pprojectile->Collide(kContact, pbody, ptarget, false);
}

void bhkCharacterListenerSpell::contactPointRemovedCallback(const hkpRootCdPoint& point) {}

TESObjectREFR* bhkCharacterListenerSpell::GetObjectREFR(const hkpCollidable* apCollidable) {
    TESObjectREFR* pRef = nullptr;
    NiAVObject* pObject = bhkUtilFunctions::GetNiAVObject(apCollidable);
    if (pObject) {
        pRef = TESObjectREFR::FindReferenceFor3D(pObject);
    }
    return pRef;
}

MagicCaster* MagicProjectile::GetCaster() {
    return pCaster;
}
