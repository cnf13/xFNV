// The audio layer's output on SDL3 (Output.h). Each voice's SDL_AudioStream is fed from its get callback on the
// device's thread, so a voice's gains apply to what plays next and its data is read only when the device needs it.
// The stream's lock (recursive) guards the voice's state everywhere, as the callback runs under it.
#include "platform/dsound/Output.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdarg>
#include <cstring>
#include <mutex>
#include <vector>

namespace audio {

    struct Voice {
        SDL_AudioStream* stream = nullptr;
        uint32_t channels = 0;
        uint32_t bitsPerSample = 0;
        // Bytes of one frame in the voice's format, and channels of what goes into the stream (float): a mono voice is
        // made stereo so that its gains pan it.
        uint32_t frameBytes = 0;
        uint32_t streamChannels = 0;
        // Under pauseLock, not the stream's: binding takes the device's lock, which the device's thread takes before
        // the stream's.
        std::mutex pauseLock;
        bool paused = true;
        float left = 1.0f;
        float right = 1.0f;
        Source source = nullptr;
        void* user = nullptr;
        // The queue: bytes from queueHead on are still to play.
        std::vector<uint8_t> queue;
        size_t queueHead = 0;
        // Frames put into the stream so far.
        uint64_t taken = 0;
        // The stream was flushed since data was last put in it.
        bool flushed = true;
        // The callback's buffers: the voice's PCM and its float conversion.
        std::vector<uint8_t> pcm;
        std::vector<float> converted;
    };

    namespace {

        SDL_AudioDeviceID gDevice = 0;
        std::once_flag gOpenOnce;

        // Opens SDL's audio and its default playback device, once.
        void OpenDevice() {
            if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
                SDL_Log("audio: SDL audio does not start: %s", SDL_GetError());
                return;
            }
            gDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
            if (gDevice == 0) {
                SDL_Log("audio: no playback device opens: %s", SDL_GetError());
                SDL_QuitSubSystem(SDL_INIT_AUDIO);
                return;
            }
            SDL_AudioSpec spec{};
            SDL_GetAudioDeviceFormat(gDevice, &spec, nullptr);
            SDL_Log(
                "audio: playback on SDL's %s driver, %d Hz, %d channels", SDL_GetCurrentAudioDriver(), spec.freq,
                spec.channels
            );
        }

        class StreamLock {
        public:
            explicit StreamLock(Voice* voice) : stream(voice->stream) {
                SDL_LockAudioStream(stream);
            }
            ~StreamLock() {
                SDL_UnlockAudioStream(stream);
            }
            StreamLock(const StreamLock&) = delete;
            StreamLock& operator=(const StreamLock&) = delete;

        private:
            SDL_AudioStream* stream;
        };

        uint32_t StreamFrameBytes(const Voice* voice) {
            return voice->streamChannels * static_cast<uint32_t>(sizeof(float));
        }

        // Frames still in the stream, not yet consumed by the device. Under the stream's lock.
        uint64_t FramesInStream(const Voice* voice) {
            const int queued = SDL_GetAudioStreamQueued(voice->stream);
            return queued > 0 ? static_cast<uint64_t>(queued) / StreamFrameBytes(voice) : 0;
        }

        float Sample(const Voice* voice, const uint8_t* at) {
            switch (voice->bitsPerSample) {
                case 8: return (static_cast<float>(*at) - 128.0f) / 128.0f;
                case 16: {
                    int16_t value;
                    std::memcpy(&value, at, sizeof(value));
                    return static_cast<float>(value) / 32768.0f;
                }
                default: {
                    float value;
                    std::memcpy(&value, at, sizeof(value));
                    return value;
                }
            }
        }

        // Converts frames of the voice's PCM to the stream's float frames with the gains applied.
        void Convert(Voice* voice, const uint8_t* pcm, size_t frames, float* out) {
            const uint32_t sampleBytes = voice->bitsPerSample / 8;
            if (voice->channels == 1) {
                for (size_t i = 0; i < frames; ++i) {
                    const float value = Sample(voice, pcm + i * sampleBytes);
                    out[2 * i] = value * voice->left;
                    out[2 * i + 1] = value * voice->right;
                }
                return;
            }
            const float mean = (voice->left + voice->right) * 0.5f;
            for (size_t i = 0; i < frames; ++i) {
                for (uint32_t c = 0; c < voice->channels; ++c) {
                    float gain = mean;
                    if (voice->channels == 2) {
                        gain = c == 0 ? voice->left : voice->right;
                    }
                    out[i * voice->channels + c] = Sample(voice, pcm + (i * voice->channels + c) * sampleBytes) * gain;
                }
            }
        }

        // The stream's get callback (device thread, stream locked): takes the frames the device asks for from the
        // source or the queue and puts them in the stream converted.
        void SDLCALL Feed(void* userdata, SDL_AudioStream* stream, int additional, int) {
            Voice* voice = static_cast<Voice*>(userdata);
            if (additional <= 0) {
                return;
            }
            const size_t frames = static_cast<size_t>(additional) / StreamFrameBytes(voice) + 1;
            size_t bytes = frames * voice->frameBytes;
            const uint8_t* pcm = nullptr;
            if (voice->source) {
                voice->pcm.resize(bytes);
                bytes = voice->source(voice->user, voice->pcm.data(), bytes);
                pcm = voice->pcm.data();
            }
            else {
                bytes = std::min(bytes, voice->queue.size() - voice->queueHead);
                pcm = voice->queue.data() + voice->queueHead;
            }
            const size_t got = bytes / voice->frameBytes;
            if (got != 0) {
                voice->converted.resize(got * voice->streamChannels);
                Convert(voice, pcm, got, voice->converted.data());
                SDL_PutAudioStreamData(
                    stream, voice->converted.data(), static_cast<int>(got * StreamFrameBytes(voice))
                );
                voice->taken += got;
                voice->flushed = false;
            }
            // The data ran out: the stream holds back its last frames for the resampler until flushed, so they play now
            // (and the played count reaches the taken one). It takes more data after a flush.
            if (got < frames && !voice->flushed) {
                SDL_FlushAudioStream(stream);
                voice->flushed = true;
            }
            if (!voice->source) {
                voice->queueHead += got * voice->frameBytes;
                if (voice->queueHead == voice->queue.size()) {
                    voice->queue.clear();
                    voice->queueHead = 0;
                }
            }
        }

    } // namespace

    bool Available() {
        std::call_once(gOpenOnce, OpenDevice);
        return gDevice != 0;
    }

    Voice* CreateVoice(uint32_t rate, uint32_t channels, uint32_t bitsPerSample) {
        if (!Available() || rate == 0 || channels == 0 ||
            (bitsPerSample != 8 && bitsPerSample != 16 && bitsPerSample != 32)) {
            return nullptr;
        }
        Voice* voice = new Voice;
        voice->channels = channels;
        voice->bitsPerSample = bitsPerSample;
        voice->frameBytes = channels * bitsPerSample / 8;
        voice->streamChannels = channels == 1 ? 2 : channels;
        const SDL_AudioSpec spec{SDL_AUDIO_F32, static_cast<int>(voice->streamChannels), static_cast<int>(rate)};
        voice->stream = SDL_CreateAudioStream(&spec, &spec);
        if (!voice->stream) {
            SDL_Log("audio: no stream for a voice: %s", SDL_GetError());
            delete voice;
            return nullptr;
        }
        SDL_SetAudioStreamGetCallback(voice->stream, Feed, voice);
        return voice;
    }

    void DestroyVoice(Voice* voice) {
        if (!voice) {
            return;
        }
        // Unbinds it, waiting for its callback to end.
        SDL_DestroyAudioStream(voice->stream);
        delete voice;
    }

    void Queue(Voice* voice, const void* data, size_t bytes) {
        StreamLock lock(voice);
        const uint8_t* begin = static_cast<const uint8_t*>(data);
        voice->queue.insert(voice->queue.end(), begin, begin + bytes);
    }

    size_t QueuedBytes(Voice* voice) {
        StreamLock lock(voice);
        return voice->queue.size() - voice->queueHead + FramesInStream(voice) * voice->frameBytes;
    }

    void Clear(Voice* voice) {
        StreamLock lock(voice);
        voice->taken -= FramesInStream(voice);
        SDL_ClearAudioStream(voice->stream);
        voice->queue.clear();
        voice->queueHead = 0;
    }

    void SetSource(Voice* voice, Source source, void* user) {
        StreamLock lock(voice);
        voice->source = source;
        voice->user = user;
    }

    void LockVoice(Voice* voice) {
        SDL_LockAudioStream(voice->stream);
    }

    void UnlockVoice(Voice* voice) {
        SDL_UnlockAudioStream(voice->stream);
    }

    void SetPaused(Voice* voice, bool paused) {
        std::lock_guard<std::mutex> lock(voice->pauseLock);
        if (paused == voice->paused) {
            return;
        }
        voice->paused = paused;
        // An unbound stream keeps its data and is not read: the voice holds its position.
        if (paused) {
            SDL_UnbindAudioStream(voice->stream);
        }
        else if (!SDL_BindAudioStream(gDevice, voice->stream)) {
            SDL_Log("audio: a voice does not bind to the device: %s", SDL_GetError());
        }
    }

    void SetGains(Voice* voice, float left, float right) {
        StreamLock lock(voice);
        voice->left = left;
        voice->right = right;
    }

    void SetFrequencyRatio(Voice* voice, float ratio) {
        SDL_SetAudioStreamFrequencyRatio(voice->stream, std::clamp(ratio, 0.01f, 100.0f));
    }

    uint64_t PlayedFrames(Voice* voice) {
        StreamLock lock(voice);
        return voice->taken - std::min(voice->taken, FramesInStream(voice));
    }

    uint64_t TakenFrames(Voice* voice) {
        StreamLock lock(voice);
        return voice->taken;
    }

    void Trace(const char* format, ...) {
        va_list args;
        va_start(args, format);
        SDL_LogMessageV(SDL_LOG_CATEGORY_AUDIO, SDL_LOG_PRIORITY_DEBUG, format, args);
        va_end(args);
    }

} // namespace audio
