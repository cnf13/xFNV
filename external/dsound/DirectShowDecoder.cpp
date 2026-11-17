// The music decoders (DirectShowDecoder.h): MP3 through minimp3's streaming decoder over the byte source, and RIFF
// WAVE PCM read directly. The only file of the layer that includes minimp3.
#include "dsound/DirectShowDecoder.h"

#include <algorithm>
#include <cstring>
#include <vector>

// minimp3's implementation, here once; no stdio or mmap (the decoder reads through its callbacks only).
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_STDIO
#include "minimp3/minimp3_ex.h"

namespace dshow {

namespace {

uint32_t Le32(const uint8_t* at) {
	return static_cast<uint32_t>(at[0]) | static_cast<uint32_t>(at[1]) << 8 | static_cast<uint32_t>(at[2]) << 16
		| static_cast<uint32_t>(at[3]) << 24;
}

uint16_t Le16(const uint8_t* at) {
	return static_cast<uint16_t>(at[0] | at[1] << 8);
}

// MP3: minimp3's mp3dec_ex, which scans the stream once when it opens (for the length and a seek index, so seeks
// land on the exact frame) and then decodes as it is read.
class Mp3Decoder final : public Decoder {
public:
	explicit Mp3Decoder(ByteSource& source) : source(source), size(source.Size()) {
		io.read = &Mp3Decoder::ReadCallback;
		io.read_data = this;
		io.seek = &Mp3Decoder::SeekCallback;
		io.seek_data = this;
	}
	~Mp3Decoder() override {
		if (open) {
			mp3dec_ex_close(&dec);
		}
	}
	Mp3Decoder(const Mp3Decoder&) = delete;
	Mp3Decoder& operator=(const Mp3Decoder&) = delete;

	bool Open() {
		if (mp3dec_ex_open_cb(&dec, &io, MP3D_SEEK_TO_SAMPLE) != 0) {
			return false;
		}
		open = true;
		return dec.info.hz > 0 && dec.info.channels > 0 && dec.samples > 0;
	}

	uint32_t Rate() const override { return static_cast<uint32_t>(dec.info.hz); }
	uint32_t Channels() const override { return static_cast<uint32_t>(dec.info.channels); }
	uint64_t Frames() const override { return dec.samples / static_cast<uint64_t>(dec.info.channels); }

	size_t Read(int16_t* out, size_t frames) override {
		const size_t channels = Channels();
		return mp3dec_ex_read(&dec, out, frames * channels) / channels;
	}

	bool Seek(uint64_t frame) override {
		return mp3dec_ex_seek(&dec, std::min(frame, Frames()) * Channels()) == 0;
	}

private:
	static size_t ReadCallback(void* buffer, size_t bytes, void* user) {
		Mp3Decoder* self = static_cast<Mp3Decoder*>(user);
		const uint64_t left = self->size - std::min(self->position, self->size);
		bytes = static_cast<size_t>(std::min<uint64_t>(bytes, left));
		if (bytes == 0 || !self->source.ReadAt(self->position, buffer, bytes)) {
			return 0;
		}
		self->position += bytes;
		return bytes;
	}

	static int SeekCallback(uint64_t position, void* user) {
		Mp3Decoder* self = static_cast<Mp3Decoder*>(user);
		if (position > self->size) {
			return -1;
		}
		self->position = position;
		return 0;
	}

	ByteSource& source;
	uint64_t size;
	uint64_t position = 0;
	mp3dec_io_t io{};
	mp3dec_ex_t dec{};
	bool open = false;
};

// RIFF WAVE with 8-bit unsigned or 16-bit signed PCM (format tag 1, or WAVE_FORMAT_EXTENSIBLE with the PCM subtype):
// the data chunk read as it is, 8-bit samples widened to 16.
class WavDecoder final : public Decoder {
public:
	explicit WavDecoder(ByteSource& source) : source(source) {}

	bool Open() {
		const uint64_t size = source.Size();
		uint8_t header[12];
		if (size < sizeof(header) || !source.ReadAt(0, header, sizeof(header)) || std::memcmp(header, "RIFF", 4) != 0
			|| std::memcmp(header + 8, "WAVE", 4) != 0) {
			return false;
		}
		bool haveFormat = false;
		uint64_t at = sizeof(header);
		while (at + 8 <= size) {
			uint8_t chunk[8];
			if (!source.ReadAt(at, chunk, sizeof(chunk))) {
				return false;
			}
			const uint64_t chunkSize = Le32(chunk + 4);
			const uint64_t body = at + 8;
			if (std::memcmp(chunk, "fmt ", 4) == 0) {
				uint8_t format[40] = {};
				const size_t formatBytes = static_cast<size_t>(std::min<uint64_t>(chunkSize, sizeof(format)));
				if (formatBytes < 16 || body + formatBytes > size || !source.ReadAt(body, format, formatBytes)) {
					return false;
				}
				const uint16_t tag = Le16(format);
				// WAVE_FORMAT_EXTENSIBLE's SubFormat starts with the format tag it stands for.
				const bool pcm = tag == 1 || (tag == 0xFFFE && formatBytes >= 26 && Le16(format + 24) == 1);
				channels = Le16(format + 2);
				rate = Le32(format + 4);
				bits = Le16(format + 14);
				if (!pcm || channels == 0 || rate == 0 || (bits != 8 && bits != 16)) {
					return false;
				}
				haveFormat = true;
			} else if (std::memcmp(chunk, "data", 4) == 0 && haveFormat) {
				dataStart = body;
				const uint64_t bytes = std::min(chunkSize, size - std::min(body, size));
				frames = bytes / (channels * (bits / 8));
				return frames > 0;
			}
			// Chunks are padded to an even size.
			at = body + chunkSize + (chunkSize & 1);
		}
		return false;
	}

	uint32_t Rate() const override { return rate; }
	uint32_t Channels() const override { return channels; }
	uint64_t Frames() const override { return frames; }

	size_t Read(int16_t* out, size_t count) override {
		count = static_cast<size_t>(std::min<uint64_t>(count, frames - next));
		if (count == 0) {
			return 0;
		}
		const size_t samples = count * channels;
		const uint64_t position = dataStart + next * channels * (bits / 8);
		if (bits == 16) {
			// RIFF's samples are little-endian, as the hosts this program runs on are.
			if (!source.ReadAt(position, out, samples * sizeof(int16_t))) {
				return 0;
			}
		} else {
			bytes.resize(samples);
			if (!source.ReadAt(position, bytes.data(), samples)) {
				return 0;
			}
			for (size_t i = 0; i < samples; ++i) {
				out[i] = static_cast<int16_t>((static_cast<int32_t>(bytes[i]) - 128) * 256);
			}
		}
		next += count;
		return count;
	}

	bool Seek(uint64_t frame) override {
		next = std::min(frame, frames);
		return true;
	}

private:
	ByteSource& source;
	uint32_t rate = 0;
	uint32_t channels = 0;
	uint32_t bits = 0;
	uint64_t dataStart = 0;
	uint64_t frames = 0;
	uint64_t next = 0;
	std::vector<uint8_t> bytes;
};

} // namespace

std::unique_ptr<Decoder> OpenDecoder(ByteSource& source) {
	auto wav = std::make_unique<WavDecoder>(source);
	if (wav->Open()) {
		return wav;
	}
	auto mp3 = std::make_unique<Mp3Decoder>(source);
	if (mp3->Open()) {
		return mp3;
	}
	return nullptr;
}

} // namespace dshow
