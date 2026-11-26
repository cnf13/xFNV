#pragma once

#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

struct hkBvTreeShapeCinfo;
class hkpShapeCollection;

// The Gamebryo side of a bounding volume tree shape (a shape that finds the shape it was built over through its
// collection).
class bhkBvTreeShape : public bhkShape {
public:
    bhkBvTreeShape();

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void Link(bool abAttach) override;
    void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;
    bhkShapeCollection* FindShapeCollection() override;
    bool CanShare(NiCloningProcess& arCloneData) override;
    void BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) override;
    void DestroyDisplayGeometry() override;
    ~bhkBvTreeShape() override;

    // The shape collection the tree is built over.
    bhkShapeCollection* GetShapeCollection();
    // The Havok tree's container as its shape collection, or null.
    hkpShapeCollection* GethkShapeCollection() const;
    // Calls bhkShape::CopyCinfo, then copies the shape collection into arInfo. The derived trees' CopyCinfo call it
    // qualified.
    void CopyCinfo(hkBvTreeShapeCinfo& arInfo);
    // Drops the tree's reference to its collection's wrapper.
    void RembhkShapeRef();

    static void RegisterRTTI();

    static NiRTTI ms_RTTI;
    // Number of live instances; the constructors count them.
    static inline uint32_t iCount;
};

// The creation info of a bounding volume tree shape: the shape collection it is built over.
struct hkBvTreeShapeCinfo : public hkShapeCinfo {
    // The serialized form (bhkSerialized.h): the 32-bit layout, 8 bytes.
    struct Serialized : hkShapeCinfo::Serialized {
        bhkSerializedPointer m_shapeCollection;
    };

    hkBvTreeShapeCinfo();

    // Fills this part from its serialized form; the collection is left null (LinkObject binds it).
    void ReadSerialized(const Serialized& arForm) {
        hkShapeCinfo::ReadSerialized(arForm);
        m_shapeCollection = nullptr;
    }

    hkpShapeCollection* m_shapeCollection;
};

static_assert(sizeof(hkBvTreeShapeCinfo::Serialized) == 0x8);

inline bhkBvTreeShape::bhkBvTreeShape() {
    ++iCount;
}

inline bhkShapeCollection* bhkBvTreeShape::GetShapeCollection() {
    hkpShape* pShape = GethkShapeCollection();
    if (pShape) {
        return static_cast<bhkShapeCollection*>(bhkShapeCollection::Getbhk(pShape));
    }

    return nullptr;
}

inline hkpShapeCollection* bhkBvTreeShape::GethkShapeCollection() const {
    // bhkBvTreeShape::operator* is bhkShape's null-safe getter.
    const hkpShape* pShape = **this;
    if (!pShape) {
        return nullptr;
    }
    // The collection's container is its second base; static_cast keeps the null test.
    return const_cast<hkpShapeCollection*>(static_cast<const hkpShapeCollection*>(pShape->getContainer()));
}
