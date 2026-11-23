#pragma once

#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <stdint.h>

class hkDisplaySerializeIStream;
class hkProcess;

// Sends the commands a client stream carries to the process registered for each command byte.
class hkCommandRouter : public hkReferencedObject {
public:
    enum : int32_t {
        COMMAND_STEP = 0x00,
        COMMAND_ACK = 0xF0,
    };

    ~hkCommandRouter() override;

    // Maps each command handler consumes to handler.
    void registerProcess(hkProcess* handler);
    void unregisterProcess(hkProcess* handler);
    // Reads commands off stream and hands each to the process registered for it until the
    // stream ends or a COMMAND_ACK; sets *apResult to true.
    hkBool consumeCommands(hkDisplaySerializeIStream* stream);

    hkPointerMap<uint8_t, hkProcess*> m_commandMap;
};
