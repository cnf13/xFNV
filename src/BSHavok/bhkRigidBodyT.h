#pragma once

#include "BSHavok/bhkRigidBody.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"

#include <stdint.h>

// A rigid body whose Havok transform is offset from its scene graph object by a local rotation and translation.
class bhkRigidBodyT : public bhkRigidBody {
public:
    bhkRigidBodyT();
    bhkRigidBodyT(bhkRigidBody* apBody);
    ~bhkRigidBodyT() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void PostInit(const hkSerializableCinfo& arInfo) override;
    NiAVObject* CreateDisplayGeometry(NiNode* apNode) override;
    hkVector4& GetPosition(hkVector4& arPos) override;
    hkQuaternion& GetRotation(hkQuaternion& arRot) override;
    void SetPosition(const hkVector4& arPos) override;
    void SetRotation(const hkQuaternion& arRot) override;
    void SetPosRot(const hkVector4& arPos, const hkQuaternion& arRot) override;
    hkVector4& GetCenterOfMassInWorld(hkVector4& arCOM) override;
    hkTransform& GetTransform(hkTransform& arTrans) override;

    // Copies the body's members onto pDest.
    void CopyMembers(bhkSerializable* pDest, NiCloningProcess& arCloneData);
    const hkVector4& GetLocalPos();
    const hkQuaternion& GetLocalRot();

    // The class loader's factory: a 16-byte aligned body run through the default constructor.
    static NiObject* CreateObject();
    // The block is 16-aligned: the padding byte in front of the object gives the distance back to its start.
    static void operator delete(void* apMemory);

    // Aligned to 16 bytes like the Havok types they hold.
    uint8_t padding1C[4];
    hkQuaternion kLocalRot;
    hkVector4 kLocalPos;

    static NiRTTI ms_RTTI;
    static inline int32_t iCount;
};
