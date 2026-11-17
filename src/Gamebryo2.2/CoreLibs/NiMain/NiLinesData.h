#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"

// Geometry data of a line list: one connect flag per vertex (whether a line runs to the next vertex).
class NiLinesData : public NiGeometryData {
public:
	NiLinesData();
	NiLinesData(int32_t usVertices, NiPoint3* pkVertex, NiColorA* pkColor, NiPoint2* pkTexture,
		int32_t usNumTextureSets, NiGeometryData::DataFlags eNBTMethod, uint8_t* pkFlags);
	~NiLinesData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void CalculateNormals() override {}

	static NiObject* CreateObject();

	// The flags are streamed bytes, which IsEqual compares byte by byte.
	uint8_t* m_pkFlags;

	static NiRTTI ms_RTTI;
};
