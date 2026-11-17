#pragma once

#include "BSHavok/bhkConvexShape.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"

#include <stdint.h>

struct hkConvexListShapeCinfo;
class hkpCdVertex;
class hkpConvexShape;

// The scene graph wrapper of a convex list shape: a shape made of several convex pieces.
class bhkConvexListShape : public bhkConvexShape {
public:
    bhkConvexListShape();
    ~bhkConvexListShape() override;

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
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
    void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;
    bool CanShare(NiCloningProcess& arCloneData) override;
    void BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) override;
    virtual void GetSupportingVertex(const hkVector4& arDir, hkpCdVertex& arSupportingVertexOut);
    virtual void ConvertVertexIdsToVertices(const uint16_t* pIds, int32_t inumIds, hkpCdVertex* pverticesOut);
    virtual void GetFirstVertex(hkVector4& arVertex);
    virtual HavokMaterialType QSubMaterial(uint32_t aiKey);

    // Copies the wrapped shape's data into the creation info. It is not the base's virtual CopyCinfo: CreateCinfo calls
    // this one directly.
    void CopyCinfo(hkConvexListShapeCinfo& arInfo);
    // The class adds no data of its own beyond the base's pInfo and eMaterial. AddbhkShapeRef adds a reference to every
    // child's bhkShape.
    void AddbhkShapeRef();
    // Releases the references AddbhkShapeRef took.
    void RembhkShapeRef();

    // Builds an empty convex list shape.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
    static inline int32_t iCount;
};

// The creation info of a convex list shape: the child shapes it is built over, whether it caches its AABB and the
// distance from which closest-point queries use the children's convex hull.
struct hkConvexListShapeCinfo : public hkConvexShapeCinfo {
    bool m_useCachedAabb;
    float m_minDistanceToUseConvexHullForGetClosestPoints;
    hkArray<const hkpConvexShape*> m_childShapes;
};
