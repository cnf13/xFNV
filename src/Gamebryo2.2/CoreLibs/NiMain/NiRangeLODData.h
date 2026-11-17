#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiLODData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// LOD by camera distance from a center: one near/far range per level.
class NiRangeLODData : public NiLODData {
public:
	struct Range : public NiMemObject {
		float m_fNear;
		float m_fFar;
		float m_fWorldNear;
		float m_fWorldFar;
	};

	NiRangeLODData();
	~NiRangeLODData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	int32_t GetLODLevel(const NiCamera* pkCamera, NiLODNode* pkLOD) override;
	void UpdateWorldData(NiLODNode* pkLOD) override;
	NiLODData* Duplicate() override;
	int32_t GetLODIndex(int32_t iLODLevel) override;

	// Resizes the range array, keeping the leading ranges.
	void SetNumRanges(uint32_t uiNumRanges);
	void CopyMembers(NiRangeLODData* pkDest, NiCloningProcess& kCloning);

	// Builds the object with its constructor, which waits for its base chain in src.
	static NiObject* CreateObject();

	NiPoint3 m_kCenter;
	NiPoint3 m_kWorldCenter;
	uint32_t m_uiNumRanges;
	Range* m_pkRanges;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiRangeLODData_vftable{};
