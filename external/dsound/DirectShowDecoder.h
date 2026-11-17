// The music decoders of the layer's DirectShow graph (DirectShow.cpp): MP3 (minimp3) and RIFF WAVE PCM, read from a
// byte source (the game's IAsyncReader) and decoded to 16-bit signed interleaved PCM.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace dshow {

// The bytes a decoder reads: random access over a stream of a known size.
class ByteSource {
public:
	virtual ~ByteSource() = default;
	// The stream's size in bytes.
	virtual uint64_t Size() = 0;
	// Reads `bytes` bytes at `position` (within Size()) into `out`; false when the read fails.
	virtual bool ReadAt(uint64_t position, void* out, size_t bytes) = 0;
};

class Decoder {
public:
	virtual ~Decoder() = default;
	// Frames per second and channels of the decoded PCM (16-bit signed, interleaved).
	virtual uint32_t Rate() const = 0;
	virtual uint32_t Channels() const = 0;
	// The stream's length in frames.
	virtual uint64_t Frames() const = 0;
	// Decodes up to `frames` frames into `out`; the frames written, 0 at the end of the stream or on an error.
	virtual size_t Read(int16_t* out, size_t frames) = 0;
	// Continues decoding at `frame` (clamped to the length); false when the stream cannot be read there.
	virtual bool Seek(uint64_t frame) = 0;
};

// A decoder for the stream: WAVE when it starts as a RIFF WAVE file of 8- or 16-bit PCM, else MP3. Null when it is
// neither. The source must outlive the decoder.
std::unique_ptr<Decoder> OpenDecoder(ByteSource& source);

} // namespace dshow
