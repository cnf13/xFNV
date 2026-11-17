#include "Fallout Shared/FaceGen/BSFaceGenMorph.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenBaseMorphExtraData.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/FaceGen/NiGeometryDataIteratorNiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "SDK/MSVC/ehvec.h"

BSFaceGenMorph::BSFaceGenMorph() {}

BSFaceGenMorph::~BSFaceGenMorph() {}

void BSFaceGenMorph::operator delete(void* apMemory) {
	ourFree(apMemory);
}

// The vertex differences are ourMalloc'd and left uninitialised, as the game does (NiPoint3's empty constructor).
BSFaceGenMorphDifferential::BSFaceGenMorphDifferential(uint32_t aiVertsCount)
	: pVerts(nullptr), iVertsCount(aiVertsCount) {
	if (iVertsCount) {
		pVerts = static_cast<NiPoint3*>(ourMalloc(sizeof(NiPoint3) * iVertsCount));
	}
}

BSFaceGenMorphDifferential::~BSFaceGenMorphDifferential() {
	if (pVerts) {
		ourFree(pVerts);
	}
}

// Moves the mesh vertices by their stored differences, scaled by afScale, up to BSMIN's count of them. afScale is
// tested against 0.0 and 1.0 as doubles, so a NaN passes both.
void BSFaceGenMorphDifferential::ApplyMorph(NiTriBasedGeom* apGeom, BSFaceGenBaseMorphExtraData* apExtraData,
	float afModifierValue) const {
	if (!apGeom) {
		return;
	}
	if (afModifierValue <= 0.0) {
		return;
	}
	if (afModifierValue > 1.0) {
		return;
	}
	NiGeometryData* pModelData = apGeom->GetModelData();
	NiGeometryDataIteratorNiPoint3 kIterator;
	FNVD_RECONSTRUCT(NiTConvertingStrideIterator<NiPoint3>)(&kIterator);
	pModelData->GetVerticesIterator(kIterator);
	NiPoint3 kVertex;
	kVertex.~NiPoint3();
	::new (&kVertex) NiPoint3;
	uint32_t uiVertices = BSMIN(pModelData->GetVertexCount(), iVertsCount);
	for (uint32_t i = 0; i < uiVertices; ++i) {
		kIterator.Get_(kVertex);
		kVertex += pVerts[i] * afModifierValue;
		kIterator.SetData(kVertex);
		kIterator.Next();
	}
}

BSFaceGenMorphStatistical::BSFaceGenMorphStatistical(uint32_t aiVertsCount, uint32_t aiFirstIndex)
	: pWhichMeshVerts(nullptr), iVertsCount(aiVertsCount), iFirstIndex(static_cast<int32_t>(aiFirstIndex)) {
	if (iVertsCount) {
		pWhichMeshVerts = static_cast<uint32_t*>(ourMalloc(sizeof(uint32_t) * iVertsCount));
	}
}

BSFaceGenMorphStatistical::~BSFaceGenMorphStatistical() {
	if (pWhichMeshVerts) {
		ourFree(pWhichMeshVerts);
	}
}

// Moves each listed mesh vertex by the difference of two of the extra data's base vertices, scaled by afModifierValue.
// afModifierValue is tested against 0.0 and 1.0 as doubles, so a NaN passes both.
void BSFaceGenMorphStatistical::ApplyMorph(NiTriBasedGeom* apGeom, BSFaceGenBaseMorphExtraData* apExtraData,
	float afModifierValue) const {
	if (!apGeom) {
		return;
	}
	if (afModifierValue <= 0.0) {
		return;
	}
	if (afModifierValue > 1.0) {
		return;
	}
	// The base vertex array is read twice: for the null test and for the moves.
	if (!apExtraData->GetVertices()) {
		return;
	}
	NiPoint3* pBaseVerts = apExtraData->GetVertices();
	NiGeometryData* pModelData = apGeom->GetModelData();
	NiGeometryDataIteratorNiPoint3 kIterator;
	FNVD_RECONSTRUCT(NiTConvertingStrideIterator<NiPoint3>)(&kIterator);
	pModelData->GetVerticesIterator(kIterator);
	NiPoint3 kVertex;
	kVertex.~NiPoint3();
	::new (&kVertex) NiPoint3;
	for (uint32_t i = 0; i < iVertsCount; ++i) {
		int32_t iIndex = pWhichMeshVerts[i];
		kIterator.Get(iIndex, kVertex);
		kVertex += (pBaseVerts[iFirstIndex + i] - pBaseVerts[iIndex]) * afModifierValue;
		kIterator.Set(iIndex, kVertex);
	}
}
