#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"

#include <stdint.h>

class NiGeometryBufferData;
class NiGeometryData;
class NiScreenTexture;
class NiSkinInstance;
class NiVBChip;

// A set of geometry sharing vertex buffers, managed by the geometry group manager. Only part of the members are
// declared.
class NiGeometryGroup : public NiMemObject {
public:
    NiGeometryGroup();

    virtual void Purge() = 0;
    virtual void
    AddObject(NiGeometryData* apData, NiSkinInstance* apSkinInstance, NiSkinPartition::Partition* apPartition) = 0;
    virtual void AddObject(NiScreenTexture* apScreenTexture) = 0;
    virtual void RemoveObject(NiGeometryData* apData) = 0;
    virtual void RemoveObject(NiSkinPartition::Partition* apPartition) = 0;
    virtual void RemoveObject(NiScreenTexture* apScreenTexture) = 0;
    // Makes the vertex chip of stream auiStream for the buffer data.
    virtual NiVBChip* CreateChip(NiGeometryBufferData* apBuffData, uint32_t auiStream) = 0;
    // Releases the vertex chip of stream auiStream of the buffer data.
    virtual void ReleaseChip(NiGeometryBufferData* apBuffData, uint32_t auiStream) = 0;
    virtual bool IsDynamic() = 0;
    virtual ~NiGeometryGroup();

    // Holds a reference for the buffer data and marks it as the group's.
    void AddObject(NiGeometryBufferData* apBuffData);
    // Drops the buffer data's reference, releases each stream's chip and its index buffer.
    void RemoveObject(NiGeometryBufferData* apBuffData);

    uint32_t m_uiRefCount;
    IDirect3DDevice9* m_pkD3DDevice;
};
