#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

// Animates one member of a texturing map's texture transform.
class NiTextureTransformController : public NiFloatInterpController {
public:
	enum TransformMember {
		TT_TRANSLATE_U,
		TT_TRANSLATE_V,
		TT_ROTATE,
		TT_SCALE_U,
		TT_SCALE_V,
	};

	NiTextureTransformController(NiTexturingProperty* pkTarget, NiTexturingProperty::Map* pkMap,
		TransformMember eMember);
	~NiTextureTransformController() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(NiUpdateData& UpdateData) override;
	void SetTarget(NiObjectNET* pkTarget) override;
	const char* GetCtlrID() override;

protected:
	bool InterpTargetIsCorrectType(NiObjectNET* pkTarget) const override;
	void GetTargetFloatValue(float& fValue) override;

	// caches the animated map of the target in m_pkMap; false while there is none
	bool SetInternalMap();

public:
	static NiObject* CreateObject();

	uint32_t m_uiLastIdx;
	NiTexturingProperty::Map* m_pkMap;
	bool m_bShaderMap;
	uint32_t m_uiMapIndex;
	TransformMember m_eMember;
	char* m_pcCtlrID;

	static NiRTTI ms_RTTI;
};
