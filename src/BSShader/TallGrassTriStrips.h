#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Reconstruction.hpp"

class NiDX9Renderer;

// Instanced grass geometry: one model drawn iInstanceCount times. Created at runtime, never stored in a NIF.
class TallGrassTriStrips : public NiTriStrips {
public:
	TallGrassTriStrips(NiTriStripsData* usVertices);
	~TallGrassTriStrips() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	// copies the local bound to the world bound, draws only when there are instances
	virtual void RenderImmediate(NiDX9Renderer* apRenderer);
	virtual void OnlyRenderImmediate(NiDX9Renderer* apRenderer);
	virtual void SetLocalBound(NiBound apBound);
	virtual void SetInstanceCount(uint16_t aiCount);
	// folded with BSResizableTriShape::GetWorldBound at 0xA86D30
	virtual NiBound* GetWorldBound();
	using NiTriStrips::OnlyRenderImmediate;
	using NiTriStrips::RenderImmediate;

protected:
	// folded with BSResizableTriShape::UpdateWorldBound at 0xC1A4A0
	void UpdateWorldBound() override;

public:
	uint16_t iInstanceCount;
	NiBound pLocalBound;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable TallGrassTriStrips_vftable{};
