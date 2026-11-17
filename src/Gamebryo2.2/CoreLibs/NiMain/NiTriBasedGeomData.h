#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

// Geometry data made of triangles: adds the triangle count.
class NiTriBasedGeomData : public NiGeometryData {
public:
	NiTriBasedGeomData();
	NiTriBasedGeomData(int32_t usVertices, NiPoint3* pkVertex, NiPoint3* pkNormal, NiColorA* pkColor,
		NiPoint2* pkTexture, int32_t usNumTextureSets, NiGeometryData::DataFlags aeDataFlags, int32_t usTriangles);
	~NiTriBasedGeomData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	virtual void SetActiveTriangleCount(uint16_t) {}
	virtual uint16_t GetActiveTriangleCount() const { return m_usTriangles; }
	virtual void GetTriangleIndices(uint16_t, uint16_t&, uint16_t&, uint16_t&) const {}
	virtual void GetStripData(uint16_t&, const uint16_t*&, const uint16_t*&, uint32_t&) const {}

	uint16_t GetTriCount();

	uint16_t m_usTriangles;
	uint8_t padding42[2];

	static NiRTTI ms_RTTI;
};

NiSmartPointer(NiTriBasedGeomData);

inline constexpr fnvd::Vtable NiTriBasedGeomData_vftable{};
