#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"

// NiTriStripsData whose vertex and triangle counts in use can be lower than the allocated counts.
class NiTriStripsDynamicData : public NiTriStripsData {
public:
    NiTriStripsDynamicData();
    NiTriStripsDynamicData(
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
        uint16_t* pusStripLists,
        uint16_t usActiveVertices,
        uint16_t usActiveTriangles
    );
    ~NiTriStripsDynamicData() override;

    const NiRTTI* GetRTTI() const override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void SetActiveVertexCount(uint16_t usActiveVertices) override;
    uint16_t GetActiveVertexCount() const override;
    // also writes the count into the first strip length when there are strips
    void SetActiveTriangleCount(uint16_t usActiveTriangles) override;
    uint16_t GetActiveTriangleCount() const override;
    void GetStripData(
        uint16_t& usStrips,
        const uint16_t*& pusStripLengths,
        const uint16_t*& pusTriList,
        uint32_t& uiStripLengthSum
    ) const override;

    // Allocates and constructs an object with both active counts zero.
    static NiObject* CreateObject();

    uint16_t m_usActiveVertices;
    uint16_t m_usActiveTriangles;

    static NiRTTI ms_RTTI;
};
