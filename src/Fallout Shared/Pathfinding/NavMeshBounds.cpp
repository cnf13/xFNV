#include "Fallout Shared/Pathfinding/NavMeshBounds.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/NewOverloads.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/NavMeshMath.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <limits>

NavMeshBounds::NavMeshBounds()
  : BoundingBoxMin(
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max()
    ),
    BoundingBoxMax(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    ) {}

bool NavMeshBounds::IsPointInBounds(const NiPoint3& aPoint) {
    if (BoundingBoxMin.x <= static_cast<double>(aPoint.x) &&
        BoundingBoxMax.x >= static_cast<double>(aPoint.x) &&
        BoundingBoxMin.y <= static_cast<double>(aPoint.y) &&
        BoundingBoxMax.y >= static_cast<double>(aPoint.y) &&
        aPoint.z >= BoundingBoxMin.z - 180.0 &&
        aPoint.z <= BoundingBoxMax.z + 180.0) {
        if (Vertices.IsEmpty()) {
            return true;
        }
        float fMaxZdistAbove = BoundingBoxMax.z - BoundingBoxMin.z;
        for (uint32_t uiIndex = 0; uiIndex < Triangles.GetSize(); ++uiIndex) {
            float fZDiff;
            NiPoint3 aTriVert1Loc = *static_cast<NiPoint3*>(&Vertices.GetAt(Triangles.GetAt(uiIndex).Vertices[0]));
            NiPoint3 aTriVert2Loc = *static_cast<NiPoint3*>(&Vertices.GetAt(Triangles.GetAt(uiIndex).Vertices[1]));
            NiPoint3 aTriVert3Loc = *static_cast<NiPoint3*>(&Vertices.GetAt(Triangles.GetAt(uiIndex).Vertices[2]));
            if (NavMeshMath::IsPointInsideTriangle(
                    aTriVert1Loc,
                    aTriVert2Loc,
                    aTriVert3Loc,
                    aPoint,
                    fZDiff,
                    fMaxZdistAbove,
                    fMaxZdistAbove
                )) {
                return true;
            }
        }
    }
    return false;
}

NiPoint3 NavMeshBounds::DistanceFromPointToBounds(const NiPoint3& aPoint) {
    NiPoint3 kResult;
    if (Vertices.IsEmpty()) {
        if (!(BoundingBoxMin.x <= static_cast<double>(aPoint.x)) ||
            !(BoundingBoxMax.x >= static_cast<double>(aPoint.x)) ||
            !(BoundingBoxMin.y <= static_cast<double>(aPoint.y)) ||
            !(BoundingBoxMax.y >= static_cast<double>(aPoint.y)) ||
            !(BoundingBoxMin.z <= static_cast<double>(aPoint.z)) ||
            !(BoundingBoxMax.z >= static_cast<double>(aPoint.z))) {
            NiPoint3 retVal = NiPoint3::ZERO;
            if (BoundingBoxMax.x < static_cast<double>(aPoint.x)) {
                retVal.x = aPoint.x - BoundingBoxMax.x;
            }
            else if (BoundingBoxMin.x > static_cast<double>(aPoint.x)) {
                retVal.x = aPoint.x - BoundingBoxMin.x;
            }
            else {
                retVal.x = 0.0f;
            }
            if (BoundingBoxMax.y < static_cast<double>(aPoint.y)) {
                retVal.y = aPoint.y - BoundingBoxMax.y;
            }
            else if (BoundingBoxMin.y > static_cast<double>(aPoint.y)) {
                retVal.y = aPoint.y - BoundingBoxMin.y;
            }
            else {
                retVal.y = 0.0f;
            }
            if (BoundingBoxMax.z < static_cast<double>(aPoint.z)) {
                retVal.z = aPoint.z - BoundingBoxMax.z;
            }
            else if (BoundingBoxMin.z > static_cast<double>(aPoint.z)) {
                retVal.z = aPoint.z - BoundingBoxMin.z;
            }
            else {
                retVal.z = 0.0f;
            }
            kResult = retVal;
            return kResult;
        }

        kResult = NiPoint3::ZERO;
        return kResult;
    }

    NiPoint3 bestPoint;
    float fBestDistanceSqr = std::numeric_limits<float>::max();
    for (uint32_t i = 0; i < Triangles.iSize; ++i) {
        float fZDiff;
        // One array of the three corners: the edge loop indexes it.
        NiPoint3 aTriVerts[3];
        aTriVerts[0] = Vertices.GetAt(static_cast<int32_t>(Triangles.GetAt(static_cast<int32_t>(i)).Vertices[0]));
        aTriVerts[1] = Vertices.GetAt(static_cast<int32_t>(Triangles.GetAt(static_cast<int32_t>(i)).Vertices[1]));
        aTriVerts[2] = Vertices.GetAt(static_cast<int32_t>(Triangles.GetAt(static_cast<int32_t>(i)).Vertices[2]));
        float fMaxZDelta = BoundingBoxMax.z - BoundingBoxMin.z;
        if (NavMeshMath::IsPointInsideTriangle(
                aTriVerts[0],
                aTriVerts[1],
                aTriVerts[2],
                aPoint,
                fZDiff,
                fMaxZDelta,
                fMaxZDelta
            )) {
            ::new (&kResult) NiPoint3(0.0f, 0.0f, fZDiff);
            return kResult;
        }
        for (int32_t e = 0; e < 3; ++e) {
            int32_t f = e + 1;
            if (e == 2) {
                f = 0;
            }
            NiPoint3 pointOnEdge = NavMeshMath::ProjectPointOntoSegment(aPoint, aTriVerts[e], aTriVerts[f]);
            float fDistSqr = (aPoint - pointOnEdge).SqrLength();
            if (fBestDistanceSqr > static_cast<double>(fDistSqr)) {
                bestPoint = pointOnEdge;
                fBestDistanceSqr = fDistSqr;
            }
        }
    }
    ::new (&kResult) NiPoint3(bestPoint - aPoint);
    return kResult;
}

void NavMeshBounds::FillBoundsChunk(char* apChunkBuffer) {
    NiPoint3 kPoint = BoundingBoxMin;
    if (TESForm::QEndianSwapOnSave()) {
        BSEndian(kPoint.x, false);
        BSEndian(kPoint.y, false);
        BSEndian(kPoint.z, false);
    }
    ourMemcpy(apChunkBuffer, &kPoint, sizeof(NiPoint3));
    apChunkBuffer += sizeof(NiPoint3);
    kPoint = BoundingBoxMax;
    if (TESForm::QEndianSwapOnSave()) {
        BSEndian(kPoint.x, false);
        BSEndian(kPoint.y, false);
        BSEndian(kPoint.z, false);
    }
    ourMemcpy(apChunkBuffer, &kPoint, sizeof(NiPoint3));
    apChunkBuffer += sizeof(NiPoint3);
    uint16_t usCount = static_cast<uint16_t>(Vertices.GetSize());
    if (TESForm::QEndianSwapOnSave()) {
        BSEndian(usCount, false);
    }
    ourMemcpy(apChunkBuffer, &usCount, sizeof(uint16_t));
    apChunkBuffer += sizeof(uint16_t);
    usCount = static_cast<uint16_t>(Triangles.GetSize());
    if (TESForm::QEndianSwapOnSave()) {
        BSEndian(usCount, false);
    }
    ourMemcpy(apChunkBuffer, &usCount, sizeof(uint16_t));
    apChunkBuffer += sizeof(uint16_t);
    if (Vertices.IsEmpty()) {
        return;
    }
    uint32_t uiCount = Vertices.GetSize();
    uint32_t uiBytes = uiCount * sizeof(NiPoint3);
    NiPoint3* pPoints = new NiPoint3[uiCount];
    ourMemcpy(pPoints, &Vertices.GetAt(0), uiBytes);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            BSEndian(pPoints[i].x, false);
            BSEndian(pPoints[i].y, false);
            BSEndian(pPoints[i].z, false);
        }
    }
    ourMemcpy(apChunkBuffer, pPoints, uiBytes);
    apChunkBuffer += uiBytes;
    delete[] pPoints;
    uiCount = Triangles.GetSize();
    uiBytes = uiCount * sizeof(NavMeshBoundsTriangle);
    NavMeshBoundsTriangle* pTriangles = static_cast<NavMeshBoundsTriangle*>(ourMalloc(
        uiCount > SIZE_MAX / sizeof(NavMeshBoundsTriangle) ? SIZE_MAX : sizeof(NavMeshBoundsTriangle) * uiCount
    ));
    ourMemcpy(pTriangles, &Triangles.GetAt(0), uiBytes);
    if (TESForm::QEndianSwapOnSave()) {
        for (uint32_t i = 0; i < uiCount; ++i) {
            for (uint16_t& usVertex : pTriangles[i].Vertices) {
                BSEndian(usVertex, false);
            }
        }
    }
    ourMemcpy(apChunkBuffer, pTriangles, uiBytes);
    apChunkBuffer += uiBytes;
    ourFree(pTriangles);
}

void NavMeshBounds::CopyTo(NavMeshBounds* apDestBounds) {
    apDestBounds->BoundingBoxMin = BoundingBoxMin;
    apDestBounds->BoundingBoxMax = BoundingBoxMax;
    Vertices.CopyTo(&apDestBounds->Vertices);
    Triangles.CopyTo(&apDestBounds->Triangles);
}

int32_t NavMeshBounds::GetBoundsChunkSize() {
    uint32_t uiSize = 12 * Vertices.BSSimpleArray<NiPoint3>::GetSize() + 28;
    return uiSize + 6 * Triangles.BSSimpleArray<NavMeshBounds::NavMeshBoundsTriangle>::GetSize();
}

// The chunk is the two corners, the two counts (uint16_t), then the points and the triangles, read in place: it is
// treated as untyped bytes, so each value is read through a cast of the buffer.
void NavMeshBounds::SetFromBoundsChunk(char* apChunkBuffer, TESFile* apFile, int32_t aiFileVersionNumber) {
    Clear();
    BoundingBoxMin = *reinterpret_cast<NiPoint3*>(apChunkBuffer);
    if (apFile->QEndian()) {
        BSEndian<float>(BoundingBoxMin.x, false);
        BSEndian<float>(BoundingBoxMin.y, false);
        BSEndian<float>(BoundingBoxMin.z, false);
    }
    apChunkBuffer += sizeof(NiPoint3);
    BoundingBoxMax = *reinterpret_cast<NiPoint3*>(apChunkBuffer);
    if (apFile->QEndian()) {
        BSEndian<float>(BoundingBoxMax.x, false);
        BSEndian<float>(BoundingBoxMax.y, false);
        BSEndian<float>(BoundingBoxMax.z, false);
    }
    apChunkBuffer += sizeof(NiPoint3);
    uint16_t sizeOfVertexArray = *reinterpret_cast<uint16_t*>(apChunkBuffer);
    if (apFile->QEndian()) {
        BSEndian<uint16_t>(sizeOfVertexArray, false);
    }
    Vertices.SetSize(sizeOfVertexArray, true);
    apChunkBuffer += sizeof(uint16_t);
    uint16_t sizeOfTriangleArray = *reinterpret_cast<uint16_t*>(apChunkBuffer);
    if (apFile->QEndian()) {
        BSEndian<uint16_t>(sizeOfTriangleArray, false);
    }
    Triangles.SetSize(sizeOfTriangleArray, true);
    apChunkBuffer += sizeof(uint16_t);
    if (!Vertices.IsEmpty()) {
        int32_t iVertexBytes = sizeof(NiPoint3) * Vertices.iSize;
        BSmemcpy(&Vertices.GetAt(0), iVertexBytes, apChunkBuffer, iVertexBytes);
        if (apFile->QEndian()) {
            for (uint32_t i = 0; i < sizeOfVertexArray; ++i) {
                BSEndian<float>(Vertices.GetAt(static_cast<int32_t>(i)).x, false);
                BSEndian<float>(Vertices.GetAt(static_cast<int32_t>(i)).y, false);
                BSEndian<float>(Vertices.GetAt(static_cast<int32_t>(i)).z, false);
            }
        }
        char* pTriangleChunk = &apChunkBuffer[sizeof(NiPoint3) * Vertices.iSize];
        int32_t iTriangleBytes = sizeof(NavMeshBounds::NavMeshBoundsTriangle) * Triangles.iSize;
        BSmemcpy(&Triangles.GetAt(0), iTriangleBytes, pTriangleChunk, iTriangleBytes);
        if (apFile->QEndian()) {
            for (uint32_t i = 0; i < sizeOfTriangleArray; ++i) {
                BSEndian<uint16_t>(Triangles.GetAt(static_cast<int32_t>(i)).Vertices[0], false);
                BSEndian<uint16_t>(Triangles.GetAt(static_cast<int32_t>(i)).Vertices[1], false);
                BSEndian<uint16_t>(Triangles.GetAt(static_cast<int32_t>(i)).Vertices[2], false);
            }
        }
    }
}

void NavMeshBounds::Free() {}

void NavMeshBounds::Clear() {
    BoundingBoxMin = NiPoint3(
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max()
    );
    BoundingBoxMax = NiPoint3(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    );
    Vertices.Clear(true);
    Triangles.Clear(true);
}

void NavMeshBounds::operator delete(void* apMemory) {
    ourFree(apMemory);
}
