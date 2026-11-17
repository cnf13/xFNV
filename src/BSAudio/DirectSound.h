#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Reconstruction.hpp"
// DirectSound's interfaces, structures and import (the SDK's dsound.h), which the PC audio classes use.
#include "SDK/DirectX/dsound.h"

// A wave file's smpl chunk with its first loop (IDA name; not in the PDB): the loop's start and end are the sample
// offsets an envelope sound loops between.
struct SamplerChunk {
	struct SampleLoop {
		uint32_t uiCuePointID;
		uint32_t uiType;
		uint32_t uiStart;
		uint32_t uiEnd;
		uint32_t uiFraction;
		uint32_t uiPlayCount;
	};

	uint32_t uiManufacturer;
	uint32_t uiProduct;
	uint32_t uiSamplePeriod;
	uint32_t uiMIDIUnityNote;
	uint32_t uiMIDIPitchFraction;
	uint32_t uiSMPTEFormat;
	uint32_t uiSMPTEOffset;
	uint32_t uiNumSampleLoops;
	uint32_t uiSamplerData;
	SampleLoop kLoop[1];
};

// dsound.dll's DirectSoundCreate8, which the exe calls through its import thunk (0xEE81E0: jmp [0xFDF034]).
inline HRESULT DirectSoundCreate8(const GUID* pcGuidDevice, IDirectSound8** ppDS8, IUnknown* pUnkOuter) {
	return fnvd::imports::DirectSoundCreate8(pcGuidDevice, ppDS8, pUnkOuter);
}
