#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiBoundingVolume.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

class NiPoint3;

// a bounding volume made of several others
class NiUnionBV : public NiBoundingVolume {
public:
    NiUnionBV();
    // A clone of each of kABV's volumes.
    NiUnionBV(const NiUnionBV& kABV);

    void LoadBinary(NiStream& stream) override;
    void SaveBinary(NiStream& stream) override;
    ~NiUnionBV() override;
    // shared constant stub
    int32_t Type() const override {
        return UNION_BV;
    }
    uint32_t WhichObjectIntersect() const override {
        return m_uiWhichObjectIntersect;
    }
    void UpdateWorldData(const NiBoundingVolume& kModelABV, const NiTransform& kWorld) override;
    NiBoundingVolume* Create() const override;
    NiBoundingVolume* Clone() const override;
    void Copy(const NiBoundingVolume& kABV) override;
    bool operator==(const NiBoundingVolume& kABV) const override;
    // one body shared by every bounding volume: the negated operator==
    bool operator!=(const NiBoundingVolume& kABV) const override;
    void AddViewerStrings(const char* pcPrefix, NiTPrimitiveArray<char*>* pkStrings) const override;

    // Adds the volume unless it is null or already in the union.
    void AddBoundingVolume(NiBoundingVolume* pkABV);
    // Deletes every volume and empties the array.
    void RemoveAllBoundingVolumes();

    // The loader of ms_apfnLoaders[UNION_BV]: a new empty union that LoadBinary fills from the stream. Hides
    // NiBoundingVolume's, as in the engine.
    static NiBoundingVolume* CreateFromStream(NiStream& stream);
    // The first of kABV0's volumes that intersects kABV1 becomes the union's intersecting object. kABV0 is a
    // union (the find table's type: the source casts it).
    static bool UnionOtherFindIntersect(
        float fTime,
        const NiBoundingVolume& kABV0,
        const NiPoint3& kV0,
        const NiBoundingVolume& kABV1,
        const NiPoint3& kV1,
        float& fIntrTime,
        NiPoint3& kIntrPt,
        int32_t bCalcNormals,
        NiPoint3& kNormal0,
        NiPoint3& kNormal1
    );
    // The first of kABV0's volumes that intersects the triangle becomes the union's intersecting object.
    static bool UnionTriFindIntersect(
        float fTime,
        const NiBoundingVolume& kABV0,
        const NiPoint3& kV0,
        const NiPoint3& kVert0,
        const NiPoint3& kVert1,
        const NiPoint3& kVert2,
        const NiPoint3& kV1,
        float& fIntrTime,
        NiPoint3& kIntrPt,
        int32_t bCalcNormals,
        NiPoint3& kNormal0,
        NiPoint3& kNormal1
    );
    // kABV0 (both kABVs for UnionUnion) is a union, but the test table's type (NiBVTestIntersectFunction) has it as
    // const NiBoundingVolume&: the code casts it, also dropping const, because these functions write its
    // m_uiWhichObjectIntersect.
    static bool UnionOtherTestIntersect(
        float fTime,
        const NiBoundingVolume& kABV0,
        const NiPoint3& kV0,
        const NiBoundingVolume& kABV1,
        const NiPoint3& kV1
    );
    static bool UnionUnionTestIntersect(
        float fTime,
        const NiBoundingVolume& kABV0,
        const NiPoint3& kV0,
        const NiBoundingVolume& kABV1,
        const NiPoint3& kV1
    );
    static bool UnionTriTestIntersect(
        float fTime,
        const NiBoundingVolume& kABV0,
        const NiPoint3& kV0,
        const NiPoint3& kVert0,
        const NiPoint3& kVert1,
        const NiPoint3& kVert2,
        const NiPoint3& kV1
    );
    // The first pair of the two unions' volumes that intersects becomes each union's intersecting object.
    static bool UnionUnionFindIntersect(
        float fTime,
        const NiBoundingVolume& kABV0,
        const NiPoint3& kV0,
        const NiBoundingVolume& kABV1,
        const NiPoint3& kV1,
        float& fIntrTime,
        NiPoint3& kIntrPt,
        int32_t bCalcNormals,
        NiPoint3& kNormal0,
        NiPoint3& kNormal1
    );
    static void operator delete(void* apMemory);

    NiTPrimitiveArray<NiBoundingVolume*> m_kBoundingVolume;
    uint32_t m_uiWhichObjectIntersect;
};
