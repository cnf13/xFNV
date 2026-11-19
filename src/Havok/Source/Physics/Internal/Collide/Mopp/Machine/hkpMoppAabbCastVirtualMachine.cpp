#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppAabbCastVirtualMachine.h"

#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverCheck.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "xfnv/Compiler.h"

#include <xmmintrin.h>

// Member-wise copy: the two vectors whole, then the scalars in member order; the padding after m_properties is not
// copied.
hkpMoppAabbCastVirtualMachineQueryInt&
hkpMoppAabbCastVirtualMachineQueryInt::operator=(const hkpMoppAabbCastVirtualMachineQueryInt& __that) {
    m_FtoBoffset.m_quad = __that.m_FtoBoffset.m_quad;
    m_extents.m_quad = __that.m_extents.m_quad;
    m_extentsSum3 = __that.m_extentsSum3;
    m_shift = __that.m_shift;
    m_FtoBScale = __that.m_FtoBScale;
    m_primitiveOffset = __that.m_primitiveOffset;
    m_properties[0] = __that.m_properties[0];
    return *this;
}

// aabbCast is a protected member of the class.
void hkpMoppAabbCastVirtualMachine::aabbCast(
    const hkpMoppAabbCastVirtualMachine::hkpAabbCastInput& input,
    hkpCdPointCollector& castCollector,
    hkpCdPointCollector* startCollector
) {
    hkpMoppAabbCastVirtualMachineQueryFloat aQueryFloat;
    hkpMoppAabbCastVirtualMachineQueryInt aScaledQuery;
    if (hkpMoppAabbCastVirtualMachine_HK_flyingcolors_mopp ||
        (hkpProcessFlyingColors(&hkpMoppAabbCastVirtualMachine_HK_flyingcolors_mopp),
         hkpMoppAabbCastVirtualMachine_HK_flyingcolors_mopp)) {
        m_input = &input;
        // The MOPP body's shape is the MOPP tree shape.
        const hkpMoppBvTreeShape* pMoppShape = static_cast<const hkpMoppBvTreeShape*>(input.m_moppBody->m_shape);
        // EXE-INLINED none: hkpMoppBvTreeShape::getMoppCode
        m_code = pMoppShape->m_code;
        m_startPointCollector = startCollector;
        m_castCollector = &castCollector;
        const hkpShapeType eCastType = static_cast<hkpShapeType>(input.m_castBody->m_shape->m_type.m_storage);
        m_earlyOutFraction = 1.0f;
        const hkpMoppCode* pCode = m_code;
        m_refEarlyOutFraction = 1.0f;
        m_castObjectType = eCastType;
        uint8_t* pCodeData = pCode->m_data.m_data;
        aScaledQuery.m_FtoBoffset.m_quad = _mm_setzero_ps();
        m_ItoFScale = 1.0 / XFNV_M128_F32(pCode->m_info.m_offset.m_quad, 3);
        aScaledQuery.m_FtoBScale = XFNV_M128_F32(pCode->m_info.m_offset.m_quad, 3) * 1.52587890625e-05;
        // The scale in lane 0 of a zeroed register, then broadcast.
        __m128 scaleLow = _mm_setzero_ps();
        XFNV_M128_F32(scaleLow, 0) = aScaledQuery.m_FtoBScale;
        const __m128 extents = _mm_mul_ps(_mm_shuffle_ps(scaleLow, scaleLow, 0), input.m_extents.m_quad);
        aScaledQuery.m_extents.m_quad = extents;
        // The extents are summed as z + (y + x) in single precision and stored as a float, then scaled by 3.
        const __m128 extentsSum = _mm_add_ss(
            _mm_shuffle_ps(extents, extents, 0xAA), _mm_add_ss(_mm_shuffle_ps(extents, extents, 0x55), extents)
        );
        const float fExtentsSum = _mm_cvtss_f32(extentsSum);
        aScaledQuery.m_extentsSum3 = fExtentsSum * 3.0;
        const __m128 codeOffset = pCode->m_info.m_offset.m_quad;
        aScaledQuery.m_primitiveOffset = 0;
        aScaledQuery.m_shift = 0;
        aScaledQuery.m_properties[0] = 0;
        // Code that starts with command 13 is chunked: the terminal ids then carry the chunk index.
        const int32_t iReindexingMask = (*pCodeData != 13) - 1;
        const __m128 scale = _mm_set1_ps(aScaledQuery.m_FtoBScale);
        aQueryFloat.m_rayEnds[0].m_quad = _mm_mul_ps(scale, _mm_sub_ps(input.m_from.m_quad, codeOffset));
        aQueryFloat.m_rayEnds[1].m_quad = _mm_mul_ps(scale, _mm_sub_ps(input.m_to.m_quad, codeOffset));
        m_reindexingMask = iReindexingMask;
        queryRayOnTree(&aScaledQuery, pCodeData, &aQueryFloat, 0);
    }
}

// Walks the MOPP code under the scaled query, narrowing the cast's segment (aQueryFloat, in the code's integer space)
// node by node, and casts the cast body against each child shape the segment reaches. Where a split's two children both
// take part, the near child gets a recursive call with a copy of the segment clipped to it, and the walk goes on with
// the far child; each hit may lower m_earlyOutFraction, which shortens the segment.
// KEEP: a property command for properties 1-3 that takes the copy reads stackQuery.m_properties[0] before anything
// wrote it (0xDA7E9E, before the operator= call) and stores it back after the copy (0xDA7EBF); the recursive calls
// are then given stackQuery, and their copies (operator= 0xDA7180, the rescale) read the uninitialised value.
void hkpMoppAabbCastVirtualMachine::queryRayOnTree(
    const hkpMoppAabbCastVirtualMachineQueryInt* query,
    const uint8_t* PC,
    hkpMoppAabbCastVirtualMachineQueryFloat* fQuery,
    int32_t chunkOffset
) {
    hkpMoppAabbCastVirtualMachineQueryInt stackQuery;
    hkpMoppAabbCastVirtualMachineQueryFloat leftFloat;
    for (;;) {
        const int32_t iCommand = *PC;
        // The axis of an axis-aligned split; the diagonal splits keep 999, which skips the early-out test.
        int32_t iAxis = 999;
        // A split node: the right child's lower plane, the left child's upper plane, the segment's ends projected on
        // the split's direction, and where each child's code starts after the node.
        float fRightLow;
        float fLeftHigh;
        float fStart;
        float fEnd;
        int32_t iLeftOffset;
        int32_t iRightOffset;
        // A double cut: the slab the segment is clipped to (its ends projected in fStart and fEnd).
        float fLow;
        float fHigh;
        // A terminal's number; a property's index and value.
        uint32_t uiId;
        int32_t iProperty;
        uint32_t uiValue;
        switch (iCommand) {
            case HK_MOPP_RETURN: return;
            case HK_MOPP_SCALE1:
            case HK_MOPP_SCALE2:
            case HK_MOPP_SCALE3:
            case HK_MOPP_SCALE4: {
                const float fX = PC[1];
                const float fY = PC[2];
                const float fZ = PC[3];
                const float fScale = static_cast<float>(1 << iCommand);
                const __m128 offset = _mm_set_ps(0.0f, fZ, fY, fX);
                const __m128 scale = _mm_set1_ps(fScale);
                fQuery->m_rayEnds[0].m_quad = _mm_sub_ps(fQuery->m_rayEnds[0].m_quad, offset);
                fQuery->m_rayEnds[1].m_quad = _mm_sub_ps(fQuery->m_rayEnds[1].m_quad, offset);
                fQuery->m_rayEnds[0].m_quad = _mm_mul_ps(scale, fQuery->m_rayEnds[0].m_quad);
                fQuery->m_rayEnds[1].m_quad = _mm_mul_ps(scale, fQuery->m_rayEnds[1].m_quad);
                stackQuery.m_FtoBoffset.m_quad = _mm_mul_ps(_mm_add_ps(offset, query->m_FtoBoffset.m_quad), scale);
                stackQuery.m_shift = query->m_shift + iCommand;
                stackQuery.m_FtoBScale = query->m_FtoBScale * fScale;
                stackQuery.m_extents.m_quad = _mm_mul_ps(query->m_extents.m_quad, scale);
                stackQuery.m_extentsSum3 = fScale * query->m_extentsSum3;
                stackQuery.m_properties[0] = query->m_properties[0];
                stackQuery.m_primitiveOffset = query->m_primitiveOffset;
                query = &stackQuery;
                PC += 4;
                continue;
            }
            case HK_MOPP_JUMP8: PC += PC[1] + 2; continue;
            case HK_MOPP_JUMP16: PC += (PC[1] << 8) + PC[2] + 3; continue;
            case HK_MOPP_JUMP24: PC += (((PC[1] << 8) + PC[2]) << 8) + PC[3] + 4; continue;
            case HK_MOPP_TERM_REOFFSET8: {
                const uint32_t uiOffset = PC[1];
                if (query != &stackQuery) {
                    stackQuery = *query;
                    query = &stackQuery;
                }
                stackQuery.m_primitiveOffset += uiOffset;
                PC += 2;
                continue;
            }
            case HK_MOPP_TERM_REOFFSET16: {
                const uint32_t uiOffset = (PC[1] << 8) + PC[2];
                if (query != &stackQuery) {
                    stackQuery = *query;
                    query = &stackQuery;
                }
                stackQuery.m_primitiveOffset += uiOffset;
                PC += 3;
                continue;
            }
            case HK_MOPP_TERM_REOFFSET32: {
                const uint32_t uiOffset = (((((static_cast<uint32_t>(PC[1]) << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
                if (query != &stackQuery) {
                    stackQuery = *query;
                    query = &stackQuery;
                }
                stackQuery.m_primitiveOffset = uiOffset;
                PC += 5;
                continue;
            }
            case HK_MOPP_JUMP_CHUNK:
                chunkOffset = ((PC[1] << 8) | PC[2]) << 9;
                PC = m_code->m_data.m_data + chunkOffset;
                continue;
            case HK_MOPP_DATA_OFFSET: PC += 5; continue;
            case HK_MOPP_SPLIT_X:
            case HK_MOPP_SPLIT_Y:
            case HK_MOPP_SPLIT_Z:
                iAxis = iCommand - HK_MOPP_SPLIT_X;
                fRightLow = PC[2] - query->m_extents(iAxis);
                fLeftHigh = PC[1] + query->m_extents(iAxis);
                fStart = fQuery->m_rayEnds[0](iAxis);
                fEnd = fQuery->m_rayEnds[1](iAxis);
                break;
            case HK_MOPP_SPLIT_YZ: {
                const double dSum = static_cast<double>(query->m_extents(2)) + query->m_extents(1);
                const float fExtent = dSum + dSum;
                fRightLow = 2 * PC[2] - fExtent;
                fLeftHigh = fExtent + 2 * PC[1];
                fStart = fQuery->m_rayEnds[0](2) + fQuery->m_rayEnds[0](1);
                fEnd = fQuery->m_rayEnds[1](2) + fQuery->m_rayEnds[1](1);
                break;
            }
            case HK_MOPP_SPLIT_YMZ: {
                const double dSum = static_cast<double>(query->m_extents(2)) + query->m_extents(1);
                const float fExtent = dSum + dSum;
                fRightLow = 2 * PC[2] - 255 - fExtent;
                fLeftHigh = fExtent + (2 * PC[1] - 255);
                fStart = fQuery->m_rayEnds[0](1) - fQuery->m_rayEnds[0](2);
                fEnd = fQuery->m_rayEnds[1](1) - fQuery->m_rayEnds[1](2);
                break;
            }
            case HK_MOPP_SPLIT_XZ: {
                const double dSum = static_cast<double>(query->m_extents(2)) + query->m_extents(0);
                const float fExtent = dSum + dSum;
                fRightLow = 2 * PC[2] - fExtent;
                fLeftHigh = fExtent + 2 * PC[1];
                fStart = fQuery->m_rayEnds[0](2) + fQuery->m_rayEnds[0](0);
                fEnd = fQuery->m_rayEnds[1](2) + fQuery->m_rayEnds[1](0);
                break;
            }
            case HK_MOPP_SPLIT_XMZ: {
                const double dSum = static_cast<double>(query->m_extents(2)) + query->m_extents(0);
                const float fExtent = dSum + dSum;
                fRightLow = 2 * PC[2] - 255 - fExtent;
                fLeftHigh = fExtent + (2 * PC[1] - 255);
                fStart = fQuery->m_rayEnds[0](0) - fQuery->m_rayEnds[0](2);
                fEnd = fQuery->m_rayEnds[1](0) - fQuery->m_rayEnds[1](2);
                break;
            }
            case HK_MOPP_SPLIT_XY: {
                const double dSum = static_cast<double>(query->m_extents(1)) + query->m_extents(0);
                const float fExtent = dSum + dSum;
                fRightLow = 2 * PC[2] - fExtent;
                fLeftHigh = fExtent + 2 * PC[1];
                fStart = fQuery->m_rayEnds[0](1) + fQuery->m_rayEnds[0](0);
                fEnd = fQuery->m_rayEnds[1](1) + fQuery->m_rayEnds[1](0);
                break;
            }
            case HK_MOPP_SPLIT_XMY: {
                const double dSum = static_cast<double>(query->m_extents(1)) + query->m_extents(0);
                const float fExtent = dSum + dSum;
                fRightLow = 2 * PC[2] - 255 - fExtent;
                fLeftHigh = fExtent + (2 * PC[1] - 255);
                fStart = fQuery->m_rayEnds[0](0) - fQuery->m_rayEnds[0](1);
                fEnd = fQuery->m_rayEnds[1](0) - fQuery->m_rayEnds[1](1);
                break;
            }
            case HK_MOPP_SPLIT_XYZ: {
                const float fExtent = query->m_extentsSum3;
                fRightLow = 3 * PC[2] - fExtent;
                fLeftHigh = fExtent + 3 * PC[1];
                fStart = fQuery->m_rayEnds[0](1) + fQuery->m_rayEnds[0](0) + fQuery->m_rayEnds[0](2);
                fEnd = fQuery->m_rayEnds[1](1) + fQuery->m_rayEnds[1](0) + fQuery->m_rayEnds[1](2);
                break;
            }
            case HK_MOPP_SPLIT_XYMZ: {
                const float fExtent = query->m_extentsSum3;
                fRightLow = 3 * (PC[2] - 85) - fExtent;
                fLeftHigh = fExtent + 3 * (PC[1] - 85);
                fStart = fQuery->m_rayEnds[0](1) + fQuery->m_rayEnds[0](0) - fQuery->m_rayEnds[0](2);
                fEnd = fQuery->m_rayEnds[1](1) + fQuery->m_rayEnds[1](0) - fQuery->m_rayEnds[1](2);
                break;
            }
            case HK_MOPP_SPLIT_XMYZ: {
                const float fExtent = query->m_extentsSum3;
                fRightLow = 3 * (PC[2] - 85) - fExtent;
                fLeftHigh = fExtent + 3 * (PC[1] - 85);
                fStart = fQuery->m_rayEnds[0](0) - fQuery->m_rayEnds[0](1) + fQuery->m_rayEnds[0](2);
                fEnd = fQuery->m_rayEnds[1](0) - fQuery->m_rayEnds[1](1) + fQuery->m_rayEnds[1](2);
                break;
            }
            case HK_MOPP_SPLIT_XMYMZ: {
                const float fExtent = query->m_extentsSum3;
                fRightLow = 3 * (PC[2] - 170) - fExtent;
                fLeftHigh = fExtent + 3 * (PC[1] - 170);
                fStart = fQuery->m_rayEnds[0](0) - fQuery->m_rayEnds[0](1) - fQuery->m_rayEnds[0](2);
                fEnd = fQuery->m_rayEnds[1](0) - fQuery->m_rayEnds[1](1) - fQuery->m_rayEnds[1](2);
                break;
            }
            case HK_MOPP_SINGLE_SPLIT_X:
            case HK_MOPP_SINGLE_SPLIT_Y:
            case HK_MOPP_SINGLE_SPLIT_Z: {
                // One plane: the left child ends one unit above it.
                const float fCut = PC[1];
                iAxis = iCommand - HK_MOPP_SINGLE_SPLIT_X;
                iRightOffset = PC[2];
                PC += 3;
                const float fCutAbove = fCut + 1.0;
                fRightLow = fCut - query->m_extents(iAxis);
                fLeftHigh = query->m_extents(iAxis) + fCutAbove;
                fStart = fQuery->m_rayEnds[0](iAxis);
                fEnd = fQuery->m_rayEnds[1](iAxis);
                iLeftOffset = 0;
                goto splitNode;
            }
            case HK_MOPP_SPLIT_JUMP_X:
            case HK_MOPP_SPLIT_JUMP_Y:
            case HK_MOPP_SPLIT_JUMP_Z:
                iAxis = iCommand - HK_MOPP_SPLIT_JUMP_X;
                fRightLow = PC[2] - query->m_extents(iAxis);
                fLeftHigh = PC[1] + query->m_extents(iAxis);
                fStart = fQuery->m_rayEnds[0](iAxis);
                fEnd = fQuery->m_rayEnds[1](iAxis);
                iLeftOffset = (PC[3] << 8) + PC[4];
                iRightOffset = (PC[5] << 8) + PC[6];
                PC += 7;
                goto splitNode;
            case HK_MOPP_DOUBLE_CUT_X:
            case HK_MOPP_DOUBLE_CUT_Y:
            case HK_MOPP_DOUBLE_CUT_Z: {
                const int32_t iCutAxis = iCommand - HK_MOPP_DOUBLE_CUT_X;
                fLow = PC[1] - query->m_extents(iCutAxis);
                fHigh = PC[2] + query->m_extents(iCutAxis);
                PC += 3;
                fStart = fQuery->m_rayEnds[0](iCutAxis);
                fEnd = fQuery->m_rayEnds[1](iCutAxis);
                goto doubleCut;
            }
            case HK_MOPP_DOUBLE_CUT24_X:
            case HK_MOPP_DOUBLE_CUT24_Y:
            case HK_MOPP_DOUBLE_CUT24_Z: {
                // The planes are in the code's float space: scaled to this query's integer space, each step rounded to
                // float.
                const int32_t iCutAxis = iCommand - HK_MOPP_DOUBLE_CUT24_X;
                const int32_t iLow = (((PC[1] << 8) + PC[2]) << 8) + PC[3];
                const int32_t iHigh = (((PC[4] << 8) + PC[5]) << 8) + PC[6];
                PC += 7;
                const float fLowF = iLow * m_ItoFScale;
                const float fLowScaled = fLowF * query->m_FtoBScale;
                const float fLowInt = fLowScaled - query->m_FtoBoffset(iCutAxis);
                fLow = fLowInt - query->m_extents(iCutAxis);
                const float fHighF = iHigh * m_ItoFScale;
                const float fHighScaled = fHighF * query->m_FtoBScale;
                const float fHighInt = fHighScaled - query->m_FtoBoffset(iCutAxis);
                fHigh = fHighInt + query->m_extents(iCutAxis);
                fStart = fQuery->m_rayEnds[0](iCutAxis);
                fEnd = fQuery->m_rayEnds[1](iCutAxis);
                goto doubleCut;
            }
            case HK_MOPP_TERM4_0:
            case HK_MOPP_TERM4_1:
            case HK_MOPP_TERM4_2:
            case HK_MOPP_TERM4_3:
            case HK_MOPP_TERM4_4:
            case HK_MOPP_TERM4_5:
            case HK_MOPP_TERM4_6:
            case HK_MOPP_TERM4_7:
            case HK_MOPP_TERM4_8:
            case HK_MOPP_TERM4_9:
            case HK_MOPP_TERM4_A:
            case HK_MOPP_TERM4_B:
            case HK_MOPP_TERM4_C:
            case HK_MOPP_TERM4_D:
            case HK_MOPP_TERM4_E:
            case HK_MOPP_TERM4_F:
            case HK_MOPP_TERM4_10:
            case HK_MOPP_TERM4_11:
            case HK_MOPP_TERM4_12:
            case HK_MOPP_TERM4_13:
            case HK_MOPP_TERM4_14:
            case HK_MOPP_TERM4_15:
            case HK_MOPP_TERM4_16:
            case HK_MOPP_TERM4_17:
            case HK_MOPP_TERM4_18:
            case HK_MOPP_TERM4_19:
            case HK_MOPP_TERM4_1A:
            case HK_MOPP_TERM4_1B:
            case HK_MOPP_TERM4_1C:
            case HK_MOPP_TERM4_1D:
            case HK_MOPP_TERM4_1E:
            case HK_MOPP_TERM4_1F: uiId = iCommand - HK_MOPP_TERM4_0; goto addHit;
            case HK_MOPP_TERM8: uiId = PC[1]; goto addHit;
            case HK_MOPP_TERM16: uiId = (PC[1] << 8) + PC[2]; goto addHit;
            case HK_MOPP_TERM24: uiId = (((PC[1] << 8) + PC[2]) << 8) + PC[3]; goto addHit;
            case HK_MOPP_TERM32:
                uiId = (((static_cast<uint32_t>(PC[1]) << 8) + PC[2]) << 16) + (PC[3] << 8) + PC[4];
                goto addHit;
            case HK_MOPP_PROPERTY8_0:
            case HK_MOPP_PROPERTY8_1:
            case HK_MOPP_PROPERTY8_2:
            case HK_MOPP_PROPERTY8_3:
                iProperty = iCommand - HK_MOPP_PROPERTY8_0;
                uiValue = PC[1];
                PC += 2;
                goto setProperty;
            case HK_MOPP_PROPERTY16_0:
            case HK_MOPP_PROPERTY16_1:
            case HK_MOPP_PROPERTY16_2:
            case HK_MOPP_PROPERTY16_3:
                iProperty = iCommand - HK_MOPP_PROPERTY16_0;
                uiValue = (PC[1] << 8) + PC[2];
                PC += 3;
                goto setProperty;
            case HK_MOPP_PROPERTY32_0:
            case HK_MOPP_PROPERTY32_1:
            case HK_MOPP_PROPERTY32_2:
            case HK_MOPP_PROPERTY32_3:
                iProperty = iCommand - HK_MOPP_PROPERTY32_0;
                uiValue = (((((static_cast<uint32_t>(PC[1]) << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
                PC += 5;
                goto setProperty;
            case HK_MOPP_JUMP_CHUNK32:
                chunkOffset =
                    static_cast<int32_t>((((((static_cast<uint32_t>(PC[1]) << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4]);
                PC = m_code->m_data.m_data + chunkOffset;
                continue;
            default: {
                alignas(16) char buf[512];
                hkErrStream errStream(buf, 512);
                errStream << "Unknown command - This mopp data has been corrupted (check for memory trashing), or an "
                             "hkpMoppBvTreeShape has been pointed at invalid mopp data.\n";
                if (hkError::s_instance->message(
                        hkError::MESSAGE_ERROR, 0x1298fedd, buf,
                        ".\\Collide\\Mopp\\Machine\\hkpMoppAabbCastVirtualMachine.cpp", 521
                    )) {
                    xfnv::DebugTrap();
                }
                continue;
            }
        }
        // The splits with a 4-byte node: the right child after the node's last byte, the left child right after it.
        iLeftOffset = 0;
        iRightOffset = PC[3];
        PC += 4;
    splitNode:
        {
            const uint8_t* pChildren = PC;
            // Both ends below the right child: the left child alone.
            if (fRightLow > fEnd && fStart < fRightLow) {
                PC = pChildren + iLeftOffset;
                continue;
            }
            PC = pChildren + iRightOffset;
            // Both ends above the left child: the right child alone.
            if (fLeftHigh < fStart && fLeftHigh < fEnd) {
                continue;
            }
            const hkVector4 rayStart = fQuery->m_rayEnds[0];
            const hkVector4 rayEnd = fQuery->m_rayEnds[1];
            leftFloat.m_rayEnds[0].m_quad = rayStart.m_quad;
            leftFloat.m_rayEnds[1].m_quad = rayEnd.m_quad;
            const float fStartAboveLeft = fStart - fLeftHigh;
            const float fStartAboveRight = fStart - fRightLow;
            const float fEndAboveLeft = fEnd - fLeftHigh;
            const float fEndAboveRight = fEnd - fRightLow;
            if (fEndAboveLeft > fStartAboveLeft) {
                // The segment runs up the split: the left child first, with the part below its upper plane; then the
                // right child, with the part above its lower plane.
                if (fEndAboveLeft * fStartAboveLeft < 0.0) {
                    const float t = fStartAboveLeft / (fStartAboveLeft - fEndAboveLeft);
                    leftFloat.m_rayEnds[1].setInterpolate4(rayStart, rayEnd, t);
                }
                queryRayOnTree(query, pChildren + iLeftOffset, &leftFloat, chunkOffset);
                if (fEndAboveRight * fStartAboveRight < 0.0) {
                    const float t = fStartAboveRight / (fStartAboveRight - fEndAboveRight);
                    fQuery->m_rayEnds[0].setInterpolate4(fQuery->m_rayEnds[0], fQuery->m_rayEnds[1], t);
                }
                // A hit in the left child lowered the early-out fraction: the segment now ends there.
                if (m_refEarlyOutFraction > m_earlyOutFraction) {
                    m_refEarlyOutFraction = m_earlyOutFraction;
                    hkVector4 earlyOutEnd;
                    earlyOutEnd.setInterpolate4(m_input->m_from, m_input->m_to, m_earlyOutFraction);
                    fQuery->m_rayEnds[1].m_quad = _mm_sub_ps(earlyOutEnd.m_quad, m_code->m_info.m_offset.m_quad);
                    fQuery->m_rayEnds[1].m_quad =
                        _mm_mul_ps(_mm_set1_ps(query->m_FtoBScale), fQuery->m_rayEnds[1].m_quad);
                    fQuery->m_rayEnds[1].m_quad = _mm_sub_ps(fQuery->m_rayEnds[1].m_quad, query->m_FtoBoffset.m_quad);
                    if (iAxis < 3 && fRightLow > fQuery->m_rayEnds[1](iAxis)) {
                        return;
                    }
                }
                continue;
            }
            // The segment runs down the split: the right child first, with the part above its lower plane; then the
            // left child, with the part below its upper plane.
            if (fEndAboveRight * fStartAboveRight < 0.0) {
                const float t = fStartAboveRight / (fStartAboveRight - fEndAboveRight);
                leftFloat.m_rayEnds[1].setInterpolate4(rayStart, rayEnd, t);
            }
            queryRayOnTree(query, PC, &leftFloat, chunkOffset);
            if (fEndAboveLeft * fStartAboveLeft < 0.0) {
                const float t = fStartAboveLeft / (fStartAboveLeft - fEndAboveLeft);
                fQuery->m_rayEnds[0].setInterpolate4(fQuery->m_rayEnds[0], fQuery->m_rayEnds[1], t);
            }
            if (m_refEarlyOutFraction > m_earlyOutFraction) {
                m_refEarlyOutFraction = m_earlyOutFraction;
                hkVector4 earlyOutEnd;
                earlyOutEnd.setInterpolate4(m_input->m_from, m_input->m_to, m_earlyOutFraction);
                fQuery->m_rayEnds[1].m_quad = _mm_sub_ps(earlyOutEnd.m_quad, m_code->m_info.m_offset.m_quad);
                fQuery->m_rayEnds[1].m_quad = _mm_mul_ps(_mm_set1_ps(query->m_FtoBScale), fQuery->m_rayEnds[1].m_quad);
                fQuery->m_rayEnds[1].m_quad = _mm_sub_ps(fQuery->m_rayEnds[1].m_quad, query->m_FtoBoffset.m_quad);
                if (iAxis < 3 && fLeftHigh < fQuery->m_rayEnds[1](iAxis)) {
                    return;
                }
            }
            PC = pChildren + iLeftOffset;
            continue;
        }
    doubleCut:
        {
            // The segment misses the slab: nothing below this node. Else it is clipped to the slab.
            int32_t iDown;
            if (fEnd > fStart) {
                if (fLow > fEnd || fHigh < fStart) {
                    return;
                }
                iDown = 0;
            }
            else {
                if (fLow > fStart || fHigh < fEnd) {
                    return;
                }
                iDown = 1;
            }
            const hkVector4 rayStart = fQuery->m_rayEnds[0];
            const hkVector4 rayEnd = fQuery->m_rayEnds[1];
            const float fStartAboveHigh = fStart - fHigh;
            const float fEndAboveHigh = fEnd - fHigh;
            if (fEndAboveHigh * fStartAboveHigh < 0.0) {
                const float t = fStartAboveHigh / (fStartAboveHigh - fEndAboveHigh);
                fQuery->m_rayEnds[1 - iDown].setInterpolate4(rayStart, rayEnd, t);
            }
            const float fStartAboveLow = fStart - fLow;
            const float fEndAboveLow = fEnd - fLow;
            if (fEndAboveLow * fStartAboveLow < 0.0) {
                const float t = fStartAboveLow / (fStartAboveLow - fEndAboveLow);
                fQuery->m_rayEnds[iDown].setInterpolate4(rayStart, rayEnd, t);
            }
            continue;
        }
    setProperty:
        {
            // m_properties has one element: properties 1-3 would land in the padding after it, which the copy and the
            // callees never read, so only property 0 is written here. For properties 1-3 on a fresh copy, the copied
            // m_properties[0] is kept.
            if (query != &stackQuery) {
                stackQuery = *query;
                query = &stackQuery;
            }
            if (iProperty == 0) {
                stackQuery.m_properties[0] = uiValue;
            }
            continue;
        }
    addHit:
        {
            // EXE-INLINED none: hkpMoppAabbCastVirtualMachine::addHit
            const uint32_t uiKey = (((chunkOffset >> 9) << 8) & m_reindexingMask) | (query->m_primitiveOffset + uiId);
            // EXE-INLINED none: hkpMoppBvTreeShape::getShapeCollection
            const hkpShapeContainer* pContainer = static_cast<const hkpShapeCollection*>(
                static_cast<const hkpMoppBvTreeShape*>(m_input->m_moppBody->m_shape)->m_child.m_childShape
            );
            alignas(16) char shapeBuffer[512];
            const hkpShape* pChildShape = pContainer->getChildShape(uiKey, shapeBuffer);
            const hkpLinearCastCollisionInput* pCollisionInput = m_input->m_collisionInput;
            if (pCollisionInput->m_filter->isCollisionEnabled(
                    *pCollisionInput, *m_input->m_castBody, *m_input->m_moppBody, *pContainer, uiKey
                )) {
                const hkpMoppAabbCastVirtualMachine::hkpAabbCastInput* pInput = m_input;
                // EXE-INLINED none: hkpCdBody::hkpCdBody
                alignas(16) hkpCdBody childBody;
                childBody.m_parent = pInput->m_moppBody;
                childBody.m_motion = pInput->m_moppBody->m_motion;
                // EXE-INLINED none: hkpCdBody::setShape
                childBody.m_shape = pChildShape;
                childBody.m_shapeKey = uiKey;
                const hkpCollisionDispatcher::LinearCastFunc pLinearCast =
                    pInput->m_collisionInput->m_dispatcher->getLinearCastFunc(m_castObjectType, pChildShape->m_type);
                pLinearCast(
                    *pInput->m_castBody, childBody, *pInput->m_collisionInput, *m_castCollector, m_startPointCollector
                );
                m_earlyOutFraction = m_castCollector->m_earlyOutDistance;
            }
            return;
        }
    }
}
