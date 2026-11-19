#include "BSHavok/bhkRigidBodyT.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkConstraint.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkRigidBodyCinfo.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <xmmintrin.h>

// The class's RTTI name string.
using bhkRigidBodyT_Name_t = const char[14];

extern bhkRigidBodyT_Name_t bhkRigidBodyT_Name;

namespace bhkConvert {} // namespace bhkConvert

bhkRigidBodyT_Name_t bhkRigidBodyT_Name{"bhkRigidBodyT"};
NiRTTI bhkRigidBodyT::ms_RTTI("bhkRigidBodyT", &bhkRigidBody::ms_RTTI);

bhkRigidBodyT::bhkRigidBodyT() {
    iFlags = 0;
    ++iCount;
}

// Takes over apBody's Havok body, keeping its shape's wrapper referenced while it moves.
bhkRigidBodyT::bhkRigidBodyT(bhkRigidBody* apBody) {
    ++iCount;
    hkpRigidBody* prb = static_cast<hkpRigidBody*>(apBody->phkObject);
    prb->addReference();
    hkpRigidBody* phkBody = static_cast<hkpRigidBody*>(apBody->phkObject);
    const hkpShape* pShape;
    bhkShape* pUserData = nullptr;
    if (phkBody && (pShape = phkBody->m_collidable.m_shape) != nullptr) {
        pUserData = pShape->m_userData;
        if (pUserData) {
            NiIncRefCount(pUserData);
        }
    }
    apBody->SethkObject(nullptr);
    bhkSerializable::SethkObject(prb);
    NiDecRefCount(pUserData);
    prb->removeReference();
}

bhkRigidBodyT::~bhkRigidBodyT() {
    bhkRigidBody::Kill();
    --iCount;
}

// Charges the thread's allocations to Havok's heap context, holds the clone critical section, switches the Havok
// assertions off around the copy, builds the clone the way CreateObject does and restores the saved context.
NiObject* bhkRigidBodyT::CreateClone(NiCloningProcess& arCloneData) {
    int32_t& iContext = GetThreadMemContext();
    int32_t iSavedContext = iContext;
    iContext = MEM_CONTEXT_HAVOK;
    bhkSerializable::CloneCritLock();
    SetHavokAssertState(false);
    uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(bhkRigidBodyT) + 0x10));
    const uint8_t ucPadding = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
    pMemory += ucPadding;
    pMemory[-1] = ucPadding;
    bhkRigidBodyT* pClone = ::new (pMemory) bhkRigidBodyT;
    CopyMembers(pClone, arCloneData);
    SetHavokAssertState(true);
    bhkSerializable::CloneCritUnlock();
    iContext = iSavedContext;
    return pClone;
}

void bhkRigidBodyT::LoadBinary(NiStream& arStream) {
    NiStream* pStream = &arStream;
    bhkRigidBody::LoadBinary(*pStream);
    bool bCreated;
    CreateCinfo(bCreated);
}

// Writes the body's local offset into the creation info's position and rotation, saves through the base class and then
// drops the info. CreateCinfo and KillCinfo are virtual calls.
void bhkRigidBodyT::SaveBinary(NiStream& arStream) {
    bool bCreated;
    bhkRigidBodyCinfo* pCinfo = &static_cast<bhkRigidBodyCinfo&>(CreateCinfo(bCreated));
    pCinfo->hk.m_position = kLocalPos;
    pCinfo->hk.m_rotation.m_vec = kLocalRot.m_vec;
    bhkRigidBody::SaveBinary(arStream);
    KillCinfo(bCreated);
}

void bhkRigidBodyT::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiPoint3 pos;
    NiQuaternion rot;
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    bhkRigidBody::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkRigidBodyT::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v19 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v19);
    rot.m_fX = kLocalRot.m_vec.m_quad[0];
    rot.m_fY = kLocalRot.m_vec.m_quad[1];
    rot.m_fZ = kLocalRot.m_vec.m_quad[2];
    rot.m_fW = kLocalRot.m_vec.m_quad[3];
    HK2NI(pos, kLocalPos);
    char* v9 = rot.GetViewerString("Local Rot");
    uint32_t v10 = apStrings->m_usSize;
    uint32_t v11 = apStrings->m_usMaxSize;
    v19 = v9;
    if (v10 >= v11) {
        apStrings->SetSize(v10 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v10, v19);
    char* v12 = pos.GetViewerString("Local Pos");
    uint32_t v13 = apStrings->m_usSize;
    uint32_t v14 = apStrings->m_usMaxSize;
    v19 = v12;
    if (v13 >= v14) {
        apStrings->SetSize(v13 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v13, v19);
    v3->eMemContext = eMemContext;
}

// Takes the local offset from the creation info, resets the info's position and rotation to zero and identity, moves
// the body there through the base class and finishes through bhkSerializable. The info is written through a const_cast
// of its const reference.
void bhkRigidBodyT::PostInit(const hkSerializableCinfo& arInfo) {
    const bhkRigidBodyCinfo& rCinfo = static_cast<const bhkRigidBodyCinfo&>(arInfo);
    bhkRigidBodyCinfo& rTempCinfo = const_cast<bhkRigidBodyCinfo&>(rCinfo);
    kLocalPos = rCinfo.hk.m_position;
    kLocalRot = rCinfo.hk.m_rotation;
    rTempCinfo.hk.m_position = hkVector4::getZero();
    rTempCinfo.hk.m_rotation.setIdentity();
    bhkRigidBody::SetPosition(rTempCinfo.hk.m_position);
    bhkRigidBody::SetRotation(rTempCinfo.hk.m_rotation);
    bhkSerializable::PostInit(arInfo);
}

// The display geometry goes under a node named after the class: apNode itself when it has no name, else a new node
// attached to it and updated at time 0, or a new parentless node without apNode. The node is placed at the local offset
// converted to game units, then the base builds the geometry.
NiAVObject* bhkRigidBodyT::CreateDisplayGeometry(NiNode* apNode) {
    int32_t& iContext = GetThreadMemContext();
    int32_t iSavedContext = iContext;
    iContext = MEM_CONTEXT_HAVOK;
    NiNode* pNode = nullptr;
    if (apNode) {
        if (!apNode->m_kName.GetHandle()) {
            pNode = apNode;
        }
        else {
            void* pMemory = NiMemObject::operator new(sizeof(NiNode));
            if (pMemory) {
                pNode = ::new (pMemory) NiNode(0);
            }
            apNode->AttachChild(pNode, true);
            NiUpdateData kUpdateData;
            kUpdateData.fTime = 0.0f;
            kUpdateData.bUpdateControllers = false;
            kUpdateData.bParallelUpdate = false;
            kUpdateData.bFoundParticles = false;
            kUpdateData.bFoundMorphController = false;
            kUpdateData.bSceneGraphChange = false;
            pNode->Update(kUpdateData);
        }
    }
    else {
        void* pMemory = NiMemObject::operator new(sizeof(NiNode));
        if (pMemory) {
            pNode = ::new (pMemory) NiNode(0);
        }
    }
    pNode->SetName(NiFixedString(bhkRigidBodyT_Name));
    hkRotation kRotation;
    kRotation.set(kLocalRot);
    hkVector4 kPosition;
    kPosition.m_quad = kLocalPos.m_quad;
    NiMatrix3 kNiRotation;
    HK2NI(kNiRotation, kRotation);
    alignas(16) NiPoint3 kNiPosition;
    HK2NI(kNiPosition, kPosition);
    pNode->m_kLocal.m_Rotate = kNiRotation;
    pNode->m_kLocal.m_Translate.x = kNiPosition.x;
    pNode->m_kLocal.m_Translate.y = kNiPosition.y;
    pNode->m_kLocal.m_Translate.z = kNiPosition.z;
    NiAVObject* pResult = bhkRigidBody::CreateDisplayGeometry(pNode);
    iContext = iSavedContext;
    return pResult;
}

hkVector4& bhkRigidBodyT::GetPosition(hkVector4& arPos) {
    if (this && phkObject) {
        hkQuaternion hkq;
        GetRotation(hkq);
        __m128 v3 = _mm_mul_ps(kLocalPos.m_quad, hkq.m_vec.m_quad);
        __m128 v4 = _mm_shuffle_ps(v3, v3, 170);
        v4[0] = v4[0] + (_mm_shuffle_ps(v3, v3, 85)[0] + v3[0]);
        __m128 v5 = _mm_shuffle_ps(hkq.m_vec.m_quad, hkq.m_vec.m_quad, 255);
        __m128 v8 = _mm_add_ps(
            _mm_mul_ps(
                _mm_sub_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(kLocalPos.m_quad, kLocalPos.m_quad, 210),
                        _mm_shuffle_ps(hkq.m_vec.m_quad, hkq.m_vec.m_quad, 201)
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(kLocalPos.m_quad, kLocalPos.m_quad, 201),
                        _mm_shuffle_ps(hkq.m_vec.m_quad, hkq.m_vec.m_quad, 210)
                    )
                ),
                v5
            ),
            _mm_add_ps(
                _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v5, v5), hkQuadRealMinusHalf), kLocalPos.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v4, v4, 0), hkq.m_vec.m_quad)
            )
        );
        bhkRigidBody::GetPosition(arPos);
        arPos.m_quad = _mm_sub_ps(arPos.m_quad, _mm_add_ps(v8, v8));
        return arPos;
    }

    arPos.m_quad = hkVector4Zero.m_quad;
    return arPos;
}

hkQuaternion& bhkRigidBodyT::GetRotation(hkQuaternion& arRot) {
    hkQuaternion hkr;
    bhkRigidBody::GetRotation(hkr);
    arRot.setMulInverse(hkr, kLocalRot);
    return arRot;
}

void bhkRigidBodyT::SetPosition(const hkVector4& arPos) {
    if ((this) && (phkObject)) {
        hkQuaternion hkq;
        hkVector4 hkp;
        GetRotation(hkq);
        __m128 v3 = _mm_mul_ps(kLocalPos.m_quad, hkq.m_vec.m_quad);
        __m128 v4 = _mm_shuffle_ps(v3, v3, 170);
        v4[0] = v4[0] + (_mm_shuffle_ps(v3, v3, 85)[0] + v3[0]);
        __m128 v5 = _mm_shuffle_ps(hkq.m_vec.m_quad, hkq.m_vec.m_quad, 255);
        __m128 v6 = _mm_add_ps(
            _mm_mul_ps(
                _mm_sub_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(kLocalPos.m_quad, kLocalPos.m_quad, 210),
                        _mm_shuffle_ps(hkq.m_vec.m_quad, hkq.m_vec.m_quad, 201)
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(kLocalPos.m_quad, kLocalPos.m_quad, 201),
                        _mm_shuffle_ps(hkq.m_vec.m_quad, hkq.m_vec.m_quad, 210)
                    )
                ),
                v5
            ),
            _mm_add_ps(
                _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v5, v5), hkQuadRealMinusHalf), kLocalPos.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v4, v4, 0), hkq.m_vec.m_quad)
            )
        );
        hkp.m_quad = _mm_add_ps(_mm_add_ps(v6, v6), arPos.m_quad);
        bhkRigidBody::SetPosition(hkp);
    }
}

// Sets the Havok body's rotation to arRot times the local rotation.
void bhkRigidBodyT::SetRotation(const hkQuaternion& arRot) {
    hkQuaternion hkq;
    hkq.setMul(arRot, kLocalRot);
    bhkRigidBody::SetRotation(hkq);
}

// Sets the Havok body's position to arPos plus the local translation rotated by arRot, and its rotation to arRot times
// the local rotation, normalized.
void bhkRigidBodyT::SetPosRot(const hkVector4& arPos, const hkQuaternion& arRot) {
    hkQuaternion hkq;
    hkVector4 hkp;
    hkp.setRotatedDir(arRot, kLocalPos);
    hkp.add4(arPos);
    hkq.setMul(arRot, kLocalRot);
    hkq.normalize();
    bhkRigidBody::SetPosRot(hkp, hkq);
}

// The base body's center of mass, moved back out of the body frame by the local position rotated into it.
hkVector4& bhkRigidBodyT::GetCenterOfMassInWorld(hkVector4& arCOM) {
    bhkRigidBody::GetCenterOfMassInWorld(arCOM);
    if (this) {
        hkpRigidBody* pBody = static_cast<hkpRigidBody*>(phkObject);
        if (pBody) {
            hkVector4 kOffset;
            kOffset.setMul3(pBody->m_motion.m_motionState.m_transform.m_rotation, kLocalPos);
            arCOM.m_quad[0] = arCOM.m_quad[0] - kOffset.m_quad[0];
            arCOM.m_quad[1] = arCOM.m_quad[1] - kOffset.m_quad[1];
            arCOM.m_quad[2] = arCOM.m_quad[2] - kOffset.m_quad[2];
            arCOM.m_quad[3] = arCOM.m_quad[3] - kOffset.m_quad[3];
        }
    }
    return arCOM;
}

hkTransform& bhkRigidBodyT::GetTransform(hkTransform& arTrans) {
    hkQuaternion hkrot;
    GetPosition(arTrans.m_translation);
    GetRotation(hkrot);
    arTrans.m_rotation.set(hkrot);
    return arTrans;
}

// Copies the local rotation and translation into the creation info, scales the translation by the clone's scale when it
// is not 1, then copies through the base class. The clone process is always a BSCloningProcess here; its Scale.x is
// compared with 1.0.
void bhkRigidBodyT::CopyMembers(bhkSerializable* pDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    bhkRigidBodyCinfo& rCinfo = static_cast<bhkRigidBodyCinfo&>(CreateCinfo(bCreated));
    rCinfo.hk.m_rotation = kLocalRot;
    rCinfo.hk.m_position = kLocalPos;
    float fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x;
    if (1.0f != fScale) {
        __m128 kScale = _mm_set_ss(fScale);
        rCinfo.hk.m_position.m_quad = _mm_mul_ps(_mm_shuffle_ps(kScale, kScale, 0), rCinfo.hk.m_position.m_quad);
    }
    bhkRigidBody::CopyMembers(pDest, arCloneData);
}

const hkVector4& bhkRigidBodyT::GetLocalPos() {
    return kLocalPos;
}

const hkQuaternion& bhkRigidBodyT::GetLocalRot() {
    return kLocalRot;
}

// The class loader's factory: allocates the object with 16 bytes of slack and aligns it to 16 in place, storing the
// padding length in the byte before it, then calls the default constructor.
NiObject* bhkRigidBodyT::CreateObject() {
    uint8_t* pMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(bhkRigidBodyT) + 0x10));
    const uint8_t ucPadding = static_cast<uint8_t>(0x10 - (reinterpret_cast<uintptr_t>(pMemory) & 0xF));
    pMemory += ucPadding;
    pMemory[-1] = ucPadding;
    return ::new (pMemory) bhkRigidBodyT;
}

void bhkRigidBodyT::operator delete(void* apMemory) {
    if (apMemory) {
        uint8_t* pMemory = static_cast<uint8_t*>(apMemory);
        MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
    }
}
