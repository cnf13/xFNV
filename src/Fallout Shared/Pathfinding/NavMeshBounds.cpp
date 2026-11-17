#include "Fallout Shared/Pathfinding/NavMeshBounds.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/NavMeshMath.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/ehvec.h"

NavMeshBounds::NavMeshBounds()
	: BoundingBoxMin(-3.4028235e+38f, -3.4028235e+38f, -3.4028235e+38f),
	  BoundingBoxMax(3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f) {}

bool NavMeshBounds::IsPointInBounds(const NiPoint3& aPoint) {
	if (BoundingBoxMin.x <= static_cast<double>(aPoint.x) &&
		(BoundingBoxMax.x >= static_cast<double>(aPoint.x) &&
			(BoundingBoxMin.y <= static_cast<double>(aPoint.y) &&
				(BoundingBoxMax.y >= static_cast<double>(aPoint.y) &&
					(aPoint.z >= BoundingBoxMin.z - 180.0 && aPoint.z <= BoundingBoxMax.z + 180.0))))) {
		if (Vertices.IsEmpty()) {
			return 1;
		}
		float afMaxZdistAbove = BoundingBoxMax.z - BoundingBoxMin.z;
		for (uint32_t auiIndex = 0;; ++auiIndex) {
			float afZDiff;
			uint32_t FormFlags = Triangles.GetSize();
			if (auiIndex >= FormFlags) {
				break;
			}
			NavMeshBounds::NavMeshBoundsTriangle* v3 = &Triangles.GetAt(auiIndex);
			NiPoint3 aTriVert1Loc = *static_cast<NiPoint3*>(&Vertices.GetAt(v3->Vertices[0]));
			NavMeshBounds::NavMeshBoundsTriangle* v4 = &Triangles.GetAt(auiIndex);
			NiPoint3 aTriVert2Loc = *static_cast<NiPoint3*>(&Vertices.GetAt(v4->Vertices[1]));
			NavMeshBounds::NavMeshBoundsTriangle* v5 = &Triangles.GetAt(auiIndex);
			NiPoint3 aTriVert3Loc = *static_cast<NiPoint3*>(&Vertices.GetAt(v5->Vertices[2]));
			if (NavMeshMath::IsPointInsideTriangle(aTriVert1Loc, aTriVert2Loc, aTriVert3Loc, aPoint, afZDiff,
					afMaxZdistAbove, afMaxZdistAbove)) {
				return 1;
			}
		}
	}
	return 0;
}

NiPoint3 NavMeshBounds::DistanceFromPointToBounds(const NiPoint3& aPoint) {
	NiPoint3 result;
	// One array of the three corners: the edge loop indexes it.
	NiPoint3 aTriVerts[3];
	if (Vertices.IsEmpty()) {
		if (!(BoundingBoxMin.x <= static_cast<double>(aPoint.x)) ||
			(!(BoundingBoxMax.x >= static_cast<double>(aPoint.x)) ||
				(!(BoundingBoxMin.y <= static_cast<double>(aPoint.y)) ||
					(!(BoundingBoxMax.y >= static_cast<double>(aPoint.y)) ||
						(!(BoundingBoxMin.z <= static_cast<double>(aPoint.z)) ||
							!(BoundingBoxMax.z >= static_cast<double>(aPoint.z))))))) {
			NiPoint3 retVal = NiPoint3_ZERO;
			if (!(BoundingBoxMax.x < static_cast<double>(aPoint.x))) {
				if (!(BoundingBoxMin.x > static_cast<double>(aPoint.x))) {
					retVal.x = 0.0f;
				} else {
					retVal.x = aPoint.x - BoundingBoxMin.x;
				}
			} else {
				retVal.x = aPoint.x - BoundingBoxMax.x;
			}
			if (!(BoundingBoxMax.y < static_cast<double>(aPoint.y))) {
				if (!(BoundingBoxMin.y > static_cast<double>(aPoint.y))) {
					retVal.y = 0.0f;
				} else {
					retVal.y = aPoint.y - BoundingBoxMin.y;
				}
			} else {
				retVal.y = aPoint.y - BoundingBoxMax.y;
			}
			if (!(BoundingBoxMax.z < static_cast<double>(aPoint.z))) {
				if (!(BoundingBoxMin.z > static_cast<double>(aPoint.z))) {
					retVal.z = 0.0f;
				} else {
					retVal.z = aPoint.z - BoundingBoxMin.z;
				}
			} else {
				retVal.z = aPoint.z - BoundingBoxMax.z;
			}
			result = retVal;
			return result;
		} else {
			result = NiPoint3_ZERO;
			return result;
		}
	} else {
		NiPoint3 bestPoint;
		float fbestDistanceSqr = 3.4028235e+38f;
		for (uint32_t i = 0;; ++i) {
			float zdiff;
			uint32_t FormFlags = Triangles.iSize;
			if (i >= FormFlags) {
				break;
			}
			rt::eh_vector_constructor_iterator(aTriVerts, sizeof(NiPoint3), 3, &emptyPointConstructor);
			aTriVerts[0] =
				Vertices.GetAt(static_cast<int32_t>((&Triangles.GetAt(static_cast<int32_t>(i)))->Vertices[0]));
			aTriVerts[1] =
				Vertices.GetAt(static_cast<int32_t>((&Triangles.GetAt(static_cast<int32_t>(i)))->Vertices[1]));
			aTriVerts[2] =
				Vertices.GetAt(static_cast<int32_t>((&Triangles.GetAt(static_cast<int32_t>(i)))->Vertices[2]));
			float maxzdelta = BoundingBoxMax.z - BoundingBoxMin.z;
			if (NavMeshMath::IsPointInsideTriangle(aTriVerts[0], aTriVerts[1], aTriVerts[2], aPoint, zdiff, maxzdelta,
					maxzdelta)) {
				::new (&result) NiPoint3(0.0f, 0.0f, zdiff);
				return result;
			}
			for (int32_t e = 0; e < 3; ++e) {
				int32_t f = e + 1;
				if (e == 2) {
					f = 0;
				}
				NiPoint3 pointOnEdge = NavMeshMath::ProjectPointOntoSegment(aPoint, aTriVerts[e], aTriVerts[f]);
				float fdistSqr = (aPoint - pointOnEdge).SqrLength();
				if (fbestDistanceSqr > static_cast<double>(fdistSqr)) {
					bestPoint = pointOnEdge;
					fbestDistanceSqr = fdistSqr;
				}
			}
		}
		::new (&result) NiPoint3(bestPoint - aPoint);
		return result;
	}
}

// The arrays are new[]'d through ourMalloc: the size is -1 when the count times the item size overflows, and only
// the points (which have a constructor) go through the constructor iterator and a null check.
void NavMeshBounds::FillBoundsChunk(char* apChunkBuffer) {
	NiPoint3 kPoint = BoundingBoxMin;
	if (TESForm::QEndianSwapOnSave()) {
		ENDIAN(kPoint.x, false);
		ENDIAN(kPoint.y, false);
		ENDIAN(kPoint.z, false);
	}
	ourMemcpy(apChunkBuffer, &kPoint, sizeof(NiPoint3));
	apChunkBuffer += sizeof(NiPoint3);
	kPoint = BoundingBoxMax;
	if (TESForm::QEndianSwapOnSave()) {
		ENDIAN(kPoint.x, false);
		ENDIAN(kPoint.y, false);
		ENDIAN(kPoint.z, false);
	}
	ourMemcpy(apChunkBuffer, &kPoint, sizeof(NiPoint3));
	apChunkBuffer += sizeof(NiPoint3);
	uint16_t usCount = static_cast<uint16_t>(Vertices.GetSize());
	if (TESForm::QEndianSwapOnSave()) {
		ENDIAN(usCount, false);
	}
	ourMemcpy(apChunkBuffer, &usCount, sizeof(uint16_t));
	apChunkBuffer += sizeof(uint16_t);
	usCount = static_cast<uint16_t>(Triangles.GetSize());
	if (TESForm::QEndianSwapOnSave()) {
		ENDIAN(usCount, false);
	}
	ourMemcpy(apChunkBuffer, &usCount, sizeof(uint16_t));
	apChunkBuffer += sizeof(uint16_t);
	if (Vertices.IsEmpty()) {
		return;
	}
	uint32_t uiCount = Vertices.GetSize();
	uint32_t uiBytes = uiCount * sizeof(NiPoint3);
	void* pMemory = ourMalloc(uiCount > SIZE_MAX / sizeof(NiPoint3) ? SIZE_MAX : sizeof(NiPoint3) * uiCount);
	NiPoint3* pPoints = nullptr;
	if (pMemory) {
		rt::eh_vector_constructor_iterator(pMemory, sizeof(NiPoint3), uiCount, &emptyPointConstructor);
		pPoints = static_cast<NiPoint3*>(pMemory);
	}
	ourMemcpy(pPoints, &Vertices.GetAt(0), uiBytes);
	if (TESForm::QEndianSwapOnSave()) {
		for (uint32_t i = 0; i < uiCount; ++i) {
			ENDIAN(pPoints[i].x, false);
			ENDIAN(pPoints[i].y, false);
			ENDIAN(pPoints[i].z, false);
		}
	}
	ourMemcpy(apChunkBuffer, pPoints, uiBytes);
	apChunkBuffer += uiBytes;
	ourFree(pPoints);
	uiCount = Triangles.GetSize();
	uiBytes = uiCount * sizeof(NavMeshBoundsTriangle);
	NavMeshBoundsTriangle* pTriangles =
		static_cast<NavMeshBoundsTriangle*>(ourMalloc(uiCount > SIZE_MAX / sizeof(NavMeshBoundsTriangle)
				? SIZE_MAX
				: sizeof(NavMeshBoundsTriangle) * uiCount));
	ourMemcpy(pTriangles, &Triangles.GetAt(0), uiBytes);
	if (TESForm::QEndianSwapOnSave()) {
		for (uint32_t i = 0; i < uiCount; ++i) {
			for (int32_t j = 0; j < 3; ++j) {
				ENDIAN(pTriangles[i].Vertices[j], false);
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
	uint32_t v3 = (12 * (&Vertices)->BSSimpleArray<NiPoint3>::GetSize()) + 28;
	return v3 + (6 * (&Triangles)->BSSimpleArray<NavMeshBounds::NavMeshBoundsTriangle>::GetSize());
}

// The chunk is the two corners, the two counts (uint16_t), then the points and the triangles, read in place: it is
// treated as untyped bytes, so each value is read through a cast of the buffer.
void NavMeshBounds::SetFromBoundsChunk(char* apChunkBuffer, TESFile* apFile, int32_t aiFileVersionNumber) {
	Clear();
	BoundingBoxMin = *reinterpret_cast<NiPoint3*>(apChunkBuffer);
	if (apFile->QEndian()) {
		ENDIAN<float>(BoundingBoxMin.x, 0);
		ENDIAN<float>(BoundingBoxMin.y, 0);
		ENDIAN<float>(BoundingBoxMin.z, 0);
	}
	apChunkBuffer += sizeof(NiPoint3);
	BoundingBoxMax = *reinterpret_cast<NiPoint3*>(apChunkBuffer);
	if (apFile->QEndian()) {
		ENDIAN<float>(BoundingBoxMax.x, 0);
		ENDIAN<float>(BoundingBoxMax.y, 0);
		ENDIAN<float>(BoundingBoxMax.z, 0);
	}
	apChunkBuffer += sizeof(NiPoint3);
	uint16_t sizeOfVertexArray = *reinterpret_cast<uint16_t*>(apChunkBuffer);
	if (apFile->QEndian()) {
		ENDIAN<uint16_t>(sizeOfVertexArray, 0);
	}
	Vertices.SetSize(sizeOfVertexArray, 1);
	apChunkBuffer += sizeof(uint16_t);
	uint16_t sizeOfTriangleArray = *reinterpret_cast<uint16_t*>(apChunkBuffer);
	if (apFile->QEndian()) {
		ENDIAN<uint16_t>(sizeOfTriangleArray, 0);
	}
	Triangles.SetSize(sizeOfTriangleArray, 1);
	apChunkBuffer += sizeof(uint16_t);
	if (!Vertices.IsEmpty()) {
		int32_t v8 = sizeof(NiPoint3) * Vertices.iSize;
		uint32_t FormFlags = Vertices.iSize;
		BSmemcpy(&Vertices.GetAt(0), sizeof(NiPoint3) * FormFlags, apChunkBuffer, v8);
		if (apFile->QEndian()) {
			for (uint32_t i = 0; i < sizeOfVertexArray; ++i) {
				ENDIAN<float>((&Vertices.GetAt(static_cast<int32_t>(i)))->x, 0);
				ENDIAN<float>((&Vertices.GetAt(static_cast<int32_t>(i)))->y, 0);
				ENDIAN<float>((&Vertices.GetAt(static_cast<int32_t>(i)))->z, 0);
			}
		}
		char* apDataStreamd = &apChunkBuffer[sizeof(NiPoint3) * Vertices.iSize];
		int32_t v9 = sizeof(NavMeshBounds::NavMeshBoundsTriangle) * Triangles.iSize;
		int32_t v7 = sizeof(NavMeshBounds::NavMeshBoundsTriangle) * Triangles.iSize;
		NavMeshBounds::NavMeshBoundsTriangle* v5 = &Triangles.GetAt(0);
		BSmemcpy(v5, v7, apDataStreamd, v9);
		if (apFile->QEndian()) {
			for (uint32_t i_2 = 0; i_2 < sizeOfTriangleArray; ++i_2) {
				ENDIAN<uint16_t>((&Triangles.GetAt(static_cast<int32_t>(i_2)))->Vertices[0], 0);
				ENDIAN<uint16_t>((&Triangles.GetAt(static_cast<int32_t>(i_2)))->Vertices[1], 0);
				ENDIAN<uint16_t>((&Triangles.GetAt(static_cast<int32_t>(i_2)))->Vertices[2], 0);
			}
		}
	}
}

void NavMeshBounds::Free() {}

void NavMeshBounds::Clear() {
	NiPoint3 v2;
	NiPoint3 dst;
	BoundingBoxMin = *::new (&dst) NiPoint3(-3.4028235e+38f, -3.4028235e+38f, -3.4028235e+38f);
	BoundingBoxMax = *::new (&v2) NiPoint3(3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f);
	Vertices.Clear(1);
	Triangles.Clear(1);
}

void NavMeshBounds::operator delete(void* apMemory) {
	ourFree(apMemory);
}
