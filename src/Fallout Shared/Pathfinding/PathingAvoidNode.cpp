#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"

#include "BSCore/BSSimpleArray.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

// Leaves every field unset, as the game does.
PathingAvoidNode::PathingAvoidNode() {}

PathingAvoidNode::PathingAvoidNode(const NiPoint3& arPoint1, const NiPoint3& arPoint2, float afRadius, float afCost)
  : Point1(arPoint1), Point2(arPoint2), fRadius(afRadius), fCost(afCost), eType(AVOID_NODE_CYLINDER) {}

PathingAvoidNode::PathingAvoidNode(const NiPoint3& arPoint, float afRadius, float afCost)
  : Point1(arPoint), fRadius(afRadius), fCost(afCost), eType(AVOID_NODE_SPHERE) {}

PathingAvoidNode::PathingAvoidNode(const NiPoint3& arPoint, float afRadius, float afCost, AVOID_NODE_TYPE aeType)
  : Point1(arPoint), fRadius(afRadius), fCost(afCost), eType(aeType) {}

float PathingAvoidNode::GetCostForPoint(NiPoint3& arPoint) {
    NiPoint3 v8;
    NiPoint3 v9;
    float v10;
    float v11;
    NiPoint3 result;
    float v14 = fRadius * fRadius;
    float v13 = 3.4028235e+38f;
    PathingAvoidNode::AVOID_NODE_TYPE eType = this->eType;
    if (eType == PathingAvoidNode::AVOID_NODE_SPHERE) {
    LABEL_4:;
        result = Point1 - arPoint;
        v13 = result.SqrLength();
        goto LABEL_9;
    }
    if (eType != PathingAvoidNode::AVOID_NODE_CYLINDER) {
        if (eType != PathingAvoidNode::AVOID_NODE_SPHERE_ACTOR) {
            goto LABEL_9;
        }
        goto LABEL_4;
    }
    v8 = Point2 - Point1;
    v9 = arPoint - Point1;
    v10 = v8.SqrLength();
    v11 = v9.Dot(v8);
    if (!(v11 < 0.0) && !(v10 < static_cast<double>(v11))) {
        double v2 = v9.SqrLength();
        v13 = v2 - v11 * v11 / v10;
    }
    else {
        v13 = 3.4028235e+38f;
    }
LABEL_9:;
    if (!(v14 > static_cast<double>(v13))) {
        return 0.0f;
    }
    else {
        return (1.0 - v13 / v14) * fCost;
    }
}

void PathingAvoidNode::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint8_t cType = static_cast<uint8_t>(eType);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cType), 1, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fRadius), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fCost), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&Point1), 0xC, 0);
    if (eType == AVOID_NODE_CYLINDER) {
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&Point2), 0xC, 0);
    }
}

// Reads what SaveGame writes.
void PathingAvoidNode::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint8_t cType = 0;
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cType), 1);
    eType = static_cast<AVOID_NODE_TYPE>(cType);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fRadius), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fCost), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&Point1), 0xC);
    if (eType == AVOID_NODE_CYLINDER) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&Point2), 0xC);
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
    arCenter = NiPoint3_ZERO;
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
    PathingAvoidNode* v2;
    uint32_t i;
    float v7 = (float)0.0;
    uint32_t formFlags = iSize;
    for (i = 0; i < formFlags; ++i) {
        v2 = &GetAt(i);
        v7 = v2->GetCostForPoint(arPoint) + v7;
    }
    return v7;
}

PathingAvoidNode& PathingAvoidNodeArray::GetAtInternal(int32_t a1) {
    return pBuffer[a1];
}
