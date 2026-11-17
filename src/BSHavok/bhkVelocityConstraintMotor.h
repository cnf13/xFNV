#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h"

class bhkBlendCollisionObject;
class bhkLimitedHingeConstraint;
class bhkNiCollisionObject;
class bhkRigidBody;
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

// The game's velocity motor: Havok's motor with Gamebryo's RTTI and type queries appended to its vtable.
class bhkVelocityConstraintMotor : public hkpVelocityConstraintMotor {
public:
    bhkVelocityConstraintMotor();
    ~bhkVelocityConstraintMotor() override;

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

    void LoadBinary(NiStream& arStream);
    // Writes the base's force limits, then the motor's own fields; called by hkConstraintCinfo::SaveMotor.
    void SaveBinary(NiStream& arStream);

    static NiRTTI ms_RTTI;
};
