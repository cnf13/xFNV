#pragma once

#include "Fallout Shared/GlobalFunc.h"
#include "BSHavok/bhkUnaryAction.h"
#include "Reconstruction.hpp"

class bhkWorldObject;
class hkpEntity;
class hkpMouseSpringAction;

// The mouse spring's creation info: the base's user data and entity, the attachment point in the body's space, the
// target in world space and the spring's damping and force limits. The PDB's members and offsets; 0xC86D20 writes
// the vectors at 0x10 and 0x20 and the floats at 0x30, 0x34, 0x38 and 0x3c. The vectors sit on a 16-byte boundary,
// so 8 bytes of padding follow the 8-byte base (sizeof 0x40).
struct hkMouseSpringActionCinfo : public hkUnaryActionCinfo {
	hkMouseSpringActionCinfo();

	bhkVector4 m_positionInRbLocal;
	bhkVector4 m_mousePositionInWorld;
	float m_springDamping;
	float m_springElasticity;
	float m_maxRelativeForce;
	float m_objectDamping;
};

// A Havok action that pulls a body toward the mouse position with a spring (the debug picking of bodies); adds no
// data to bhkUnaryAction.
class bhkMouseSpringAction : public bhkUnaryAction {
public:
	bhkMouseSpringAction();
	bhkMouseSpringAction(const hkMouseSpringActionCinfo& arInfo);
	~bhkMouseSpringAction() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

	// The entity the spring pulls.
	hkpEntity* GetEntity();
	// Copies the spring's parameters into the creation info.
	void CopyCinfo(hkMouseSpringActionCinfo& arInfo);
	bhkWorldObject* GetWorldObject();
	// The Havok action (the exe's shared null-safe body getter, thiscall, ret 0, 0x4AE750).
	hkpMouseSpringAction* operator*() const;
	// IDA name GetMousePosInWorld; the PDB name. Copies the spring's target in world space to arPos.
	void GetMousePositionInWorld(hkVector4& arPos);
	// The PDB's game-unit overload: the spring's target in world space, in game units, to arPos (thiscall, ret 4,
	// 0x82AA50).
	void GetMousePositionInWorld(NiPoint3& arPos);
	// IDA name GetPositionInRbLocal; the PDB name. Copies the spring's attachment point in the body's space to arPos.
	void GetMousePositionInRBLocal(hkVector4& arPos);
	// IDA name _sub_8C5D90; the PDB name. Moves the spring's target to the game-unit point arPos, in Havok units
	// (thiscall, ret 4, 0x8C5D90).
	void SetMousePosition(const NiPoint3& arPos);
	// The PDB name. Moves the spring's target to arPos, when there is a wrapped action (thiscall, ret 4, 0x8C5DF0).
	void SetMousePosition(const hkVector4& arPos);

	// The engine's object factory for the class.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	static inline uint32_t iCount;
};
inline constexpr fnvd::Vtable bhkMouseSpringAction_vftable{};
