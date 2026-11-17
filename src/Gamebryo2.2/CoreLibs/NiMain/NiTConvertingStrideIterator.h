#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// Walks the elements of a vertex stream m_iStride bytes apart; m_bCompressed marks a packed stream whose elements
// are converted to T on access. The PDB's debug bounds members are not in the release build.
template <class T> class NiTConvertingStrideIterator {
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
	// The address of the current element. The exe reads it through its shared one-field getter 0x559450 (folded with
	// NiPointer::operator->); SWEEP-INLINE at each site.
	void* GetCurrentPosition() const { return m_pvCurrentPosition; }

	void* m_pvCurrentPosition;
	uint32_t m_iStride;
	bool m_bCompressed;
};

// Stores arData at the current element (NiPoint3's 0x6533F0: a plain copy).
template <class T> void NiTConvertingStrideIterator<T>::SetData(const T& arData) {
	*static_cast<T*>(m_pvCurrentPosition) = arData;
}

// Copies the current element to arData (NiPoint3's 0x656FD0).
template <class T> void NiTConvertingStrideIterator<T>::Get_(T& arData) const {
	arData = *static_cast<const T*>(m_pvCurrentPosition);
}

// Steps to the next element, m_iStride bytes on (NiPoint3's 0x657BE0).
template <class T> NiTConvertingStrideIterator<T>& NiTConvertingStrideIterator<T>::Next() {
	m_pvCurrentPosition = static_cast<char*>(m_pvCurrentPosition) + m_iStride;
	return *this;
}

// Reads element aiIndex into arData as stored, whatever m_bCompressed says (NiPoint3's 0x534570 copies the 12 bytes
// without testing it).
template <class T> void NiTConvertingStrideIterator<T>::Get(int32_t aiIndex, T& arData) {
	// The stream is raw vertex memory the exe itself walks as untyped bytes.
	uint8_t* pElement = static_cast<uint8_t*>(m_pvCurrentPosition) + m_iStride * static_cast<uint32_t>(aiIndex);
	arData = *reinterpret_cast<T*>(pElement);
}

// NiColorA's copy unpacks a compressed stream's colour dword; defined in NiTConvertingStrideIterator.cpp.
template <> void NiTConvertingStrideIterator<NiColorA>::Get(int32_t aiIndex, NiColorA& arData);
