#pragma once

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cstdint>

class Actor;
class bhkRigidBody;
class CombatGroup;
class MobileObject;
class NiAVObject;
class NiD3DPass;
class NiD3DTextureStage;
class NiFixedString;
class NiGeometry;
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
// Element storage of the object containers (NiTObjectArray, NiTObjectSet...): new[]/delete[], so a NiMemObject type
// goes through NiMemObject's operators (BSNiAllocator, which zero-fills) and any other type through Bethesda's global
// ones on MemoryManager. Elements are default-initialised: a pointer or a type without a constructor is left as the
// allocator returned it.
template <class T>
class NiTNewInterface {
public:
    static T* Allocate(uint32_t uiNumElements) {
        return new T[uiNumElements];
    }

    static void Deallocate(T* pArray) {
        delete[] pArray;
    }
};
