#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Reconstruction.hpp"

// Typed transfers: auiNumEls elements of sizeof(T) bytes, each element one component (a swapping stream swaps each
// element as a whole). Types made of several components (NiPoint3, NiColorA...) stream through their own members.
template <class T> void NiBinaryStreamLoad(NiBinaryStream& arStream, T* apValue, uint32_t auiNumEls) {
	uint32_t uiSize = sizeof(T);
	arStream.BinaryRead(apValue, sizeof(T) * auiNumEls, &uiSize, 1);
}

template <class T> void NiBinaryStreamSave(NiBinaryStream& os, const T* pValue, uint32_t uiNumEls) {
	uint32_t uiSize = sizeof(T);
	os.BinaryWrite(pValue, sizeof(T) * uiNumEls, &uiSize, 1);
}

template <class T> void NiStreamLoadBinary(NiBinaryStream& arStream, T& arValue) {
	NiBinaryStreamLoad(arStream, &arValue, 1);
}

template <class T> void NiStreamLoadBinary(NiBinaryStream& arStream, T* apValue, uint32_t auiNumEls) {
	NiBinaryStreamLoad(arStream, apValue, auiNumEls);
}

template <class T> void NiStreamSaveBinary(NiBinaryStream& arStream, const T& arValue) {
	NiBinaryStreamSave(arStream, &arValue, 1);
}

template <class T> void NiStreamSaveBinary(NiBinaryStream& arStream, const T* apValue, uint32_t auiNumEls) {
	NiBinaryStreamSave(arStream, apValue, auiNumEls);
}
