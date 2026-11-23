#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"

#include "BSCore/BSSimpleArray.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <limits>

// Leaves every field unset.
PathingAvoidNode::PathingAvoidNode() {}

PathingAvoidNode::PathingAvoidNode(const NiPoint3& arPoint1, const NiPoint3& arPoint2, float afRadius, float afCost)
  : Point1(arPoint1), Point2(arPoint2), fRadius(afRadius), fCost(afCost), eType(AVOID_NODE_CYLINDER) {}

PathingAvoidNode::PathingAvoidNode(const NiPoint3& arPoint, float afRadius, float afCost)
  : Point1(arPoint), fRadius(afRadius), fCost(afCost), eType(AVOID_NODE_SPHERE) {}

PathingAvoidNode::PathingAvoidNode(const NiPoint3& arPoint, float afRadius, float afCost, AVOID_NODE_TYPE aeType)
  : Point1(arPoint), fRadius(afRadius), fCost(afCost), eType(aeType) {}

float PathingAvoidNode::GetCostForPoint(NiPoint3& arPoint) {
    NiPoint3 kAxis;
    NiPoint3 kToPoint;
    float fAxisLengthSqr;
    float fDot;
    NiPoint3 kDelta;
    float fRadiusSqr = fRadius * fRadius;
    float fDistSqr = std::numeric_limits<float>::max();
    PathingAvoidNode::AVOID_NODE_TYPE eType = this->eType;
    switch (eType) {
        case PathingAvoidNode::AVOID_NODE_SPHERE:
        case PathingAvoidNode::AVOID_NODE_SPHERE_ACTOR: {
            kDelta = Point1 - arPoint;
            fDistSqr = kDelta.SqrLength();
            break;
        }
        case PathingAvoidNode::AVOID_NODE_CYLINDER: {
            kAxis = Point2 - Point1;
            kToPoint = arPoint - Point1;
            fAxisLengthSqr = kAxis.SqrLength();
            fDot = kToPoint.Dot(kAxis);
            if (fDot < 0.0 || fAxisLengthSqr < static_cast<double>(fDot)) {
                fDistSqr = std::numeric_limits<float>::max();
            }
            else {
                double dToPointSqr = kToPoint.SqrLength();
                fDistSqr = dToPointSqr - fDot * fDot / fAxisLengthSqr;
            }
            break;
        }
        default: {
            break;
        }
    }
    if (fRadiusSqr > static_cast<double>(fDistSqr)) {
        return (1.0 - fDistSqr / fRadiusSqr) * fCost;
    }

    return 0.0f;
}

void PathingAvoidNode::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint8_t cType = static_cast<uint8_t>(eType);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cType), sizeof(cType), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fRadius), sizeof(fRadius), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fCost), sizeof(fCost), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&Point1), sizeof(Point1), 0);
    if (eType == AVOID_NODE_CYLINDER) {
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&Point2), sizeof(Point2), 0);
    }
}

// Reads what SaveGame writes.
void PathingAvoidNode::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint8_t cType = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cType), sizeof(cType));
    eType = static_cast<AVOID_NODE_TYPE>(cType);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fRadius), sizeof(fRadius));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fCost), sizeof(fCost));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&Point1), sizeof(Point1));
    if (eType == AVOID_NODE_CYLINDER) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&Point2), sizeof(Point2));
    }
}

void PathingAvoidNode::SetRadius(float afRadius) {
    fRadius = afRadius;
}

void PathingAvoidNode::SetCost(float afCost) {
    fCost = afCost;
}

PathingAvoidNodeArray::PathingAvoidNodeArray() : BSSimpleArrayRefCounted<PathingAvoidNode, 1024>() {}

PathingAvoidNodeArray::~PathingAvoidNodeArray() {}

PathingAvoidNode& PathingAvoidNodeArray::GetAt(uint32_t auiIndex) {
    return GetAtInternal(auiIndex);
}

float PathingAvoidNodeArray::ComputeCenter(const NiPoint3& arStartPoint, NiPoint3& arCenter) {
    float fRadiusSum = 0.0f;
    arCenter = NiPoint3::ZERO;
    float fCostSum = 0.0f;
    uint32_t uiCount = iSize;
    if (uiCount == 0) {
        return fRadiusSum;
    }
    for (uint32_t i = 0; i < uiCount; ++i) {
        PathingAvoidNode& rNode = GetAt(i);
        float fCost = rNode.fCost;
        NiPoint3 kPoint = rNode.Point1;
        if (rNode.eType == PathingAvoidNode::AVOID_NODE_CYLINDER) {
            NiPoint3 kAxis = rNode.Point2 - rNode.Point1;
            NiPoint3 kToStart = arStartPoint - rNode.Point1;
            float fDot = kToStart.Dot(kAxis);
            if (fDot < 0.0) {
                kPoint = rNode.Point1;
            }
            else if (fDot > kAxis.SqrLength()) {
                kPoint = rNode.Point2;
            }
            else {
                float fLength = kAxis.UnitizeGetLength();
                kPoint = rNode.Point1 + kAxis * fDot / fLength;
            }
        }
        arCenter += kPoint * fCost;
        fRadiusSum += rNode.fRadius;
        fCostSum += fCost;
    }
    arCenter /= fCostSum;
    return fRadiusSum;
}

float PathingAvoidNodeArray::GetCostForPoint(NiPoint3& arPoint) {
    uint32_t i;
    float fCostSum = 0.0f;
    uint32_t uiCount = iSize;
    for (i = 0; i < uiCount; ++i) {
        fCostSum = GetAt(i).GetCostForPoint(arPoint) + fCostSum;
    }
    return fCostSum;
}

PathingAvoidNode& PathingAvoidNodeArray::GetAtInternal(int32_t aiIndex) {
    return pBuffer[aiIndex];
}
