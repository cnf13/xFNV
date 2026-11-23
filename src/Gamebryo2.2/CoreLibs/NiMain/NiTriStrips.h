#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"

// A triangle-strip mesh.
class NiTriStrips : public NiTriBasedGeom {
public:
    NiTriStrips();
    NiTriStrips(NiTriStripsData* pkModelData);
    // Takes ownership of the given arrays.
    NiTriStrips(
        int32_t usVertices,
        NiPoint3* pkVertex,
        NiPoint3* pkNormal,
        NiColorA* pkColor,
        NiPoint2* pkTexture,
        int32_t usNumTextureSets,
        int32_t eNBTMethod,
        int32_t usTriangles,
        int32_t usStrips,
        uint16_t* pusStripLengths,
        uint16_t* pusStripLists
    );
    ~NiTriStrips() override;

    const NiRTTI* GetRTTI() const override;
    NiTriStrips* IsTriStrips() override {
        return this;
    }
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void RenderImmediate(NiRenderer* apkRenderer) override;
    void OnlyRenderImmediate(NiRenderer* apkRenderer) override;
    void GetModelTriangle(uint16_t usTriangle, NiPoint3*& pkP0, NiPoint3*& pkP1, NiPoint3*& pkP2) override;

protected:
    void CopyMembers(NiTriStrips* pkDest, NiCloningProcess& kCloning);

public:
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
};

NiSmartPointer(NiTriStrips);
