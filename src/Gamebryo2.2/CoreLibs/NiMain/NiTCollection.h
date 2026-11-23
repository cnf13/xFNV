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
class NiD3DRenderStateGroup;
class NiD3DTextureStage;
class NiD3DTextureStageGroup;
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
// Bethesda's global operator new[] on MemoryManager (not NiMemObject's), each element value-initialised. A NiMemObject
// type with no destructor is specialised below: ::new T[] would store its count under the Itanium ABI only.
template <class T>
class NiTNewInterface {
public:
    static T* Allocate(uint32_t uiNumElements) {
        return ::new T[uiNumElements]();
    }

    static void Deallocate(T* pArray) {
        ::delete[] pArray;
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
// The texture stage group pool's new[] and delete[]: a MemoryManager block holding only the groups. Defined in
// NiD3DTextureStageGroup.cpp.
template <>
NiD3DTextureStageGroup* NiTNewInterface<NiD3DTextureStageGroup>::Allocate(uint32_t uiNumElements);
template <>
void NiTNewInterface<NiD3DTextureStageGroup>::Deallocate(NiD3DTextureStageGroup* pArray);
