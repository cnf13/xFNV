#include "BSHavok/WeaponObject.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkAction.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkBlendController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkLimitedHingeConstraint.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSUtilities.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <cstring>

NiRTTI WeaponObject::ms_RTTI("WeaponObject", &bhkBlendCollisionObject::ms_RTTI);

void GetTransformRecursive(NiAVObject* apObject, NiAVObject* apRoot, NiTransform& aWorldTransform) {
    NiTransform kTransform;
    if (apObject) {
        if (apObject == apRoot) {
            aWorldTransform.MakeIdentity();
        }
        else {
            GetTransformRecursive(apObject->m_pkParent, apRoot, aWorldTransform);
            aWorldTransform = (kTransform = aWorldTransform * apObject->m_kLocal);
        }
    }
    else {
        // The identity, as at apRoot (a departure): a parent chain that ends before apRoot reaches here, and the exe
        // leaves aWorldTransform unwritten for the levels below to multiply.
        aWorldTransform.MakeIdentity();
    }
}

WeaponObject::WeaponObject() {}

WeaponObject::WeaponObject(
    CFilter akFilter,
    NiAVObject* apOwner3d,
    NiAVObject* apObject,
    bhkCollisionObject* apReplaceObject,
    bool abDefaultKeyframed
) {
    Init(akFilter, apOwner3d, apObject, apReplaceObject, abDefaultKeyframed);
}

WeaponObject::~WeaponObject() {
    Kill();
}

const NiRTTI* WeaponObject::GetRTTI() const {
    return &WeaponObject::ms_RTTI;
}

// The clone is allocated in the Havok heap context.
NiObject* WeaponObject::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    WeaponObject* pkClone = new WeaponObject;
    CopyMembers(pkClone, arCloneData);
    return pkClone;
}

void WeaponObject::UpdateWorldData(NiUpdateData& arUpdateData) {
    NiAVObject* m_pkSceneObject;
    m_pkSceneObject = this->m_pkSceneObject;
    if (m_pkSceneObject &&
        (m_pkSceneObject->m_kLocal.m_Translate != NiPoint3::ZERO ||
         (!(m_pkSceneObject->m_kLocal.m_Rotate == NiMatrix3::IDENTITY)))) {
        std::memcpy(&m_pkSceneObject->m_kLocal, &NiMatrix3::IDENTITY, 0x24u);
        m_pkSceneObject->m_kLocal.m_Translate = NiPoint3::ZERO;
    }

    bhkBlendCollisionObject::UpdateWorldData(arUpdateData);
}

bool WeaponObject::QIncludedInSkeleton() const {
    return false;
}

// The gathered bodies are a Havok array that is set up and freed by hand.
void WeaponObject::UpdateChildRB3D() {
    NiAVObject* pkSceneObject = m_pkSceneObject;
    if (!pkSceneObject) {
        return;
    }
    bhkNiCollisionObject* pkNextCollisionObject = bhkUtilFunctions::FindNextCollisionObject(pkSceneObject);
    bhkRigidBody* pkWorldObject = nullptr;
    if (pkNextCollisionObject) {
        pkWorldObject = pkNextCollisionObject->spWorldObject;
    }
    hkArray<hkpRigidBody*> kBodyA;
    bhkUtilFunctions::GatherRigidBodies(pkSceneObject, kBodyA);
    for (hkpWorldObject* pkBody : kBodyA) {
        if (!pkWorldObject) {
            break;
        }
        if (pkBody == pkWorldObject->phkObject) {
            continue;
        }
        if (pkBody->m_world) {
            continue;
        }
        // The property is keyed by the collision object's RTTI address.
        hkpPropertyValue kProperty;
        bhkWorldObject::GetProperty(&kProperty, pkBody, bhkPropertyKey(bhkNiCollisionObject::ms_RTTI));
        // The property value is an untyped integer holding the body's collision object.
        bhkNiCollisionObject* pkBodyObject = reinterpret_cast<bhkNiCollisionObject*>(
            static_cast<uintptr_t>(kProperty.m_data)
        );
        NiAVObject* pkBodySceneObject = pkBodyObject->m_pkSceneObject;
        pkBodySceneObject->m_uFlags = static_cast<NiAVObject::NiFlags>(
            pkBodySceneObject->m_uFlags | NiAVObject::APP_CULLED
        );
    }
}

void WeaponObject::Init(
    CFilter aOwnerFilter,
    NiAVObject* apOwner3d,
    NiAVObject* apObject,
    bhkCollisionObject* apReplaceObject,
    bool abDefaultKeyframed
) {
    bhkCollisionObject* v10;
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (apOwner3d && (apObject && (v10 = apReplaceObject) != nullptr)) {
        usFlags |= ACTIVE_POS | RESET_POS;
        bhkBlendCollisionObject* pblendObject = static_cast<bhkBlendCollisionObject*>(
            v10->DynamicCastMember(&bhkBlendCollisionObject::ms_RTTI)
        );
        if (pblendObject) {
            fHeirGain = pblendObject->fHeirGain;
            fVelGain = pblendObject->fVelGain;
            pStoredWorld = pblendObject->pStoredWorld;
            iForceAddCount = pblendObject->iForceAddCount;
            ePrevType = pblendObject->ePrevType;
        }
        else {
            ePrevType = hkpMotion::MOTION_KEYFRAMED;
            fHeirGain = 1.0f;
            fVelGain = 1.0f;
        }
        NiRefObject* spWorldObject = v10->spWorldObject;
        if (spWorldObject) {
            NiIncRefCount(spWorldObject);
        }
        v10->SetWorldObject(nullptr);
        NiAVObject* psceneObject = v10->m_pkSceneObject;
        psceneObject->SetCollisionObject(this);
        SetWorldObject(static_cast<bhkWorldObject*>(spWorldObject));
        UpdateConstraints(true, aOwnerFilter, apOwner3d, abDefaultKeyframed);
        bhkRigidBody* v14 = this->spWorldObject;
        if (v14) {
            // Without a Havok body the motion type is 5 (MOTION_FIXED), and its address is passed.
            hkpMotion::MotionType eMotionType;
            hkpRigidBody* phkObject = static_cast<hkpRigidBody*>(v14->phkObject);
            if (phkObject) {
                eMotionType = static_cast<hkpMotion::MotionType>(phkObject->m_motion.m_type.m_storage);
            }
            else {
                eMotionType = hkpMotion::MOTION_FIXED;
            }
            if (bhkRigidBody::IsMotionTypeDynamic(eMotionType)) {
                ePrevType = hkpMotion::MOTION_DYNAMIC;
                fHeirGain = 0.0f;
                fVelGain = 0.0f;
            }
        }
        NiTimeController* pcon = psceneObject->GetController(&bhkBlendController::ms_RTTI);
        if (!pcon) {
            pcon = new bhkBlendController;
        }
        pcon->m_usFlags |= NiTimeController::ACTIVE_MASK;
        pcon->SetTarget(psceneObject);
        if (spWorldObject) {
            NiDecRefCount(spWorldObject);
        }
    }
}

void WeaponObject::Kill() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    NiAVObject* m_pkSceneObject = this->m_pkSceneObject;
    if (m_pkSceneObject) {
        bhkWorld::RemoveObjects(m_pkSceneObject, true, true);
        NiTimeController* Controller = m_pkSceneObject->GetController(&bhkBlendController::ms_RTTI);
        if (Controller) {
            m_pkSceneObject->RemoveController(Controller);
        }
    }
}

// The base's copy, then the three members the blend keeps.
void WeaponObject::CopyMembers(WeaponObject* apDest, NiCloningProcess& arCloneData) {
    bhkBlendCollisionObject::CopyMembers(apDest, arCloneData);
    apDest->fVelGain = fVelGain;
    apDest->fHeirGain = fHeirGain;
    apDest->pStoredWorld = pStoredWorld;
}

void WeaponObject::UpdateConstraints(
    bool abEnable,
    CFilter aOwnerFilter,
    NiAVObject* apRoot3d,
    bool abDefaultKeyframed
) {
    NiAVObject* pweaponAttach;
    NiAVObject* pbackpackAttach;
    NiAVObject* pweaponBone = BSUtilities::GetObjectByName(apRoot3d, "Weapon", false);
    if (pweaponBone) {
        pweaponAttach = pweaponBone->m_pkParent;
    }
    else {
        pweaponAttach = nullptr;
    }
    for (; pweaponAttach; pweaponAttach = pweaponAttach->m_pkParent) {
        bhkNiCollisionObject* m_pObject = pweaponAttach->m_spCollisionObject.m_pObject;
        if (m_pObject &&
            (!m_pObject->IsBhkBlendCollisionObject() ||
             pweaponAttach->m_spCollisionObject->IsBhkBlendCollisionObject()->spWorldObject)) {
            break;
        }
        if (pweaponAttach == apRoot3d) {
            pweaponAttach = nullptr;
            break;
        }
    }
    if (pweaponAttach == apRoot3d) {
        pweaponAttach = nullptr;
    }
    if (pweaponBone && pweaponAttach) {
        WeaponObject::Constrain(abEnable, aOwnerFilter, pweaponBone, pweaponAttach, abDefaultKeyframed);
    }

    static NiFixedString strBackpack("Backpack");
    NiAVObject* pbackpack = BSUtilities::GetObjectByName(apRoot3d, strBackpack, false);
    if (pbackpack) {
        pbackpackAttach = pbackpack->m_pkParent;
    }
    else {
        pbackpackAttach = nullptr;
    }
    for (; pbackpackAttach; pbackpackAttach = pbackpackAttach->m_pkParent) {
        bhkNiCollisionObject* v12 = pbackpackAttach->m_spCollisionObject.m_pObject;
        if (v12 &&
            (!v12->IsBhkBlendCollisionObject() ||
             pbackpackAttach->m_spCollisionObject->IsBhkBlendCollisionObject()->spWorldObject)) {
            break;
        }
        if (pbackpackAttach == apRoot3d) {
            pbackpackAttach = nullptr;
            break;
        }
    }
    if (pbackpackAttach == apRoot3d) {
        pbackpackAttach = nullptr;
    }
    if (pbackpack && pbackpackAttach) {
        WeaponObject::Constrain(abEnable, aOwnerFilter, pbackpack, pbackpackAttach, abDefaultKeyframed);
    }
}

// The object is allocated in the Havok heap context.
WeaponObject* WeaponObject::CreateWeaponObject(
    CFilter aOwnerFilter,
    NiAVObject* apOwner3d,
    NiAVObject* apObject,
    bool abDefaultKeyframed
) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (!apObject) {
        return nullptr;
    }
    bhkNiCollisionObject* pkCollisionObject = bhkUtilFunctions::FindNextCollisionObject(apObject);
    if (!pkCollisionObject) {
        return nullptr;
    }
    // The found collision object is passed on unchanged; the constructor takes it as a collision object.
    WeaponObject* pkResult = new WeaponObject(
        aOwnerFilter,
        apOwner3d,
        apObject,
        static_cast<bhkCollisionObject*>(pkCollisionObject),
        abDefaultKeyframed
    );
    return pkResult;
}

void WeaponObject::UpdateChain(NiAVObject* apWeaponObj, bhkWorld* apWorld) {
    NiNode* pkRootNode = apWeaponObj->IsNode();
    if (!pkRootNode) {
        return;
    }
    NiFixedString kChain;
    NiGlobalStringTable::GlobalStringHandle kChainHandle = NiGlobalStringTable::AddString("Chain01");
    kChain.SetHandle(kChainHandle);
    NiAVObject* pkChainNode = pkRootNode->GetObjectByName(kChain);
    // The interned buffer's header is untyped raw memory: the count is the word 8 bytes before the characters, and the
    // empty handle carries none.
    if (kChainHandle != NiGlobalStringTable::NULL_STRING) {
        NiAtomicDecrement(*reinterpret_cast<uint32_t*>(const_cast<char*>(kChainHandle - 8)));
    }
    if (!pkChainNode) {
        return;
    }
    NiUpdateData kUpdateData;
    kUpdateData.fTime = 0.0f;
    kUpdateData.bUpdateControllers = false;
    kUpdateData.bParallelUpdate = false;
    kUpdateData.bFoundParticles = false;
    kUpdateData.bFoundMorphController = false;
    kUpdateData.bSceneGraphChange = false;
    apWeaponObj->Update(kUpdateData);
    NiNode* pkParent = pkChainNode->m_pkParent;
    if (!pkParent) {
        return;
    }
    for (uint32_t i = 0; i < pkParent->m_kChildren.m_usSize; ++i) {
        NiAVObject* pkChild = pkParent->m_kChildren.m_pBase[i].m_pObject;
        if (!pkChild) {
            continue;
        }
        bhkNiCollisionObject* pkCollisionObject = pkChild->m_spCollisionObject.m_pObject;
        if (!pkCollisionObject) {
            continue;
        }
        pkCollisionObject->NitohkTransform();
        bhkWorld::SetMotion(pkChild, hkpMotion::MOTION_DYNAMIC, false, true, true);
    }
}

void WeaponObject::UpdateRigidBodyTransforms(NiAVObject* apObject, NiAVObject* apParent) {
    bhkCollisionObject* pweapCol;
    NiAVObject* pweaponObject;
    bhkRigidBody* pweapRB;
    bhkNiCollisionObject* m_pObject;
    bhkCollisionObject* pparentCol;
    bhkRigidBody* pparentRB;
    if (apObject) {
        pweapCol = bhkUtilFunctions::FindNextCollisionObject(apObject);
    }
    else {
        pweapCol = nullptr;
    }
    if (pweapCol) {
        pweaponObject = pweapCol->m_pkSceneObject;
        pweapRB = pweapCol->spWorldObject;
    }
    else {
        pweaponObject = nullptr;
        pweapRB = nullptr;
    }
    if (apParent && (m_pObject = apParent->m_spCollisionObject.m_pObject) != nullptr) {
        pparentCol = static_cast<bhkCollisionObject*>(m_pObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI));
    }
    else {
        pparentCol = nullptr;
    }
    if (pparentCol) {
        pparentRB = pparentCol->spWorldObject;
    }
    else {
        pparentRB = nullptr;
    }
    if (pweapRB && pparentRB && pweaponObject && apParent && apParent->m_pkParent) {
        NiQuaternion niq;
        NiTransform parentTransform;
        NiTransform parentToWeaponTransform;
        hkQuaternion hkr;
        hkVector4 hkv;
        hkTransform hktrans;
        pparentRB->GetTransform(hktrans);
        HK2NI(parentTransform, hktrans);
        // HK2NI leaves the scale, so the parent's world scale is copied.
        parentTransform.m_fScale = apParent->m_kWorld.m_fScale;
        GetTransformRecursive(pweaponObject, apParent, parentToWeaponTransform);
        NiTransform weaponObjectTransform = parentTransform * parentToWeaponTransform;
        niq.FromRotation(weaponObjectTransform.m_Rotate);
        NI2HK(hkv, weaponObjectTransform.m_Translate);
        NI2HK(hkr, niq);
        hkr.normalize();
        bhkNiCollisionObject::kKeyframeLock.Lock(nullptr);
        pweapRB->SetPosRot(hkv, hkr);
        bhkNiCollisionObject::kKeyframeLock.Unlock();
    }
}

// Ties apFrom's rigid body to apTo's with a limited hinge. Every constraint and action on apFrom's body is taken off it
// (out of the world first when it is in one) except chain, hinge-limit and ragdoll-limit constraints, which stay and
// make the chain follow afterwards; with abEnable, the hinge is built along apFrom's world x axis, the body's motion
// type is matched to the target's and the collision filters are merged.
void WeaponObject::Constrain(
    bool abEnable,
    CFilter aOwnerFilter,
    NiAVObject* apFrom,
    NiAVObject* apTo,
    bool abDefaultKeyframed
) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (!apFrom || !apTo) {
        return;
    }
    WeaponObject::UpdateRigidBodyTransforms(apFrom, apTo);
    bhkCollisionObject* psourceCol = bhkUtilFunctions::FindNextCollisionObject(apFrom);
    bhkCollisionObject* ptargetCol = bhkUtilFunctions::FindNextCollisionObject(apTo);
    if (!psourceCol || !ptargetCol) {
        return;
    }
    bhkRigidBody* prba = psourceCol->spWorldObject;
    bhkRigidBody* prbb = ptargetCol->spWorldObject;
    BSSimpleList<NiPointer<bhkSerializable>>* pacList = &prba->kActionConstrants;
    bool bhasChain = false;
    if (pacList) {
        do {
            bhkSerializable* pitem = pacList->m_item;
            if (!pitem) {
                break;
            }
            if (pitem->IsKindOf(&bhkConstraint::ms_RTTI)) {
                bhkConstraint* pcon = static_cast<bhkConstraint*>(pitem);
                hkpConstraintData* pdata = pcon->GetData();
                // A chain or limits constraint stays on the list; the walk steps past it.
                if (pdata->getType() > hkpConstraintData::BEGIN_CONSTRAINT_CHAIN_TYPES ||
                    pdata->getType() == hkpConstraintData::CONSTRAINT_TYPE_HINGE_LIMITS ||
                    pdata->getType() == hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL_LIMITS) {
                    pacList = pacList->m_pkNext;
                    bhasChain = true;
                    continue;
                }
                if (pcon->bhkConstraint::GethkWorld()) {
                    pcon->Remove();
                }
            }
            else if (pitem->IsKindOf(&bhkAction::ms_RTTI)) {
                bhkAction* pact = static_cast<bhkAction*>(pitem);
                if (pact->bhkAction::GethkWorld()) {
                    pact->Remove();
                }
            }
            pacList->RemoveHead();
        }
        while (pacList);
    }
    if (!abEnable) {
        return;
    }

    hkLimitedHingeConstraintCinfo ci;
    ci.hkLimitedHingeConstraintCinfo::CreateData();
    if (abDefaultKeyframed) {
        prba->SetMotionType(hkpMotion::MOTION_KEYFRAMED);
    }
    else {
        hkpMotion::MotionType emotionA = static_cast<hkpMotion::MotionType>(prba->GetMotionType());
        hkpMotion::MotionType emotionB = static_cast<hkpMotion::MotionType>(prbb->GetMotionType());
        bool bDynamicB = bhkRigidBody::IsMotionTypeDynamic(emotionB);
        if (bhkRigidBody::IsMotionTypeDynamic(emotionA) != bDynamicB) {
            // The target's blend controller decides: keyframed unless its last key's gain is below 1 (keyframed
            // when 1 <= gain or the compare is unordered); without one, the target's own motion type.
            hkpMotion::MotionType emotion;
            uint32_t uiKeys;
            bhkBlendController* pcon = static_cast<bhkBlendController*>(
                ptargetCol->m_pkSceneObject->GetController(&bhkBlendController::ms_RTTI)
            );
            if (pcon && (uiKeys = pcon->pBlendKeyA.m_uiESize) != 0) {
                if (!(pcon->pBlendKeyA.m_pBase[uiKeys - 1].fGain < 1.0f)) {
                    emotion = hkpMotion::MOTION_KEYFRAMED;
                }
                else {
                    emotion = hkpMotion::MOTION_DYNAMIC;
                }
            }
            else {
                hkpMotion::MotionType emotionTarget = static_cast<hkpMotion::MotionType>(prbb->GetMotionType());
                emotion = bhkRigidBody::IsMotionTypeDynamic(emotionTarget) ? hkpMotion::MOTION_DYNAMIC
                                                                           : hkpMotion::MOTION_KEYFRAMED;
            }
            prba->SetMotionType(emotion);
        }
    }
    ci.SetEntities(prba, prbb);

    hkpRigidBody* phkBodyA = **prba;
    hkTransform fixPointa = phkBodyA->m_motion.m_motionState.m_transform;
    hkpRigidBody* phkBodyB = **prbb;
    hkTransform fixPointb = phkBodyB->m_motion.m_motionState.m_transform;
    // The converted pivot is not used.
    hkTransform pivot;
    NI2HK(pivot, apFrom->m_kWorld);
    fixPointa.m_rotation.renormalize();
    fixPointb.m_rotation.renormalize();
    static_cast<hkpLimitedHingeConstraintData*>(ci.pData)->setMinAngularLimit(-0.1f);
    static_cast<hkpLimitedHingeConstraintData*>(ci.pData)->setMaxAngularLimit(0.1f);
    static_cast<hkpLimitedHingeConstraintData*>(ci.pData)->setMaxFrictionTorque(1.0f);
    // The hinge axis: the first column of the source object's world rotation.
    NiPoint3 xAxisWorld;
    const NiMatrix3& kRotate = psourceCol->m_pkSceneObject->m_kWorld.m_Rotate;
    kRotate.GetCol(0, xAxisWorld);
    hkVector4 axis;
    axis.set(xAxisWorld.x, xAxisWorld.y, xAxisWorld.z, 0.0f);
    axis.normalize3();
    static_cast<hkpLimitedHingeConstraintData*>(ci.pData)
        ->setInWorldSpace(fixPointa, fixPointb, fixPointa.m_translation, axis);

    bhkLimitedHingeConstraint* ph = new bhkLimitedHingeConstraint(ci);
    prba->AddAction(ph);
    // The merged filter: the source's bits 0xE080, aOwnerFilter's bits 0xFFFF1400, the target's layer, and
    // 0x1400.
    CFilter ifilterA;
    prba->GetCollisionFilter(&ifilterA);
    CFilter ifilterB;
    CFilter* pfilterB = prbb->GetCollisionFilter(&ifilterB);
    CFilter ifilter;
    ifilter.iFilter = (ifilterA.iFilter & 0xE080) | (aOwnerFilter.iFilter & 0xFFFF1400) | (pfilterB->iFilter & 0x7F) |
        0x1400;
    prba->SetCollisionFilter(ifilter);

    bhkWorld* pw = bhkWorld::Getbhk(prbb->GethkWorld());
    if (!pw) {
        bhkBlendCollisionObject* pblendCol = static_cast<bhkBlendCollisionObject*>(
            NiObject::DynamicCastStatic(&bhkBlendCollisionObject::ms_RTTI, ptargetCol)
        );
        if (pblendCol) {
            pw = pblendCol->pStoredWorld;
        }
    }
    if (pw && prbb->IsAddedToWorld()) {
        if (prba->GethkWorld()) {
            prba->AddActConToWorld(pw);
        }
        else {
            pw->Add(prba);
        }
    }
    if (bhasChain) {
        WeaponObject::UpdateChain(apFrom, pw);
    }
}

// hkConstraintCinfo's constructor sets no data, PSI priority and no entities.
hkLimitedHingeConstraintCinfo::hkLimitedHingeConstraintCinfo() {}
