#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPersistentSrcTextureRendererData.h"
#include "Reconstruction.hpp"

NiObject* NiDX9PersistentSrcTextureRendererData_CreateObject();

// The DX9 renderer's subclass of NiPersistentSrcTextureRendererData (streamable pre-converted texture pixels).
class NiDX9PersistentSrcTextureRendererData : public NiPersistentSrcTextureRendererData {
public:
	NiDX9PersistentSrcTextureRendererData();
	~NiDX9PersistentSrcTextureRendererData() override;

	const NiRTTI* GetRTTI() const override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	const NiRTTI* GetStreamableRTTI() const override;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiDX9PersistentSrcTextureRendererData_vftable{};
