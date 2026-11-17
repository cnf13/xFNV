#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"

class NiDX9Renderer;

// Instanced grass geometry: one model drawn iInstanceCount times. Created at runtime, never stored in a NIF.
class TallGrassTriShape : public NiTriShape {
public:
	TallGrassTriShape(NiTriShapeData* usVertices);
	~TallGrassTriShape() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	// copies the local bound to the world bound, draws only when there are instances
	virtual void RenderImmediate(NiDX9Renderer* apRenderer);
	virtual void OnlyRenderImmediate(NiDX9Renderer* apRenderer);
	virtual void SetLocalBound(NiBound apBound);
	virtual void SetInstanceCount(uint16_t aiCount);
	virtual NiBound* GetWorldBound();
	using NiTriShape::OnlyRenderImmediate;
	using NiTriShape::RenderImmediate;

protected:
	void UpdateWorldBound() override;

public:
	uint16_t iInstanceCount;
	NiBound pLocalBound;

	static NiRTTI ms_RTTI;
};
