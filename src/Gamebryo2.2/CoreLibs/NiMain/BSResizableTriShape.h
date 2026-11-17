#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"

// A triangle-list mesh whose drawn triangle count and bound the game changes at runtime.
class BSResizableTriShape : public NiTriShape {
public:
	BSResizableTriShape();
	BSResizableTriShape(NiTriShapeData* usVertices);
	~BSResizableTriShape() override;

	const NiRTTI* GetRTTI() const override;
	BSResizableTriShape* IsResizableTriShape() override { return this; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void UpdateWorldBound() override;
	void RenderImmediate(NiRenderer* apRenderer) override;
	void OnlyRenderImmediate(NiRenderer* apRenderer) override;
	// sets the local bound and the world bound
	virtual void SetLocalBound(NiBound aBound);
	virtual const NiBound& GetWorldBound();

	void CopyMembers(BSResizableTriShape* pkDest, NiCloningProcess& kCloning);

	uint32_t uiNumPrimitives;
	NiBound LocalBound;

	static NiRTTI ms_RTTI;
};
