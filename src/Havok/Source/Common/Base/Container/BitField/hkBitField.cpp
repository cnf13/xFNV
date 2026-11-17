#include "Havok/Source/Common/Base/Container/BitField/hkBitField.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

static void finishLoadedObjecthkBitField(void* p, int32_t finishing);
// Frees a loaded hkBitField's words.
static void cleanupLoadedObjecthkBitField(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkBitFieldTypeInfo = {"hkBitField", finishLoadedObjecthkBitField, cleanupLoadedObjecthkBitField,
                                 nullptr,      &typeid(hkBitField),          sizeof(hkBitField)};

// Reflection of hkBitField: the words array and the bit count.
using hkBitFieldClass_MembersTable = const hkClassMember[2];
static hkBitFieldClass_MembersTable hkBitFieldClass_Members{
    {"words",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_UINT32},
     0,
     {0},
     offsetof(hkBitField, m_words),
     nullptr},
    {"numBits",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     offsetof(hkBitField, m_numBits),
     nullptr}
};
hkClass hkBitFieldClass(
    "hkBitField",
    nullptr,
    sizeof(hkBitField),
    nullptr,
    0,
    nullptr,
    0,
    hkBitFieldClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);

static void finishLoadedObjecthkBitField(void* p, int32_t finishing) {
    ;
}

// The type info's cleanup callback for hkBitField: runs its destructor (m_words' clearAndDeallocate).
static void cleanupLoadedObjecthkBitField(void* p) {
    hkBitField* pThis = static_cast<hkBitField*>(p);
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    const int32_t iCapacityAndFlags = pThis->m_words.getCapacityAndFlags();
    pThis->m_words.m_size = 0;
    if (iCapacityAndFlags >= 0) {
        pHeap->bufFree(
            pThis->m_words.m_data, (iCapacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK) * sizeof(uint32_t)
        );
    }
    pThis->m_words.m_capacityAndFlags = hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG;
    pThis->m_words.m_data = nullptr;
}

int32_t hkBitField::fillUnusedBits(int32_t a1) {
    int32_t m_numBits = this->m_numBits;
    int32_t v3 = (m_numBits - 1) >> 5;
    uint32_t* v4 = &m_words.m_data[v3];
    int32_t result = 32 * v3;
    int32_t v6 = m_numBits - result;
    if (v6 < 32) {
        result = -1 << v6;
        if (a1) {
            *v4 |= result;
        }
        else {
            result = ~result;
            *v4 &= result;
        }
    }
    return result;
}

// Before growing a non-empty field, the unused bits of its last word take fillValue (fillUnusedBits); then
// m_words.setSize(numWords, fill) and the new bit count.
void hkBitField::setSize(int32_t numBits, int32_t fillValue) {
    if (m_numBits > 0 && m_numBits < numBits) {
        fillUnusedBits(fillValue);
    }
    const uint32_t fill = fillValue ? 0xFFFFFFFFu : 0u;
    const int32_t numWords = (numBits + 31) >> 5;
    // EXE-INLINED none: hkArray<uint32_t, hkContainerHeapAllocator>::setSize(int, const uint32_t&)
    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
    const int32_t capacity = m_words.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK;
    if (capacity < numWords) {
        const int32_t cap2 = 2 * capacity;
        hkArrayUtil::_reserve(heap, &m_words, numWords < cap2 ? cap2 : numWords, sizeof(uint32_t));
    }
    for (int32_t i = m_words.m_size; i < numWords; ++i) {
        m_words.m_data[i] = fill;
    }
    m_words.m_size = numWords;
    m_numBits = numBits;
}

int32_t hkBitField::countBits(const uint32_t* words, int32_t numWords) {
    int32_t v2 = 0;
    int32_t v3 = 0;
    int32_t v4 = 0;
    uint32_t v11 = 0;
    if (numWords >= 2) {
        do {
            uint32_t v5 = (words[v4] & 0x55555555) + (words[v4] >> 1 & 0x55555555);
            uint32_t v6 = (((v5 & 0x33333333) + (v5 >> 2 & 0x33333333) & 0xF0F0F0F) +
                               (((v5 & 0x33333333) + (v5 >> 2 & 0x33333333)) >> 4 & 0xF0F0F0F) &
                           0xF000F) +
                ((((v5 & 0x33333333) + (v5 >> 2 & 0x33333333) & 0xF0F0F0F) +
                  (((v5 & 0x33333333) + (v5 >> 2 & 0x33333333)) >> 4 & 0xF0F0F0F)) >>
                     8 &
                 0xF000F);
            v3 += static_cast<uint8_t>(v6) + static_cast<uint16_t>(v6 >> 16);
            uint32_t v7 = (words[v4 + 1] & 0x55555555) + (words[v4 + 1] >> 1 & 0x55555555);
            v2 += static_cast<uint8_t>(
                      (((v7 & 51) + (v7 >> 2 & 51) & 15) + (((v7 & 0x33333333) + (v7 >> 2 & 0x33333333)) >> 4 & 15) &
                       15) +
                      (static_cast<uint16_t>(
                           ((v7 & 0x3333) + (v7 >> 2 & 0x3333) & 3855) +
                           (((v7 & 0x33333333) + (v7 >> 2 & 0x33333333)) >> 4 & 3855)
                       ) >> 8 &
                       15)
                  ) +
                (((((v7 & 0x33333333) + (v7 >> 2 & 0x33333333) & 0xF0F0F0F) +
                       (((v7 & 0x33333333) + (v7 >> 2 & 0x33333333)) >> 4 & 0xF0F0F0F) &
                   0xF000F) +
                  ((((v7 & 0x33333333) + (v7 >> 2 & 0x33333333) & 0xF0F0F0F) +
                    (((v7 & 0x33333333) + (v7 >> 2 & 0x33333333)) >> 4 & 0xF0F0F0F)) >>
                       8 &
                   0xF000F)) >>
                 16);
            v4 += 2;
        }
        while (v4 < numWords - 1);
    }
    if (v4 < numWords) {
        uint32_t v8 = words[v4];
        uint32_t v9 = (((v8 & 0x55555555) + (v8 >> 1 & 0x55555555) & 0x33333333) +
                           (((v8 & 0x55555555) + (v8 >> 1 & 0x55555555)) >> 2 & 0x33333333) &
                       0xF0F0F0F) +
            ((((v8 & 0x55555555) + (v8 >> 1 & 0x55555555) & 0x33333333) +
              (((v8 & 0x55555555) + (v8 >> 1 & 0x55555555)) >> 2 & 0x33333333)) >>
                 4 &
             0xF0F0F0F);
        v11 = static_cast<uint8_t>((v9 & 15) + (static_cast<uint8_t>(v9 >> 8) & 15)) +
            (((v9 & 0xF000F) + (v9 >> 8 & 0xF000F)) >> 16);
    }
    return v11 + (v3 + v2);
}
