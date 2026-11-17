#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// a light that shines along one world direction
class NiDirectionalLight : public NiLight {
public:
	NiDirectionalLight();
	~NiDirectionalLight() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& stream) override;
	void LinkObject(NiStream& stream) override;
	bool RegisterStreamables(NiStream& stream) override;
	void SaveBinary(NiStream& stream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;
	void UpdateWorldData(NiUpdateData& UpdateData) override;

	static NiObject* CreateObject();

	NiPoint3 m_kWorldDir;

	static NiRTTI ms_RTTI;
};
