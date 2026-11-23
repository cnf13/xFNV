#pragma once

#include "Fallout Shared/BSCloneReserver.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class ActorPathingMessageQueue;
class bhkRigidBody;
class bhkShape;
class BSMultiBoundNode;
class BSMultiBoundShape;
class BSShaderAccumulator;
class NavMesh;
class NiAVObject;
class NiCamera;
class NiD3DPass;
class NiD3DTextureStage;
class NiGeometry;
class NiGeometryData;
class NiLines;
class NiMaterialProperty;
class NiNode;
class NiObject;
class NiPixelData;
class NiScreenElements;
class NiSkinPartition;
class NiTriBasedGeom;
class NiTriShape;
class NiTriShapeData;
class NiTriStripsData;
class PathingAvoidNodeArray;
class PathingLOSGridMap;
class PathingRequest;
class PathingSolution;
class QueuedFile;
class TESAnimGroup;

// A smart pointer that holds a reference on a NiRefObject-derived object: constructing, copying and assigning take one,
// destroying releases it. A by-value NiPointer parameter is released by the callee's destructor, and a caller's copy
// takes the reference that release consumes.
template <class T>
class NiPointer {
public:
    constexpr NiPointer() : m_pObject(nullptr) {}
    NiPointer(T* apObject);
    NiPointer(const NiPointer& ptr);
    ~NiPointer();

    operator T*() const {
        return m_pObject;
    }

    // Holds a reference on apObject and releases the old one; returns this.
    NiPointer* SetNew(T* apObject) {
        *this = apObject;
        return this;
    }

    // Copies apSource's object and takes a reference on it, without releasing a previous one.
    NiPointer* SetPointer(const NiPointer* apSource) {
        m_pObject = apSource->m_pObject;
        if (m_pObject) {
            NiIncRefCount(m_pObject);
        }
        return this;
    }

    // Assignment from another NiPointer: holds a reference on apPointer's object and releases the old one; returns
    // this. Defined per instantiation.
    NiPointer* SetPointer_(const NiPointer* apPointer);

    // Assigns apOther to this pointer unless apOther holds no object.
    void SetIfNotNull(const NiPointer* apOther) {
        if (!(*apOther == nullptr)) {
            *this = *apOther;
        }
    }

    // Holds a reference on apObject and releases the old one.
    NiPointer& operator=(T* apObject);

    // Holds a reference on the other pointer's object and releases the old one. The other pointer is read again after
    // the release.
    NiPointer& operator=(const NiPointer& ptr) {
        if (m_pObject != ptr.m_pObject) {
            if (m_pObject) {
                NiDecRefCount(m_pObject);
            }
            m_pObject = ptr.m_pObject;
            if (m_pObject) {
                NiIncRefCount(m_pObject);
            }
        }
        return *this;
    }

    T* operator->() const;

    bool operator==(T* pObject) const;
    bool operator==(const NiPointer& ptr) const;
    bool operator!=(T* pObject) const;
    bool operator!=(const NiPointer& ptr) const;
    void Free();

    // The native side of Destroy: releases the object as NiPointer<Model>::Destroy does, and nulls the
    // pointer, since the native local's own destructor runs again at the end of its scope.
    static void Destroy(NiPointer* apMemory) {
        if (apMemory->m_pObject) {
            apMemory->m_pObject->DecRefCount();
            apMemory->m_pObject = nullptr;
        }
    }

    T* m_pObject;
};

// The object as a U*, where U derives from T: a downcast through the smart pointer (a template counterpart of
// Gamebryo's NiSmartPointerCast(type, sp) macro).
template <class U, class T>
inline U* NiSmartPointerCast(const NiPointer<T>& sp) {
    return static_cast<U*>(static_cast<T*>(sp));
}

// Declares classname and its smart pointer type classname##Ptr, as Gamebryo's NiSmartPointer.h does.
#define NiSmartPointer(classname)                                                                                      \
    class classname;                                                                                                   \
    typedef NiPointer<classname> classname##Ptr

// NiRefObject's smart pointer is declared here: NiRefObject.h is included by this header, so it cannot use the macro.
NiSmartPointer(NiRefObject);

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.inl"
