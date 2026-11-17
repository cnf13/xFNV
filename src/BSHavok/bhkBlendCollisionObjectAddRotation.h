#pragma once

#include "BSHavok/bhkBlendCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Reconstruction.hpp"

// A blend collision object that turns its node by an additional rotation (the gun wobble).
class bhkBlendCollisionObjectAddRotation : public bhkBlendCollisionObject {
public:
	bhkBlendCollisionObjectAddRotation();
	virtual ~bhkBlendCollisionObjectAddRotation();

	const NiRTTI* GetRTTI() const override { return &bhkBlendCollisionObjectAddRotation::ms_RTTI; }
	bool RegisterStreamables(NiStream& arStream) override;
	// A new object under the Havok heap context: the base's construction, the identity rotation, the copied members,
	// then this object's additional rotation.
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	// The four stream and compare slots jump straight to the base's (thunks in the exe).
	void LoadBinary(NiStream& arStream) override { bhkBlendCollisionObject::LoadBinary(arStream); }
	void LinkObject(NiStream& arStream) override { bhkBlendCollisionObject::LinkObject(arStream); }
	void SaveBinary(NiStream& arStream) override { bhkBlendCollisionObject::SaveBinary(arStream); }
	bool IsEqual(NiObject* apObject) override { return bhkBlendCollisionObject::IsEqual(apObject); }
	void AddViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void UpdateWorldData(NiUpdateData& UpdateData) override;

	// Takes over apBlendObj's world object and blend settings.
	void CopyFrom(bhkBlendCollisionObject* apBlendObj);
	void SetAdditionalRotation(const NiMatrix3& aAdditionalRotation);
	// Copies the blend members into apDest when cloning (the additional rotation is not copied).
	void CopyMembers(bhkBlendCollisionObjectAddRotation* apDest, NiCloningProcess& arCloneData);

	NiMatrix3 AdditionalRotation;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable bhkBlendCollisionObjectAddRotation_vftable{};
