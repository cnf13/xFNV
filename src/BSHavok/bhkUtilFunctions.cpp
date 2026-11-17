#include "BSHavok/bhkUtilFunctions.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/ahkpWorld.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkCapsuleShape.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkPCollisionObject.h"
#include "BSHavok/bhkPhantom.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkRigidBodyCinfo.h"
#include "BSHavok/bhkRigidBodyT.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/bhkStiffSpringConstraint.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldNS.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSXFlags.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/Array/hkSmallArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Collide/hkpResponseModifier.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Ragdoll/hkpRagdollConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/StiffSpring/hkpStiffSpringConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkConstraintInternal.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "Reconstruction.hpp"

#include <cstdint>
#include <xmmintrin.h>

namespace bhkConvert {} // namespace bhkConvert

// The shape collection's wrapper: through the container of a bounding-volume tree or MOPP, or the list shape itself.
bhkShapeCollection* bhkUtilFunctions::FindbhkShapeCollection(const hkpCdBody* arBody) {
    const hkpCdBody* pParent = arBody->m_parent;
    if (!pParent) {
        pParent = arBody;
    }
    bhkShapeCollection* pResult = nullptr;
    if (!pParent) {
        return pResult;
    }
    const hkpShape* pShape = pParent->m_shape;
    hkpShapeType eType = pShape->m_type;
    if (eType == HK_SHAPE_BV_TREE || eType == HK_SHAPE_MOPP) {
        const hkpShapeCollection* pCollection = static_cast<const hkpShapeCollection*>(pShape->getContainer());
        if (pCollection) {
            return static_cast<bhkShapeCollection*>(pCollection->m_userData);
        }
        return nullptr;
    }
    if (pShape && eType == HK_SHAPE_LIST) {
        return static_cast<bhkShapeCollection*>(pShape->m_userData);
    }
    return pResult;
}

// The scene graph object of the collidable's owner: an entity's through the collision object its property holds; a
// phantom's the same, else through its wrapper. The property is keyed by the collision object's RTTI address; its value
// is an untyped word holding the collision object, read by its low 32 bits.
NiAVObject* bhkUtilFunctions::GetNiAVObject(const hkpCollidable* apCollidable) {
    hkpPropertyValue kProperty;
    // EXE-INLINED 0x4B59F0
    hkpRigidBody* pBody = apCollidable->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_ENTITY
        ? static_cast<hkpRigidBody*>(static_cast<hkpWorldObject*>(apCollidable->getOwner()))
        : nullptr;
    if (pBody) {
        auto* pCollisionObject = reinterpret_cast<bhkNiCollisionObject*>(static_cast<uintptr_t>(
            bhkWorldObject::GetProperty(&kProperty, pBody, bhkPropertyKey(bhkNiCollisionObject::ms_RTTI))->m_data
        ));
        return pCollisionObject ? pCollisionObject->m_pkSceneObject : nullptr;
    }
    // EXE-INLINED 0x4B5950
    hkpPhantom* pPhantom = apCollidable->m_broadPhaseHandle.m_type == hkpWorldObject::BROAD_PHASE_PHANTOM
        ? static_cast<hkpPhantom*>(static_cast<hkpWorldObject*>(apCollidable->getOwner()))
        : nullptr;
    if (!pPhantom) {
        return nullptr;
    }
    auto* pCollisionObject = reinterpret_cast<bhkNiCollisionObject*>(static_cast<uintptr_t>(
        bhkWorldObject::GetProperty(&kProperty, pPhantom, bhkPropertyKey(bhkNiCollisionObject::ms_RTTI))->m_data
    ));
    if (pCollisionObject) {
        return pCollisionObject->m_pkSceneObject;
    }
    auto* pWrapper = static_cast<bhkPhantom*>(pPhantom->getUserData());
    return pWrapper ? pWrapper->GetAVObject(0) : nullptr;
}

bhkBlendCollisionObject* bhkUtilFunctions::FindbhkBlendCollisionObject(NiNode* apNode) {
    bhkBlendCollisionObject* v1 = nullptr;
    if (apNode) {
        bhkNiCollisionObject* m_pObject = apNode->m_spCollisionObject.m_pObject;
        NiObject* v3 = m_pObject ? m_pObject->DynamicCastMember(&bhkBlendCollisionObject::ms_RTTI) : nullptr;
        v1 = static_cast<bhkBlendCollisionObject*>(v3);
        if (!v3) {
            NiAVObject* v4;
            NiNode* v5;
            if (apNode->m_kChildren.m_usSize && (v4 = apNode->m_kChildren.m_pBase->m_pObject) != nullptr) {
                NiNode* v6 = static_cast<NiNode*>(v4->DynamicCastMember(&NiNode::ms_RTTI));
                v5 = v6;
                if (v6) {
                    NiObject* v8;
                    NiObject* v7 = v6->m_spCollisionObject.m_pObject;
                    if (v7) {
                        v8 = v7->DynamicCastMember(&bhkBlendCollisionObject::ms_RTTI);
                    }
                    else {
                        v8 = nullptr;
                    }
                    v1 = static_cast<bhkBlendCollisionObject*>(v8);
                }
            }
            else {
                v5 = nullptr;
            }
            if (!v1) {
                NiAVObject* v9;
                NiNode* v10;
                NiObject* v12;
                if (!apNode->m_kChildren.m_usSize ||
                    ((v9 = apNode->m_kChildren.m_pBase->m_pObject) == nullptr ||
                     (v10 = static_cast<NiNode*>(v9->DynamicCastMember(&NiNode::ms_RTTI)), (v5 = v10) == nullptr))) {
                LABEL_23:;
                    v5 = apNode;
                LABEL_24:;
                    if (!v1) {
                        uint32_t v13 = v5->m_kChildren.m_usESize != 1;
                        if (v5->m_kChildren.m_usSize > v13) {
                            NiAVObject* v14 = v5->m_kChildren.m_pBase[v13].m_pObject;
                            if (v14) {
                                NiObject* v16;
                                bhkNiCollisionObject* v15 = v14->m_spCollisionObject.m_pObject;
                                if (v15) {
                                    v16 = v15->DynamicCastMember(&bhkBlendCollisionObject::ms_RTTI);
                                }
                                else {
                                    v16 = nullptr;
                                }
                                v1 = static_cast<bhkBlendCollisionObject*>(v16);
                                if (!v16) {
                                    NiNode* v17 = v14->IsNode();
                                    if (v17) {
                                        uint32_t v19 = 0;
                                        if (v17->m_kChildren.m_usSize) {
                                            do {
                                                NiObject* v21;
                                                NiObject* v20 = v17->m_kChildren.m_pBase[v19]
                                                                    .m_pObject->m_spCollisionObject.m_pObject;
                                                if (v20) {
                                                    v21 = v20->DynamicCastMember(&bhkBlendCollisionObject::ms_RTTI);
                                                }
                                                else {
                                                    v21 = nullptr;
                                                }
                                                v1 = static_cast<bhkBlendCollisionObject*>(v21);
                                                if (v21) {
                                                    break;
                                                }
                                                ++v19;
                                            }
                                            while (v19 < v17->m_kChildren.m_usSize);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    return v1;
                }
                NiObject* v11 = v10->m_spCollisionObject.m_pObject;
                if (v11) {
                    v12 = v11->DynamicCastMember(&bhkBlendCollisionObject::ms_RTTI);
                }
                else {
                    v12 = nullptr;
                }
                v1 = static_cast<bhkBlendCollisionObject*>(v12);
            }
            if (v5) {
                goto LABEL_24;
            }
            goto LABEL_23;
        }
    }
    return v1;
}

float bhkUtilFunctions::GetHeirGain(NiNode* apNode) {
    bhkBlendCollisionObject* pColObj = FindbhkBlendCollisionObject(apNode);
    if (pColObj) {
        return pColObj->fHeirGain;
    }
    return 1.0f;
}

bhkPCollisionObject* bhkUtilFunctions::IsbhkPCollisionObject(hkpWorldObject* apBody) {
    bhkNiCollisionObject* pCollisionObject = bhkNiCollisionObject::Getbhk(apBody);
    return static_cast<bhkPCollisionObject*>(
        NiObject::DynamicCastStatic(&bhkPCollisionObject::ms_RTTI, pCollisionObject)
    );
}

// The first collision object under apObject, searched breadth first to at most auiMaxLevel levels down, on a queue that
// starts with room for 32 levels.
bhkCollisionObject* bhkUtilFunctions::FindCollisionObjectBFS(NiAVObject* apObject, const uint32_t auiMaxLevel) {
    using bfsQueueA_t = BSSimpleArray<bhkUtilFunctions::AVLevel, 1024>;
    bfsQueueA_t bfsQueueA(32u, 0u);
    bhkCollisionObject* pResult = bhkUtilFunctions::FindCollisionObjectBFSRec(apObject, bfsQueueA, auiMaxLevel, 0);
    return pResult;
}

bool bhkUtilFunctions::IsRigidBodyActive(NiAVObject* apObject) {
    bhkNiCollisionObject* v3;
    BSFadeNode* v1 = static_cast<BSFadeNode*>(apObject);
    bhkNiCollisionObject* m_pObject = apObject->m_spCollisionObject.m_pObject;
    if (!m_pObject || (v3 = m_pObject->IsBhkNiCollisionObject()) == nullptr) {
        BSXFlags* v4 = BSXFlags::Find(v1);
        if (!v4) {
            return false;
        }
        if ((v4->m_iValue & 2) == 0) {
            return false;
        }
        NiAVObject* v5 = v1->m_kChildren.m_usSize ? v1->m_kChildren.m_pBase->m_pObject : nullptr;
        bhkNiCollisionObject* v6 = v5->m_spCollisionObject.m_pObject;
        if (!v6) {
            return false;
        }
        v3 = v6->IsBhkNiCollisionObject();
        if (!v3) {
            return false;
        }
    }
    NiObject* spWorldObject = (NiObject*)v3->spWorldObject;
    bool result = false;
    if (spWorldObject) {
        NiObject* v8 = spWorldObject->DynamicCastMember(&bhkRigidBody::ms_RTTI);
        if (v8) {
            // The rigid body's Havok entity (bhkRefObject::phkObject, +8).
            hkpEntity* v9 = static_cast<hkpEntity*>(static_cast<bhkRigidBody*>(v8)->phkObject);
            if (v9) {
                if (v9->isActive().m_bool) {
                    return true;
                }
            }
        }
    }
    return result;
}

// Replaces each limited-hinge or ragdoll constraint (also one wrapped in a malleable constraint) on the rigid bodies of
// apObject's subtree by a stiff spring between the same pivots, at their current distance. The thread's memory context
// is set to Havok's and restored on both exits.
void bhkUtilFunctions::ReplaceConstraints(NiAVObject* apObject, hkpConstraintData::ConstraintType aeType) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    if (!apObject || aeType != hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING) {
        pTLSData->eMemContext = eMemContext;
        return;
    }
    bhkNiCollisionObject* pNiCollision = apObject->m_spCollisionObject.m_pObject;
    if (pNiCollision) {
        bhkCollisionObject* pco =
            static_cast<bhkCollisionObject*>(pNiCollision->DynamicCastMember(&bhkCollisionObject::ms_RTTI));
        if (pco) {
            bhkRigidBody* prb = pco->spWorldObject;
            if (prb) {
                for (BSSimpleList<NiPointer<bhkSerializable>>* pactCon = &prb->kActionConstrants;
                     pactCon && pactCon->m_item.m_pObject; pactCon = pactCon->m_pkNext) {
                    bhkConstraint* pcon = static_cast<bhkConstraint*>(
                        pactCon->m_item.m_pObject->DynamicCastMember(&bhkConstraint::ms_RTTI)
                    );
                    if (!pcon) {
                        continue;
                    }
                    hkVector4 pivotA;
                    hkVector4 pivotB;
                    bhkRigidBody* prbA = nullptr;
                    bhkRigidBody* prbB = nullptr;
                    hkpConstraintInstance* phkConstraint;
                    hkpEntity* phkEntity;
                    // EXE-INLINED 0x887A10
                    hkpConstraintData* pdata = nullptr;
                    // EXE-INLINED 0x4AE750
                    phkConstraint = static_cast<hkpConstraintInstance*>(pcon->phkObject);
                    if (phkConstraint) {
                        pdata = phkConstraint->getDataRw();
                    }
                    else if (pcon->pInfo) {
                        pdata = static_cast<hkConstraintCinfo*>(pcon->pInfo)->pData;
                    }
                    // The limited-hinge and ragdoll datas both start with the same hkpSetLocalTransformsConstraintAtom.
                    switch (pdata->getType()) {
                        case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE: {
                            // EXE-INLINED 0x887A10
                            hkpConstraintData* pData = nullptr;
                            // EXE-INLINED 0x4AE750
                            phkConstraint = static_cast<hkpConstraintInstance*>(pcon->phkObject);
                            if (phkConstraint) {
                                pData = phkConstraint->getDataRw();
                            }
                            else if (pcon->pInfo) {
                                pData = static_cast<hkConstraintCinfo*>(pcon->pInfo)->pData;
                            }
                            hkpLimitedHingeConstraintData* pHinge = static_cast<hkpLimitedHingeConstraintData*>(pData);
                            pivotA = pHinge->m_atoms.m_transforms.m_transformA.getTranslation();
                            pivotB = pHinge->m_atoms.m_transforms.m_transformB.getTranslation();
                            // EXE-INLINED none: bhkConstraint::GetEntityA
                            phkEntity = pcon->GethkEntityA();
                            prbA = phkEntity ? static_cast<bhkRigidBody*>(phkEntity->m_userData) : nullptr;
                            // EXE-INLINED 0x4D8530
                            phkEntity = pcon->GethkEntityB();
                            prbB = phkEntity ? static_cast<bhkRigidBody*>(phkEntity->m_userData) : nullptr;
                            break;
                        }
                        case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL: {
                            // EXE-INLINED 0x887A10
                            hkpConstraintData* pData = nullptr;
                            // EXE-INLINED 0x4AE750
                            phkConstraint = static_cast<hkpConstraintInstance*>(pcon->phkObject);
                            if (phkConstraint) {
                                pData = phkConstraint->getDataRw();
                            }
                            else if (pcon->pInfo) {
                                pData = static_cast<hkConstraintCinfo*>(pcon->pInfo)->pData;
                            }
                            hkpRagdollConstraintData* pRagdoll = static_cast<hkpRagdollConstraintData*>(pData);
                            pivotA = pRagdoll->m_atoms.m_transforms.m_transformA.getTranslation();
                            pivotB = pRagdoll->m_atoms.m_transforms.m_transformB.getTranslation();
                            // EXE-INLINED none: bhkConstraint::GetEntityA
                            phkEntity = pcon->GethkEntityA();
                            prbA = phkEntity ? static_cast<bhkRigidBody*>(phkEntity->m_userData) : nullptr;
                            // EXE-INLINED 0x4D8530
                            phkEntity = pcon->GethkEntityB();
                            prbB = phkEntity ? static_cast<bhkRigidBody*>(phkEntity->m_userData) : nullptr;
                            break;
                        }
                        case hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE: {
                            // EXE-INLINED 0x887A10
                            hkpConstraintData* pData = nullptr;
                            // EXE-INLINED 0x4AE750
                            phkConstraint = static_cast<hkpConstraintInstance*>(pcon->phkObject);
                            if (phkConstraint) {
                                pData = phkConstraint->getDataRw();
                            }
                            else if (pcon->pInfo) {
                                pData = static_cast<hkConstraintCinfo*>(pcon->pInfo)->pData;
                            }
                            hkpConstraintData* pwrapped =
                                static_cast<hkpMalleableConstraintData*>(pData)->getWrappedConstraintData();
                            // Both tests run: a ragdoll that is not a limited hinge keeps the ragdoll's results.
                            if (pwrapped->getType() == hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL) {
                                hkpRagdollConstraintData* pRagdoll = static_cast<hkpRagdollConstraintData*>(pwrapped);
                                pivotA = pRagdoll->m_atoms.m_transforms.m_transformA.getTranslation();
                                pivotB = pRagdoll->m_atoms.m_transforms.m_transformB.getTranslation();
                                // EXE-INLINED none: bhkConstraint::GetEntityA
                                phkEntity = pcon->GethkEntityA();
                                prbA = phkEntity ? static_cast<bhkRigidBody*>(phkEntity->m_userData) : nullptr;
                                // EXE-INLINED 0x4D8530
                                phkEntity = pcon->GethkEntityB();
                                prbB = phkEntity ? static_cast<bhkRigidBody*>(phkEntity->m_userData) : nullptr;
                            }
                            if (pwrapped->getType() == hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE) {
                                hkpLimitedHingeConstraintData* pHinge =
                                    static_cast<hkpLimitedHingeConstraintData*>(pwrapped);
                                pivotA = pHinge->m_atoms.m_transforms.m_transformA.getTranslation();
                                pivotB = pHinge->m_atoms.m_transforms.m_transformB.getTranslation();
                                // EXE-INLINED none: bhkConstraint::GetEntityA
                                phkEntity = pcon->GethkEntityA();
                                prbA = phkEntity ? static_cast<bhkRigidBody*>(phkEntity->m_userData) : nullptr;
                                // EXE-INLINED 0x4D8530
                                phkEntity = pcon->GethkEntityB();
                                prbB = phkEntity ? static_cast<bhkRigidBody*>(phkEntity->m_userData) : nullptr;
                            }
                            break;
                        }
                        default: continue;
                    }
                    if (!prbA || !prbB) {
                        continue;
                    }

                    alignas(16) hkStiffSpringConstraintCinfo cinfo;
                    cinfo.hkStiffSpringConstraintCinfo::CreateData();
                    // EXE-INLINED 0x9BA350
                    cinfo.pEntityA = static_cast<hkpEntity*>(prbA->phkObject);
                    // EXE-INLINED 0x9BA390
                    cinfo.pEntityB = static_cast<hkpEntity*>(prbB->phkObject);
                    if (cinfo.pData) {
                        // EXE-INLINED none: hkStiffSpringConstraintCinfo::SetPivotInA
                        static_cast<hkpStiffSpringConstraintData*>(cinfo.pData)->m_atoms.m_pivots.m_translationA =
                            pivotA;
                        // EXE-INLINED none: hkStiffSpringConstraintCinfo::SetPivotInB
                        static_cast<hkpStiffSpringConstraintData*>(cinfo.pData)->m_atoms.m_pivots.m_translationB =
                            pivotB;
                    }
                    hkTransform rbATransf;
                    hkTransform rbBTransf;
                    prbA->GetTransform(rbATransf);
                    prbB->GetTransform(rbBTransf);
                    hkVector4 posA;
                    hkVector4 posB;
                    posA.setTransformedPos(rbATransf, pivotA);
                    posB.setTransformedPos(rbBTransf, pivotB);
                    posA.sub4(posB);
                    float flen = posA.length3();
                    if (flen == 0.0f) {
                        flen = 0.001f;
                    }
                    if (cinfo.pData) {
                        // EXE-INLINED none: hkStiffSpringConstraintCinfo::SetSpringLength
                        static_cast<hkpStiffSpringConstraintData*>(cinfo.pData)->m_atoms.m_spring.m_length = flen;
                    }
                    void* pMemory = NiMemObject::operator new(sizeof(bhkStiffSpringConstraint));
                    bhkConstraint* pspring = pMemory ? ::new (pMemory) bhkStiffSpringConstraint(&cinfo) : nullptr;
                    const hkpWorld* phkWorld = prbA->GethkWorld();
                    bhkWorld* pw = phkWorld
                        ? static_cast<bhkWorld*>(static_cast<const ahkpWorld*>(phkWorld)->m_userData)
                        : nullptr;
                    // EXE-INLINED 0x4AE750
                    phkConstraint = static_cast<hkpConstraintInstance*>(pcon->phkObject);
                    if (phkConstraint->getOwner()) {
                        hkpResponseModifier::disableConstraint(phkConstraint, *phkConstraint->getOwner());
                    }
                    if (pactCon->m_pkNext) {
                        NiPointer<bhkSerializable> spspring(pspring);
                        pactCon->m_pkNext->AddHead(spspring);
                    }
                    else {
                        NiPointer<bhkSerializable> spspring(pspring);
                        pactCon->AddTail(spspring);
                    }
                    // EXE-INLINED 0x56D2C0
                    if (pspring) {
                        pspring->Add(pw);
                    }
                }
            }
        }
    }
    NiNode* pnode = apObject->IsNode();
    if (pnode) {
        for (uint32_t ui = 0; ui < pnode->m_kChildren.m_usSize; ++ui) {
            bhkUtilFunctions::ReplaceConstraints(pnode->m_kChildren.m_pBase[ui].m_pObject, aeType);
        }
    }
    pTLSData->eMemContext = eMemContext;
}

void bhkUtilFunctions::DampBodiesVelocity(NiAVObject* apObject, float afDamp) {
    if (apObject) {
        bhkNiCollisionObject* m_pObject = apObject->m_spCollisionObject.m_pObject;
        if (m_pObject) {
            bhkCollisionObject* v3 =
                static_cast<bhkCollisionObject*>(m_pObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI));
            if (v3) {
                bhkRigidBody* spWorldObject = v3->spWorldObject;
                if (spWorldObject) {
                    hkVector4 b;
                    hkVector4 v16;
                    hkpRigidBody* phkObject = static_cast<hkpRigidBody*>(spWorldObject->phkObject);
                    hkVector4* p_m_linearVelocity = &phkObject->m_motion.m_linearVelocity;
                    if (!phkObject) {
                        p_m_linearVelocity = &hkVector4Zero;
                    }
                    __m128 m_quad = p_m_linearVelocity->m_quad;
                    b.m_quad = p_m_linearVelocity->m_quad;
                    hkVector4* p_m_angularVelocity = &phkObject->m_motion.m_angularVelocity;
                    if (!phkObject) {
                        p_m_angularVelocity = &hkVector4Zero;
                    }
                    __m128 v9 = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(afDamp), _mm_set_ss(afDamp), 0), m_quad);
                    __m128 v10 = _mm_mul_ps(
                        _mm_shuffle_ps(_mm_set_ss(afDamp), _mm_set_ss(afDamp), 0), p_m_angularVelocity->m_quad
                    );
                    b.m_quad = v9;
                    v16.m_quad = v10;
                    if (phkObject) {
                        phkObject->setLinearVelocity(b);
                    }
                    hkpRigidBody* v11 = static_cast<hkpRigidBody*>(spWorldObject->phkObject);
                    if (v11) {
                        v11->setAngularVelocity(v16);
                    }
                }
            }
        }
        NiNode* v12 = apObject->IsNode();
        if (v12) {
            uint32_t v14 = 0;
            if (v12->m_kChildren.m_usSize) {
                do {
                    bhkUtilFunctions::DampBodiesVelocity(v12->m_kChildren.m_pBase[v14++].m_pObject, afDamp);
                }
                while (v14 < v12->m_kChildren.m_usSize);
            }
        }
    }
}

// The first collision object in apObj's subtree, depth first, apObj's own first.
bhkCollisionObject* bhkUtilFunctions::FindNextCollisionObject(NiAVObject* apObj) {
    auto* pCollisionObject = static_cast<bhkCollisionObject*>(apObj->m_spCollisionObject.m_pObject);
    if (pCollisionObject || !apObj->IsNode()) {
        return pCollisionObject;
    }
    // The children are read through apObj, not through IsNode's result.
    auto* pNode = static_cast<NiNode*>(apObj);
    for (uint32_t ui = 0; ui < pNode->m_kChildren.m_usSize; ++ui) {
        if (pCollisionObject) {
            break;
        }
        NiAVObject* pChild = pNode->m_kChildren.m_pBase[ui].m_pObject;
        if (pChild) {
            pCollisionObject = bhkUtilFunctions::FindNextCollisionObject(pChild);
        }
    }
    return pCollisionObject;
}

void bhkUtilFunctions::GatherRigidBodies(
    NiAVObject* apObject,
    hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arBodyA
) {
    if (apObject) {
        bhkNiCollisionObject* m_pObject = apObject->m_spCollisionObject.m_pObject;
        if (m_pObject) {
            bhkCollisionObject* v3 =
                static_cast<bhkCollisionObject*>(m_pObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI));
            if (v3) {
                bhkRigidBody* spWorldObject = v3->spWorldObject;
                if (spWorldObject) {
                    hkpRigidBody* phkObject = static_cast<hkpRigidBody*>(spWorldObject->phkObject);
                    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
                    if (arBodyA.m_size == (arBodyA.m_capacityAndFlags & hkArrayBase<hkpRigidBody*>::CAPACITY_MASK)) {
                        hkArrayUtil::_reserveMore(*Instance->m_heap, &arBodyA, sizeof(hkpRigidBody*));
                    }
                    arBodyA.m_data[arBodyA.m_size++] = phkObject;
                }
            }
        }
        NiNode* v7 = apObject->IsNode();
        if (v7) {
            uint32_t v9 = 0;
            if (v7->m_kChildren.m_usSize) {
                do {
                    bhkUtilFunctions::GatherRigidBodies(v7->m_kChildren.m_pBase[v9++].m_pObject, arBodyA);
                }
                while (v9 < v7->m_kChildren.m_usSize);
            }
        }
    }
}

// Moves the root's local translation to where the ragdoll's root bone is (its rigid body's position and rotation
// when it is in the world; else, with no heir gain, the root's own local transform, otherwise the bone node's world
// transform), keeping the root's height unless the actor is dead and the blend has no heir gain, and writes the
// movement into arOut (not on the no-gain path out of the world). With no heir gain the root also takes the bone's
// rotation. A living actor's root then updates its world transform and its subtree.
void bhkUtilFunctions::GetKnockedDownMovementDelta(NiNode* apObject, NiPoint3* arOut, bool abAlive) {
    if (!apObject) {
        return;
    }
    NiNode* pRoot = apObject->IsNode();
    if (!pRoot || !pRoot->m_kChildren.m_usSize) {
        return;
    }
    NiAVObject* pChild = pRoot->m_kChildren.m_pBase[0].m_pObject;
    if (!pChild) {
        return;
    }
    NiNode* pChildNode = pChild->IsNode();
    if (!pChildNode || !pChildNode->m_kChildren.m_usSize) {
        return;
    }
    NiAVObject* pBone = pChildNode->m_kChildren.m_pBase[0].m_pObject;
    if (!pBone) {
        return;
    }
    NiNode* pBoneNode = pBone->IsNode();
    if (!pBoneNode) {
        return;
    }
    bhkBlendCollisionObject* pBlend = bhkUtilFunctions::FindbhkBlendCollisionObject(pChildNode);
    if (!pBlend) {
        return;
    }
    bool bNoGain = 0.0f == pBlend->fHeirGain;
    if (!pBoneNode->m_spCollisionObject.m_pObject && !bNoGain) {
        return;
    }
    NiQuaternion kQuaternion;
    NiTransform kTransform;
    bhkRigidBody* pBody = pBlend->spWorldObject;
    bool bFromLocal = false;
    if (pBody && pBody->IsAddedToWorld()) {
        pBody->GetPosition(kTransform.m_Translate);
        pBody->GetRotation(kQuaternion);
        kQuaternion.ToRotation(kTransform.m_Rotate);
    }
    else if (bNoGain) {
        kTransform.m_Translate = pRoot->m_kLocal.m_Translate;
        bFromLocal = true;
        kQuaternion.FromRotation(pRoot->m_kLocal.m_Rotate);
        kQuaternion.ToRotation(kTransform.m_Rotate);
    }
    else {
        kTransform = pBoneNode->m_kWorld;
    }
    NiTransform kLocal = pRoot->m_kLocal;
    if (abAlive || !bNoGain) {
        float fZ = kLocal.m_Translate.z;
        kTransform.m_Translate.z = fZ;
    }
    float fX = kTransform.m_Translate.x;
    float fY = kTransform.m_Translate.y;
    float fZ = kTransform.m_Translate.z;
    if (!bFromLocal) {
        arOut->x = fX;
        arOut->y = fY;
        arOut->z = fZ;
        *arOut -= pRoot->m_kLocal.m_Translate;
    }
    pRoot->m_kLocal.m_Translate.x = fX;
    pRoot->m_kLocal.m_Translate.y = fY;
    pRoot->m_kLocal.m_Translate.z = fZ;
    if (bNoGain) {
        pRoot->m_kLocal.m_Rotate = kTransform.m_Rotate;
    }
    if (abAlive) {
        pRoot->UpdateWorldLoc();
        NiUpdateData kUpdateData;
        kUpdateData.fTime = 0.0f;
        kUpdateData.bUpdateControllers = false;
        kUpdateData.bParallelUpdate = false;
        kUpdateData.bFoundParticles = false;
        kUpdateData.bFoundMorphController = false;
        kUpdateData.bSceneGraphChange = false;
        static_cast<NiAVObject*>(pRoot)->UpdateWorldData(kUpdateData);
    }
}

// The rigid body below apRoot whose shape is closest to arSphere, or null.
bhkRigidBody* bhkUtilFunctions::GetClosestBody(NiAVObject* apRoot, hkSphere& arSphere, int32_t abIgnoreHead) {
    ClosestBody clBody;
    clBody.pBody = nullptr;
    GetClosestBodyRec(apRoot, arSphere, clBody, abIgnoreHead);
    return clBody.pBody;
}

// The summed mass of the rigid bodies in apObject's subtree.
float bhkUtilFunctions::GetTotalMass(NiAVObject* apObject) {
    float fMass = 0.0f;
    if (apObject) {
        bhkCollisionObject* pCollisionObject = bhkCollisionObject::GetbhkCollisionObject(apObject);
        bhkRigidBody* pBody = pCollisionObject ? pCollisionObject->GetRigidBody() : nullptr;
        if (pBody) {
            fMass = pBody->GetMass() + fMass;
        }
        NiNode* pNode = apObject->IsNode();
        if (pNode && pNode->GetArrayCount()) {
            for (uint32_t ui = 0; ui < pNode->GetArrayCount(); ++ui) {
                fMass = bhkUtilFunctions::GetTotalMass(pNode->GetAt(ui)) + fMass;
            }
        }
    }
    return fMass;
}

void bhkUtilFunctions::SetUseVelocityRecurse(bool abUseVelocity, NiAVObject* apObject) {
    bhkNiCollisionObject* m_pObject = apObject->m_spCollisionObject.m_pObject;
    if (m_pObject) {
        if (abUseVelocity) {
            m_pObject->usFlags |= 32u;
        }
        else {
            m_pObject->usFlags &= ~32u;
        }
    }
    NiNode* v3 = apObject->IsNode();
    if (v3) {
        for (uint32_t i = 0; i < v3->m_kChildren.m_usSize; ++i) {
            NiPointer<NiAVObject>* m_pBase = v3->m_kChildren.m_pBase;
            if (m_pBase[i].m_pObject) {
                bhkUtilFunctions::SetUseVelocityRecurse(abUseVelocity, m_pBase[i].m_pObject);
            }
        }
    }
}

// The capsule's creation info and the body's are filled in without running their constructors (the cinfo's bhkVector4
// ones would be calls here), so both are typed views of 16-aligned raw storage.
bhkRigidBody* bhkUtilFunctions::CreateCapsuleBody(float afHeight, float afRadius) {
    alignas(16) uint8_t aCapsuleStorage[sizeof(hkCapsuleShapeCinfo)];
    alignas(16) uint8_t aBodyStorage[sizeof(bhkRigidBodyCinfo)];
    hkCapsuleShapeCinfo& rkCapsule = *static_cast<hkCapsuleShapeCinfo*>(static_cast<void*>(aCapsuleStorage));
    bhkRigidBodyCinfo& rkBody = *static_cast<bhkRigidBodyCinfo*>(static_cast<void*>(aBodyStorage));
    rkCapsule.m_vertexA.fW = 0.0f;
    rkCapsule.m_userData = 0;
    rkCapsule.m_vertexB.fW = 0.0f;
    // Sums the radius twice and adds the double 1.0 at double precision before rounding to the float.
    if (afHeight <= afRadius + afRadius) {
        afHeight = static_cast<float>(static_cast<double>(afRadius) + afRadius + 1.0);
    }
    const float fScale = bhkConvert::fBS2HkScaleSC;
    const float fRadius = fScale * afRadius;
    rkCapsule.m_vertexA.fX = 0.0f;
    rkCapsule.m_vertexA.fY = 0.0f;
    const float fTop = afHeight - afRadius;
    rkCapsule.m_vertexA.fZ = fScale * fTop;
    rkCapsule.m_vertexB.fX = 0.0f;
    rkCapsule.m_vertexB.fY = 0.0f;
    rkCapsule.m_vertexB.fZ = fRadius;
    rkCapsule.m_radius = fRadius;
    void* pMemory = NiMemObject::operator new(sizeof(bhkCapsuleShape));
    bhkCapsuleShape* pShape = pMemory ? ::new (pMemory) bhkCapsuleShape(rkCapsule) : nullptr;
    if (!pShape) {
        return nullptr;
    }
    pShape->eMaterial = BHK_MATERIAL_WOOD;
    ::new (&rkBody) bhkRigidBodyCinfo;
    hkpShape* pkShape = static_cast<hkpShape*>(pShape->phkObject);
    rkBody.hk.m_mass = 0.0f;
    rkBody.hk.m_friction = 0.6f;
    rkBody.hk.m_motionType.m_storage = hkpMotion::MOTION_FIXED;
    // The literal collision filter info 0x40009 is used as the game does, stored in both infos.
    rkBody.m_collisionFilterInfo = 0x40009;
    rkBody.hk.m_collisionFilterInfo = 0x40009;
    rkBody.m_shape = pkShape;
    rkBody.hk.m_shape = pkShape;
    void* pBodyMemory = NiMemObject::operator new(sizeof(bhkRigidBody));
    bhkRigidBody* pBody = pBodyMemory ? ::new (pBodyMemory) bhkRigidBody(rkBody) : nullptr;
    // EXE-INLINED none: hkArray<hkpProperty>::~hkArray. The exe scales the capacity by 8 bytes per element, as
    // in bhkWorldObject::LoadBinary; the body info's own destructor is not called.
    hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
    int32_t iCapacityAndFlags = rkBody.m_properties.m_capacityAndFlags;
    rkBody.m_properties.m_size = 0;
    if (iCapacityAndFlags >= 0) {
        kRouter.m_heap->bufFree(
            rkBody.m_properties.m_data,
            (iCapacityAndFlags & hkArrayBase<hkpProperty>::CAPACITY_MASK) * sizeof(hkWorldObjectCinfo::Property)
        );
    }
    return pBody;
}

// The rigid body of biped part a2 in a1's subtree: a1's own when its filter's part is a2, else the first found below.
bhkRigidBody* bhkUtilFunctions::FindBipedPart(NiAVObject* a1, int32_t a2) {
    bhkRigidBody* pBody = nullptr;
    if (!a1) {
        return pBody;
    }
    bhkNiCollisionObject* pNiCollisionObject = a1->m_spCollisionObject.m_pObject;
    if (pNiCollisionObject) {
        auto* pCollisionObject =
            static_cast<bhkCollisionObject*>(pNiCollisionObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI));
        if (pCollisionObject) {
            bhkRigidBody* pRigidBody = pCollisionObject->spWorldObject;
            if (pRigidBody) {
                auto* pWorldObject = static_cast<hkpWorldObject*>(pRigidBody->phkObject);
                const hkpCollidable* pCollidable = pWorldObject ? &pWorldObject->m_collidable : nullptr;
                uint32_t uiFilterInfo = pCollidable ? pCollidable->m_broadPhaseHandle.m_collisionFilterInfo : 0;
                // EXE-INLINED 0x4D9DA0
                if (((uiFilterInfo >> 8) & 0x1F) == static_cast<uint32_t>(a2)) {
                    return pRigidBody;
                }
            }
        }
    }
    NiNode* pNode = a1->IsNode();
    if (pNode) {
        for (uint32_t i = 0; i < pNode->m_kChildren.m_usSize;) {
            pBody = bhkUtilFunctions::FindBipedPart(pNode->m_kChildren.m_pBase[i].m_pObject, a2);
            ++i;
            if (pBody) {
                break;
            }
        }
    }
    return pBody;
}

// The material of the collidable's shape key, asked of the bhkShapeCollection behind the shape of the collidable's
// parent (the collidable itself when it has none): a bv tree's or MOPP's child collection, or a list shape itself;
// BHK_MATERIAL_MAX when there is none.
HavokMaterialType bhkUtilFunctions::GetMaterialValue(const hkpCdBody* a1) {
    const hkpCdBody* pBody = a1->m_parent;
    if (!pBody) {
        pBody = a1;
    }
    bhkShape* pOwner = nullptr;
    if (pBody) {
        const hkpShape* pShape = pBody->m_shape;
        uint32_t uiType = pShape->m_type.m_storage;
        if (uiType == HK_SHAPE_BV_TREE || uiType == HK_SHAPE_MOPP) {
            const hkpShapeCollection* pCollection = static_cast<const hkpShapeCollection*>(pShape->getContainer());
            if (pCollection) {
                pOwner = pCollection->m_userData;
            }
        }
        else if (pShape && uiType == HK_SHAPE_LIST) {
            pOwner = pShape->m_userData;
        }
    }
    if (pOwner) {
        return static_cast<bhkShapeCollection*>(pOwner)->QSubMaterial(a1->m_shapeKey);
    }
    return BHK_MATERIAL_MAX;
}

const hkpRigidBody* bhkUtilFunctions::FindConstrainedSystemRoot(const hkpRigidBody* apBody) {
    const hkpRigidBody* v1 = apBody;
    if (apBody) {
    LABEL_2:;
        int32_t v2 = 0;
        bool v3 = true;
        if (v1->m_constraintsMaster.m_size) {
            int32_t v4 = 0;
            while (v3) {
                hkpConstraintData* m_data = v1->m_constraintsMaster.m_data[v4].m_constraint->getDataRw();
                v3 = m_data->getType() == 11;
                ++v2;
                ++v4;
                if (v2 >= v1->m_constraintsMaster.m_size) {
                    if (v3) {
                        return v1;
                    }
                    break;
                }
            }
            int32_t v6 = 0;
            int32_t v7 = 0;
            while (v6 < v1->m_constraintsMaster.m_size) {
                hkpConstraintInstance* m_constraint = v1->m_constraintsMaster.m_data[v7].m_constraint;
                if (m_constraint->getDataRw()->getType() != 11 &&
                    (m_constraint->getDataRw()->getType() != 12 ||
                     !*(static_cast<uint8_t*>(m_constraint->getInternal()->m_runtime) +
                        m_constraint->m_data[2].m_memSizeAndFlags))) {
                    hkpRigidBody* v9;
                    if (m_constraint->m_entities[0] == v1) {
                        v9 = static_cast<hkpRigidBody*>(m_constraint->m_entities[1]);
                    }
                    else {
                        v9 = static_cast<hkpRigidBody*>(m_constraint->m_entities[0]);
                    }
                    v1 = v9;
                    if (v9) {
                        goto LABEL_2;
                    }
                    return v1;
                }
                ++v6;
                ++v7;
            }
        }
    }
    return v1;
}

// Puts into arClosest the rigid body of apObject's subtree whose capsule comes closest to arSphere: biped and dead
// biped bodies of the head (unless abIgnoreHead) and of the body parts up to the right foot. A blended body is
// measured at its scene graph transform, times its local transform for a transformed body, while it is out of the
// world or fully blended. Nothing sets the result: it is the children's results or'ed, so always 0.
char bhkUtilFunctions::GetClosestBodyRec(
    NiAVObject* apObject,
    hkSphere& arSphere,
    ClosestBody& arClosest,
    int32_t abIgnoreHead
) {
    char bres = 0;
    if (!apObject) {
        return bres;
    }
    bhkNiCollisionObject* pNiCollisionObject = apObject->m_spCollisionObject.m_pObject;
    bhkCollisionObject* pc = pNiCollisionObject
        ? static_cast<bhkCollisionObject*>(pNiCollisionObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI))
        : nullptr;
    bhkRigidBody* pb = pc ? pc->spWorldObject : nullptr;
    if (pb) {
        auto* pWorldObject = static_cast<hkpWorldObject*>(pb->phkObject);
        const hkpCollidable* pCollidable = pWorldObject ? &pWorldObject->m_collidable : nullptr;
        uint32_t uiFilterInfo = pCollidable ? pCollidable->m_broadPhaseHandle.m_collisionFilterInfo : 0;
        // EXE-INLINED 0x624070
        uint32_t uiLayer = uiFilterInfo & 0x7F;
        bool bdo = false;
        if (uiLayer == L_BIPED || uiLayer == L_DEADBIP) {
            // EXE-INLINED 0x4D9DA0
            uint32_t uiPart = (uiFilterInfo >> 8) & 0x1F;
            if (uiPart == P_HEAD) {
                bdo = static_cast<uint8_t>(abIgnoreHead) == 0;
            }
            else {
                bdo = uiPart - P_BODY <= static_cast<uint32_t>(P_RFOOT - P_BODY);
            }
        }
        bhkShape* ps = bdo ? pb->GetShape() : nullptr;
        auto* pcs =
            ps ? static_cast<bhkCapsuleShape*>(NiObject::DynamicCastStatic(&bhkCapsuleShape::ms_RTTI, ps)) : nullptr;
        if (pcs) {
            hkTransform hkt;
            hkContactPoint contact;
            pb->GetTransform(hkt);
            if (NiObject::IsKindOf_0(&bhkBlendCollisionObject::ms_RTTI, pc)) {
                auto* pblend = static_cast<bhkBlendCollisionObject*>(pc);
                if (!pb->IsAddedToWorld() || 1.0f == pblend->fHeirGain) {
                    NI2HK(hkt, apObject->m_kWorld);
                    if (NiObject::IsKindOf_0(&bhkRigidBodyT::ms_RTTI, pb)) {
                        auto* prbt = static_cast<bhkRigidBodyT*>(pb);
                        hkTransform hklocal;
                        hkTransform hkpWorld;
                        hklocal.m_rotation = hkt.m_rotation;
                        hklocal.m_translation.m_quad = hkt.m_translation.m_quad;
                        hkpWorld.m_rotation.set(prbt->kLocalRot);
                        hkpWorld.m_translation.m_quad = prbt->kLocalPos.m_quad;
                        hkt.setMul(hklocal, hkpWorld);
                    }
                }
            }
            else {
                pb->GetTransform(hkt);
            }
            if (pcs->GetClosestPoints(arSphere, hkt, contact) &&
                (!arClosest.pBody || contact.m_separatingNormal(3) < arClosest.Contact.m_separatingNormal(3))) {
                __m128 position = contact.m_position.m_quad;
                arClosest.pBody = pb;
                arClosest.Contact.m_position.m_quad = position;
                arClosest.Contact.m_separatingNormal.m_quad = contact.m_separatingNormal.m_quad;
            }
        }
    }
    NiNode* pnode = apObject->IsNode();
    if (pnode) {
        for (uint32_t i = 0; i < pnode->m_kChildren.m_usSize; ++i) {
            bres |= bhkUtilFunctions::GetClosestBodyRec(
                pnode->m_kChildren.m_pBase[i].m_pObject, arSphere, arClosest, abIgnoreHead
            );
        }
    }
    return bres;
}

// Sets the layer bits of the collision filter of apObject's rigid body and of every rigid body below it.
void bhkUtilFunctions::SetLayer(NiAVObject* apObject, COL_LAYER aeLayer) {
    if (apObject) {
        // EXE-INLINED 0x43B610
        bhkNiCollisionObject* pNiCollision = apObject->m_spCollisionObject.m_pObject;
        bhkCollisionObject* pco = pNiCollision
            ? static_cast<bhkCollisionObject*>(pNiCollision->DynamicCastMember(&bhkCollisionObject::ms_RTTI))
            : nullptr;
        if (pco) {
            // EXE-INLINED 0x6FA820
            bhkRigidBody* prb = pco->spWorldObject;
            if (prb) {
                // EXE-INLINED 0x43B4F0
                // EXE-INLINED 0x43B560
                hkpWorldObject* phkObject = static_cast<hkpWorldObject*>(prb->phkObject);
                const hkpCollidable* pCollidable = phkObject ? &phkObject->m_collidable : nullptr;
                CFilter filter;
                filter.iFilter = pCollidable ? pCollidable->getCollisionFilterInfo() : 0;
                // EXE-INLINED 0x4A39F0
                filter.iFilter &= ~0x7Fu;
                filter.iFilter |= static_cast<uint32_t>(aeLayer) & 127;
                prb->SetCollisionFilter(filter);
            }
        }
        NiNode* pnode = apObject->IsNode();
        if (pnode) {
            for (uint32_t i = 0; i < pnode->m_kChildren.m_usSize; ++i) {
                bhkUtilFunctions::SetLayer(pnode->m_kChildren.m_pBase[i].m_pObject, aeLayer);
            }
        }
    }
}

// Walks the action list of apObject's rigid body and of its children's: a limited-hinge, ragdoll or malleable
// constraint without an owner is first added to the world of its entity A, and every such constraint with an owner is
// enabled again; a stiff-spring constraint is taken out of its entity A's world and off the list.
void bhkUtilFunctions::RemoveSpringConstraints(NiAVObject* apObject) {
    if (apObject) {
        bhkNiCollisionObject* pCollisionObject = apObject->m_spCollisionObject.m_pObject;
        if (pCollisionObject) {
            bhkCollisionObject* pco =
                static_cast<bhkCollisionObject*>(pCollisionObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI));
            if (pco) {
                bhkRigidBody* prb = pco->spWorldObject;
                if (prb) {
                    BSSimpleList<NiPointer<bhkSerializable>>* pactCon = &prb->kActionConstrants;
                    if (pactCon) {
                        do {
                            bhkSerializable* pitem = pactCon->m_item;
                            if (!pitem) {
                                break;
                            }
                            // EXE-INLINED 0x6532C0
                            bhkConstraint* pcon =
                                pitem->IsKindOf(&bhkConstraint::ms_RTTI) ? static_cast<bhkConstraint*>(pitem) : nullptr;
                            if (pcon) {
                                // EXE-INLINED 0x887A10
                                hkpConstraintData* pdata;
                                hkpConstraintInstance* pInstance = static_cast<hkpConstraintInstance*>(pcon->phkObject);
                                if (pInstance) {
                                    pdata = pInstance->getDataRw();
                                }
                                else if (pcon->pInfo) {
                                    pdata = static_cast<hkConstraintCinfo*>(pcon->pInfo)->pData;
                                }
                                else {
                                    pdata = nullptr;
                                }
                                switch (pdata->getType()) {
                                    case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE:
                                    case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL:
                                    case hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE: {
                                        if (!static_cast<hkpConstraintInstance*>(pcon->phkObject)->getOwner()) {
                                            hkpEntity* pEntityA = pcon->GethkEntityA();
                                            // prbA is deliberately not tested here, as the game does: no entity A is a
                                            // null call.
                                            bhkRigidBody* prbA = pEntityA
                                                ? static_cast<bhkRigidBody*>(pEntityA->getUserData())
                                                : nullptr;
                                            const hkpWorld* pWorld = prbA->GethkWorld();
                                            // EXE-INLINED 0x621480
                                            bhkWorld* pw = pWorld
                                                ? static_cast<bhkWorld*>(
                                                      static_cast<const ahkpWorld*>(pWorld)->m_userData
                                                  )
                                                : nullptr;
                                            pcon->Add(pw);
                                        }
                                        hkpConstraintInstance* pConInstance =
                                            static_cast<hkpConstraintInstance*>(pcon->phkObject);
                                        if (pConInstance->getOwner()) {
                                            hkpResponseModifier::enableConstraint(
                                                pConInstance, *pConInstance->getOwner()
                                            );
                                        }
                                        break;
                                    }
                                    case hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING: {
                                        hkpEntity* pEntityA = pcon->GethkEntityA();
                                        if (pEntityA) {
                                            bhkRigidBody* prbA = static_cast<bhkRigidBody*>(pEntityA->getUserData());
                                            if (prbA) {
                                                const hkpWorld* pWorld = prbA->GethkWorld();
                                                // EXE-INLINED 0x621480
                                                bhkWorld* pw = pWorld
                                                    ? static_cast<bhkWorld*>(
                                                          static_cast<const ahkpWorld*>(pWorld)->m_userData
                                                      )
                                                    : nullptr;
                                                pw->RemoveBody(pcon);
                                            }
                                        }
                                        NiPointer<bhkSerializable> spcon = pcon;
                                        pactCon->Remove(spcon);
                                        break;
                                    }
                                    default: break;
                                }
                            }
                            pactCon = pactCon->m_pkNext;
                        }
                        while (pactCon);
                    }
                }
            }
        }
        NiNode* pnode = apObject->IsNode();
        if (pnode) {
            for (uint32_t i = 0; i < pnode->m_kChildren.m_usSize; ++i) {
                bhkUtilFunctions::RemoveSpringConstraints(pnode->m_kChildren.m_pBase[i].m_pObject);
            }
        }
    }
}

void bhkUtilFunctions::GatherConstrainedBodies(
    const hkpRigidBody* apBody,
    hkArray<const hkpRigidBody*, hkContainerHeapAllocator>& arBodyA
) {
    const hkpRigidBody* psystemRoot = bhkUtilFunctions::FindConstrainedSystemRoot(apBody);
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (arBodyA.m_size == (arBodyA.m_capacityAndFlags & hkArrayBase<const hkpRigidBody*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, &arBodyA, sizeof(const hkpRigidBody*));
    }
    arBodyA.m_data[arBodyA.m_size++] = psystemRoot;
}

// One step of the breadth-first search for a bhkCollisionObject: apObject's own, else its children (up to
// auiMaxLevel) go into apQueue and the search goes on with the queue's last entry.
bhkCollisionObject* bhkUtilFunctions::FindCollisionObjectBFSRec(
    NiAVObject* apObject,
    BSSimpleArray<bhkUtilFunctions::AVLevel, 1024>& apQueue,
    const uint32_t auiMaxLevel,
    uint32_t auiCurLevel
) {
    // EXE-INLINED 0x43B610
    bhkNiCollisionObject* pNiCollision = apObject->m_spCollisionObject.m_pObject;
    bhkCollisionObject* pcol = pNiCollision
        ? static_cast<bhkCollisionObject*>(pNiCollision->DynamicCastMember(&bhkCollisionObject::ms_RTTI))
        : nullptr;
    if (!pcol && apObject->IsNode() && auiCurLevel < auiMaxLevel) {
        // Reads the children through apObject itself, not through IsNode's result.
        NiNode* pNode = static_cast<NiNode*>(apObject);
        for (uint32_t i = 0; i < pNode->m_kChildren.m_usSize; ++i) {
            NiAVObject* pChild = pNode->m_kChildren.m_pBase[i].m_pObject;
            if (pChild) {
                bhkUtilFunctions::AVLevel avchild;
                avchild.pObj = pChild;
                avchild.uiLevel = auiCurLevel + 1;
                apQueue.SetAt(0, avchild);
            }
        }
    }
    uint32_t iSize = apQueue.iSize;
    if (!iSize || pcol) {
        return pcol;
    }
    bhkUtilFunctions::AVLevel* pBuffer = apQueue.pBuffer;
    NiAVObject* pObj = pBuffer[iSize - 1].pObj;
    uint32_t uiLevel = pBuffer[iSize - 1].uiLevel;
    apQueue.iSize = iSize - 1;
    return bhkUtilFunctions::FindCollisionObjectBFSRec(pObj, apQueue, auiMaxLevel, uiLevel);
}

// The constraint info's constructor sets its fields; this class adds none.
hkStiffSpringConstraintCinfo::hkStiffSpringConstraintCinfo() {}

// A null cinfo reaches Init as a null base, as the pointer conversion gives.
bhkStiffSpringConstraint::bhkStiffSpringConstraint(const hkStiffSpringConstraintCinfo* apInfo) {
    const hkSerializableCinfo* pBaseInfo = apInfo;
    bhkConstraint::Init(*pBaseInfo);
    ++iCount;
}

const NiRTTI* bhkStiffSpringConstraint::GetRTTI() const {
    return &bhkStiffSpringConstraint::ms_RTTI;
}
