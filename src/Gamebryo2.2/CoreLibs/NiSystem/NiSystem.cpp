#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include "BSShader/BSShaderProperty.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/Windows/winnt.h"

#include <atomic>

int32_t NiAtomicIncrement(int32_t& aiValue) {
    return ++std::atomic_ref(aiValue);
}

uint32_t NiAtomicIncrement(uint32_t& auiValue) {
    return ++std::atomic_ref(auiValue);
}

int32_t NiAtomicDecrement(int32_t& aiValue) {
    return --std::atomic_ref(aiValue);
}

uint32_t NiAtomicDecrement(uint32_t& auiValue) {
    return --std::atomic_ref(auiValue);
}

int32_t NiStricmp(const char* s1, const char* s2) {
    return _stricmp(s1, s2);
}

uint32_t XInputGetState(uint32_t dwUserIndex, _XINPUT_STATE* pXInputState) {
    return xfnv::imports::XInputGetState(dwUserIndex, pXInputState);
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
    *reinterpret_cast<float*>(&PerformanceCount.u.LowPart) = static_cast<double>(PerformanceCount.QuadPart) /
        static_cast<double>(freq.QuadPart);
    return *reinterpret_cast<float*>(&PerformanceCount.u.LowPart);
}

// Only calls the XINPUT1_3.dll import and returns its result.
uint32_t XInputSetState(uint32_t dwUserIndex, _XINPUT_VIBRATION* pVibration) {
    return imp_XInputSetState(dwUserIndex, pVibration);
}
