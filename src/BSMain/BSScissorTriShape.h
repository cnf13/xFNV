#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Reconstruction.hpp"

// A triangle list shape drawn through a scissor rectangle.
class BSScissorTriShape : public NiTriShape {
public:
	BSScissorTriShape();
	// The shape over new shape data built from the given arrays, clipped to the rectangle (aiLeft, aiTop, aiRight,
	// aiBottom).
	BSScissorTriShape(uint16_t usVertices, NiPoint3* pkVertex, NiPoint3* pkNormal, NiColorA* pkColor,
		NiPoint2* pkTexture, uint16_t usNumTextureSets, NiGeometryData::DataFlags eNBTMethod, uint16_t usTriangles,
		uint16_t* pusTriList, int32_t aiLeft, int32_t aiTop, int32_t aiRight, int32_t aiBottom);
	BSScissorTriShape(int32_t a1, int32_t a2, int32_t a3, int32_t a4);
	~BSScissorTriShape() override;

	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void RenderImmediate(NiRenderer* pkRenderer) override;

	void SetScissorRect(int32_t aiLeft, int32_t aiTop, int32_t aiRight, int32_t aiBottom);
	// Copies the scissor rectangle into pkDest, then the base's members.
	void CopyMembers(BSScissorTriShape* pDest, NiCloningProcess& arCloneData);

	// The scissor rectangle the shape is drawn through.
	RECT kRect;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSScissorTriShape_vftable{};
