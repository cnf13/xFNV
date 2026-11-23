#pragma once

#include "BSCore/BSThread.h"
#include "BSHavok/HavokThreadMemoryStruct.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
#include "SDK/MSVC/ehvec.h"

// FalloutAudioMedia's worker thread.
class FAMThread : public BSThread {
public:
    // The thread's parameter is FalloutAudioMedia::MediaThreadInfo, which FAMThread also holds.
    using TrackData = FalloutAudioMedia::MediaThreadInfo;

    FAMThread();
    ~FAMThread() override;

    // Runs FalloutAudioMedia::ThreadProc on m_param each time the event is signalled while m_state bit 0 is set, with
    // the thread's Havok memory router.
    uint32_t ThreadProc() override;

    void AddNewTrack(FAMThread::TrackData* apTrack);
    void Wait();

    static void operator delete(void* apMemory);
    // The array is allocated from and freed back to ourMalloc's heap, as the game's Startup and vector deleting
    // destructor do.
    static void* operator new[](size_t astSize);
    static void operator delete[](void* apMemory);

    BSSystemEvent* m_event;
    TrackData m_param;
    uint32_t m_state;
    HavokThreadMemoryStruct m_havok_memory;

    static FAMThread::TrackData kNewTrack;
};

// The two audio-media worker threads FalloutAudioMedia::Startup allocates.
struct FalloutAudioManagerThreads {
    FAMThread kThreads[2];
};
