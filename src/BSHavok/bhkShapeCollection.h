#pragma once

#include "BSHavok/bhkShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"

// The Gamebryo side of a shape made of child shapes, each reached through a key (a list shape, a mesh shape).
class bhkShapeCollection : public bhkShape {
public:
    bhkShapeCollection() {
        ++iCount;
    }
    ~bhkShapeCollection() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    bhkShapeCollection* FindShapeCollection() override {
        return this;
    }
    bool CanShare(NiCloningProcess& arCloneData) override;
    void BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) override;

    // The material of the child shape with key auiKey.
    virtual HavokMaterialType QSubMaterial(uint32_t auiKey) = 0;

    // The Havok shape, or null for a null this.
    hkpShapeCollection* operator*() const;
    // The Havok collection's first child key, and the key after auiKey.
    uint32_t GetFirstKey();
    uint32_t GetNextKey(uint32_t aiOldKey);

    static bhkShape* Getbhk(hkpShape* apShape);

    static NiRTTI ms_RTTI;
    static inline uint32_t iCount;
};

inline hkpShapeCollection* bhkShapeCollection::operator*() const {
    if (this) {
        return static_cast<hkpShapeCollection*>(const_cast<bhkShapeCollection*>(this)->Gethk());
    }

    return nullptr;
}

// The container base is taken without a null test, so it goes through a reference.
inline uint32_t bhkShapeCollection::GetFirstKey() {
    const hkpShapeContainer& kContainer = ***this;
    return kContainer.getFirstKey();
}

inline uint32_t bhkShapeCollection::GetNextKey(uint32_t aiOldKey) {
    const hkpShapeContainer& kContainer = ***this;
    return kContainer.getNextKey(aiOldKey);
}

inline bhkShape* bhkShapeCollection::Getbhk(hkpShape* apShape) {
    return bhkShape::Getbhk(apShape);
}
