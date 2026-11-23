#include "BSHavok/bhkConstraint.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/ahkpWorld.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkGenericConstraint.h"
#include "BSHavok/bhkHingeLimitsConstraint.h"
#include "BSHavok/bhkMalleableConstraint.h"
#include "BSHavok/bhkPositionConstraintMotor.h"
#include "BSHavok/bhkPrismaticConstraint.h"
#include "BSHavok/bhkRagdollLimitsConstraint.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkSpringDamperConstraintMotor.h"
#include "BSHavok/bhkStiffSpringConstraint.h"
#include "BSHavok/bhkVelocityConstraintMotor.h"
#include "BSHavok/bhkWheelConstraint.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

NiRTTI bhkConstraint::ms_RTTI("bhkConstraint", &bhkSerializable::ms_RTTI);

bhkConstraint::bhkConstraint() {
    ++bhkConstraint::iCount;
}

bhkConstraint::~bhkConstraint() {
    bhkSerializable::Kill();
    --bhkConstraint::iCount;
}

void bhkConstraint::LoadBinary(NiStream& arStream) {
    bhkSerializable::LoadBinary(arStream);
}

// Takes the two link IDs the loader queued, binds their wrappers' Havok entities into the creation info's entity words
// and links the base. The creation info keeps its tag base after the Havok cinfo, so the returned pointer is re-based 4
// bytes back, null-checked (a pointer cast: a cast of `&ref` would drop the test).
void bhkConstraint::LinkObject(NiStream& arStream) {
    arStream.GetNumberOfLinkIDs();
    NiObject* pkObjectA = arStream.GetObjectFromLinkID();
    NiObject* pkObjectB = arStream.GetObjectFromLinkID();
    bool bCreated;
    hkSerializableCinfo* pkInfo = &CreateCinfo(bCreated);
    hkConstraintCinfo* pkCinfo = static_cast<hkConstraintCinfo*>(pkInfo);
    if (pkObjectA) {
        pkCinfo->pEntityA = static_cast<hkpEntity*>(static_cast<bhkEntity*>(pkObjectA)->phkObject);
        if (pkObjectB) {
            pkCinfo->pEntityB = static_cast<hkpEntity*>(static_cast<bhkEntity*>(pkObjectB)->phkObject);
            bhkSerializable::LinkObject(arStream);
            return;
        }
        pkCinfo->pEntityB = nullptr;
    }
    bhkSerializable::LinkObject(arStream);
}

bool bhkConstraint::RegisterStreamables(NiStream& arStream) {
    hkpEntity* v3 = GethkEntityB();
    if (v3) {
        bhkEntity* pent = static_cast<bhkEntity*>(v3->m_userData);
        if (pent) {
            pent->RegisterStreamables(arStream);
        }
    }
    return bhkSerializable::RegisterStreamables(arStream);
}

void bhkConstraint::SaveBinary(NiStream& arStream) {
    // The creation info is rebuilt from the wrapped Havok object and saved through its own virtual. The cast goes
    // through a pointer so that it stays null-checked (a cast of `&ref` would drop the test).
    bool bCreated;
    hkSerializableCinfo* pkInfo = &CreateCinfo(bCreated);
    hkConstraintCinfo* pkCinfo = static_cast<hkConstraintCinfo*>(pkInfo);
    pkCinfo->SaveBinary(arStream);
}

// The base's comparison, then the two entities: the other object is asked for each entity before this one, and the two
// results are combined with a bitwise and, so both getters always run.
bool bhkConstraint::IsEqual(NiObject* apObject) {
    if (!NiObject::IsEqual(apObject)) {
        return false;
    }
    bhkConstraint* pkOther = static_cast<bhkConstraint*>(apObject);
    hkpEntity* pkOtherA = pkOther->GethkEntityA();
    const bool bSameA = GethkEntityA() == pkOtherA;
    hkpEntity* pkOtherB = pkOther->GethkEntityB();
    const bool bSameB = GethkEntityB() == pkOtherB;
    return (bSameA & bSameB) != 0;
}

// The base's strings, then the creation info's, rebuilt for the purpose and dropped again. The cinfo pointer is
// re-based as LoadCInfo does, null-checked (a pointer cast: a cast of `&ref` would drop the test).
void bhkConstraint::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    bhkSerializable::GetViewerStrings(apStrings);
    bool bCreated;
    hkSerializableCinfo* pkInfo = &CreateCinfo(bCreated);
    hkConstraintCinfo* pkCinfo = static_cast<hkConstraintCinfo*>(pkInfo);
    pkCinfo->GetViewerStrings(apStrings);
    KillCinfo(true);
}

void bhkConstraint::SethkObject(hkReferencedObject* aphkObject) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    // Null-checks this before reading the field.
    hkpConstraintInstance* pkConstraint;
    if (this) {
        pkConstraint = static_cast<hkpConstraintInstance*>(phkObject);
    }
    else {
        pkConstraint = nullptr;
    }
    if (static_cast<const void*>(aphkObject) != static_cast<const void*>(pkConstraint)) {
        // The constraint being replaced leaves its owner, with the world's critical operations locked.
        if (pkConstraint) {
            hkpConstraintOwner* pkOwner = pkConstraint->m_owner;
            if (pkOwner) {
                hkpWorld* pkWorld = pkConstraint->m_entities[0]->m_world;
                if (pkWorld) {
                    pkWorld->lockCriticalOperations();
                }
                pkOwner->removeConstraintFromCriticalLockedIsland(pkConstraint);
                if (pkWorld) {
                    pkWorld->unlockCriticalOperations();
                }
            }
        }
        bhkRefObject::SethkObject(aphkObject);
    }
}

// While attached, the wrapped constraint instance's user data points back to this wrapper; detached, it is cleared.
void bhkConstraint::Link(bool abAttach) {
    if (this) {
        hkpConstraintInstance* pkConstraint = static_cast<hkpConstraintInstance*>(phkObject);
        if (pkConstraint) {
            // The SDK's user data is an hkUlong: the instance keeps its wrapper's address as an integer.
            pkConstraint->m_userData = abAttach ? reinterpret_cast<hkUlong>(this) : 0;
        }
    }
    bhkRefObject::Link(abAttach);
}

const hkpWorld* bhkConstraint::GethkWorld() const {
    hkpEntity* pe;
    hkpRigidBody* phkObject = static_cast<hkpRigidBody*>(this->phkObject);
    if (phkObject && (phkObject->m_world && (pe = GethkEntityA()) != nullptr)) {
        return pe->m_world;
    }

    return nullptr;
}

// Same as GethkWorld, through the virtual.
hkpWorld* bhkConstraint::GethkWorldRw() {
    return const_cast<hkpWorld*>(GethkWorld());
}

// Moves the constraint into apWorld: false when it has no instance or already is in that world's Havok world. A
// null world only takes it out of its current one.
bool bhkConstraint::Add(bhkWorld* apWorld) {
    hkpConstraintInstance* pc;
    if (this) {
        pc = static_cast<hkpConstraintInstance*>(phkObject);
    }
    else {
        pc = nullptr;
    }
    if (!pc) {
        return false;
    }
    if (!apWorld) {
        Remove();
        return false;
    }
    const hkpWorld* pkCurrent = GethkWorld();
    if (pkCurrent == apWorld->GethkWorld()) {
        return false;
    }
    Remove();
    ProcessEntityB(apWorld, true);
    apWorld->Add(pc);
    return true;
}

// Takes the constraint out of its world's wrapper (the Havok world's user data), with the Havok memory context.
bool bhkConstraint::Remove() {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkWorld* pbw = nullptr;
    const hkpWorld* v5 = GethkWorld();
    if (v5) {
        pbw = static_cast<const ahkpWorld*>(v5)->m_userData;
    }
    if (pbw) {
        // RemoveConstraint returns 0 or 1, which is returned as it is.
        bool bres = pbw->RemoveConstraint(static_cast<hkpConstraintInstance*>(phkObject)) != 0;
        ProcessEntityB(pbw, false);
        return bres;
    }

    return false;
}

// Loads the creation info through its own virtual, clears its constraint data's back pointer and returns the cinfo's
// size. The creation info keeps its tag base after the Havok cinfo, so the returned pointer is re-based with a
// null-checked pointer cast (a cast of `&ref` would drop the test).
int32_t bhkConstraint::LoadCInfo(NiStream& arStream) {
    bool bCreated;
    hkSerializableCinfo* pkInfo = &CreateCinfo(bCreated);
    hkConstraintCinfo* pkCinfo = static_cast<hkConstraintCinfo*>(pkInfo);
    pkCinfo->LoadBinary(arStream);
    pkCinfo->pData->m_userData = 0;
    return QCinfoSize();
}

// The constraint instance is built from the creation info's entities, data and priority, and stored through SethkObject
// (which takes the reference, so the local one is dropped).
void bhkConstraint::Init(const hkSerializableCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    // The creation info's empty tag base sits at the cinfo's own address, so the argument is not re-based before
    // reading the cinfo.
    hkConstraintCinfo* pkCinfo = static_cast<hkConstraintCinfo*>(const_cast<hkSerializableCinfo*>(&arInfo));
    hkpConstraintInstance*
        pkCreated = new hkpConstraintInstance(pkCinfo->pEntityA, pkCinfo->pEntityB, pkCinfo->pData, pkCinfo->ePriority);
    SethkObject(pkCreated);
    pkCreated->removeReference();
    PostInit(arInfo);
}

// Null-checks this and the wrapped instance, then calls the instance's own entity setter.
void bhkConstraint::SetEntityA(hkpEntity* apEntity) {
    if (this && phkObject) {
        static_cast<ahkpConstraintInstance*>(phkObject)->setEntityA(apEntity);
    }
}

void bhkConstraint::SetEntityB(hkpEntity* apEntity) {
    if (this && phkObject) {
        static_cast<ahkpConstraintInstance*>(phkObject)->setEntityB(apEntity);
    }
}

// The wrapped constraint instance's first entity, null without one.
hkpEntity* bhkConstraint::GethkEntityA() const {
    hkReferencedObject* phkObject = this->phkObject;
    hkpEntity* pent = nullptr;
    if (phkObject) {
        return static_cast<hkpConstraintInstance*>(phkObject)->getEntityA();
    }
    return pent;
}

// The wrapped constraint instance's second entity, null without one.
hkpEntity* bhkConstraint::GethkEntityB() const {
    hkReferencedObject* phkObject = this->phkObject;
    hkpEntity* pent = nullptr;
    if (phkObject) {
        return static_cast<hkpConstraintInstance*>(phkObject)->getEntityB();
    }
    return pent;
}

bool bhkConstraint::ProcessEntityB(bhkWorld* apWorld, bool abAdd) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bool bResult = false;
    hkpEntity* pEntityB = GethkEntityB();
    if (!abAdd) {
        // The entity is the world's fixed body: unhook it.
        const hkpWorld* pWorld = apWorld->GethkWorld();
        hkpRigidBody* pFixed = nullptr;
        if (pWorld) {
            pFixed = pWorld->m_fixedRigidBody;
        }
        if (pEntityB == pFixed) {
            SetEntityB(nullptr);
        }
    }
    else {
        bhkEntity* pe;
        if (!pEntityB || (pe = static_cast<bhkEntity*>(pEntityB->getUserData())) == nullptr) {
            // No wrapper: attach the constraint to the world's fixed body.
            bResult = true;
            const hkpWorld* pWorld = apWorld->GethkWorld();
            hkpRigidBody* pFixed = nullptr;
            if (pWorld) {
                pFixed = pWorld->m_fixedRigidBody;
            }
            SetEntityB(pFixed);
        }
        else {
            hkpWorldObject* pObject = static_cast<hkpWorldObject*>(pe->phkObject);
            bool bInWorld = false;
            if (pObject) {
                pe->MarkRead();
                bInWorld = pObject->isAddedToWorld();
                pe->UnMarkRead();
            }
            if (!bInWorld) {
                // Take entity A's collision group (the filter info's upper half).
                pEntityB->m_collidable.m_broadPhaseHandle
                    .m_collisionFilterInfo = static_cast<uint16_t>(
                                                 pEntityB->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo
                                             ) |
                    (GethkEntityA()->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo & 0xFFFF0000u);
            }
            if (!apWorld->iBatchAdd) {
                pe->Add(apWorld);
                if (!pe->GethkWorld() &&
                    (pe->iFlags & (bhkWorldObject::BATCH_ADD | bhkWorldObject::BATCH_REMOVE)) == 0) {
                    char acBuffer[512];
                    {
                        hkOstream kStream(acBuffer, 512, true);
                        kStream << "A Constraint is being added to the world before its target bodies have been added. "
                                   "Did an bad actor skeleton just switch from keyframed?\n";
                        hkError::s_instance->message(
                            hkError::MESSAGE_WARNING,
                            0x234F24FC,
                            acBuffer,
                            "D:\\_Fallout3\\Platforms\\Common\\Code\\BSHavok\\bhkConstraint.cpp",
                            749
                        );
                    }
                    pe->ForceAdd(apWorld);
                }
            }
        }
    }
    return bResult;
}

// Copies the wrapped constraint instance's data, entities and priority into apInfo. Without an instance the data
// and both entities are cleared and the priority is left as it is.
void bhkConstraint::CopyCinfo(hkConstraintCinfo& arInfo) {
    hkpConstraintInstance* pkConstraint;
    if (this && (pkConstraint = static_cast<hkpConstraintInstance*>(phkObject)) != nullptr) {
        arInfo.SetData(pkConstraint->m_data);
        arInfo.pEntityA = pkConstraint->m_entities[0];
        arInfo.pEntityB = pkConstraint->m_entities[1];
        arInfo.ePriority = static_cast<hkpConstraintInstance::ConstraintPriority>(pkConstraint->m_priority.m_storage);
    }
    else {
        arInfo.SetData(nullptr);
        arInfo.pEntityA = nullptr;
        arInfo.pEntityB = nullptr;
    }
}

// Fills the clone's creation info from this one's, then the base copies its members. A clone at a scale other than 1
// gets its constraint data cloned (the clone process is always a BSCloningProcess; it reads Scale.x). The entities are
// remapped to their clones: entity A only when its clone exists already, entity B (looked at only when entity A is set)
// cloned here when it has none yet. The cinfo pointer is re-based as LoadCInfo does, null-checked (a pointer cast: a
// cast of `&ref` would drop the test).
void bhkConstraint::CopyCloneMembers(bhkSerializable* apClone, NiCloningProcess& arCloning) {
    bool bCreated;
    hkSerializableCinfo* pkInfo = &CreateCinfo(bCreated);
    hkConstraintCinfo* pkCinfo = static_cast<hkConstraintCinfo*>(pkInfo);
    if (1.0f != static_cast<BSCloningProcess&>(arCloning).Scale.x) {
        pkCinfo->CloneData(nullptr, arCloning);
    }
    hkpEntity* pEntityA = pkCinfo->pEntityA;
    bhkEntity* pa = pEntityA ? static_cast<bhkEntity*>(pEntityA->m_userData) : nullptr;
    hkpEntity* pEntityB = pkCinfo->pEntityB;
    bhkEntity* pb = pEntityB ? static_cast<bhkEntity*>(pEntityB->m_userData) : nullptr;
    pkCinfo->pEntityA = nullptr;
    pkCinfo->pEntityB = nullptr;
    if (pa) {
        NiObject* pkCloneA;
        if (arCloning.m_pkCloneMap->GetAt(pa, pkCloneA)) {
            pkCinfo->pEntityA = pkCloneA ? static_cast<hkpEntity*>(static_cast<bhkEntity*>(pkCloneA)->phkObject)
                                         : nullptr;
        }
        if (pb) {
            NiObject* pkCloneB;
            if (!arCloning.m_pkCloneMap->GetAt(pb, pkCloneB)) {
                pkCloneB = pb->CreateClone(arCloning);
            }
            pkCinfo->pEntityB = pkCloneB ? static_cast<hkpEntity*>(static_cast<bhkEntity*>(pkCloneB)->phkObject)
                                         : nullptr;
        }
    }
    bhkSerializable::CopyMembers(apClone, arCloning);
}

bhkEntity* bhkConstraint::GetEntityB() {
    hkpRigidBody* v1 = static_cast<hkpRigidBody*>(GethkEntityB());
    return bhkEntity::Getbhk(v1);
}

hkpConstraintInstance* bhkConstraint::operator*() {
    if (this) {
        return static_cast<hkpConstraintInstance*>(GethkObject());
    }

    return nullptr;
}

// The new entity is referenced before the old one is released, under the Havok memory context.
void ahkpConstraintInstance::setEntityA(hkpEntity* apEntity) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (apEntity) {
        apEntity->addReference();
    }
    if (m_entities[0]) {
        m_entities[0]->removeReference();
    }
    m_entities[0] = apEntity;
}

// As setEntityA, for the second entity.
void ahkpConstraintInstance::setEntityB(hkpEntity* apEntity) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (apEntity) {
        apEntity->addReference();
    }
    if (m_entities[1]) {
        m_entities[1]->removeReference();
    }
    m_entities[1] = apEntity;
}

void hkConstraintCinfo::CloneData(hkpConstraintData* apData, NiCloningProcess&) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    SetData(apData);
    apData->removeReference();
}

void hkConstraintCinfo::LoadBinary(NiStream& arStream) {
    arStream.ReadMultipleLinkIDs();
    CreateData();
    NiStreamLoadBinary(arStream, ePriority);
}

void hkConstraintCinfo::SaveBinary(NiStream& arStream) {
    // The loader reads a link ID count and that many link IDs back (ReadMultipleLinkIDs): the cinfo always stores both
    // entities, so the constant 2 is written.
    uint32_t uiNumLinks = 2;
    NiStreamSaveBinary(arStream, uiNumLinks);
    // The entities are saved as their game objects, the Havok objects' user data.
    arStream.SaveLinkID(pEntityA ? pEntityA->m_userData : nullptr);
    arStream.SaveLinkID(pEntityB ? pEntityB->m_userData : nullptr);
    CreateData();
    NiStreamSaveBinary(arStream, ePriority);
}

void hkConstraintCinfo::SetData(hkpConstraintData* apData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    hkpConstraintData* pData = this->pData;
    if (pData) {
        pData->m_userData = 0;
        this->pData->removeReference();
    }
    if (apData) {
        apData->addReference();
        // The SDK's user data is an hkUlong: the constraint keeps its cinfo's address as an integer.
        apData->m_userData = reinterpret_cast<uintptr_t>(this);
    }
    this->pData = apData;
}

hkpConstraintMotor* hkConstraintCinfo::LoadMotor(NiStream& arStream) {
    hkpConstraintMotor* pmotor;
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    NiBinaryStream* m_pkIstr = arStream.Istr();
    int8_t cType = 0;
    NiBinaryStream::NIBINARYSTREAMREADFN m_pfnRead = m_pkIstr->m_pfnRead;
    uint32_t uiComponentSize = 1;
    m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &cType, 1, &uiComponentSize, 1);
    switch (cType) {
        case 1: {
            bhkPositionConstraintMotor* pposMotor = new bhkPositionConstraintMotor;
            pposMotor->LoadBinary(arStream);
            pmotor = pposMotor;
            break;
        }
        case 2: {
            bhkVelocityConstraintMotor* pvelMotor = new bhkVelocityConstraintMotor;
            pvelMotor->LoadBinary(arStream);
            pmotor = pvelMotor;
            break;
        }
        case 3: {
            bhkSpringDamperConstraintMotor* psdMotor = new bhkSpringDamperConstraintMotor;
            psdMotor->LoadBinary(arStream);
            pmotor = psdMotor;
            break;
        }
        default: {
            pmotor = nullptr;
            break;
        }
    }
    return pmotor;
}

// The motor's type byte, then the motor's own data through its game class's SaveBinary; a null motor is saved as type 0
// alone.
void hkConstraintCinfo::SaveMotor(hkpConstraintMotor* apMotor, NiStream& arStream) {
    uint32_t uiComponentSize;
    if (apMotor) {
        uiComponentSize = 1;
        arStream.Ostr()->BinaryWrite(&apMotor->m_type, 1, &uiComponentSize, 1);
        switch (apMotor->m_type.m_storage) {
            case hkpConstraintMotor::TYPE_POSITION:
                static_cast<bhkPositionConstraintMotor*>(apMotor)->SaveBinary(arStream);
                break;
            case hkpConstraintMotor::TYPE_VELOCITY:
                static_cast<bhkVelocityConstraintMotor*>(apMotor)->SaveBinary(arStream);
                break;
            case hkpConstraintMotor::TYPE_SPRING_DAMPER:
                static_cast<bhkSpringDamperConstraintMotor*>(apMotor)->SaveBinary(arStream);
                break;
        }
    }
    else {
        uint8_t ucType = 0;
        uiComponentSize = 1;
        arStream.Ostr()->BinaryWrite(&ucType, 1, &uiComponentSize, 1);
    }
}
