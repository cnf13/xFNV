#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Reconstruction.hpp"

class NiGeometryBufferData;

// Screen rectangles copied from one texture.
class NiScreenTexture : public NiObject {
public:
	// One rectangle: its pixel position and size on screen, its texel origin and its color.
	struct ScreenRect {
		int16_t m_sPixTop;
		int16_t m_sPixLeft;
		uint16_t m_usPixWidth;
		uint16_t m_usPixHeight;
		uint16_t m_usTexTop;
		uint16_t m_usTexLeft;
		NiColorA m_kColor;
	};

	NiScreenTexture();
	~NiScreenTexture() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	// NiObject's members, then the rectangles and the texturing property.
	void CopyMembers(NiScreenTexture* pkDest, NiCloningProcess& kCloning);

	static NiObject* CreateObject();

	NiTPrimitiveSet<ScreenRect> m_kScreenRects;
	NiPointer<NiTexturingProperty> m_spTexProp;
	uint16_t m_usDirtyFlags;
	NiGeometryBufferData* m_pkBuffData;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiScreenTexture_vftable{};
