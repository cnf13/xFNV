#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"

// Fog state: enable bit and fog function in m_uFlags, depth and color.
class NiFogProperty : public NiProperty {
public:
	enum FogFunction : int32_t {
		FOG_Z_LINEAR = 0x0,
		FOG_RANGE_SQ = 0x1,
		FOG_VERTEX_ALPHA = 0x2,
	};

	enum Flags : uint16_t {
		FOG_MASK = 0x1,
		FOG_FUNC_MASK = 0x6,
		FOG_FUNC_POS = 0x1,
	};

	// The constructor sets no flags, depth 1.0 and a black colour.
	NiFogProperty();

	~NiFogProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	int32_t Type() const override { return FOG; }

	// Sets or clears FOG_MASK in m_uFlags.
	void SetFog(bool bFog);
	// Sets or clears the bits uMask of m_uFlags.
	void SetBit(bool bVal, uint16_t uMask);

	// "<prefix> = <function name>" in a fresh _NiMalloc block.
	static char* GetViewerString(const char* pcPrefix, FogFunction eFunc);
	// Builds the engine's default fog property into ms_spDefault.
	static void _SDMInit();
	// Releases ms_spDefault.
	static void _SDMShutdown();
	static NiObject* CreateObject();

	uint16_t m_uFlags;
	float m_fDepth;
	NiColor m_kColor;

	static NiRTTI ms_RTTI;
	static NiPointer<NiFogProperty> ms_spDefault;
};
