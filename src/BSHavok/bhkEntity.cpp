#include "BSHavok/bhkEntity.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSHavok/hkEntityCinfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/LocalFrame/hkLocalFrame.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityActivationListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldAgentUtil.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationQueue.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

NiRTTI bhkEntity::ms_RTTI("bhkEntity", &bhkWorldObject::ms_RTTI);

// Drops the wrapped entity's local frame, kills the serializable part and counts the wrapper out.
bhkEntity::~bhkEntity() {
    hkpEntity* pEntity = static_cast<hkpEntity*>(phkObject);
    if (pEntity) {
        if (pEntity->m_localFrame) {
            pEntity->m_localFrame->removeReference();
        }
        pEntity->m_localFrame = nullptr;
    }
    bhkSerializable::Kill();
    --iCount;
}

// The base's data, then the creation info's contact callback delay, byte-swapped in place when the file's order is not
// this machine's. The flag's byte is tested against 1, not for zero, so the byte is read as untyped memory.
void bhkEntity::LoadBinary(NiStream& arStream) {
    bhkWorldObject::LoadBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bool bCreated;
        hkEntityCinfo* pCinfo = &static_cast<hkEntityCinfo&>(CreateCinfo(bCreated));
        uint16_t uiDelay = pCinfo->m_processContactCallbackDelay;
        pCinfo->m_processContactCallbackDelay = static_cast<uint16_t>((uiDelay >> 8) | (uiDelay << 8));
    }
}

// Swaps the delay to the file's order, saves the base's data, then swaps it back; the stream's flag is re-read for each
// check and its byte is tested against 1, so the byte is read as untyped memory.
void bhkEntity::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkEntityCinfo* pCinfo = &static_cast<hkEntityCinfo&>(CreateCinfo(bCreated));
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        uint16_t uiDelay = pCinfo->m_processContactCallbackDelay;
        pCinfo->m_processContactCallbackDelay = static_cast<uint16_t>((uiDelay >> 8) | (uiDelay << 8));
    }
    bhkWorldObject::SaveBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        uint16_t uiDelay = pCinfo->m_processContactCallbackDelay;
        pCinfo->m_processContactCallbackDelay = static_cast<uint16_t>((uiDelay >> 8) | (uiDelay << 8));
    }
}

void bhkEntity::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    char acBuffer[64];
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    bhkWorldObject::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(bhkEntity::ms_RTTI.GetName());
    apStrings->Add(pcString);
    bool bActive = this && phkObject && static_cast<hkpEntity*>(phkObject)->isActive().m_bool != 0;
    pcString = NiGetViewerString("Active", bActive);
    apStrings->Add(pcString);
    bool bFixed =
        this && phkObject && static_cast<hkpEntity*>(phkObject)->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED;
    pcString = NiGetViewerString("Fixed", bFixed);
    apStrings->Add(pcString);
    hkpSimulationIsland* pIsland = nullptr;
    if (this && phkObject) {
        pIsland = static_cast<hkpEntity*>(phkObject)->m_simulationIsland;
    }
    BSsprintf(acBuffer, 64, "0x%08X", pIsland);
    pcString = NiGetViewerString("SimIsland", static_cast<const char*>(acBuffer));
    apStrings->Add(pcString);
    pTLSData->eMemContext = eMemContext;
}

// The bhkWorld is the Havok world's user data; with no world the result stays false.
bool bhkEntity::Remove() {
    bool bResult = false;
    ahkpWorld* pkhkWorld = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
    bhkWorld* pkWorld = pkhkWorld ? static_cast<bhkWorld*>(pkhkWorld->m_userData) : nullptr;
    if (pkWorld) {
        return pkWorld->RemoveEntity(static_cast<hkpEntity*>(phkObject));
    }
    return bResult;
}

// Drops the wrapped entity's local frame, then kills the serializable part.
void bhkEntity::Kill() {
    hkpEntity* pEntity = static_cast<hkpEntity*>(phkObject);
    if (pEntity) {
        if (pEntity->m_localFrame) {
            pEntity->m_localFrame->removeReference();
        }
        pEntity->m_localFrame = nullptr;
    }
    bhkSerializable::Kill();
}

void bhkEntity::UpdateCFilter() {
    ahkpWorld* pkWorld = static_cast<ahkpWorld*>(GethkWorldRw());
    if (pkWorld) {
        pkWorld->updateCollisionFilterOnEntity(
            static_cast<hkpEntity*>(phkObject), HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK,
            HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS
        );
    }
}

void bhkEntity::AddEntityListener(hkpEntityListener* apListener) {
    hkpRigidBody* hkReferencedObject = static_cast<bhkRigidBody*>(this)->operator*();
    if (hkReferencedObject) {
        hkReferencedObject->hkpEntity::addEntityListener(apListener);
    }
}

void bhkEntity::RemoveEntityListener(hkpEntityListener* apList) {
    hkpEntity* v2 = static_cast<hkpEntity*>(bhkWorldObject::operator*());
    if (v2) {
        v2->removeEntityListener(apList);
    }
}

void bhkEntity::AddEntityActivationListener(hkpEntityActivationListener* apList) {
    hkpRigidBody* hkReferencedObject = static_cast<bhkRigidBody*>(this)->operator*();
    if (hkReferencedObject) {
        hkReferencedObject->hkpEntity::addEntityActivationListener(apList);
    }
}

void bhkEntity::AddContactListener(hkpContactListener* apList) {
    hkpRigidBody* hkReferencedObject = static_cast<bhkRigidBody*>(this)->operator*();
    if (hkReferencedObject) {
        hkReferencedObject->hkpEntity::addContactListener(apList);
    }
}

void bhkEntity::Deactivate() {
    hkpEntity* pkEntity = static_cast<hkpEntity*>(phkObject);
    if (pkEntity) {
        pkEntity->deactivate();
    }
}

void bhkEntity::RemoveEntityActivationListener(hkpEntityActivationListener* apList) {
    hkpEntity* v2 = static_cast<hkpEntity*>(bhkWorldObject::operator*());
    if (v2) {
        v2->removeEntityActivationListener(apList);
    }
}

bhkEntity* bhkEntity::Getbhk(const hkpWorldObject* apEnt) {
    return static_cast<bhkEntity*>(bhkWorldObject::Getbhk(apEnt));
}
