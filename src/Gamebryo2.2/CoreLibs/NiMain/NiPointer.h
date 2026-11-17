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
    // the release, as the game does.
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

// A default constructor closure shared by every NiPointer instance whose arrays are built (NiTexture in
// BSShaderBloodSplatterProperty, bhkPhantom in LoadedAreaBound, AnimIdle in AnimData...); it constructs the pointer as
// null. Because one function serves every instance, apThis is typed as NiPointer<NiRefObject>*.
void NiPointerDefaultConstructor(NiPointer<NiRefObject>* apThis);
// Default constructor closure that stores a null pointer. BSShaderManager::Initialize, BSRenderedTexture's and
// BSShaderBloodSplatter's constructors, the shader constant map entries' Allocate and NiControllerSequence::LoadBinary
// hand it to `eh vector constructor iterator'.
void NiPointer_DefaultConstructorClosure_B54620(NiPointer<NiRefObject>* apThis);
// Destroys a NiPointer<NiRefObject> with the release done in place: InterlockedDecrement of the reference count, then
// DeleteThis at zero.
void NiPointer_Destruct_AE9290(NiPointer<NiRefObject>* apThis);
void NiPointer_Destruct_B66F20(NiPointer<NiRefObject>* apThis);
// Constructor of NiPointer<ActorPathingMessageQueue>: holds apObject through NiRefObject::IncRefCount on its
// NiRefObject base and returns apThis. PathingTaskData, HighProcess, CombatState and ActorMover's constructors call it.
NiPointer<ActorPathingMessageQueue>*
NiPointer_Construct_6EBF60(NiPointer<ActorPathingMessageQueue>* apThis, ActorPathingMessageQueue* apObject);

// Declares classname and its smart pointer type classname##Ptr, as Gamebryo's NiSmartPointer.h does.
#define NiSmartPointer(classname)                                                                                      \
    class classname;                                                                                                   \
    typedef NiPointer<classname> classname##Ptr

// NiRefObject's smart pointer is declared here: NiRefObject.h is included by this header, so it cannot use the macro.
NiSmartPointer(NiRefObject);

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.inl"
