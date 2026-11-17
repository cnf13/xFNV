#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Reconstruction.hpp"

// The "TXD" extra data of a model: the objects its textures belong to.
class BSTextureData : public NiExtraData {
public:
	BSTextureData();
	~BSTextureData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	// The extra data's part, then the object array copied into pDest's.
	void CopyMembers(BSTextureData* pDest, NiCloningProcess& kCloning);
	NiAVObject* GetValue(uint32_t aiIndex) const;
	int32_t AddValue(NiAVObject* aObject);

	// Creates the "TXD" tag.
	static void InitSDM();
	static NiObject* CreateObject();
	static void KillSDM();

	BSSimpleArray<NiAVObject*> pObjects;

	static inline NiFixedString* pTag;
	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSTextureData_vftable{};
