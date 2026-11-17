#pragma once

#include "Fallout Shared/GlobalFunc.h"
#include "BSHavok/bhkBinaryAction.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

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

	// bhkBinaryAction::CopyCinfo, then the pivots, the spring's parameters and both flags into arInfo (thiscall, ret 4,
	// 0xCC2B60; CreateCinfo's callee). Protected in the PDB. The exe leaves the last flag in al; CreateCinfo ignores it
	// (PDB: void).
	void CopyCinfo(hkSpringActionCinfo& arInfo);

	// The engine's object factory for the class.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	static inline uint32_t iCount;
};

// The creation info of a spring action: the two pivots (bhkVector4, so the struct keeps Havok's 16-byte alignment)
// and the spring's parameters. The PDB's members; the PDB defines it in this file's .cpp. The constructor (0xCC2F40)
// sets both bools at 0x3C and 0x3D; bhkSpringAction::CreateCinfo (0xCC2F90) allocates it in 0x50 bytes aligned to 16.
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

inline constexpr fnvd::Vtable bhkSpringAction_vftable{};
