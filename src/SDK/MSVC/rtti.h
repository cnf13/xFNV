#pragma once

#include <cstdint>

// The vcruntime's own __RTDynamicCast (rttidata.h, which is not a public header), which rt::__RTDynamicCast forwards
// to. Declared with C linkage here so that its name does not clash with rt's.
namespace fnvd::imports {
extern "C" void* __RTDynamicCast(void* apObject, int32_t aiVfDelta, const void* apSrcType,
	const void* apDstType, int32_t abIsReference);
} // namespace fnvd::imports

// rt: runtime support the game relies on. Here the VC9 CRT's RTTI support (crtw32\eh\rtti.cpp in the PDB's CRT
// modules). It goes when the classes use dynamic_cast.
namespace rt {

// Not reconstructed yet: __RTDynamicCast, 0xEC43FB (cdecl), the code behind dynamic_cast. The type descriptors are
// the exe's (BSCore/RTTI.h). Returns the cast object, or null.
inline void* __RTDynamicCast(void* apObject, int32_t aiVfDelta, const void* apSrcType, const void* apDstType,
	int32_t abIsReference)
{
	return fnvd::imports::__RTDynamicCast(apObject, aiVfDelta, apSrcType, apDstType, abIsReference);
}

} // namespace rt
