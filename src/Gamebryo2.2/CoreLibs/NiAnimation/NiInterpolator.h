#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Reconstruction.hpp"

class NiAVObjectPalette;
class NiBoolInterpolator;
class NiObjectNET;
class NiQuatTransform;

// Produces an animated value of one type for a time; the base supports no value type.
class NiInterpolator : public NiObject {
public:
	NiInterpolator();
	~NiInterpolator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	// the overloads sit in the vtable in reverse declaration order: the NiQuatTransform one is slot 35
	virtual bool Update(float, NiObjectNET*, bool&) { return false; }
	virtual bool Update(float, NiObjectNET*, float&) { return false; }
	virtual bool Update(float, NiObjectNET*, NiQuaternion&) { return false; }
	virtual bool Update(float, NiObjectNET*, NiPoint3&) { return false; }
	virtual bool Update(float, NiObjectNET*, NiColorA&) { return false; }
	virtual bool Update(float, NiObjectNET*, NiQuatTransform&) { return false; }
	virtual bool IsBoolValueSupported() const { return false; }
	virtual bool IsFloatValueSupported() const { return false; }
	virtual bool IsQuaternionValueSupported() const { return false; }
	virtual bool IsPoint3ValueSupported() const { return false; }
	virtual bool IsColorAValueSupported() const { return false; }
	virtual bool IsTransformValueSupported() const { return false; }
	virtual void Collapse() {}
	virtual void GetActiveTimeRange(float& fBeginKeyTime, float& fEndKeyTime) const;
	virtual void GuaranteeTimeRange(float, float) {}
	virtual NiInterpolator* GetSequenceInterpolator(float fStartTime, float fEndTime);
	virtual bool ResolveDependencies(NiAVObjectPalette*) { return true; }
	virtual bool SetUpDependencies() { return true; }
	virtual bool AlwaysUpdate() const { return false; }
	virtual NiBoolInterpolator* IsNiBoolInterpolator() { return nullptr; }

	// NiObject's part (thiscall, ret 8; the exe's jmp at 0xAA1000).
	void CopyMembers(NiInterpolator* pkDest, NiCloningProcess& kCloning);

	float m_fLastTime;

	static NiRTTI ms_RTTI;
	// The value of an interpolator with nothing to sample: 2 for a bool, -FLT_MAX for a float and every component
	// of a point or color.
	// Gamebryo's invalid values are -NI_INFINITY, i.e. -FLT_MAX, in every component.
	static inline uint8_t INVALID_BOOL = 2;
	static float INVALID_FLOAT;
	static inline NiQuaternion INVALID_QUATERNION = {-3.40282347e+38f, -3.40282347e+38f, -3.40282347e+38f,
		-3.40282347e+38f};
	static NiPoint3 INVALID_POINT3;
	static NiColorA INVALID_COLORA;
};
inline constexpr fnvd::Vtable NiInterpolator_vftable{};
