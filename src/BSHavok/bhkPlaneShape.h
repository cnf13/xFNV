#pragma once

#include "BSHavok/bhkHeightFieldShape.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Reconstruction.hpp"

#include <stdint.h>

struct hkPlaneShapeCinfo;

// The game wrapper of a plane collision shape. Its members are not declared yet.
class bhkPlaneShape : public bhkHeightFieldShape {
public:
	bhkPlaneShape();
	~bhkPlaneShape() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;

protected:
	// bhkShape's CopyCinfo, then the wrapped hkpPlaneShape's plane (+0x10), half extents
	// (+0x30) and centre (+0x20) into arInfo (thiscall, ret 4, 0xCCB1B0); CreateCinfo 0xCCB730 calls it. Not an
	// override: like bhkShape's, it hides the base's CopyCinfo, whose vtable slot keeps bhkSerializable's.
	void CopyCinfo(hkPlaneShapeCinfo& arInfo);

public:
	// The exe's factory: a new default-constructed plane shape, or null when the allocation fails (cdecl, ret 0,
	// 0xCCB200).
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	// Live wrappers of this class, counted by the inlined constructors.
	static inline uint32_t iCount;
};

// The creation info of a height field shape: the PDB has it with hkShapeCinfo as its base and no members of its own
// (its PDB source is BSHavok\bhkPlaneShape.cpp, like the plane shape's).
struct hkHeightFieldShapeCinfo : public hkShapeCinfo {};

// The creation info of a plane shape (IDA spells it hkPlaneShapeCInfo): the plane and the bounding box's half
// extents and centre. The members are the PDB's bhkVector4 (CopyCinfo 0xCCB1B0 fills them with bhkVector4::operator=
// 0x4B4DB0); being 16-byte aligned, they sit at the PDB's 0x10, 0x20 and 0x30 past the 4-byte cinfo base.
struct hkPlaneShapeCinfo : public hkHeightFieldShapeCinfo {
	bhkVector4 m_plane;
	bhkVector4 m_aabbHalfExtents;
	bhkVector4 m_aabbCenter;
};

inline constexpr fnvd::Vtable bhkPlaneShape_vftable{};
