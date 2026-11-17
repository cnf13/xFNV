#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// A line-list primitive.
class NiLines : public NiGeometry {
public:
    NiLines();
    NiLines(
        int32_t ausVertices,
        NiPoint3* apVertex,
        NiColorA* apColor,
        NiPoint2* apTexture,
        int32_t ausNumTextureSets,
        NiGeometryData::DataFlags aeNBTMethod,
        uint8_t* apFlags
    );
    ~NiLines() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiLines* IsLinesGeom() override {
        return this;
    }
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void RenderImmediate(NiRenderer* apkRenderer) override;

    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
};

NiSmartPointer(NiLines);
