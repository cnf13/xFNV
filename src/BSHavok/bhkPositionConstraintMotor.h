#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h"
#include "Reconstruction.hpp"

class BSFadeNode;
class BSMultiBoundNode;
class BSResizableTriShape;
class BSSegmentedTriShape;
class NiGeometry;
class NiLines;
class NiNode;
class NiParticles;
class NiStream;
class NiTriBasedGeom;
class NiTriShape;
class NiTriStrips;
class bhkBlendCollisionObject;
class bhkLimitedHingeConstraint;
class bhkNiCollisionObject;
class bhkRigidBody;

// The game's position motor: Havok's motor with Gamebryo's RTTI and type queries appended to its vtable.
class bhkPositionConstraintMotor : public hkpPositionConstraintMotor {
public:
	bhkPositionConstraintMotor();
	~bhkPositionConstraintMotor() override;

	virtual const NiRTTI* GetRTTI() const;
	virtual NiNode* IsNode();
	virtual BSFadeNode* IsFadeNode();
	virtual BSMultiBoundNode* IsMultiBoundNode();
	virtual NiGeometry* IsGeometry();
	virtual NiTriBasedGeom* IsTriBasedGeom();
	virtual NiTriStrips* IsTriStrips();
	virtual NiTriShape* IsTriShape();
	virtual BSSegmentedTriShape* IsSegmentedTriShape();
	virtual BSResizableTriShape* IsResizableTriShape();
	virtual NiParticles* IsParticlesGeom();
	virtual NiLines* IsLinesGeom();
	virtual bhkNiCollisionObject* IsBhkNiCollisionObject();
	virtual bhkBlendCollisionObject* IsBhkBlendCollisionObject();
	virtual bhkRigidBody* IsBhkRigidBody();
	virtual bhkLimitedHingeConstraint* IsBhkLimitedHingeConstraint();
	virtual void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings);

	// The base's force limits, then the motor's four floats. Not a vtable entry.
	void LoadBinary(NiStream& arStream);
	// The base's force limits, then the motor's four floats (thiscall, ret 4, 0xCF4260; called by
	// hkConstraintCinfo::SaveMotor). Not a vtable entry.
	void SaveBinary(NiStream& arStream);

	static NiRTTI ms_RTTI;
};

inline constexpr fnvd::Vtable bhkPositionConstraintMotor_vftable{};
