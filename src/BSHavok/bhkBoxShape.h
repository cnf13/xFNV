#pragma once

#include "Fallout Shared/GlobalFunc.h"
#include "BSHavok/bhkConvexShape.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A box shape. Only what the reconstructed code uses is declared.
class bhkBoxShape : public bhkConvexShape {
public:
	bhkBoxShape();
	bhkBoxShape(const hkVector4& arExt);
	bhkBoxShape(const NiPoint3& arExt);
	virtual ~bhkBoxShape();

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override { return 0x20; }
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;
	void BuildDisplayGeometry(NiNode* apNode, const NiColor& apColor) override;

	// The half extents arExt, in Havok units, as a new creation info handed to the virtual Init.
	void Init(const hkVector4& arExt);
	NiPoint3* GetHalfExtents(NiPoint3* apResult);
	const hkVector4& GethkHalfExtents();

	// The exe's factory: a new default-constructed box shape, or null when the allocation fails (cdecl, ret 0,
	// 0xC9FE30).
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	// The guard of the PDB's bhkBoxShape::BuildDisplayGeometry::dbgGeomNameSC, the debug geometry's name (the PDB's
	// `$S1`, bit 0).
	static inline uint32_t iCount;
};

// The creation info of a box shape: the half extents (a bhkVector4, so the struct sits on Havok's 16-byte alignment).
struct hkBoxShapeCinfo : public hkConvexShapeCinfo {
	hkBoxShapeCinfo();

	bhkVector4 m_halfExtents;
};

inline constexpr fnvd::Vtable bhkBoxShape_vftable{};

// The info is its own serialized form (bhkSerialized.h): no pointer member, the 32-bit layout on every width, so
// bhkSerializable::LoadCInfo's raw read of QCinfoSize() bytes over it is right on both.
static_assert(offsetof(hkBoxShapeCinfo, m_halfExtents) == 0x10 && sizeof(hkBoxShapeCinfo) == 0x20);
