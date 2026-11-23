#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class NiPoint2;

// The old screen-space API: screen geometry, screen polygons and the camera that draws them.

// A triangle-list mesh in screen space.
class NiScreenGeometry : public NiTriShape {
public:
    NiScreenGeometry();
    ~NiScreenGeometry() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
};

// The geometry data of NiScreenGeometry: a list of screen elements (2D vertices, colors, texture coordinates).
class NiScreenGeometryData : public NiTriShapeData {
public:
    // An element's vertices, colors and texture coordinates. The destructor is its only virtual function.
    struct ScreenElement {
        ScreenElement();
        virtual ~ScreenElement();

        // The deleting destructors free through MemoryManager::Deallocate, an element or a new[] array.
        static void operator delete(void* apMemory);
        static void operator delete[](void* apMemory);

        uint16_t m_usNumVerts;
        uint16_t m_usNumTexCoordSets;
        NiPoint2* m_pkVerts;
        NiColorA* m_pkColors;
        NiPoint2* m_pkTexCoords;
    };

    // The pool the screen elements come from; _SDMInit makes it and _SDMShutdown frees it.
    using ScreenElementPool = NiTObjectPool<ScreenElement>;

    NiScreenGeometryData();
    ~NiScreenGeometryData() override;

    const NiRTTI* GetRTTI() const override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

    static NiObject* CreateObject();
    static void _SDMInit();
    static void _SDMShutdown();

    bool m_bPixelAccurate;
    bool m_bUnorderedElements;
    uint32_t m_usActiveVerts;
    NiTPrimitiveArray<ScreenElement*> m_kScreenElements;

    static NiRTTI ms_RTTI;
    static inline ScreenElementPool* ms_pkScreenElementPool;
};

// new[] of uiCount screen elements, each element built by its constructor.
NiScreenGeometryData::ScreenElement* AllocateScreenElements(uint32_t uiCount);

// A screen-space polygon: vertices, texture coordinates and colors drawn with its own property state.
class NiScreenPolygon : public NiObject {
public:
    NiScreenPolygon();
    ~NiScreenPolygon() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& stream) override;
    void LinkObject(NiStream& stream) override;
    bool RegisterStreamables(NiStream& stream) override;
    void SaveBinary(NiStream& stream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

    static NiObject* CreateObject();

    // A plain pointer rather than a smart pointer, because NiPropertyState has no reference count.
    NiPropertyState* m_spPropertyState;
    uint16_t m_usVertices;
    NiPoint3* m_pkVertex;
    NiPoint2* m_pkTexture;
    NiColorA* m_pkColor;

    static NiRTTI ms_RTTI;
};

// A camera that also draws lists of screen polygons and screen textures.
class NiScreenSpaceCamera : public NiCamera {
public:
    NiScreenSpaceCamera();
    ~NiScreenSpaceCamera() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void ProcessClone(NiCloningProcess& kCloning) override;

    // NiCamera's members, then the screen polygon and texture arrays.
    void CopyMembers(NiScreenSpaceCamera* pkDest, NiCloningProcess& kCloning);

    static NiObject* CreateObject();

    NiTObjectArray<NiPointer<NiScreenPolygon>> m_kScreenPolygons;
    NiTObjectArray<NiPointer<NiScreenTexture>> m_kScreenTextures;

    static NiRTTI ms_RTTI;
};
