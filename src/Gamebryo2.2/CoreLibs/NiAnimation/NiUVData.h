#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

NiSmartPointer(NiUVData);

// The float key arrays of a texture coordinate animation: U and V offset and tiling.
class NiUVData : public NiObject {
public:
	NiUVData();
	~NiUVData() override;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	// Each frees the channel's current keys and takes over pkKeys.
	void ReplaceUOffsetAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);
	void ReplaceVOffsetAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);
	void ReplaceUTilingAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);
	void ReplaceVTilingAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType);

	static NiObject* CreateObject();

	uint32_t m_uiNumUOffsetKeys;
	NiFloatKey* m_pkUOffsetKeys;
	NiFloatKey::KeyType m_eUOffsetType;
	uint32_t m_uiNumVOffsetKeys;
	NiFloatKey* m_pkVOffsetKeys;
	NiFloatKey::KeyType m_eVOffsetType;
	uint32_t m_uiNumUTilingKeys;
	NiFloatKey* m_pkUTilingKeys;
	NiFloatKey::KeyType m_eUTilingType;
	uint32_t m_uiNumVTilingKeys;
	NiFloatKey* m_pkVTilingKeys;
	NiFloatKey::KeyType m_eVTilingType;
	float m_fLastUOffset;
	float m_fLastVOffset;
	float m_fLastUTiling;
	float m_fLastVTiling;
	uint8_t m_ucUOffsetSize;
	uint8_t m_ucVOffsetSize;
	uint8_t m_ucUTilingSize;
	uint8_t m_ucVTilingSize;

	static NiRTTI ms_RTTI;
};
