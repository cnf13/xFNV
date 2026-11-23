#include "BSHavok/bhkEntityListener.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkRigidBody.h"
#include "Fallout Shared/BGSDecalNode.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"

bhkEntityListener::bhkEntityListener(uint32_t auiFlag) : iFlag(auiFlag), bEnabled(true) {}

bhkEntityListener::~bhkEntityListener() {}

void bhkEntityListener::entityAddedCallback(hkpEntity* apEntity) {
    bhkRigidBody* v3 = GetRB(apEntity);
    AddEntity(v3);
}

void bhkEntityListener::entityRemovedCallback(hkpEntity* apEntity) {
    bhkRigidBody* v3 = GetRB(apEntity);
    RemoveEntity(v3);
}

// The rigid body of apEntity when its flags share a bit with the listener's iFlag, else null.
bhkRigidBody* bhkEntityListener::GetRB(hkpEntity* apEntity) {
    if (!apEntity) {
        return nullptr;
    }
    bhkWorldObject* pUserData = apEntity->m_userData;
    if (!pUserData) {
        return nullptr;
    }
    bhkRigidBody* pRigidBody = static_cast<bhkRigidBody*>(pUserData->DynamicCastMember(&bhkRigidBody::ms_RTTI));
    if (!pRigidBody || !(pRigidBody->iFlags & iFlag)) {
        return nullptr;
    }
    return pRigidBody;
}

// Charges the list's node to the Havok heap context, then puts apEntity at the head of the list. A temporary NiPointer
// is built from apEntity for the item (taking the reference) and destroyed after the call (releasing it, and deleting
// the entity when that was the last reference).
void bhkEntityListener::AddEntity(bhkEntity* apEntity) {
    AutoMemContext memContext(MEM_CONTEXT_HAVOK);
    if (apEntity) {
        Entities.AddHead(apEntity);
    }
}

// Charges the removal to the Havok heap context and drops apEntity from the list through the same temporary NiPointer
// the add builds. Skips an entity whose reference count is already zero (an unsigned test against 0).
void bhkEntityListener::RemoveEntity(bhkEntity* apEntity) {
    AutoMemContext memContext(MEM_CONTEXT_HAVOK);
    if (apEntity && apEntity->m_uiRefCount) {
        Entities.Remove(apEntity);
    }
}

void bhkEntityListener::SetEnabled(bool abTF) {
    bEnabled = abTF;
}
