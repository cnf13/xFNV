// The audio layer's output: one opened playback device and its voices, on SDL3's audio. The only file of the layer
// that names SDL; DirectSound (DirectSound*.cpp) and DirectShow music (DirectShow*.cpp) play through it.
//
// A voice is one SDL_AudioStream bound to the device. It takes PCM in its own format (rate, channels, bits) either
// queued (Queue) or pulled from its owner on the device's thread (SetSource), converts it to float with the voice's
// left/right gains applied, and the stream resamples it to the device by the voice's frequency ratio.
#pragma once

#include <cstddef>
#include <cstdint>

namespace audio {

struct Voice;

// Whether a playback device is open (the first call opens it). False when SDL's audio or its device does not open:
// the callers then fail as DirectSound does without a driver, and the game goes on silent.
bool Available();

// A voice for PCM of this format: 8-bit unsigned or 16-bit signed integer samples, or 32-bit float; 1 or more
// channels interleaved. Created paused, with gains 1 and frequency ratio 1. Null when no device is open or the
// format is not one of those.
Voice* CreateVoice(uint32_t rate, uint32_t channels, uint32_t bitsPerSample);
// Stops the voice and frees it; its source (SetSource) is never called again once this returns.
void DestroyVoice(Voice* voice);

// Appends PCM (whole frames) to the voice's queue; it plays in order while the voice is not paused.
void Queue(Voice* voice, const void* data, size_t bytes);
// The bytes queued and not yet played (in the voice's format).
size_t QueuedBytes(Voice* voice);
// Drops everything queued and not yet played.
void Clear(Voice* voice);

// A voice's pull source: called on the device's thread when the voice needs more PCM, with room for `bytes` (whole
// frames) at `out`; returns the bytes it wrote there (fewer means it has no more for now; the voice plays silence
// until the next call). Used instead of the queue while set.
using Source = size_t (*)(void* user, void* out, size_t bytes);
// Sets the voice's source (null: back to the queue). Waits for a call in progress on the device's thread to end.
void SetSource(Voice* voice, Source source, void* user);
// The lock a voice's source is called under (recursive), for the owner's state the source reads. Every function here
// may be called with it held except SetPaused and DestroyVoice, which take the device's lock (the device's thread
// takes that one first).
void LockVoice(Voice* voice);
void UnlockVoice(Voice* voice);

// Paused: the voice keeps its queued data and its position and outputs nothing.
void SetPaused(Voice* voice, bool paused);
// Linear gains: a mono voice is panned by them, a stereo one scaled per channel; a voice of more channels is scaled by
// their mean.
void SetGains(Voice* voice, float left, float right);
// Playback rate over the voice's own rate (DirectSound's frequency over the buffer's): 0.01 to 100.
void SetFrequencyRatio(Voice* voice, float ratio);
// The voice's frames the device has consumed since it was created (play cursors are taken from this).
uint64_t PlayedFrames(Voice* voice);
// The voice's frames handed to the device so far: queued data taken off the queue, or a source's frames. Data beyond
// this has not been read yet (DirectSound's write cursor).
uint64_t TakenFrames(Voice* voice);

// A line of the layer's trace: SDL's audio log category at debug priority, so it shows with SDL_LOGGING=audio=debug.
[[gnu::format(printf, 1, 2)]] void Trace(const char* format, ...);

} // namespace audio
