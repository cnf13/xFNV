#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleArrayRefCounted.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <stdint.h>

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;

// A place a path should keep away from: a sphere around Point1, or a cylinder from Point1 to Point2, and what it costs
// to cross it.
class PathingAvoidNode {
public:
    enum AVOID_NODE_TYPE : int32_t {
        AVOID_NODE_SPHERE = 0x0,
        AVOID_NODE_CYLINDER = 0x1,
        AVOID_NODE_SPHERE_ACTOR = 0x2,
        AVOID_NODE_SPHERE_TARGET = 0x3,
        AVOID_NODE_SPHERE_THREAT = 0x4,
        AVOID_NODE_SPHERE_OBSTACLE = 0x5,
    };

    PathingAvoidNode();
    PathingAvoidNode(const NiPoint3& arPoint1, const NiPoint3& arPoint2, float afRadius, float afCost);
    PathingAvoidNode(const NiPoint3& arPoint, float afRadius, float afCost);
    PathingAvoidNode(const NiPoint3& arPoint, float afRadius, float afCost, AVOID_NODE_TYPE aeType);

    // The cost of passing at arPoint.
    float GetCostForPoint(NiPoint3& arPoint);
    // Writes the type as a byte, then the radius, cost and points; Point2 only for a cylinder.
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    // Reads what SaveGame writes.
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    // The node itself is read as Point1.
    NiPoint3* GetPoint1() {
        return &Point1;
    }
    float GetRadius() {
        return fRadius;
    }
    // eType, through the shared +0x20 getter.
    AVOID_NODE_TYPE GetType() {
        return eType;
    }
    void SetRadius(float afRadius);
    void SetCost(float afCost);

    NiPoint3 Point1;
    NiPoint3 Point2;
    float fRadius;
    float fCost;
    AVOID_NODE_TYPE eType;
};

// A pathing request's places to avoid. Only some of the members are declared.
class PathingAvoidNodeArray : public BSSimpleArrayRefCounted<PathingAvoidNode, 1024> {
public:
    // An empty array.
    PathingAvoidNodeArray();
    ~PathingAvoidNodeArray() override;

    // The node at auiIndex, through GetAtInternal.
    PathingAvoidNode& GetAt(uint32_t auiIndex);
    // The radius sum of the nodes; arCenter gets their cost-weighted center, each cylinder counted at the point of
    // its axis closest to arStartPoint.
    float ComputeCenter(const NiPoint3& arStartPoint, NiPoint3& arCenter);
    float GetCostForPoint(NiPoint3& arPoint);
    PathingAvoidNode& GetAtInternal(int32_t a1);
};

NiSmartPointer(PathingAvoidNodeArray);

// NiPointer's constructor from a raw pointer; defined in Pathfinding/PathSmootherPOV.cpp.
NiPointer<PathingAvoidNodeArray>*
NiPointer_PathingAvoidNodeArray_Construct(void* apMemory, PathingAvoidNodeArray* apObject);

using BSSimpleArray_PathingAvoidNode_1024 = BSSimpleArray<PathingAvoidNode, 1024>;
