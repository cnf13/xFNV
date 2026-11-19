#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppFindAllVirtualMachine.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppVirtualMachine.h"

// Walks the MOPP code from assertBuf and appends every terminal under it, offset by the query's primitive offset, to
// m_primitives_out; each node's both children are walked (the first through a recursive call). The query is
// copied into the local query before the walk changes its offset or a property.
// KEEP: on a property command (96 to 107) the exe stores the value into query's slot for that property
// (properties 1 to 3 land past the 8-byte local, in other frame slots: property 1 on the saved this), reads
// query.m_properties[0], copies the query into query when scaledQuery is not query yet, and writes the value it
// read back. With the copy taken, that read is of a local nothing wrote yet, and the value goes on to the
// recursive calls in query: src stores property 0 only.
void hkpMoppFindAllVirtualMachine::queryOnTree(
    const hkpMoppFindAllVirtualMachineQuery* scaledQuery,
    const uint8_t* PC
) const {
    hkpMoppFindAllVirtualMachineQuery query;
    while (true) {
        const int32_t command = *PC;
        int32_t offset;
        int32_t terminal;
        int32_t property;
        uint32_t propertyValue;
        switch (command) {
            case 0: return;
            case 1:
            case 2:
            case 3:
            case 4:
                // A rescale: this walk tests no bounds.
                PC += 4;
                continue;
            case 5: PC += PC[1] + 2; continue;
            case 6: PC += (PC[1] << 8) + PC[2] + 3; continue;
            case 7: PC += (((PC[1] << 8) + PC[2]) << 8) + PC[3] + 4; continue;
            case 9:
                offset = PC[1];
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                query.m_primitiveOffset += offset;
                PC += 2;
                continue;
            case 10:
                offset = (PC[1] << 8) + PC[2];
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                query.m_primitiveOffset += offset;
                PC += 3;
                continue;
            case 11:
                offset = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                query.m_primitiveOffset = offset;
                PC += 5;
                continue;
            case 16:
            case 17:
            case 18:
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
                // A split with an 8-bit offset to the second child; the first follows.
                const int32_t rightOffset = PC[3];
                PC += 4;
                queryOnTree(scaledQuery, PC);
                PC += rightOffset;
                continue;
            }
            case 32:
            case 33:
            case 34: {
                const int32_t rightOffset = PC[2];
                PC += 3;
                queryOnTree(scaledQuery, PC);
                PC += rightOffset;
                continue;
            }
            case 35:
            case 36:
            case 37: {
                // Both children at 16-bit offsets.
                const int32_t leftOffset = (PC[3] << 8) + PC[4];
                const int32_t rightOffset = (PC[5] << 8) + PC[6];
                PC += 7;
                queryOnTree(scaledQuery, PC + leftOffset);
                PC += rightOffset;
                continue;
            }
            case 38:
            case 39:
            case 40: PC += 3; continue;
            case 41:
            case 42:
            case 43: PC += 7; continue;
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
                    const uint32_t id = scaledQuery->m_primitiveOffset + terminal;
                    m_primitives_out->pushBack(hkpMoppPrimitiveInfo{id});
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
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                if (property == 0) {
                    query.m_properties[0] = propertyValue;
                }
                continue;
            default: {
                // HK_ERROR; the same command is then read again, as the game does.
                char buf[512];
                hkErrStream kStream(buf, 512);
                kStream.operator<<(
                    "Unknown command - This mopp data has been corrupted (check for memory trashing), or an "
                    "hkpMoppBvTreeShape has been pointed at invalid mopp data.\n"
                );
                if (hkError::s_instance->message(
                        hkError::MESSAGE_ERROR, 0x1298FEDD, buf,
                        ".\\Collide\\Mopp\\Machine\\hkpMoppFindAllVirtualMachine.cpp", 113
                    )) {
                    __builtin_debugtrap();
                }
                continue;
            }
        }
    }
}

// queryOnTree's walk down the first child of each node only, with no recursion. Property commands as in
// queryOnTreeRight.
void hkpMoppFindAllVirtualMachine::queryOnTreeLeft(
    const hkpMoppFindAllVirtualMachineQuery* scaledQuery,
    const uint8_t* PC
) const {
    hkpMoppFindAllVirtualMachineQuery query;
    while (true) {
        const int32_t command = *PC;
        int32_t offset;
        int32_t terminal;
        int32_t property;
        uint32_t propertyValue;
        switch (command) {
            case 0: return;
            case 1:
            case 2:
            case 3:
            case 4:
            case 16:
            case 17:
            case 18:
            case 19:
            case 20:
            case 21:
            case 22:
            case 23:
            case 24:
            case 25:
            case 26:
            case 27:
            case 28: PC += 4; continue;
            case 5: PC += PC[1] + 2; continue;
            case 6: PC += (PC[1] << 8) + PC[2] + 3; continue;
            case 7: PC += (((PC[1] << 8) + PC[2]) << 8) + PC[3] + 4; continue;
            case 9:
                offset = PC[1];
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                query.m_primitiveOffset += offset;
                PC += 2;
                continue;
            case 10:
                offset = (PC[1] << 8) + PC[2];
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                query.m_primitiveOffset += offset;
                PC += 3;
                continue;
            case 11:
                offset = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                query.m_primitiveOffset = offset;
                PC += 5;
                continue;
            case 32:
            case 33:
            case 34:
            case 38:
            case 39:
            case 40: PC += 3; continue;
            case 35:
            case 36:
            case 37: PC += (PC[3] << 8) + PC[4] + 7; continue;
            case 41:
            case 42:
            case 43: PC += 7; continue;
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
                    const uint32_t id = scaledQuery->m_primitiveOffset + terminal;
                    m_primitives_out->pushBack(hkpMoppPrimitiveInfo{id});
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
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                if (property == 0) {
                    query.m_properties[0] = propertyValue;
                }
                continue;
            default: {
                // HK_ERROR; the same command is then read again, as the game does.
                char buf[512];
                hkErrStream kStream(buf, 512);
                kStream.operator<<(
                    "Unknown command - This mopp data has been corrupted (check for memory trashing), or an "
                    "hkpMoppBvTreeShape has been pointed at invalid mopp data.\n"
                );
                if (hkError::s_instance->message(
                        hkError::MESSAGE_ERROR, 0x1298FEDD, buf,
                        ".\\Collide\\Mopp\\Machine\\hkpMoppFindAllVirtualMachine.cpp", 305
                    )) {
                    __builtin_debugtrap();
                }
                continue;
            }
        }
    }
}

// queryOnTree's walk down the second child of each node only, with no recursion. On a property command (96 to 107) only
// property 0 is stored into query. A value read for a property 1 to 3 is dead: query's address reaches no callee and
// only its primitive offset is read.
void hkpMoppFindAllVirtualMachine::queryOnTreeRight(
    const hkpMoppFindAllVirtualMachineQuery* scaledQuery,
    const uint8_t* PC
) const {
    hkpMoppFindAllVirtualMachineQuery query;
    while (true) {
        const int32_t command = *PC;
        int32_t offset;
        int32_t terminal;
        int32_t property;
        uint32_t propertyValue;
        switch (command) {
            case 0: return;
            case 1:
            case 2:
            case 3:
            case 4: PC += 4; continue;
            case 5: PC += PC[1] + 2; continue;
            case 6: PC += (PC[1] << 8) + PC[2] + 3; continue;
            case 7: PC += (((PC[1] << 8) + PC[2]) << 8) + PC[3] + 4; continue;
            case 9:
                offset = PC[1];
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                query.m_primitiveOffset += offset;
                PC += 2;
                continue;
            case 10:
                offset = (PC[1] << 8) + PC[2];
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                query.m_primitiveOffset += offset;
                PC += 3;
                continue;
            case 11:
                offset = (((((PC[1] << 8) + PC[2]) << 8) + PC[3]) << 8) + PC[4];
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                query.m_primitiveOffset = offset;
                PC += 5;
                continue;
            case 16:
            case 17:
            case 18:
            case 19:
            case 20:
            case 21:
            case 22:
            case 23:
            case 24:
            case 25:
            case 26:
            case 27:
            case 28: PC += PC[3] + 4; continue;
            case 32:
            case 33:
            case 34: PC += PC[2] + 3; continue;
            case 35:
            case 36:
            case 37: PC += (PC[5] << 8) + PC[6] + 7; continue;
            case 38:
            case 39:
            case 40: PC += 3; continue;
            case 41:
            case 42:
            case 43: PC += 7; continue;
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
                    const uint32_t id = scaledQuery->m_primitiveOffset + terminal;
                    m_primitives_out->pushBack(hkpMoppPrimitiveInfo{id});
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
                if (scaledQuery != &query) {
                    query = *scaledQuery;
                    scaledQuery = &query;
                }
                if (property == 0) {
                    query.m_properties[0] = propertyValue;
                }
                continue;
            default: {
                // HK_ERROR; the same command is then read again, as the game does.
                char buf[512];
                hkErrStream kStream(buf, 512);
                kStream.operator<<(
                    "Unknown command - This mopp data has been corrupted (check for memory trashing), or an "
                    "hkpMoppBvTreeShape has been pointed at invalid mopp data.\n"
                );
                if (hkError::s_instance->message(
                        hkError::MESSAGE_ERROR, 0x1298FEDD, buf,
                        ".\\Collide\\Mopp\\Machine\\hkpMoppFindAllVirtualMachine.cpp", 211
                    )) {
                    __builtin_debugtrap();
                }
                continue;
            }
        }
    }
}
