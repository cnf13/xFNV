#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include "BSShader/BSShaderProperty.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/Windows/winnt.h"

int32_t NiAtomicIncrement(int32_t& aiValue) {
	return __atomic_add_fetch(&aiValue, 1, __ATOMIC_SEQ_CST);
}

uint32_t NiAtomicIncrement(uint32_t& auiValue) {
	return __atomic_add_fetch(&auiValue, 1, __ATOMIC_SEQ_CST);
}

int32_t NiAtomicDecrement(int32_t& aiValue) {
	return __atomic_sub_fetch(&aiValue, 1, __ATOMIC_SEQ_CST);
}

uint32_t NiAtomicDecrement(uint32_t& auiValue) {
	return __atomic_sub_fetch(&auiValue, 1, __ATOMIC_SEQ_CST);
}

int32_t NiStricmp(const char* s1, const char* s2) {
	return _stricmp(s1, s2);
}

uint32_t XInputGetState(uint32_t dwUserIndex, _XINPUT_STATE* pXInputState) {
	return fnvd::imports::XInputGetState(dwUserIndex, pXInputState);
}

int32_t NiStrnicmp(const char* s1, const char* s2, uint32_t n) {
	return _strnicmp(s1, s2, n);
}

float NiGetCurrentTimeInSec() {
	LARGE_INTEGER PerformanceCount;
	if (bFirst) {
		QueryPerformanceFrequency(&freq);
		QueryPerformanceCounter(&initial);
		bFirst = 0;
	}
	QueryPerformanceCounter(&PerformanceCount);
	PerformanceCount.QuadPart -= initial.QuadPart;
	*reinterpret_cast<float*>(&PerformanceCount.u.LowPart) =
		static_cast<double>(PerformanceCount.QuadPart) / static_cast<double>(freq.QuadPart);
	return *reinterpret_cast<float*>(&PerformanceCount.u.LowPart);
}

// The exe's import thunk: a `jmp [slot]` to its XINPUT1_3.dll import, so it only calls the slot and returns its
// result.
uint32_t XInputSetState(uint32_t dwUserIndex, _XINPUT_VIBRATION* pVibration) {
	return imp_XInputSetState(dwUserIndex, pVibration);
}
