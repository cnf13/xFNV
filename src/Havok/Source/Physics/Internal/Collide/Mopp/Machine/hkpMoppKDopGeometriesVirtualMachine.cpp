#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppKDopGeometriesVirtualMachine.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "SDK/MSVC/string.h"

float exchangeLowBound(hkp26Dop* apDop, int32_t aiAxis, float afValue) {
    float v5 = *(&apDop->m_lx + 2 * aiAxis);
    *(&apDop->m_lx + 2 * aiAxis) = afValue;
    return v5;
}

float exchangeHighBound(hkp26Dop* apDop, int32_t aiAxis, float afValue) {
    float v5 = *(&apDop->m_hx + 2 * aiAxis);
    *(&apDop->m_hx + 2 * aiAxis) = afValue;
    return v5;
}

// Reads the float pair aiIndex of apPairs into apFirstOut and apSecondOut, then stores afFirst and afSecond in its
// place.
int32_t
swapPair(float* apPairs, float afFirst, float afSecond, float* apFirstOut, int32_t aiIndex, float* apSecondOut) {
    float* pPair = &apPairs[2 * aiIndex];
    float fValue = pPair[0];
    *apFirstOut = fValue;
    fValue = pPair[1];
    *apSecondOut = fValue;
    fValue = afFirst;
    pPair[0] = fValue;
    fValue = afSecond;
    pPair[1] = fValue;
    return aiIndex;
}

// hkpMoppKDopQuery has no constructor of its own: the SDK's inline one is written out (depth -1, the rest zero).
hkpMoppKDopGeometriesVirtualMachine::hkpMoppKDopGeometriesVirtualMachine() {
    m_queryObject.m_earlyExit.m_bool = 0;
    m_queryObject.m_kdopDepth = -1;
    m_queryObject.m_useSpecifiedID.m_bool = 0;
    m_queryObject.m_specifiedId = 0;
}

// The visited terminals' storage goes with m_visitedTerminals.
hkpMoppKDopGeometriesVirtualMachine::~hkpMoppKDopGeometriesVirtualMachine() {}

void hkpMoppKDopGeometriesVirtualMachine::queryMopp(
    const hkpMoppCode* code,
    const hkpMoppKDopQuery& queryInput,
    hkpMoppInfo* kDopGeometries
) {
    hkpMoppKDopGeometriesVirtualMachine::hkpMoppKDopGeometriesVirtualMachineQuery scaledQuery;
    m_kDopGeometries = kDopGeometries;
    double v4 = 1.0 / code->m_info.m_offset.m_quad[3];
    std::memset(&scaledQuery, 0, 20);
    m_ItoFScale = v4;
    m_offset = code->m_info.m_offset;
    m_hitFound.m_bool = 0;
    double v5 = m_ItoFScale * 16777216.0;
    m_queryObject = queryInput;
    float v10 = v5;
    double v6 = v10;
    m_terminaloffset = -1;
    m_level = 0;
    float* p_m_hx = &m_kdop.m_hx;
    float v11 = -v10 * 2.0;
    int32_t v8 = 13;
    do {
        p_m_hx += 2;
        --v8;
        *(p_m_hx - 3) = v11;
        float v12 = v6 * 2.0;
        *(p_m_hx - 2) = v12;
    }
    while (v8);
    uint8_t* m_data = code->m_data.m_data;
    scaledQuery.m_properties[0] = 0;
    queryMoppKDopGeometriesRecurse(&scaledQuery, m_data);
}

// Walks the MOPP code from assertBuf until m_hitFound, narrowing m_kdop at each split for each child in turn (the first
// child with the split's lower plane as the axis's high bound, the second with its upper plane as the low bound, each
// restored after its walk); a node with bounds records the DOP through addKDopGeometry when the depth allows, and a
// terminal goes to addHit. A rescale, a terminal offset or a property command moves the query into query first, which
// the recursive calls are given. The bounds are copied as floats.
// KEEP: on a property command (96 to 107) the exe stores the value into query's slot for that property (properties
// 1 to 3 land past the 24-byte local, in other frame slots), reads query.m_properties[0], copies the query into
// query when scaledQuery is not query yet, and writes the value it read back. With the copy taken, that read is of a
// local nothing wrote yet, and the value goes on to the recursive calls and to addHit in query: src stores property
// 0 only.
void hkpMoppKDopGeometriesVirtualMachine::queryMoppKDopGeometriesRecurse(
    const hkpMoppKDopGeometriesVirtualMachineQuery* scaledQuery,
    const uint8_t* PC
) {
    hkpMoppKDopGeometriesVirtualMachineQuery query;
    while (!m_hitFound.m_bool) {
        const int32_t command = *PC;
        // A split: its axis, the first child's high bound and the second child's low bound, and both children's
        // offsets past the node.
        int32_t axis;
        float leftHigh;
        float rightLow;
        int32_t leftOffset;
        int32_t rightOffset;
        // A node with bounds: its axis and bounds, and whether the DOP was recorded for it.
        int32_t boundsAxis;
        float lowBound;
        float highBound;
        int32_t property;
        uint32_t propertyValue;
        switch (command) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4: {
                // Rescale: the node's space is finer by command bits; the offsets move in the old space.
                const int32_t shift = 16 - scaledQuery->m_shift;
                query.m_offset_x = scaledQuery->m_offset_x + (PC[1] << shift);
                query.m_offset_y = scaledQuery->m_offset_y + (PC[2] << (16 - scaledQuery->m_shift));
                query.m_offset_z = scaledQuery->m_offset_z + (PC[3] << (16 - scaledQuery->m_shift));
                query.m_shift = scaledQuery->m_shift + command;
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
                // A split along one axis, the second child an 8-bit offset away.
                axis = command - 16;
                const int32_t shift = 16 - scaledQuery->m_shift;
                const int32_t offset = (&scaledQuery->m_offset_x)[axis];
                leftHigh = static_cast<double>((PC[1] << shift) + offset) * m_ItoFScale;
                rightLow = static_cast<double>((PC[2] << shift) + offset) * m_ItoFScale;
                leftOffset = 0;
                rightOffset = PC[3];
                PC += 4;
                break;
            }
            case 19:
            case 20:
            case 21:
            case 22:
            case 23:
            case 24:
            case 25:
            case 26:
            case 27:
            case 28: {
                // A split along a diagonal: the query's offset along it, the planes in the node's space.
                const int32_t shift = 16 - scaledQuery->m_shift;
                int32_t offset;
                int32_t lo;
                int32_t hi;
                switch (command) {
                    case 19:
                        offset = scaledQuery->m_offset_z + scaledQuery->m_offset_y;
                        lo = 2 * PC[1] << shift;
                        hi = 2 * PC[2] << shift;
                        break;
                    case 20:
                        offset = scaledQuery->m_offset_y - scaledQuery->m_offset_z;
                        lo = (2 * PC[1] - 255) << shift;
                        hi = (2 * PC[2] - 255) << shift;
                        break;
                    case 21:
                        offset = scaledQuery->m_offset_z + scaledQuery->m_offset_x;
                        lo = 2 * PC[1] << shift;
                        hi = 2 * PC[2] << shift;
                        break;
                    case 22:
                        offset = scaledQuery->m_offset_x - scaledQuery->m_offset_z;
                        lo = (2 * PC[1] - 255) << shift;
                        hi = (2 * PC[2] - 255) << shift;
                        break;
                    case 23:
                        offset = scaledQuery->m_offset_y + scaledQuery->m_offset_x;
                        lo = 2 * PC[1] << shift;
                        hi = 2 * PC[2] << shift;
                        break;
                    case 24:
                        offset = scaledQuery->m_offset_x - scaledQuery->m_offset_y;
                        lo = (2 * PC[1] - 255) << shift;
                        hi = (2 * PC[2] - 255) << shift;
                        break;
                    case 25:
                        offset = scaledQuery->m_offset_z + scaledQuery->m_offset_y + scaledQuery->m_offset_x;
                        lo = 3 * PC[1] << shift;
                        hi = 3 * PC[2] << shift;
                        break;
                    case 26:
                        offset = scaledQuery->m_offset_y - scaledQuery->m_offset_z + scaledQuery->m_offset_x;
                        lo = 3 * (PC[1] - 85) << shift;
                        hi = 3 * (PC[2] - 85) << shift;
                        break;
                    case 27:
                        offset = scaledQuery->m_offset_z - scaledQuery->m_offset_y + scaledQuery->m_offset_x;
                        lo = 3 * (PC[1] - 85) << shift;
                        hi = 3 * (PC[2] - 85) << shift;
                        break;
                    default:
                        offset = scaledQuery->m_offset_x - scaledQuery->m_offset_z - scaledQuery->m_offset_y;
                        lo = 3 * (PC[1] - 170) << shift;
                        hi = 3 * (PC[2] - 170) << shift;
                        break;
                }
                // The axes 3 to 12 of hkp26Dop follow the commands 19 to 28.
                axis = command - 16;
                const float offsetF = static_cast<float>(offset);
                const float loPlane = static_cast<double>(lo) + offsetF;
                leftHigh = loPlane * m_ItoFScale;
                const float hiPlane = offsetF + static_cast<double>(hi);
                rightLow = hiPlane * m_ItoFScale;
                leftOffset = 0;
                rightOffset = PC[3];
                PC += 4;
                break;
            }
            case 32:
            case 33:
            case 34: {
                // A single plane along one axis: one cell of the node's space wide.
                axis = command - 32;
                const int32_t shift = 16 - scaledQuery->m_shift;
                const float plane = static_cast<float>((PC[1] << shift) + (&scaledQuery->m_offset_x)[axis]);
                const int32_t width = 1 << shift;
                const float planeEnd = static_cast<double>(width) + plane;
                rightLow = plane * m_ItoFScale;
                leftHigh = m_ItoFScale * planeEnd;
                leftOffset = 0;
                rightOffset = PC[2];
                PC += 3;
                break;
            }
            case 35:
            case 36:
            case 37: {
                // A split along one axis with 16-bit offsets to both children.
                axis = command - 35;
                const int32_t shift = 16 - scaledQuery->m_shift;
                const int32_t offset = (&scaledQuery->m_offset_x)[axis];
                leftHigh = static_cast<double>((PC[1] << shift) + offset) * m_ItoFScale;
                rightLow = static_cast<double>((PC[2] << shift) + offset) * m_ItoFScale;
                leftOffset = (PC[3] << 8) + PC[4];
                rightOffset = (PC[5] << 8) + PC[6];
                PC += 7;
                break;
            }
            case 38:
            case 39:
            case 40: {
                // The node's bounds along one axis.
                boundsAxis = command - 38;
                const int32_t shift = 16 - scaledQuery->m_shift;
                const int32_t offset = (&scaledQuery->m_offset_x)[boundsAxis];
                lowBound = static_cast<double>((PC[1] << shift) + offset) * m_ItoFScale;
                highBound = static_cast<double>((PC[2] << shift) + offset) * m_ItoFScale;
                PC += 3;
                goto walkBounds;
            }
            case 41:
            case 42:
            case 43:
                // The node's unscaled 24-bit bounds along one axis.
                boundsAxis = command - 41;
                lowBound = static_cast<double>((((PC[1] << 8) + PC[2]) << 8) + PC[3]) * m_ItoFScale;
                highBound = static_cast<double>((((PC[4] << 8) + PC[5]) << 8) + PC[6]) * m_ItoFScale;
                PC += 7;
            walkBounds:
                {
                    const int32_t depth = m_queryObject.m_kdopDepth;
                    bool recorded;
                    if (depth && m_level >= depth) {
                        recorded = false;
                    }
                    else {
                        recorded = true;
                        addKDopGeometry(false, 0xFFFFFFFFu);
                    }
                    float* pLow = &(&m_kdop.m_lx)[2 * boundsAxis];
                    float* pHigh = &(&m_kdop.m_hx)[2 * boundsAxis];
                    const float oldLow = *pLow;
                    const float oldHigh = *pHigh;
                    *pLow = lowBound;
                    *pHigh = highBound;
                    ++m_level;
                    queryMoppKDopGeometriesRecurse(scaledQuery, PC);
                    --m_level;
                    // Only the 8-bit bounds test m_hitFound before the record is taken back.
                    if (recorded && m_queryObject.m_useSpecifiedID.m_bool && (command > 40 || !m_hitFound.m_bool)) {
                        --m_kDopGeometries;
                    }
                    *pLow = oldLow;
                    *pHigh = oldHigh;
                    return;
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
            case 79: m_terminaloffset = command - 48; goto addTerminal;
            case 80: m_terminaloffset = PC[1]; goto addTerminal;
            case 81: m_terminaloffset = (PC[1] << 8) + PC[2]; goto addTerminal;
            case 82:
                m_terminaloffset = PC[2] << 8;
                m_terminaloffset = (PC[1] << 16) + ((PC[2] << 8) + PC[3]);
            addTerminal:
                m_terminaloffset += scaledQuery->m_primitiveOffset;
                addHit(m_terminaloffset, scaledQuery->m_properties);
                m_terminaloffset = 0xFFFFFFFFu;
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
                        ".\\Collide\\Mopp\\Machine\\hkpMoppKDopGeometriesVirtualMachine.cpp", 593
                    )) {
                    __builtin_debugtrap();
                }
                continue;
            }
        }
        // A split (16 to 37): the first child with the lower plane as the axis's high bound, then the second with
        // the upper plane as its low bound, each bound restored after its walk.
        float* pHigh = &(&m_kdop.m_hx)[2 * axis];
        const float oldHigh = *pHigh;
        *pHigh = leftHigh;
        queryMoppKDopGeometriesRecurse(scaledQuery, PC + leftOffset);
        *pHigh = oldHigh;
        float* pLow = &(&m_kdop.m_lx)[2 * axis];
        const float oldLow = *pLow;
        *pLow = rightLow;
        queryMoppKDopGeometriesRecurse(scaledQuery, PC + rightOffset);
        *pLow = oldLow;
        return;
    }
}

void hkpMoppKDopGeometriesVirtualMachine::addHit(uint32_t id, const uint32_t* properties) {
    uint32_t* m_data;
    int32_t m_size = m_visitedTerminals.m_size;
    hkArray<uint32_t, hkContainerHeapAllocator>* p_m_visitedTerminals = &m_visitedTerminals;
    int32_t v6 = 0;
    if (m_size <= 0) {
        goto LABEL_7;
    }
    m_data = p_m_visitedTerminals->begin();
    while (*m_data != id) {
        ++v6;
        ++m_data;
        if (v6 >= m_size) {
            goto LABEL_7;
        }
    }
    if (v6 == -1) {
    LABEL_7:;
        char m_bool = m_queryObject.m_useSpecifiedID.m_bool;
        bool v9 = m_bool && id == m_queryObject.m_specifiedId;
        if (!m_bool || v9) {
            addKDopGeometry(true, id);
            char v10 = m_queryObject.m_earlyExit.m_bool || v9;
            m_queryObject.m_earlyExit.m_bool = v10;
        }
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        if (m_visitedTerminals.m_size ==
            (m_visitedTerminals.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*Instance->m_heap, &m_visitedTerminals, sizeof(uint32_t));
        }
        uint32_t* v12 = p_m_visitedTerminals->begin();
        m_visitedTerminals.m_data[m_visitedTerminals.m_size++] = id;
    }
    if (m_queryObject.m_earlyExit.m_bool) {
        m_hitFound.m_bool = 1;
    }
}

void hkpMoppKDopGeometriesVirtualMachine::addKDopGeometry(bool isTerminal, uint32_t shapeKey) {
    hkpMoppInfo* m_kDopGeometries = this->m_kDopGeometries;
    m_kDopGeometries->m_isTerminal.m_bool = isTerminal;
    if (isTerminal) {
        m_kDopGeometries->m_shapeKey = shapeKey;
    }
    m_kDopGeometries->m_level = m_level;
    (*m_kDopGeometries).m_dop = m_kdop;
    ++this->m_kDopGeometries;
}
