#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppObbVirtualMachine.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverCheck.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"
#include "SDK/MSVC/string.h"

// Walks the MOPP code from PC, appending every terminal whose node the box overlaps to m_primitives_out. query holds
// the box in the current node's integer space; a rescale, a terminal offset or a property command moves it into
// scaledQuery first, which the recursive calls are given. On a property command, only property 0 is stored.
void hkpMoppObbVirtualMachine::queryAabbOnTree(
    const hkpMoppObbVirtualMachineQuery* query,
    const uint8_t* PC,
    int32_t chunkOffset
) {
    if (!hkpMoppObbVirtualMachine_HK_flyingcolors_mopp) {
        hkpProcessFlyingColors(&hkpMoppObbVirtualMachine_HK_flyingcolors_mopp);
        if (!hkpMoppObbVirtualMachine_HK_flyingcolors_mopp) {
            return;
        }
    }
    hkpMoppObbVirtualMachineQuery scaledQuery;
    while (true) {
        const int32_t command = *PC;
        int32_t lo;
        int32_t hi;
        int32_t loSplit;
        int32_t hiSplit;
        int32_t terminal;
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
                scaledQuery.m_offset_x = (query->m_offset_x + PC[1]) << shift;
                scaledQuery.m_offset_y = (query->m_offset_y + PC[2]) << shift;
                scaledQuery.m_offset_z = (query->m_offset_z + PC[3]) << shift;
                scaledQuery.m_shift = query->m_shift + shift;
                const int32_t downShift = 16 - scaledQuery.m_shift;
                scaledQuery.m_xLo = (m_xLo >> downShift) - scaledQuery.m_offset_x;
                scaledQuery.m_yLo = (m_yLo >> downShift) - scaledQuery.m_offset_y;
                scaledQuery.m_zLo = (m_zLo >> downShift) - scaledQuery.m_offset_z;
                scaledQuery.m_xHi = (m_xHi >> downShift) - scaledQuery.m_offset_x + 1;
                scaledQuery.m_yHi = (m_yHi >> downShift) - scaledQuery.m_offset_y + 1;
                scaledQuery.m_zHi = (m_zHi >> downShift) - scaledQuery.m_offset_z + 1;
                scaledQuery.m_properties[0] = query->m_properties[0];
                scaledQuery.m_primitiveOffset = query->m_primitiveOffset;
                query = &scaledQuery;
                PC += 4;
                continue;
            }
            case 5: PC += PC[1] + 2; continue;
            case 6: PC += (PC[1] << 8) + PC[2] + 3; continue;
            case 7: PC += (((PC[1] << 8) + PC[2]) << 8) + PC[3] + 4; continue;
            case 9: {
                const int32_t offset = PC[1];
                if (query != &scaledQuery) {
                    scaledQuery = *query;
                    query = &scaledQuery;
                }
                scaledQuery.m_primitiveOffset += offset;
                PC += 2;
                continue;
            }
            case 10: {
                const int32_t offset = (PC[1] << 8) + PC[2];
                if (query != &scaledQuery) {
                    scaledQuery = *query;
                    query = &scaledQuery;
                }
                scaledQuery.m_primitiveOffset += offset;
                PC += 3;
                continue;
            }
            case 11: {
                const int32_t offset = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
                if (query != &scaledQuery) {
                    scaledQuery = *query;
                    query = &scaledQuery;
                }
                scaledQuery.m_primitiveOffset = offset;
                PC += 5;
                continue;
            }
            case 12:
                chunkOffset = ((PC[1] << 8) | PC[2]) << 9;
                PC = &m_code.m_storage->m_data.m_data[chunkOffset];
                continue;
            case 13: PC += 5; continue;
            case 16:
            case 17:
            case 18: {
                // A split along one axis, the right child an 8-bit offset away.
                const int32_t axis = command - 16;
                loSplit = PC[1];
                hiSplit = PC[2];
                PC += 4;
                const int32_t queryLo = (&query->m_xLo)[axis];
                if ((&query->m_xHi)[axis] <= hiSplit) {
                    if (queryLo >= loSplit) {
                        return;
                    }
                    continue;
                }
                const int32_t offset = PC[-1];
                PC += offset;
                if (queryLo < loSplit) {
                    queryAabbOnTree(query, PC - offset, chunkOffset);
                }
                continue;
            }
            case 19:
                lo = query->m_yLo + query->m_zLo;
                hi = query->m_yHi + query->m_zHi;
                loSplit = 2 * PC[1];
                hiSplit = 2 * PC[2];
                break;
            case 20:
                lo = query->m_yLo - query->m_zHi;
                hi = query->m_yHi - query->m_zLo;
                loSplit = 2 * PC[1] - 255;
                hiSplit = 2 * PC[2] - 255;
                break;
            case 21:
                lo = query->m_xLo + query->m_zLo;
                hi = query->m_xHi + query->m_zHi;
                loSplit = 2 * PC[1];
                hiSplit = 2 * PC[2];
                break;
            case 22:
                lo = query->m_xLo - query->m_zHi;
                hi = query->m_xHi - query->m_zLo;
                loSplit = 2 * PC[1] - 255;
                hiSplit = 2 * PC[2] - 255;
                break;
            case 23:
                lo = query->m_xLo + query->m_yLo;
                hi = query->m_xHi + query->m_yHi;
                loSplit = 2 * PC[1];
                hiSplit = 2 * PC[2];
                break;
            case 24:
                lo = query->m_xLo - query->m_yHi;
                hi = query->m_xHi - query->m_yLo;
                loSplit = 2 * PC[1] - 255;
                hiSplit = 2 * PC[2] - 255;
                break;
            case 25:
                lo = query->m_xLo + query->m_yLo + query->m_zLo;
                hi = query->m_xHi + query->m_yHi + query->m_zHi;
                loSplit = 3 * PC[1];
                hiSplit = 3 * PC[2];
                break;
            case 26:
                lo = query->m_xLo + query->m_yLo - query->m_zHi;
                hi = query->m_xHi + query->m_yHi - query->m_zLo;
                loSplit = 3 * (PC[1] - 85);
                hiSplit = 3 * (PC[2] - 85);
                break;
            case 27:
                lo = query->m_xLo - query->m_yHi + query->m_zLo;
                hi = query->m_xHi - query->m_yLo + query->m_zHi;
                loSplit = 3 * (PC[1] - 85);
                hiSplit = 3 * (PC[2] - 85);
                break;
            case 28:
                lo = query->m_xLo - query->m_zHi - query->m_yHi;
                hi = query->m_xHi - query->m_zLo - query->m_yLo;
                loSplit = 3 * (PC[1] - 170);
                hiSplit = 3 * (PC[2] - 170);
                break;
            case 32:
            case 33:
            case 34: {
                // A single plane along one axis: the left child follows, the right one an 8-bit offset away.
                const int32_t axis = command - 32;
                const int32_t split = PC[1];
                PC += 3;
                if ((&query->m_xHi)[axis] <= split) {
                    continue;
                }
                const int32_t offset = PC[-1];
                PC += offset;
                if ((&query->m_xLo)[axis] > split) {
                    continue;
                }
                queryAabbOnTree(query, PC - offset, chunkOffset);
                continue;
            }
            case 35:
            case 36:
            case 37: {
                // A split along one axis with 16-bit offsets to both children.
                const int32_t axis = command - 35;
                loSplit = PC[1];
                const int32_t queryLo = (&query->m_xLo)[axis];
                hiSplit = PC[2];
                const int32_t leftOffset = (PC[3] << 8) + PC[4];
                PC += 7;
                if ((&query->m_xHi)[axis] <= hiSplit) {
                    if (queryLo >= loSplit) {
                        return;
                    }
                    PC += leftOffset;
                    continue;
                }
                const int32_t rightOffset = (PC[-2] << 8) + PC[-1];
                PC += rightOffset;
                if (queryLo < loSplit) {
                    queryAabbOnTree(query, PC + (leftOffset - rightOffset), chunkOffset);
                }
                continue;
            }
            case 38:
            case 39:
            case 40: {
                // Cuts the box against the node's bounds along one axis.
                const int32_t axis = command - 38;
                if ((&query->m_xHi)[axis] < PC[1] || (&query->m_xLo)[axis] >= PC[2]) {
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
            case 79: terminal = command - 48; goto addTerminal;
            case 80: terminal = PC[1]; goto addTerminal;
            case 81: terminal = (PC[1] << 8) + PC[2]; goto addTerminal;
            case 82: terminal = (((PC[1] << 8) + PC[2]) << 8) + PC[3]; goto addTerminal;
            case 83:
                terminal = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
            addTerminal:
                {
                    hkpMoppPrimitiveInfo info;
                    info.ID = static_cast<uint32_t>(((chunkOffset >> 9) << 8) & m_reindexingMask.m_storage) |
                        (query->m_primitiveOffset + terminal);
                    m_primitives_out->pushBack(info);
                    return;
                }
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
                if (query != &scaledQuery) {
                    scaledQuery = *query;
                    query = &scaledQuery;
                }
                if (property == 0) {
                    scaledQuery.m_properties[0] = propertyValue;
                }
                continue;
            case 112:
                chunkOffset = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
                PC = &m_code.m_storage->m_data.m_data[chunkOffset];
                continue;
            default: {
                // HK_ERROR; the same command is then read again, as the game does.
                alignas(16) char buf[512];
                hkErrStream kStream(buf, 512);
                kStream.operator<<(
                    "Unknown command - This mopp data has been corrupted (check for memory trashing), or an "
                    "hkpMoppBvTreeShape has been pointed at invalid mopp data.\n"
                );
                if (hkError::s_instance->message(
                        hkError::MESSAGE_ERROR, 0x1298FEDD, buf,
                        ".\\Collide\\Mopp\\Machine\\hkpMoppObbVirtualMachine.cpp", 459
                    )) {
                    __builtin_debugtrap();
                }
                continue;
            }
        }
        // The two-axis and three-axis splits (19 to 28): the right child is an 8-bit offset away.
        PC += 4;
        if (hi > hiSplit) {
            const int32_t offset = PC[-1];
            if (lo < loSplit) {
                queryAabbOnTree(query, PC, chunkOffset);
            }
            PC += offset;
        }
        else if (lo >= loSplit) {
            return;
        }
    }
}

// Collects the primitives the box (extent around BvToWorld, grown by radius) overlaps: the box's world AABB in the
// code's integer space, then the walk from the root. query is handed to queryAabbOnTree, which copies it whole on a
// terminal offset or property command, and nothing writes its paddings or its tail.
void hkpMoppObbVirtualMachine::queryObb(
    const hkpMoppCode* code,
    const hkTransform& BvToWorld,
    const hkVector4& extent,
    float radius,
    hkArray<hkpMoppPrimitiveInfo, hkContainerHeapAllocator>* primitives_out
) {
    hkpMoppObbVirtualMachineQuery query;
    m_primitives_out = primitives_out;
    m_code.m_storage = code;
    const float scale = code->m_info.m_offset.m_quad[3];
    const __m128 e = extent.m_quad;
    // The box's half extent along the world axes: the absolute rotated extent, plus the radius.
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
    const __m128 r = _mm_set_ss(radius);
    halfExtent = _mm_add_ps(halfExtent, _mm_shuffle_ps(r, r, 0));
    const __m128 center = _mm_sub_ps(BvToWorld.m_translation.m_quad, code->m_info.m_offset.m_quad);
    const __m128 s = _mm_set_ss(scale);
    const __m128 scale4 = _mm_shuffle_ps(s, s, 0);
    const __m128 hi = _mm_mul_ps(_mm_add_ps(center, halfExtent), scale4);
    const __m128 lo = _mm_mul_ps(scale4, _mm_add_ps(_mm_xor_ps(halfExtent, hkQuadRealNegateMask), center));
    m_xLo = hkMath::hkToIntFast(lo[0]) - 1;
    m_xHi = hkMath::hkToIntFast(hi[0]) + 1;
    m_yLo = hkMath::hkToIntFast(lo[1]) - 1;
    m_yHi = hkMath::hkToIntFast(hi[1]) + 1;
    m_zLo = hkMath::hkToIntFast(lo[2]) - 1;
    const int32_t zHi = hkMath::hkToIntFast(hi[2]) + 1;
    m_zHi = zHi;
    // The root's space: the high 16 bits.
    query.m_xLo = m_xLo >> 16;
    query.m_xHi = (m_xHi >> 16) + 1;
    query.m_yLo = m_yLo >> 16;
    query.m_yHi = (m_yHi >> 16) + 1;
    query.m_zHi = (zHi >> 16) + 1;
    query.m_zLo = m_zLo >> 16;
    query.m_offset_x = 0;
    query.m_offset_y = 0;
    query.m_offset_z = 0;
    query.m_primitiveOffset = 0;
    query.m_shift = 0;
    query.m_properties[0] = 0;
    const uint8_t* codeData = m_code.m_storage->m_data.m_data;
    // Code that starts with command 13 is chunked: the terminal ids then carry the chunk index.
    m_reindexingMask.m_storage = (*codeData != 13) - 1;
    queryAabbOnTree(&query, codeData, 0);
}

// Collects the primitives the AABB overlaps: the box in the code's integer space, then the walk from the root.
// KEEP (partcopy:): query is handed to queryAabbOnTree, which copies it whole (rep movsd, 0x40 bytes) on a terminal
// offset or property command, and nothing writes its paddings (+0xC to +0xF, +0x1C to +0x1F) or its tail (+0x38 to
// +0x3F).
void hkpMoppObbVirtualMachine::queryAabb(
    const hkpMoppCode* code,
    const hkAabb& aabb,
    hkArray<hkpMoppPrimitiveInfo, hkContainerHeapAllocator>* primitives_out
) {
    hkpMoppObbVirtualMachineQuery query;
    m_primitives_out = primitives_out;
    m_code.m_storage = code;
    const float scale = code->m_info.m_offset.m_quad[3];
    const float offsetX = code->m_info.m_offset.m_quad[0];
    m_xLo = hkMath::hkToIntFast((aabb.m_min.m_quad[0] - offsetX) * scale) - 1;
    m_xHi = hkMath::hkToIntFast((aabb.m_max.m_quad[0] - offsetX) * scale) + 1;
    const float offsetY = code->m_info.m_offset.m_quad[1];
    m_yLo = hkMath::hkToIntFast((aabb.m_min.m_quad[1] - offsetY) * scale) - 1;
    m_yHi = hkMath::hkToIntFast((aabb.m_max.m_quad[1] - offsetY) * scale) + 1;
    const float offsetZ = code->m_info.m_offset.m_quad[2];
    m_zLo = hkMath::hkToIntFast((aabb.m_min.m_quad[2] - offsetZ) * scale) - 1;
    const int32_t zHi = hkMath::hkToIntFast((aabb.m_max.m_quad[2] - offsetZ) * scale) + 1;
    // The root's space: the high 16 bits.
    query.m_xLo = m_xLo >> 16;
    query.m_xHi = (m_xHi >> 16) + 1;
    query.m_yLo = m_yLo >> 16;
    query.m_yHi = (m_yHi >> 16) + 1;
    query.m_zLo = m_zLo >> 16;
    m_zHi = zHi;
    query.m_zHi = (zHi >> 16) + 1;
    query.m_offset_x = 0;
    query.m_offset_y = 0;
    query.m_offset_z = 0;
    query.m_primitiveOffset = 0;
    query.m_shift = 0;
    query.m_properties[0] = 0;
    const uint8_t* codeData = code->m_data.m_data;
    // Code that starts with command 13 is chunked: the terminal ids then carry the chunk index.
    m_reindexingMask.m_storage = (*codeData != 13) - 1;
    queryAabbOnTree(&query, codeData, 0);
}
