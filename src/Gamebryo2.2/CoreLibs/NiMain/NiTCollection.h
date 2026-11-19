#pragma once

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cstdint>
#include <new>
#include <type_traits>

class Actor;
class bhkRigidBody;
class CombatGroup;
class MobileObject;
class NiAVObject;
class NiD3DPass;
class NiD3DRenderStateGroup;
class NiD3DTextureStage;
class NiFixedString;
class NiGeometry;
class NiGeomMorpherUpdateTask;
class NiPSysUpdateTask;
class NiScreenTexture;
class TESIdleForm;
class TESObjectCELL;
class TESObjectREFR;
class TESTopicInfo;

// Element storage of the primitive containers (NiTPrimitiveArray, NiTPrimitiveSet...): raw blocks from the Gamebryo
// heap (NiMalloc/NiFree), no constructors or destructors run.
template <class T>
class NiTMallocInterface {
public:
    static T* Allocate(uint32_t uiNumElements) {
        return static_cast<T*>(_NiMalloc(sizeof(T) * uiNumElements));
    }

    static void Deallocate(T* pArray) {
        _NiFree(pArray);
    }
};
// Element storage of the object containers (NiTObjectArray, NiTObjectSet...): the engine's new[]/delete[], which is
// Bethesda's global operator new[] on MemoryManager (not NiMemObject's). An element type with a destructor gets an
// array cookie (the count, in the size_t before the elements: 4 bytes on x86, 8 on x64); a trivially destructible one
// does not.
template <class T>
class NiTNewInterface {
public:
    static T* Allocate(uint32_t uiNumElements) {
        constexpr bool bCookie = !std::is_trivially_destructible_v<T>;
        constexpr size_t stCookieSize = bCookie ? sizeof(size_t) : 0;
        // new[] saturates the size on overflow so the allocation fails rather than comes back short.
        uint64_t ulSize = static_cast<uint64_t>(sizeof(T)) * uiNumElements + stCookieSize;
        size_t stSize = ulSize > SIZE_MAX ? SIZE_MAX : static_cast<size_t>(ulSize);
        uint8_t* pucMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(stSize));
        if (!pucMemory) {
            return nullptr;
        }
        if constexpr (bCookie) {
            pucMemory += stCookieSize;
            reinterpret_cast<size_t*>(pucMemory)[-1] = uiNumElements;
        }
        T* pArray = reinterpret_cast<T*>(pucMemory);
        // new[]'s loop counts down a signed copy of the count: a count with the top bit set constructs nothing.
        T* pElement = pArray;
        for (int32_t iCount = static_cast<int32_t>(uiNumElements); --iCount >= 0; ++pElement) {
            ::new (static_cast<void*>(pElement)) T();
        }
        return pArray;
    }

    static void Deallocate(T* pArray) {
        if (!pArray) {
            return;
        }
        uint8_t* pucMemory = reinterpret_cast<uint8_t*>(pArray);
        if constexpr (!std::is_trivially_destructible_v<T>) {
            uint32_t uiNumElements = static_cast<uint32_t>(reinterpret_cast<size_t*>(pArray)[-1]);
            pucMemory = reinterpret_cast<uint8_t*>(reinterpret_cast<size_t*>(pArray) - 1);
            while (uiNumElements > 0) {
                pArray[--uiNumElements].~T();
            }
        }
        MemoryManager_Singleton.Deallocate(pucMemory);
    }
};

// The geomorpher task pool's new[]: NiMemObject's operator new[] with the count cookie, each task
// through its constructor. Defined in NiGeomMorpherUpdateTask.cpp.
template <>
NiGeomMorpherUpdateTask* NiTNewInterface<NiGeomMorpherUpdateTask>::Allocate(uint32_t uiNumElements);
// The particle task pool's new[]: NiMemObject's operator new[] with the count cookie, each task
// through its constructor. Defined in NiPSysUpdateTask.cpp.
template <>
NiPSysUpdateTask* NiTNewInterface<NiPSysUpdateTask>::Allocate(uint32_t uiNumElements);
// The render state group pool's new[] and delete[]: NiMemObject's operator new[] and delete[] with the count cookie.
// Defined in NiD3DRenderStateGroup.cpp.
template <>
NiD3DRenderStateGroup* NiTNewInterface<NiD3DRenderStateGroup>::Allocate(uint32_t uiNumElements);
template <>
void NiTNewInterface<NiD3DRenderStateGroup>::Deallocate(NiD3DRenderStateGroup* pArray);
