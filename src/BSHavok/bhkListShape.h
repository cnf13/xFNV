#pragma once

#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"

class hkpShape;

// The creation info every shape collection's starts with; it adds nothing to hkShapeCinfo.
struct hkShapeCollectionCinfo : public hkShapeCinfo {};

// The creation info of a list shape: the child shapes and each child's collision filter info.
struct hkListShapeCinfo : public hkShapeCollectionCinfo {
    // The serialized form (bhkSerialized.h): the 32-bit layout, 0x1C bytes. Both arrays are empty in the files:
    // LinkObject adds the children, LoadBinary reads the filter infos after the info.
    struct Serialized : hkShapeCinfo::Serialized {
        bhkSerializedArray m_childShapes;
        bhkSerializedArray m_childShapeFilterInfo;
    };

    ~hkListShapeCinfo();

    // Fills the info from its serialized form.
    void ReadSerialized(const Serialized& arForm) {
        hkShapeCinfo::ReadSerialized(arForm);
        arForm.m_childShapes.ToNative(m_childShapes);
        arForm.m_childShapeFilterInfo.ToNative(m_childShapeFilterInfo);
    }

    hkArray<const hkpShape*> m_childShapes;
    hkArray<uint32_t> m_childShapeFilterInfo;
};

static_assert(offsetof(hkListShapeCinfo::Serialized, m_childShapes) == 0x4);
static_assert(offsetof(hkListShapeCinfo::Serialized, m_childShapeFilterInfo) == 0x10);
static_assert(sizeof(hkListShapeCinfo::Serialized) == 0x1C);

// The Gamebryo side of a list shape: a collection of child shapes tested one after another. Its members are the Havok
// shape's, reached through phkObject; it declares no fields of its own.
class bhkListShape : public bhkShapeCollection {
public:
    bhkListShape();
    bhkListShape(const hkListShapeCinfo& arInfo);
    ~bhkListShape() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void Link(bool abAttach) override;
    // Frees the creation info when this shape made it.
    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override;
    // Reads the serialized form of the cinfo.
    int32_t LoadCInfo(NiStream& arStream) override;
    void Init(const hkSerializableCinfo& arInfo) override;
    // A new hkListShapeCinfo, filled from the wrapped list shape when there is one.
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
    void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;
    bool CanShare(NiCloningProcess& arCloneData) override;
    void BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) override;
    HavokMaterialType QSubMaterial(uint32_t aiKey) override;

    // The Havok shape of child aiAt.
    hkpShape* GetChildhkShape(int32_t aiAt);
    // The child shape aiAt.
    bhkShape* GetChildShape(int32_t aiAt);
    // The shape's user data, then each child's shape and collision filter info. It hides
    // bhkShape::CopyCinfo rather than overriding the base's virtual.
    void CopyCinfo(hkListShapeCinfo& arInfo);
    int32_t GetNumChildShapes() const;
    // Takes a reference on each child's bhkShape, last child first.
    void AddbhkShapeRef();
    // Drops the reference on each child's bhkShape, last child first, under the Havok heap context.
    void RembhkShapeRef();

    // Builds an empty list shape.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    static inline uint32_t iCount;
};
