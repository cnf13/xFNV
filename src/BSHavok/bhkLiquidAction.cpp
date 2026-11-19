#include "BSHavok/bhkLiquidAction.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkAction.h"
#include "BSHavok/bhkBvTreeShape.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkUnaryAction.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSHavok/hkLiquidAction.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpDynamicsCpIdMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpSimpleContactConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"
#include "SDK/MSVC/string.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

namespace bhkConvert {} // namespace bhkConvert

NiRTTI bhkLiquidAction::ms_RTTI("bhkLiquidAction", &bhkUnaryAction::ms_RTTI);
bhkLiquidActionListMap bhkLiquidAction::LiquidListMap;

int32_t getContactPointIdAt(hkpSimpleContactConstraintData* apThis, [[maybe_unused]] void* apEdx, int32_t a2) {
    uint8_t* m_data;
    int32_t result = (int32_t)(apThis->m_idMgrA.m_values.m_size - 1);
    if (result >= 0) {
        m_data = static_cast<uint8_t*>(static_cast<void*>(apThis->m_idMgrA.m_values.m_data));
        do {
            if (((uint32_t)m_data[result]) == ((uint32_t)a2)) {
                break;
            }
            --result;
        }
        while (result >= 0);
    }
    return result;
}

bhkLiquidAction::bhkLiquidAction()
  : pTargetObject(nullptr), fNeighborDistance(0.0f), fNeighborStrength(0.0f), fInitialStickForce(0.0f),
    fStickStrength(0.0f), StickPoint(NiPoint3_ZERO), StickNormal(NiPoint3_ZERO), bFirstUpdate(true) {
    ++iCount;
}

bhkLiquidAction::bhkLiquidAction(const hkLiquidActionCinfo& arInfo)
  : pTargetObject(nullptr), fNeighborDistance(0.0f), fNeighborStrength(0.0f), fInitialStickForce(0.0f),
    fStickStrength(0.0f), StickPoint(NiPoint3_ZERO), StickNormal(NiPoint3_ZERO), bFirstUpdate(true) {
    bhkLiquidAction::Init(arInfo);
    ++iCount;
}

// Inside the Havok heap context: the action leaves its target's list (the list and its map entry go when it is the
// last), then the serializable part is killed and the action counted out.
bhkLiquidAction::~bhkLiquidAction() {
    TLSData* pTLSData = GetThreadTLSData();
    MEM_CONTEXT eMemContext = pTLSData->eMemContext;
    pTLSData->eMemContext = MEM_CONTEXT_HAVOK;
    if (pTargetObject) {
        BSSimpleList<bhkLiquidAction*>* pList = nullptr;
        LiquidListMap.GetAt(pTargetObject, pList);
        if (pList) {
            bhkLiquidAction* pAction = this;
            pList->Remove(pAction);
            if (!pList->GetNext() && !pList->m_item) {
                LiquidListMap.RemoveAt(pTargetObject);
                pList->RemoveAll();
                MemoryManager_Singleton.Deallocate(pList);
            }
        }
    }
    bhkSerializable::Kill();
    --iCount;
    pTLSData->eMemContext = eMemContext;
}

const NiRTTI* bhkLiquidAction::GetRTTI() const {
    return &bhkLiquidAction::ms_RTTI;
}

// Clones the action into a new object of the class: the engine's heap, under the engine's clone critical section, with
// the clone charged to the Havok heap context.
NiObject* bhkLiquidAction::CreateClone(NiCloningProcess& arCloneData) {
    int32_t iMemContext = GetThreadMemContext();
    GetThreadMemContext() = MEM_CONTEXT_HAVOK;
    bhkLiquidAction* pClone = nullptr;
    bhkSerializable::CloneCritLock();
    void* pMemory = NiMemObject::operator new(sizeof(bhkLiquidAction));
    if (pMemory) {
        pClone = ::new (pMemory) bhkLiquidAction;
    }
    this->CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    GetThreadMemContext() = iMemContext;
    return pClone;
}

// Byte-swaps the creation info's four strengths after the base's load when the stream is big-endian.
void bhkLiquidAction::LoadBinary(NiStream& arStream) {
    bhkUnaryAction::LoadBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
        bool bCreated;
        hkLiquidActionCinfo& rInfo = static_cast<hkLiquidActionCinfo&>(CreateCinfo(bCreated));
        rInfo.fInitialStickForce =
            std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.fInitialStickForce)));
        rInfo.fStickStrength = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.fStickStrength)));
        rInfo.fNeighborDistance =
            std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.fNeighborDistance)));
        rInfo.fNeighborStrength =
            std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.fNeighborStrength)));
    }
}

// Byte-swaps the creation info's four strengths around the base's save when the stream is big-endian, so that the
// info is written swapped and left as it was.
void bhkLiquidAction::SaveBinary(NiStream& arStream) {
    bool bCreated;
    hkLiquidActionCinfo& rInfo = static_cast<hkLiquidActionCinfo&>(CreateCinfo(bCreated));
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        rInfo.fInitialStickForce =
            std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.fInitialStickForce)));
        rInfo.fStickStrength = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.fStickStrength)));
        rInfo.fNeighborDistance =
            std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.fNeighborDistance)));
        rInfo.fNeighborStrength =
            std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.fNeighborStrength)));
    }
    bhkUnaryAction::SaveBinary(arStream);
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        rInfo.fInitialStickForce =
            std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.fInitialStickForce)));
        rInfo.fStickStrength = std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.fStickStrength)));
        rInfo.fNeighborDistance =
            std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.fNeighborDistance)));
        rInfo.fNeighborStrength =
            std::bit_cast<float>(__builtin_bswap32(std::bit_cast<uint32_t>(rInfo.fNeighborStrength)));
    }
}

// Appends the base's strings and then the class's RTTI name to the viewer string array.
void bhkLiquidAction::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    int32_t iMemContext = GetThreadMemContext();
    GetThreadMemContext() = MEM_CONTEXT_HAVOK;
    bhkUnaryAction::GetViewerStrings(apStrings);
    char* pcString = NiGetViewerString(bhkLiquidAction::ms_RTTI.m_pcName);
    apStrings->Add(pcString);
    GetThreadMemContext() = iMemContext;
}

// Releases a creation info this action made: the aligned block it sits in, found through the padding byte in front of
// it, under the Havok heap context.
void bhkLiquidAction::KillCinfo(bool abCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    if (abCreated) {
        if (pInfo) {
            // The block is treated as raw memory: the alignment padding byte sits in front of the pointer.
            uint8_t* pMemory = static_cast<uint8_t*>(static_cast<void*>(pInfo));
            MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
        }
        pInfo = nullptr;
    }
}

// The size of the creation info in a file: its serialized form's.
uint32_t bhkLiquidAction::QCinfoSize() const {
    return sizeof(hkLiquidActionCinfo::Serialized);
}

// Reads the serialized form and fills the creation info from it, and returns the form's size.
int32_t bhkLiquidAction::LoadCInfo(NiStream& arStream) {
    bool bCreated;
    hkLiquidActionCinfo& rCinfo = static_cast<hkLiquidActionCinfo&>(CreateCinfo(bCreated));
    hkLiquidActionCinfo::Serialized kForm{};
    arStream.Istr()->BinaryRead(&kForm, sizeof(kForm), nullptr, 0);
    rCinfo.ReadSerialized(kForm);
    return sizeof(kForm);
}

// Builds the wrapped liquid action from the creation info and makes it follow the scene object's parent node. Charges
// the allocation to the Havok heap context, takes the action from the router's heap, reads the property the cinfo's
// body carries under the collision object base's RTTI, and follows the collision object to the scene object's parent.
// The low 32 bits of the property value are the object.
void bhkLiquidAction::Init(const hkSerializableCinfo& arInfo) {
    int32_t iMemContext = GetThreadMemContext();
    GetThreadMemContext() = MEM_CONTEXT_HAVOK;
    const hkLiquidActionCinfo& rCinfo = static_cast<const hkLiquidActionCinfo&>(arInfo);
    fInitialStickForce = rCinfo.fInitialStickForce;
    fStickStrength = rCinfo.fStickStrength;
    fNeighborDistance = rCinfo.fNeighborDistance;
    fNeighborStrength = rCinfo.fNeighborStrength;
    hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
    hkLiquidAction* pAction = static_cast<hkLiquidAction*>(kRouter.m_heap->blockAlloc(sizeof(hkLiquidAction)));
    pAction->m_memSizeAndFlags = sizeof(hkLiquidAction);
    pAction = ::new (pAction) hkLiquidAction(rCinfo.m_body, 0);
    this->SethkObject(pAction);
    pAction->removeReference();
    // The key is the collision object base's RTTI address, stored as the property's untyped value; the value is a
    // pointer too.
    hkpPropertyValue kValue;
    bhkWorldObject::GetProperty(&kValue, rCinfo.m_body, bhkPropertyKey(bhkNiCollisionObject::ms_RTTI));
    NiObject* pObject = reinterpret_cast<NiObject*>(static_cast<uintptr_t>(kValue.m_data));
    if (pObject != nullptr) {
        bhkCollisionObject* pCollisionObject =
            static_cast<bhkCollisionObject*>(pObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI));
        if (pCollisionObject != nullptr) {
            NiAVObject* pSceneObject = pCollisionObject->m_pkSceneObject;
            if (pSceneObject != nullptr && pSceneObject->m_pkParent != nullptr) {
                SetTargetObject(pSceneObject->m_pkParent);
            }
        }
    }
    this->PostInit(arInfo);
    GetThreadMemContext() = iMemContext;
}

// Makes the cinfo from the wrapped action, or returns the one already made; the whole body runs under the Havok heap
// context. The cinfo is allocated with slack for the 16-byte alignment it wants, with the padding length kept in the
// byte before the object, and its fields are stored in this order; with an action present it then runs the base's
// CopyCinfo and copies the liquid's four strengths.
hkSerializableCinfo& bhkLiquidAction::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    arCreated = (pInfo == nullptr);
    if (arCreated) {
        char* pMemory = static_cast<char*>(MemoryManager_Singleton.Allocate(sizeof(hkLiquidActionCinfo) + 0x10));
        uint8_t uiPadding = (uint8_t)(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
        // The block is treated as raw memory: the object is moved up to the next 16-byte boundary and the padding
        // length is kept in the byte before it.
        char* pAligned = pMemory + uiPadding;
        static_cast<uint8_t*>(static_cast<void*>(pAligned))[-1] = uiPadding;
        hkLiquidActionCinfo* pCinfo = static_cast<hkLiquidActionCinfo*>(static_cast<void*>(pAligned));
        pCinfo->m_userData = 0;
        pCinfo->fInitialStickForce = 5000.0f;
        pCinfo->m_body = nullptr;
        pCinfo->fStickStrength = 15.0f;
        pCinfo->fNeighborDistance = 64.0f;
        pCinfo->fNeighborStrength = 15.0f;
        pInfo = pCinfo;
        if (phkObject) {
            bhkUnaryAction::CopyCinfo(*pCinfo);
            pCinfo->fInitialStickForce = fInitialStickForce;
            pCinfo->fStickStrength = fStickStrength;
            pCinfo->fNeighborDistance = fNeighborDistance;
            pCinfo->fNeighborStrength = fNeighborStrength;
        }
    }
    return *pInfo;
}

// Each step: follows the body's collision object's parent when the action has no target yet; sticks the body to the
// first static or terrain contact point facing up; damps the body's velocity (harder while stuck) and adds gravity
// while stuck or always anti-grav; pulls the body away from the other actions on its target, towards its stick point
// and along the target's z axis; and hands that force, in Havok units, to the Havok action.
void bhkLiquidAction::Update(float afDelta) {
    // EXE-INLINED none: bhkLiquidAction::Gethk
    hkLiquidAction* pliquid = this ? static_cast<hkLiquidAction*>(phkObject) : nullptr;
    hkpEntity* pentity = pliquid ? pliquid->m_entity : nullptr;
    NiPoint3 force = NiPoint3_ZERO;
    if (pentity) {
        bhkRigidBody* pbody = static_cast<bhkRigidBody*>(pentity->getUserData());
        if (pbody) {
            if (!pTargetObject) {
                // The property's key is the collision object's RTTI address.
                alignas(16) hkpPropertyValue kValue;
                NiObject* pobject = static_cast<NiObject*>(bhkWorldObject::GetProperty(
                                                               &kValue, static_cast<hkpWorldObject*>(pbody->phkObject),
                                                               bhkPropertyKey(bhkNiCollisionObject::ms_RTTI)
                )
                                                               ->getPtr());
                if (pobject) {
                    bhkCollisionObject* pcol =
                        static_cast<bhkCollisionObject*>(pobject->DynamicCastMember(&bhkCollisionObject::ms_RTTI));
                    if (pcol) {
                        NiAVObject* pscene = pcol->m_pkSceneObject;
                        if (pscene) {
                            NiNode* pparent = pscene->m_pkParent;
                            if (pparent) {
                                SetTargetObject(pparent);
                            }
                        }
                    }
                }
            }
            hkVector4 kPosHK;
            NiPoint3 pos;
            HK2NI(pos, pbody->GetPosition(kPosHK));
            hkpRigidBody* phk = static_cast<hkpRigidBody*>(pbody->phkObject);
            bool bantiGrav = balwaysAntiGrav;
            bool bsticking = false;
            const hkVector4* pVelocity = phk ? &phk->m_motion.m_linearVelocity : &hkVector4Zero;
            hkVector4 velocity;
            velocity.m_quad = pVelocity->m_quad;
            if (phk && phk->getNumConstraints() && StickPoint == NiPoint3_ZERO) {
                // Sticks to the first contact with a static or terrain body whose normal points up.
                for (int32_t i = 0; StickPoint == NiPoint3_ZERO && i < phk->getNumConstraints(); ++i) {
                    hkpConstraintInstance* pconstraint = phk->getConstraint(i);
                    hkpConstraintData* pdata = pconstraint->getDataRw();
                    if (pdata->getType() != hkpConstraintData::CONSTRAINT_TYPE_CONTACT) {
                        continue;
                    }
                    CFilter filter;
                    if (pconstraint->m_entities[0] == phk && pconstraint->m_entities[1]) {
                        filter.iFilter =
                            pconstraint->m_entities[1]->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo;
                    }
                    else if (pconstraint->m_entities[1] == phk && pconstraint->m_entities[0]) {
                        filter.iFilter =
                            pconstraint->m_entities[0]->m_collidable.m_broadPhaseHandle.m_collisionFilterInfo;
                    }
                    else {
                        continue;
                    }
                    // EXE-INLINED 0x43B4D0
                    COL_LAYER eLayer = static_cast<COL_LAYER>(filter.iFilter & 0x7F);
                    if (eLayer != L_STATIC && eLayer != L_TERRAIN) {
                        continue;
                    }
                    hkpSimpleContactConstraintData* pcontact = static_cast<hkpSimpleContactConstraintData*>(pdata);
                    hkpSimpleContactConstraintAtom* patom = pcontact->m_atom;
                    if (!patom->m_numContactPoints) {
                        continue;
                    }
                    uint16_t id = static_cast<uint16_t>(getContactPointIdAt(pcontact, nullptr, 0));
                    // EXE-INLINED none: hkpSimpleContactConstraintData::getContactPoint
                    hkContactPoint* pPoint = &patom->getContactPoints()[pcontact->m_idMgrA.m_values[id]];
                    hkContactPoint point;
                    point.m_position.m_quad = pPoint->m_position.m_quad;
                    point.m_separatingNormal.m_quad = pPoint->m_separatingNormal.m_quad;
                    if (point.m_separatingNormal.m_quad[2] < 0.5f) {
                        HK2NI(StickPoint, point.m_position);
                        HK2NINS(StickNormal, point.m_separatingNormal);
                        velocity.set(zero, zero, zero, zero);
                    }
                }
            }
            float fdampeningFactor;
            if (NiPoint3_ZERO == StickPoint) {
                fdampeningFactor = fdampenNormal;
            }
            else {
                NiPoint3 toStick = StickPoint - pos;
                bantiGrav = true;
                bsticking = true;
                float flength = toStick.Length();
                if (flength > zero) {
                    float fstickFactor = StickNormal.Dot(toStick);
                    force += StickNormal * fstickFactor;
                }
                fdampeningFactor = fdampenStick;
            }
            fdampeningFactor = fdampeningFactor * -1.0 * afDelta;
            fdampeningFactor = std::exp(fdampeningFactor);
            // The SDK's inline mul4: the factor broadcast, times the velocity.
            velocity.m_quad = _mm_mul_ps(_mm_set_ps1(fdampeningFactor), velocity.m_quad);
            if (bantiGrav) {
                hkVector4 gravity;
                gravity.set(zero, zero, -afDelta * fgravity * fgravityfactor, zero);
                // Adds with the gravity as the first operand, as the game does.
                velocity.m_quad = _mm_add_ps(gravity.m_quad, velocity.m_quad);
            }
            hkpRigidBody* phkBody = static_cast<hkpRigidBody*>(pbody->phkObject);
            if (phkBody) {
                pbody->MarkWrite();
                phkBody->setLinearVelocity(velocity);
                pbody->UnMarkWrite();
            }
            // Pushes the body away from the other actions on the target closer than twice the neighbour distance,
            // without the part along the stick normal while stuck.
            bhkLiquidActionList* pliquidList = nullptr;
            LiquidListMap.GetAt(pTargetObject, pliquidList);
            if (pliquidList) {
                do {
                    if (!pliquidList->GetNext() && !pliquidList->m_item) {
                        break;
                    }
                    bhkLiquidAction* paction = pliquidList->m_item;
                    // EXE-INLINED none: bhkLiquidAction::Gethk
                    hkLiquidAction* pneighborLiquid =
                        paction ? static_cast<hkLiquidAction*>(paction->phkObject) : nullptr;
                    hkpEntity* pneighborEntity = pneighborLiquid ? pneighborLiquid->m_entity : nullptr;
                    bhkRigidBody* pneighbor =
                        pneighborEntity ? static_cast<bhkRigidBody*>(pneighborEntity->getUserData()) : nullptr;
                    hkVector4 kNeighborPosHK;
                    const hkVector4& rkNeighborPosHK = pneighbor->GetPosition(kNeighborPosHK);
                    float fX = rkNeighborPosHK.m_quad[0];
                    float fY = rkNeighborPosHK.m_quad[1];
                    float fZ = rkNeighborPosHK.m_quad[2];
                    NiPoint3 neighborPos;
                    neighborPos.x = fX * bhkConvert::fHk2BSScaleSC;
                    neighborPos.y = fY * bhkConvert::fHk2BSScaleSC;
                    neighborPos.z = bhkConvert::fHk2BSScaleSC * fZ;
                    alignas(16) NiPoint3 fromNeighbor = pos - neighborPos;
                    float flength = fromNeighbor.UnitizeGetLength();
                    if (flength > 0.0f && fNeighborDistance + fNeighborDistance > flength) {
                        float fneighborFactor =
                            (fNeighborDistance - flength) / (fNeighborDistance * fNeighborMaxPercentage);
                        if (fneighborFactor > 1.0) {
                            fneighborFactor = 1.0f;
                        }
                        if (fneighborFactor < -0.5) {
                            fneighborFactor = -0.5f;
                        }
                        fneighborFactor = fNeighborStrength * fneighborFactor;
                        NiPoint3 neighborForce = fromNeighbor * fneighborFactor;
                        if (bsticking) {
                            float fdot = StickNormal.Dot(neighborForce);
                            neighborForce -= StickNormal * fdot;
                        }
                        force += neighborForce;
                    }
                    pliquidList = pliquidList->GetNext();
                }
                while (pliquidList);
            }
            NiAVObject* pTarget = pTargetObject;
            if (pTarget) {
                // The stick force along the target's z axis, decaying each step.
                float fstick = -fInitialStickForce;
                NiPoint3 targetForce = pTarget->m_kWorld.m_Rotate * (NiPoint3_UNIT_Z * fstick);
                fInitialStickForce = fInitialStickForce * 0.8999999761581421;
                force += targetForce;
            }
            hkVector4 forceHK;
            forceHK.set(
                force.x * bhkConvert::fBS2HkScaleSC, force.y * bhkConvert::fBS2HkScaleSC,
                bhkConvert::fBS2HkScaleSC * force.z, 0.0f
            );
            static_cast<hkLiquidAction*>(phkObject)->Force.m_quad = forceHK.m_quad;
            if (NiPoint3_ZERO != force) {
                hkpEntity* phkEntity = static_cast<hkpEntity*>(pbody->phkObject);
                if (phkEntity) {
                    pbody->MarkWrite();
                    phkEntity->activate();
                    pbody->UnMarkWrite();
                }
            }
        }
    }
    bFirstUpdate = false;
}

// Makes apObject the object the action follows, then puts the action in the map's list for that object, making the list
// when the object has none yet.
void bhkLiquidAction::SetTargetObject(NiAVObject* apObject) {
    int32_t iMemContext = GetThreadMemContext();
    GetThreadMemContext() = MEM_CONTEXT_HAVOK;
    if (!apObject) {
        GetThreadMemContext() = iMemContext;
        return;
    }

    BSSimpleList<bhkLiquidAction*>* pList = nullptr;
    this->pTargetObject = apObject;
    LiquidListMap.GetAt(apObject, pList);
    if (!pList) {
        pList = static_cast<BSSimpleList<bhkLiquidAction*>*>(
            MemoryManager_Singleton.Allocate(sizeof(BSSimpleList<bhkLiquidAction*>))
        );
        if (pList) {
            pList->m_item = nullptr;
            pList->m_pkNext = nullptr;
        }
        LiquidListMap.SetAt(this->pTargetObject, pList);
        if (!pList) {
            GetThreadMemContext() = iMemContext;
            return;
        }
    }

    bhkLiquidAction* pAction = this;
    pList->AddHead(pAction);
    GetThreadMemContext() = iMemContext;
}

// The factory: a new action in the engine's heap, built in place.
NiObject* bhkLiquidAction::CreateObject() {
    void* pMemory = NiMemObject::operator new(sizeof(bhkLiquidAction));
    if (pMemory) {
        return ::new (pMemory) bhkLiquidAction;
    }
    return nullptr;
}

hkLiquidActionCinfo::hkLiquidActionCinfo()
  : fInitialStickForce(5000.0f), fStickStrength(15.0f), fNeighborDistance(64.0f), fNeighborStrength(15.0f) {}
