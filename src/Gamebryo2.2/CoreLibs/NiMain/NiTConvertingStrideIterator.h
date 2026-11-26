#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"

#include <stdint.h>

// Walks the elements of a vertex stream m_iStride bytes apart; m_bCompressed marks a packed stream whose elements are
// converted to T on access. The debug bounds members are not declared because the release build does not have them.
template <class T>
class NiTConvertingStrideIterator {
public:
    // An iterator positioned on nothing.
    NiTConvertingStrideIterator() : m_pvCurrentPosition(nullptr), m_iStride(0), m_bCompressed(false) {}

    // Stores arData at the current element. Defined per element type.
    void SetData(const T& arData);
    // Copies the current element to arData. Defined per element type.
    void Get_(T& arData) const;
    // Steps to the next element, m_iStride bytes on. Defined per element type.
    NiTConvertingStrideIterator& Next();
    // Reads element aiIndex into arData. Defined per element type.
    void Get(int32_t aiIndex, T& arData);
    void* GetCurrentPosition() const {
        return m_pvCurrentPosition;
    }

    void* m_pvCurrentPosition;
    uint32_t m_iStride;
    bool m_bCompressed;
};

// Stores arData at the current element as a plain copy.
template <class T>
void NiTConvertingStrideIterator<T>::SetData(const T& arData) {
    *static_cast<T*>(m_pvCurrentPosition) = arData;
}

template <class T>
void NiTConvertingStrideIterator<T>::Get_(T& arData) const {
    arData = *static_cast<const T*>(m_pvCurrentPosition);
}

// Steps to the next element, m_iStride bytes on.
template <class T>
NiTConvertingStrideIterator<T>& NiTConvertingStrideIterator<T>::Next() {
    m_pvCurrentPosition = static_cast<char*>(m_pvCurrentPosition) + m_iStride;
    return *this;
}

// Reads element aiIndex into arData as stored, whatever m_bCompressed says.
template <class T>
void NiTConvertingStrideIterator<T>::Get(int32_t aiIndex, T& arData) {
    // The stream is raw vertex memory walked as untyped bytes.
    uint8_t* pElement = static_cast<uint8_t*>(m_pvCurrentPosition) + m_iStride * static_cast<uint32_t>(aiIndex);
    arData = *reinterpret_cast<T*>(pElement);
}

// NiColorA's Get differs from the generic one: a compressed stream packs the colour in one 32-bit value (B in the low
// byte, then G, R, A), each channel scaled by 1/255.
template <>
inline void NiTConvertingStrideIterator<NiColorA>::Get(int32_t aiIndex, NiColorA& arData) {
    // The stream is raw vertex memory walked as untyped bytes.
    uint8_t* pElement = static_cast<uint8_t*>(m_pvCurrentPosition) + m_iStride * static_cast<uint32_t>(aiIndex);
    if (m_bCompressed) {
        uint32_t uiStoredVal = *reinterpret_cast<uint32_t*>(pElement);
        uint8_t ucR = static_cast<uint8_t>(uiStoredVal >> 16);
        uint8_t ucG = static_cast<uint8_t>(uiStoredVal >> 8);
        uint8_t ucB = static_cast<uint8_t>(uiStoredVal);
        uint8_t ucA = static_cast<uint8_t>(uiStoredVal >> 24);
        arData.r = static_cast<double>(ucR) / 255.0;
        arData.g = static_cast<double>(ucG) / 255.0;
        arData.b = static_cast<double>(ucB) / 255.0;
        arData.a = static_cast<double>(ucA) / 255.0;
    }
    else {
        arData = *reinterpret_cast<NiColorA*>(pElement);
    }
}
