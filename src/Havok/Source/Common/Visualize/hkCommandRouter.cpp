#include "Havok/Source/Common/Visualize/hkCommandRouter.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/System/Io/IArchive/hkIArchive.h"
#include "Havok/Source/Common/Serialize/Serialize/hkDisplaySerializeIStream.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"

// Maps each command handler consumes to handler.
void hkCommandRouter::registerProcess(hkProcess* handler) {
    uint8_t* commands = nullptr;
    int32_t numCommands = 0;
    handler->getConsumableCommands(commands, numCommands);
    for (int32_t i = 0; i < numCommands; ++i) {
        m_commandMap.insert(commands[i], handler);
    }
}

void hkCommandRouter::unregisterProcess(hkProcess* handler) {
    int32_t v4 = 0;
    uint8_t* v7 = nullptr;
    int32_t v6 = 0;
    handler->getConsumableCommands(v7, v6);
    if (v6 > 0) {
        hkPointerMap<uint8_t, hkProcess*, hkContainerHeapAllocator>* p_m_commandMap = &m_commandMap;
        do {
            p_m_commandMap->m_map.remove(static_cast<uintptr_t>(v7[v4++]));
        }
        while (v4 < v6);
    }
}

hkBool hkCommandRouter::consumeCommands(hkDisplaySerializeIStream* stream) {
    uint8_t commandRead;
    uint8_t command;
    do {
        stream->readArrayGeneric(&commandRead, 1, 1);
        command = commandRead;
        if (command != COMMAND_ACK) {
            decltype(m_commandMap)::Storage uiProcess;
            if (m_commandMap.m_map.getWithDefaultStatus(command, &uiProcess) == HK_SUCCESS && uiProcess) {
                decltype(m_commandMap)::FromMapInt<hkProcess*>(uiProcess)->consumeCommand(command);
            }
            else {
                // No handler: skip the command's known payloads and warn.
                alignas(16) hkVector4 skippedVector;
                if (command == 0xB0) {
                    uint64_t skippedId;
                    stream->readQuadVector4(skippedVector);
                    stream->readArrayGeneric(&skippedId, 8, 1);
                }
                else if (command == 0xB1) {
                    stream->readQuadVector4(skippedVector);
                }
                hkStringBuf warning;
                warning.printf("VDB: Found a command (%x) with no handler. Could corrupt the stream.", command);
            }
        }
    }
    while (stream->isOk().m_bool && command != COMMAND_ACK);
    return 1;
}
