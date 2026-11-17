// DirectShow's filter graph and ole32's COM functions for the music the game plays (FalloutAudioMedia::ThreadProc),
// on the layer's output (Output.h). The game creates the graph with CoCreateInstance(CLSID_FilterGraph), adds its own
// source filter (CBSAReader: an async reader whose output pin exposes IAsyncReader) and renders that pin; it then
// drives the graph through IMediaControl, IMediaPosition, IBasicAudio and IMediaEvent. The graph here connects the
// pin to a renderer pin of its own (the pull model: the reader's pin asks it for IAsyncReader), decodes the stream
// (DirectShowDecoder.h: MP3, WAV) on a thread of its own and queues the PCM on an audio::Voice.
#include "platform/dsound/DirectShowDecoder.h"
#include "platform/dsound/Output.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/DirectShow/CAsyncOutputPin.h"
#include "SDK/DirectShow/CBaseFilter.h"
#include "SDK/DirectShow/CMediaType.h"
#include "SDK/DirectShow/control.h"

#if defined(_WIN32)
#include "platform/System.h"
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace {

    // The SDK's interface and class identifiers this file answers to (uuids.h, strmif.h, control.h, unknwn.h).
    constexpr GUID kIidUnknown = {0x00000000, 0x0000, 0x0000, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
    constexpr GUID kIidPin = {0x56A86891, 0x0AD4, 0x11CE, {0xB0, 0x3A, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70}};
    constexpr GUID kIidFilterGraph = {0x56A8689F, 0x0AD4, 0x11CE, {0xB0, 0x3A, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70}};
    constexpr GUID kIidGraphBuilder = {0x56A868A9, 0x0AD4, 0x11CE, {0xB0, 0x3A, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70}};
    constexpr GUID kIidAsyncReader = {0x56A868AA, 0x0AD4, 0x11CE, {0xB0, 0x3A, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70}};
    constexpr GUID kIidMediaEventSink = {0x56A868A2, 0x0AD4, 0x11CE, {0xB0, 0x3A, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70}};
    constexpr GUID kIidMediaControl = {0x56A868B1, 0x0AD4, 0x11CE, {0xB0, 0x3A, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70}};
    constexpr GUID kIidMediaPosition = {0x56A868B2, 0x0AD4, 0x11CE, {0xB0, 0x3A, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70}};
    constexpr GUID kIidBasicAudio = {0x56A868B3, 0x0AD4, 0x11CE, {0xB0, 0x3A, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70}};
    constexpr GUID kIidMediaEvent = {0x56A868B6, 0x0AD4, 0x11CE, {0xB0, 0x3A, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70}};
    constexpr GUID kClsidFilterGraph = {0xE436EBB3, 0x524F, 0x11CE, {0x9F, 0x53, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70}};
    constexpr GUID kMediaTypeStream = {0xE436EB83, 0x524F, 0x11CE, {0x9F, 0x53, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70}};

    // The SDK's codes this file returns (winerror.h, vfwmsgs.h).
    constexpr HRESULT kAbort = static_cast<HRESULT>(0x80004004);              // E_ABORT
    constexpr HRESULT kClassNotRegistered = static_cast<HRESULT>(0x80040154); // REGDB_E_CLASSNOTREG
    constexpr HRESULT kNoAggregation = static_cast<HRESULT>(0x80040110);      // CLASS_E_NOAGGREGATION
    constexpr HRESULT kAlreadyConnected = static_cast<HRESULT>(0x80040204);   // VFW_E_ALREADY_CONNECTED
    constexpr HRESULT kNotConnected = static_cast<HRESULT>(0x80040209);       // VFW_E_NOT_CONNECTED
    constexpr HRESULT kCannotRender = static_cast<HRESULT>(0x80040218);       // VFW_E_CANNOT_RENDER
    constexpr HRESULT kTypeNotAccepted = static_cast<HRESULT>(0x8004022A);    // VFW_E_TYPE_NOT_ACCEPTED
    constexpr HRESULT kNoAudioHardware = static_cast<HRESULT>(0x80040256);    // VFW_E_NO_AUDIO_HARDWARE
    constexpr HRESULT kNoTransport = static_cast<HRESULT>(0x80040266);        // VFW_E_NO_TRANSPORT

    // evcode.h: the stream played to its end.
    constexpr int32_t kEventComplete = 1;
    // IBasicAudio's volume floor (hundredths of a decibel): silence.
    constexpr int32_t kVolumeSilent = -10000;
    // OAFilterState.
    enum class State : int32_t { Stopped = 0, Paused = 1, Running = 2 };

    bool Same(const GUID& a, const GUID& b) {
        return std::memcmp(&a, &b, sizeof(GUID)) == 0;
    }

    // The renderer's input pin: what the reader's output pin connects to. It accepts any MEDIATYPE_Stream type and
    // pulls the bytes through the IAsyncReader it asks the output pin for. The graph owns it; its reference count is
    // its own (the output pin holds one while connected), not the graph's, as a renderer filter's pins are counted on
    // that filter.
    class RendererPin final : public IPin {
    public:
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
            if (!ppv) {
                return E_POINTER;
            }
            if (Same(riid, kIidUnknown) || Same(riid, kIidPin)) {
                *ppv = static_cast<IPin*>(this);
                AddRef();
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        uint32_t STDMETHODCALLTYPE AddRef() override {
            return ++refs;
        }
        uint32_t STDMETHODCALLTYPE Release() override {
            return --refs;
        }

        // The input pin is connected to; it never connects itself.
        HRESULT STDMETHODCALLTYPE Connect(IPin*, const AM_MEDIA_TYPE*) override {
            return E_UNEXPECTED;
        }
        HRESULT STDMETHODCALLTYPE ReceiveConnection(IPin* connector, const AM_MEDIA_TYPE* type) override {
            if (!connector || !type) {
                return E_POINTER;
            }
            if (upstream) {
                return kAlreadyConnected;
            }
            if (!Same(type->majortype, kMediaTypeStream)) {
                return kTypeNotAccepted;
            }
            // The reader's pin completes the connection only once its peer has asked for IAsyncReader.
            IAsyncReader* asked = nullptr;
            if (connector->QueryInterface(kIidAsyncReader, reinterpret_cast<void**>(&asked)) < 0 || !asked) {
                return kNoTransport;
            }
            reader = asked;
            upstream = connector;
            upstream->AddRef();
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE Disconnect() override {
            if (!upstream) {
                return S_FALSE;
            }
            reader->Release();
            reader = nullptr;
            upstream->Release();
            upstream = nullptr;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE ConnectedTo(IPin** pin) override {
            if (!pin) {
                return E_POINTER;
            }
            *pin = upstream;
            if (!upstream) {
                return kNotConnected;
            }
            upstream->AddRef();
            return S_OK;
        }
        // The game's pin never asks for these.
        HRESULT STDMETHODCALLTYPE ConnectionMediaType(AM_MEDIA_TYPE*) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE QueryPinInfo(PIN_INFO*) override {
            return E_NOTIMPL;
        }
        // CBasePin::CheckConnect refuses a peer of its own direction.
        HRESULT STDMETHODCALLTYPE QueryDirection(int32_t* direction) override {
            if (!direction) {
                return E_POINTER;
            }
            *direction = PINDIR_INPUT;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE QueryId(uint16_t**) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE QueryAccept(const AM_MEDIA_TYPE* type) override {
            return type && Same(type->majortype, kMediaTypeStream) ? S_OK : S_FALSE;
        }
        // No types of its own: CBasePin::AgreeMediaType then tries the reader's (its one type).
        HRESULT STDMETHODCALLTYPE EnumMediaTypes(IEnumMediaTypes**) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE QueryInternalConnections(IPin**, uint32_t*) override {
            return E_NOTIMPL;
        }
        // A pull pin gets no pushed stream events.
        HRESULT STDMETHODCALLTYPE EndOfStream() override {
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE BeginFlush() override {
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE EndFlush() override {
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE NewSegment(int64_t, int64_t, double) override {
            return S_OK;
        }

        // The output pin connected to, and its IAsyncReader (each referenced while connected).
        IPin* upstream = nullptr;
        IAsyncReader* reader = nullptr;

    private:
        std::atomic<uint32_t> refs{0};
    };

    // The decoder's bytes: IAsyncReader's Length and SyncRead. SyncRead reports a short read only as S_FALSE, so reads
    // stay within the length.
    class ReaderSource final : public dshow::ByteSource {
    public:
        explicit ReaderSource(IAsyncReader* reader) : reader(reader) {
            int64_t total = 0;
            int64_t available = 0;
            if (reader->Length(&total, &available) >= 0 && total > 0) {
                size = static_cast<uint64_t>(total);
            }
        }
        uint64_t Size() override {
            return size;
        }
        bool ReadAt(uint64_t position, void* out, size_t bytes) override {
            if (position > size || bytes > size - position) {
                return false;
            }
            uint8_t* at = static_cast<uint8_t*>(out);
            while (bytes != 0) {
                const int32_t chunk = static_cast<int32_t>(std::min<size_t>(bytes, kMaxRead));
                if (reader->SyncRead(static_cast<int64_t>(position), chunk, at) != S_OK) {
                    return false;
                }
                position += static_cast<uint64_t>(chunk);
                at += chunk;
                bytes -= static_cast<size_t>(chunk);
            }
            return true;
        }

    private:
        static constexpr size_t kMaxRead = 1u << 20;
        IAsyncReader* reader;
        uint64_t size = 0;
    };

    // The filter graph manager: one source filter, rendered to one voice. Every interface shares the graph's one
    // reference count; the last Release tears the graph down (thread, voice, connection, filter).
    class FilterGraph final : public IGraphBuilder,
                              public IMediaControl,
                              public IMediaEvent,
                              public IMediaPosition,
                              public IBasicAudio,
                              public IMediaEventSink {
    public:
        FilterGraph() = default;
        FilterGraph(const FilterGraph&) = delete;
        FilterGraph& operator=(const FilterGraph&) = delete;

        ~FilterGraph() {
            if (thread.joinable()) {
                {
                    std::lock_guard<std::mutex> hold(lock);
                    quit = true;
                }
                changed.notify_all();
                thread.join();
            }
            audio::DestroyVoice(voice);
            decoder.reset();
            source.reset();
            if (filter) {
                // The output pin disconnects only from a stopped filter.
                filter->Stop();
                if (pin.upstream) {
                    pin.upstream->Disconnect();
                }
                pin.Disconnect();
                filter->JoinFilterGraph(nullptr, nullptr);
                filter->Release();
            }
        }

        // IUnknown, for every interface.
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
            if (!ppv) {
                return E_POINTER;
            }
            void* found = nullptr;
            if (Same(riid, kIidUnknown) || Same(riid, kIidFilterGraph) || Same(riid, kIidGraphBuilder)) {
                found = static_cast<IGraphBuilder*>(this);
            }
            else if (Same(riid, kIidMediaControl)) {
                found = static_cast<IMediaControl*>(this);
            }
            else if (Same(riid, kIidMediaEvent)) {
                found = static_cast<IMediaEvent*>(this);
            }
            else if (Same(riid, kIidMediaPosition)) {
                found = static_cast<IMediaPosition*>(this);
            }
            else if (Same(riid, kIidBasicAudio)) {
                found = static_cast<IBasicAudio*>(this);
            }
            else if (Same(riid, kIidMediaEventSink)) {
                found = static_cast<IMediaEventSink*>(this);
            }
            *ppv = found;
            if (!found) {
                return E_NOINTERFACE;
            }
            AddRef();
            return S_OK;
        }
        uint32_t STDMETHODCALLTYPE AddRef() override {
            return ++refs;
        }
        uint32_t STDMETHODCALLTYPE Release() override {
            const uint32_t left = --refs;
            if (left == 0) {
                delete this;
            }
            return left;
        }

        // IDispatch (of the four automation interfaces): the game never scripts the graph.
        HRESULT STDMETHODCALLTYPE GetTypeInfoCount(uint32_t*) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE GetTypeInfo(uint32_t, uint32_t, ITypeInfo**) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID, uint16_t**, uint32_t, uint32_t, int32_t*) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE
        Invoke(int32_t, REFIID, uint32_t, uint16_t, tagDISPPARAMS*, tagVARIANT*, tagEXCEPINFO*, uint32_t*) override {
            return E_NOTIMPL;
        }

        // IFilterGraph. The game adds its one source filter; the graph holds it and tells it which graph it joined.
        HRESULT STDMETHODCALLTYPE AddFilter(IBaseFilter* added, const uint16_t* name) override {
            if (!added) {
                return E_POINTER;
            }
            // One source is all this graph renders (the game adds one filter per graph).
            if (filter) {
                return E_FAIL;
            }
            HRESULT hr = added->JoinFilterGraph(static_cast<IFilterGraph*>(this), name);
            if (hr < 0) {
                return hr;
            }
            filter = added;
            filter->AddRef();
            return S_OK;
        }
        // The rest of IFilterGraph and IGraphBuilder: the game never calls them.
        HRESULT STDMETHODCALLTYPE RemoveFilter(IBaseFilter*) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE EnumFilters(IEnumFilters**) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE FindFilterByName(const uint16_t*, IBaseFilter**) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE ConnectDirect(IPin*, IPin*, const AM_MEDIA_TYPE*) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE Reconnect(IPin*) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE Disconnect(IPin*) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE SetDefaultSyncSource() override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE Connect(IPin*, IPin*) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE RenderFile(const uint16_t*, const uint16_t*) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE AddSourceFilter(const uint16_t*, const uint16_t*, IBaseFilter**) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE SetLogFile(uintptr_t) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE Abort() override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE ShouldOperationContinue() override {
            return E_NOTIMPL;
        }

        // IGraphBuilder::Render: connects the source's output pin to the renderer pin, opens the decoder over the
        // pin's IAsyncReader and the voice for its PCM, and starts the decoding thread. The graph is left stopped.
        HRESULT STDMETHODCALLTYPE Render(IPin* output) override {
            if (!output) {
                return E_POINTER;
            }
            if (voice) {
                return kAlreadyConnected;
            }
            HRESULT hr = output->Connect(&pin, nullptr);
            if (hr < 0) {
                return hr;
            }
            source = std::make_unique<ReaderSource>(pin.reader);
            decoder = dshow::OpenDecoder(*source);
            if (!decoder) {
                Unrender(output);
                return kCannotRender;
            }
            voice = audio::CreateVoice(decoder->Rate(), decoder->Channels(), 16);
            if (!voice) {
                Unrender(output);
                return kNoAudioHardware;
            }
            frames = decoder->Frames();
            ApplyVolume();
            thread = std::thread(&FilterGraph::Decode, this);
            return S_OK;
        }

        // IMediaControl: the filter's state follows the graph's, as the graph manager sets it; the voice plays while
        // running. Stop drops what was decoded ahead and resumes from the current position on the next Run.
        HRESULT STDMETHODCALLTYPE Run() override {
            if (filter) {
                filter->Run(0);
            }
            std::lock_guard<std::mutex> hold(lock);
            state = State::Running;
            if (voice) {
                audio::SetPaused(voice, false);
            }
            changed.notify_all();
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE Pause() override {
            if (filter) {
                filter->Pause();
            }
            std::lock_guard<std::mutex> hold(lock);
            state = State::Paused;
            if (voice) {
                audio::SetPaused(voice, true);
            }
            changed.notify_all();
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE Stop() override {
            if (filter) {
                filter->Stop();
            }
            std::lock_guard<std::mutex> hold(lock);
            if (voice && state != State::Stopped) {
                audio::SetPaused(voice, true);
                SeekLocked(PositionLocked());
            }
            state = State::Stopped;
            changed.notify_all();
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GetState(int32_t, int32_t* filterState) override {
            if (!filterState) {
                return E_POINTER;
            }
            std::lock_guard<std::mutex> hold(lock);
            *filterState = static_cast<int32_t>(state);
            return S_OK;
        }
        // The rest of IMediaControl: the game never calls them.
        HRESULT STDMETHODCALLTYPE RenderFile(uint16_t*) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE AddSourceFilter(uint16_t*, IDispatch**) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE get_FilterCollection(IDispatch**) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE get_RegFilterCollection(IDispatch**) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE StopWhenReady() override {
            return E_NOTIMPL;
        }

        // IMediaEvent: the queue the graph and its filter post to. GetEvent waits up to msTimeout (the game polls with
        // 0) and fails with E_ABORT when nothing came.
        HRESULT STDMETHODCALLTYPE GetEventHandle(intptr_t*) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE
        GetEvent(int32_t* code, intptr_t* param1, intptr_t* param2, int32_t msTimeout) override {
            if (!code || !param1 || !param2) {
                return E_POINTER;
            }
            std::unique_lock<std::mutex> hold(lock);
            WaitLocked(hold, msTimeout, [this] { return !events.empty(); });
            if (events.empty()) {
                return kAbort;
            }
            *code = events.front().code;
            *param1 = events.front().param1;
            *param2 = events.front().param2;
            events.pop_front();
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE WaitForCompletion(int32_t msTimeout, int32_t* code) override {
            if (!code) {
                return E_POINTER;
            }
            std::unique_lock<std::mutex> hold(lock);
            WaitLocked(hold, msTimeout, [this] { return completed; });
            *code = completed ? kEventComplete : 0;
            return completed ? S_OK : kAbort;
        }
        HRESULT STDMETHODCALLTYPE CancelDefaultHandling(int32_t) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE RestoreDefaultHandling(int32_t) override {
            return E_NOTIMPL;
        }
        // The events posted here carry no allocated parameters.
        HRESULT STDMETHODCALLTYPE FreeEventParams(int32_t, intptr_t, intptr_t) override {
            return S_OK;
        }

        // IMediaEventSink: the filter's events (CBaseFilter::NotifyEvent) join the queue.
        HRESULT STDMETHODCALLTYPE Notify(int32_t code, intptr_t param1, intptr_t param2) override {
            std::lock_guard<std::mutex> hold(lock);
            events.push_back({code, param1, param2});
            changed.notify_all();
            return S_OK;
        }

        // IMediaPosition, in seconds.
        HRESULT STDMETHODCALLTYPE get_Duration(double* length) override {
            if (!length) {
                return E_POINTER;
            }
            if (!decoder) {
                return E_UNEXPECTED;
            }
            *length = static_cast<double>(frames) / decoder->Rate();
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE put_CurrentPosition(double time) override {
            if (!decoder) {
                return E_UNEXPECTED;
            }
            const double frame = std::max(0.0, time) * decoder->Rate();
            std::lock_guard<std::mutex> hold(lock);
            SeekLocked(std::min(static_cast<uint64_t>(frame), frames));
            changed.notify_all();
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_CurrentPosition(double* time) override {
            if (!time) {
                return E_POINTER;
            }
            if (!decoder) {
                return E_UNEXPECTED;
            }
            std::lock_guard<std::mutex> hold(lock);
            *time = static_cast<double>(PositionLocked()) / decoder->Rate();
            return S_OK;
        }
        // The rest of IMediaPosition: the game never calls them.
        HRESULT STDMETHODCALLTYPE get_StopTime(double*) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE put_StopTime(double) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE get_PrerollTime(double*) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE put_PrerollTime(double) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE put_Rate(double) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE get_Rate(double*) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE CanSeekForward(int32_t*) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE CanSeekBackward(int32_t*) override {
            return E_NOTIMPL;
        }

        // IBasicAudio: the volume in hundredths of a decibel, -10000 (silence) to 0.
        HRESULT STDMETHODCALLTYPE put_Volume(int32_t value) override {
            if (value < kVolumeSilent || value > 0) {
                return E_INVALIDARG;
            }
            std::lock_guard<std::mutex> hold(lock);
            volume = value;
            ApplyVolume();
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE get_Volume(int32_t* value) override {
            if (!value) {
                return E_POINTER;
            }
            std::lock_guard<std::mutex> hold(lock);
            *value = volume;
            return S_OK;
        }
        // The game never sets the balance.
        HRESULT STDMETHODCALLTYPE put_Balance(int32_t) override {
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE get_Balance(int32_t*) override {
            return E_NOTIMPL;
        }

    private:
        struct Event {
            int32_t code;
            intptr_t param1;
            intptr_t param2;
        };

        // Frames decoded ahead of the device: a quarter of a second.
        static constexpr uint32_t kAheadDivisor = 4;
        // Frames decoded per step.
        static constexpr size_t kStepFrames = 2048;

        // Undoes a connection Render made when it cannot render the stream.
        void Unrender(IPin* output) {
            decoder.reset();
            source.reset();
            output->Disconnect();
            pin.Disconnect();
        }

        void ApplyVolume() {
            if (!voice) {
                return;
            }
            const float gain = volume <= kVolumeSilent
                ? 0.0f
                : static_cast<float>(std::pow(10.0, static_cast<double>(volume) / 2000.0));
            audio::SetGains(voice, gain, gain);
        }

        // The frame playing now: where the stream was last placed, advanced by what the device played since.
        uint64_t PositionLocked() const {
            const uint64_t played = audio::PlayedFrames(voice) - playedAtBase;
            return std::min(base + played, frames);
        }

        // Places the stream at `frame`: what was queued is dropped, decoding restarts there (on the decoding thread)
        // and the stream may complete again.
        void SeekLocked(uint64_t frame) {
            ++generation;
            audio::Clear(voice);
            playedAtBase = audio::PlayedFrames(voice);
            base = frame;
            seekTo = frame;
            seekPending = true;
            ended = false;
            completed = false;
        }

        template <class Ready>
        void WaitLocked(std::unique_lock<std::mutex>& hold, int32_t msTimeout, Ready ready) {
            // A negative timeout is INFINITE.
            if (msTimeout < 0) {
                changed.wait(hold, ready);
            }
            else if (msTimeout > 0) {
                changed.wait_for(hold, std::chrono::milliseconds(msTimeout), ready);
            }
        }

        // The decoding thread: keeps the voice's queue a quarter of a second ahead, carries out seeks, and posts
        // EC_COMPLETE once the stream ended and the device played all of it while running.
        void Decode() {
            const uint32_t channels = decoder->Channels();
            const size_t aheadBytes = static_cast<size_t>(decoder->Rate() / kAheadDivisor) * channels * sizeof(int16_t);
            std::vector<int16_t> pcm(kStepFrames * channels);
            std::unique_lock<std::mutex> hold(lock);
            while (!quit) {
                if (seekPending) {
                    seekPending = false;
                    const uint64_t frame = seekTo;
                    hold.unlock();
                    const bool placed = decoder->Seek(frame);
                    hold.lock();
                    if (!placed && !seekPending) {
                        ended = true;
                    }
                    continue;
                }
                if (!ended && audio::QueuedBytes(voice) < aheadBytes) {
                    const uint32_t decodedFor = generation;
                    hold.unlock();
                    const size_t got = decoder->Read(pcm.data(), kStepFrames);
                    hold.lock();
                    // A seek since the read made this data stale; the seek is carried out next.
                    if (decodedFor != generation) {
                        continue;
                    }
                    if (got == 0) {
                        ended = true;
                    }
                    else {
                        audio::Queue(voice, pcm.data(), got * channels * sizeof(int16_t));
                    }
                    continue;
                }
                if (ended && !completed && state == State::Running && audio::QueuedBytes(voice) == 0) {
                    completed = true;
                    events.push_back({kEventComplete, 0, 0});
                    changed.notify_all();
                    continue;
                }
                // While running the device drains the queue: look again soon. Otherwise only a call changes anything.
                if (state == State::Running && !completed) {
                    changed.wait_for(hold, std::chrono::milliseconds(10));
                }
                else {
                    changed.wait(hold);
                }
            }
        }

        std::atomic<uint32_t> refs{0};
        IBaseFilter* filter = nullptr;
        RendererPin pin;
        std::unique_ptr<ReaderSource> source;
        std::unique_ptr<dshow::Decoder> decoder;
        audio::Voice* voice = nullptr;
        // The stream's length in frames (set by Render).
        uint64_t frames = 0;
        std::thread thread;

        // Under `lock`: the state, the position bookkeeping, the decoding thread's orders and the event queue.
        std::mutex lock;
        std::condition_variable changed;
        State state = State::Stopped;
        int32_t volume = 0;
        bool quit = false;
        // The stream frame at which the voice had played playedAtBase frames.
        uint64_t base = 0;
        uint64_t playedAtBase = 0;
        // Bumped by every seek, so that data decoded before it is not queued after it.
        uint32_t generation = 0;
        bool seekPending = false;
        uint64_t seekTo = 0;
        // The decoder reached the end; EC_COMPLETE was posted for it.
        bool ended = false;
        bool completed = false;
        std::deque<Event> events;
    };

    // The layer's CoCreateInstance: DirectShow's filter graph manager (CLSID_FilterGraph, without aggregation) is made
    // here. Any other class is ole32's on Windows, where SDL's own audio back ends create theirs through the same
    // import; on Linux no other class is registered (the game creates no other).
    HRESULT WINAPI CreateInstance(const GUID* rclsid, IUnknown* outer, uint32_t context, const GUID* riid, void** ppv) {
        if (rclsid && Same(*rclsid, kClsidFilterGraph)) {
            if (!ppv) {
                return E_POINTER;
            }
            *ppv = nullptr;
            if (!riid) {
                return E_INVALIDARG;
            }
            if (outer) {
                return kNoAggregation;
            }
            FilterGraph* graph = new FilterGraph;
            graph->AddRef();
            const HRESULT hr = graph->QueryInterface(*riid, ppv);
            graph->Release();
            return hr;
        }
#if defined(_WIN32)
        static const CoCreateInstance_t ole32 = reinterpret_cast<CoCreateInstance_t>(
            platform::LibrarySymbol(platform::LibraryLoad("ole32.dll"), "CoCreateInstance")
        );
        if (ole32) {
            return ole32(rclsid, outer, context, riid, ppv);
        }
#else
        (void)context;
#endif
        if (ppv) {
            *ppv = nullptr;
        }
        return kClassNotRegistered;
    }

} // namespace

// The exports the game's imports name. On Linux, which has no ole32, the C functions themselves. On Windows
// (FNVD_SDL_AUDIO) only CoCreateInstance's import slot `__imp_`, which the game's dllimport calls go through, so
// ole32.lib (which the program links for the rest of COM) never supplies it.
#if defined(_WIN32)
#if defined(_M_IX86)
extern "C" {
decltype(&CreateInstance) fnvd_imp_CoCreateInstance __asm__("__imp__CoCreateInstance@20") = &CreateInstance;
}
#else
extern "C" {
decltype(&CreateInstance) fnvd_imp_CoCreateInstance __asm__("__imp_CoCreateInstance") = &CreateInstance;
}
#endif
#else
extern "C" {

// COM has no apartments here: the one class (the filter graph) is free-threaded, so there is nothing to set up.
int32_t WINAPI CoInitialize(void*) {
    return S_OK;
}

// As CoInitialize.
int32_t WINAPI CoInitializeEx(void*, uint32_t) {
    return S_OK;
}

// Nothing was set up, so nothing is torn down.
void WINAPI CoUninitialize() {}

// No COM server library is ever loaded, so none is freed.
void WINAPI CoFreeUnusedLibraries() {}

// COM's task allocator: the C heap, as ole32's is a heap of its own (callers only pair it with CoTaskMemFree).
void* WINAPI CoTaskMemAlloc(size_t cb) {
    return std::malloc(cb);
}

void WINAPI CoTaskMemFree(void* pv) {
    std::free(pv);
}

int32_t WINAPI
CoCreateInstance(const GUID* rclsid, IUnknown* pUnkOuter, uint32_t dwClsContext, const GUID* riid, void** ppv) {
    return CreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}

} // extern "C"
#endif
