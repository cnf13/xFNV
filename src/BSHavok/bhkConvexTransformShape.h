#pragma once

#include "BSHavok/bhkConvexShape.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTransform/hkpConvexTransformShape.h"

#include <stdint.h>

struct hkConvexTransformShapeCinfo;

// The Gamebryo side of Havok's convex transform shape: a convex shape placed by a transform. It adds no members of its
// own.
class bhkConvexTransformShape : public bhkConvexShape {
public:
    bhkConvexTransformShape();
    ~bhkConvexTransformShape() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void Link(bool abAttach) override;
    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override;
    // This override reads the serialized form instead of reading the cinfo raw through bhkSerializable::LoadCInfo.
    int32_t LoadCInfo(NiStream& arStream) override;
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
    void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;
    bool CanShare(NiCloningProcess& arCloneData) override;
    void BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) override;

    // Fills arInfo from the wrapped Havok shape: the base's part, the child shape and the transform.
    void CopyCinfo(hkConvexTransformShapeCinfo& arInfo);
    // Releases the reference the shape holds on its child shape's wrapper.
    void RembhkShapeRef();

    // Returns the child Havok shape's wrapper, or null. Callers test this, the Havok shape and its child for null in
    // turn, reloading phkObject at each use.
    bhkShape* GetChildShape() const {
        if (!this) {
            return nullptr;
        }
        const hkpConvexTransformShape* pShape = static_cast<const hkpConvexTransformShape*>(phkObject);
        if (!pShape) {
            return nullptr;
        }
        const hkpConvexShape* pChildShape = pShape->getChildShape();
        if (!pChildShape) {
            return nullptr;
        }
        return pChildShape->m_userData;
    }

    // The class's factory entry: a default constructed shape.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    static inline int32_t iCount;
};

// The creation info of a convex transform shape: the child shape and the transform it is placed by. CreateCinfo writes
// an identity hkTransform, CopyCinfo copies the shape's hkTransform there and Init passes it to
// hkpConvexTransformShape's constructor as one.
struct hkConvexTransformShapeCinfo : public hkConvexShapeCinfo {
    // The serialized form (bhkSerialized.h): the 32-bit layout, 0x50 bytes.
    struct Serialized : hkConvexShapeCinfo::Serialized {
        bhkSerializedPointer m_childShape;
        uint8_t padding0C[4];
        hkTransform m_transform;
    };

    // Fills the info from its serialized form; the child is left null (LinkObject binds it).
    void ReadSerialized(const Serialized& arForm) {
        hkConvexShapeCinfo::ReadSerialized(arForm);
        m_childShape = nullptr;
        m_transform = arForm.m_transform;
    }

    hkpConvexShape* m_childShape;
    hkTransform m_transform;
};

static_assert(offsetof(hkConvexTransformShapeCinfo::Serialized, m_transform) == 0x10);
static_assert(sizeof(hkConvexTransformShapeCinfo::Serialized) == 0x50);
