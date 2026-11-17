#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Reconstruction.hpp"

class NiPoint2;
class NiTriShapeData;

// A triangle-list mesh.
class NiTriShape : public NiTriBasedGeom {
public:
	NiTriShape();
	NiTriShape(int32_t usVertices, NiPoint3* pkVertex, NiPoint3* pkNormal, NiColorA* pkColor, NiPoint2* pkTexture,
		int32_t usNumTextureSets, NiGeometryData::DataFlags eNBTMethod, int32_t usTriangles, uint16_t* pusTriList);
	NiTriShape(NiTriShapeData* apModelData);

	~NiTriShape() override;

	const NiRTTI* GetRTTI() const override;
	NiTriShape* IsTriShape() override { return this; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void RenderImmediate(NiRenderer* apkRenderer) override;
	void OnlyRenderImmediate(NiRenderer* apkRenderer) override;
	void GetModelTriangle(uint16_t usTriangle, NiPoint3*& pkP0, NiPoint3*& pkP1, NiPoint3*& pkP2) override;

	void CopyMembers(NiTriShape* pkDest, NiCloningProcess& kCloning);

	// Inferred type: a1: NiTriShapeData* from 0x68bddd (result of BSTempEffectSimpleDecal::CreateTriData)
	static NiTriShape* Create(NiTriShapeData* a1);
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};

NiSmartPointer(NiTriShape);

inline constexpr fnvd::Vtable NiTriShape_vftable{};
