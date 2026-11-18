#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppEarlyExitObbVirtualMachine.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverCheck.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"

#include <bit>
#include <xmmintrin.h>

hkBool hkpMoppEarlyExitObbVirtualMachine::queryObb(
    const hkpMoppCode* code,
    const hkTransform& BvToWorld,
    const hkVector4& extent,
    const float& radius
) {
    alignas(16) hkpMoppEarlyExitObbVirtualMachineQuery scaledQuery;
    if (hkpMoppEarlyExitObbVirtualMachine_HK_flyingcolors_mopp ||
        (hkpProcessFlyingColors(&hkpMoppEarlyExitObbVirtualMachine_HK_flyingcolors_mopp),
         hkpMoppEarlyExitObbVirtualMachine_HK_flyingcolors_mopp)) {
        m_code = code;
        uint8_t* m_data = code->m_data.m_data;
        generateQueryFromNode(extent, BvToWorld, radius, scaledQuery);
        scaledQuery.m_properties[0] = 0;
        m_hitFound.m_bool = 0;
        queryOBBOnTree(&scaledQuery, m_data);
        return m_hitFound.m_bool;
    }

    return 0;
}

// Walks the MOPP code from PC until a terminal under the box sets m_hitFound. scaledQuery holds the box in the current
// node's integer space; a rescale, a terminal offset or a property command moves it into query first, which the
// recursive calls are given. On a property command only property 0 is stored.
void hkpMoppEarlyExitObbVirtualMachine::queryOBBOnTree(
    const hkpMoppEarlyExitObbVirtualMachineQuery* scaledQuery,
    const uint8_t* PC
) {
    hkpMoppEarlyExitObbVirtualMachineQuery query;
    while (!m_hitFound.m_bool) {
        const int32_t command = *PC;
        // A two- or three-axis split: the box's centre and radius along the split's direction, the left child's
        // lower bound and the right child's upper bound.
        int32_t centre;
        int32_t radius;
        int32_t loSplit;
        int32_t hiSplit;
        int32_t property;
        uint32_t propertyValue;
        switch (command) {
            case 0: return;
            case 1:
            case 2:
            case 3:
            case 4: {
                // Rescale: the node's space is finer by command bits.
                const int32_t shift = command;
                query.m_offset_x = (PC[1] + scaledQuery->m_offset_x) << shift;
                query.m_offset_y = (PC[2] + scaledQuery->m_offset_y) << shift;
                query.m_offset_z = (PC[3] + scaledQuery->m_offset_z) << shift;
                query.m_shift = scaledQuery->m_shift + shift;
                const int32_t downShift = 16 - query.m_shift;
                query.m_xLo = (m_xLo >> downShift) - query.m_offset_x;
                query.m_yLo = (m_yLo >> downShift) - query.m_offset_y;
                query.m_zLo = (m_zLo >> downShift) - query.m_offset_z;
                query.m_xHi = (m_xHi >> downShift) + 1 - query.m_offset_x;
                query.m_yHi = (m_yHi >> downShift) + 1 - query.m_offset_y;
                query.m_zHi = (m_zHi >> downShift) + 1 - query.m_offset_z;
                query.m_cx = (m_cx >> downShift) - query.m_offset_x;
                query.m_cy = (m_cy >> downShift) - query.m_offset_y;
                query.m_cz = (m_cz >> downShift) - query.m_offset_z;
                query.m_cradius = (m_cradius >> downShift) + 1;
                query.m_properties[0] = scaledQuery->m_properties[0];
                query.m_primitiveOffset = scaledQuery->m_primitiveOffset;
                scaledQuery = &query;
                PC += 4;
                continue;
            }
            case 5: PC += PC[1] + 2; continue;
            case 6: PC += (PC[1] << 8) + PC[2] + 3; continue;
            case 7: PC += (((PC[1] << 8) + PC[2]) << 8) + PC[3] + 4; continue;
            case 9: {
                const int32_t offset = PC[1];
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                query.m_primitiveOffset += offset;
                PC += 2;
                continue;
            }
            case 10: {
                const int32_t offset = (PC[1] << 8) + PC[2];
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                query.m_primitiveOffset += offset;
                PC += 3;
                continue;
            }
            case 11: {
                const int32_t offset = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                query.m_primitiveOffset = offset;
                PC += 5;
                continue;
            }
            case 16:
            case 17:
            case 18: {
                // A split along one axis, the right child an 8-bit offset away.
                const int32_t axis = command - 16;
                const int32_t lo = PC[1];
                const int32_t hi = PC[2];
                PC += 4;
                if ((&scaledQuery->m_xHi)[axis] <= hi) {
                    continue;
                }
                const int32_t offset = PC[-1];
                PC += offset;
                if ((&scaledQuery->m_xLo)[axis] >= lo) {
                    continue;
                }
                queryOBBOnTree(scaledQuery, PC - offset);
                continue;
            }
            case 19:
                loSplit = 2 * PC[1];
                hiSplit = 2 * PC[2];
                centre = scaledQuery->m_cz + scaledQuery->m_cy;
                radius = (scaledQuery->m_cradius >> 1) + scaledQuery->m_cradius + 1;
                break;
            case 20:
                loSplit = 2 * PC[1];
                hiSplit = 2 * PC[2];
                centre = scaledQuery->m_cy - scaledQuery->m_cz + 255;
                radius = (scaledQuery->m_cradius >> 1) + scaledQuery->m_cradius + 1;
                break;
            case 21:
                loSplit = 2 * PC[1];
                hiSplit = 2 * PC[2];
                centre = scaledQuery->m_cz + scaledQuery->m_cx;
                radius = (scaledQuery->m_cradius >> 1) + scaledQuery->m_cradius + 1;
                break;
            case 22:
                loSplit = 2 * PC[1];
                hiSplit = 2 * PC[2];
                centre = scaledQuery->m_cx - scaledQuery->m_cz + 255;
                radius = (scaledQuery->m_cradius >> 1) + scaledQuery->m_cradius + 1;
                break;
            case 23:
                loSplit = 2 * PC[1];
                hiSplit = 2 * PC[2];
                centre = scaledQuery->m_cy + scaledQuery->m_cx;
                radius = (scaledQuery->m_cradius >> 1) + scaledQuery->m_cradius + 1;
                break;
            case 24:
                loSplit = 2 * PC[1];
                hiSplit = 2 * PC[2];
                centre = scaledQuery->m_cx - scaledQuery->m_cy + 255;
                radius = (scaledQuery->m_cradius >> 1) + scaledQuery->m_cradius + 1;
                break;
            case 25:
                loSplit = 3 * PC[1];
                hiSplit = 3 * PC[2];
                centre = scaledQuery->m_cz + scaledQuery->m_cy + scaledQuery->m_cx;
                radius = 4 * scaledQuery->m_cradius;
                break;
            case 26:
                loSplit = 3 * PC[1];
                hiSplit = 3 * PC[2];
                centre = scaledQuery->m_cy - scaledQuery->m_cz + scaledQuery->m_cx + 255;
                radius = 4 * scaledQuery->m_cradius;
                break;
            case 27:
                loSplit = 3 * PC[1];
                hiSplit = 3 * PC[2];
                centre = scaledQuery->m_cz - scaledQuery->m_cy + scaledQuery->m_cx + 255;
                radius = 4 * scaledQuery->m_cradius;
                break;
            case 28:
                loSplit = 3 * PC[1];
                hiSplit = 3 * PC[2];
                centre = scaledQuery->m_cx - scaledQuery->m_cz - scaledQuery->m_cy + 510;
                radius = 4 * scaledQuery->m_cradius;
                break;
            case 32:
            case 33:
            case 34: {
                // A single plane along one axis: the left child follows, the right one an 8-bit offset away.
                const int32_t axis = command - 32;
                const int32_t split = PC[1];
                PC += 3;
                if ((&scaledQuery->m_xHi)[axis] <= split) {
                    continue;
                }
                const int32_t offset = PC[-1];
                PC += offset;
                if ((&scaledQuery->m_xLo)[axis] > split) {
                    continue;
                }
                queryOBBOnTree(scaledQuery, PC - offset);
                continue;
            }
            case 35:
            case 36:
            case 37: {
                // A split along one axis with 16-bit offsets to both children.
                const int32_t axis = command - 35;
                const int32_t lo = PC[1];
                const int32_t hi = PC[2];
                const int32_t leftOffset = (PC[3] << 8) + PC[4];
                PC += 7;
                if ((&scaledQuery->m_xHi)[axis] <= hi) {
                    PC += leftOffset;
                    continue;
                }
                const int32_t rightOffset = (PC[-2] << 8) + PC[-1];
                PC += rightOffset;
                if ((&scaledQuery->m_xLo)[axis] >= lo) {
                    continue;
                }
                queryOBBOnTree(scaledQuery, PC + (leftOffset - rightOffset));
                continue;
            }
            case 38:
            case 39:
            case 40: {
                // Cuts the box against the node's bounds along one axis.
                const int32_t axis = command - 38;
                if ((&scaledQuery->m_xHi)[axis] < PC[1] || (&scaledQuery->m_xLo)[axis] >= PC[2]) {
                    return;
                }
                PC += 3;
                continue;
            }
            case 41:
            case 42:
            case 43: {
                // Cuts the unscaled box against 24-bit bounds along one axis.
                const int32_t axis = command - 41;
                if ((&m_xHi)[axis] < (((PC[1] << 8) + PC[2]) << 8) + PC[3] ||
                    (&m_xLo)[axis] > (((PC[4] << 8) + PC[5]) << 8) + PC[6]) {
                    return;
                }
                PC += 7;
                continue;
            }
            case 48:
            case 49:
            case 50:
            case 51:
            case 52:
            case 53:
            case 54:
            case 55:
            case 56:
            case 57:
            case 58:
            case 59:
            case 60:
            case 61:
            case 62:
            case 63:
            case 64:
            case 65:
            case 66:
            case 67:
            case 68:
            case 69:
            case 70:
            case 71:
            case 72:
            case 73:
            case 74:
            case 75:
            case 76:
            case 77:
            case 78:
            case 79:
            case 80:
            case 81:
            case 82:
            case 83:
                // Any terminal under the box is a hit: the walk ends.
                m_hitFound.m_bool = true;
                return;
            case 96:
            case 97:
            case 98:
            case 99:
                property = command - 96;
                propertyValue = PC[1];
                PC += 2;
                goto setProperty;
            case 100:
            case 101:
            case 102:
            case 103:
                property = command - 100;
                propertyValue = (PC[1] << 8) + PC[2];
                PC += 3;
                goto setProperty;
            case 104:
            case 105:
            case 106:
            case 107:
                property = command - 104;
                propertyValue = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
                PC += 5;
            setProperty:
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                if (property == 0) {
                    query.m_properties[0] = propertyValue;
                }
                continue;
            default: {
                // HK_ERROR; the same command is then read again.
                char buf[512];
                hkErrStream kStream(buf, 512);
                kStream.operator<<(
                    "Unknown command - This mopp data has been corrupted (check for memory trashing), or an "
                    "hkpMoppBvTreeShape has been pointed at invalid mopp data.\n"
                );
                if (hkError::s_instance->message(
                        hkError::MESSAGE_ERROR, 0x1298FEDD, buf,
                        ".\\Collide\\Mopp\\Machine\\hkpMoppEarlyExitObbVirtualMachine.cpp", 342
                    )) {
                    __builtin_debugtrap();
                }
                continue;
            }
        }
        // The two- and three-axis splits (19 to 28): the right child is an 8-bit offset away.
        PC += 4;
        if (radius + centre <= hiSplit) {
            continue;
        }
        const int32_t offset = PC[-1];
        PC += offset;
        if (centre >= radius + loSplit) {
            continue;
        }
        queryOBBOnTree(scaledQuery, PC - offset);
    }
}

// The box's half extent along the world axes (the absolute rotated extent plus the radius) around the transform's
// translation, in the code's integer space: the high and low corners, the centre and the radius, each rounded in
// the FPU's mode, and the root's query (their high 16 bits).
void hkpMoppEarlyExitObbVirtualMachine::generateQueryFromNode(
    const hkVector4& extent,
    const hkTransform& BvToWorld,
    const float& radius,
    hkpMoppEarlyExitObbVirtualMachineQuery& query
) {
    const __m128 e = extent.m_quad;
    __m128 halfExtent = _mm_add_ps(
        _mm_and_ps(
            _mm_mul_ps(_mm_shuffle_ps(e, e, 0x55), BvToWorld.m_rotation.m_col1.m_quad), hkVector4_setAbs4_absMask
        ),
        _mm_and_ps(
            _mm_mul_ps(_mm_shuffle_ps(e, e, 0x00), BvToWorld.m_rotation.m_col0.m_quad), hkVector4_setAbs4_absMask
        )
    );
    halfExtent = _mm_add_ps(
        halfExtent,
        _mm_and_ps(
            _mm_mul_ps(_mm_shuffle_ps(e, e, 0xAA), BvToWorld.m_rotation.m_col2.m_quad), hkVector4_setAbs4_absMask
        )
    );
    const __m128 r = _mm_load_ss(&radius);
    halfExtent = _mm_add_ps(halfExtent, _mm_shuffle_ps(r, r, 0));
    const __m128 lo = _mm_add_ps(BvToWorld.m_translation.m_quad, _mm_xor_ps(halfExtent, hkQuadRealNegateMask));
    const __m128 hi = _mm_add_ps(BvToWorld.m_translation.m_quad, halfExtent);
    const float xLo = (lo[0] - m_code->m_info.m_offset.m_quad[0]) * m_code->m_info.m_offset.m_quad[3];
    m_xLo = hkMath::hkToIntFast(xLo) - 1;
    const float xHi = (hi[0] - m_code->m_info.m_offset.m_quad[0]) * m_code->m_info.m_offset.m_quad[3];
    m_xHi = hkMath::hkToIntFast(xHi) + 1;
    const float yLo = (lo[1] - m_code->m_info.m_offset.m_quad[1]) * m_code->m_info.m_offset.m_quad[3];
    m_yLo = hkMath::hkToIntFast(yLo) - 1;
    const float yHi = (hi[1] - m_code->m_info.m_offset.m_quad[1]) * m_code->m_info.m_offset.m_quad[3];
    m_yHi = hkMath::hkToIntFast(yHi) + 1;
    const float zLo = (lo[2] - m_code->m_info.m_offset.m_quad[2]) * m_code->m_info.m_offset.m_quad[3];
    m_zLo = hkMath::hkToIntFast(zLo) - 1;
    const float zHi = (hi[2] - m_code->m_info.m_offset.m_quad[2]) * m_code->m_info.m_offset.m_quad[3];
    const int32_t zHiInt = hkMath::hkToIntFast(zHi) + 1;
    m_cx = (m_xLo + m_xHi) >> 1;
    m_cy = (m_yLo + m_yHi) >> 1;
    m_zHi = zHiInt;
    m_cz = (m_zLo + zHiInt) >> 1;
    // The radius: the extent's length plus radius.
    const __m128 squared = _mm_mul_ps(extent.m_quad, extent.m_quad);
    const __m128 sumXY = _mm_add_ss(_mm_shuffle_ps(squared, squared, 0x55), squared);
    const __m128 sumXYZ = _mm_add_ss(_mm_shuffle_ps(squared, squared, 0xAA), sumXY);
    const float length = _mm_cvtss_f32(_mm_sqrt_ss(sumXYZ));
    const float grown = radius + length;
    const float scaledRadius = grown * m_code->m_info.m_offset.m_quad[3];
    m_cradius = hkMath::hkToIntFast(scaledRadius) + 1;
    query.m_xLo = m_xLo >> 16;
    query.m_xHi = (m_xHi >> 16) + 1;
    query.m_yLo = m_yLo >> 16;
    query.m_yHi = (m_yHi >> 16) + 1;
    query.m_zLo = m_zLo >> 16;
    query.m_zHi = (m_zHi >> 16) + 1;
    query.m_cx = m_cx >> 16;
    query.m_cy = m_cy >> 16;
    query.m_cz = m_cz >> 16;
    query.m_cradius = (m_cradius >> 16) + 1;
    query.m_offset_x = 0;
    query.m_offset_y = 0;
    query.m_offset_z = 0;
    query.m_primitiveOffset = 0;
    query.m_shift = 0;
    query.m_properties[0] = 0;
}
