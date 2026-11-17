#pragma once

#include "BSMain/BSNiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

class NiBillboardNode;
class NiCloningProcess;

// A scene graph node that carries an integer value.
class BSValueNode : public BSNiNode {
public:
	BSValueNode();
	BSValueNode(NiBillboardNode* a2);
	~BSValueNode() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void UpdateWorldData(NiUpdateData& UpdateData) override;
	virtual bool ParseNameForValue();

	void CopyMembers(BSValueNode* pDest, NiCloningProcess& arCloneData);
	// The node's value (iValue); the exe's shared one-field getter (thiscall, ret 0,
	// 0x9EE040).
	int32_t QValue();

	static NiObject* CreateObject();

	uint8_t m_uFlags;
	uint8_t padding0AD[3];
	int32_t iValue;
	NiObject* spReservedObject;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSValueNode_vftable{};
