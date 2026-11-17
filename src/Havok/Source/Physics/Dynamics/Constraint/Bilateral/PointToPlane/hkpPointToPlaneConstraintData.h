#pragma once

#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpLinConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalTransformsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverResults.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"

// A constraint that keeps a pivot of one body on a plane of the other. It overrides isValid, getType, getRuntimeInfo
// and getConstraintInfo.
class hkpPointToPlaneConstraintData : public hkpConstraintData {
public:
    // The runtime memory of one instance.
    struct Runtime {
        hkpSolverResults m_solverResults[1];
    };

    // The atoms the constraint is made of.
    struct Atoms {
        // The reflection class hkpPointToPlaneConstraintDataAtomsClass.
        static const hkClass& staticClass();

        hkpSetLocalTransformsConstraintAtom m_transforms;
        hkpLinConstraintAtom m_lin;
    };

    hkpPointToPlaneConstraintData();
    // The SDK's finish constructor: the loaded atoms keep their values (the atoms' finish constructors do nothing).
    hkpPointToPlaneConstraintData(hkFinishLoadedObjectFlag aFlag) : hkpConstraintData(aFlag) {}

    hkBool* isValid(hkBool* arResult) const override;
    int32_t getType() const override;
    void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
    void getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const override;

    void setInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB, const hkVector4& planeNormalB);
    // Transforms pivotW into each body's space, and writes planeNormalW into B's rotation column at the axis index and
    // two perpendicular columns.
    void setInWorldSpace(
        const hkTransform& bodyATransform,
        const hkTransform& bodyBTransform,
        const hkVector4& pivotW,
        const hkVector4& planeNormalW
    );

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    // The atoms start on a 16-byte boundary, so the 4 bytes after the base class are the compiler's alignment padding.
    alignas(16) Atoms m_atoms;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpPointToPlaneConstraintDataAtomsTypeInfo;
extern hkClass hkpPointToPlaneConstraintDataAtomsClass;
extern hkClass hkpPointToPlaneConstraintDataClass;
extern hkTypeInfo hkpPointToPlaneConstraintDataTypeInfo;
