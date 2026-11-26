#include "BSHavok/bhkPCollisionObject.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"
#include "xfnv/Compiler.h"

#include <xmmintrin.h>

namespace bhkConvert {} // namespace bhkConvert

NiRTTI bhkPCollisionObject::ms_RTTI("bhkPCollisionObject", &bhkNiCollisionObject::ms_RTTI);

bhkPCollisionObject::bhkPCollisionObject() {}

bhkPCollisionObject::bhkPCollisionObject(NiAVObject* apObject) : bhkNiCollisionObject(apObject) {}

// The clone's allocation is charged to the Havok heap context.
NiObject* bhkPCollisionObject::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkPCollisionObject* pClone = new bhkPCollisionObject;
    // Copies even into a null clone.
    CopyMembers(pClone, arCloneData);
    return pClone;
}

// Bethesda stream version 21 and later carry the Havok collision object's data; older streams only the base's.
void bhkPCollisionObject::LoadBinary(NiStream& arStream) {
    if (arStream.m_BSStreamHeader.uiVersion >= 0x15) {
        bhkNiCollisionObject::LoadBinary(arStream);
    }
    else {
        NiCollisionObject::LoadBinary(arStream);
    }
}

// The links follow the same version split as LoadBinary.
void bhkPCollisionObject::LinkObject(NiStream& arStream) {
    if (arStream.m_BSStreamHeader.uiVersion >= 0x15) {
        bhkNiCollisionObject::LinkObject(arStream);
    }
    else {
        NiCollisionObject::LinkObject(arStream);
    }
}

bool bhkPCollisionObject::RegisterStreamables(NiStream& arStream) {
    return bhkNiCollisionObject::RegisterStreamables(arStream);
}

void bhkPCollisionObject::SaveBinary(NiStream& arStream) {
    bhkNiCollisionObject::SaveBinary(arStream);
}

bool bhkPCollisionObject::IsEqual(NiObject* apObject) {
    return bhkNiCollisionObject::IsEqual(apObject);
}

void bhkPCollisionObject::AddViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkNiCollisionObject::AddViewerStrings(apStrings);
    char* pcString = NiGetViewerString(bhkPCollisionObject::ms_RTTI.m_pcName);
    apStrings->Add(pcString);
}

// Sets the scene object's transform from the phantom's AABB: an identity rotation, unit scale and
// half the AABB's extent in game units as the translation (the AABB's centre is never read).
void bhkPCollisionObject::hktoNiTransform() {
    bhkRigidBody* pWorldObject = spWorldObject;
    if (pWorldObject) {
        hkReferencedObject* phkObject = pWorldObject->phkObject;
        if (phkObject) {
            NiTransform kTransform;
            hkAabb kAabb;
            kTransform.m_Rotate.m_pEntry[0][0] = 1.0f;
            kTransform.m_Rotate.m_pEntry[0][1] = 0.0f;
            kTransform.m_Rotate.m_pEntry[0][2] = 0.0f;
            kTransform.m_Rotate.m_pEntry[1][0] = 0.0f;
            kTransform.m_Rotate.m_pEntry[1][2] = 0.0f;
            kTransform.m_Rotate.m_pEntry[2][0] = 0.0f;
            kTransform.m_Rotate.m_pEntry[2][1] = 0.0f;
            kTransform.m_Rotate.m_pEntry[1][1] = 1.0f;
            kTransform.m_Rotate.m_pEntry[2][2] = 1.0f;
            kTransform.m_fScale = 1.0f;
            // The world object of a phantom collision object holds an hkpPhantom.
            static_cast<hkpPhantom*>(phkObject)->calcAabb(kAabb);
            hkVector4 kHalfExtent;
            kHalfExtent.m_quad = _mm_mul_ps(
                _mm_shuffle_ps(_mm_set_ss(fHalf), _mm_set_ss(fHalf), 0),
                _mm_sub_ps(kAabb.m_max.m_quad, kAabb.m_min.m_quad)
            );
            kAabb.m_min.m_quad = _mm_add_ps(kHalfExtent.m_quad, kAabb.m_min.m_quad);
            HK2NI(kTransform.m_Translate, kHalfExtent);
            SetTransform(kTransform);
        }
    }
}

// Sets the phantom's AABB from the scene object's world transform: the AABB keeps its extent and is centred on the
// world translation in Havok units. The world transform is copied whole before the phantom's AABB is read; the store
// runs under the keyframe lock.
void bhkPCollisionObject::NitohkTransform() {
    bhkRigidBody* pWorldObject = spWorldObject;
    NiAVObject* pSceneObject = m_pkSceneObject;
    if (pSceneObject) {
        pSceneObject->UpdateWorldLoc();
        if (pWorldObject) {
            hkReferencedObject* phkObject = pWorldObject->phkObject;
            if (phkObject) {
                NiTransform kWorld = pSceneObject->m_kWorld;
                hkAabb kAabb;
                // The world object of a phantom collision object holds an hkpPhantom.
                static_cast<hkpPhantom*>(phkObject)->calcAabb(kAabb);
                __m128 kHalfExtent = _mm_mul_ps(
                    _mm_shuffle_ps(_mm_set_ss(fHalf), _mm_set_ss(fHalf), 0),
                    _mm_sub_ps(kAabb.m_max.m_quad, kAabb.m_min.m_quad)
                );
                hkVector4 kCenter;
                XFNV_M128_F32(kCenter.m_quad, 0) = kWorld.m_Translate.x * bhkConvert::fBS2HkScaleSC;
                XFNV_M128_F32(kCenter.m_quad, 1) = kWorld.m_Translate.y * bhkConvert::fBS2HkScaleSC;
                XFNV_M128_F32(kCenter.m_quad, 2) = bhkConvert::fBS2HkScaleSC * kWorld.m_Translate.z;
                XFNV_M128_F32(kCenter.m_quad, 3) = 0.0f;
                kAabb.m_min.m_quad = _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(-1.0f), _mm_set_ss(-1.0f), 0), kHalfExtent),
                    kCenter.m_quad
                );
                kAabb.m_max.m_quad = _mm_add_ps(kCenter.m_quad, kHalfExtent);
                bhkNiCollisionObject::kKeyframeLock.Lock(nullptr);
                hkpAabbPhantom* pPhantom = static_cast<hkpAabbPhantom*>(pWorldObject->phkObject);
                if (pPhantom) {
                    pWorldObject->MarkWrite();
                    pPhantom->setAabb(kAabb);
                    pWorldObject->UnMarkWrite();
                }
                bhkNiCollisionObject::kKeyframeLock.Unlock();
            }
        }
    }
}

bool bhkPCollisionObject::QKeyframe() {
    return true;
}

// Copies the base's members, the flags and a clone of the world object into apDest. The clone lookup is
// NiObject::CreateSharedClone written out.
void bhkPCollisionObject::CopyMembers(bhkPCollisionObject* apDest, NiCloningProcess& arCloneData) {
    NiObject::CopyMembers(apDest, arCloneData);
    apDest->usFlags = usFlags;
    bhkRigidBody* pWorldObject = spWorldObject;
    if (pWorldObject) {
        NiPointer<bhkWorldObject> spSource(pWorldObject);
        NiPointer<bhkWorldObject> spClone;
        // pkClone starts as apDest, so a lookup that succeeds without writing its result leaves apDest.
        NiObject* pkClone = apDest;
        if (!arCloneData.m_pkCloneMap->GetAt(pWorldObject, pkClone)) {
            pkClone = pWorldObject->CreateClone(arCloneData);
        }
        spClone.SetNew(static_cast<bhkWorldObject*>(pkClone));
        apDest->SetWorldObject(spClone);
    }
}

// The class loader's factory: a bare bhkPCollisionObject block, or null when the allocation fails.
NiObject* bhkPCollisionObject::CreateObject() {
    return new bhkPCollisionObject;
}

bhkPCollisionObject::~bhkPCollisionObject() {}
