#include "Havok/Source/Common/Base/System/Io/Writer/Array/hkArrayStreamWriter.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "SDK/MSVC/string.h"

hkClass hkArrayStreamWriterClass("hkArrayStreamWriter", nullptr, 20, nullptr, 0, nullptr, 0, nullptr, 0, nullptr,
	nullptr, 0, 0);

hkMemoryTrack::hkMemoryTrack(int32_t numBytesPerSector)
	: m_numBytesPerSector(numBytesPerSector), m_numBytesLastSector(numBytesPerSector) {}

hkMemoryTrack::~hkMemoryTrack() {
	clear();
}

void hkMemoryTrack::clear() {
	for (int32_t i = 0; i < m_sectors.m_size; ++i) {
		int32_t m_numBytesPerSector = this->m_numBytesPerSector;
		char* v4 = m_sectors.m_data[i];
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		Instance->m_heap->blockFree(v4, m_numBytesPerSector);
	}
	hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t m_capacityAndFlags = m_sectors.m_capacityAndFlags;
	m_sectors.m_size = 0;
	if (m_capacityAndFlags >= 0) {
		m_heap->bufFree(m_sectors.begin(), sizeof(char*) * m_capacityAndFlags);
	}
	m_sectors.m_data = 0;
	m_sectors.m_capacityAndFlags = hkArrayBase<char*>::DONT_DEALLOCATE_FLAG;
	m_numBytesLastSector = this->m_numBytesPerSector;
}

// Fills the last sector, then appends a new one from the heap each time it is full.
void hkMemoryTrack::write(const void* data, int32_t numBytes) {
	const char* apSrc = static_cast<const char*>(data);
	int32_t aiSize = numBytes;
	int32_t v13 = m_sectors.m_size - 1;
	if (aiSize > 0) {
		char* v12;
		hkArray<char*, hkContainerHeapAllocator>* p_m_sectors = &m_sectors;
		while (1) {
			int32_t m_numBytesPerSector = this->m_numBytesPerSector;
			int32_t v7 = this->m_numBytesPerSector - m_numBytesLastSector;
			if (this->m_numBytesPerSector == m_numBytesLastSector) {
				hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
				char* v9 = static_cast<char*>(Instance->m_heap->blockAlloc(m_numBytesPerSector));
				hkMemoryRouter* v10 = &hkMemoryRouter::getInstance();
				if (m_sectors.m_size == (m_sectors.m_capacityAndFlags & hkArrayBase<char*>::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*v10->m_heap, &m_sectors, sizeof(char*));
				}
				char** m_data = p_m_sectors->begin();
				m_sectors.m_data[m_sectors.m_size++] = v9;
				++v13;
				v7 = this->m_numBytesPerSector;
				m_numBytesLastSector = 0;
			}
			v12 = &p_m_sectors->m_data[v13][m_numBytesLastSector];
			if (aiSize <= v7) {
				break;
			}
			hkString::memCpy(v12, apSrc, v7);
			apSrc += v7;
			aiSize -= v7;
			m_numBytesLastSector = this->m_numBytesPerSector;
			if (aiSize <= 0) {
				return;
			}
		}
		hkString::memCpy(v12, apSrc, aiSize);
		m_numBytesLastSector += aiSize;
	}
}

void hkMemoryTrack::appendByMove(hkMemoryTrack* apOther) {
	int32_t v4 = 0;
	int32_t other = apOther->m_sectors.m_size;
	int32_t v3 = other;
	if (other - 1 > 0) {
		do {
			char* v5 = apOther->m_sectors.m_data[v4];
			write(v5, apOther->m_numBytesPerSector);
			int32_t m_numBytesPerSector = apOther->m_numBytesPerSector;
			hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
			Instance->m_heap->blockFree(v5, m_numBytesPerSector);
			++v4;
		} while (v4 < other - 1);
		v3 = other;
	}
	if (v3) {
		char* v8 = apOther->m_sectors.m_data[v3 - 1];
		write(v8, apOther->m_numBytesLastSector);
		int32_t v9 = apOther->m_numBytesPerSector;
		hkMemoryRouter* v10 = &hkMemoryRouter::getInstance();
		v10->m_heap->blockFree(v8, v9);
	}
	hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t m_capacityAndFlags = apOther->m_sectors.m_capacityAndFlags;
	apOther->m_sectors.m_size = 0;
	if (m_capacityAndFlags >= 0) {
		m_heap->bufFree(apOther->m_sectors.begin(), sizeof(char*) * m_capacityAndFlags);
	}
	apOther->m_sectors.m_data = 0;
	apOther->m_sectors.m_capacityAndFlags = hkArrayBase<char*>::DONT_DEALLOCATE_FLAG;
	apOther->clear();
}

// Reads from the sector holding offset, each sector's bytes in turn (the last one only up to its fill).
void hkMemoryTrack::read(void* data, int32_t numBytes, int32_t offset) const {
	char* apDest = static_cast<char*>(data);
	int32_t v6 = numBytes;
	int32_t v7 = offset / m_numBytesPerSector;
	int32_t v8 = offset % m_numBytesPerSector;
	if (numBytes > 0) {
		char* v11;
		while (1) {
			int32_t v9 = v7 >= m_sectors.m_size - 1 ? m_numBytesLastSector : m_numBytesPerSector;
			int32_t v10 = v9 - v8;
			v11 = &m_sectors.m_data[v7][v8];
			if (v6 <= v10) {
				break;
			}
			hkString::memCpy(apDest, v11, v10);
			apDest += v10;
			v6 -= v10;
			++v7;
			v8 = 0;
			if (v6 <= 0) {
				return;
			}
		}
		hkString::memCpy(apDest, v11, v6);
	}
}

// The SDK's write: the bytes go to the end of the track; returns the count.
int32_t hkMemoryTrackStreamWriter::write(const void* mem, int32_t size) {
	m_track->write(mem, size);
	return size;
}

hkMemoryTrackStreamReader::hkMemoryTrackStreamReader(const hkMemoryTrack* track,
	hkMemoryTrackStreamReader::MemoryType t)
	: m_track(track), m_offset(0), m_memType(t) {}

hkMemoryTrackStreamReader::~hkMemoryTrackStreamReader() {
	if (m_memType == MEMORY_TAKE) {
		hkMemoryTrack* pTrack = const_cast<hkMemoryTrack*>(m_track);
		if (pTrack) {
			pTrack->~hkMemoryTrack();
			hkMemoryRouter::getInstance().m_heap->blockFree(pTrack, sizeof(hkMemoryTrack));
		}
	}
}

// The SDK's read: at most the bytes left after m_offset, through the track's read; the offset moves by the count
// asked for, as in the SDK.
int32_t hkMemoryTrackStreamReader::read(void* buf, int32_t nbytes) {
	hkBool bOk;
	if (!isOk(&bOk)->m_bool) {
		return 0;
	}
	int32_t iOffset = m_offset;
	int32_t iAvailable = m_track->getSize() - iOffset;
	int32_t iNumBytes = nbytes < iAvailable ? nbytes : iAvailable;
	m_track->read(buf, iNumBytes, iOffset);
	m_offset += nbytes;
	return iNumBytes;
}

hkBool* hkMemoryTrackStreamReader::markSupported(hkBool* arResult) const {
	arResult->m_bool = 0;
	return arResult;
}

hkResult hkMemoryTrackStreamReader::setMark(int32_t markLimit) {
	return HK_FAILURE;
}

hkResult hkMemoryTrackStreamReader::rewindToMark() {
	return HK_FAILURE;
}

hkBool* hkMemoryTrackStreamReader::seekTellSupported(hkBool* arResult) const {
	arResult->m_bool = 0;
	return arResult;
}

hkResult hkMemoryTrackStreamReader::seek(int32_t offset, hkStreamReader::SeekWhence whence) {
	return HK_FAILURE;
}

hkArrayStreamWriter::hkArrayStreamWriter(hkArray<char, hkContainerHeapAllocator>* arr,
	hkArrayStreamWriter::ArrayOwnership o)
	: m_arr(arr), m_offset(arr->m_size), m_ownerShip(o) {
	// Room for a zero byte after the last element, growing the capacity at least twofold.
	int32_t iNeeded = arr->m_size + 1;
	int32_t iCapacity = arr->m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK;
	if (iCapacity < iNeeded) {
		if (iNeeded < 2 * iCapacity) {
			iNeeded = 2 * iCapacity;
		}
		hkArrayUtil::_reserve(*hkMemoryRouter::getInstance().m_heap, arr, iNeeded, sizeof(char));
	}
	m_arr->begin()[m_arr->m_size] = 0;
}

hkArrayStreamWriter::~hkArrayStreamWriter() {
	if (m_ownerShip == ARRAY_TAKE && m_arr) {
		m_arr->clearAndDeallocate();
		hkMemoryRouter::getInstance().m_heap->blockFree(m_arr, sizeof(hkArray<char>));
	}
}

hkBool* hkArrayStreamWriter::isOk(hkBool* arResult) const {
	arResult->m_bool = 1;
	return arResult;
}

int32_t hkArrayStreamWriter::write(const void* mem, int32_t size) {
	int32_t v3 = size;
	hkArray<char, hkContainerHeapAllocator>* m_arr = this->m_arr;
	int32_t m_size = m_arr->m_size;
	int32_t v7 = m_size - m_offset;
	if (size <= v7) {
		if ((m_arr->m_capacityAndFlags & 0x3FFFFFFF) > m_size) {
			m_arr->m_data[m_size] = 0;
		}
	} else {
		int32_t v8 = size + (m_size - v7);
		hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
		int32_t v10 = m_arr->m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK;
		if (v10 < v8 + 1) {
			int32_t v11 = 2 * v10;
			if (v8 + 1 >= v11) {
				v11 = v8 + 1;
			}
			hkArrayUtil::_reserve(m_heap, m_arr, v11, sizeof(char));
		}
		v3 = size;
		this->m_arr->m_size = v8;
		this->m_arr->m_data[v8] = 0;
	}
	hkString::memCpy(&this->m_arr->m_data[m_offset], mem, v3);
	m_offset += v3;
	return v3;
}

hkBool* hkArrayStreamWriter::seekTellSupported(hkBool* arResult) const {
	arResult->m_bool = 1;
	return arResult;
}

hkResult hkArrayStreamWriter::seek(int32_t offset, hkStreamWriter::SeekWhence whence) {
	int32_t m_offset = this->m_offset;
	if (whence) {
		if (whence == hkStreamWriter::STREAM_CUR) {
			m_offset += offset;
		} else {
			if (whence == hkStreamWriter::STREAM_END) {
				m_offset = this->m_arr->m_size - offset;
			}
		}
	} else {
		m_offset = offset;
	}
	if (m_offset < 0) {
		return HK_FAILURE;
	}
	hkArray<char, hkContainerHeapAllocator>* m_arr = this->m_arr;
	if (m_offset > m_arr->m_size) {
		int32_t v6 = m_offset + 1;
		hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
		int32_t v8 = m_arr->m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK;
		if (v8 < m_offset + 1) {
			int32_t v9 = 2 * v8;
			if (v6 >= v9) {
				v9 = m_offset + 1;
			}
			hkArrayUtil::_reserve(*m_heap, m_arr, v9, sizeof(char));
		}
		int32_t m_size = m_arr->m_size;
		if (v6 - m_size > 0) {
			std::memset(&m_arr->m_data[m_size], 0, v6 - m_size);
		}
		m_arr->m_size = v6;
		this->m_arr->m_size = m_offset;
	}
	this->m_offset = m_offset;
	return HK_SUCCESS;
}

void hkArrayStreamWriter::clear() {
	this->m_arr->m_size = 0;
	hkArray<char, hkContainerHeapAllocator>* m_arr = this->m_arr;
	m_offset = 0;
	int32_t v3 = m_arr->m_size + 1;
	hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
	int32_t v5 = m_arr->m_capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK;
	if (v5 < v3) {
		int32_t v6 = 2 * v5;
		if (v3 < v6) {
			v3 = v6;
		}
		hkArrayUtil::_reserve(m_heap, m_arr, v3, sizeof(char));
	}
	this->m_arr->m_data[this->m_arr->m_size] = 0;
}

void hkArrayStreamWriter::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
	hkArray<char, hkContainerHeapAllocator>* m_arr = this->m_arr;
	if (m_arr) {
		int32_t m_capacityAndFlags = m_arr->m_capacityAndFlags;
		if (m_capacityAndFlags >= 0 || collector->m_includeUnmanagedArrayMemory) {
			int32_t AllocatedSize =
				hkStatisticsCollector::getAllocatedSize(m_arr->begin(), m_capacityAndFlags & 0x3FFFFFFF);
			collector->addChunk(hkStatisticsCollector::MEM_ARRAY, "array", m_arr->begin(), m_arr->m_size,
				AllocatedSize);
		}
	}
}
