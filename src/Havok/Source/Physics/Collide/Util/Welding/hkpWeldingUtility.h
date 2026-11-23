#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

int32_t add1SaturateAndSetLowBit(int32_t value);

// Havok's contact welding helpers: snaps contact normals on triangle edges against the neighbour triangles' angles.
// The shapes store a WeldingType per triangle or mesh; the sin/cos table holds the sector bounds per edge angle.
class hkpWeldingUtility {
public:
    enum SectorType : int32_t {
        SNAP_0 = 0x0,
        ACCEPT_0 = 0x1,
        REJECT = 0x2,
        ACCEPT_1 = 0x3,
        SNAP_1 = 0x4,
    };

    enum WeldingType : int32_t {
        WELDING_TYPE_ANTICLOCKWISE = 0x0,
        WELDING_TYPE_CLOCKWISE = 0x4,
        WELDING_TYPE_TWO_SIDED = 0x5,
        WELDING_TYPE_NONE = 0x6,
    };

    enum NumAngles : int32_t {
        NUM_ANGLES = 0x1F,
    };

    // The sector bounds for one edge angle: the cosines and sines of the accept and snap angles.
    struct SinCosTableEntry {
        float m_cosAccept0;
        float m_sinAccept0;
        float m_cosSnap0;
        float m_cosSnap1;
        float m_cosAccept1;
        float m_sinAccept1;
    };

    // NUM_ANGLES + 1 entries: initWeldingTable fills the first NUM_ANGLES, and the shapes index the table with a
    // five-bit edge bitcode (0 to 31; hkpTriangleShape::weldContactPointImpl).
    using SinCosTable = SinCosTableEntry[NUM_ANGLES + 1];

    // Fills m_sinCosTable for NUM_ANGLES edge angles from -pi in steps of 0.2094.
    static void initWeldingTable(float edgeNormalSnapDeltaAngle, float triangleNormalSnapDeltaAngle);

    // The sector of edge angle edgeBitcode that the collision normal falls in, by its cosine to the triangle normal.
    static SectorType
    getSector(const hkVector4& triangleNormal, const hkVector4& collisionNormal, int32_t edgeBitcode) {
        const float cosAngle = triangleNormal.dot3(collisionNormal);
        const SinCosTableEntry& entry = m_sinCosTable[edgeBitcode];
        if (cosAngle >= entry.m_cosAccept0) {
            return ACCEPT_0;
        }
        if (cosAngle >= entry.m_cosSnap0) {
            return SNAP_0;
        }
        if (cosAngle >= entry.m_cosSnap1) {
            return REJECT;
        }
        if (cosAngle >= entry.m_cosAccept1) {
            return SNAP_1;
        }
        return ACCEPT_1;
    }

    // Whether a one-sided weld snaps the collision normal: the entry is read as an array of reals indexed by the
    // welding type, and an anticlockwise weld snaps below that cosine, a clockwise one above it.
    static hkBool shouldSnapOneSided(
        WeldingType weldingType,
        const hkVector4& triangleNormal,
        const hkVector4& collisionNormal,
        int32_t edgeBitcode
    ) {
        const float cosAngle = triangleNormal.dot3(collisionNormal);
        const float* sinCosTableEntry = &m_sinCosTable[edgeBitcode].m_cosAccept0;
        return (weldingType == WELDING_TYPE_ANTICLOCKWISE) ? cosAngle < sinCosTableEntry[weldingType]
                                                           : cosAngle > sinCosTableEntry[weldingType];
    }

    // The normalized direction of the sector's bound in the plane of the triangle normal and the edge's normal: the
    // entry is read as an array of reals, its cosine at sector and its sine after it.
    static void calcSnapVector(
        const hkVector4& triangleNormal,
        const hkVector4& edge,
        int32_t edgeBitcode,
        SectorType sector,
        hkVector4& snapVectorOut
    ) {
        hkVector4 orthNormalEdge;
        orthNormalEdge.setCross(edge, triangleNormal);
        const float* sinCosTableEntry = &m_sinCosTable[edgeBitcode].m_cosAccept0;
        hkVector4 hVec0;
        hVec0.setMul4(sinCosTableEntry[sector], triangleNormal);
        hkVector4 hVec1;
        hVec1.setMul4(sinCosTableEntry[sector + 1], orthNormalEdge);
        snapVectorOut.setAdd4(hVec0, hVec1);
        snapVectorOut.normalize3();
    }

    // calcSnapVector at the welding type's entries.
    static void calcSnapVectorOneSided(
        const hkVector4& triangleNormal,
        const hkVector4& edge,
        int32_t edgeBitcode,
        WeldingType weldingType,
        hkVector4& snapVectorOut
    ) {
        calcSnapVector(triangleNormal, edge, edgeBitcode, static_cast<SectorType>(weldingType), snapVectorOut);
    }

    // The collision normal moved onto the snap sector's bound, keeping its component along the edge, and normalized.
    static void snapCollisionNormal(
        const hkVector4& triangleNormal,
        const hkVector4& edge,
        int32_t edgeBitcode,
        SectorType sector,
        hkVector4& collisionNormalInOut
    ) {
        hkVector4 snapVector;
        calcSnapVector(triangleNormal, edge, edgeBitcode, sector, snapVector);
        const hkSimdReal dot0 = snapVector.dot3(collisionNormalInOut);
        const hkSimdReal dot1 = edge.dot3(collisionNormalInOut);
        hkVector4 hVec0;
        hVec0.setMul4(dot0, snapVector);
        hkVector4 hVec1;
        hVec1.setMul4(dot1, edge);
        collisionNormalInOut.setAdd4(hVec0, hVec1);
        collisionNormalInOut.normalize3();
    }
    // Havok SDK signature (hkpCollide.lib, tied by target).
    static const hkClass& staticClass();

    static inline SinCosTable m_sinCosTable{};
};

// The class's reflected enums. They are extern rather than file-local because the member tables of five other files
// hold their address.
using hkpWeldingUtilityEnumsArray = const hkClassEnum[3];
extern hkpWeldingUtilityEnumsArray hkpWeldingUtilityEnums;

// The reflection's WeldingType enum, by pointer (the first of the class's enum table). The mesh shapes' member tables
// store it into their weldingType members at start-up.
inline const hkClassEnum* hkpWeldingUtilityWeldingTypeEnum = &hkpWeldingUtilityEnums[0];

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpWeldingUtilityTypeInfo;
extern hkClass hkpWeldingUtilityClass;
