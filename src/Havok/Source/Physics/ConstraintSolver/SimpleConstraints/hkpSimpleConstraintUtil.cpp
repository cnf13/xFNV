#include "Havok/Source/Physics/ConstraintSolver/SimpleConstraints/hkpSimpleConstraintUtil.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverCheck.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpBoxMotion.h"

#include <cmath>
#include <xmmintrin.h>

float hkpSimpleConstraintInfo::BodyInfo::getEnergy(const hkpBodyVelocity& vel) const {
    const hkTransform* m_transform = this->m_transform;
    __m128 m_quad = m_transform->m_rotation.m_col1.m_quad;
    __m128 v4 = m_transform->m_rotation.m_col2.m_quad;
    __m128 v5 = _mm_unpacklo_ps(m_transform->m_rotation.m_col0.m_quad, m_quad);
    __m128 v6 = _mm_unpackhi_ps(m_transform->m_rotation.m_col0.m_quad, m_quad);
    __m128 v7 = _mm_movelh_ps(v5, v4);
    __m128 v8 = _mm_max_ps(m_invMasses.m_quad, hkQuadRealEps);
    __m128 v9 = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(vel.m_angular.m_quad, vel.m_angular.m_quad, 85),
                _mm_shuffle_ps(_mm_movehl_ps(v7, v5), v4, 212)
            ),
            _mm_mul_ps(_mm_shuffle_ps(vel.m_angular.m_quad, vel.m_angular.m_quad, 0), v7)
        ),
        _mm_mul_ps(_mm_shuffle_ps(v6, v4, 228), _mm_shuffle_ps(vel.m_angular.m_quad, vel.m_angular.m_quad, 170))
    );
    __m128 v10 = _mm_rcp_ps(v8);
    __m128 v11 = _mm_sub_ps(_mm_add_ps(v10, v10), _mm_mul_ps(_mm_mul_ps(v10, v8), v10));
    __m128 v12 = _mm_mul_ps(_mm_mul_ps(v9, v9), v11);
    __m128 v13 = _mm_shuffle_ps(v11, v11, 255);
    __m128 v14 =
        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v13, v13, 0), _mm_mul_ps(vel.m_linear.m_quad, vel.m_linear.m_quad)), v12);
    return _mm_shuffle_ps(v14, v14, 170)[0] + (_mm_shuffle_ps(v14, v14, 85)[0] + v14[0]);
}

// Per body: the jacobians are massRelPos crossed with each direction, the inverse inertia is applied to them, the
// jacobians are transposed in place and jac^T * invIjac is added to the inverse mass matrix, whose diagonal starts at
// the summed inverse masses. The body's mass is 1 / max(invMass, 1e-11f). The mass matrix is the inverse mass matrix's
// cofactors over its determinant (clamped below by 1e-33f), then column 0's w gets 1 / mass00 and column 1's w the
// inverse determinant of the lower 2x2 block, both clamped, with a NaN kept. Each product and sum keeps the game's
// association order.
void hkSimpleConstraintUtil_InitInfo(
    const hkpSimpleConstraintInfoInitInput& bodyA,
    const hkpSimpleConstraintInfoInitInput& bodyB,
    const hkMatrix3& directions,
    hkpSimpleConstraintInfo& out
) {
    const float fInvMassSum = bodyA.m_invMasses.m_quad[3] + bodyB.m_invMasses.m_quad[3];
    hkMatrix3& invMassMatrix = out.m_invMassMatrix;
    const __m128 kZero = _mm_setzero_ps();
    _mm_store_ps(&reinterpret_cast<float*>(&invMassMatrix.m_col0.m_quad)[0], kZero);
    _mm_store_ps(&reinterpret_cast<float*>(&invMassMatrix.m_col1.m_quad)[0], kZero);
    _mm_store_ps(&reinterpret_cast<float*>(&invMassMatrix.m_col2.m_quad)[0], kZero);
    invMassMatrix.m_col0.m_quad[0] = fInvMassSum;
    invMassMatrix.m_col1.m_quad[1] = fInvMassSum;
    invMassMatrix.m_col2.m_quad[2] = fInvMassSum;
    _mm_store_ps(
        &reinterpret_cast<float*>(&out.m_base.m_col0.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<const float*>(&directions.m_col0.m_quad)[0])
    );
    _mm_store_ps(
        &reinterpret_cast<float*>(&out.m_base.m_col1.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<const float*>(&directions.m_col1.m_quad)[0])
    );
    _mm_store_ps(
        &reinterpret_cast<float*>(&out.m_base.m_col2.m_quad)[0],
        _mm_load_ps(&reinterpret_cast<const float*>(&directions.m_col2.m_quad)[0])
    );

    const hkpSimpleConstraintInfoInitInput* pBody = &bodyA;
    for (int32_t iBody = 0; iBody < 2; ++iBody) {
        hkpSimpleConstraintInfo::BodyInfo& info = out.m_bodyInfo[iBody];
        hkVector4* pJacT = &info.m_jacT.m_col0;

        for (int32_t j = 0; j < 3; ++j) {
            pJacT[j].setCross(pBody->m_massRelPos, directions.getColumn(j));
        }

        info.m_invIjac.setMul(pBody->m_invInertia, info.m_jacT);

        info.m_jacT.transpose();

        hkMatrix3 tmp;
        tmp.setMul(info.m_jacT, info.m_invIjac);
        invMassMatrix.add(tmp);

        _mm_store_ps(
            &reinterpret_cast<float*>(&info.m_invMasses.m_quad)[0],
            _mm_load_ps(&reinterpret_cast<const float*>(&pBody->m_invMasses.m_quad)[0])
        );
        const float fInvMass = pBody->m_invMasses.m_quad[3];
        const float fClamped = (fInvMass > 1e-11f) ? fInvMass : 1e-11f;
        info.m_mass = 1.0f / fClamped;
        info.m_transform = pBody->m_transform;
        pBody = &bodyB;
    }

    // EXE-INLINED none: hkMatrix3::invert
    const __m128 kC1 = _mm_load_ps(&reinterpret_cast<float*>(&invMassMatrix.m_col1.m_quad)[0]);
    const __m128 kC2 = _mm_load_ps(&reinterpret_cast<float*>(&invMassMatrix.m_col2.m_quad)[0]);
    const __m128 kC0 = _mm_load_ps(&reinterpret_cast<float*>(&invMassMatrix.m_col0.m_quad)[0]);
    const __m128 kR0 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kC2, kC2, 0xD2), _mm_shuffle_ps(kC1, kC1, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(kC2, kC2, 0xC9), _mm_shuffle_ps(kC1, kC1, 0xD2))
    );
    const __m128 kR1 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kC2, kC2, 0xC9), _mm_shuffle_ps(kC0, kC0, 0xD2)),
        _mm_mul_ps(_mm_shuffle_ps(kC2, kC2, 0xD2), _mm_shuffle_ps(kC0, kC0, 0xC9))
    );
    const __m128 kR2 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kC0, kC0, 0xC9), _mm_shuffle_ps(kC1, kC1, 0xD2)),
        _mm_mul_ps(_mm_shuffle_ps(kC0, kC0, 0xD2), _mm_shuffle_ps(kC1, kC1, 0xC9))
    );
    const __m128 kD = _mm_mul_ps(kR0, kC0);
    // Only lane 0 is read: z + (y + x), at least 1e-33f, then 1 over it splatted.
    const __m128 kDet = _mm_add_ps(_mm_shuffle_ps(kD, kD, 0xAA), _mm_add_ps(_mm_shuffle_ps(kD, kD, 0x55), kD));
    const __m128 kInvDet = _mm_div_ps(_mm_set1_ps(1.0f), _mm_max_ps(kDet, _mm_set_ss(1e-33f)));
    const __m128 kScale = _mm_shuffle_ps(kInvDet, kInvDet, 0x00);
    _mm_store_ps(&reinterpret_cast<float*>(&out.m_massMatrix.m_col0.m_quad)[0], _mm_mul_ps(kR0, kScale));
    _mm_store_ps(&reinterpret_cast<float*>(&out.m_massMatrix.m_col1.m_quad)[0], _mm_mul_ps(kR1, kScale));
    _mm_store_ps(&reinterpret_cast<float*>(&out.m_massMatrix.m_col2.m_quad)[0], _mm_mul_ps(kR2, kScale));

    // EXE-INLINED none: hkpSimpleConstraintInfo::getMass00
    float fMass00 = invMassMatrix.m_col0.m_quad[0];
    if (fMass00 < 9.999999960041972e-12) {
        fMass00 = 1e-11f;
    }
    invMassMatrix.m_col0.m_quad[3] = 1.0f / fMass00;
    float fDet12 = invMassMatrix.m_col2.m_quad[2] * invMassMatrix.m_col1.m_quad[1] -
        invMassMatrix.m_col2.m_quad[1] * invMassMatrix.m_col1.m_quad[2];
    if (fDet12 < 9.999999682655225e-23) {
        fDet12 = 9.9999997e-23f;
    }
    // EXE-INLINED none: hkpSimpleConstraintInfo::getInvDetM12
    invMassMatrix.m_col1.m_quad[3] = 1.0f / fDet12;
}

const hkpSimpleConstraintInfo* hkSimpleConstraintUtil_applyImpulse(
    const hkpSimpleConstraintInfo& info,
    const hkVector4& impulse,
    hkpBodyVelocity& velA,
    hkpBodyVelocity& velB
) {
    __m128 v5 = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(impulse.m_quad, impulse.m_quad, 85), info.m_base.m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(impulse.m_quad, impulse.m_quad, 0), info.m_base.m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(impulse.m_quad, impulse.m_quad, 170), info.m_base.m_col2.m_quad)
    );
    __m128 v6 = _mm_shuffle_ps(info.m_bodyInfo[0].m_invMasses.m_quad, info.m_bodyInfo[0].m_invMasses.m_quad, 255);
    velA.m_linear.m_quad = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v6, v6, 0), v5), velA.m_linear.m_quad);
    __m128 v7 = _mm_shuffle_ps(info.m_bodyInfo[1].m_invMasses.m_quad, info.m_bodyInfo[1].m_invMasses.m_quad, 255);
    velB.m_linear.m_quad = _mm_sub_ps(velB.m_linear.m_quad, _mm_mul_ps(_mm_shuffle_ps(v7, v7, 0), v5));
    velA.m_angular.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(impulse.m_quad, impulse.m_quad, 85), info.m_bodyInfo[0].m_invIjac.m_col1.m_quad
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(impulse.m_quad, impulse.m_quad, 0), info.m_bodyInfo[0].m_invIjac.m_col0.m_quad
                )
            ),
            _mm_mul_ps(_mm_shuffle_ps(impulse.m_quad, impulse.m_quad, 170), info.m_bodyInfo[0].m_invIjac.m_col2.m_quad)
        ),
        velA.m_angular.m_quad
    );
    velB.m_angular.m_quad = _mm_sub_ps(
        velB.m_angular.m_quad,
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(impulse.m_quad, impulse.m_quad, 85), info.m_bodyInfo[1].m_invIjac.m_col1.m_quad
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(impulse.m_quad, impulse.m_quad, 0), info.m_bodyInfo[1].m_invIjac.m_col0.m_quad
                )
            ),
            _mm_mul_ps(_mm_shuffle_ps(impulse.m_quad, impulse.m_quad, 170), info.m_bodyInfo[1].m_invIjac.m_col2.m_quad)
        )
    );
    velA.m_angular.m_quad = _mm_and_ps(velA.m_angular.m_quad, hkVector4_zeroElement_cx[3]);
    velB.m_angular.m_quad = _mm_and_ps(velB.m_angular.m_quad, hkVector4_zeroElement_cx[3]);
    velA.m_linear.m_quad = _mm_and_ps(velA.m_linear.m_quad, hkVector4_zeroElement_cx[3]);
    velB.m_linear.m_quad = _mm_and_ps(velB.m_linear.m_quad, hkVector4_zeroElement_cx[3]);
    return &info;
}

hkVector4* hkSimpleConstraintUtil_getPointVelocity(
    const hkpSimpleConstraintInfo& info,
    hkpBodyVelocity& velA,
    hkpBodyVelocity& velB,
    hkVector4& out
) {
    __m128 v4 = info.m_base.m_col1.m_quad;
    __m128 v5 = info.m_base.m_col2.m_quad;
    __m128 v6 = _mm_sub_ps(velA.m_linear.m_quad, velB.m_linear.m_quad);
    __m128 v7 = info.m_base.m_col0.m_quad;
    __m128 v8 = _mm_unpackhi_ps(v7, v4);
    __m128 v9 = _mm_unpacklo_ps(v7, v4);
    __m128 v11 = velA.m_angular.m_quad;
    __m128 v13 = _mm_movelh_ps(v9, v5);
    out.m_quad = _mm_add_ps(
        _mm_sub_ps(
            _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(v11, v11, 85), info.m_bodyInfo[0].m_jacT.m_col1.m_quad),
                    _mm_mul_ps(_mm_shuffle_ps(v11, v11, 0), info.m_bodyInfo[0].m_jacT.m_col0.m_quad)
                ),
                _mm_mul_ps(_mm_shuffle_ps(v11, v11, 170), info.m_bodyInfo[0].m_jacT.m_col2.m_quad)
            ),
            _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(velB.m_angular.m_quad, velB.m_angular.m_quad, 85),
                        info.m_bodyInfo[1].m_jacT.m_col1.m_quad
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(velB.m_angular.m_quad, velB.m_angular.m_quad, 0),
                        info.m_bodyInfo[1].m_jacT.m_col0.m_quad
                    )
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(velB.m_angular.m_quad, velB.m_angular.m_quad, 170),
                    info.m_bodyInfo[1].m_jacT.m_col2.m_quad
                )
            )
        ),
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v6, v6, 85), _mm_shuffle_ps(_mm_movehl_ps(v13, v9), v5, 212)),
                _mm_mul_ps(_mm_shuffle_ps(v6, v6, 0), v13)
            ),
            _mm_mul_ps(_mm_shuffle_ps(v8, v5, 228), _mm_shuffle_ps(v6, v6, 170))
        )
    );
    return &out;
}

double hkSimpleConstraintUtil_Collide(
    const hkpSimpleConstraintInfo& info,
    hkpSimpleConstraintUtilCollideParams& params,
    hkpBodyVelocity& velA,
    hkpBodyVelocity& velB
) {
    hkVector4 v41;
    hkVector4 v42;
    if (!HK_flyingcolors_constraintutil) {
        hkpProcessFlyingColors(&HK_flyingcolors_constraintutil);
        if (!HK_flyingcolors_constraintutil) {
            return 0.0;
        }
    }
    hkSimpleConstraintUtil_getPointVelocity(info, velA, velB, v41);
    if (params.m_externalSeperatingVelocity < static_cast<double>(v41.m_quad[0])) {
        v41.m_quad[0] = params.m_externalSeperatingVelocity;
        if (params.m_extraSeparatingVelocity <= static_cast<double>(params.m_externalSeperatingVelocity)) {
            return 0.0;
        }
    }
    double m_restitution = params.m_restitution;
    __m128 v6 = info.m_massMatrix.m_col1.m_quad;
    __m128 v7 = info.m_massMatrix.m_col0.m_quad;
    v41.m_quad = _mm_xor_ps(v41.m_quad, hkQuadRealNegateMask);
    float v32 = m_restitution * v41.m_quad[0];
    v41.m_quad[0] = v41.m_quad[0] + (params.m_extraSeparatingVelocity + v32);
    __m128 v8 = v41.m_quad;
    float m_maxImpulse = params.m_maxImpulse;
    __m128 v9 = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v41.m_quad, v41.m_quad, 85), v6),
            _mm_mul_ps(_mm_shuffle_ps(v41.m_quad, v41.m_quad, 0), v7)
        ),
        _mm_mul_ps(_mm_shuffle_ps(v41.m_quad, v41.m_quad, 170), info.m_massMatrix.m_col2.m_quad)
    );
    v42.m_quad = v9;
    double result = v9[0];
    params.m_contactImpulseLimitBreached.m_bool = 0;
    if (m_maxImpulse < static_cast<double>(v9[0])) {
        const float fScale = m_maxImpulse / result;
        v9 = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fScale), _mm_set_ss(fScale), 0), v9);
        v42.m_quad = v9;
        params.m_contactImpulseLimitBreached.m_bool = 1;
        result = v9[0];
    }
    double m_friction = params.m_friction;
    __m128 v43 = _mm_mul_ps(v9, v9);
    float v26 = v43[2] + v43[1];
    double v11 = m_friction * m_friction * v43[0];
    double v12 = v26;
    float v27 = v11;
    if (v27 < v12 || result < 0.0) {
        double v21;
        double v13 = m_maxImpulse;
        v42.m_quad = _mm_setzero_ps();
        float v39 = v41.m_quad[2] * v41.m_quad[2] + v41.m_quad[1] * v41.m_quad[1];
        float v23 = v41.m_quad[0] * info.m_invMassMatrix.m_col0.m_quad[3];
        double v14 = v23;
        if (v23 > static_cast<double>(m_maxImpulse)) {
            params.m_contactImpulseLimitBreached.m_bool = 1;
            v23 = m_maxImpulse;
            v14 = m_maxImpulse;
        }
        double v15 = info.m_invMassMatrix.m_col2.m_quad[2];
        __m128 v16 = _mm_sub_ps(
            v8, _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v23), _mm_set_ss(v23), 0), info.m_invMassMatrix.m_col0.m_quad)
        );
        v41.m_quad = v16;
        float v24 =
            (v15 * v16[1] - v16[2] * info.m_invMassMatrix.m_col2.m_quad[1]) * info.m_invMassMatrix.m_col1.m_quad[3];
        float v28 = (v16[2] * info.m_invMassMatrix.m_col1.m_quad[1] - v16[1] * info.m_invMassMatrix.m_col2.m_quad[1]) *
            info.m_invMassMatrix.m_col1.m_quad[3];
        float v35 = v28 * v28 + v24 * v24;
        float v33 = params.m_friction * v14;
        float v34 = v33 * v33;
        if (v34 < static_cast<double>(v35)) {
            float v36 = v34 / v35;
            float v37 = std::sqrt((double)v36);
            v16 = v41.m_quad;
            v24 = v37 * v24;
            v28 = v37 * v28;
            v14 = v23;
            v13 = m_maxImpulse;
        }
        v42.m_quad[1] = v24;
        v42.m_quad[2] = v28;
        __m128 v17 = _mm_sub_ps(
            _mm_sub_ps(
                v16, _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v24), _mm_set_ss(v24), 0), info.m_invMassMatrix.m_col1.m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v28), _mm_set_ss(v28), 0), info.m_invMassMatrix.m_col2.m_quad)
        );
        v41.m_quad = v17;
        float v38 = (v17[0] - v32) * info.m_invMassMatrix.m_col0.m_quad[3];
        double v18 = v38;
        if (v38 < 0.0) {
            v18 = 0.0;
        }
        float v29 = v18;
        if (!(v29 + v14 > v13)) {
            v21 = v14;
        }
        else {
            params.m_contactImpulseLimitBreached.m_bool = 1;
            double v20 = v13 - v14;
            v21 = v14;
            v29 = v20;
        }
        __m128 v22 = info.m_invMassMatrix.m_col0.m_quad;
        v42.m_quad[0] = v21 + v29;
        v41.m_quad = _mm_sub_ps(v17, _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v29), _mm_set_ss(v29), 0), v22));
        float v30 = v41.m_quad[2] * v41.m_quad[2] + v41.m_quad[1] * v41.m_quad[1];
        if (!(v39 > static_cast<double>(v30))) {
            v30 = v39;
        }
        float v40 = std::sqrt((double)v30);
        v42.m_quad[0] = v40 * info.m_invMassMatrix.m_col0.m_quad[3] * params.m_extraSlope + v42.m_quad[0];
        result = v42.m_quad[0];
        if (m_maxImpulse < static_cast<double>(v42.m_quad[0])) {
            params.m_contactImpulseLimitBreached.m_bool = 1;
            v42.m_quad[0] = m_maxImpulse;
            result = m_maxImpulse;
        }
    }
    hkSimpleConstraintUtil_applyImpulse(info, v42, velA, velB);
    return result;
}
