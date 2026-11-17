#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStreamMacro.h"
#include "Reconstruction.hpp"

// Typed transfers on the stream's input (load) or output (save) byte stream; see NiBinaryStreamMacro.h.
template <class T> void NiStreamLoadBinary(NiStream& arStream, T& arValue) {
	NiStreamLoadBinary(*arStream.Istr(), arValue);
}

template <class T> void NiStreamLoadBinary(NiStream& arStream, T* apValue, uint32_t auiNumEls) {
	NiStreamLoadBinary(*arStream.Istr(), apValue, auiNumEls);
}

template <class T> void NiStreamSaveBinary(NiStream& arStream, const T& arValue) {
	NiStreamSaveBinary(*arStream.Ostr(), arValue);
}

template <class T> void NiStreamSaveBinary(NiStream& arStream, const T* apValue, uint32_t auiNumEls) {
	NiStreamSaveBinary(*arStream.Ostr(), apValue, auiNumEls);
}
