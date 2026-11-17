#pragma once

#include "BSCore/BSThread.h"
#include "BSHavok/HavokThreadMemoryStruct.h"
#include "Fallout/Audio/FalloutAudioMedia.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

// FalloutAudioMedia's worker thread, named in an anonymous namespace in the PDB.
class FAMThread : public BSThread {
public:
	// The thread's parameter is the PDB's FalloutAudioMedia::MediaThreadInfo, which the PDB's FAMThread also holds
	// here (at 0x44 in the Xbox build). IDA's name for it, kept for the emitted bodies that still spell it.
	using TrackData = FalloutAudioMedia::MediaThreadInfo;

	FAMThread();
	~FAMThread() override;

	// Runs FalloutAudioMedia::ThreadProc on m_param each time the event is signalled while
	// m_state bit 0 is set, with the thread's Havok memory router (thiscall, ret 0, 0x82F7B0; vtable slot 1).
	uint32_t ThreadProc() override;

	void AddNewTrack(FAMThread::TrackData* a1);
	void Wait();

	static void operator delete(void* apMemory);
	// The array comes from and goes back to ourMalloc's heap, as the exe's Startup and vector deleting destructor do.
	static void* operator new[](size_t astSize);
	static void operator delete[](void* apMemory);

	BSSystemEvent* m_event;
	TrackData m_param;
	uint32_t m_state;
	HavokThreadMemoryStruct m_havok_memory;

	static FAMThread::TrackData kNewTrack;
};

// The two audio-media worker threads FalloutAudioMedia::Startup allocates (IDA name; not in the PDB). The exe
// allocates the 2-element array with its cookie (0x9C bytes at 0x82F830: 2*0x4C plus the cookie) and initializes
// both threads, the second at +0x4C.
struct FalloutAudioManagerThreads {
	FAMThread kThreads[2];
};

inline constexpr fnvd::Vtable FAMThread_vftable{};
