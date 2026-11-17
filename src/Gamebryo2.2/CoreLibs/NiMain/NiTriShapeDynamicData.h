#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"

// NiTriShapeData whose vertex and triangle counts in use can be lower than the allocated counts.
class NiTriShapeDynamicData : public NiTriShapeData {
public:
	NiTriShapeDynamicData();
	NiTriShapeDynamicData(uint16_t usVertices, NiPoint3* pkVertex, NiPoint3* pkNormal, NiColorA* pkColor,
		NiPoint2* pkTexture, int32_t usNumTextureSets, NiGeometryData::DataFlags eNBTMethod, uint16_t usTriangles,
		uint16_t* pusTriList, int32_t usActiveVertices, int32_t usActiveTriangles);
	~NiTriShapeDynamicData() override;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void SetActiveVertexCount(uint16_t usActiveVertices) override;
	uint16_t GetActiveVertexCount() const override;
	void SetActiveTriangleCount(uint16_t usActiveTriangles) override;
	uint16_t GetActiveTriangleCount() const override;
	void GetStripData(uint16_t& usStrips, const uint16_t*& pusStripLengths, const uint16_t*& pusTriList,
		uint32_t& uiStripLengthSum) const override;

	static NiObject* CreateObject();

	uint16_t m_usActiveVertices;
	uint16_t m_usActiveTriangles;

	static NiRTTI ms_RTTI;
};
