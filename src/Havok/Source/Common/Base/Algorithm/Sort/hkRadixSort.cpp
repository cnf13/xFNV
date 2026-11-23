#include "Havok/Source/Common/Base/Algorithm/Sort/hkRadixSort.h"

#include <xmmintrin.h>

// Two counting and scattering passes over 16-bit records, four records at a time: the low key byte scatters data into
// buffer and the high key byte scatters buffer back into data. Counts and scatters in blocks of four, so a count that
// is not a multiple of four touches the entries past the last one; this is deliberate.
void hkRadixSort::sort16(SortData16* data, int32_t numObjects, SortData16* buffer) {
    uint32_t counters0[256] = {};
    uint32_t counters1[256] = {};
    SortData16* ptr0[256];
    SortData16* ptr1[256];

    for (int32_t i = 0; i < numObjects; i += 4) {
        ++counters0[data[i].m_keys[0]];
        ++counters1[data[i].m_keys[1]];
        ++counters0[data[i + 1].m_keys[0]];
        ++counters1[data[i + 1].m_keys[1]];
        _mm_prefetch(reinterpret_cast<const char*>(data + i + 256), _MM_HINT_NTA);
        _mm_prefetch(reinterpret_cast<const char*>(buffer + i + 256), _MM_HINT_NTA);
        ++counters0[data[i + 2].m_keys[0]];
        ++counters1[data[i + 2].m_keys[1]];
        ++counters0[data[i + 3].m_keys[0]];
        ++counters1[data[i + 3].m_keys[1]];
    }
    ptr1[0] = buffer;
    ptr0[0] = data;
    for (int32_t i = 1; i < 256; ++i) {
        ptr1[i] = ptr1[i - 1] + counters0[i - 1];
        ptr0[i] = ptr0[i - 1] + counters1[i - 1];
    }
    for (int32_t i = 0; i < numObjects; i += 4) {
        *ptr1[data[i].m_keys[0]]++ = data[i];
        *ptr1[data[i + 1].m_keys[0]]++ = data[i + 1];
        *ptr1[data[i + 2].m_keys[0]]++ = data[i + 2];
        *ptr1[data[i + 3].m_keys[0]]++ = data[i + 3];
    }
    for (int32_t i = 0; i < numObjects; i += 4) {
        *ptr0[buffer[i].m_keys[1]]++ = buffer[i];
        *ptr0[buffer[i + 1].m_keys[1]]++ = buffer[i + 1];
        *ptr0[buffer[i + 2].m_keys[1]]++ = buffer[i + 2];
        *ptr0[buffer[i + 3].m_keys[1]]++ = buffer[i + 3];
    }
}

// Four counting and scattering passes over 32-bit records, four records at a time: data scatters into buffer by key
// byte 0, buffer into data by byte 1, data into buffer by byte 2 and buffer into data by byte 3, so the result ends in
// data. As in sort16, it works in blocks of four and touches the entries past the last one when the count is not a
// multiple of four.
void hkRadixSort::sort32(SortData32* data, int32_t numObjects, SortData32* buffer) {
    uint32_t counters0[256] = {};
    uint32_t counters1[256] = {};
    uint32_t counters2[256] = {};
    uint32_t counters3[256] = {};
    SortData32* ptr0[256];
    SortData32* ptr1[256];
    SortData32* ptr2[256];
    SortData32* ptr3[256];

    for (int32_t i = 0; i < numObjects; i += 4) {
        ++counters0[data[i].m_keys[0]];
        ++counters1[data[i].m_keys[1]];
        ++counters2[data[i].m_keys[2]];
        ++counters3[data[i].m_keys[3]];
        ++counters0[data[i + 1].m_keys[0]];
        ++counters1[data[i + 1].m_keys[1]];
        ++counters2[data[i + 1].m_keys[2]];
        ++counters3[data[i + 1].m_keys[3]];
        _mm_prefetch(reinterpret_cast<const char*>(data + i + 128), _MM_HINT_NTA);
        _mm_prefetch(reinterpret_cast<const char*>(buffer + i + 128), _MM_HINT_NTA);
        ++counters0[data[i + 2].m_keys[0]];
        ++counters1[data[i + 2].m_keys[1]];
        ++counters2[data[i + 2].m_keys[2]];
        ++counters3[data[i + 2].m_keys[3]];
        ++counters0[data[i + 3].m_keys[0]];
        ++counters1[data[i + 3].m_keys[1]];
        ++counters2[data[i + 3].m_keys[2]];
        ++counters3[data[i + 3].m_keys[3]];
    }
    ptr3[0] = buffer;
    ptr2[0] = data;
    ptr1[0] = buffer;
    ptr0[0] = data;
    for (int32_t i = 1; i < 256; ++i) {
        ptr3[i] = ptr3[i - 1] + counters0[i - 1];
        ptr2[i] = ptr2[i - 1] + counters1[i - 1];
        ptr1[i] = ptr1[i - 1] + counters2[i - 1];
        ptr0[i] = ptr0[i - 1] + counters3[i - 1];
    }
    for (int32_t i = 0; i < numObjects; i += 4) {
        *ptr3[data[i].m_keys[0]]++ = data[i];
        *ptr3[data[i + 1].m_keys[0]]++ = data[i + 1];
        *ptr3[data[i + 2].m_keys[0]]++ = data[i + 2];
        *ptr3[data[i + 3].m_keys[0]]++ = data[i + 3];
    }
    for (int32_t i = 0; i < numObjects; i += 4) {
        *ptr2[buffer[i].m_keys[1]]++ = buffer[i];
        *ptr2[buffer[i + 1].m_keys[1]]++ = buffer[i + 1];
        *ptr2[buffer[i + 2].m_keys[1]]++ = buffer[i + 2];
        *ptr2[buffer[i + 3].m_keys[1]]++ = buffer[i + 3];
    }
    for (int32_t i = 0; i < numObjects; i += 4) {
        *ptr1[data[i].m_keys[2]]++ = data[i];
        *ptr1[data[i + 1].m_keys[2]]++ = data[i + 1];
        *ptr1[data[i + 2].m_keys[2]]++ = data[i + 2];
        *ptr1[data[i + 3].m_keys[2]]++ = data[i + 3];
    }
    for (int32_t i = 0; i < numObjects; i += 4) {
        *ptr0[buffer[i].m_keys[3]]++ = buffer[i];
        *ptr0[buffer[i + 1].m_keys[3]]++ = buffer[i + 1];
        *ptr0[buffer[i + 2].m_keys[3]]++ = buffer[i + 2];
        *ptr0[buffer[i + 3].m_keys[3]]++ = buffer[i + 3];
    }
}
