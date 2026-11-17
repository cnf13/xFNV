#pragma once

#include "Fallout Shared/FaceGen/BSFaceGenModel.h"

class BSFaceGenBaseMorphExtraData;
class BSFaceGenMorphDifferential;

// The morph data of a FaceGen hair: one morph (the "HairMorph" record of the .tri file) as per-vertex differences.
class BSFaceGenMorphDataHair : public BSFaceGenMorphData {
public:
	// The constructor from the .tri file's data apFaceGenTri;
	// returns apMemory.
	BSFaceGenMorphDataHair(FffTriFileDataS* apFaceGenTri);
	~BSFaceGenMorphDataHair() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }

	// Applies pHairMorph's per-vertex differences to apGeom, scaled by afModifierValue.
	void ApplyMorph(NiTriBasedGeom* apGeom, BSFaceGenBaseMorphExtraData* apExtraData, float afModifierValue);

	BSFaceGenMorphDifferential* pHairMorph;

	static NiRTTI ms_RTTI;
};
