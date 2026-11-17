#pragma once

#include "Reconstruction.hpp"

class BSFaceGenBaseMorphExtraData;
class NiPoint3;
class NiTriBasedGeom;

// Base of the face morphs: ApplyMorph moves a geometry's vertices by the morph, scaled by a factor.
class BSFaceGenMorph {
public:
	BSFaceGenMorph();
	virtual ~BSFaceGenMorph();

	virtual void ApplyMorph(NiTriBasedGeom*, BSFaceGenBaseMorphExtraData*, float) const = 0;

	static void operator delete(void* apMemory);
};

// A morph stored as per-vertex differences (pVerts, iVertsCount).
class BSFaceGenMorphDifferential : public BSFaceGenMorph {
public:
	BSFaceGenMorphDifferential(uint32_t aiVertsCount);
	~BSFaceGenMorphDifferential() override;

	void ApplyMorph(NiTriBasedGeom* apGeom, BSFaceGenBaseMorphExtraData* apExtraData,
		float afModifierValue) const override;

	NiPoint3* pVerts;
	uint32_t iVertsCount;
};

// A morph stored as a sparse set of mesh-vertex moves: mesh vertex pWhichMeshVerts[i] moves by the difference of the
// base vertices iFirstIndex + i and pWhichMeshVerts[i], scaled by the modifier value.
class BSFaceGenMorphStatistical : public BSFaceGenMorph {
public:
	BSFaceGenMorphStatistical(uint32_t aiVertsCount, uint32_t aiFirstIndex);
	~BSFaceGenMorphStatistical() override;

	void ApplyMorph(NiTriBasedGeom* apGeom, BSFaceGenBaseMorphExtraData* apExtraData,
		float afModifierValue) const override;

	// The mesh vertices the morph moves, iVertsCount of them.
	uint32_t* pWhichMeshVerts;
	uint32_t iVertsCount;
	// The first of the two base vertices each move reads.
	int32_t iFirstIndex;
};

inline constexpr fnvd::Vtable BSFaceGenMorph_vftable{};
inline constexpr fnvd::Vtable BSFaceGenMorphDifferential_vftable{};
inline constexpr fnvd::Vtable BSFaceGenMorphStatistical_vftable{};
