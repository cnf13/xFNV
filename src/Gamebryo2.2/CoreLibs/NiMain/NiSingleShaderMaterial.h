#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterial.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiShader;

// A material that renders with one named shader, cached after the first lookup.
class NiSingleShaderMaterial : public NiMaterial {
public:
	NiSingleShaderMaterial(const NiFixedString& kName, const char* pcName, bool bPreloadShader);
	~NiSingleShaderMaterial() override;

	const NiRTTI* GetRTTI() const override;
	bool IsShaderCurrent(NiShader*, const NiGeometry*, const NiSkinInstance*, const NiPropertyState*,
		const NiDynamicEffectState*, uint32_t) override;
	NiShader* GetCurrentShader(const NiGeometry*, const NiSkinInstance*, const NiPropertyState*,
		const NiDynamicEffectState*, uint32_t) override;
	void UnloadShaders() override;

	// The SDK's
	// NiSingleShaderMaterial::Create(const char* pcName, bool bPreloadShader): the name is read only.
	// The exe passes the caller's whole dword on; bool would be zero-extended.
	static NiSingleShaderMaterial* Create(const char* materialname, int32_t a2);

	NiFixedString m_kShaderName;
	NiPointer<NiShader> m_spCachedShader;
	bool m_bManualShaderManagement;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiSingleShaderMaterial_vftable{};
