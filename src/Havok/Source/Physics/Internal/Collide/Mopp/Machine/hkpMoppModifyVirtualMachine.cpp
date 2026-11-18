#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppModifyVirtualMachine.h"

#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Solve/hkpSolverCheck.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppModifier.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppObbVirtualMachine.h"

// Walks the MOPP code from PC inside the box and asks m_modifier about each terminal; the result is whether the whole
// subtree is to be removed. Where only one child of a split is to be removed, the modifier gets that child's relative
// address. query holds the box in the current node's integer space; a rescale, a terminal offset or a property command
// moves it into scaledQuery first, which the recursive calls are given. The rescale (commands 1 to 4) writes neither
// scaledQuery's paddings nor its tail. A property command stores property 0 only.
hkBool hkpMoppModifyVirtualMachine::queryModicationPointsRecursive(
    const hkpMoppObbVirtualMachineQuery* query,
    const uint8_t* PC,
    int32_t chunkOffset
) {
    hkpMoppObbVirtualMachineQuery scaledQuery;
    while (true) {
        const int32_t command = *PC;
        // A split node: the query's low and high value along the split's direction, the left child's upper bound
        // and the right child's lower bound, and the offsets of both children from PC.
        int32_t lo;
        int32_t hi;
        int32_t leftMax;
        int32_t rightMin;
        int32_t leftOffset;
        int32_t rightOffset;
        int32_t terminal;
        int32_t property;
        uint32_t propertyValue;
        switch (command) {
            case 0: return m_tempLastShouldTerminalBeRemoved.m_bool;
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
                lo = (&query->m_xLo)[axis];
                hi = (&query->m_xHi)[axis];
                leftMax = PC[1];
                rightMin = PC[2];
                leftOffset = 0;
                rightOffset = PC[3];
                PC += 4;
                break;
            }
            case 19:
                lo = query->m_yLo + query->m_zLo;
                hi = query->m_yHi + query->m_zHi;
                leftMax = 2 * PC[1];
                rightMin = 2 * PC[2];
                leftOffset = 0;
                rightOffset = PC[3];
                PC += 4;
                break;
            case 20:
                lo = query->m_yLo - query->m_zHi;
                hi = query->m_yHi - query->m_zLo;
                leftMax = 2 * PC[1] - 255;
                rightMin = 2 * PC[2] - 255;
                leftOffset = 0;
                rightOffset = PC[3];
                PC += 4;
                break;
            case 21:
                lo = query->m_xLo + query->m_zLo;
                hi = query->m_xHi + query->m_zHi;
                leftMax = 2 * PC[1];
                rightMin = 2 * PC[2];
                leftOffset = 0;
                rightOffset = PC[3];
                PC += 4;
                break;
            case 22:
                lo = query->m_xLo - query->m_zHi;
                hi = query->m_xHi - query->m_zLo;
                leftMax = 2 * PC[1] - 255;
                rightMin = 2 * PC[2] - 255;
                leftOffset = 0;
                rightOffset = PC[3];
                PC += 4;
                break;
            case 23:
                lo = query->m_xLo + query->m_yLo;
                hi = query->m_xHi + query->m_yHi;
                leftMax = 2 * PC[1];
                rightMin = 2 * PC[2];
                leftOffset = 0;
                rightOffset = PC[3];
                PC += 4;
                break;
            case 24:
                lo = query->m_xLo - query->m_yHi;
                hi = query->m_xHi - query->m_yLo;
                leftMax = 2 * PC[1] - 255;
                rightMin = 2 * PC[2] - 255;
                leftOffset = 0;
                rightOffset = PC[3];
                PC += 4;
                break;
            case 25:
                lo = query->m_xLo + (query->m_yLo + query->m_zLo);
                hi = query->m_xHi + (query->m_yHi + query->m_zHi);
                leftMax = 3 * PC[1];
                rightMin = 3 * PC[2];
                leftOffset = 0;
                rightOffset = PC[3];
                PC += 4;
                break;
            case 26:
                lo = query->m_xLo + (query->m_yLo - query->m_zHi);
                hi = query->m_xHi + (query->m_yHi - query->m_zLo);
                leftMax = 3 * (PC[1] - 85);
                rightMin = 3 * (PC[2] - 85);
                leftOffset = 0;
                rightOffset = PC[3];
                PC += 4;
                break;
            case 27:
                lo = query->m_xLo + (query->m_zLo - query->m_yHi);
                hi = query->m_xHi + (query->m_zHi - query->m_yLo);
                leftMax = 3 * (PC[1] - 85);
                rightMin = 3 * (PC[2] - 85);
                leftOffset = 0;
                rightOffset = PC[3];
                PC += 4;
                break;
            case 28:
                lo = query->m_xLo - query->m_zHi - query->m_yHi;
                hi = query->m_xHi - query->m_zLo - query->m_yLo;
                leftMax = 3 * (PC[1] - 170);
                rightMin = 3 * (PC[2] - 170);
                leftOffset = 0;
                rightOffset = PC[3];
                PC += 4;
                break;
            case 32:
            case 33:
            case 34: {
                // A single plane along one axis, the right child an 8-bit offset away.
                const int32_t axis = command - 32;
                lo = (&query->m_xLo)[axis];
                hi = (&query->m_xHi)[axis];
                rightMin = PC[1];
                leftMax = PC[1] + 1;
                leftOffset = 0;
                rightOffset = PC[2];
                PC += 3;
                break;
            }
            case 35:
            case 36:
            case 37: {
                // A split along one axis with 16-bit offsets to both children.
                const int32_t axis = command - 35;
                lo = (&query->m_xLo)[axis];
                hi = (&query->m_xHi)[axis];
                leftMax = PC[1];
                rightMin = PC[2];
                leftOffset = (PC[3] << 8) + PC[4];
                rightOffset = (PC[5] << 8) + PC[6];
                PC += 7;
                break;
            }
            case 38:
            case 39:
            case 40: {
                // Cuts the box against the node's bounds along one axis.
                const int32_t axis = command - 38;
                if ((&query->m_xHi)[axis] < PC[1] || (&query->m_xLo)[axis] >= PC[2]) {
                    return 0;
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
                    return 0;
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
            case 79: terminal = command - 48; goto askTerminal;
            case 80: terminal = PC[1]; goto askTerminal;
            case 81: terminal = (PC[1] << 8) + PC[2]; goto askTerminal;
            case 82: terminal = (((PC[1] << 8) + PC[2]) << 8) + PC[3]; goto askTerminal;
            case 83:
                terminal = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
            askTerminal:
                {
                    const uint32_t id = static_cast<uint32_t>(((chunkOffset >> 9) << 8) & m_reindexingMask.m_storage) |
                        (query->m_primitiveOffset + terminal);
                    m_tempLastShouldTerminalBeRemoved.m_bool =
                        m_modifier->shouldTerminalBeRemoved(id, query->m_properties).m_bool;
                    return m_tempLastShouldTerminalBeRemoved.m_bool;
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
                        ".\\Collide\\Mopp\\Machine\\hkpMoppModifyVirtualMachine.cpp", 293
                    )) {
                    __builtin_debugtrap();
                }
                continue;
            }
        }
        // A split: the children the box reaches are walked, the left one first.
        char removeLeft = 0;
        char removeRight = 0;
        if (hi > rightMin) {
            if (lo < leftMax) {
                removeLeft = queryModicationPointsRecursive(query, PC + leftOffset, chunkOffset).m_bool;
                removeRight = queryModicationPointsRecursive(query, PC + rightOffset, chunkOffset).m_bool;
            }
            else {
                removeRight = queryModicationPointsRecursive(query, PC + rightOffset, chunkOffset).m_bool;
            }
        }
        else {
            if (lo >= leftMax) {
                return 0;
            }
            removeLeft = queryModicationPointsRecursive(query, PC + leftOffset, chunkOffset).m_bool;
        }
        // One child to remove and not the other: the modifier removes that child alone.
        if (removeRight == 0 && removeLeft != 0) {
            m_modifier->addTerminalRemoveInfo(static_cast<int32_t>(PC + leftOffset - m_code.m_storage->m_data.m_data));
        }
        if (removeRight != 0 && removeLeft == 0) {
            m_modifier->addTerminalRemoveInfo(static_cast<int32_t>(PC + rightOffset - m_code.m_storage->m_data.m_data));
        }
        return removeLeft != 0 && removeRight != 0;
    }
}

// With the MOPP machines enabled (hkpMoppModifyVirtualMachine_HK_flyingcolors_mopp, which hkpProcessFlyingColors fills
// on first use), walks modifierOut over the terminals of code inside aabb. Each bound goes into the code's integer
// space as a float and is rounded to nearest, widened by one unit. The query holds the box's top 16 bits (the high
// corner's plus one) and zero offsets, shift and properties; its two padding fields stay unwritten, as the game does.
// When the walk removes the whole tree, the modifier gets the root's relative address: 5 past a reindexing header
// (command 0x0D), else 0.
void hkpMoppModifyVirtualMachine::queryAabb(const hkpMoppCode* code, const hkAabb& aabb, hkpMoppModifier* modifierOut) {
    if (!hkpMoppModifyVirtualMachine_HK_flyingcolors_mopp) {
        hkpProcessFlyingColors(&hkpMoppModifyVirtualMachine_HK_flyingcolors_mopp);
        if (!hkpMoppModifyVirtualMachine_HK_flyingcolors_mopp) {
            return;
        }
    }
    m_code.m_storage = code;
    m_modifier = modifierOut;
    m_primitives_out = nullptr;
    float fScale = code->m_info.m_offset.m_quad[3];
    float fOffset = code->m_info.m_offset.m_quad[0];
    float fScaled;
    int32_t iRounded;
    fScaled = (aabb.m_min.m_quad[0] - fOffset) * fScale;
    __asm__("flds %1\n\tfistpl %0" : "=m"(iRounded) : "m"(fScaled));
    m_xLo = iRounded - 1;
    fScaled = (aabb.m_max.m_quad[0] - fOffset) * fScale;
    __asm__("flds %1\n\tfistpl %0" : "=m"(iRounded) : "m"(fScaled));
    m_xHi = iRounded + 1;
    fOffset = code->m_info.m_offset.m_quad[1];
    fScaled = (aabb.m_min.m_quad[1] - fOffset) * fScale;
    __asm__("flds %1\n\tfistpl %0" : "=m"(iRounded) : "m"(fScaled));
    m_yLo = iRounded - 1;
    fScaled = (aabb.m_max.m_quad[1] - fOffset) * fScale;
    __asm__("flds %1\n\tfistpl %0" : "=m"(iRounded) : "m"(fScaled));
    m_yHi = iRounded + 1;
    fOffset = code->m_info.m_offset.m_quad[2];
    fScaled = (aabb.m_min.m_quad[2] - fOffset) * fScale;
    __asm__("flds %1\n\tfistpl %0" : "=m"(iRounded) : "m"(fScaled));
    m_zLo = iRounded - 1;
    fScaled = (aabb.m_max.m_quad[2] - fOffset) * fScale;
    __asm__("flds %1\n\tfistpl %0" : "=m"(iRounded) : "m"(fScaled));
    int32_t iZHi = iRounded + 1;
    m_zHi = iZHi;
    // The query queryModicationPointsRecursive walks: the high corner, its padding, the low corner, its padding, then
    // the offsets, primitive offset, shift and properties.
    hkpMoppObbVirtualMachineQuery query;
    query.m_xHi = (m_xHi >> 16) + 1;
    query.m_yHi = (m_yHi >> 16) + 1;
    query.m_zHi = (iZHi >> 16) + 1;
    query.m_xLo = m_xLo >> 16;
    query.m_yLo = m_yLo >> 16;
    query.m_zLo = m_zLo >> 16;
    query.m_offset_x = 0;
    query.m_offset_y = 0;
    query.m_offset_z = 0;
    query.m_primitiveOffset = 0;
    query.m_shift = 0;
    query.m_properties[0] = 0;
    uint8_t* pCommands = const_cast<uint8_t*>(m_code.m_storage->m_data.begin());
    m_reindexingMask.m_storage = (*pCommands != 13) - 1;
    hkBool bRemoveAll = queryModicationPointsRecursive(&query, pCommands, 0);
    if (bRemoveAll) {
        modifierOut->addTerminalRemoveInfo(*m_code.m_storage->m_data.begin() != 13 ? 0 : 5);
    }
}
