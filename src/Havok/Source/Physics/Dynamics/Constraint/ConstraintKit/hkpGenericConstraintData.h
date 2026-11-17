#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpGenericConstraintDataScheme.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkpConstraintQueryIn;
class hkpConstraintQueryOut;
class hkpGenericConstraintDataParameters;

// The angle about the angular axis aiAxis: atan2 of Bw's next-but-one and next axis, each dotted with Aw's next axis.
double calcAngleAboutAxis(const hkpGenericConstraintDataParameters* apVars, int32_t aiAxis);

// A constraint built at run time from a scheme of generic commands (the constraint kit), bridged to the solver through
// atoms. It overrides the destructor, isValid, getType, getRuntimeInfo, buildJacobian and getConstraintInfo.
class hkpGenericConstraintData : public hkpConstraintData {
public:
    hkpGenericConstraintData();
    // The finish constructor: a1 is the hkFinishLoadedObjectFlag's m_finishing.
    hkpGenericConstraintData(int32_t a1);
    ~hkpGenericConstraintData() override;

    hkBool* isValid(hkBool* arResult) const override;
    int32_t getType() const override;
    void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
    void buildJacobian(const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out) override;
    void getConstraintInfo(hkpConstraintData::ConstraintInfo& info) const override;

    // Havok SDK signature (hkpDynamics.lib, tied by reloc).
    hkpGenericConstraintDataScheme* getScheme();
    // hatchScheme runs the scheme's commands, each through one of the members below.
    void
    hatchScheme(hkpGenericConstraintDataScheme* scheme, const hkpConstraintQueryIn& inOrig, hkpConstraintQueryOut& out);
    hkVector4* getParameters(int32_t parameterIndex);
    void setParameters(int32_t parameterIndex, int32_t numParameters, const hkVector4* newValues);
    // Command members, called by hatchScheme's command switch: the command's hkpGenericConstraintDataScheme enumerator
    // selects the call.
    void constrainLinearW(
        int32_t*& currentCommand,
        hkVector4*& currentData,
        const hkpGenericConstraintDataScheme& scheme,
        hkpGenericConstraintDataParameters& vars,
        const hkpConstraintQueryIn& in,
        hkpConstraintQueryOut& out
    ) const;
    // e_constrainAllLinearW.
    void constrainAllLinearW(
        int32_t*& currentCommand,
        hkVector4*& currentData,
        const hkpGenericConstraintDataScheme& scheme,
        hkpGenericConstraintDataParameters& vars,
        const hkpConstraintQueryIn& in,
        hkpConstraintQueryOut& out
    ) const;
    // e_constrainToAngularW.
    void constrainToAngularW(
        int32_t*& currentCommand,
        hkVector4*& currentData,
        const hkpGenericConstraintDataScheme& scheme,
        hkpGenericConstraintDataParameters& vars,
        const hkpConstraintQueryIn& in,
        hkpConstraintQueryOut& out
    ) const;
    // e_constrainAllAngularW.
    void constrainAllAngularW(
        int32_t*& currentCommand,
        hkVector4*& currentData,
        const hkpGenericConstraintDataScheme& scheme,
        hkpGenericConstraintDataParameters& vars,
        const hkpConstraintQueryIn& in,
        hkpConstraintQueryOut& out
    ) const;
    // e_setLinearLimit.
    void setLinearLimitW(
        int32_t*& currentCommand,
        hkVector4*& currentData,
        const hkpGenericConstraintDataScheme& scheme,
        hkpGenericConstraintDataParameters& vars,
        const hkpConstraintQueryIn& in,
        hkpConstraintQueryOut& out
    ) const;
    // e_setAngularLimit.
    void setAngularLimitW(
        int32_t*& currentCommand,
        hkVector4*& currentData,
        const hkpGenericConstraintDataScheme& scheme,
        hkpGenericConstraintDataParameters& vars,
        const hkpConstraintQueryIn& in,
        hkpConstraintQueryOut& out
    ) const;
    // e_setConeLimit.
    void setConeLimitW(
        int32_t*& currentCommand,
        hkVector4*& currentData,
        const hkpGenericConstraintDataScheme& scheme,
        hkpGenericConstraintDataParameters& vars,
        const hkpConstraintQueryIn& in,
        hkpConstraintQueryOut& out
    ) const;
    // e_setTwistLimit.
    // Havok SDK signature (hkpDynamics.lib, tied by bytes).
    void setTwistLimitW(
        int32_t*& currentCommand,
        hkVector4*& currentData,
        const hkpGenericConstraintDataScheme& scheme,
        hkpGenericConstraintDataParameters& vars,
        const hkpConstraintQueryIn& in,
        hkpConstraintQueryOut& out
    ) const;
    // e_setAngularMotor.
    void setAngularMotorW(
        int32_t*& currentCommand,
        hkVector4*& currentData,
        hkpGenericConstraintDataScheme& scheme,
        hkpGenericConstraintDataParameters& vars,
        const hkpConstraintQueryIn& in,
        hkpConstraintQueryOut& out
    ) const;
    // e_setLinearMotor.
    void setLinearMotorW(
        int32_t*& currentCommand,
        hkVector4*& currentData,
        hkpGenericConstraintDataScheme& scheme,
        hkpGenericConstraintDataParameters& vars,
        const hkpConstraintQueryIn& in,
        hkpConstraintQueryOut& out
    ) const;
    // e_setAngularFriction.
    void setAngularFrictionW(
        int32_t*& currentCommand,
        hkVector4*& currentData,
        hkpGenericConstraintDataScheme& scheme,
        hkpGenericConstraintDataParameters& vars,
        const hkpConstraintQueryIn& in,
        hkpConstraintQueryOut& out
    ) const;
    // e_setLinearFriction.
    void setLinearFrictionW(
        int32_t*& currentCommand,
        hkVector4*& currentData,
        hkpGenericConstraintDataScheme& scheme,
        hkpGenericConstraintDataParameters& vars,
        const hkpConstraintQueryIn& in,
        hkpConstraintQueryOut& out
    ) const;

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    hkpBridgeAtoms m_atoms;
    hkpGenericConstraintDataScheme m_scheme;
};

// The next axis of each axis, {1, 2, 0, 1}: entry i is the next axis of i and entry i + 1 the one after.
using hkGenericConstraintDataAxisOrder_t = int32_t[4];
inline hkGenericConstraintDataAxisOrder_t hkGenericConstraintDataAxisOrder{1, 2, 0, 1};
