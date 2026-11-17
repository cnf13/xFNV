#pragma once

#include "BSHavok/bhkBinaryAction.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

struct hkSpringActionCinfo;

// The Gamebryo side of a spring between two rigid bodies. It adds no fields to bhkBinaryAction (its CreateObject
// allocates 0x10 bytes).
class bhkSpringAction : public bhkBinaryAction {
public:
	bhkSpringAction();
	~bhkSpringAction() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) override;

	// Calls bhkBinaryAction::CopyCinfo, then copies the pivots, the spring's parameters and both flags into arInfo.
	// Called by CreateCinfo.
	void CopyCinfo(hkSpringActionCinfo& arInfo);

	// The engine's object factory for the class.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	static inline uint32_t iCount;
};

// The creation info of a spring action: the two pivots (bhkVector4, so the struct keeps Havok's 16-byte alignment) and
// the spring's parameters. The constructor sets both bools; bhkSpringAction::CreateCinfo allocates it aligned to 16.
struct hkSpringActionCinfo : public hkBinaryActionCinfo {
	hkSpringActionCinfo();

	bhkVector4 m_pivotA;
	bhkVector4 m_pivotB;
	float m_restLength;
	float m_strength;
	float m_damping;
	hkBool m_onCompression;
	hkBool m_onExtension;
};
