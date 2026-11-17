#pragma once

#include "BSHavok/bhkBvTreeShape.h"
#include "Reconstruction.hpp"

class hkTriSampledHeightFieldBvTreeShapeCinfo;

// A bounding volume tree over a sampled height field of triangles (the landscape's collision shape). The Havok
// shape holds the data; IDA has no fields for it.
class bhkTriSampledHeightFieldBvTreeShape : public bhkBvTreeShape {
public:
	bhkTriSampledHeightFieldBvTreeShape();
	bhkTriSampledHeightFieldBvTreeShape(const hkTriSampledHeightFieldBvTreeShapeCinfo& arInfo);
	~bhkTriSampledHeightFieldBvTreeShape() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;

	// Copies the Havok shape's data into arInfo (thiscall, ret 4, 0xCB0030). IDA name `CB0030`; the PDB's
	// CopyCinfo(arInfo) of this class, matched by its file and shape.
	void CopyCinfo(hkTriSampledHeightFieldBvTreeShapeCinfo& arInfo);

	static NiRTTI ms_RTTI;
	static inline uint32_t iCount;
};
inline constexpr fnvd::Vtable bhkTriSampledHeightFieldBvTreeShape_vftable{};
