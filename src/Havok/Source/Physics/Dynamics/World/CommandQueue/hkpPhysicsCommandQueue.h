#pragma once

#include "Havok/Source/Physics/Dynamics/World/CommandQueue/hkpPhysicsCommand.h"

#include <cstddef>

// A pointer stored as the byte distance from the pointer itself to its target.
template <typename TYPE>
class hkpRelativePointer32 {
public:
    // The SDK's inline: the target, this pointer's address plus the stored distance.
    TYPE* val() const {
        return reinterpret_cast<TYPE*>(
            const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(this)) + m_relativeAddress
        );
    }

    uint32_t m_relativeAddress;
};

// A queue of 16-byte commands in a buffer.
class hkpPhysicsCommandQueue {
public:
    enum {
        BYTES_PER_COMMAND = 16,
    };

    // The SDK's inline (hkpPhysicsCommandQueue.inl): copies the command whole into the buffer's next free bytes and
    // steps the size by the command's size rounded up to 16. The buffer is untyped storage the commands are written
    // into in place.
    template <typename COMMAND_STRUCT>
    void addCommand(COMMAND_STRUCT command) {
        COMMAND_STRUCT* dest = reinterpret_cast<COMMAND_STRUCT*>(reinterpret_cast<uint8_t*>(m_start.val()) + m_size);
        *dest = command;
        m_size += static_cast<uint32_t>((sizeof(COMMAND_STRUCT) + 15) & ~size_t{15});
    }

    void init(hkpPhysicsCommand* bufferStart, int32_t bufferSize) {
        m_start.m_relativeAddress = static_cast<uint32_t>(
            reinterpret_cast<uint8_t*>(bufferStart) - reinterpret_cast<uint8_t*>(this)
        );
        m_size = 0;
        m_capacity = static_cast<uint32_t>(bufferSize);
    }

    hkpRelativePointer32<hkpPhysicsCommand> m_start;
    uint32_t m_size;
    uint32_t m_capacity;
};

// A command queue with its own buffer of NUM_BYTES, 16-byte aligned.
template <int32_t NUM_BYTES>
class hkpFixedSizePhysicsCommandQueue : public hkpPhysicsCommandQueue {
public:
    // An empty queue over m_buffer.
    hkpFixedSizePhysicsCommandQueue() {
        // m_start points at m_buffer: the distance between the two fields.
        m_start.m_relativeAddress = static_cast<uint32_t>(
            offsetof(hkpFixedSizePhysicsCommandQueue, m_buffer) - offsetof(hkpPhysicsCommandQueue, m_start)
        );
        m_size = 0;
        m_capacity = NUM_BYTES;
    }

    alignas(16) uint8_t m_buffer[NUM_BYTES];
};
