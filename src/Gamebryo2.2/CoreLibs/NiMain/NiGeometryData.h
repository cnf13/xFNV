#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

#include <stdint.h>

class NiGeometryBufferData;
class NiGeometryDataIteratorNiBlendIndices;
class NiGeometryDataIteratorNiBlendWeights;
class NiGeometryDataIteratorNiColorA;
class NiGeometryDataIteratorNiPoint2;
class NiGeometryDataIteratorNiPoint3;
class NiPoint2;
class NiStream;
class NiTriShapeData;
class NiTriStripsData;

// The vertex arrays and bound shared by NiGeometry objects.
class NiGeometryData : public NiObject {
public:
    // The renderer's per-geometry vertex buffer data (defined in NiGeometryBufferData.h).
    class RendererData;

    // The data kept on the CPU after it is packed for the renderer.
    enum {
        KEEP_XYZ = 0x01,
        KEEP_NORM = 0x02,
        KEEP_COLOR = 0x04,
        KEEP_UV = 0x08,
        KEEP_INDICES = 0x10,
        KEEP_BONEDATA = 0x20,
        KEEP_ALL = KEEP_XYZ | KEEP_NORM | KEEP_COLOR | KEEP_UV | KEEP_INDICES | KEEP_BONEDATA,
    };

    // How often the vertex data changes; kept in the consistency bits of m_usDataFlags.
    enum Consistency : int32_t {
        MUTABLE = 0x0,
        STATIC = 0x4000,
        CONSISTENCY_MASK = 0x7000,
    };

    // Revision flags passed to MarkAsChanged.
    enum : uint16_t {
        VERTEX_MASK = 0x1,
        NORMAL_MASK = 0x2,
        COLOR_MASK = 0x4,
        TEXTURE_MASK = 0x8,
        DIRTY_MASK = 0xFFF,
    };

    // The normal/binormal/tangent method bits of m_usDataFlags (0xF000, as GetBlockAllocationSize tests them) and the
    // texture set count bits.
    enum DataFlags : int32_t {
        NBT_METHOD_NONE = 0x0,
        NBT_METHOD_NDL = 0x1000,
        NBT_METHOD_DEPRECATED = 0x2000,
        NBT_METHOD_ATI = 0x3000,
        NBT_METHOD_MASK = 0xF000,
        // Gamebryo's mask. This build's texture set count is one bit wide (BS_TEXTURE_SET_MASK); the Havok bits sit
        // above it.
        TEXTURE_SET_MASK = 0x3F,
        BS_TEXTURE_SET_MASK = 0x1,
        HAVOK_PLATFORM_MASK = 0x20,
        HAVOK_STAIRS_MASK = 0x800,
    };

    enum Compression : uint32_t {
        COMPRESS_NORM = 0x1,
    };

    NiGeometryData();
    NiGeometryData(
        int32_t usVertices,
        NiPoint3* pkVertex,
        NiPoint3* pkNormal,
        NiColorA* pkColor,
        NiPoint2* pkTexture,
        int32_t usNumTextureSets,
        NiGeometryData::DataFlags eNBTMethod
    );
    ~NiGeometryData() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void SetBound(BSNiBound* apBound) override;
    uint32_t GetBlockAllocationSize() const override;
    NiObjectGroup* GetGroup() const override {
        return m_pkGroup;
    }
    void SetGroup(NiObjectGroup* pkGroup) override {
        m_pkGroup = pkGroup;
    }
    virtual void SetActiveVertexCount(uint16_t) {}
    virtual uint16_t GetActiveVertexCount() const {
        return m_usVertices;
    }
    virtual NiTriStripsData* IsTriStripsData() {
        return nullptr;
    }
    virtual NiTriShapeData* IsTriShapeData() {
        return nullptr;
    }
    virtual bool ContainsVertexData(NiShaderDeclaration::ShaderParameter eParameter) const;
    virtual void CalculateNormals() = 0;

    void SetBound(const NiBound& kBound);
    void SetConsistency(Consistency eConsistency);
    void MarkAsChanged(uint16_t usFlags);
    uint16_t GetVertexCount() const;
    NiPoint3* GetVertices() const;
    // Fills arIterator to walk m_pkVertex.
    void GetVerticesIterator(NiGeometryDataIteratorNiPoint3& arIterator);
    // Fills arIterator to walk m_pkNormal.
    void GetNormalsIterator(NiGeometryDataIteratorNiPoint3& arIterator);
    // Fills arIterator to walk the first texture set.
    void GetTextureSetIterator(uint16_t usSet, NiGeometryDataIteratorNiPoint2& arIterator);
    // Fills arIterator to walk the packed blend weights.
    void GetBlendWeightsIterator(NiGeometryDataIteratorNiBlendWeights& arIterator);
    // Fills arIterator to walk the packed blend indices.
    void GetBlendIndicesIterator(NiGeometryDataIteratorNiBlendIndices& arIterator);
    // Returns m_pkNormal.
    NiPoint3* GetNormals() const;
    NiColorA* GetColors() const;
    // Returns m_spAdditionalGeomData through NiPointer's operator->.
    NiAdditionalGeometryData* GetAdditionalGeometryData() const;
    // Holds a reference on apAdditionalGeomData in m_spAdditionalGeomData, releasing the old one.
    void SetAdditionalGeometryData(NiAdditionalGeometryData* pkAddData);
    // Swaps in new vertex arrays; eNBTMethod is the NBT method bits of m_usDataFlags.
    void ReplaceData(
        uint16_t usVertices,
        NiPoint3* pkVertex,
        NiPoint3* pkNormal,
        NiColorA* pkColor,
        NiPoint2* pkTexture,
        uint16_t usNumTextureSets,
        DataFlags eNBTMethod
    );
    bool LockPackedVertexData(bool bWriteAccess);
    void UnlockPackedVertexData();
    // Swaps in new vertex arrays, releasing the vertex buffers of the arrays replaced.
    void Replace(
        uint16_t usVertices,
        NiPoint3* pkVertex,
        NiPoint3* pkNormal,
        NiColorA* pkColor,
        NiPoint2* pkTexture,
        uint16_t usNumTextureSets,
        DataFlags eNBTMethod
    );

    // The texture coordinates (one set on PC: the set index is ignored).
    NiPoint2* GetTextureSet(uint16_t usSet) const {
        return m_pkTexture;
    }
    uint16_t GetTextureSets() const {
        return static_cast<uint16_t>(m_usDataFlags & BS_TEXTURE_SET_MASK);
    }

    void CreateNormals(bool bInitialize);
    // Swaps in a normal array with one (or three, for an NBT method) points per vertex, copying the old normals and
    // zeroing the first point past them.
    void StoreNormalBinormalTangent(DataFlags eNBTMethod);
    void SetCompressFlags(uint32_t uiCompressFlags);

    NiGeometryData::Consistency GetConsistency() const {
        return static_cast<NiGeometryData::Consistency>(m_usDirtyFlags & CONSISTENCY_MASK);
    }

    void SetKeepFlags(uint32_t uiKeepFlags);
    void SetData(
        uint16_t usVertices,
        NiPoint3* pkVertex,
        NiPoint3* pkNormal,
        NiColorA* pkColor,
        NiPoint2* pkTexture,
        uint16_t usNumTextureSets,
        NiGeometryData::DataFlags eNBTMethod
    );
    int32_t VertexCompare(uint16_t v0, uint16_t v1, int32_t mask) const;
    int32_t ChoosePivot(uint16_t* pusIndex, int32_t mask, int32_t l, int32_t r) const;
    void IndexQSort(uint16_t* pusIndex, int32_t mask, int32_t l, int32_t r) const;
    void IndexSort(uint16_t*& pusIndex0, uint16_t*& pusIndex1, int32_t mask) const;
    // Fills arIterator to walk m_pkColor or the packed colours.
    void GetColorsIterator(NiGeometryDataIteratorNiColorA& arIterator);
    void ClearNormals();

    static int32_t FloatCompare(const float* pf0, const float* pf1, uint32_t uiNum);

    uint16_t m_usVertices;
    uint16_t m_usID;
    uint16_t m_usDataFlags;
    uint16_t m_usDirtyFlags;
    NiBound m_kBound;
    NiPoint3* m_pkVertex;
    NiPoint3* m_pkNormal;
    NiColorA* m_pkColor;
    NiPoint2* m_pkTexture;
    NiPointer<NiAdditionalGeometryData> m_spAdditionalGeomData;
    NiGeometryBufferData* m_pkBuffData;
    uint8_t m_ucKeepFlags;
    uint8_t m_ucCompressFlags;
    bool m_bVBLocked;
    bool m_bVBLockWrite;
    bool m_bSaveVertexData;

    static NiRTTI ms_RTTI;
    // No code reads it; it is only initialised at start-up and deleted at exit.
    static NiCriticalSection ms_kCritSec;
    static inline NiObjectGroup* m_pkGroup;
    static inline uint16_t ms_usNextID = 1;
};

NiSmartPointer(NiGeometryData);
