#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

class NiCloningProcess;
class NiCullingProcess;
class NiRenderer;
class NiStream;

// a particle system that draws its particles as ribbon strips.
class BSStripParticleSystem : public NiParticleSystem {
public:
	BSStripParticleSystem();
	~BSStripParticleSystem() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void OnVisible(NiCullingProcess& kCuller) override;
	void RenderImmediate(NiRenderer* apkRenderer) override;

	// The class' stream loader entry.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};
