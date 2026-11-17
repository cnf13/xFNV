#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiCloningProcess;
class NiMatrix3;

// Moves along a path: interpolates a position from an NiPosData path and the percentage travelled from an NiFloatData,
// and can bank and follow the path's direction.
class NiPathInterpolator : public NiKeyBasedInterpolator {
public:
	// The direction the path banks into.
	enum BankDir : int32_t {
		NEGATIVE = -1,
		POSITIVE = 1,
	};

	// The key channels.
	enum Channel {
		PATH,
		PERCENTAGE,
	};

	// Bits of m_uFlags.
	enum {
		CVDATA_NEEDS_UPDATE_MASK = 0x1,
		CURVE_TYPE_OPEN_MASK = 0x2,
		ALLOW_FLIP_MASK = 0x4,
		BANK_MASK = 0x8,
		CONST_VELOCITY_MASK = 0x10,
		FOLLOW_MASK = 0x20,
		FLIP_MASK = 0x40,
	};

	NiPathInterpolator(NiPosData* pkPathData, NiFloatData* pkPctData);
	~NiPathInterpolator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	// Binds the data, refreshes the constant-velocity length and caches the path's largest curvature.
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) override;
	bool IsTransformValueSupported() const override { return true; }
	void GetActiveTimeRange(float& fBeginKeyTime, float& fEndKeyTime) const override;
	void GuaranteeTimeRange(float fStartTime, float fEndTime) override;
	NiInterpolator* GetSequenceInterpolator(float fStartTime, float fEndTime) override;
	uint16_t GetKeyChannelCount() const override { return 2; }
	uint32_t GetKeyCount(uint16_t usChannel) const override;
	NiAnimationKey::KeyType GetKeyType(uint16_t usChannel) const override;
	NiAnimationKey::KeyContent GetKeyContent(uint16_t usChannel) const override;
	NiAnimationKey* GetKeyArray(uint16_t usChannel) const override;
	uint8_t GetKeyStride(uint16_t usChannel) const override;
	bool GetChannelPosed(uint16_t) const override { return false; }
	using NiInterpolator::Update;

	// The path keys, their count, type and key size; null and zeros without path data.
	NiPosKey* GetPathKeys(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const;
	// Length of the whole path; computes the partial lengths when the cached value is stale.
	float GetTotalPathLength();
	// Whether the path's followers turn to face along it.
	bool GetFollow();

protected:
	// Position on the path between keys aiI1 and aiI2 at afNormTime.
	NiPoint3 GenPathInterp(int32_t iI1, int32_t iI2, float fNormTime);
	// Orientation along the path between keys auiI1 and auiI2 (follow, bank and flip applied).
	void CalcRefFrame(uint32_t ui1, uint32_t ui2, float afNormTime, NiMatrix3& kRefFrame);
	// The path keys bracketing afPercent of the path, and the normalised time between them.
	void GetLoc(float fTime, uint32_t& uiI1, uint32_t& uiI2, float& fNormTime);
	// The path's speed between keys aiI0 and aiI1 at afTime (thiscall, ret 0xC, ST0 float, 0xA3E5F0).
	float PathSpeed(int32_t iI0, int32_t iI1, float fTime) const;
	// The path's length between keys aiI0 and aiI1 up to afTime: 5-point Gauss-Legendre quadrature of PathSpeed.
	float PathLength(int32_t iI0, int32_t iI1, float fTime) const;
	// The path keys bracketing the point afPercent of the way along the path's length, and the normalised time
	// between them (Newton iteration on PathLength).
	void GetPercentLoc(float fPercent, uint32_t& uiI1, uint32_t& uiI2, float& fNormTime);

public:
	void SetConstantVelocityData();
	// Inferred type: apDest: NiPathInterpolator* from 0xa3f547 (result of NiPathInterpolator::Construct), its members
	// used at 0xa3f309
	void CopyMembers(NiPathInterpolator* apDest, NiCloningProcess* arCloning);

	static NiObject* CreateObject();

	uint16_t m_uFlags;
	uint32_t m_uiLastPathIdx;
	uint32_t m_uiLastPctIdx;
	NiPointer<NiPosData> m_spPathData;
	NiPointer<NiFloatData> m_spPctData;
	float* m_pfPartialLength;
	float m_fTotalLength;
	float m_fMaxBankAngle;
	float m_fSmoothing;
	int16_t m_sFollowAxis;
	float m_fMaxCurvature;
	BankDir m_eBankDir;
	NiQuaternion m_kRotation;
	NiPoint3 m_kPosition;
	// Written by Update, which compares against NiInterpolator::m_fLastTime instead (never written here).
	float m_fLastTime;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiPathInterpolator_vftable{};
