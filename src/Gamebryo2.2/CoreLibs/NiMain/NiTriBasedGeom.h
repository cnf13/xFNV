#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"

class NiTriBasedGeomData;

// Geometry made of triangles (shapes and strips).
class NiTriBasedGeom : public NiGeometry {
public:
	NiTriBasedGeom();
	NiTriBasedGeom(NiTriBasedGeomData* pkModelData);
	~NiTriBasedGeom() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiTriBasedGeom* IsTriBasedGeom() override { return this; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void SetModelData(NiGeometryData* pkModelData) override;
	// The three corners of model-space triangle ausTriangle.
	virtual void GetModelTriangle(uint16_t ausTriangle, NiPoint3*& arpP0, NiPoint3*& arpP1, NiPoint3*& arpP2) = 0;

	uint16_t GetActiveTriangleCount() const;
	uint16_t GetTriCount();

	static NiRTTI ms_RTTI;
};
