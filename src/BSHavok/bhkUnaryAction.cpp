#include "BSHavok/bhkUnaryAction.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkAction.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkLiquidAction.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

NiRTTI bhkUnaryAction::ms_RTTI("bhkUnaryAction", &bhkAction::ms_RTTI);

void bhkUnaryAction::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    hkReferencedObject* phkObject;
    hkpEntity* pEntity;
    char apDest[32];
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT result = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    bhkAction::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkUnaryAction::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* element = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, element);
    if (this && (phkObject = this->phkObject) != nullptr) {
        pEntity = static_cast<hkpUnaryAction*>(phkObject)->m_entity;
    }
    else {
        pEntity = nullptr;
    }
    // The entity's address, printed as a hex number.
    BSsprintf(apDest, 32, "0x%08X", pEntity);
    char* ViewerString_9 = NiGetViewerString("hkpRigidBody", static_cast<const char*>(apDest));
    uint32_t v10 = apStrings->m_usSize;
    element = ViewerString_9;
    if (v10 >= apStrings->m_usMaxSize) {
        apStrings->SetSize(v10 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v10, element);
    v3->eMemContext = result;
}

// Reads the queued link ID and the base's data, then clears hkUnaryActionCinfo's m_body in the creation info the
// virtual builds before the link pass fills it.
void bhkUnaryAction::LoadBinary(NiStream& arStream) {
    arStream.ReadLinkID();
    bhkSerializable::LoadBinary(arStream);
    bool bCreated;
    auto* pkCinfo = &static_cast<hkUnaryActionCinfo&>(CreateCinfo(bCreated));
    pkCinfo->m_body = nullptr;
}

void bhkUnaryAction::LinkObject(NiStream& arStream) {
    bool v5;
    hkLiquidActionCinfo* v3 = &static_cast<hkLiquidActionCinfo&>(CreateCinfo(v5));
    bhkRigidBody* pbody = static_cast<bhkRigidBody*>(arStream.GetObjectFromLinkID());
    if (pbody) {
        v3->m_body = static_cast<hkpRigidBody*>(pbody->phkObject);
    }
    bhkSerializable::LinkObject(arStream);
}

bool bhkUnaryAction::RegisterStreamables(NiStream& arStream) {
    return bhkAction::RegisterStreamables(arStream);
}

// Saves the link ID of the game object the wrapped action's entity carries, then the base's component.
void bhkUnaryAction::SaveBinary(NiStream& arStream) {
    auto* pAction = this ? static_cast<hkpUnaryAction*>(phkObject) : nullptr;
    hkpEntity* pEntity = pAction ? pAction->m_entity : nullptr;
    NiObject* pObject = pEntity ? pEntity->m_userData : nullptr;
    arStream.SaveLinkID(pObject);
    bhkSerializable::SaveBinary(arStream);
}

// The base's test, then the wrapped actions' entities compared (apObject is taken as a unary action unchecked).
bool bhkUnaryAction::IsEqual(NiObject* apObject) {
    bool bEqual = bhkSerializable::IsEqual(apObject);
    if (!bEqual) {
        return bEqual;
    }
    auto* pOther = static_cast<bhkUnaryAction*>(apObject);
    hkpEntity* pEntity = (this && phkObject) ? static_cast<hkpUnaryAction*>(phkObject)->m_entity : nullptr;
    hkpEntity* pOtherEntity =
        (pOther && pOther->phkObject) ? static_cast<hkpUnaryAction*>(pOther->phkObject)->m_entity : nullptr;
    return pEntity == pOtherEntity;
}

// The base's world check and add, with the action's entity run through the world first, all under the Havok heap
// context.
bool bhkUnaryAction::Add(bhkWorld* apWorld) {
    int32_t& iMemContext = GetThreadMemContext();
    int32_t iSavedMemContext = iMemContext;
    iMemContext = MEM_CONTEXT_HAVOK;
    bool bAdded = false;
    hkpAction* pAction = this ? static_cast<hkpAction*>(phkObject) : nullptr;
    if (pAction) {
        if (apWorld) {
            if (GethkWorld() != static_cast<ahkpWorld*>(apWorld->phkObject)) {
                Remove();
                ProcessEntity(apWorld);
                apWorld->Add(pAction);
                bAdded = true;
            }
        }
        else {
            Remove();
        }
    }
    iMemContext = iSavedMemContext;
    return bAdded;
}

// Adds the action's entity to apWorld when it is in no world yet and the world adds no batch; an entity whose flags do
// not mark it as added anyway gets a warning and is forced in.
void bhkUnaryAction::ProcessEntity(bhkWorld* apWorld) {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    auto* pAction = this ? static_cast<hkpUnaryAction*>(phkObject) : nullptr;
    hkpEntity* pEntity = pAction ? pAction->m_entity : nullptr;
    if (pEntity) {
        auto* pBody = static_cast<bhkEntity*>(pEntity->getUserData());
        if (pBody && !pBody->GethkWorld() && !apWorld->iBatchAdd) {
            pBody->Add(apWorld);
            if ((pBody->iFlags & 0x30) == 0) {
                {
                    char acBuffer[512];
                    hkOstream kStream(acBuffer, 512, true);
                    kStream.operator<<("A unary action is being added to the world before its target entities.\n");
                    hkError::s_instance->message(
                        hkError::MESSAGE_WARNING, 0x234F24FC, acBuffer,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\BSHavok\\bhkUnaryAction.cpp", 200
                    );
                }
                pBody->ForceAdd(apWorld);
            }
        }
    }
    pTLSData->eMemContext = eMemContext;
}

// Moves the creation info's body to its clone: the clone's Havok body when the cloning process has cloned the body's
// game object, else 0; then the base's members.
void bhkUnaryAction::CopyMembers(bhkSerializable* pDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    auto* pkCinfo = &static_cast<hkUnaryActionCinfo&>(CreateCinfo(bCreated));
    hkpRigidBody* pBody = pkCinfo->m_body;
    auto* pRigidBody = pBody ? static_cast<bhkRigidBody*>(pBody->getUserData()) : nullptr;
    pkCinfo->m_body = nullptr;
    if (pRigidBody) {
        NiObject* pkClone;
        if (arCloneData.m_pkCloneMap->GetAt(pRigidBody, pkClone)) {
            auto* pCloneBody = static_cast<bhkRigidBody*>(pkClone);
            pkCinfo->m_body = pCloneBody ? static_cast<hkpRigidBody*>(pCloneBody->phkObject) : nullptr;
        }
    }
    bhkSerializable::CopyMembers(pDest, arCloneData);
}

// The base's user data, then the wrapped action's entity, or 0 without an action.
void bhkUnaryAction::CopyCinfo(hkUnaryActionCinfo& arInfo) {
    bhkAction::CopyCinfo(arInfo);
    auto* pAction = this ? static_cast<hkpUnaryAction*>(phkObject) : nullptr;
    arInfo.m_body = pAction ? static_cast<hkpRigidBody*>(pAction->m_entity) : nullptr;
}
