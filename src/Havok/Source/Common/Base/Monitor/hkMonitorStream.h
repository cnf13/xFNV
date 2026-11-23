#pragma once

#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "SDK/Windows/winbase.h"
#include "xfnv/Compiler.h"

// A thread's buffer of timer and monitor events.
class hkMonitorStream {
public:
    // A command in the stream: its type character and the monitor name, as one string.
    struct Command {
        const char* m_commandAndMonitor;
    };

    // A value added to a monitor ("M"): hkMonitorStreamAnalyzer::makeStatisticsTreeForSingleFrame adds
    // the float at +4 to the node's value and steps 8 bytes.
    struct AddValueCommand : Command {
        float m_value;
    };

    // A timer command: the command and the time stamp it was taken at.
    struct TimerCommand : Command {
        // Stores the low 32 bits of rdtsc into m_time0; m_time1 is left as it was.
        void setTime() {
            m_time0 = static_cast<uint32_t>(__rdtsc());
        }

        uint32_t m_time0;
        uint32_t m_time1;
    };

    // The command that opens a timer list, with the name of its first split.
    struct TimerBeginListCommand : TimerCommand {
        const char* m_nameOfFirstSplit;
    };

    // The command that opens a timer named after an object ("O"): the analyzer reads m_objectName at +0xC and steps
    // 16 bytes.
    struct TimerBeginObjectNameCommand : TimerCommand {
        const char* m_objectName;
    };

    // A memory event ("m"): the block's address and its size in bytes, the high bit set for a free (the SDK's
    // HK_MONITOR_MEMORY_* commands).
    struct MemoryCommand : Command {
        void* m_ptr;
        int32_t m_sizeAndFlags;
    };

    // Empties the buffer (m_end = m_start).
    void reset();
    // Frees the thread's stream and clears its TLS slot.
    void quit();
    // Reallocates the buffer to newSize bytes.
    void resize(int32_t newSize);
    // Frees a heap buffer (resize(0)) and makes buffer, of bufferSize bytes, the stream's static buffer.
    void setStaticBuffer(char* buffer, int32_t bufferSize);

    // Inline accessors the timer macros (HK_TIMER_BEGIN, HK_TIMER_END) write through.
    char* getEnd() {
        return m_end;
    }

    char* getCapacityMinus16() {
        return m_capacityMinus16;
    }

    hkBool memoryAvailable() {
        return getEnd() < getCapacityMinus16();
    }

    void setEnd(char* ptr) {
        m_end = ptr;
    }

    // The thread's stream, kept in the TLS slot m_instance.
    static hkMonitorStream& getInstance() {
        return *static_cast<hkMonitorStream*>(TlsGetValue(m_instance.m_slotID));
    }

    static void init();

    char* m_start;
    char* m_end;
    char* m_capacity;
    char* m_capacityMinus16;
    hkBool m_isBufferAllocatedOnTheHeap;

    // The SDK's hkMonitorStream__m_instance (HK_THREAD_LOCAL(hkMonitorStream*)).
    static hkThreadLocalData<hkMonitorStream*> m_instance;
};

// HK_TIME_CODE_BLOCK's guard: its inline destructor writes the end-timer command into the thread's stream. No data.
class hkTimeFunctionHelper {};

// The SDK's timer macros (hkMonitorStream.inl, monitors enabled): a command naming the timer, with its time stamp,
// appended to the thread's stream while 16 bytes are left. The stream is an untyped byte buffer the commands are
// written into in place, so its end is cast to the command and back.
#define HK_MONITOR_TYPE_TIMER "t"
#define HK_MONITOR_COMMAND_BEGIN_TIMER "T"
#define HK_MONITOR_COMMAND_END_TIMER "E"
#define HK_TIMER_INTERN(NAME, COMMAND, OBJECT)                                                                         \
    {                                                                                                                  \
        hkMonitorStream& mStream = hkMonitorStream::getInstance();                                                     \
        if (mStream.memoryAvailable()) {                                                                               \
            hkMonitorStream::TimerCommand* h = reinterpret_cast<hkMonitorStream::TimerCommand*>(mStream.getEnd());     \
            h->m_commandAndMonitor = COMMAND HK_MONITOR_TYPE_TIMER NAME;                                               \
            h->setTime();                                                                                              \
            mStream.setEnd(reinterpret_cast<char*>(h + 1));                                                            \
        }                                                                                                              \
    }
#define HK_TIMER_BEGIN(NAME, OBJECT) HK_TIMER_INTERN(NAME, HK_MONITOR_COMMAND_BEGIN_TIMER, OBJECT)
#define HK_TIMER_END() HK_TIMER_INTERN("", HK_MONITOR_COMMAND_END_TIMER, nullptr)
#define HK_TIMER_NAMED_END(NAME) HK_TIMER_INTERN(NAME, HK_MONITOR_COMMAND_END_TIMER, nullptr)

// Timer list macros: a list opens with a command naming the list and its first split, each split and the list's end are
// plain timer commands, all written in place into the stream's untyped buffer.
#define HK_MONITOR_COMMAND_BEGIN_LIST "L"
#define HK_MONITOR_COMMAND_SPLIT_LIST "S"
#define HK_MONITOR_COMMAND_END_LIST "l"
#define HK_TIMER_BEGIN_LIST(NAME, FIRST_SPLIT)                                                                         \
    {                                                                                                                  \
        hkMonitorStream& mStream = hkMonitorStream::getInstance();                                                     \
        if (mStream.memoryAvailable()) {                                                                               \
            hkMonitorStream::TimerBeginListCommand* h = reinterpret_cast<hkMonitorStream::TimerBeginListCommand*>(     \
                mStream.getEnd()                                                                                       \
            );                                                                                                         \
            h->m_commandAndMonitor = HK_MONITOR_COMMAND_BEGIN_LIST HK_MONITOR_TYPE_TIMER NAME;                         \
            h->m_nameOfFirstSplit = HK_MONITOR_COMMAND_SPLIT_LIST HK_MONITOR_TYPE_TIMER FIRST_SPLIT;                   \
            h->setTime();                                                                                              \
            mStream.setEnd(reinterpret_cast<char*>(h + 1));                                                            \
        }                                                                                                              \
    }
#define HK_TIMER_SPLIT_LIST(NAME) HK_TIMER_INTERN(NAME, HK_MONITOR_COMMAND_SPLIT_LIST, nullptr)
#define HK_TIMER_END_LIST() HK_TIMER_INTERN("", HK_MONITOR_COMMAND_END_LIST, nullptr)

// The SDK's HK_MONITOR_ADD_VALUE: a value command ("M", the monitor's type, the name) and the value, appended to the
// thread's stream in place like the timer commands.
#define HK_MONITOR_COMMAND_ADD_VALUE "M"
#define HK_MONITOR_TYPE_INT "i"
#define HK_MONITOR_ADD_VALUE(NAME, VALUE, MONITOR_TYPE)                                                                \
    {                                                                                                                  \
        hkMonitorStream& mStream = hkMonitorStream::getInstance();                                                     \
        if (mStream.memoryAvailable()) {                                                                               \
            hkMonitorStream::AddValueCommand* h = reinterpret_cast<hkMonitorStream::AddValueCommand*>(                 \
                mStream.getEnd()                                                                                       \
            );                                                                                                         \
            h->m_commandAndMonitor = HK_MONITOR_COMMAND_ADD_VALUE MONITOR_TYPE NAME;                                   \
            h->m_value = VALUE;                                                                                        \
            mStream.setEnd(reinterpret_cast<char*>(h + 1));                                                            \
        }                                                                                                              \
    }
