#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

class NiGeometry;

// The "FOD" extra data of a FaceGen mesh: a copy of the mesh's base vertices (plus bonus vertices after them) that the
// morphs are applied to. The PDB's source module is BSFaceGenMorphData.cpp; its Xbox-only spMorphModelData is not
// present in the PC type.
class BSFaceGenBaseMorphExtraData : public NiExtraData {
public:
	// The exe's constructor (thiscall, ret 0xC): the mesh's vertices copied out, apAdditionalVertices appended.
	BSFaceGenBaseMorphExtraData(NiGeometry* apMesh, NiPoint3* apAdditionalVertices,
		uint32_t auiAdditionalVerticesCount);
	~BSFaceGenBaseMorphExtraData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }

	// pVerts; the exe's shared one-field getter 0x84E3A0 (thiscall, ret 0).
	NiPoint3* GetVertices() const;
	// iVertsModelCount when abMeshOnly, else iVertsCount.
	uint32_t GetVertexCount(bool abMeshOnly) const;

	// The tag the extra data is named by.
	static const NiFixedString& Tag();
	// IDA name InitTag; the PDB's InitSDM. Creates the tag "FOD" (cdecl, 0x662760).
	static void InitSDM();
	static void KillSDM();

	// The PDB's pVerts; IDA names it pBaseVerts.
	NiPoint3* pVerts;
	uint32_t iVertsCount;
	uint32_t iVertsModelCount;

	static inline NiFixedString* pTag;
	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSFaceGenBaseMorphExtraData_vftable{};
