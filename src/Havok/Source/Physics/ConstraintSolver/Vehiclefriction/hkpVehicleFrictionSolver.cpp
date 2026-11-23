#include "Havok/Source/Physics/ConstraintSolver/Vehiclefriction/hkpVehicleFrictionSolver.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Animation/Animation/Compression/Spline/hkaNURBS.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Vehicle/Friction/hkpVehicleFriction.h"

#include <cmath>
#include <xmmintrin.h>

// Per axle: the wheel radius and its inverse, the surface inertia (the axle's angular inertia over the radius) and its
// inverse; the down force factors from each wheel's distance along the chassis's front axis; the chassis mass ratio
// from the inverse mass the chassis shows at the wheel's contact point (below the wheel by its radius along the up
// axis); and the friction circle table, 1 - (1 - x)^(1 / ratio) at 15 points x from xStart = ratio / 16, each step a
// factor (1 / xStart)^(1 / 15), the last entry 1.
void hkVehicleFrictionDescriptionInitValues(
    const hkpVehicleFrictionDescription::Cinfo& ci,
    hkpVehicleFrictionDescription& out
) {
    hkpVehicleFrictionDescription::AxisDescription& axle0 = out.m_axleDescr[0];
    hkpVehicleFrictionDescription::AxisDescription& axle1 = out.m_axleDescr[1];
    out.m_chassisMassInv = ci.m_chassisMassInv;
    axle0.m_wheelRadius = ci.m_wheelRadius[0];
    axle1.m_wheelRadius = ci.m_wheelRadius[1];
    axle0.m_wheelRadiusInv = 1.0 / ci.m_wheelRadius[0];
    axle1.m_wheelRadiusInv = 1.0 / ci.m_wheelRadius[1];
    axle0.m_wheelSurfaceInertia = ci.m_wheelAxleAngularInertia[0] * axle0.m_wheelRadiusInv;
    axle1.m_wheelSurfaceInertia = ci.m_wheelAxleAngularInertia[1] * axle1.m_wheelRadiusInv;
    axle0.m_wheelSurfaceInertiaInv = 1.0 / axle0.m_wheelSurfaceInertia;
    axle1.m_wheelSurfaceInertiaInv = 1.0 / axle1.m_wheelSurfaceInertia;

    const float d0 = ci.m_directionFront.dot3(ci.m_wheelPosition[0]);
    const float d1 = ci.m_wheelPosition[1].dot3(ci.m_directionFront);
    const float diff = d0 - d1;
    const float distance = std::fabs(diff);
    out.m_wheelDistance = distance;
    const float invDistance = 1.0 / distance;
    axle0.m_wheelDownForceFactor = 1.0 - ci.m_frictionEqualizer;
    const float rel0 = -d1 * invDistance;
    const float absRel0 = std::fabs(rel0);
    axle0.m_wheelDownForceSumFactor = absRel0 * ci.m_frictionEqualizer;
    axle1.m_wheelDownForceFactor = 1.0 - ci.m_frictionEqualizer;
    const float rel1 = d0 * invDistance;
    const float absRel1 = std::fabs(rel1);
    axle1.m_wheelDownForceSumFactor = absRel1 * ci.m_frictionEqualizer;

    // The inverse mass at each wheel's contact point along the right axis: the chassis's inverse mass plus
    // (r x right) . I^-1 (r x right), r the contact point relative to the centre of mass.
    const hkMatrix3& inertiaInv = ci.m_chassisFrictionInertiaInv;
    float massInvAtWheel[2];
    for (int32_t i = 0; i < 2; ++i) {
        const float negRadius = -ci.m_wheelRadius[i];
        const __m128 radiusSplat = _mm_set_ss(negRadius);
        hkVector4 r;
        r.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(radiusSplat, radiusSplat, 0), ci.m_directionUp.m_quad),
            _mm_sub_ps(ci.m_wheelPosition[i].m_quad, ci.m_chassisCenterOfMass.m_quad)
        );
        hkVector4 c;
        c.setCross(r, ci.m_directionRight);
        hkVector4 h;
        h.setMul3(inertiaInv, c);
        const float dot = h.dot3(c);
        massInvAtWheel[i] = out.m_chassisMassInv + dot;
    }

    for (int32_t i = 0; i < 2; ++i) {
        hkpVehicleFrictionDescription::AxisDescription& axle = out.m_axleDescr[i];
        const float ratio = massInvAtWheel[i] * axle.m_wheelSurfaceInertia;
        axle.m_wheelChassisMassRatio = ratio;
        const float xStart = ratio * 0.0625;
        axle.m_xStart = xStart;
        const float invXStart = 1.0 / xStart;
        const float xStep = std::pow(invXStart, 0.06666667014360428);
        axle.m_xStep = xStep;
        const float invRatio = 1.0 / ratio;
        float x = xStart;
        for (int32_t j = 0; j < 15; ++j) {
            const float base = 1.0 - x;
            const float power = std::pow(base, invRatio);
            axle.m_frictionCircleYtab[j] = 1.0 - power;
            x = xStep * x;
        }
        axle.m_frictionCircleYtab[15] = 1.0f;
    }
}

hkBool invert2x2(const float* apMatrix, float* apInverse, float afTolerance) {
    float fDet = apMatrix[0] * apMatrix[3] - apMatrix[2] * apMatrix[1];
    if (!(afTolerance * afTolerance > fDet * fDet)) {
        float fInvDet = 1.0 / fDet;
        double dInvDet = fInvDet;
        float fInv2 = -apMatrix[1] * fInvDet;
        float fInv1 = -apMatrix[2] * dInvDet;
        float fInv3 = apMatrix[0] * dInvDet;
        apInverse[0] = dInvDet * apMatrix[3];
        apInverse[2] = fInv2;
        apInverse[1] = fInv1;
        apInverse[3] = fInv3;
        return 1;
    }

    return 0;
}

void hkVehicleFrictionSolverCalcRelativeForces(
    hkpVehicleFrictionStatus::AxisStatus* axis,
    const hkpVehicleFrictionAxisTmp* status
) {
    axis->m_relativeSideForce = status->m_invMaxSideImpulse * status->m_sideImpulse;
    axis->m_relativeForwardForce = status->m_invMaxForwardImpulse * status->m_forwardImpulse;
}

// Clips one axle's forward and side impulses to its friction circle (the description's table) and keeps the forward
// impulse it cuts as the rest slip impulse.
void hkVehicleFrictionSolverClipImpulse(
    const hkpVehicleFrictionDescription::AxisDescription* apDescr,
    hkpVehicleFrictionStatus::AxisStatus* apStatus,
    hkpVehicleFrictionAxisTmp* apAxis
) {
    hkVehicleFrictionSolverCalcRelativeForces(apStatus, apAxis);
    float v43 = apStatus->m_relativeSideForce;
    float v38 = apStatus->m_relativeForwardForce;
    double v3 = v43;
    if (!(v38 * v38 + v3 * v3 < 1.0)) {
        double v10;
        double v14;
        double v15;
        float v33;
        int32_t v4 = 0;
        double v5 = v3 * apDescr->m_xStart;
        double v6 = v38;
        float v32 = v5;
        float v39 = 0.0f;
        float v44 = 0.0f;
        float v26 = 0.0f;
        float v35 = apDescr->m_frictionCircleYtab[0] * v6;
        double v7 = v35;
        double v8 = v32;
        float fValueb = v35 * v35 + v32 * v32;
        double v9 = fValueb;
        if (!(fValueb > 1.0)) {
            while (++v4 < 16) {
                v39 = v8;
                v44 = v7;
                v26 = v9;
                v32 = v8 * apDescr->m_xStep;
                v35 = apDescr->m_frictionCircleYtab[v4] * v6;
                float fValue = v35 * v35 + v32 * v32;
                if (fValue > 1.0) {
                    v10 = fValue;
                    goto found_crossing;
                }
                v7 = v35;
                v9 = fValue;
                v8 = v32;
            }
        }
        v10 = v9;
    found_crossing:;
        float fValuec = std::sqrt(v10);
        float fValued = fValuec - 1.0;
        float v27 = std::sqrt(static_cast<double>(v26));
        float v28 = v27 - 1.0;
        double v11 = fValued;
        double v12 = v28;
        float v29 = fValued - v28;
        float fValuee = v29 - 0.0;
        float fValuef = std::fabs(static_cast<double>(fValuee));
        if (fValuef < HK_REAL_EPSILON) {
            v29 = HK_REAL_EPSILON;
        }
        float v30 = 1.0 / v29;
        float fValuea = -v12 * v30;
        float v31 = v11 * v30;
        float v40 = v31 * v39 + fValuea * v32;
        double v13 = 1.1f;
        if (!(v40 < -1.1f)) {
            if (!(v40 > 1.1f)) {
                v13 = v40;
            }
            v33 = v13;
            v14 = -1.1f;
            v15 = 1.1f;
        }
        else {
            v14 = -1.1f;
            v15 = 1.1f;
            v33 = -1.1f;
        }
        double v16 = v14;
        double v17 = v15;
        float v45 = v31 * v44 + fValuea * v35;
        if (v45 < v14) {
            v17 = v16;
        }
        else {
            v16 = v45;
            if (!(v45 > v15)) {
                v17 = v16;
            }
        }
        float v36 = v17;
        float v46 = apAxis->m_maxSideImpulse * v33;
        float v41 = apAxis->m_maxForwardImpulse * v36;
        float v34 = apAxis->m_sideImpulse - v46;
        apAxis->m_sideImpulse = v46;
        apAxis->m_restForwardSlipImpulse = apAxis->m_forwardImpulse - v41;
        apAxis->m_forwardImpulse = v41;
        float v37 = apAxis->m_sideSlipVel * apAxis->m_sideSlipVel + apAxis->m_forward_vel * apAxis->m_forward_vel;
        float v47 = v37 - 0.0;
        float v48 = std::fabs(static_cast<double>(v47));
        if (v48 < HK_REAL_EPSILON) {
            v37 = HK_REAL_EPSILON;
        }
        float v42 = apAxis->m_invWheelForwardVirtMass * apAxis->m_restForwardSlipImpulse * apAxis->m_maxForwardImpulse;
        float v49 = apAxis->m_invSideVirtMass * v34 * apAxis->m_maxSideImpulse;
        float v50 = v49 * v49 + v42 * v42;
        double v18 = v50;
        float v51 = 1.0 / v37;
        float v52 = v18 * v51;
        float v53 = std::sqrt(static_cast<double>(v52));
        apStatus->m_skid_energy_density = v53;
    }
    else {
        apAxis->m_restForwardSlipImpulse = 0.0f;
    }
}
