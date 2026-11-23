#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h"

#include <xmmintrin.h>

// The Jacobian elements the constraint builders write into the schemas. The right-hand side, the inverse diagonal and
// the user value live in the vectors' w components; their accessors are not written here.

// Two angular rows: the angular parts for body A and body B.
class hkp2AngJacobian {
public:
    hkVector4 m_angular[2];
};

// One linear row shared by both bodies and an angular row for each.
class hkp1Lin2AngJacobian {
public:
    // The angular terms times the bodies' inverse inertias plus their inverse masses (the w lanes), and
    // HK_REAL_EPSILON.
    float getDiag(const hkpVelocityAccumulator& mA, const hkpVelocityAccumulator& mB) const {
        __m128 ang0 = m_angular[0].m_quad;
        __m128 ang1 = m_angular[1].m_quad;
        __m128 invMassesA = mA.m_invMasses.m_quad;
        __m128 invMassesB = mB.m_invMasses.m_quad;
        __m128 terms = _mm_add_ps(
            _mm_mul_ps(_mm_mul_ps(ang0, ang0), invMassesA),
            _mm_mul_ps(_mm_mul_ps(ang1, ang1), invMassesB)
        );
        __m128 lanes = _mm_shuffle_ps(terms, _mm_unpackhi_ps(terms, _mm_add_ps(invMassesA, invMassesB)), 0xC4);
        __m128 sum2 = _mm_add_ps(_mm_shuffle_ps(lanes, lanes, 0x4E), lanes);
        __m128 sum = _mm_add_ps(_mm_shuffle_ps(sum2, sum2, 0xB1), sum2);
        float fSum = _mm_cvtss_f32(sum);
        return fSum + 1.1920928955078125e-07;
    }

    hkVector4 m_linear0;
    hkVector4 m_angular[2];
};

// A linear and an angular row for each body (the pulley).
class hkp2Lin2AngJacobian {
public:
    hkVector4 m_linear[2];
    hkVector4 m_angular[2];
};

// Two bilateral rows solved together (2d friction, a contact pair).
class hkpJacDouble2Bil {
public:
    hkp1Lin2AngJacobian m_jac0;
    hkp1Lin2AngJacobian m_jac1;
};

// Two bilateral rows and an angular one (3d friction).
class hkpJacTriple2Bil1Ang {
public:
    hkp1Lin2AngJacobian m_jac0;
    hkp1Lin2AngJacobian m_jac1;
    hkp2AngJacobian m_jac2;
};
