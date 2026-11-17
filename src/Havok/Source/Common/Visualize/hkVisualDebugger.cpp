#include "Havok/Source/Common/Visualize/hkVisualDebugger.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkVtableClassRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/FileSystem/hkFileSystem.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Socket/hkSocket.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/System/Stopwatch/hkStopwatch.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Visualize/Process/hkDebugDisplayProcess.h"
#include "Havok/Source/Common/Visualize/Serialize/hkDisplaySerializeOStream.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/hkProcessHandler.h"
#include "Havok/Source/Common/Visualize/hkProcessRegisterUtil.h"
#include "Havok/Source/Common/Visualize/hkServerProcessHandler.h"
#include "Havok/Source/Common/Visualize/hkVersionReporter.h"

// The contexts copied and told their owner, the report, the common processes registered and the three default viewers
// added. hkStopwatch has no constructor, so the name and reset() are done here.
hkVisualDebugger::hkVisualDebugger(const hkArray<hkProcessContext*>* contexts, const hkVtableClassRegistry* classReg)
  : m_server(nullptr), s_SuppressPollForNewClients(false), m_classReg(classReg), m_amTimingFrame(false) {
    m_frameTimer.m_name = "Frame Timer";
    m_frameTimer.reset();
    m_contexts.copyFromArray(*hkMemoryRouter::getInstance().m_heap, *contexts, hkTypeIsPod());
    for (int32_t i = 0; i < m_contexts.m_size; ++i) {
        m_contexts.m_data[i]->setOwner(this);
    }
    hkError::s_instance->sectionBegin(0x1293ADEF, "Visual Debugger");
    {
        char acBuffer[512];
        hkErrStream kStream(acBuffer, 512);
        kStream << "VDB Server instance has been created";
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\hkVisualDebugger.cpp", 40);
    }
    hkProcessRegisterUtil::registerAllCommonProcesses();
    addDefaultProcess(hkDebugDisplayProcess_Name);
    addDefaultProcess("Shapes");
    addDefaultProcess("MousePicking");
    if (m_classReg) {
        m_classReg->addReference();
    }
    hkError::s_instance->sectionEnd();
}

// The SDK's destructor: shut down, the contexts let go, the default processes dropped and the class registry
// released; the arrays free themselves.
hkVisualDebugger::~hkVisualDebugger() {
    shutdown();
    for (int32_t i = 0; i < m_contexts.m_size; ++i) {
        m_contexts.m_data[i]->setOwner(nullptr);
    }
    m_defaultProcesses.clear();
    if (m_classReg) {
        m_classReg->removeReference();
    }
}

// Serves one frame. When the debugger times the frame itself, the elapsed time (rounded to a float of milliseconds)
// replaces the caller's frame time; the timing then stops and the flag is set. The frame time falls back to 16 ms when
// the caller passes 0 (or a value of magnitude at most FLT_EPSILON). Each client is asked twice whether its handler is
// still alive: once to step and write it, and once more to drop it, so a handler that dies during its own step is
// cleaned up at once.
void hkVisualDebugger::step(float frameTimeInMs) {
    float fFrameTime = 16.0f;
    if (m_amTimingFrame.m_bool) {
        // The elapsed ticks are read before the clock's rate; clang would evaluate divide64's arguments right to left,
        // so the ticks go through a local first.
        // EXE-INLINED none: hkStopwatch::getElapsedSeconds
        const uint64_t uiTicks = m_frameTimer.getElapsedTicks();
        fFrameTime = static_cast<float>(hkStopwatch::divide64(uiTicks, hkStopwatch::getTicksPerSecond()) * 1000.0);
        m_frameTimer.stop();
    }
    m_amTimingFrame.m_bool = 1;
    if (__builtin_fabsf(frameTimeInMs) > 1.1920929e-7f) {
        fFrameTime = frameTimeInMs;
    }
    pollForNewClients();
    for (int32_t i = m_clients.m_size - 1; i >= 0; --i) {
        if (m_clients.m_data[i].m_processHandler->isOk()) {
            m_clients.m_data[i].m_processHandler->step(fFrameTime);
            writeStep(i, fFrameTime);
        }
        if (!m_clients.m_data[i].m_processHandler->isOk()) {
            hkError::s_instance->sectionBegin(0x76E3A642, "Client Dies");
            alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
            char acBuffer[0x200];
            hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
            *pStream << "Client has died, cleaning up (host name not available at present)";
            hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\hkVisualDebugger.cpp", 288);
            FNVD_DESTRUCT(hkOstream)(pStream);
            hkError::s_instance->sectionEnd();
            deleteClient(i);
        }
    }
    // The virtual calls above can clear the flag, so it is tested again before restarting the timer.
    if (m_amTimingFrame.m_bool) {
        m_frameTimer.reset();
        m_frameTimer.start();
    }
}

void hkVisualDebugger::pollForNewClients() {
    FNVD_INLINED_LOCAL(hkErrStream, apThis, std::nullopt, std::nullopt);
    if (!s_SuppressPollForNewClients) {
        if ((m_amTimingFrame.m_bool) && (m_frameTimer.m_running_flag.m_bool)) {
            m_frameTimer.m_running_flag.m_bool = 0;
            uint64_t TickCounter = hkStopwatch::getTickCounter();
            m_frameTimer.m_ticks_total += TickCounter - m_frameTimer.m_ticks_at_start;
            m_frameTimer.m_split_total += TickCounter - m_frameTimer.m_ticks_at_split;
            ++m_frameTimer.m_num_timings;
        }

        if (m_server) {
            hkSocket* v3 = m_server->pollForNewClient();
            if (v3) {
                char buf[512];
                ::new (&apThis) hkErrStream(buf, 512);
                apThis.operator<<("A new network client has been received (host name not available at present)");
                hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\hkVisualDebugger.cpp", 232);
                FNVD_DESTRUCT(hkOstream)(&apThis);
                createClient(v3, &v3->m_reader, &v3->m_writer);
            }
        }
        if (m_amTimingFrame.m_bool) {
            m_frameTimer.m_running_flag.m_bool = 1;
            uint64_t v4 = hkStopwatch::getTickCounter();
            m_frameTimer.m_ticks_at_start = v4;
            v4 = static_cast<uint64_t>((v4 & 0xFFFFFFFFull) | (static_cast<uint64_t>(static_cast<uint32_t>(v4)) << 32));
            v4 = static_cast<uint64_t>(
                (v4 & 0xFFFFFFFF00000000ull) |
                (static_cast<uint64_t>(static_cast<uint32_t>(m_frameTimer.m_ticks_at_start >> 32)))
            );
            m_frameTimer.m_ticks_at_split = static_cast<uint64_t>(
                (m_frameTimer.m_ticks_at_split & 0xFFFFFFFF00000000ull) |
                (static_cast<uint64_t>(static_cast<uint32_t>(v4 >> 32)))
            );
            m_frameTimer.m_ticks_at_split = static_cast<uint64_t>(
                (m_frameTimer.m_ticks_at_split & 0xFFFFFFFFull) |
                (static_cast<uint64_t>(static_cast<uint32_t>(v4)) << 32)
            );
        }
    }
}

// Creates the server socket through the platform's factory and listens on the port; one report on every path.
void hkVisualDebugger::serve(int32_t listenPort) {
    FNVD_INLINED_LOCAL(hkErrStream, kStream, std::nullopt, std::nullopt);
    char description[512];
    hkError::s_instance->sectionBegin(0x1293ADE8, "Serving");
    if (m_server) {
        ::new (&kStream) hkErrStream(description, 512);
        kStream << "Server has already been created, only one server allowed per visual debugger instance";
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, description, ".\\hkVisualDebugger.cpp", 112);
    }
    else {
        m_server = hkSocket::create();
        if (m_server) {
            m_server->listen(listenPort);
            ::new (&kStream) hkErrStream(description, 512);
            kStream << "Server created and will poll for new client(s) on port " << listenPort << " every frame";
            hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, description, ".\\hkVisualDebugger.cpp", 103);
        }
        else {
            ::new (&kStream) hkErrStream(description, 512);
            kStream << "Server could not be created, please check that you platform supports sockets with the hkBase "
                       "library";
            hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, description, ".\\hkVisualDebugger.cpp", 107);
        }
    }
    FNVD_DESTRUCT(hkOstream)(&kStream);
    hkError::s_instance->sectionEnd();
}

// Captures to a file: the file system's writer becomes the new client's writer (the client is appended, so the last
// client is the new one), and every default process is created on that client's handler and listed in the report. A
// writer that cannot be opened, or that reports not ok, gets a message instead.
void hkVisualDebugger::capture(const char* captureFilename) {
    hkError::s_instance->sectionBegin(0x1293ADE7, "Capturing");
    hkStreamWriter* pWriter = hkFileSystem::s_instance->openWriter(captureFilename);
    hkBool bOk;
    // Tests the byte isOk's result points at rather than using hkBool's conversion operator.
    if (pWriter && pWriter->isOk(&bOk)->m_bool) {
        alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
        char acBuffer[0x200];
        hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << "Capturing simulation state to '" << captureFilename << "'";
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\hkVisualDebugger.cpp", 125);
        FNVD_DESTRUCT(hkOstream)(pStream);
        createClient(nullptr, nullptr, pWriter);
        pWriter->removeReference();
        hkVisualDebuggerClient* pClient = &m_clients.m_data[m_clients.m_size - 1];
        FNVD_INLINED_LOCAL(hkStringBuf, kViewers, std::nullopt, std::nullopt);
        ::new (&kViewers) hkStringBuf;
        kViewers = "Turning on the following viewers: [";
        for (int32_t i = 0; i < m_defaultProcesses.m_size; ++i) {
            const char* pcName = m_defaultProcesses.m_data[i].cString();
            const int32_t iId = pClient->m_processHandler->getProcessId(pcName);
            if (iId >= 0) {
                kViewers.appendJoin(" ", pcName, nullptr, nullptr, nullptr, nullptr);
                pClient->m_processHandler->createProcess(iId);
            }
        }
        kViewers += "]";
        pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << kViewers.m_string.m_data;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\hkVisualDebugger.cpp", 147);
        FNVD_DESTRUCT(hkOstream)(pStream);
        // EXE-INLINED 0xCBBD40
        hkMemoryAllocator* pTemp = hkMemoryRouter::getInstance().m_temp;
        kViewers.m_string.m_size = 0;
        if (kViewers.m_string.m_capacityAndFlags >= 0) {
            pTemp->bufFree(kViewers.m_string.m_data, kViewers.m_string.m_capacityAndFlags & 0x3FFFFFFF);
        }
    }
    else {
        alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
        char acBuffer[0x200];
        hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << "Capture file '" << captureFilename << "' could not be opened for writing";
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\hkVisualDebugger.cpp", 151);
        FNVD_DESTRUCT(hkOstream)(pStream);
    }
    hkError::s_instance->sectionEnd();
}

// Reads the client count once; each handler's list is inserted at the end through the heap.
void hkVisualDebugger::getCurrentProcesses(hkArray<hkProcess*>& process) const {
    const int32_t iNumClients = m_clients.m_size;
    for (int32_t i = 0; i < iNumClients; ++i) {
        hkServerProcessHandler* pHandler = m_clients.m_data[i].m_processHandler;
        if (pHandler) {
            hkArray<hkProcess*>& rList = pHandler->m_processList;
            const int32_t iCount = rList.m_size;
            hkProcess* const* ppList = rList.m_data;
            const int32_t iIndex = process.m_size;
            process._insertAt(*hkMemoryRouter::getInstance().m_heap, iIndex, ppList, iCount);
        }
    }
}

// indexOf, then removeAt on both arrays (the last element moves into the hole). Keeps indexOf's `index >= 0` test,
// which the search cannot fail, as the game does.
int32_t hkVisualDebugger::removeTrackedObjectCallback(hkVisualDebuggerTrackedObjectCallback callback) {
    const int32_t iSize = m_trackCallbacks.m_size;
    int32_t iIndex = 0;
    if (iSize <= 0) {
        return iIndex;
    }
    for (hkVisualDebuggerTrackedObjectCallback* pCallback = m_trackCallbacks.m_data; *pCallback != callback;
         ++pCallback) {
        if (++iIndex >= iSize) {
            return iIndex;
        }
    }
    if (iIndex >= 0) {
        --m_trackCallbacks.m_size;
        const int32_t iLast = m_trackCallbacks.m_size;
        if (iLast != iIndex) {
            m_trackCallbacks.m_data[iIndex] = m_trackCallbacks.m_data[iLast];
        }
        --m_trackCallbackHandles.m_size;
        const int32_t iLastHandle = m_trackCallbackHandles.m_size;
        if (iLastHandle != iIndex) {
            m_trackCallbackHandles.m_data[iIndex] = m_trackCallbackHandles.m_data[iLastHandle];
        }
    }
    return iIndex;
}

// Untracks obj: removeAt on the tracked objects (the last record moves into the hole), then every callback is told,
// with no class and no tag.
void hkVisualDebugger::removeTrackedObject(void* obj) {
    const int32_t iSize = m_trackedObjects.m_size;
    int32_t iIndex = 0;
    if (iSize <= 0) {
        return;
    }
    for (hkVisualDebuggerTrackedObject* pObject = m_trackedObjects.m_data; pObject->m_ptr != obj; ++pObject) {
        if (++iIndex >= iSize) {
            return;
        }
    }
    const int32_t iLast = --m_trackedObjects.m_size;
    if (iLast != iIndex) {
        m_trackedObjects.m_data[iIndex] = m_trackedObjects.m_data[iLast];
    }
    for (int32_t j = 0; j < m_trackCallbacks.m_size; ++j) {
        const int32_t iAdded = (4 * j) & ~0xFF;
        reinterpret_cast<void (*)(void*, const hkClass*, int32_t, hkUlong, void*)>(m_trackCallbacks.m_data[j])(
            obj, nullptr, iAdded, 0, m_trackCallbackHandles.m_data[j]
        );
    }
}

// Reads the client count once and the client array on every pass (deleteClient moves the last client down).
void hkVisualDebugger::deleteDeadClients() {
    for (int32_t i = m_clients.m_size - 1; i >= 0; --i) {
        if (!m_clients.m_data[i].m_socket) {
            deleteClient(i);
        }
    }
}

// Releases the client's handler, then its socket; removeAt moves the last client into the slot.
void hkVisualDebugger::deleteClient(int32_t i) {
    hkVisualDebuggerClient* pClient = &m_clients.m_data[i];
    if (pClient->m_processHandler) {
        pClient->m_processHandler->removeReference();
    }
    if (pClient->m_socket) {
        pClient->m_socket->removeReference();
    }
    const int32_t iLast = --m_clients.m_size;
    if (iLast != i) {
        m_clients.m_data[i] = m_clients.m_data[iLast];
    }
}

void hkVisualDebugger::createClient(hkSocket* socket, hkStreamReader* reader, hkStreamWriter* writer) {
    if (writer) {
        hkVersionReporter::sendVersionInformation(writer);
    }
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t m_size = m_clients.m_size;
    int32_t v7 = m_clients.m_capacityAndFlags & hkArrayBase<hkVisualDebuggerClient>::CAPACITY_MASK;
    if (v7 < m_size + 1) {
        int32_t v8 = 2 * v7;
        if (m_size + 1 >= v8) {
            v8 = m_size + 1;
        }
        hkArrayUtil::_reserve(*m_heap, &m_clients, v8, sizeof(hkVisualDebuggerClient));
    }
    hkVisualDebuggerClient* m_data = m_clients.begin();
    ++m_clients.m_size;
    hkVisualDebuggerClient* v10 = &m_data[m_size];
    v10->m_socket = socket;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkServerProcessHandler* v12 =
        static_cast<hkServerProcessHandler*>(Instance->m_heap->blockAlloc(sizeof(hkServerProcessHandler)));
    v12->m_memSizeAndFlags = sizeof(hkServerProcessHandler);
    hkServerProcessHandler* v13 = ::new (v12) hkServerProcessHandler(m_contexts, reader, writer);
    v10->m_processHandler = v13;
    v13->registerAllAvailableProcesss();
    for (int32_t i = 0; i < m_defaultProcesses.m_size; ++i) {
        int32_t v15 = v10->m_processHandler->getProcessId(
            reinterpret_cast<const char*>(
                reinterpret_cast<uintptr_t>(m_defaultProcesses.m_data[i].m_stringAndFlag) & ~uintptr_t(1)
            )
        );
        if (v15 >= 0) {
            v10->m_processHandler->selectProcess(v15);
        }
    }
    writeStep(m_clients.m_size - 1, 0.0f);
}

void hkVisualDebugger::writeStep(int32_t i, float t) {
    hkVisualDebuggerClient* v3 = &m_clients.m_data[i];
    if (v3->m_processHandler->isOk()) {
        hkOArchive* m_outStream = v3->m_processHandler->m_outStream;
        m_outStream->write32u(5u);
        m_outStream->write8u(0);
        m_outStream->writeFloat32(t);
        hkStreamWriter* StreamWriter = m_outStream->getStreamWriter();
        StreamWriter->flush();
    }
}

void hkVisualDebugger::SupressPollForNewClient(bool val) {
    s_SuppressPollForNewClients = val;
}

void hkVisualDebugger::shutdown() {
    FNVD_INLINED_LOCAL(hkErrStream, apThis, std::nullopt, std::nullopt);
    char buf[512];
    hkError::s_instance->sectionBegin(0x1293ADEF, "Shuting down Visual Debugger..");
    for (int32_t i = m_clients.m_size - 1; i >= 0; --i) {
        writeStep(i, 0.0f);
        deleteClient(i);
        ::new (&apThis) hkErrStream(buf, 512);
        apThis.operator<<("Client deleted.");
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\hkVisualDebugger.cpp", 81);
        FNVD_DESTRUCT(hkOstream)(&apThis);
    }
    hkSocket* m_server = this->m_server;
    if (m_server) {
        delete m_server;
        this->m_server = nullptr;
        ::new (&apThis) hkErrStream(buf, 512);
        apThis.operator<<("Server deleted.");
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\hkVisualDebugger.cpp", 88);
        FNVD_DESTRUCT(hkOstream)(&apThis);
    }
    hkError::s_instance->sectionEnd();
}

void hkVisualDebugger::addDefaultProcess(const char* viewerName) {
    hkArray<hkStringPtr, hkContainerHeapAllocator>* p_m_defaultProcesses = &m_defaultProcesses;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_defaultProcesses->m_size ==
        (p_m_defaultProcesses->m_capacityAndFlags & hkArrayBase<hkStringPtr>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_defaultProcesses, sizeof(hkStringPtr));
    }
    hkStringPtr* v4 = &p_m_defaultProcesses->m_data[p_m_defaultProcesses->m_size];
    if (v4) {
        ::new (v4) hkStringPtr;
    }
    int32_t m_size = p_m_defaultProcesses->m_size;
    hkStringPtr* v6 = &p_m_defaultProcesses->m_data[m_size];
    p_m_defaultProcesses->m_size = m_size + 1;
    v6->operator=(viewerName);
}

// Tracks obj: appends its record, then tells every callback.
void hkVisualDebugger::addTrackedObject(void* obj, const hkClass& klass, const char* group, hkUlong tagId) {
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    if (m_trackedObjects.m_size ==
        (m_trackedObjects.m_capacityAndFlags & hkArrayBase<hkVisualDebuggerTrackedObject>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*pHeap, &m_trackedObjects, sizeof(hkVisualDebuggerTrackedObject));
    }
    hkVisualDebuggerTrackedObject* pObject = &m_trackedObjects.m_data[m_trackedObjects.m_size++];
    pObject->m_ptr = obj;
    pObject->m_class = &klass;
    pObject->m_tag = tagId;
    for (int32_t i = 0; i < m_trackCallbacks.m_size; ++i) {
        const int32_t iAdded = ((4 * i) & ~0xFF) | 1;
        reinterpret_cast<void (*)(void*, const hkClass*, int32_t, hkUlong, void*)>(m_trackCallbacks.m_data[i])(
            obj, &klass, iAdded, tagId, m_trackCallbackHandles.m_data[i]
        );
    }
}

void hkVisualDebugger::removeDefaultProcess(const char* processName) {
    FNVD_INLINED_LOCAL(hkErrStream, apThis, std::nullopt, std::nullopt);
    int32_t v3 = 0;
    if (m_defaultProcesses.m_size <= 0) {
        char buf[512];
    LABEL_4:;
        hkError::s_instance->sectionBegin(0x76565454, "removeDefaultProcess");
        ::new (&apThis) hkErrStream(buf, 512);
        hkOstream* v4 = &apThis.operator<<("The default Process'");
        hkOstream* v5 = &v4->operator<<(processName);
        v5->operator<<("', cannot not be removed from the default process list as it cannot be found!");
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\hkVisualDebugger.cpp", 318);
        FNVD_DESTRUCT(hkOstream)(&apThis);
        hkError::s_instance->sectionEnd();
    }
    else {
        while (hkString::strCmp(
            reinterpret_cast<const char*>(
                reinterpret_cast<uintptr_t>(m_defaultProcesses.m_data[v3].m_stringAndFlag) & ~uintptr_t(1)
            ),
            processName
        )) {
            if (++v3 >= m_defaultProcesses.m_size) {
                goto LABEL_4;
            }
        }
        m_defaultProcesses.m_data[v3].~hkStringPtr();
        int32_t v6 = --m_defaultProcesses.m_size;
        if (v6 != v3) {
            m_defaultProcesses.m_data[v3].m_stringAndFlag = m_defaultProcesses.m_data[v6].m_stringAndFlag;
        }
    }
}

void hkVisualDebugger::addTrackedObjectCallback(hkVisualDebuggerTrackedObjectCallback callback, void* userHandle) {
    hkArray<hkVisualDebuggerTrackedObjectCallback, hkContainerHeapAllocator>* p_m_trackCallbacks = &m_trackCallbacks;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_trackCallbacks->m_size ==
        (p_m_trackCallbacks->m_capacityAndFlags & hkArrayBase<hkVisualDebuggerTrackedObjectCallback>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(
            *Instance->m_heap, p_m_trackCallbacks, sizeof(void (*&)(void*, const hkClass*, hkBool, uint32_t, void*))
        );
    }
    p_m_trackCallbacks->m_data[p_m_trackCallbacks->m_size++] = callback;
    hkMemoryRouter* v6 = &hkMemoryRouter::getInstance();
    if (m_trackCallbackHandles.m_size ==
        (m_trackCallbackHandles.m_capacityAndFlags & hkArrayBase<void*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*v6->m_heap, &m_trackCallbackHandles, sizeof(void*));
    }
    m_trackCallbackHandles.m_data[m_trackCallbackHandles.m_size++] = userHandle;
}
