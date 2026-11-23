// DirectSound 8 on the audio layer's output (Output.h): the device, the primary buffer with its 3D listener, and the
// secondary buffers with their 3D and effect interfaces.
//
// A secondary buffer keeps its bytes as DirectSound does (the game writes them with Lock and Unlock, static or as a
// ring it refills behind the play cursor) and is one voice, which pulls the bytes from the buffer's read position on
// the device's thread when it needs them. The play cursor is the buffer position of the frames the device has
// consumed; the write cursor is where the voice will read next.
//
// The 3D listener and 3D buffer settings follow DirectSound's immediate and deferred rules; a 3D buffer's gains and
// rate are DirectSound3D.cpp's model of them, computed again whenever a setting takes effect (an immediate one, or
// CommitDeferredSettings, which the game calls each frame). The effects (SetFX, GetObjectInPath) are recorded and
// change nothing.
#include "platform/dsound/DirectSound.h"
#include "platform/dsound/Output.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <vector>

namespace audio::ds {

    namespace {

        // The SDK's interface and class GUIDs the game passes (dsound.h).
        constexpr GUID IID_IUnknown{0x00000000, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
        constexpr GUID IID_IDirectSound8{0xC50A7E93, 0xF395, 0x4834, {0x9E, 0xF6, 0x7F, 0xA9, 0x9D, 0xE5, 0x09, 0x66}};
        constexpr GUID
            IID_IDirectSoundBuffer{0x279AFA85, 0x4981, 0x11CE, {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}};
        constexpr GUID
            IID_IDirectSoundBuffer8{0x6825A449, 0x7524, 0x4D82, {0x92, 0x0F, 0x50, 0xE3, 0x6A, 0xB3, 0xAB, 0x1E}};
        constexpr GUID
            IID_IDirectSound3DListener{0x279AFA84, 0x4981, 0x11CE, {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}};
        constexpr GUID
            IID_IDirectSound3DBuffer{0x279AFA86, 0x4981, 0x11CE, {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}};
        constexpr GUID
            IID_IDirectSoundFXDistortion{0x8ECF4326, 0x455F, 0x4D8B, {0xBD, 0xA9, 0x8D, 0x5D, 0x3E, 0x9E, 0x3E, 0x0B}};
        constexpr GUID
            IID_IDirectSoundFXI3DL2Reverb{0x4B166A6A, 0x0D66, 0x43F3, {0x80, 0xE3, 0xEE, 0x62, 0x80, 0xDE, 0xE1, 0xA4}};
        constexpr GUID
            GUID_DSFX_STANDARD_DISTORTION{0xEF114C90, 0xCD1D, 0x484E, {0x96, 0xE5, 0x09, 0xCF, 0xAF, 0x91, 0x2A, 0x21}};
        constexpr GUID GUID_DSFX_STANDARD_I3DL2REVERB{
            0xEF985E71,
            0xD5C7,
            0x42D4,
            {0xBA, 0x4D, 0x2D, 0x07, 0x3E, 0x2E, 0x96, 0xF4}
        };
        // DSDEVID_DefaultPlayback: the one device DirectSoundCreate8 opens besides the null GUID.
        constexpr GUID
            DSDEVID_DefaultPlayback{0xDEF00000, 0x9C6D, 0x47ED, {0xAA, 0xF1, 0x4D, 0xDA, 0x8F, 0x2B, 0x5C, 0x03}};

        // The listener's settings: current (what plays) and deferred (set with DS3D_DEFERRED, current at the next
        // CommitDeferredSettings).
        std::mutex gListenerLock;
        ListenerParams gListener;
        ListenerParams gListenerDeferred;

        // Every secondary buffer, for CommitDeferredSettings (which commits the 3D buffers' deferred settings too).
        class SecondaryBuffer;
        std::mutex gBuffersLock;
        std::vector<SecondaryBuffer*> gBuffers;

        // A volume in hundredths of a decibel (DSBVOLUME_MIN to DSBVOLUME_MAX) as a linear gain; the minimum is
        // silence.
        float Attenuation(int32_t hundredthsDb) {
            if (hundredthsDb <= DSBVOLUME_MIN) {
                return 0.0f;
            }
            return std::pow(10.0f, static_cast<float>(hundredthsDb) / 2000.0f);
        }

        // An interface whose count of references is its own, of an object that lives while any of its interfaces has
        // one (as DirectSound's buffers count each interface apart: the game releases the buffer interface to zero
        // before its 3D interface).
        template <class Owner>
        struct InterfaceCount {
            std::atomic<uint32_t> count{0};

            uint32_t AddRef(Owner* owner) {
                owner->live.fetch_add(1);
                return count.fetch_add(1) + 1;
            }

            uint32_t Release(Owner* owner) {
                uint32_t old = count.load();
                while (old != 0 && !count.compare_exchange_weak(old, old - 1)) {}
                if (old == 0) {
                    return 0;
                }
                if (owner->live.fetch_sub(1) == 1) {
                    delete owner;
                }
                return old - 1;
            }
        };

        // --- Effects
        // -------------------------------------------------------------------------------------------------------

        // An effect object GetObjectInPath hands out, with its own count of references (the game never releases them,
        // as a DMO outlives the buffer when the program holds it). Its settings are recorded.
        class DistortionEffect final : public IDirectSoundFXDistortion {
        public:
            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
                if (!ppv) {
                    return E_POINTER;
                }
                if (SameGuid(riid, IID_IUnknown) || SameGuid(riid, IID_IDirectSoundFXDistortion)) {
                    AddRef();
                    *ppv = this;
                    return S_OK;
                }
                *ppv = nullptr;
                return E_NOINTERFACE;
            }
            uint32_t STDMETHODCALLTYPE AddRef() override {
                return refs.fetch_add(1) + 1;
            }
            uint32_t STDMETHODCALLTYPE Release() override {
                const uint32_t left = refs.fetch_sub(1) - 1;
                if (left == 0) {
                    delete this;
                }
                return left;
            }
            HRESULT STDMETHODCALLTYPE SetAllParameters(const DSFXDistortion* params) override {
                if (!params) {
                    return E_POINTER;
                }
                settings = *params;
                return S_OK;
            }
            HRESULT STDMETHODCALLTYPE GetAllParameters(DSFXDistortion*) override {
                return E_NOTIMPL;
            }

        private:
            std::atomic<uint32_t> refs{1};
            DSFXDistortion settings{-18.0f, 15.0f, 2400.0f, 2400.0f, 8000.0f};
        };

        class ReverbEffect final : public IDirectSoundFXI3DL2Reverb {
        public:
            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
                if (!ppv) {
                    return E_POINTER;
                }
                if (SameGuid(riid, IID_IUnknown) || SameGuid(riid, IID_IDirectSoundFXI3DL2Reverb)) {
                    AddRef();
                    *ppv = this;
                    return S_OK;
                }
                *ppv = nullptr;
                return E_NOINTERFACE;
            }
            uint32_t STDMETHODCALLTYPE AddRef() override {
                return refs.fetch_add(1) + 1;
            }
            uint32_t STDMETHODCALLTYPE Release() override {
                const uint32_t left = refs.fetch_sub(1) - 1;
                if (left == 0) {
                    delete this;
                }
                return left;
            }
            HRESULT STDMETHODCALLTYPE SetAllParameters(const _DSFXI3DL2Reverb*) override {
                return E_NOTIMPL;
            }
            HRESULT STDMETHODCALLTYPE GetAllParameters(_DSFXI3DL2Reverb*) override {
                return E_NOTIMPL;
            }
            // The I3DL2 presets run from DSFX_I3DL2_ENVIRONMENT_PRESET_DEFAULT (0) to ..._PLATE (29).
            HRESULT STDMETHODCALLTYPE SetPreset(uint32_t value) override {
                if (value > 29) {
                    return E_INVALIDARG;
                }
                preset = value;
                return S_OK;
            }
            HRESULT STDMETHODCALLTYPE GetPreset(uint32_t*) override {
                return E_NOTIMPL;
            }
            HRESULT STDMETHODCALLTYPE SetQuality(int32_t) override {
                return E_NOTIMPL;
            }
            HRESULT STDMETHODCALLTYPE GetQuality(int32_t*) override {
                return E_NOTIMPL;
            }

        private:
            std::atomic<uint32_t> refs{1};
            uint32_t preset = 0;
        };

        // --- Secondary buffers
        // ---------------------------------------------------------------------------------------------

        class SecondaryBuffer {
        public:
            static HRESULT Create(const DSBUFFERDESC& desc, IDirectSoundBuffer** out);

            // The interfaces: the buffer (IDirectSoundBuffer8, which is IDirectSoundBuffer too) and its 3D one.
            class BufferInterface final : public IDirectSoundBuffer8 {
            public:
                explicit BufferInterface(SecondaryBuffer* owner) : self(owner) {}
                HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
                    return self->QueryInterface(riid, ppv);
                }
                uint32_t STDMETHODCALLTYPE AddRef() override {
                    return self->bufferRefs.AddRef(self);
                }
                uint32_t STDMETHODCALLTYPE Release() override {
                    return self->bufferRefs.Release(self);
                }
                HRESULT STDMETHODCALLTYPE GetCaps(DSBCAPS*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetCurrentPosition(uint32_t* play, uint32_t* write) override {
                    return self->GetCurrentPosition(play, write);
                }
                HRESULT STDMETHODCALLTYPE GetFormat(WAVEFORMATEX* format, uint32_t size, uint32_t* written) override {
                    return self->GetFormat(format, size, written);
                }
                HRESULT STDMETHODCALLTYPE GetVolume(int32_t*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetPan(int32_t*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetFrequency(uint32_t* frequency) override {
                    return self->GetFrequency(frequency);
                }
                HRESULT STDMETHODCALLTYPE GetStatus(uint32_t* status) override {
                    return self->GetStatus(status);
                }
                HRESULT STDMETHODCALLTYPE Initialize(IDirectSound*, const DSBUFFERDESC*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE Lock(
                    uint32_t offset,
                    uint32_t bytes,
                    void** audio1,
                    uint32_t* bytes1,
                    void** audio2,
                    uint32_t* bytes2,
                    uint32_t flags
                ) override {
                    return self->Lock(offset, bytes, audio1, bytes1, audio2, bytes2, flags);
                }
                HRESULT STDMETHODCALLTYPE Play(uint32_t, uint32_t, uint32_t flags) override {
                    return self->Play(flags);
                }
                HRESULT STDMETHODCALLTYPE SetCurrentPosition(uint32_t position) override {
                    return self->SetCurrentPosition(position);
                }
                HRESULT STDMETHODCALLTYPE SetFormat(const WAVEFORMATEX*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE SetVolume(int32_t volume) override {
                    return self->SetVolume(volume);
                }
                HRESULT STDMETHODCALLTYPE SetPan(int32_t pan) override {
                    return self->SetPan(pan);
                }
                HRESULT STDMETHODCALLTYPE SetFrequency(uint32_t frequency) override {
                    return self->SetFrequency(frequency);
                }
                HRESULT STDMETHODCALLTYPE Stop() override {
                    return self->Stop();
                }
                // The bytes stay where Lock pointed: nothing to copy.
                HRESULT STDMETHODCALLTYPE Unlock(void*, uint32_t, void*, uint32_t) override {
                    return DS_OK;
                }
                HRESULT STDMETHODCALLTYPE Restore() override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE SetFX(uint32_t count, DSEFFECTDESC* effects, uint32_t* results) override {
                    return self->SetFX(count, effects, results);
                }
                HRESULT STDMETHODCALLTYPE AcquireResources(uint32_t, uint32_t, uint32_t*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE
                GetObjectInPath(const GUID* object, uint32_t index, const GUID* iid, void** ppv) override {
                    return self->GetObjectInPath(object, index, iid, ppv);
                }

            private:
                SecondaryBuffer* self;
            };

            class Buffer3DInterface final : public IDirectSound3DBuffer {
            public:
                explicit Buffer3DInterface(SecondaryBuffer* owner) : self(owner) {}
                HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
                    return self->QueryInterface(riid, ppv);
                }
                uint32_t STDMETHODCALLTYPE AddRef() override {
                    return self->refs3D.AddRef(self);
                }
                uint32_t STDMETHODCALLTYPE Release() override {
                    return self->refs3D.Release(self);
                }
                HRESULT STDMETHODCALLTYPE GetAllParameters(DS3DBUFFER*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetConeAngles(uint32_t*, uint32_t*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetConeOrientation(D3DVECTOR*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetConeOutsideVolume(int32_t*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetMaxDistance(float*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetMinDistance(float*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetMode(uint32_t*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetPosition(D3DVECTOR*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetVelocity(D3DVECTOR*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE SetAllParameters(const DS3DBUFFER*, uint32_t) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE SetConeAngles(uint32_t, uint32_t, uint32_t) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE SetConeOrientation(float, float, float, uint32_t) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE SetConeOutsideVolume(int32_t, uint32_t) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE SetMaxDistance(float distance, uint32_t apply) override {
                    if (!(distance > 0.0f)) {
                        return DSERR_INVALIDPARAM;
                    }
                    return self->Set3D(apply, [distance](SourceParams& p) { p.maxDistance = distance; });
                }
                HRESULT STDMETHODCALLTYPE SetMinDistance(float distance, uint32_t apply) override {
                    if (!(distance > 0.0f)) {
                        return DSERR_INVALIDPARAM;
                    }
                    return self->Set3D(apply, [distance](SourceParams& p) { p.minDistance = distance; });
                }
                HRESULT STDMETHODCALLTYPE SetMode(uint32_t mode, uint32_t apply) override {
                    if (mode > DS3DMODE_DISABLE) {
                        return DSERR_INVALIDPARAM;
                    }
                    return self->Set3D(apply, [mode](SourceParams& p) { p.mode = mode; });
                }
                HRESULT STDMETHODCALLTYPE SetPosition(float x, float y, float z, uint32_t apply) override {
                    return self->Set3D(apply, [x, y, z](SourceParams& p) { p.position = D3DVECTOR{x, y, z}; });
                }
                HRESULT STDMETHODCALLTYPE SetVelocity(float x, float y, float z, uint32_t apply) override {
                    return self->Set3D(apply, [x, y, z](SourceParams& p) { p.velocity = D3DVECTOR{x, y, z}; });
                }

            private:
                SecondaryBuffer* self;
            };

            // The deferred 3D settings become current (the listener's CommitDeferredSettings).
            void CommitDeferred();
            // The listener's settings changed now: a 3D buffer's sound follows.
            void ListenerChanged();

        private:
            friend struct InterfaceCount<SecondaryBuffer>;

            SecondaryBuffer(const DSBUFFERDESC& desc, Voice* voice);
            ~SecondaryBuffer();

            HRESULT QueryInterface(REFIID riid, void** ppv);
            HRESULT GetCurrentPosition(uint32_t* play, uint32_t* write);
            HRESULT GetFormat(WAVEFORMATEX* format, uint32_t size, uint32_t* written);
            HRESULT GetFrequency(uint32_t* frequency);
            HRESULT GetStatus(uint32_t* status);
            HRESULT Lock(
                uint32_t offset,
                uint32_t bytes,
                void** audio1,
                uint32_t* bytes1,
                void** audio2,
                uint32_t* bytes2,
                uint32_t flags
            );
            HRESULT Play(uint32_t flags);
            HRESULT SetCurrentPosition(uint32_t position);
            HRESULT SetVolume(int32_t volume);
            HRESULT SetPan(int32_t pan);
            HRESULT SetFrequency(uint32_t frequency);
            HRESULT Stop();
            HRESULT SetFX(uint32_t count, DSEFFECTDESC* effects, uint32_t* results);
            HRESULT GetObjectInPath(const GUID* object, uint32_t index, const GUID* iid, void** ppv);
            template <class Change>
            HRESULT Set3D(uint32_t apply, Change change);

            // The voice's source (device thread, voice locked): the buffer's bytes from the read position.
            static size_t Pull(void* user, void* out, size_t bytes);
            // Under the voice's lock: the play and write cursors.
            uint32_t PlayCursor();
            uint32_t WriteCursor();
            // Under the voice's lock: restarts the cursors' count at `position`, dropping what the voice had taken.
            void Rebase(uint32_t position);
            // Under apiLock: a buffer played once that the device has played to its end stops, at position 0.
            void EndIfPlayed();
            void ApplyGains();
            void ReleaseEffects();

            InterfaceCount<SecondaryBuffer> bufferRefs;
            InterfaceCount<SecondaryBuffer> refs3D;
            std::atomic<uint32_t> live{0};
            BufferInterface bufferInterface{this};
            Buffer3DInterface buffer3DInterface{this};

            // Serialises the game's calls on the buffer (they come from the audio and main threads); taken before the
            // voice's lock, never under it, so that pausing the voice (the device's lock) is never under the voice's
            // lock.
            std::mutex apiLock;
            Voice* voice;
            WAVEFORMATEX format;
            uint32_t flags;
            std::vector<uint8_t> data;

            // The playback state, under the voice's lock (Pull reads it).
            bool playing = false;
            bool looping = false;
            // Pull reached the end of a buffer played once; endFrame is the voice's frame count there.
            bool atEnd = false;
            uint64_t endFrame = 0;
            // The next byte Pull reads.
            uint32_t readPosition = 0;
            // The play cursor was basePosition when the voice had played basePlayed frames.
            uint32_t basePosition = 0;
            uint64_t basePlayed = 0;

            int32_t volume = DSBVOLUME_MAX;
            int32_t pan = 0;
            uint32_t frequency;

            SourceParams source;
            SourceParams sourceDeferred;

            // The effects SetFX set (one reference each) and their class GUIDs.
            struct Effect {
                GUID guid;
                IUnknown* object;
            };
            std::vector<Effect> effects;
        };

        HRESULT SecondaryBuffer::Create(const DSBUFFERDESC& desc, IDirectSoundBuffer** out) {
            const WAVEFORMATEX* wfx = desc.lpwfxFormat;
            if (!wfx || desc.dwBufferBytes < DSBSIZE_MIN || desc.dwBufferBytes > DSBSIZE_MAX) {
                return DSERR_INVALIDPARAM;
            }
            if (wfx->wFormatTag != WAVE_FORMAT_PCM ||
                wfx->nChannels == 0 ||
                wfx->nChannels > 8 ||
                (wfx->wBitsPerSample != 8 && wfx->wBitsPerSample != 16) ||
                wfx->nSamplesPerSec < DSBFREQUENCY_MIN ||
                wfx->nSamplesPerSec > DSBFREQUENCY_MAX ||
                wfx->nBlockAlign != wfx->nChannels * wfx->wBitsPerSample / 8) {
                return DSERR_BADFORMAT;
            }
            // A 3D buffer is mono.
            if ((desc.dwFlags & DSBCAPS_CTRL3D) && wfx->nChannels != 1) {
                return DSERR_INVALIDPARAM;
            }
            // Effects need a buffer of DSBSIZE_FX_MIN milliseconds.
            if ((desc.dwFlags & DSBCAPS_CTRLFX) &&
                static_cast<uint64_t>(desc.dwBufferBytes) * 1000 <
                    static_cast<uint64_t>(wfx->nAvgBytesPerSec) * DSBSIZE_FX_MIN) {
                return DSERR_BUFFERTOOSMALL;
            }
            Voice* voice = CreateVoice(wfx->nSamplesPerSec, wfx->nChannels, wfx->wBitsPerSample);
            if (!voice) {
                return DSERR_OUTOFMEMORY;
            }
            SecondaryBuffer* buffer = new SecondaryBuffer(desc, voice);
            buffer->bufferInterface.AddRef();
            *out = &buffer->bufferInterface;
            return DS_OK;
        }

        SecondaryBuffer::SecondaryBuffer(const DSBUFFERDESC& desc, Voice* voice)
          : voice(voice), format(*desc.lpwfxFormat), flags(desc.dwFlags), frequency(desc.lpwfxFormat->nSamplesPerSec) {
            format.cbSize = 0;
            // The size in whole frames, rounded up as DirectSound does (Wine's tests, passing on Windows): the game
            // copies dwBufferBytes into a buffer locked whole (BSWin32GameSound::Copy, PrepareReverb), so it must hold
            // them all. Silence (0x80 for 8-bit samples) until written.
            const uint32_t size = desc.dwBufferBytes +
                (format.nBlockAlign - desc.dwBufferBytes % format.nBlockAlign) % format.nBlockAlign;
            data.assign(size, format.wBitsPerSample == 8 ? 0x80 : 0);
            sourceDeferred = source;
            SetSource(voice, Pull, this);
            std::lock_guard<std::mutex> lock(gBuffersLock);
            gBuffers.push_back(this);
        }

        SecondaryBuffer::~SecondaryBuffer() {
            {
                std::lock_guard<std::mutex> lock(gBuffersLock);
                gBuffers.erase(std::find(gBuffers.begin(), gBuffers.end(), this));
            }
            // The voice's source is not called once it is destroyed.
            DestroyVoice(voice);
            ReleaseEffects();
        }

        HRESULT SecondaryBuffer::QueryInterface(REFIID riid, void** ppv) {
            if (!ppv) {
                return E_POINTER;
            }
            if (SameGuid(riid, IID_IUnknown) ||
                SameGuid(riid, IID_IDirectSoundBuffer) ||
                SameGuid(riid, IID_IDirectSoundBuffer8)) {
                bufferInterface.AddRef();
                *ppv = static_cast<IDirectSoundBuffer8*>(&bufferInterface);
                return S_OK;
            }
            if (SameGuid(riid, IID_IDirectSound3DBuffer) && (flags & DSBCAPS_CTRL3D)) {
                buffer3DInterface.AddRef();
                *ppv = static_cast<IDirectSound3DBuffer*>(&buffer3DInterface);
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }

        size_t SecondaryBuffer::Pull(void* user, void* out, size_t bytes) {
            SecondaryBuffer* self = static_cast<SecondaryBuffer*>(user);
            if (!self->playing || self->atEnd) {
                return 0;
            }
            const uint32_t size = static_cast<uint32_t>(self->data.size());
            uint8_t* to = static_cast<uint8_t*>(out);
            size_t copied = 0;
            while (copied < bytes) {
                if (self->readPosition == size) {
                    if (!self->looping) {
                        self->atEnd = true;
                        break;
                    }
                    self->readPosition = 0;
                }
                const size_t chunk = std::min<size_t>(bytes - copied, size - self->readPosition);
                std::memcpy(to + copied, self->data.data() + self->readPosition, chunk);
                self->readPosition += static_cast<uint32_t>(chunk);
                copied += chunk;
            }
            if (self->atEnd) {
                // The voice counts this call's frames after it returns.
                self->endFrame = TakenFrames(self->voice) + copied / self->format.nBlockAlign;
            }
            return copied;
        }

        uint32_t SecondaryBuffer::PlayCursor() {
            if (!playing) {
                return readPosition;
            }
            const uint64_t played = PlayedFrames(voice);
            const uint64_t frames = played > basePlayed ? played - basePlayed : 0;
            const uint64_t position = basePosition + frames * format.nBlockAlign;
            const uint32_t size = static_cast<uint32_t>(data.size());
            if (!looping && atEnd && position >= size) {
                return size;
            }
            return static_cast<uint32_t>(position % size);
        }

        uint32_t SecondaryBuffer::WriteCursor() {
            if (!playing) {
                return readPosition;
            }
            return readPosition == data.size() ? 0 : readPosition;
        }

        void SecondaryBuffer::Rebase(uint32_t position) {
            Clear(voice);
            readPosition = position;
            basePosition = position;
            basePlayed = PlayedFrames(voice);
            atEnd = false;
        }

        void SecondaryBuffer::EndIfPlayed() {
            bool ended = false;
            LockVoice(voice);
            if (playing && atEnd && PlayedFrames(voice) >= endFrame) {
                playing = false;
                Rebase(0);
                ended = true;
            }
            UnlockVoice(voice);
            if (ended) {
                SetPaused(voice, true);
            }
        }

        HRESULT SecondaryBuffer::GetCurrentPosition(uint32_t* play, uint32_t* write) {
            std::lock_guard<std::mutex> api(apiLock);
            EndIfPlayed();
            LockVoice(voice);
            if (play) {
                *play = PlayCursor();
            }
            if (write) {
                *write = WriteCursor();
            }
            UnlockVoice(voice);
            return DS_OK;
        }

        HRESULT SecondaryBuffer::GetFormat(WAVEFORMATEX* out, uint32_t size, uint32_t* written) {
            if (!out) {
                if (!written) {
                    return DSERR_INVALIDPARAM;
                }
                *written = sizeof(WAVEFORMATEX);
                return DS_OK;
            }
            const uint32_t bytes = std::min<uint32_t>(size, sizeof(WAVEFORMATEX));
            std::memcpy(out, &format, bytes);
            if (written) {
                *written = bytes;
            }
            return DS_OK;
        }

        HRESULT SecondaryBuffer::GetFrequency(uint32_t* out) {
            if (!out) {
                return DSERR_INVALIDPARAM;
            }
            if (!(flags & DSBCAPS_CTRLFREQUENCY)) {
                return DSERR_CONTROLUNAVAIL;
            }
            std::lock_guard<std::mutex> api(apiLock);
            *out = frequency;
            return DS_OK;
        }

        HRESULT SecondaryBuffer::GetStatus(uint32_t* status) {
            if (!status) {
                return DSERR_INVALIDPARAM;
            }
            std::lock_guard<std::mutex> api(apiLock);
            EndIfPlayed();
            LockVoice(voice);
            *status = playing ? DSBSTATUS_PLAYING | (looping ? DSBSTATUS_LOOPING : 0) : 0;
            UnlockVoice(voice);
            return DS_OK;
        }

        HRESULT SecondaryBuffer::Lock(
            uint32_t offset,
            uint32_t bytes,
            void** audio1,
            uint32_t* bytes1,
            void** audio2,
            uint32_t* bytes2,
            uint32_t lockFlags
        ) {
            if (!audio1 || !bytes1) {
                return DSERR_INVALIDPARAM;
            }
            const uint32_t size = static_cast<uint32_t>(data.size());
            std::lock_guard<std::mutex> api(apiLock);
            if (lockFlags & DSBLOCK_FROMWRITECURSOR) {
                LockVoice(voice);
                offset = WriteCursor();
                UnlockVoice(voice);
            }
            if (lockFlags & DSBLOCK_ENTIREBUFFER) {
                bytes = size;
            }
            if (offset >= size || bytes == 0 || bytes > size) {
                return DSERR_INVALIDPARAM;
            }
            // The part up to the buffer's end, and the rest from its start when the caller takes a second part.
            const uint32_t first = std::min(bytes, size - offset);
            *audio1 = data.data() + offset;
            *bytes1 = first;
            if (audio2) {
                *audio2 = first < bytes ? data.data() : nullptr;
            }
            if (bytes2) {
                *bytes2 = first < bytes && audio2 ? bytes - first : 0;
            }
            return DS_OK;
        }

        HRESULT SecondaryBuffer::Play(uint32_t playFlags) {
            std::lock_guard<std::mutex> api(apiLock);
            EndIfPlayed();
            LockVoice(voice);
            const bool start = !playing;
            if (playing) {
                // Playing on: the cursors count on from the current play position.
                basePosition = PlayCursor() % static_cast<uint32_t>(data.size());
                basePlayed = PlayedFrames(voice);
            }
            else {
                basePlayed = PlayedFrames(voice);
            }
            looping = (playFlags & DSBPLAY_LOOPING) != 0;
            if (looping) {
                atEnd = false;
            }
            playing = true;
            UnlockVoice(voice);
            if (start) {
                Trace(
                    "dsound: play %p from %u%s, volume %d, %u Hz",
                    static_cast<void*>(&bufferInterface),
                    readPosition,
                    looping ? " looping" : "",
                    volume,
                    frequency
                );
                SetPaused(voice, false);
            }
            return DS_OK;
        }

        HRESULT SecondaryBuffer::Stop() {
            std::lock_guard<std::mutex> api(apiLock);
            EndIfPlayed();
            if (!playing) {
                return DS_OK;
            }
            Trace("dsound: stop %p", static_cast<void*>(&bufferInterface));
            SetPaused(voice, true);
            LockVoice(voice);
            const uint32_t position = PlayCursor() % static_cast<uint32_t>(data.size());
            playing = false;
            Rebase(position);
            UnlockVoice(voice);
            return DS_OK;
        }

        HRESULT SecondaryBuffer::SetCurrentPosition(uint32_t position) {
            std::lock_guard<std::mutex> api(apiLock);
            if (position >= data.size()) {
                return DSERR_INVALIDPARAM;
            }
            EndIfPlayed();
            LockVoice(voice);
            Rebase(position - position % format.nBlockAlign);
            UnlockVoice(voice);
            return DS_OK;
        }

        // The voice's gains and rate: the volume, then the pan (SetPan's, or the 3D model's for a 3D buffer) by
        // DirectSound's pan law (the far channel attenuated by the pan's hundredths of a decibel), and the frequency
        // with the Doppler ratio.
        void SecondaryBuffer::ApplyGains() {
            float gain = Attenuation(volume);
            int32_t panning = pan;
            float ratio = static_cast<float>(frequency) / static_cast<float>(format.nSamplesPerSec);
            if (flags & DSBCAPS_CTRL3D) {
                ListenerParams listener;
                {
                    std::lock_guard<std::mutex> lock(gListenerLock);
                    listener = gListener;
                }
                const Spatial spatial = Compute(listener, source, (flags & DSBCAPS_MUTE3DATMAXDISTANCE) != 0);
                gain *= spatial.gain;
                panning = spatial.pan;
                ratio *= spatial.ratio;
            }
            const float left = panning > 0 ? Attenuation(-panning) : 1.0f;
            const float right = panning < 0 ? Attenuation(panning) : 1.0f;
            SetGains(voice, gain * left, gain * right);
            SetFrequencyRatio(voice, ratio);
        }

        void SecondaryBuffer::ListenerChanged() {
            std::lock_guard<std::mutex> api(apiLock);
            ApplyGains();
        }

        HRESULT SecondaryBuffer::SetVolume(int32_t value) {
            if (!(flags & DSBCAPS_CTRLVOLUME)) {
                return DSERR_CONTROLUNAVAIL;
            }
            if (value < DSBVOLUME_MIN || value > DSBVOLUME_MAX) {
                return DSERR_INVALIDPARAM;
            }
            std::lock_guard<std::mutex> api(apiLock);
            volume = value;
            ApplyGains();
            return DS_OK;
        }

        HRESULT SecondaryBuffer::SetPan(int32_t value) {
            if (!(flags & DSBCAPS_CTRLPAN)) {
                return DSERR_CONTROLUNAVAIL;
            }
            if (value < DSBPAN_LEFT || value > DSBPAN_RIGHT) {
                return DSERR_INVALIDPARAM;
            }
            std::lock_guard<std::mutex> api(apiLock);
            pan = value;
            ApplyGains();
            return DS_OK;
        }

        HRESULT SecondaryBuffer::SetFrequency(uint32_t value) {
            if (!(flags & DSBCAPS_CTRLFREQUENCY)) {
                return DSERR_CONTROLUNAVAIL;
            }
            if (value == DSBFREQUENCY_ORIGINAL) {
                value = format.nSamplesPerSec;
            }
            if (value < DSBFREQUENCY_MIN || value > DSBFREQUENCY_MAX) {
                return DSERR_INVALIDPARAM;
            }
            std::lock_guard<std::mutex> api(apiLock);
            frequency = value;
            ApplyGains();
            return DS_OK;
        }

        void SecondaryBuffer::ReleaseEffects() {
            for (Effect& effect : effects) {
                effect.object->Release();
            }
            effects.clear();
        }

        HRESULT SecondaryBuffer::SetFX(uint32_t count, DSEFFECTDESC* descs, uint32_t* results) {
            if (!(flags & DSBCAPS_CTRLFX)) {
                return DSERR_CONTROLUNAVAIL;
            }
            if (count != 0 && !descs) {
                return DSERR_INVALIDPARAM;
            }
            std::lock_guard<std::mutex> api(apiLock);
            EndIfPlayed();
            if (playing) {
                return DSERR_INVALIDCALL;
            }
            for (uint32_t i = 0; i < count; ++i) {
                if (!SameGuid(descs[i].guidDSFXClass, GUID_DSFX_STANDARD_DISTORTION) &&
                    !SameGuid(descs[i].guidDSFXClass, GUID_DSFX_STANDARD_I3DL2REVERB)) {
                    return DSERR_FXUNAVAILABLE;
                }
            }
            ReleaseEffects();
            for (uint32_t i = 0; i < count; ++i) {
                const GUID& guid = descs[i].guidDSFXClass;
                IUnknown* object = nullptr;
                if (SameGuid(guid, GUID_DSFX_STANDARD_DISTORTION)) {
                    object = new DistortionEffect;
                }
                else {
                    object = new ReverbEffect;
                }
                effects.push_back(Effect{guid, object});
                if (results) {
                    results[i] = DSFXR_LOCSOFTWARE;
                }
            }
            return DS_OK;
        }

        HRESULT SecondaryBuffer::GetObjectInPath(const GUID* object, uint32_t index, const GUID* iid, void** ppv) {
            if (!object || !iid || !ppv) {
                return DSERR_INVALIDPARAM;
            }
            std::lock_guard<std::mutex> api(apiLock);
            uint32_t seen = 0;
            for (Effect& effect : effects) {
                if (SameGuid(effect.guid, *object) && seen++ == index) {
                    return effect.object->QueryInterface(*iid, ppv);
                }
            }
            *ppv = nullptr;
            return DSERR_OBJECTNOTFOUND;
        }

        template <class Change>
        HRESULT SecondaryBuffer::Set3D(uint32_t apply, Change change) {
            std::lock_guard<std::mutex> api(apiLock);
            change(sourceDeferred);
            if (!(apply & DS3D_DEFERRED)) {
                change(source);
                ApplyGains();
            }
            return DS_OK;
        }

        void SecondaryBuffer::CommitDeferred() {
            if (!(flags & DSBCAPS_CTRL3D)) {
                return;
            }
            std::lock_guard<std::mutex> api(apiLock);
            source = sourceDeferred;
            ApplyGains();
        }

        // --- The primary buffer and the listener
        // ---------------------------------------------------------------------------

        class PrimaryBuffer {
        public:
            static IDirectSoundBuffer* Get();

            class BufferInterface final : public IDirectSoundBuffer {
            public:
                explicit BufferInterface(PrimaryBuffer* owner) : self(owner) {}
                HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
                    return self->QueryInterface(riid, ppv);
                }
                uint32_t STDMETHODCALLTYPE AddRef() override {
                    return self->bufferRefs.AddRef(self);
                }
                uint32_t STDMETHODCALLTYPE Release() override {
                    return self->bufferRefs.Release(self);
                }
                HRESULT STDMETHODCALLTYPE GetCaps(DSBCAPS*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetCurrentPosition(uint32_t*, uint32_t*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetFormat(WAVEFORMATEX*, uint32_t, uint32_t*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetVolume(int32_t*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetPan(int32_t*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetFrequency(uint32_t*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetStatus(uint32_t*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE Initialize(IDirectSound*, const DSBUFFERDESC*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE
                Lock(uint32_t, uint32_t, void**, uint32_t*, void**, uint32_t*, uint32_t) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE Play(uint32_t, uint32_t, uint32_t) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE SetCurrentPosition(uint32_t) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE SetFormat(const WAVEFORMATEX*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE SetVolume(int32_t) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE SetPan(int32_t) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE SetFrequency(uint32_t) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE Stop() override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE Unlock(void*, uint32_t, void*, uint32_t) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE Restore() override {
                    return E_NOTIMPL;
                }

            private:
                PrimaryBuffer* self;
            };

            class ListenerInterface final : public IDirectSound3DListener {
            public:
                explicit ListenerInterface(PrimaryBuffer* owner) : self(owner) {}
                HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
                    return self->QueryInterface(riid, ppv);
                }
                uint32_t STDMETHODCALLTYPE AddRef() override {
                    return self->listenerRefs.AddRef(self);
                }
                uint32_t STDMETHODCALLTYPE Release() override {
                    return self->listenerRefs.Release(self);
                }
                HRESULT STDMETHODCALLTYPE GetAllParameters(DS3DLISTENER*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetDistanceFactor(float*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetDopplerFactor(float*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetOrientation(D3DVECTOR*, D3DVECTOR*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetPosition(D3DVECTOR*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetRolloffFactor(float*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE GetVelocity(D3DVECTOR*) override {
                    return E_NOTIMPL;
                }
                HRESULT STDMETHODCALLTYPE SetAllParameters(const DS3DLISTENER*, uint32_t) override {
                    return E_NOTIMPL;
                }
                // The factors' ranges are DirectSound's (DS3D_MINDISTANCEFACTOR ... DS3D_MAXDOPPLERFACTOR).
                HRESULT STDMETHODCALLTYPE SetDistanceFactor(float factor, uint32_t apply) override {
                    if (!(factor > 0.0f)) {
                        return DSERR_INVALIDPARAM;
                    }
                    return Set(apply, [factor](ListenerParams& p) { p.distanceFactor = factor; });
                }
                HRESULT STDMETHODCALLTYPE SetDopplerFactor(float factor, uint32_t apply) override {
                    if (!(factor >= 0.0f && factor <= 10.0f)) {
                        return DSERR_INVALIDPARAM;
                    }
                    return Set(apply, [factor](ListenerParams& p) { p.dopplerFactor = factor; });
                }
                HRESULT STDMETHODCALLTYPE SetOrientation(
                    float xFront,
                    float yFront,
                    float zFront,
                    float xTop,
                    float yTop,
                    float zTop,
                    uint32_t apply
                ) override {
                    return Set(apply, [=](ListenerParams& p) {
                        p.front = D3DVECTOR{xFront, yFront, zFront};
                        p.top = D3DVECTOR{xTop, yTop, zTop};
                    });
                }
                HRESULT STDMETHODCALLTYPE SetPosition(float x, float y, float z, uint32_t apply) override {
                    return Set(apply, [=](ListenerParams& p) { p.position = D3DVECTOR{x, y, z}; });
                }
                HRESULT STDMETHODCALLTYPE SetRolloffFactor(float factor, uint32_t apply) override {
                    if (!(factor >= 0.0f && factor <= 10.0f)) {
                        return DSERR_INVALIDPARAM;
                    }
                    return Set(apply, [factor](ListenerParams& p) { p.rolloffFactor = factor; });
                }
                HRESULT STDMETHODCALLTYPE SetVelocity(float x, float y, float z, uint32_t apply) override {
                    return Set(apply, [=](ListenerParams& p) { p.velocity = D3DVECTOR{x, y, z}; });
                }
                // The listener's and every 3D buffer's deferred settings become current.
                HRESULT STDMETHODCALLTYPE CommitDeferredSettings() override {
                    {
                        std::lock_guard<std::mutex> lock(gListenerLock);
                        gListener = gListenerDeferred;
                    }
                    std::lock_guard<std::mutex> lock(gBuffersLock);
                    for (SecondaryBuffer* buffer : gBuffers) {
                        buffer->CommitDeferred();
                    }
                    return DS_OK;
                }

            private:
                template <class Change>
                static HRESULT Set(uint32_t apply, Change change) {
                    {
                        std::lock_guard<std::mutex> lock(gListenerLock);
                        change(gListenerDeferred);
                        if (apply & DS3D_DEFERRED) {
                            return DS_OK;
                        }
                        change(gListener);
                    }
                    // Applied now: every 3D buffer hears the listener's new settings.
                    std::lock_guard<std::mutex> lock(gBuffersLock);
                    for (SecondaryBuffer* buffer : gBuffers) {
                        buffer->ListenerChanged();
                    }
                    return DS_OK;
                }

                PrimaryBuffer* self;
            };

        private:
            friend struct InterfaceCount<PrimaryBuffer>;

            PrimaryBuffer() = default;
            ~PrimaryBuffer();

            HRESULT QueryInterface(REFIID riid, void** ppv) {
                if (!ppv) {
                    return E_POINTER;
                }
                if (SameGuid(riid, IID_IUnknown) || SameGuid(riid, IID_IDirectSoundBuffer)) {
                    bufferInterface.AddRef();
                    *ppv = static_cast<IDirectSoundBuffer*>(&bufferInterface);
                    return S_OK;
                }
                if (SameGuid(riid, IID_IDirectSound3DListener)) {
                    listenerInterface.AddRef();
                    *ppv = static_cast<IDirectSound3DListener*>(&listenerInterface);
                    return S_OK;
                }
                *ppv = nullptr;
                return E_NOINTERFACE;
            }

            InterfaceCount<PrimaryBuffer> bufferRefs;
            InterfaceCount<PrimaryBuffer> listenerRefs;
            std::atomic<uint32_t> live{0};
            BufferInterface bufferInterface{this};
            ListenerInterface listenerInterface{this};
        };

        // The one primary buffer, while any of its interfaces is held: DirectSound hands out the same one each time.
        std::mutex gPrimaryLock;
        PrimaryBuffer* gPrimary = nullptr;

        IDirectSoundBuffer* PrimaryBuffer::Get() {
            std::lock_guard<std::mutex> lock(gPrimaryLock);
            if (!gPrimary) {
                gPrimary = new PrimaryBuffer;
            }
            gPrimary->bufferInterface.AddRef();
            return &gPrimary->bufferInterface;
        }

        PrimaryBuffer::~PrimaryBuffer() {
            std::lock_guard<std::mutex> lock(gPrimaryLock);
            gPrimary = nullptr;
        }

        // --- The device
        // ----------------------------------------------------------------------------------------------------

        class Device final : public IDirectSound8 {
        public:
            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
                if (!ppv) {
                    return E_POINTER;
                }
                if (SameGuid(riid, IID_IUnknown) || SameGuid(riid, IID_IDirectSound8)) {
                    AddRef();
                    *ppv = this;
                    return S_OK;
                }
                *ppv = nullptr;
                return E_NOINTERFACE;
            }
            uint32_t STDMETHODCALLTYPE AddRef() override {
                return refs.fetch_add(1) + 1;
            }
            uint32_t STDMETHODCALLTYPE Release() override {
                const uint32_t left = refs.fetch_sub(1) - 1;
                if (left == 0) {
                    delete this;
                }
                return left;
            }

            HRESULT STDMETHODCALLTYPE
            CreateSoundBuffer(const DSBUFFERDESC* desc, IDirectSoundBuffer** out, IUnknown* outer) override {
                if (!out) {
                    return DSERR_INVALIDPARAM;
                }
                *out = nullptr;
                if (!desc || desc->dwSize != sizeof(DSBUFFERDESC)) {
                    return DSERR_INVALIDPARAM;
                }
                if (outer) {
                    return DSERR_NOAGGREGATION;
                }
                if (desc->dwFlags & DSBCAPS_PRIMARYBUFFER) {
                    *out = PrimaryBuffer::Get();
                    Trace("dsound: primary buffer");
                    return DS_OK;
                }
                const HRESULT result = SecondaryBuffer::Create(*desc, out);
                const WAVEFORMATEX* format = desc->lpwfxFormat;
                Trace(
                    "dsound: buffer %p flags 0x%X, %u bytes, %u Hz, %u channels, %u bits: 0x%08X",
                    static_cast<void*>(*out),
                    desc->dwFlags,
                    desc->dwBufferBytes,
                    format ? format->nSamplesPerSec : 0,
                    format ? format->nChannels : 0,
                    format ? format->wBitsPerSample : 0,
                    static_cast<uint32_t>(result)
                );
                return result;
            }

            // A device of software buffers only, as DirectSound reports on every Windows since Vista.
            HRESULT STDMETHODCALLTYPE GetCaps(DSCAPS* caps) override {
                if (!caps || caps->dwSize != sizeof(DSCAPS)) {
                    return DSERR_INVALIDPARAM;
                }
                std::memset(caps, 0, sizeof(DSCAPS));
                caps->dwSize = sizeof(DSCAPS);
                // DSCAPS_PRIMARYMONO, ..STEREO, ..8BIT, ..16BIT, DSCAPS_CONTINUOUSRATE and the SECONDARY ones.
                caps->dwFlags = 0x1 | 0x2 | 0x4 | 0x8 | 0x10 | 0x100 | 0x200 | 0x400 | 0x800;
                caps->dwMinSecondarySampleRate = DSBFREQUENCY_MIN;
                caps->dwMaxSecondarySampleRate = DSBFREQUENCY_MAX;
                caps->dwPrimaryBuffers = 1;
                return DS_OK;
            }

            HRESULT STDMETHODCALLTYPE DuplicateSoundBuffer(IDirectSoundBuffer*, IDirectSoundBuffer**) override {
                return E_NOTIMPL;
            }

            // DSSCL_NORMAL (1) to DSSCL_WRITEPRIMARY (4); the level changes nothing here.
            HRESULT STDMETHODCALLTYPE SetCooperativeLevel(HWND__*, uint32_t level) override {
                return level >= 1 && level <= 4 ? DS_OK : DSERR_INVALIDPARAM;
            }

            HRESULT STDMETHODCALLTYPE Compact() override {
                return E_NOTIMPL;
            }
            HRESULT STDMETHODCALLTYPE GetSpeakerConfig(uint32_t*) override {
                return E_NOTIMPL;
            }
            HRESULT STDMETHODCALLTYPE SetSpeakerConfig(uint32_t) override {
                return E_NOTIMPL;
            }
            HRESULT STDMETHODCALLTYPE Initialize(const GUID*) override {
                return E_NOTIMPL;
            }
            HRESULT STDMETHODCALLTYPE VerifyCertification(uint32_t*) override {
                return E_NOTIMPL;
            }

        private:
            std::atomic<uint32_t> refs{1};
        };

    } // namespace

    bool SameGuid(const GUID& a, const GUID& b) {
        return std::memcmp(&a, &b, sizeof(GUID)) == 0;
    }

    // dsound.dll's DirectSoundCreate8: the default playback device, when the output has one.
    HRESULT WINAPI Create8(const GUID* device, IDirectSound8** out, IUnknown* outer) {
        if (!out) {
            return DSERR_INVALIDPARAM;
        }
        *out = nullptr;
        if (outer) {
            return DSERR_NOAGGREGATION;
        }
        if ((device && !SameGuid(*device, DSDEVID_DefaultPlayback)) || !Available()) {
            return DSERR_NODRIVER;
        }
        *out = new Device;
        Trace("dsound: device");
        return DS_OK;
    }

} // namespace audio::ds

// The export the game's import names: on Linux the C function itself; on Windows (XFNV_SDL_AUDIO) the import slot
// `__imp_` the game's dllimport calls go through, so dsound.lib is never pulled in for it.
#if defined(_WIN32)
extern "C" {
decltype(&audio::ds::Create8) xfnv_imp_DirectSoundCreate8 __asm__("__imp_DirectSoundCreate8") = &audio::ds::Create8;
}
#else
extern "C" HRESULT WINAPI DirectSoundCreate8(const GUID* device, IDirectSound8** out, IUnknown* outer) {
    return audio::ds::Create8(device, out, outer);
}
#endif
