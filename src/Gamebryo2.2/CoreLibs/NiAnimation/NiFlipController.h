#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

class NiTexture;

// Flips its target texturing map through a list of textures, picking one by the animated float.
class NiFlipController : public NiFloatInterpController {
public:
	NiFlipController();
	~NiFlipController() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void Update(NiUpdateData& UpdateData) override;
	const char* GetCtlrID() override;
	NiBlendInterpolator* CreateBlendInterpolator(uint16_t usIndex, int32_t bManagerControlled,
		bool bAccumulateAnimations, float fWeightThreshold, int32_t ucArraySize) const override;

protected:
	bool InterpTargetIsCorrectType(NiObjectNET* pkTarget) const override;
	void GetTargetFloatValue(float& fValue) override;

public:
	NiTexture* GetTextureAt(uint32_t uiIndex);

protected:
	// The base's members, then the affected map and each texture onto apDest.
	void CopyMembers(NiFlipController* pkDest, NiCloningProcess& kCloning);

public:
	// Grows the texture list to hold uiIndex and sets the texture there.
	void AttachTexture(NiTexture* pkTexture, uint32_t uiIndex);

	static NiObject* CreateObject();

	NiTObjectArray<NiPointer<NiTexture>> m_kTextures;
	uint32_t m_uiIndex;
	// the texturing map slot being flipped
	uint32_t m_uiAffectedMap;
	char* m_pcCtlrID;

	static NiRTTI ms_RTTI;
};
