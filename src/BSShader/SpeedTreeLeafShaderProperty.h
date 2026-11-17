#pragma once

#include "BSShader/SpeedTreeShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The leaf data a leaf property shares with its tree model (IDA's SpeedTreeLeafShaderProperty::LeafData). The exe
// allocates 0x20 bytes for it (0x6689A8) and its constructor (0xBB3950) zeroes the floats at 0xC to 0x14; the leaf
// shader reads the floats at 0xC to 0x1C (0xBB28EE to 0xBB2968).
class STLSPData : public NiRefObject {
public:
	STLSPData();
	~STLSPData() override;

	// Copies up to 0xC0 leaf-map floats and zeroes the rest of the 0xC0.
	void SetLeafMaps(const float* apLeafMaps, uint32_t aiCount);

	void SetCurveScalar(float afCurveScalar);
	void SetRock(float afRock);
	void SetRustle(float afRustle);
	void SetRockSpeed(float afRockSpeed);
	void SetRustleSpeed(float afRustleSpeed);

	float* pLeafMaps;
	float fCurveScalar;
	float fRock;
	float fRustle;
	float fRockSpeed;
	float fRustleSpeed;
};

// The property that selects the leaf shader for a SpeedTree model: the leaf maps and the tree's level of detail.
class SpeedTreeLeafShaderProperty : public SpeedTreeShaderLightingProperty {
public:
	SpeedTreeLeafShaderProperty(uint16_t asLevel, STSPData* apData, STLSPData* apLeafData);
	~SpeedTreeLeafShaderProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	bool SetupGeometry(NiGeometry*) override { return true; }
	BSShaderProperty::RenderPassArray* GetRenderPasses(NiGeometry* apGeom, int32_t aiEnabledPasses, uint16_t& asCount,
		uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList) override;
	SpeedTreeLeafShaderProperty* CreateCopy() override;
	virtual NiAdditionalGeometryData* CreateRendererSpecificProperty(NiGeometry* apGeometry);
	virtual uint16_t GetLevel() const { return sLevel; }
	virtual STLSPData* GetLeafData() { return spLeafData; }
	virtual float* GetLeafMaps();
	virtual void SetLeafMaps(const float* apLeafMaps, uint32_t aiCount);

	// Copies this property's members into pDest while cloning (thiscall, ret 8, 0xBB3D80;
	// IDA: CopyMambers).
	void CopyMembers(SpeedTreeLeafShaderProperty* apDest, NiCloningProcess& arCloneData);

	NiPointer<STLSPData> spLeafData;
	uint16_t sLevel;
	uint8_t padding8E[2];

	static NiRTTI ms_RTTI;
};

inline constexpr fnvd::Vtable STLSPData_vftable{};
inline constexpr fnvd::Vtable SpeedTreeLeafShaderProperty_vftable{};
