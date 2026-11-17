#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix6.h"

// Gauss-Jordan inversion without pivoting, on columns: this starts as the identity; for each column p of a copy of in,
// column p of both is scaled by the reciprocal of the copy's diagonal element (p, p), then every other column q of
// both gets column p times minus the copy's element (p, q) added, which clears row p of the copy outside the
// diagonal. A column's top half is a column of m_m[0][block], its bottom half one of m_m[1][block].
void hkMatrix6::setInvert(const hkMatrix6& tmpIn) {
    hkMatrix6 tmp = tmpIn;
    m_m[0][0].m_col0.setZero4();
    m_m[0][0].m_col1.setZero4();
    m_m[0][0].m_col2.setZero4();
    m_m[0][0](0, 0) = 1.0f;
    m_m[0][0](1, 1) = 1.0f;
    m_m[0][0](2, 2) = 1.0f;
    m_m[0][1].m_col0.setZero4();
    m_m[0][1].m_col1.setZero4();
    m_m[0][1].m_col2.setZero4();
    m_m[1][0].m_col0.setZero4();
    m_m[1][0].m_col1.setZero4();
    m_m[1][0].m_col2.setZero4();
    m_m[1][1].m_col0.setZero4();
    m_m[1][1].m_col1.setZero4();
    m_m[1][1].m_col2.setZero4();
    m_m[1][1](0, 0) = 1.0f;
    m_m[1][1](1, 1) = 1.0f;
    m_m[1][1](2, 2) = 1.0f;
    for (int32_t b = 0; b < 2; ++b) {
        for (int32_t k = 0; k < 3; ++k) {
            // The reciprocal is an x87 division stored as a float, then broadcast.
            const float fInv = 1.0f / tmp.m_m[b][b](k, k);
            const __m128 kInv = _mm_set1_ps(fInv);
            hkVector4& rTmpTop = (&tmp.m_m[0][b].m_col0)[k];
            hkVector4& rTmpBottom = (&tmp.m_m[1][b].m_col0)[k];
            hkVector4& rOutTop = (&m_m[0][b].m_col0)[k];
            hkVector4& rOutBottom = (&m_m[1][b].m_col0)[k];
            rTmpTop.m_quad = _mm_mul_ps(kInv, rTmpTop.m_quad);
            rTmpBottom.m_quad = _mm_mul_ps(kInv, rTmpBottom.m_quad);
            rOutTop.m_quad = _mm_mul_ps(kInv, rOutTop.m_quad);
            rOutBottom.m_quad = _mm_mul_ps(kInv, rOutBottom.m_quad);
            for (int32_t i = 0; i < 2; ++i) {
                for (int32_t j = 0; j < 3; ++j) {
                    if (i == b && j == k) {
                        continue;
                    }
                    // Minus the copy's element (row p, column q), an x87 fchs stored as a float.
                    const float fFactor = -tmp.m_m[b][i](k, j);
                    const __m128 kFactor = _mm_set1_ps(fFactor);
                    hkVector4& rTmpTopQ = (&tmp.m_m[0][i].m_col0)[j];
                    hkVector4& rTmpBottomQ = (&tmp.m_m[1][i].m_col0)[j];
                    hkVector4& rOutTopQ = (&m_m[0][i].m_col0)[j];
                    hkVector4& rOutBottomQ = (&m_m[1][i].m_col0)[j];
                    rTmpTopQ.m_quad = _mm_add_ps(_mm_mul_ps(kFactor, rTmpTop.m_quad), rTmpTopQ.m_quad);
                    rTmpBottomQ.m_quad = _mm_add_ps(_mm_mul_ps(kFactor, rTmpBottom.m_quad), rTmpBottomQ.m_quad);
                    rOutTopQ.m_quad = _mm_add_ps(_mm_mul_ps(kFactor, rOutTop.m_quad), rOutTopQ.m_quad);
                    rOutBottomQ.m_quad = _mm_add_ps(_mm_mul_ps(kFactor, rOutBottom.m_quad), rOutBottomQ.m_quad);
                }
            }
        }
    }
}

void hkMatrix6SetInvert(hkMatrix6& out, const hkMatrix6& in) {
    out.setInvert(in);
}

const hkMatrix6* hkMatrix6SetMulV(hkVector8& out, const hkMatrix6& a, const hkVector8& b) {
    const __m128 lin = b.m_lin.m_quad;
    __m128 v4 = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(lin, lin, 85), a.m_m[0][0].m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(lin, lin, 0), a.m_m[0][0].m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(lin, lin, 170), a.m_m[0][0].m_col2.m_quad)
    );
    out.m_lin.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(b.m_ang.m_quad, b.m_ang.m_quad, 85), a.m_m[0][1].m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(b.m_ang.m_quad, b.m_ang.m_quad, 0), a.m_m[0][1].m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(b.m_ang.m_quad, b.m_ang.m_quad, 170), a.m_m[0][1].m_col2.m_quad)
    );
    // v is read again after the store to out, as the game does (out may be v).
    __m128 v5 = b.m_ang.m_quad;
    const __m128 lin2 = b.m_lin.m_quad;
    __m128 v6 = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(lin2, lin2, 85), a.m_m[1][0].m_col1.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(lin2, lin2, 0), a.m_m[1][0].m_col0.m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(lin2, lin2, 170), a.m_m[1][0].m_col2.m_quad)
    );
    __m128 v7 = _mm_add_ps(
        _mm_mul_ps(_mm_shuffle_ps(v5, v5, 85), a.m_m[1][1].m_col1.m_quad),
        _mm_mul_ps(_mm_shuffle_ps(v5, v5, 0), a.m_m[1][1].m_col0.m_quad)
    );
    __m128 v8 = _mm_mul_ps(_mm_shuffle_ps(v5, v5, 170), a.m_m[1][1].m_col2.m_quad);
    out.m_lin.m_quad = _mm_add_ps(out.m_lin.m_quad, v4);
    out.m_ang.m_quad = _mm_add_ps(v7, v8);
    out.m_ang.m_quad = _mm_add_ps(out.m_ang.m_quad, v6);
    return &a;
}

// The SDK's inline hkMatrix6::setTranspose: each block of out is the transpose of in's mirrored block.
void hkMatrix6SetTranspose(hkMatrix6& out, const hkMatrix6& in) {
    out.m_m[0][0].setTranspose(in.m_m[0][0]);
    out.m_m[0][1].setTranspose(in.m_m[1][0]);
    out.m_m[1][0].setTranspose(in.m_m[0][1]);
    out.m_m[1][1].setTranspose(in.m_m[1][1]);
}

__m128* addMatrix6(__m128* a1, __m128* a2) {
    *a1 = _mm_add_ps(*a1, *a2);
    a1[1] = _mm_add_ps(a2[1], a1[1]);
    a1[2] = _mm_add_ps(a2[2], a1[2]);
    a1[3] = _mm_add_ps(a1[3], a2[3]);
    a1[4] = _mm_add_ps(a2[4], a1[4]);
    a1[5] = _mm_add_ps(a2[5], a1[5]);
    a1[6] = _mm_add_ps(a2[6], a1[6]);
    a1[7] = _mm_add_ps(a2[7], a1[7]);
    a1[8] = _mm_add_ps(a2[8], a1[8]);
    a1[9] = _mm_add_ps(a2[9], a1[9]);
    a1[10] = _mm_add_ps(a2[10], a1[10]);
    a1[11] = _mm_add_ps(a2[11], a1[11]);
    return a1;
}
