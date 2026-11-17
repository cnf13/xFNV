#pragma once

#include "BSHavok/bhkSphereRepShape.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Reconstruction.hpp"

struct hkMultiSphereShapeCinfo;

// hkArray<hkVector4>'s destructor (thiscall): the object comes first, then the unused edx.
void DestroyVector4Array(void* apThis, [[maybe_unused]] void* apEdx);
// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx. Returns
// nothing: eax is what _clearAndDeallocate (0x66DAE0) left.
void clearAndDeallocate(void* apThis, [[maybe_unused]] void* apEdx);

// A shape of several spheres (a rigid body's sphere representation). The Havok shape holds the spheres; IDA has no
// fields for it.
class bhkMultiSphereShape : public bhkSphereRepShape {
public:
	bhkMultiSphereShape();
	bhkMultiSphereShape(const hkMultiSphereShapeCinfo& arInfo);
	~bhkMultiSphereShape() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;

	// Copies the spheres of the wrapped Havok shape into arCinfo.
	void GetCinfo(hkMultiSphereShapeCinfo& arCinfo);

	// A bare shape with no creation info yet.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	static inline uint32_t iCount;
};

// The creation info of a multi-sphere shape: the sphere-rep shape's, then the spheres. The PDB defines it in
// BSTreeNode.cpp.
struct hkMultiSphereShapeCinfo : public hkSphereRepShapeCinfo {
	hkMultiSphereShapeCinfo();
	~hkMultiSphereShapeCinfo();

	hkArray<hkVector4> m_spheres;
};

inline float bhkMultiSphereShape_fHk2BSScaleSC = 6.9991255f;
inline constexpr fnvd::Vtable bhkMultiSphereShape_vftable{};
