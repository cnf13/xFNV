#include "Fallout Shared/FaceGen/BSFaceGenMorphData.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenBaseMorphExtraData.h"
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/FaceGen/BSFaceGenMorph.h"
#include "Fallout Shared/FaceGen/BSFaceGenMorphDataHair.h"
#include "Fallout Shared/FaceGen/FffTriFileDataS.h"
#include "Fallout Shared/FaceGen/NiGeometryDataIteratorNiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "SDK/FaceGen/TriFileIO.h"
#include "SDK/MSVC/ehvec.h"

NiRTTI BSFaceGenBaseMorphExtraData::ms_RTTI("BSFaceGenBaseMorphExtraData", &NiExtraData::ms_RTTI);
NiRTTI BSFaceGenMorphData::ms_RTTI("BSFaceGenMorphData", nullptr);
NiRTTI BSFaceGenMorphDataHead::ms_RTTI("BSFaceGenMorphDataHead", &BSFaceGenMorphData::ms_RTTI);
NiRTTI BSFaceGenMorphDataHair::ms_RTTI("BSFaceGenMorphDataHair", &BSFaceGenMorphData::ms_RTTI);

// The registration takes the class as one token.
using FffTriDiffMorphVector = std::vector<FffTriDiffMorphS, std::allocator<FffTriDiffMorphS>>;

// The mesh's vertices copied out, apAdditionalVertices appended; no vertices when the mesh has no vertex data.
BSFaceGenBaseMorphExtraData::BSFaceGenBaseMorphExtraData(NiGeometry* apMesh, NiPoint3* apAdditionalVertices,
	uint32_t auiAdditionalVerticesCount)
	: NiExtraData(Tag()) {
	NiGeometryDataIteratorNiPoint3 kIterator;
	if (apMesh != nullptr && apMesh->GetModelData() != nullptr && apMesh->GetModelData()->LockPackedVertexData(true)) {
		apMesh->GetModelData()->GetVerticesIterator(kIterator);
	}
	if (kIterator.GetCurrentPosition() != nullptr) {
		iVertsModelCount = apMesh->GetVertexCount();
		iVertsCount = iVertsModelCount;
		if (apAdditionalVertices != nullptr && auiAdditionalVerticesCount != 0) {
			iVertsCount += auiAdditionalVerticesCount;
		}
		pVerts = static_cast<NiPoint3*>(ourMalloc(iVertsCount * sizeof(NiPoint3)));
		for (uint32_t i = 0; i < iVertsCount; ++i) {
			::new (static_cast<void*>(&pVerts[i])) NiPoint3;
		}
		for (uint32_t i = 0; i < iVertsModelCount; ++i) {
			NiPoint3 kPoint;
			kIterator.Get_(kPoint);
			pVerts[i] = kPoint;
			kIterator.Next();
		}
		if (apAdditionalVertices != nullptr && auiAdditionalVerticesCount != 0) {
			ourMemcpy(&pVerts[iVertsModelCount], apAdditionalVertices, auiAdditionalVerticesCount * sizeof(NiPoint3));
		}
		apMesh->GetModelData()->UnlockPackedVertexData();
	} else {
		pVerts = nullptr;
		iVertsModelCount = 0;
		iVertsCount = 0;
	}
}

BSFaceGenBaseMorphExtraData::~BSFaceGenBaseMorphExtraData() {
	if (pVerts) {
		ourFree(pVerts);
	}
}

const NiFixedString& BSFaceGenBaseMorphExtraData::Tag() {
	return *BSFaceGenBaseMorphExtraData::pTag;
}

// The exe's cleanup-only C++ EH frame is left out.
void BSFaceGenBaseMorphExtraData::InitSDM() {
	NiFixedString* FOD;
	NiFixedString* v1 = static_cast<NiFixedString*>(ourMalloc(sizeof(NiFixedString)));
	if (v1) {
		FOD = ::new (static_cast<void*>(v1)) NiFixedString("FOD");
	} else {
		FOD = 0;
	}
	BSFaceGenBaseMorphExtraData::pTag = FOD;
}

void BSFaceGenBaseMorphExtraData::KillSDM() {
	if (BSFaceGenBaseMorphExtraData::pTag) {
		delete BSFaceGenBaseMorphExtraData::pTag;
	}
	BSFaceGenBaseMorphExtraData::pTag = nullptr;
}

BSFaceGenMorphData::BSFaceGenMorphData() {}

BSFaceGenMorphData::~BSFaceGenMorphData() {}

BSFaceGenMorphData* BSFaceGenMorphData::DynamicCastMember(const NiRTTI* apRTTI) const {
	return IsKindOfMember(apRTTI) ? const_cast<BSFaceGenMorphData*>(this) : nullptr;
}

bool BSFaceGenMorphData::IsKindOfMember(const NiRTTI* apRTTI) const {
	for (const NiRTTI* pRTTI = GetRTTI(); pRTTI; pRTTI = pRTTI->GetBaseRTTI()) {
		if (pRTTI == apRTTI) {
			return true;
		}
	}
	return false;
}

BSFaceGenMorphData* BSFaceGenMorphData::DynamicCast(const NiRTTI* pkRTTI, const BSFaceGenMorphData* pkObject) {
	if (!pkObject) {
		return nullptr;
	}
	return pkObject->DynamicCastMember(pkRTTI);
}

void BSFaceGenMorphDataHead::ApplyMorph(BS_FACEGEN_MORPH_TYPE aeType, int32_t aiIndex, NiTriBasedGeom* apGeom,
	BSFaceGenBaseMorphExtraData* apBaseMorph, float afModifier) {
	BSFaceGenMorph* pMorph = 0;
	switch (aeType) {
	case BS_FG_MORPH_PHONEME: {
		pMorph = GetPhonemeMorph(static_cast<BS_FACEGEN_PHONEME>(aiIndex));
		break;
	}
	case BS_FG_MORPH_EXPRESSION: {
		pMorph = GetExpressionMorph(static_cast<BS_FACEGEN_EXPRESSION>(aiIndex));
		break;
	}
	case BS_FG_MORPH_MODIFIER: {
		pMorph = GetModifierMorph(static_cast<BS_FACEGEN_MODIFIER>(aiIndex));
		break;
	}
	case BS_FG_MORPH_CUSTOM: {
		pMorph = GetCustomMorph(static_cast<BS_FACEGEN_CUSTOM_MORPH>(aiIndex));
		break;
	}
	default: {
		break;
	}
	}
	if (pMorph) {
		pMorph->ApplyMorph(apGeom, apBaseMorph, afModifier);
	}
}

// The hair morph is the .tri file's first difference morph when it is labelled "HairMorph".
BSFaceGenMorphDataHair::BSFaceGenMorphDataHair(FffTriFileDataS* apFaceGenTri) : pHairMorph(nullptr) {
	if (!apFaceGenTri) {
		return;
	}
	const FffTriDiffMorphS& rMorph = apFaceGenTri->diffMorphList[0];
	if (BSstricmp("HairMorph", rMorph.label.c_str())) {
		return;
	}
	uint32_t uiVertCount = static_cast<uint32_t>(rMorph.vtxDelta.size());
	pHairMorph = new BSFaceGenMorphDifferential(uiVertCount);
	for (uint32_t i = 0; i < uiVertCount; ++i) {
		const FR2Vect3TC<float>& rDelta = rMorph.vtxDelta[i];
		pHairMorph->pVerts[i] = NiPoint3(rDelta.x1, rDelta.x2, rDelta.x3);
	}
}

BSFaceGenMorphDataHair::~BSFaceGenMorphDataHair() {
	if (pHairMorph) {
		delete pHairMorph;
	}
}

void BSFaceGenMorphDataHair::ApplyMorph(NiTriBasedGeom* apGeom, BSFaceGenBaseMorphExtraData* apExtraData,
	float afModifierValue) {
	if (pHairMorph) {
		if (apGeom) {
			if (!(afModifierValue < 0.0)) {
				pHairMorph->ApplyMorph(apGeom, apExtraData, afModifierValue);
			}
		}
	}
}
