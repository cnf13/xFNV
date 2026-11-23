#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"

class NiScreenElementsData;

// A screen-space polygon list the UI draws with.
class NiScreenElements : public NiTriShape {
public:
    NiScreenElements();
    NiScreenElements(NiScreenElementsData* pkModelData);
    ~NiScreenElements() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

    int32_t Insert(uint16_t usNumVertices, uint16_t usNumTriangles, const uint16_t* ausTriList);
    bool SetRectangle(int32_t iPolygon, float fLeft, float fTop, float fWidth, float fHeight);
    void UpdateBound();
    void Draw(NiRenderer* pkRenderer);
    bool SetTextures(int32_t iPolygon, uint16_t usSet, float fLeft, float fTop, float fRight, float fBottom);
    NiScreenElementsData* Data();

    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
};

NiSmartPointer(NiScreenElements);
