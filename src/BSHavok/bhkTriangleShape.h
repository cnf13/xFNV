#pragma once

#include "BSHavok/bhkConvexShape.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Reconstruction.hpp"

#include <stdint.h>

struct hkTriangleShapeCinfo;

// The game wrapper of a single-triangle convex shape. Its members are not declared yet.
class bhkTriangleShape : public bhkConvexShape {
public:
	// The PDB's bhkTriangleShape::bhkTriangleShape default constructor (thiscall, ret 0,
	// 0xCCA950).
	bhkTriangleShape();
	~bhkTriangleShape() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	uint32_t QCinfoSize() const override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;

	// bhkConvexShape::CopyCinfo, then the three corners into arInfo (thiscall, ret 4, 0xCCAA30; CreateCinfo's
	// callee). Protected in the PDB.
	void CopyCinfo(hkTriangleShapeCinfo& arInfo);

	// The class loader's factory: a new shape built by the constructor.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	static inline int32_t iCount;
};

// The creation info of a triangle shape: its three corners (bhkVector4, so the struct keeps Havok's 16-byte
// alignment). The PDB's members; CreateCinfo (0xCCADB0) allocates 0x40 bytes and QCinfoSize (0xCCB140) returns 0x40.
struct hkTriangleShapeCinfo : public hkConvexShapeCinfo {
	// Scales the three corners, all four components of each, by afScale;
	// CopyMembers runs it on the clone's creation info with the cloning process's scale.
	void Scale(float afScale);

	bhkVector4 m_vertexA;
	bhkVector4 m_vertexB;
	bhkVector4 m_vertexC;
};

inline constexpr fnvd::Vtable bhkTriangleShape_vftable{};
