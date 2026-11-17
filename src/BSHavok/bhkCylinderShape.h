#pragma once

#include "BSHavok/bhkConvexShape.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Reconstruction.hpp"

#include <stdint.h>

struct hkCylinderShapeCinfo;

// The wrapper of an hkpCylinderShape. Only its RTTI and the virtuals it overrides are modelled.
class bhkCylinderShape : public bhkConvexShape {
public:
	// The default constructor, run in place on apMemory.
	bhkCylinderShape();
	~bhkCylinderShape() override;

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
	void BuildDisplayGeometry(NiNode* apNode, const NiColor& arDefColor) override;

	// Creates the hkpCylinderShape from arTop to arBottom with radius afRad and sets it as
	// this wrapper's object; the virtual Init calls it.
	void Init(const hkVector4& arTop, const hkVector4& arBottom, float afRad);
	// bhkConvexShape::CopyCinfo, then the two end points and the cylinder radius into arInfo
	// (thiscall, ret 4, 0xCCD1D0; CreateCinfo, BuildDisplayGeometry and GetViewerStrings call it). Protected in the
	// PDB.
	void CopyCinfo(hkCylinderShapeCinfo& arInfo);

	// The stream's object factory: a new, unbuilt wrapper.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	static inline int32_t iCount;
};

// The creation info of a cylinder shape: its two end points and the cylinder's radius after the convex radius. The
// PDB defines it in bhkCylinderShape.cpp (members m_vertexA, m_vertexB, fCylinderRadius). The vertices are the PDB's
// bhkVector4: CopyCinfo (0xCCD1D0) fills them with bhkVector4::operator= (0x4B4DB0). bhkCylinderShape::CreateCinfo
// (0xCCD520) allocates 0x50 bytes, aligns the block to 16 and inlines the constructor (0x40 bytes: the vertices are
// 16-aligned at 0x10 and 0x20, the radius at 0x30).
struct hkCylinderShapeCinfo : public hkConvexShapeCinfo {
	// Multiplies the two vertices (all four words each) and fCylinderRadius by afScale;
	// bhkCylinderShape::CopyMembers calls it on the clone's cinfo when the cloning
	// scale is not 1.
	void Scale(float afScale);

	bhkVector4 m_vertexA;
	bhkVector4 m_vertexB;
	float fCylinderRadius;
};

inline constexpr fnvd::Vtable bhkCylinderShape_vftable{};
