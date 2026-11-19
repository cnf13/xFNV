#include "BSHavok/bhkBinaryAction.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkAction.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkWorld.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpBinaryAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

NiRTTI bhkBinaryAction::ms_RTTI("bhkBinaryAction", &bhkAction::ms_RTTI);

// Skips the two entities' link IDs (LinkObject resolves them), loads the serializable part, then clears the two bodies
// of the creation info.
void bhkBinaryAction::LoadBinary(NiStream& arStream) {
    arStream.ReadLinkID();
    arStream.ReadLinkID();
    bhkSerializable::LoadBinary(arStream);
    bool bCreated;
    hkBinaryActionCinfo* pkCinfo = &static_cast<hkBinaryActionCinfo&>(CreateCinfo(bCreated));
    pkCinfo->m_bodyA = nullptr;
    pkCinfo->m_bodyB = nullptr;
}

void bhkBinaryAction::LinkObject(NiStream& arStream) {
    NiStream* pStream = &arStream;
    NiStream* v2 = pStream;
    bhkRigidBody* pba = static_cast<bhkRigidBody*>(pStream->GetObjectFromLinkID());
    bhkRigidBody* pbb = static_cast<bhkRigidBody*>(v2->GetObjectFromLinkID());
    bool bCreated;
    hkBinaryActionCinfo* cinfo = static_cast<hkBinaryActionCinfo*>(&CreateCinfo(bCreated));
    if (pba) {
        cinfo->m_bodyA = static_cast<hkpRigidBody*>(pba->phkObject);
        if (pbb) {
            cinfo->m_bodyB = static_cast<hkpRigidBody*>(pbb->phkObject);
            bhkSerializable::LinkObject(*v2);
            return;
        }
        cinfo->m_bodyB = nullptr;
    }
    bhkSerializable::LinkObject(*v2);
}

// Registers only entity B's wrapper, as the game does, then the base's streamables.
bool bhkBinaryAction::RegisterStreamables(NiStream& arStream) {
    hkpEntity* pEntityB = (this && phkObject) ? static_cast<hkpBinaryAction*>(phkObject)->m_entityB : nullptr;
    if (pEntityB) {
        bhkRigidBody* pBodyB = static_cast<bhkRigidBody*>(pEntityB->m_userData);
        if (pBodyB) {
            pBodyB->RegisterStreamables(arStream);
        }
    }
    return bhkSerializable::RegisterStreamables(arStream);
}

// Saves the link IDs of the two wrapped entities' bhk wrappers (the user data each hkpWorldObject carries), then the
// serializable part. Tests this and phkObject for null before each entity, reloads phkObject after the first
// SaveLinkID, and saves a null link for an entity that is not there.
void bhkBinaryAction::SaveBinary(NiStream& arStream) {
    hkpEntity* pkEntityA = nullptr;
    if (this != nullptr && phkObject != nullptr) {
        pkEntityA = static_cast<hkpBinaryAction*>(phkObject)->m_entityA;
    }
    arStream.SaveLinkID(pkEntityA != nullptr ? pkEntityA->m_userData : nullptr);
    hkpEntity* pkEntityB = nullptr;
    if (this != nullptr && phkObject != nullptr) {
        pkEntityB = static_cast<hkpBinaryAction*>(phkObject)->m_entityB;
    }
    arStream.SaveLinkID(pkEntityB != nullptr ? pkEntityB->m_userData : nullptr);
    bhkSerializable::SaveBinary(arStream);
}

// The base's test, then the same two Havok entities on both actions (null for a missing action). Both entity
// tests run before the results are combined.
bool bhkBinaryAction::IsEqual(NiObject* apObject) {
    bool bEqual = bhkSerializable::IsEqual(apObject);
    if (!bEqual) {
        return bEqual;
    }
    bhkBinaryAction* pOther = static_cast<bhkBinaryAction*>(apObject);
    hkpEntity* pEntityA = (this && phkObject) ? static_cast<hkpBinaryAction*>(phkObject)->m_entityA : nullptr;
    hkpEntity* pOtherA =
        (pOther && pOther->phkObject) ? static_cast<hkpBinaryAction*>(pOther->phkObject)->m_entityA : nullptr;
    bool bEqualA = pEntityA == pOtherA;
    hkpEntity* pEntityB = (this && phkObject) ? static_cast<hkpBinaryAction*>(phkObject)->m_entityB : nullptr;
    hkpEntity* pOtherB =
        (pOther && pOther->phkObject) ? static_cast<hkpBinaryAction*>(pOther->phkObject)->m_entityB : nullptr;
    bool bEqualB = pEntityB == pOtherB;
    return bEqualA & bEqualB;
}

// The base's lines, then the class's RTTI name, appended the way NiTArray::Add does it (unsigned compare against
// m_usMaxSize, grow by m_usGrowBy, store at m_usSize), all under the Havok heap context.
void bhkBinaryAction::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    MEM_CONTEXT& iMemContext = GetThreadTLSData()->eMemContext;
    MEM_CONTEXT iSavedMemContext = iMemContext;
    iMemContext = MEM_CONTEXT_HAVOK;
    bhkAction::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(ms_RTTI.GetName());
    apStrings->Add(pcString);
    iMemContext = iSavedMemContext;
}

// Under the Havok heap context: with a world, adds entity B's wrapper first (when there is one) and the action only
// when that succeeded; without one, removes the action. False without an action or a world.
bool bhkBinaryAction::Add(bhkWorld* apWorld) {
    MEM_CONTEXT& iMemContext = GetThreadTLSData()->eMemContext;
    MEM_CONTEXT iSavedMemContext = iMemContext;
    iMemContext = MEM_CONTEXT_HAVOK;
    bool bAdded = false;
    if (this && phkObject) {
        if (apWorld) {
            hkpEntity* pEntityB = static_cast<hkpBinaryAction*>(phkObject)->m_entityB;
            bhkRigidBody* pBodyB = pEntityB ? static_cast<bhkRigidBody*>(pEntityB->m_userData) : nullptr;
            if (!pBodyB || pBodyB->Add(apWorld)) {
                bAdded = bhkAction::Add(apWorld);
            }
        }
        else {
            Remove();
        }
    }
    iMemContext = iSavedMemContext;
    return bAdded;
}

// The base's removal under the Havok heap context, then, when entity B is the world's fixed body (a binary action on
// one body, attached to the world), the action drops it. Both worlds come through GethkWorld: this action's, then the
// bhkWorld's own. The result is the base's; false when the action is in no world.
bool bhkBinaryAction::Remove() {
    MEM_CONTEXT& iMemContext = GetThreadTLSData()->eMemContext;
    MEM_CONTEXT iSavedMemContext = iMemContext;
    iMemContext = MEM_CONTEXT_HAVOK;
    ahkpWorld* pWorld = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
    bhkWorld* pWorldM = pWorld ? static_cast<bhkWorld*>(pWorld->m_userData) : nullptr;
    if (!pWorldM) {
        iMemContext = iSavedMemContext;
        return false;
    }
    bool bRemoved = bhkAction::Remove();
    hkpBinaryAction* pAction = static_cast<hkpBinaryAction*>(phkObject);
    hkpEntity* pEntityB = pAction ? pAction->m_entityB : nullptr;
    bhkSerializable* pWorldObject = pWorldM;
    ahkpWorld* pHavokWorld = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(pWorldObject->GethkWorld()));
    hkpEntity* pFixedBody = pHavokWorld ? pHavokWorld->m_fixedRigidBody : nullptr;
    if (pEntityB == pFixedBody && phkObject) {
        static_cast<hkpBinaryAction*>(phkObject)->setEntityB(nullptr);
    }
    iMemContext = iSavedMemContext;
    return bRemoved;
}

// Hands the body's Havok entity to the wrapped action (deliberately no test of apRigidBody, as the game does).
void bhkBinaryAction::SetRigidBodyA(bhkRigidBody* apBody) {
    if (this) {
        hkpBinaryAction* pAction = static_cast<hkpBinaryAction*>(phkObject);
        if (pAction) {
            pAction->setEntityA(static_cast<hkpEntity*>(apBody->phkObject));
        }
    }
}

void bhkBinaryAction::SetRigidBodyB(bhkRigidBody* apBody) {
    if (this) {
        hkpBinaryAction* phkObject = static_cast<hkpBinaryAction*>(this->phkObject);
        if (phkObject) {
            phkObject->setEntityB(static_cast<hkpEntity*>(apBody->phkObject));
        }
    }
}

// Takes the two bodies' wrappers out of the creation info and clears it, then puts back A's clone when the clone map
// has one and B's clone (made here when the map has none), and copies the serializable part. A is looked up even when
// it is null, as the game does.
void bhkBinaryAction::CopyMembers(bhkSerializable* pDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    hkBinaryActionCinfo* pkCinfo = static_cast<hkBinaryActionCinfo*>(&CreateCinfo(bCreated));
    hkpRigidBody* pBodyA = pkCinfo->m_bodyA;
    bhkRigidBody* pRigidBodyA = pBodyA ? static_cast<bhkRigidBody*>(pBodyA->m_userData) : nullptr;
    hkpRigidBody* pBodyB = pkCinfo->m_bodyB;
    bhkRigidBody* pRigidBodyB = pBodyB ? static_cast<bhkRigidBody*>(pBodyB->m_userData) : nullptr;
    pkCinfo->m_bodyA = nullptr;
    pkCinfo->m_bodyB = nullptr;
    NiObject* pkClone;
    if (arCloneData.m_pkCloneMap->GetAt(pRigidBodyA, pkClone)) {
        pkCinfo->m_bodyA =
            pkClone ? static_cast<hkpRigidBody*>(static_cast<bhkRigidBody*>(pkClone)->phkObject) : nullptr;
    }
    if (pRigidBodyB) {
        bhkRigidBody* pCloneB;
        if (arCloneData.m_pkCloneMap->GetAt(pRigidBodyB, pkClone)) {
            pCloneB = static_cast<bhkRigidBody*>(pkClone);
        }
        else {
            pCloneB = static_cast<bhkRigidBody*>(pRigidBodyB->CreateClone(arCloneData));
        }
        pkCinfo->m_bodyB = pCloneB ? static_cast<hkpRigidBody*>(pCloneB->phkObject) : nullptr;
    }
    bhkSerializable::CopyMembers(pDest, arCloneData);
}

bhkBinaryAction::~bhkBinaryAction() {
    bhkSerializable::Kill();
    --iCount;
}

// The bhk wrapper (the Havok entity's user data) of entity A for 'A' or 'a', else of entity B; null without an
// action or entity.
bhkRigidBody* bhkBinaryAction::GetRigidBody(char acBody) {
    hkpEntity* pEntity;
    if (acBody == 'A' || acBody == 'a') {
        pEntity = (this && phkObject) ? static_cast<hkpBinaryAction*>(phkObject)->m_entityA : nullptr;
    }
    else {
        pEntity = (this && phkObject) ? static_cast<hkpBinaryAction*>(phkObject)->m_entityB : nullptr;
    }
    return pEntity ? static_cast<bhkRigidBody*>(pEntity->m_userData) : nullptr;
}

bhkRigidBody* bhkBinaryAction::GetRigidBodyB() {
    return GetRigidBody(66);
}

// bhkAction's part, then the wrapped action's two entities as the creation info's bodies (nulls without an action).
void bhkBinaryAction::CopyCinfo(hkBinaryActionCinfo& arInfo) {
    bhkAction::CopyCinfo(arInfo);
    if (this && phkObject) {
        arInfo.m_bodyA = static_cast<hkpRigidBody*>(static_cast<hkpBinaryAction*>(phkObject)->m_entityA);
    }
    else {
        arInfo.m_bodyA = nullptr;
    }
    if (this && phkObject) {
        arInfo.m_bodyB = static_cast<hkpRigidBody*>(static_cast<hkpBinaryAction*>(phkObject)->m_entityB);
    }
    else {
        arInfo.m_bodyB = nullptr;
    }
}
