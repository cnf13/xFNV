#pragma once

#include "BSHavok/bhkShape.h"

// A shape that can be approximated by spheres. Its members are not declared yet.
class bhkSphereRepShape : public bhkShape {
public:
	bhkSphereRepShape();

	const NiRTTI* GetRTTI() const override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	~bhkSphereRepShape() override;

	static NiRTTI ms_RTTI;
	static inline uint32_t iCount;
};

// The creation info of a shape that can be approximated by spheres; it adds no fields to hkShapeCinfo.
struct hkSphereRepShapeCinfo : public hkShapeCinfo {
	hkSphereRepShapeCinfo();
};
