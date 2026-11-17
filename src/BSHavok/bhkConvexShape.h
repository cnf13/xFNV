#pragma once

#include "BSHavok/bhkSphereRepShape.h"
#include "Reconstruction.hpp"

struct hkConvexShapeCinfo;

// A convex shape. Its members are not declared yet.
class bhkConvexShape : public bhkSphereRepShape {
public:
	// The base constructor.
	bhkConvexShape();

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	~bhkConvexShape() override;

	// Copies the wrapped convex shape's data into arInfo (thiscall, ret 4, 0xC9FB70). Not an
	// override of bhkSerializable's virtual CopyCinfo: the exe's vtables keep the base's empty body in that slot
	// (0x4534F0), so this is the separate typed overload the derived classes call qualified.
	void CopyCinfo(hkConvexShapeCinfo& arInfo);

	static NiRTTI ms_RTTI;
	// Number of live instances; the constructors count them.
	static inline uint32_t iCount;
};

// The creation info of a convex shape: the radius of its rounded surface (the default is the float at 0x10C72BC).
// The PDB defines it in TESObjectREFR.cpp.
struct hkConvexShapeCinfo : public hkSphereRepShapeCinfo {
	// The serialized form (bhkSerialized.h): the 32-bit layout, 8 bytes.
	struct Serialized : hkShapeCinfo::Serialized {
		float m_radius;
	};

	// The default constructor.
	hkConvexShapeCinfo();

	// Fills this part from its serialized form.
	void ReadSerialized(const Serialized& arForm) {
		hkShapeCinfo::ReadSerialized(arForm);
		m_radius = arForm.m_radius;
	}

	float m_radius;
};

inline float bhkConvexShape_fHk2BSScaleSC = 6.9991255f;
inline constexpr fnvd::Vtable bhkConvexShape_vftable{};

static_assert(sizeof(hkConvexShapeCinfo::Serialized) == 0x8);
