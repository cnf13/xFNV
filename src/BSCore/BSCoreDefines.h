#pragma once

#include <bit>
#include <cstdint>
#include <cstring>
#include <type_traits>

// The engine's byte swap of a value read from a big-endian file. Callers pass a second, bool argument that the body
// never reads.
// A pointer holds the 4-byte form ID Load read until InitItem resolves it, so its four low bytes are swapped.
template <class T>
void BSEndian(T& arValue, bool abUnused) {
    if constexpr (std::is_pointer_v<T>) {
        // The pointer field still holds the untyped form ID that was read into it, not an object's address.
        uint32_t uiFormID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arValue));
        arValue = reinterpret_cast<T>(static_cast<uintptr_t>(std::byteswap<uint32_t>(uiFormID)));
    }
    else if constexpr (sizeof(T) == 2) {
        uint16_t usValue;
        std::memcpy(&usValue, &arValue, sizeof(usValue));
        usValue = std::byteswap<uint16_t>(usValue);
        std::memcpy(&arValue, &usValue, sizeof(usValue));
    }
    else if constexpr (sizeof(T) == 4) {
        uint32_t uiValue;
        std::memcpy(&uiValue, &arValue, sizeof(uiValue));
        uiValue = std::byteswap<uint32_t>(uiValue);
        std::memcpy(&arValue, &uiValue, sizeof(uiValue));
    }
    else {
        static_assert(sizeof(T) == 8, "BSEndian swaps 2, 4 or 8 bytes");
        uint64_t ullValue;
        std::memcpy(&ullValue, &arValue, sizeof(ullValue));
        ullValue = std::byteswap<uint64_t>(ullValue);
        std::memcpy(&arValue, &ullValue, sizeof(ullValue));
    }
}
