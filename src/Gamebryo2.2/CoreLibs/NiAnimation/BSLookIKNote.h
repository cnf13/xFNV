#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/BSAnimNote.h"

class NiTextKey;

// A note that starts or stops look IK.
class BSLookIKNote : public BSAnimNote {
public:
	// Whether the note starts or stops looking.
	enum LOOKIK_STATE {
		BS_LIK_NONE,
		BS_LIK_START,
		BS_LIK_STOP,
	};

	BSLookIKNote();
	~BSLookIKNote() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	bool ParseTextKey(const NiTextKey* apTextKey) override;

	static NiObject* CreateObject();

	float fGain;
	LOOKIK_STATE eState;

	static NiRTTI ms_RTTI;
};
