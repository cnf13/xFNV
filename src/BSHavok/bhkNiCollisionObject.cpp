#include "BSHavok/bhkNiCollisionObject.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

#include <cmath>

NiRTTI bhkNiCollisionObject::ms_RTTI("bhkNiCollisionObject", &NiCollisionObject::ms_RTTI);
BSSpinLock bhkNiCollisionObject::kKeyframeLock;
SettingT<INISettingCollection> bhkNiCollisionObject::bWONameSync("bWONameSync:Havok", true);

// Whether each row of arM1 matches the same row of arM2 within afEpsilon. Compares one row at a time as a NiPoint3
// through BSUtilities::ComparePoints, the arM1 row first. Each translation unit keeps its own copy of this function, so
// it stays file-local here.
static bool CloseTo(const NiMatrix3& arM1, const NiMatrix3& arM2, float afEpsilon) {
    for (uint32_t uiRow = 0; uiRow < 3; ++uiRow) {
        NiPoint3 kP1;
        NiPoint3 kM2;
        kP1.x = arM1.m_pEntry[uiRow][0];
        kP1.y = arM1.m_pEntry[uiRow][1];
        kP1.z = arM1.m_pEntry[uiRow][2];
        kM2.x = arM2.m_pEntry[uiRow][0];
        kM2.y = arM2.m_pEntry[uiRow][1];
        kM2.z = arM2.m_pEntry[uiRow][2];
        if (!BSUtilities::ComparePoints(&kP1, &kM2, afEpsilon)) {
            return false;
        }
    }
    return true;
}

bhkNiCollisionObject::bhkNiCollisionObject() : spWorldObject(nullptr) {
    ++iCount;
    usFlags = ACTIVE_POS | RESET_POS;
}

bhkNiCollisionObject::bhkNiCollisionObject(NiAVObject* apSceneObject)
  : NiCollisionObject(apSceneObject), spWorldObject(nullptr) {
    ++iCount;
    usFlags = ACTIVE_POS | RESET_POS;
}

bhkNiCollisionObject::~bhkNiCollisionObject() {
    --iCount;
    if (spWorldObject) {
        spWorldObject->Remove();
    }
    SetWorldObject(nullptr);
    if (spWorldObject) {
        NiDecRefCount(spWorldObject);
    }
}

// The base's scene object link, the two flag bytes (through the stream's transfer function), then the world object's
// link.
void bhkNiCollisionObject::LoadBinary(NiStream& arStream) {
    NiCollisionObject::LoadBinary(arStream);
    NiStreamLoadBinary(arStream, this->usFlags);
    arStream.ReadLinkID();
}

void bhkNiCollisionObject::LinkObject(NiStream& arStream) {
    NiAVObject* m_pkSceneObject;
    NiCollisionObject::LinkObject(arStream);
    m_pkSceneObject = this->m_pkSceneObject;
    if (m_pkSceneObject) {
        m_pkSceneObject->m_uFlags = static_cast<NiAVObject::NiFlags>(
            (m_pkSceneObject->m_uFlags & ~NiAVObject::SELECTIVE_UPDATE_RIGID) | NiAVObject::SELECTIVE_UPDATE |
            NiAVObject::SELECTIVE_UPDATE_TRANSFORMS
        );
    }
    bhkWorldObject* objectFromLinkID = static_cast<bhkWorldObject*>(arStream.GetObjectFromLinkID());
    SetWorldObject(objectFromLinkID);
}

bool bhkNiCollisionObject::RegisterStreamables(NiStream& arStream) {
    bhkRigidBody* spWorldObject;
    spWorldObject = this->spWorldObject;
    if (spWorldObject) {
        spWorldObject->RegisterStreamables(arStream);
    }
    return NiCollisionObject::RegisterStreamables(arStream);
}

// The base's scene object link, the two flag bytes (through the stream's transfer function), then the world object's
// link, saved through the stream's vtable.
void bhkNiCollisionObject::SaveBinary(NiStream& arStream) {
    NiCollisionObject::SaveBinary(arStream);
    NiStreamSaveBinary(arStream, this->usFlags);
    arStream.SaveLinkID(this->spWorldObject);
}

// Equal when the base finds the objects equal, the flags match and the world objects are equal (or neither object holds
// one).
bool bhkNiCollisionObject::IsEqual(NiObject* apObject) {
    if (!NiCollisionObject::IsEqual(apObject)) {
        return false;
    }
    bhkNiCollisionObject* pOther = static_cast<bhkNiCollisionObject*>(apObject);
    bhkRigidBody* pWorldObject = this->spWorldObject;
    if (pWorldObject) {
        bool bEqual = pWorldObject->IsEqual(pOther->spWorldObject);
        return bEqual & (pOther->usFlags == usFlags);
    }
    return (pOther->spWorldObject == nullptr) & (pOther->usFlags == usFlags);
}

// The collision object's viewer lines: the base's, the class name, the world object's address, the flags, whether it is
// keyframed and whether the scene object faces down, all in the Havok memory context.
void bhkNiCollisionObject::AddViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    NiCollisionObject::AddViewerStrings(apStrings);
    char* pcString = NiGetViewerString(ms_RTTI.GetName());
    apStrings->Add(pcString);
    char acBuffer[32];
    BSsprintf(acBuffer, 32, "0x%8X", spWorldObject);
    pcString = NiGetViewerString("bhkWorldObject", static_cast<const char*>(acBuffer));
    apStrings->Add(pcString);
    pcString = NiGetViewerString("bActive", (usFlags & ACTIVE_POS) != 0);
    apStrings->Add(pcString);
    pcString = NiGetViewerString("bReset", (usFlags & RESET_POS) != 0);
    apStrings->Add(pcString);
    pcString = NiGetViewerString("bNotify", (usFlags & NOTIFY_POS) != 0);
    apStrings->Add(pcString);
    pcString = NiGetViewerString("bSetLocal", (usFlags & SETLOCAL_POS) != 0);
    apStrings->Add(pcString);
    pcString = NiGetViewerString("bKeyframe", QKeyframe());
    apStrings->Add(pcString);
    // Faces down: the world rotation's [2][0] entry is below zero (false for a NaN or no scene object).
    NiAVObject* pSceneObject = this->m_pkSceneObject;
    bool bFaceDown = false;
    if (pSceneObject) {
        bFaceDown = pSceneObject->m_kWorld.m_Rotate.m_pEntry[2][0] < 0.0f;
    }
    pcString = NiGetViewerString("bFaceDown", bFaceDown);
    apStrings->Add(pcString);
}

void bhkNiCollisionObject::SetSceneGraphObject(NiAVObject* apSceneObject) {
    NiCollisionObject::SetSceneGraphObject(apSceneObject);
}

void bhkNiCollisionObject::UpdateWorldData(NiUpdateData& arUpdateData) {
    bhkNiCollisionObject::SYNCMODE eHavokSyncMode = bhkNiCollisionObject::eHavokSyncMode;
    if (eHavokSyncMode != bhkNiCollisionObject::SYNC_NONE) {
        if ((usFlags & bhkNiCollisionObject::RESET_POS) != 0) {
            if (eHavokSyncMode != bhkNiCollisionObject::SYNC_FROMHAVOK) {
                ClearVelocities();
                NitohkTransform();
                usFlags &= ~RESET_POS;
                return;
            }
        }
        else {
            if ((usFlags & bhkNiCollisionObject::ACTIVE_POS) != 0) {
                if (QKeyframe()) {
                    if (bhkNiCollisionObject::eHavokSyncMode != bhkNiCollisionObject::SYNC_FROMHAVOK) {
                        NitohkTransform();
                        return;
                    }
                }
                else {
                    if (bhkNiCollisionObject::eHavokSyncMode != bhkNiCollisionObject::SYNC_TOHAVOK) {
                        hktoNiTransform();
                        return;
                    }
                }
            }
        }
    }
    NiAVObject* m_pkSceneObject = this->m_pkSceneObject;
    if (m_pkSceneObject) {
        m_pkSceneObject->UpdateWorldLoc();
    }
}

// True when the Havok rigid body is keyframed or fixed, and when there is no rigid body to ask.
bool bhkNiCollisionObject::QKeyframe() {
    bhkRigidBody* pWorldObject = this->spWorldObject;
    if (pWorldObject) {
        const hkpCollidable* pCollidable = pWorldObject->GetCollidable();
        hkpRigidBody* pBody = hkpGetRigidBody(pCollidable);
        if (pBody) {
            uint8_t ucType = pBody->m_motion.m_type.m_storage;
            return ucType == hkpMotion::MOTION_KEYFRAMED || ucType == hkpMotion::MOTION_FIXED;
        }
    }
    return true;
}

// Puts arTrans on the scene graph object: as its local transform (relative to the parent's world transform) when the
// set-local flag is set, and as its world transform when it moved (rotation beyond 0.001 or translation beyond 0.01,
// or always when bAlwaysReportTransform). Only the rotation and the translation are copied, never the scale. The
// change callback, when there is one and the notify flag is set, gets the changed parts (2 rotation, 1 translation).
void bhkNiCollisionObject::SetTransform(NiTransform& arTrans) {
    NiAVObject* pSceneObject = this->m_pkSceneObject;
    if (!pSceneObject) {
        return;
    }
    NiNode* pParent = pSceneObject->m_pkParent;
    if (usFlags & SETLOCAL_POS) {
        NiTransform kLocal;
        if (pParent) {
            pParent->m_kWorld.Invert(kLocal);
            NiTransform kProduct = kLocal * arTrans;
            kLocal = kProduct;
        }
        else {
            kLocal = arTrans;
        }
        pSceneObject->m_kLocal.m_Rotate = kLocal.m_Rotate;
        pSceneObject->m_kLocal.m_Translate = kLocal.m_Translate;
    }
    uint32_t uiChanged = 0;
    if (bAlwaysReportTransform) {
        uiChanged = 3;
    }
    else {
        if (!CloseTo(pSceneObject->m_kWorld.m_Rotate, arTrans.m_Rotate, 0.001f)) {
            uiChanged = 2;
        }
        if (!BSUtilities::ComparePoints(&pSceneObject->m_kWorld.m_Translate, &arTrans.m_Translate, 0.01f)) {
            uiChanged |= 1;
        }
        if (!uiChanged) {
            return;
        }
    }
    pSceneObject->m_kWorld.m_Rotate = arTrans.m_Rotate;
    pSceneObject->m_kWorld.m_Translate = arTrans.m_Translate;
    TransChangeFunc pFunc = pTransChangeFunc;
    if (pFunc && (usFlags & NOTIFY_POS)) {
        pFunc(this, uiChanged);
    }
}

// Turns the collision object's debug display on or off, all of it inside the Havok memory context. On: a fresh
// "bhkColDisp" node is attached to the scene object's parent (or is the scene object itself when it is a node), scaled
// so that it cancels the parent's world scale, and the world object's display geometry is put in it. Off: the world
// object's display geometry is destroyed and every child of the node whose own name is "bhkColDisp" is detached.
void bhkNiCollisionObject::SetDebugDisplay(bool abTF) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    NiUpdateData kUpdateData;
    if (abTF) {
        if ((usFlags & DEBUGDISPLAY_POS) != 0) {
            return;
        }
        bhkRigidBody* pWorldObject = this->spWorldObject;
        if (!pWorldObject) {
            return;
        }
        hkpWorldObject* pkObject = static_cast<hkpWorldObject*>(pWorldObject->phkObject);
        uint32_t uiFilter = 0;
        if (pkObject) {
            uiFilter = pkObject->m_collidable.getCollisionFilterInfo();
        }
        // Bodies on the ground layer get no display geometry.
        if ((uiFilter & 0x7F) == L_GROUND) {
            return;
        }
        NiAVObject* pSceneObject = this->m_pkSceneObject;
        NiNode* pParent = pSceneObject ? pSceneObject->IsNode() : nullptr;
        if (!pParent) {
            pParent = pSceneObject->m_pkParent;
        }
        NiNode* pNode = new NiNode(0);
        NiFixedString kName("bhkColDisp");
        pNode->SetName(kName);
        if (pParent) {
            float fScale = pParent->m_kWorld.m_fScale;
            fScale = 1.0f / fScale;
            fScale = std::fabsf(fScale);
            pNode->m_kLocal.m_fScale = fScale;
            pParent->AttachChild(pNode, true);
            kUpdateData.fTime = 0.0f;
            kUpdateData.bUpdateControllers = false;
            kUpdateData.bParallelUpdate = false;
            kUpdateData.bFoundParticles = false;
            kUpdateData.bFoundMorphController = false;
            kUpdateData.bSceneGraphChange = false;
            pNode->Update(kUpdateData);
        }
        NiAVObject* pDisplay = pWorldObject->CreateDisplayGeometry(pNode);
        NiPointer<NiAVObject> spDisplay(pDisplay);
        if (pDisplay) {
            if (pParent) {
                pDisplay->UpdateProperties();
                kUpdateData.fTime = 0.0f;
                kUpdateData.bUpdateControllers = false;
                kUpdateData.bParallelUpdate = false;
                kUpdateData.bFoundParticles = false;
                kUpdateData.bFoundMorphController = false;
                kUpdateData.bSceneGraphChange = false;
                pDisplay->Update(kUpdateData);
            }
        }
        else if (pParent && pNode) {
            NiPointer<NiAVObject> spDetached;
            pParent->DetachChild(pNode, spDetached);
        }

        usFlags |= DEBUGDISPLAY_POS;
    }
    else {
        if ((usFlags & DEBUGDISPLAY_POS) == 0) {
            return;
        }
        NiAVObject* pSceneObject = this->m_pkSceneObject;
        NiNode* pNode = pSceneObject ? pSceneObject->IsNode() : nullptr;
        bhkRigidBody* pWorldObject = this->spWorldObject;
        if (pWorldObject) {
            pWorldObject->DestroyDisplayGeometry();
        }
        if (!pNode) {
            pNode = pSceneObject->m_pkParent;
        }
        if (pNode) {
            const uint32_t uiCount = pNode->m_kChildren.m_usSize;
            for (uint32_t uiChild = 0; uiChild < uiCount; ++uiChild) {
                NiAVObject* pChild = pNode->m_kChildren.m_pBase[uiChild];
                NiAVObject* pFound;
                if (pChild) {
                    // Calls the base implementation directly, not virtually: it answers for a child whose own name is
                    // the display node's.
                    pFound = pChild->NiAVObject::GetObjectByName("bhkColDisp");
                }
                else {
                    pFound = nullptr;
                }
                if (pFound) {
                    NiPointer<NiAVObject> spDetached;
                    pNode->DetachChild(pFound, spDetached);
                }
            }
        }
        usFlags &= ~DEBUGDISPLAY_POS;
    }
}

bool bhkNiCollisionObject::GetFlag(uint16_t ausFlag) {
    return (usFlags & ausFlag) != 0;
}

bool bhkNiCollisionObject::IsReset() {
    return GetFlag(RESET_POS);
}

bhkWorldObject* bhkNiCollisionObject::GetWorldObject() {
    return spWorldObject;
}

// Holds apWorldObject as this object's Havok body. The old body loses the property that points back at this object
// (keyed by the class's RTTI address) between its write marks, the reference-counted swap follows, then the new body
// gets the property the same way. The assignment re-reads the field after the old body's property change.
void bhkNiCollisionObject::SetWorldObject(bhkWorldObject* apWorldObject) {
    bhkRigidBody* worldObject = this->spWorldObject;
    if (worldObject != apWorldObject) {
        if (worldObject) {
            hkpWorldObject* hkObject = static_cast<hkpWorldObject*>(worldObject->phkObject);
            if (hkObject) {
                worldObject->MarkWrite();
                // The property key is the address of the class's RTTI object.
                hkObject->removeProperty(bhkPropertyKey(ms_RTTI));
                worldObject->MarkWrite();
            }
        }
        if (this->spWorldObject != static_cast<bhkRigidBody*>(apWorldObject)) {
            if (this->spWorldObject) {
                NiDecRefCount(this->spWorldObject);
            }
            this->spWorldObject = static_cast<bhkRigidBody*>(apWorldObject);
            if (apWorldObject) {
                NiIncRefCount(apWorldObject);
            }
        }
        worldObject = this->spWorldObject;
        if (worldObject) {
            hkpWorldObject* hkObject = static_cast<hkpWorldObject*>(worldObject->phkObject);
            if (hkObject) {
                worldObject->MarkWrite();
                // The value is this object as an untyped pointer-sized word.
                hkpPropertyValue kValue;
                kValue.m_data = reinterpret_cast<uintptr_t>(this);
                hkObject->addProperty(bhkPropertyKey(ms_RTTI), kValue);
                worldObject->MarkWrite();
            }
        }
    }
}

bool bhkNiCollisionObject::QNotify() {
    return GetFlag(NOTIFY_POS);
}

bool bhkNiCollisionObject::HasVelocity() {
    return GetFlag(USEVEL_POS);
}

bool bhkNiCollisionObject::QSetLocal() {
    return GetFlag(SETLOCAL_POS);
}

void bhkNiCollisionObject::SetReset(bool abTF) {
    SetBit(abTF, RESET_POS);
}

void bhkNiCollisionObject::SetSetLocal(bool abTF) {
    SetBit(abTF, SETLOCAL_POS);
}

bool bhkNiCollisionObject::GetUseVelocity() {
    return (usFlags & USEVEL_POS) != 0 && !IsReset();
}

void bhkNiCollisionObject::SetBit(bool bVal, uint16_t uMask) {
    if (bVal) {
        usFlags |= uMask;
    }
    else {
        usFlags &= (~uMask);
    }
}

void bhkNiCollisionObject::SetTransChangeCallback(TransChangeFunc apFunc) {
    pTransChangeFunc = apFunc;
}

void bhkNiCollisionObject::SetDebugDisplayCallback(void (*apCallbackFunc)(NiGeometry*, const NiColor&)) {
    pDebugDispFunc = apCallbackFunc;
}

// Returns the collision object apBody carries as its property keyed by the collision object's RTTI address. The
// property value is an untyped word holding the collision object.
bhkNiCollisionObject* bhkNiCollisionObject::Getbhk(const hkpWorldObject* apBody) {
    hkpPropertyValue kProperty;
    return reinterpret_cast<bhkNiCollisionObject*>(
        static_cast<uintptr_t>(bhkWorldObject::GetProperty(&kProperty, apBody, bhkPropertyKey(ms_RTTI))->m_data)
    );
}

bhkNiCollisionObject* bhkNiCollisionObject::GetbhkNiCollisionObject(NiAVObject* apObject) {
    bhkNiCollisionObject* collisionObject = static_cast<bhkNiCollisionObject*>(
        apObject->NiAVObject::GetCollisionObject()
    );
    if (collisionObject) {
        return collisionObject->IsBhkNiCollisionObject();
    }

    return nullptr;
}

// Whether the object below apRoot named apName (apRoot itself when apName is null or names no object) faces up: its
// world rotation's [2][1] entry is above zero (false for a NaN).
bool bhkNiCollisionObject::GetFaceUp(NiAVObject* apRoot, const char* apName) {
    if (!apRoot) {
        return false;
    }
    NiAVObject* pObject = nullptr;
    if (apName) {
        pObject = apRoot->GetObjectByName(apName);
    }
    if (!pObject) {
        pObject = apRoot;
    }
    return pObject->m_kWorld.m_Rotate.m_pEntry[2][1] > 0.0f;
}

void bhkNiCollisionObject::ResetSim(NiAVObject* apObject, bool abRecurse) {
    if (apObject) {
        bhkNiCollisionObject* m_pObject = apObject->m_spCollisionObject.m_pObject;
        if (m_pObject) {
            bhkNiCollisionObject* v3 = m_pObject->IsBhkNiCollisionObject();
            if (v3) {
                v3->usFlags |= bhkNiCollisionObject::RESET_POS;
            }
        }
        if (abRecurse) {
            NiNode* pnode = apObject->IsNode();
            if (pnode) {
                int32_t m_usSize = pnode->m_kChildren.m_usSize;
                if (pnode->m_kChildren.m_usSize) {
                    do {
                        NiAVObject* v7 = pnode->m_kChildren.m_pBase[--m_usSize].m_pObject;
                        bhkNiCollisionObject::ResetSim(v7, true);
                    }
                    while (m_usSize);
                }
            }
        }
    }
}

// Walks apObject and the objects below it. An object whose collision object is a bhkNiCollisionObject (or any object
// once abForce is set) first gets its world transform updated, then its world data with a zero update (time 0, no
// controllers, no parallel update), which forces the same on its children. Every object's world bound is refreshed
// last.
void bhkNiCollisionObject::SynchronizeRec(NiAVObject* apObject, bool abUpdate) {
    if (!apObject) {
        return;
    }
    bhkNiCollisionObject* pCollisionObject = apObject->m_spCollisionObject;
    if ((pCollisionObject && pCollisionObject->IsBhkNiCollisionObject()) || abUpdate) {
        apObject->UpdateWorldLoc();
        // The update data has a zero time and no controllers.
        NiUpdateData kUpdateData;
        kUpdateData.fTime = 0.0f;
        kUpdateData.bUpdateControllers = false;
        kUpdateData.bParallelUpdate = false;
        kUpdateData.bFoundParticles = false;
        kUpdateData.bFoundMorphController = false;
        kUpdateData.bSceneGraphChange = false;
        apObject->UpdateWorldData(kUpdateData);
        abUpdate = true;
    }
    NiNode* pNode = apObject->IsNode();
    if (pNode) {
        // Compares the index against the zero-extended child count, unsigned, and reloads the count after each call
        // (the child may change the array).
        for (uint32_t uiChild = 0; uiChild < static_cast<uint32_t>(pNode->m_kChildren.m_usSize); ++uiChild) {
            SynchronizeRec(pNode->m_kChildren.m_pBase[uiChild], abUpdate);
        }
    }
    apObject->UpdateWorldBound();
}

// Copies transforms between the scene graph and Havok for apObject with the calling thread's sync mode set to aeMode
// (not forced), restoring the previous mode afterwards.
void bhkNiCollisionObject::Synchronize(NiAVObject* apObject, SYNCMODE aeMode) {
    SYNCMODE eOldMode = eHavokSyncMode;
    eHavokSyncMode = aeMode;
    SynchronizeRec(apObject, false);
    eHavokSyncMode = eOldMode;
}

void bhkNiCollisionObject::EnableCollision(NiAVObject* apObject, bool abEnable) {
    bhkNiCollisionObject* m_pObject = apObject->m_spCollisionObject.m_pObject;
    if (m_pObject) {
        bhkNiCollisionObject* v3 = m_pObject->IsBhkNiCollisionObject();
        if (v3) {
            bhkWorldObject* spWorldObject = v3->spWorldObject;
            if (spWorldObject) {
                spWorldObject->EnableCollision(abEnable);
            }
        }
    }
}

void bhkNiCollisionObject::SetRigidBodyName(NiAVObject* apObject) {
    if (apObject) {
        bhkNiCollisionObject* m_pObject = apObject->m_spCollisionObject.m_pObject;
        if (m_pObject) {
            bhkNiCollisionObject* v2 = m_pObject->IsBhkNiCollisionObject();
            if (v2) {
                NiObject* spWorldObject = v2->spWorldObject;
                if (spWorldObject) {
                    bhkRigidBody* prigidBody = static_cast<bhkRigidBody*>(
                        spWorldObject->DynamicCastMember(&bhkRigidBody::ms_RTTI)
                    );
                    if (prigidBody) {
                        static_cast<hkpWorldObject*>(prigidBody->phkObject)->setName(apObject->m_kName.GetHandle());
                    }
                }
            }
        }
        NiNode* pnode = apObject->IsNode();
        if (pnode) {
            int32_t m_usSize = pnode->m_kChildren.m_usSize;
            int32_t ichildIdx = 0;
            if (pnode->m_kChildren.m_usSize) {
                do {
                    bhkNiCollisionObject::SetRigidBodyName(pnode->m_kChildren.m_pBase[ichildIdx++].m_pObject);
                }
                while (ichildIdx < m_usSize);
            }
        }
    }
}
