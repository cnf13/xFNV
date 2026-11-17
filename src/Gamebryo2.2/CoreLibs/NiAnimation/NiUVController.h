#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiUVData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"

// Animates the offset and tiling of its target's texture coordinates.
class NiUVController : public NiTimeController {
public:
	NiUVController();
	~NiUVController() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;
	void Update(NiUpdateData& UpdateData) override;
	void SetTarget(NiObjectNET* pkTarget) override;
	bool IsVertexController() const override { return true; }
	void OnPreDisplay() override;

protected:
	bool TargetIsRequiredType() const override;

public:
	// The U offset keys of the UV data.
	NiFloatKey* GetUOffsetData(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const;
	// The V offset keys of the UV data.
	NiFloatKey* GetVOffsetData(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const;
	// The U tiling keys of the UV data.
	NiFloatKey* GetUTilingData(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const;
	// The V tiling keys of the UV data.
	NiFloatKey* GetVTilingData(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const;
	// The key time range over the four channels' first and last keys.
	void ResetKeyTimeExtrema();
	void SetUVData(NiUVData* a1);

	static NiObject* CreateObject();

	uint32_t m_uiLastUOffsetIdx;
	uint32_t m_uiLastUTilingIdx;
	uint32_t m_uiLastVOffsetIdx;
	uint32_t m_uiLastVTilingIdx;
	uint16_t m_usTextureSet;
	NiPointer<NiUVData> m_spUVData;
	bool m_bNeedPreDisplay;

	static NiRTTI ms_RTTI;
};
