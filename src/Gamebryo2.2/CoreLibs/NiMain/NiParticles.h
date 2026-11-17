#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"

class NiCloningProcess;
class NiCullingProcess;
class NiRenderer;
class NiStream;

// a point-sprite particle primitive; its model data is a NiParticlesData.
class NiParticles : public NiGeometry {
public:
	NiParticles();
	~NiParticles() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiParticles* IsParticlesGeom() override { return this; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& stream) override;
	void LinkObject(NiStream& stream) override;
	bool RegisterStreamables(NiStream& stream) override;
	void SaveBinary(NiStream& stream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;
	void OnVisible(NiCullingProcess& kCuller) override;
	void RenderImmediate(NiRenderer* apkRenderer) override;
	void CalculateConsistency(bool bTool) override;

	void CopyMembers(NiParticles* pDest, NiCloningProcess& kCloning);

	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};
