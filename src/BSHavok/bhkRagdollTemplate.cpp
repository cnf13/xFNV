#include "BSHavok/bhkRagdollTemplate.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/ahkpWorld.h"
#include "BSHavok/bhkBallAndSocketConstraint.h"
#include "BSHavok/bhkCapsuleShape.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkConstraintManager.h"
#include "BSHavok/bhkHingeConstraint.h"
#include "BSHavok/bhkLimitedHingeConstraint.h"
#include "BSHavok/bhkMalleableConstraint.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRagdollConstraint.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSMain/BSCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

NiRTTI bhkRagdollTemplateData::ms_RTTI("bhkRagdollTemplateData", &NiObject::ms_RTTI);
NiRTTI bhkRagdollTemplate::ms_RTTI("bhkRagdollTemplate", &NiExtraData::ms_RTTI);

bhkRagdollTemplateData::bhkRagdollTemplateData() {}

// Deletes each constraint info; the arrays and the name release themselves.
bhkRagdollTemplateData::~bhkRagdollTemplateData() {
    for (uint32_t i = 0; i < ConstraintsA.iSize; ++i) {
        hkConstraintCinfo* pkCinfo = ConstraintsA.pBuffer[i];
        if (pkCinfo) {
            delete pkCinfo;
        }
    }
}

const NiRTTI* bhkRagdollTemplateData::GetRTTI() const {
    return &bhkRagdollTemplateData::ms_RTTI;
}

// KEEP (niread:): each constraint type is read into a local nothing initialised (eType), so a short read stores its
// garbage into ConstraintTypeA; the count (uiConstraints) is zeroed before its read. The two arrays' SetSize(count,
// true) and the constraint infos' constructors are inlined.
void bhkRagdollTemplateData::LoadBinary(NiStream& arStream) {
    NiObject::LoadBinary(arStream);
    arStream.LoadFixedString(ObjectName);
    uint32_t uiComponentSize = 4;
    arStream.Istr()->BinaryRead(&fRBMass, 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    arStream.Istr()->BinaryRead(&fRBRestitution, 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    arStream.Istr()->BinaryRead(&fRBFriction, 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    arStream.Istr()->BinaryRead(&fRBRadius, 4, &uiComponentSize, 1);
    uiComponentSize = 4;
    arStream.Istr()->BinaryRead(&uiRBMaterial, 4, &uiComponentSize, 1);
    uint32_t uiConstraints = 0;
    uiComponentSize = 4;
    arStream.Istr()->BinaryRead(&uiConstraints, 4, &uiComponentSize, 1);

    ConstraintsA.SetSize(uiConstraints, true);

    ConstraintTypeA.SetSize(uiConstraints, true);

    // One slot for every read: a short read leaves the previous type (or, the first time, garbage) there.
    hkpConstraintData::ConstraintType eType;
    for (uint32_t i = 0; i < uiConstraints; ++i) {
        uiComponentSize = 4;
        arStream.Istr()->BinaryRead(&eType, 4, &uiComponentSize, 1);
        ConstraintTypeA.pBuffer[i] = eType;
    }

    // A new constraint info of each known type (the default case keeps the slot as it is), then its own LoadBinary.
    for (uint32_t i = 0; i < uiConstraints; ++i) {
        switch (ConstraintTypeA.pBuffer[i]) {
            case hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET: {
                hkBallAndSocketConstraintCinfo* pkCinfo = new hkBallAndSocketConstraintCinfo;
                ConstraintsA.pBuffer[i] = pkCinfo;
                break;
            }
            case hkpConstraintData::CONSTRAINT_TYPE_HINGE: {
                hkHingeConstraintCinfo* pkCinfo = new hkHingeConstraintCinfo;
                ConstraintsA.pBuffer[i] = pkCinfo;
                break;
            }
            case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE: {
                hkLimitedHingeConstraintCinfo* pkCinfo = new hkLimitedHingeConstraintCinfo;
                ConstraintsA.pBuffer[i] = pkCinfo;
                break;
            }
            case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL: {
                hkRagdollConstraintCinfo* pkCinfo = new hkRagdollConstraintCinfo;
                ConstraintsA.pBuffer[i] = pkCinfo;
                break;
            }
            case hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE: {
                hkMalleableConstraintCinfo* pkCinfo = new hkMalleableConstraintCinfo;
                ConstraintsA.pBuffer[i] = pkCinfo;
                break;
            }
            default: break;
        }
        ConstraintsA.pBuffer[i]->LoadBinary(arStream);
    }
}

void bhkRagdollTemplateData::LinkObject(NiStream& arStream) {
    NiObject::LinkObject(arStream);
}

// NiObject's part, then the template's name.
bool bhkRagdollTemplateData::RegisterStreamables(NiStream& arStream) {
    if (!NiObject::RegisterStreamables(arStream)) {
        return false;
    }
    arStream.RegisterFixedString(ObjectName);
    return true;
}

// The name, then the rigid body settings, the constraint count and types, then every constraint info.
void bhkRagdollTemplateData::SaveBinary(NiStream& arStream) {
    NiObject::SaveBinary(arStream);

    arStream.SaveFixedString(this->ObjectName);
    uint32_t uiComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&this->fRBMass, 4, &uiComponentSize, 1);
    arStream.m_pkOstr->BinaryWrite(&this->fRBRestitution, 4, &uiComponentSize, 1);
    arStream.m_pkOstr->BinaryWrite(&this->fRBFriction, 4, &uiComponentSize, 1);
    arStream.m_pkOstr->BinaryWrite(&this->fRBRadius, 4, &uiComponentSize, 1);
    arStream.m_pkOstr->BinaryWrite(&this->uiRBMaterial, 4, &uiComponentSize, 1);
    uint32_t uiConstraints = this->ConstraintsA.iSize;
    arStream.m_pkOstr->BinaryWrite(&uiConstraints, 4, &uiComponentSize, 1);
    for (uint32_t i = 0; i < uiConstraints; i++) {
        hkpConstraintData::ConstraintType eType = this->ConstraintTypeA.pBuffer[i];
        arStream.m_pkOstr->BinaryWrite(&eType, 4, &uiComponentSize, 1);
    }
    for (uint32_t i = 0; i < uiConstraints; i++) {
        hkConstraintCinfo* pkCinfo = this->ConstraintsA.pBuffer[i];
        pkCinfo->SaveBinary(arStream);
    }
}

bool bhkRagdollTemplateData::IsEqual(NiObject* apObject) {
    return NiObject::IsEqual(apObject);
}

// Gives apObj's rigid body this template's settings: the mass, friction, restitution and material, and a new capsule
// shape with the old shape's end points and the template's radius times afScale. Then, for each constraint info, the
// entities of the body's first constraint, and a new constraint of the info's type (its data scaled by afScale while it
// is built, cloned unless afScale is 0, then scaled back), which replaces the body's first constraint and is added to
// the body's world.
void bhkRagdollTemplateData::ReplaceRagdollData(NiAVObject* apObj, float afScale) {
    bhkNiCollisionObject* pNiCollision = apObj->m_spCollisionObject.m_pObject;
    if (!pNiCollision) {
        return;
    }
    bhkCollisionObject* pCollision = static_cast<bhkCollisionObject*>(
        pNiCollision->DynamicCastMember(&bhkCollisionObject::ms_RTTI)
    );
    if (!pCollision) {
        return;
    }
    bhkRigidBody* pRigidBody = pCollision->spWorldObject;
    if (!pRigidBody) {
        return;
    }
    hkpRigidBody* pHkBody = static_cast<hkpRigidBody*>(pRigidBody->phkObject);
    if (!pHkBody) {
        return;
    }

    float fMass = fRBMass;
    hkpMotion* pMotion = pHkBody->getStoredDynamicMotion();
    if (pMotion) {
        pMotion->setMass(fMass);
    }
    else {
        hkpRigidBody* pBody = static_cast<hkpRigidBody*>(pRigidBody->phkObject);
        if (pBody) {
            pBody->setMass(fMass);
        }
    }
    hkpRigidBody* pBody = static_cast<hkpRigidBody*>(pRigidBody->phkObject);
    pBody->m_material.m_friction = fRBFriction;
    pBody->m_material.m_restitution = fRBRestitution;

    bhkShape* pShape = pRigidBody->GetShape();
    pShape->eMaterial = static_cast<HavokMaterialType>(uiRBMaterial);

    // The old shape is taken to be a capsule: its end points, or the origin twice when it wraps nothing.
    bhkCapsuleShape* pNewShape = new bhkCapsuleShape(
        pShape->phkObject ? static_cast<const hkpCapsuleShape*>(pShape->phkObject)->m_vertexA : hkVector4Zero,
        pShape->phkObject ? static_cast<const hkpCapsuleShape*>(pShape->phkObject)->m_vertexB : hkVector4Zero,
        fRBRadius * afScale
    );
    // The new shape's reference, taken with no null test.
    NiIncRefCount(pNewShape);
    pNewShape->eMaterial = static_cast<HavokMaterialType>(uiRBMaterial);
    pHkBody->setShape(static_cast<const hkpShape*>(pNewShape->phkObject));
    NiDecRefCount(pShape);

    const hkpWorld* pHkWorld = pRigidBody->GethkWorld();
    bhkWorld* pWorld = nullptr;
    if (pHkWorld) {
        pWorld = static_cast<const ahkpWorld*>(pHkWorld)->m_userData;
    }

    for (uint32_t i = 0; i < ConstraintsA.iSize; ++i) {
        bhkSerializable* pOldConstraint = pRigidBody->kActionConstrants.m_item;
        if (!pOldConstraint) {
            continue;
        }
        hkConstraintCinfo* pOldCinfo = static_cast<bhkConstraint*>(pOldConstraint)->GetCinfo();
        ConstraintsA.pBuffer[i]->pEntityA = pOldCinfo->pEntityA;
        ConstraintsA.pBuffer[i]->pEntityB = pOldCinfo->pEntityB;

        BSCloningProcess kCloning(1.0f);
        bhkConstraint* pConstraint = nullptr;
        switch (ConstraintTypeA.pBuffer[i]) {
            case hkpConstraintData::CONSTRAINT_TYPE_BALLANDSOCKET: {
                hkBallAndSocketConstraintCinfo::ApplyScale(afScale, ConstraintsA.pBuffer[i]->pData);
                pConstraint = new bhkBallAndSocketConstraint(
                    *static_cast<hkBallAndSocketConstraintCinfo*>(ConstraintsA.pBuffer[i])
                );
                if (0.0f != afScale) {
                    hkpConstraintData* pClone = bhkConstraintManager::Clone(ConstraintsA.pBuffer[i]->pData, kCloning);
                    if (pClone) {
                        ConstraintsA.pBuffer[i]->SetData(pClone);
                        pClone->removeReference();
                    }
                }
                float fInvScale = 1.0f / afScale;
                hkBallAndSocketConstraintCinfo::ApplyScale(fInvScale, ConstraintsA.pBuffer[i]->pData);
                break;
            }
            case hkpConstraintData::CONSTRAINT_TYPE_HINGE: {
                hkHingeConstraintCinfo::ApplyScale(afScale, ConstraintsA.pBuffer[i]->pData);
                pConstraint = new bhkHingeConstraint(ConstraintsA.pBuffer[i]);
                if (0.0f != afScale) {
                    hkpConstraintData* pClone = bhkConstraintManager::Clone(ConstraintsA.pBuffer[i]->pData, kCloning);
                    if (pClone) {
                        ConstraintsA.pBuffer[i]->SetData(pClone);
                        pClone->removeReference();
                    }
                }
                float fInvScale = 1.0f / afScale;
                hkHingeConstraintCinfo::ApplyScale(fInvScale, ConstraintsA.pBuffer[i]->pData);
                break;
            }
            case hkpConstraintData::CONSTRAINT_TYPE_LIMITEDHINGE: {
                // A limited hinge's data is scaled with the hinge's ApplyScale.
                hkHingeConstraintCinfo::ApplyScale(afScale, ConstraintsA.pBuffer[i]->pData);
                pConstraint = new bhkLimitedHingeConstraint(
                    *static_cast<hkLimitedHingeConstraintCinfo*>(ConstraintsA.pBuffer[i])
                );
                if (0.0f != afScale) {
                    hkpConstraintData* pClone = bhkConstraintManager::Clone(ConstraintsA.pBuffer[i]->pData, kCloning);
                    if (pClone) {
                        ConstraintsA.pBuffer[i]->SetData(pClone);
                        pClone->removeReference();
                    }
                }
                float fInvScale = 1.0f / afScale;
                hkHingeConstraintCinfo::ApplyScale(fInvScale, ConstraintsA.pBuffer[i]->pData);
                break;
            }
            case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL: {
                // A ragdoll constraint's data is scaled with the hinge's ApplyScale too.
                hkHingeConstraintCinfo::ApplyScale(afScale, ConstraintsA.pBuffer[i]->pData);
                pConstraint = new bhkRagdollConstraint(
                    *static_cast<hkRagdollConstraintCinfo*>(ConstraintsA.pBuffer[i])
                );
                if (0.0f != afScale) {
                    hkpConstraintData* pClone = bhkConstraintManager::Clone(ConstraintsA.pBuffer[i]->pData, kCloning);
                    if (pClone) {
                        ConstraintsA.pBuffer[i]->SetData(pClone);
                        pClone->removeReference();
                    }
                }
                float fInvScale = 1.0f / afScale;
                hkHingeConstraintCinfo::ApplyScale(fInvScale, ConstraintsA.pBuffer[i]->pData);
                break;
            }
            case hkpConstraintData::CONSTRAINT_TYPE_MALLEABLE: {
                hkMalleableConstraintCinfo::ApplyScale(afScale, ConstraintsA.pBuffer[i]->pData);
                pConstraint = new bhkMalleableConstraint(
                    *static_cast<hkMalleableConstraintCinfo*>(ConstraintsA.pBuffer[i])
                );
                if (0.0f != afScale) {
                    hkpConstraintData* pClone = bhkConstraintManager::Clone(ConstraintsA.pBuffer[i]->pData, kCloning);
                    if (pClone) {
                        ConstraintsA.pBuffer[i]->SetData(pClone);
                        pClone->removeReference();
                    }
                }
                float fInvScale = 1.0f / afScale;
                hkMalleableConstraintCinfo::ApplyScale(fInvScale, ConstraintsA.pBuffer[i]->pData);
                break;
            }
            default: break;
        }

        // A local NiPointer holds the new constraint while it replaces the body's first one (made only for a
        // constraint); the constraint is then added with no null test.
        if (pConstraint) {
            NiIncRefCount(pConstraint);
        }
        if (pConstraint) {
            bhkSerializable* pFirst = pRigidBody->kActionConstrants.m_item;
            if (pFirst != pConstraint) {
                if (pFirst) {
                    NiDecRefCount(pFirst);
                }
                pRigidBody->kActionConstrants.m_item.m_pObject = pConstraint;
                NiIncRefCount(pConstraint);
            }
        }
        if (pConstraint) {
            NiDecRefCount(pConstraint);
        }
        pConstraint->Add(pWorld);
    }
}

const NiFixedString& bhkRagdollTemplateData::GetName() const {
    return ObjectName;
}

NiObject* bhkRagdollTemplateData::CreateObject() {
    return new bhkRagdollTemplateData;
}

bhkRagdollTemplate::bhkRagdollTemplate() {}

// The template array releases its templates itself.
bhkRagdollTemplate::~bhkRagdollTemplate() {}

const NiRTTI* bhkRagdollTemplate::GetRTTI() const {
    return &bhkRagdollTemplate::ms_RTTI;
}

// The template count, then one link ID per template.
void bhkRagdollTemplate::LoadBinary(NiStream& arStream) {
    NiExtraData::LoadBinary(arStream);

    uint32_t uiTemplates = 0;
    uint32_t uiComponentSize = 4;
    arStream.m_pkIstr->BinaryRead(&uiTemplates, 4, &uiComponentSize, 1);
    this->RagdollTemplateA.SetSize(uiTemplates, true);
    for (uint32_t i = 0; i < uiTemplates; i++) {
        arStream.ReadLinkID();
    }
}

void bhkRagdollTemplate::LinkObject(NiStream& arStream) {
    NiExtraData::LinkObject(arStream);
    for (uint32_t i = 0; i < RagdollTemplateA.iSize; ++i) {
        NiPointer<bhkRagdollTemplateData>* v3 = &RagdollTemplateA.pBuffer[i];
        bhkRagdollTemplateData* ObjectFromLinkID = static_cast<bhkRagdollTemplateData*>(arStream.GetObjectFromLinkID());
        bhkRagdollTemplateData* m_pObject = v3->m_pObject;
        bhkRagdollTemplateData* v6 = ObjectFromLinkID;
        if (v3->m_pObject != ObjectFromLinkID) {
            if (m_pObject) {
                NiDecRefCount(m_pObject);
            }
            v3->m_pObject = v6;
            if (v6) {
                NiIncRefCount(v6);
            }
        }
    }
}

// The base's part, then each template's own. Calls the base directly, not through the vtable.
bool bhkRagdollTemplate::RegisterStreamables(NiStream& arStream) {
    if (!NiExtraData::RegisterStreamables(arStream)) {
        return false;
    }
    uint32_t uiTemplates = this->RagdollTemplateA.iSize;
    for (uint32_t i = 0; i < uiTemplates; i++) {
        bhkRagdollTemplateData* pkTemplate = this->RagdollTemplateA.pBuffer[i].m_pObject;
        pkTemplate->RegisterStreamables(arStream);
    }
    return true;
}

// The template count, then one link ID per template.
void bhkRagdollTemplate::SaveBinary(NiStream& arStream) {
    NiExtraData::SaveBinary(arStream);

    uint32_t uiTemplates = this->RagdollTemplateA.iSize;
    uint32_t uiComponentSize = 4;
    arStream.m_pkOstr->BinaryWrite(&uiTemplates, 4, &uiComponentSize, 1);
    for (uint32_t i = 0; i < uiTemplates; i++) {
        arStream.SaveLinkID(this->RagdollTemplateA.pBuffer[i].m_pObject);
    }
}

bool bhkRagdollTemplate::IsEqual(NiObject* apObject) {
    return NiExtraData::IsEqual(apObject);
}

void bhkRagdollTemplate::CopyMembers(bhkRagdollTemplate* pDest, NiCloningProcess& arCloneData) {
    NiExtraData::CopyMembers(pDest, arCloneData);
}

uint32_t bhkRagdollTemplate::QNumTemplates() {
    return RagdollTemplateA.iSize;
}

// Names the object the empty string unconditionally, a failed allocation included.
NiObject* bhkRagdollTemplate::CreateObject() {
    bhkRagdollTemplate* pObject = new bhkRagdollTemplate;
    pObject->SetName(EmptyFS);
    return pObject;
}

const NiRTTI* bhkBallAndSocketConstraint::GetRTTI() const {
    return &bhkBallAndSocketConstraint::ms_RTTI;
}

// hkConstraintCinfo's constructor clears the data and entities and sets the PSI priority; this class adds nothing.
hkBallAndSocketConstraintCinfo::hkBallAndSocketConstraintCinfo() : hkConstraintCinfo() {}

// The info goes to Init as its hkSerializableCinfo base.
bhkConstraint::bhkConstraint(const hkConstraintCinfo& arInfo) {
    bhkConstraint::Init(arInfo);
    ++bhkConstraint::iCount;
}

// CreateCinfo's hkSerializableCinfo base, back to its hkConstraintCinfo (four bytes before it), or null. The created
// flag CreateCinfo fills is not read.
hkConstraintCinfo* bhkConstraint::GetCinfo() {
    bool bCreated;
    hkSerializableCinfo* pkCinfo = &CreateCinfo(bCreated);
    return static_cast<hkConstraintCinfo*>(pkCinfo);
}

// hkConstraintCinfo's constructor sets no data, PSI priority and no entities.
hkRagdollConstraintCinfo::hkRagdollConstraintCinfo() {}

bhkRagdollConstraint::bhkRagdollConstraint(const hkRagdollConstraintCinfo& arInfo) {
    bhkConstraint::Init(arInfo);
    ++iCount;
}
