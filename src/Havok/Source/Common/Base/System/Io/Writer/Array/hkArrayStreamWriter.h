#pragma once

#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Reconstruction.hpp"

// A byte buffer kept as a list of fixed-size sectors; the last one holds m_numBytesLastSector bytes. src models only
// what the reconstructed code calls.
class hkMemoryTrack {
public:
	hkMemoryTrack(int32_t numBytesPerSector);
	~hkMemoryTrack();

	// Frees every sector and empties the track.
	void clear();

	// Appends the bytes at the end, a new sector each time the last one is full.
	void write(const void* data, int32_t numBytes);
	// Moves the sectors of apOther onto the end of this track.
	void appendByMove(hkMemoryTrack* apOther);

	// The number of bytes the track holds.
	int32_t getSize() const { return (m_sectors.m_size - 1) * m_numBytesPerSector + m_numBytesLastSector; }

	// Copies aiNumBytes bytes, from byte aiOffset of the track, to apData.
	void read(void* data, int32_t numBytes, int32_t offset) const;

	int32_t m_numBytesPerSector;
	int32_t m_numBytesLastSector;
	hkArray<char*> m_sectors;
};

// A stream writer appending to an hkMemoryTrack, which it owns or borrows. The SDK declares it here; the PDB places
// its bodies in hkOStream.cpp. src never constructs it: the vtable stays the exe's, and only the members src replaces
// are declared.
class hkMemoryTrackStreamWriter : public hkStreamWriter {
public:
	enum TrackOwnership {
		TRACK_TAKE = 0,
		TRACK_BORROW = 1,
	};

	// The SDK's hkMemoryTrackStreamWriter(hkMemoryTrack*, TrackOwnership), inline in the exe at its one use
	// (hkOstream's constructor on a track, 0xCC0550): the track and whether the writer frees it.
	hkMemoryTrackStreamWriter(hkMemoryTrack* apTrack, TrackOwnership aeOwnership)
		: m_track(apTrack), m_ownerShip(aeOwnership) {}

	// The SDK's inline bodies (hkArrayStreamWriter.h). The exe's vtable slots hold folded copies:
	hkBool* isOk(hkBool* arResult) const override {
		arResult->m_bool = 1;
		return arResult;
	}

	int32_t write(const void* mem, int32_t size) override;

	hkBool* seekTellSupported(hkBool* arResult) const override {
		arResult->m_bool = 0;
		return arResult;
	}

	hkResult seek(int32_t offset, hkStreamWriter::SeekWhence whence) override { return HK_FAILURE; }
	int32_t tell() const override { return m_track->getSize(); }
	// slot 9
	virtual void clear();
	~hkMemoryTrackStreamWriter() override;

	hkMemoryTrack* m_track;
	TrackOwnership m_ownerShip;
};

// A stream reader over an hkMemoryTrack, which it copies, takes or reads in place. The vtable is the exe's.
class hkMemoryTrackStreamReader : public hkStreamReader {
public:
	enum MemoryType : int32_t {
		MEMORY_COPY = 0,
		MEMORY_TAKE = 1,
		MEMORY_INPLACE = 2,
	};

	hkMemoryTrackStreamReader(const hkMemoryTrack* track, hkMemoryTrackStreamReader::MemoryType t);
	~hkMemoryTrackStreamReader() override;

	// Whether the offset is within the track.
	hkBool* isOk(hkBool* arResult) const override {
		arResult->m_bool = m_offset <= m_track->getSize();
		return arResult;
	}

	int32_t read(void* buf, int32_t nbytes) override;
	// Moves the offset by aiNumBytes and returns aiNumBytes.
	int32_t skip(int32_t nbytes) override {
		m_offset += nbytes;
		return nbytes;
	}
	// The next four are fixed answers in the SDK: no marks, no seeking (the failures return 1).
	hkBool* markSupported(hkBool* arResult) const override;
	hkResult setMark(int32_t markLimit) override;
	hkResult rewindToMark() override;
	hkBool* seekTellSupported(hkBool* arResult) const override;
	hkResult seek(int32_t offset, hkStreamReader::SeekWhence whence) override;
	int32_t tell() const override { return m_offset; }

	const hkMemoryTrack* m_track;
	int32_t m_offset;
	MemoryType m_memType;
};

// A stream writer that appends to an hkArray<char>, which it owns or borrows. The vtable is the exe's.
class hkArrayStreamWriter : public hkStreamWriter {
public:
	enum ArrayOwnership : int32_t {
		ARRAY_TAKE = 0,
		ARRAY_BORROW = 1,
	};

	// A writer with no array, safe to destroy: the sites declare the local, then construct it again in place.
	hkArrayStreamWriter() : m_arr(nullptr), m_offset(0), m_ownerShip(ARRAY_BORROW) {}
	hkArrayStreamWriter(hkArray<char, hkContainerHeapAllocator>* arr, hkArrayStreamWriter::ArrayOwnership o);
	~hkArrayStreamWriter() override;

	hkBool* isOk(hkBool* arResult) const override;
	int32_t write(const void* mem, int32_t size) override;
	hkBool* seekTellSupported(hkBool* arResult) const override;
	hkResult seek(int32_t offset, hkStreamWriter::SeekWhence whence) override;
	int32_t tell() const override { return m_offset; }
	// Slot 9, new here: empties the array.
	virtual void clear();
	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	const hkClass* getClassType() const override;

	// Puts a zero byte after the last element, outside the size.
	void nullTerminate();
	void* getData() { return m_arr->m_data; }
	int32_t getDataSize() { return m_arr->m_size; }

	hkArray<char>* m_arr;
	int32_t m_offset;
	ArrayOwnership m_ownerShip;
};

inline constexpr fnvd::Vtable hkMemoryTrackStreamWriter_vftable{};
inline constexpr fnvd::Vtable hkMemoryTrackStreamReader_vftable{};
inline constexpr fnvd::Vtable hkArrayStreamWriter_vftable{};
