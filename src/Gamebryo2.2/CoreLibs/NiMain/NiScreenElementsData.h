#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"

// The geometry data of NiScreenElements: polygons, each a run of vertices and triangles in the shared arrays.
class NiScreenElementsData : public NiTriShapeData {
public:
	struct Polygon {
		uint16_t m_usNumVertices;
		uint16_t m_usVOffset;
		uint16_t m_usNumTriangles;
		uint16_t m_usIOffset;
	};

	NiScreenElementsData();
	NiScreenElementsData(bool bWantNormals, bool bWantColors, uint16_t usNumTextureSets, int32_t iMaxPQuantity,
		int32_t iPGrowBy, int32_t iMaxVQuantity, int32_t iVGrowBy, int32_t iMaxTQuantity, int32_t iTGrowBy);
	~NiScreenElementsData() override;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

private:
	void CalculateNormals() override {}

public:
	// Appends a polygon; returns its index.
	int32_t Insert(uint16_t usNumVertices, uint16_t usNumTriangles, const uint16_t* ausTriList);
	bool SetRectangle(int32_t iPolygon, float fLeft, float fTop, float fWidth, float fHeight);
	bool SetTextures(int32_t iPolygon, uint16_t usSet, float fLeft, float fTop, float fRight, float fBottom);
	void UpdateBound();

	static NiObject* CreateObject();

	Polygon* m_akPolygon;
	uint16_t* m_ausPIndexer;
	uint16_t m_usMaxPQuantity;
	uint16_t m_usPGrowBy;
	uint16_t m_usPQuantity;
	uint16_t m_usMaxVQuantity;
	uint16_t m_usVGrowBy;
	uint16_t m_usMaxIQuantity;
	uint16_t m_usIGrowBy;
	bool m_bBoundNeedsUpdate;

	static NiRTTI ms_RTTI;
};
