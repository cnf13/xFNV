#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/FaceGen/NiGeometryDataIteratorNiPoint3.h"
#include "Fallout Shared/NiGeometryDataIteratorNiColorA.h"
#include "Fallout Shared/TempEffects/NiGeometryDataIteratorNiBlendIndices.h"
#include "Fallout Shared/TempEffects/NiGeometryDataIteratorNiBlendWeights.h"
#include "Fallout Shared/TempEffects/NiGeometryDataIteratorNiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSPackedAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"

NiRTTI NiGeometryData::ms_RTTI("NiGeometryData", &NiObject::ms_RTTI);

NiCriticalSection NiGeometryData::ms_kCritSec;

NiGeometryData::NiGeometryData()
  : m_usVertices(0), m_usDataFlags(0), m_usDirtyFlags(0), m_pkVertex(nullptr), m_pkNormal(nullptr), m_pkColor(nullptr),
    m_pkTexture(nullptr), m_pkBuffData(nullptr), m_ucKeepFlags(0), m_ucCompressFlags(0), m_bVBLocked(false),
    m_bVBLockWrite(false), m_bSaveVertexData(true) {
    if (!m_spAdditionalGeomData || !m_spAdditionalGeomData->GetVBReady()) {
        m_usDirtyFlags &= ~CONSISTENCY_MASK;
    }
}

NiGeometryData::NiGeometryData(
    int32_t usVertices,
    NiPoint3* pkVertex,
    NiPoint3* pkNormal,
    NiColorA* pkColor,
    NiPoint2* pkTexture,
    int32_t usNumTextureSets,
    NiGeometryData::DataFlags eNBTMethod
)
  : m_usVertices(static_cast<uint16_t>(usVertices)), m_usID(ms_usNextID++),
    m_usDataFlags(static_cast<uint16_t>(eNBTMethod | (usNumTextureSets & BS_TEXTURE_SET_MASK))), m_usDirtyFlags(0),
    m_pkVertex(pkVertex), m_pkNormal(pkNormal), m_pkColor(pkColor), m_pkTexture(pkTexture), m_pkBuffData(nullptr),
    m_ucKeepFlags(0), m_ucCompressFlags(0), m_bVBLocked(false), m_bVBLockWrite(false), m_bSaveVertexData(true) {
    if (m_usVertices && m_pkVertex) {
        m_kBound.ComputeFromData(m_usVertices, m_pkVertex);
    }
    if (!m_spAdditionalGeomData || !m_spAdditionalGeomData->GetVBReady()) {
        m_usDirtyFlags &= ~CONSISTENCY_MASK;
    }
}

NiGeometryData::~NiGeometryData() {
    if (NiRenderer::ms_pkRenderer) {
        NiRenderer::ms_pkRenderer->PurgeGeometryData(this);
    }
    m_spAdditionalGeomData = nullptr;
    if (m_pkGroup) {
        // Each array held a reference on the shared group; the last one frees its vertex buffer and the group.
        const void* apvArrays[] = {m_pkVertex, m_pkNormal, m_pkColor, m_pkTexture};
        for (const void* pvArray : apvArrays) {
            if (pvArray && --m_pkGroup->m_uiRefCount == 0) {
                m_pkGroup->FreeVertexBuffer();
                delete m_pkGroup;
            }
        }
    }
    else {
        delete[] m_pkVertex;
        delete[] m_pkNormal;
        delete[] m_pkColor;
        delete[] m_pkTexture;
    }
}

// KEEP (niread:): the has-vertices, has-normals and has-colors bools are read from the stream into stack slots
// nothing initialised (the first into arStream's own argument slot), so a short read leaves them unwritten and the
// body tests them.
void NiGeometryData::LoadBinary(NiStream& kStream) {
    bool bHasVertex;
    bool bHasNormal;
    bool bHasColor;
    // Shared by several reads: the group ID's component size, then the normal count, an old file's data flags and
    // its texture set count.
    uint32_t uiScratch;
    uint32_t uiID;
    NiObject::LoadBinary(kStream);
    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 114)) {
        NiBinaryStream* pkIstr = kStream.Istr();
        NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pkIstr->m_pfnRead;
        uiScratch = 4;
        pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, &uiID, 4, &uiScratch, 1);
        NiGeometryData::m_pkGroup = kStream.GetGroupFromID(uiID);
    }
    NiBinaryStream* pkVerticesIstr = kStream.Istr();
    NiBinaryStream::NIBINARYSTREAMREADFN pfnVerticesRead = pkVerticesIstr->m_pfnRead;
    uint16_t* pusVertices = &this->m_usVertices;
    uiID = 2;
    pkVerticesIstr->m_uiAbsoluteCurrentPos += pfnVerticesRead(pkVerticesIstr, &this->m_usVertices, 2, &uiID, 1);
    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 0, 1, 16)) {
        NiBinaryStream* pkKeepIstr = kStream.Istr();
        NiBinaryStream::NIBINARYSTREAMREADFN pfnKeepRead = pkKeepIstr->m_pfnRead;
        uiID = 1;
        pkKeepIstr->m_uiAbsoluteCurrentPos += pfnKeepRead(pkKeepIstr, &m_ucKeepFlags, 1, &uiID, 1);
        NiBinaryStream* pkCompressIstr = kStream.Istr();
        NiBinaryStream::NIBINARYSTREAMREADFN pfnCompressRead = pkCompressIstr->m_pfnRead;
        uiID = 1;
        pkCompressIstr->m_uiAbsoluteCurrentPos += pfnCompressRead(pkCompressIstr, &m_ucCompressFlags, 1, &uiID, 1);
    }
    NiBinaryStream* pkHasVertexIstr = kStream.Istr();
    NiBinaryStream::NIBINARYSTREAMREADFN pfnHasVertexRead = pkHasVertexIstr->m_pfnRead;
    if (kStream.GetFileVersion() >= NiStream::GetVersion(4, 1, 0, 0)) {
        uiID = 1;
        pkHasVertexIstr->m_uiAbsoluteCurrentPos += pfnHasVertexRead(pkHasVertexIstr, &bHasVertex, 1, &uiID, 1);
    }
    else {
        uiID = 4;
        pkHasVertexIstr->m_uiAbsoluteCurrentPos += pfnHasVertexRead(pkHasVertexIstr, &this->m_pkVertex, 4, &uiID, 1);
        bHasVertex = this->m_pkVertex != nullptr;
    }
    if (bHasVertex) {
        NiObjectGroup* pkGroup = GetGroup();
        int32_t iVertices = *pusVertices;
        if (pkGroup) {
            NiObjectGroup* pkVertexGroup = GetGroup();
            NiPoint3* pkFree = static_cast<NiPoint3*>(pkVertexGroup->m_pvFree);
            ++pkVertexGroup->m_uiRefCount;
            pkVertexGroup->m_pvFree = &pkFree[iVertices];
            this->m_pkVertex = pkFree;
        }
        else {
            this->m_pkVertex = new NiPoint3[*pusVertices];
        }
        if (kStream.m_BSStreamHeader.uiVersion < 19 || m_bSaveVertexData) {
            NiBinaryStream* pkIstr = kStream.Istr();
            int32_t iBytes = 12 * *pusVertices;
            NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pkIstr->m_pfnRead;
            NiPoint3* pkVertex = this->m_pkVertex;
            uiID = 4;
            pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, pkVertex, iBytes, &uiID, 1);
        }
        else {
            std::memset(this->m_pkVertex, 0, sizeof(NiPoint3) * *pusVertices);
        }
    }
    bool bNoDataFlags = kStream.GetFileVersion() < NiStream::GetVersion(10, 0, 0, 2);
    uint32_t uiNormals = *pusVertices;
    uiScratch = uiNormals;
    if (!bNoDataFlags) {
        NiBinaryStream* pkIstr = kStream.Istr();
        NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pkIstr->m_pfnRead;
        uiID = 2;
        pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, &this->m_usDataFlags, 2, &uiID, 1);
        if ((this->m_usDataFlags & NBT_METHOD_MASK) != 0) {
            uiScratch = 3 * uiNormals;
            uiNormals *= 3;
        }
    }
    NiBinaryStream* pkHasNormalIstr = kStream.Istr();
    NiBinaryStream::NIBINARYSTREAMREADFN pfnHasNormalRead = pkHasNormalIstr->m_pfnRead;
    if (kStream.GetFileVersion() >= NiStream::GetVersion(4, 1, 0, 0)) {
        uiID = 1;
        pkHasNormalIstr->m_uiAbsoluteCurrentPos += pfnHasNormalRead(pkHasNormalIstr, &bHasNormal, 1, &uiID, 1);
    }
    else {
        uiID = 4;
        pkHasNormalIstr->m_uiAbsoluteCurrentPos += pfnHasNormalRead(pkHasNormalIstr, &this->m_pkNormal, 4, &uiID, 1);
        uiNormals = uiScratch;
        bHasNormal = this->m_pkNormal != nullptr;
    }
    if (bHasNormal) {
        if (GetGroup()) {
            NiObjectGroup* pkGroup = GetGroup();
            NiPoint3* pkFree = static_cast<NiPoint3*>(pkGroup->m_pvFree);
            ++pkGroup->m_uiRefCount;
            pkGroup->m_pvFree = &pkFree[uiNormals];
            this->m_pkNormal = pkFree;
        }
        else {
            this->m_pkNormal = new NiPoint3[uiNormals];
        }
        if (kStream.m_BSStreamHeader.uiVersion < 19 || m_bSaveVertexData) {
            NiBinaryStream* pkIstr = kStream.Istr();
            NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pkIstr->m_pfnRead;
            NiPoint3* pkNormal = this->m_pkNormal;
            uiID = 4;
            pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, pkNormal, 12 * uiNormals, &uiID, 1);
        }
    }
    m_kBound.LoadBinary(kStream);
    if (kStream.GetFileVersion() >= NiStream::GetVersion(4, 1, 0, 0)) {
        NiBinaryStream* pkIstr = kStream.Istr();
        NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pkIstr->m_pfnRead;
        uiID = 1;
        pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, &bHasColor, 1, &uiID, 1);
    }
    else {
        NiBinaryStream* pkIstr = kStream.Istr();
        NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pkIstr->m_pfnRead;
        uiID = 4;
        pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, &this->m_pkColor, 4, &uiID, 1);
        bHasColor = this->m_pkColor != nullptr;
    }
    if (bHasColor) {
        if (GetGroup()) {
            int32_t iVertices = this->m_usVertices;
            NiObjectGroup* pkGroup = GetGroup();
            NiColorA* pkFree = static_cast<NiColorA*>(pkGroup->m_pvFree);
            ++pkGroup->m_uiRefCount;
            pkGroup->m_pvFree = &pkFree[iVertices];
            this->m_pkColor = pkFree;
        }
        else {
            this->m_pkColor = new NiColorA[this->m_usVertices];
        }
        if (kStream.m_BSStreamHeader.uiVersion < 19 || m_bSaveVertexData) {
            NiBinaryStream* pkIstr = kStream.Istr();
            int32_t iBytes = 16 * this->m_usVertices;
            NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pkIstr->m_pfnRead;
            NiColorA* pkColor = this->m_pkColor;
            uiID = 4;
            pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, pkColor, iBytes, &uiID, 1);
        }
    }
    uint32_t uiFileVersion = kStream.GetFileVersion();
    if (uiFileVersion >= NiStream::GetVersion(5, 0, 0, 10)) {
        if (uiFileVersion < NiStream::GetVersion(10, 0, 0, 2)) {
            NiBinaryStream* pkIstr = kStream.Istr();
            NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pkIstr->m_pfnRead;
            uiID = 2;
            pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, &this->m_usDataFlags, 2, &uiID, 1);
        }
    }
    else {
        NiBinaryStream* pkIstr = kStream.Istr();
        NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pkIstr->m_pfnRead;
        uiID = 2;
        pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, &uiScratch, 2, &uiID, 1);
        // Only the texture set bit of the old flags is kept.
        this->m_usDataFlags = static_cast<uint16_t>(
            (this->m_usDataFlags & ~BS_TEXTURE_SET_MASK) | (uiScratch & BS_TEXTURE_SET_MASK)
        );
    }
    if (kStream.GetFileVersion() < NiStream::GetVersion(4, 1, 0, 0)) {
        NiBinaryStream* pkIstr = kStream.Istr();
        NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pkIstr->m_pfnRead;
        uiID = 4;
        pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, &this->m_pkTexture, 4, &uiID, 1);
    }
    if (kStream.m_BSStreamHeader.uiVersion >= 27) {
        uiScratch = 0;
    }
    else {
        uint16_t usDataFlags = this->m_usDataFlags;
        uiScratch = usDataFlags & 31;
        if (static_cast<uint16_t>(uiScratch) > 1u) {
            this->m_usDataFlags = usDataFlags | BS_TEXTURE_SET_MASK;
        }
    }
    uint16_t usFlags = this->m_usDataFlags;
    if ((usFlags & BS_TEXTURE_SET_MASK) != 0) {
        uint32_t uiTexCoords = this->m_usVertices * (usFlags & BS_TEXTURE_SET_MASK);
        if (GetGroup()) {
            NiObjectGroup* pkGroup = GetGroup();
            NiPoint2* pkFree = static_cast<NiPoint2*>(pkGroup->m_pvFree);
            ++pkGroup->m_uiRefCount;
            pkGroup->m_pvFree = &pkFree[uiTexCoords];
            this->m_pkTexture = pkFree;
        }
        else {
            this->m_pkTexture = new NiPoint2[uiTexCoords];
        }
        if (kStream.m_BSStreamHeader.uiVersion < 19 || m_bSaveVertexData) {
            NiBinaryStream* pkIstr = kStream.Istr();
            NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pkIstr->m_pfnRead;
            NiPoint2* pkTexture = this->m_pkTexture;
            uiID = 4;
            pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, pkTexture, 8 * uiTexCoords, &uiID, 1);
            // Older files' extra texture sets are read into a scratch array and dropped.
            if (static_cast<uint16_t>(uiScratch) > 1u) {
                uint32_t uiExtraTexCoords = this->m_usVertices * (static_cast<uint16_t>(uiScratch) - 1);
                NiPoint2* pkExtraSets = new NiPoint2[uiExtraTexCoords];
                NiPoint2::LoadBinary(kStream, pkExtraSets, uiExtraTexCoords);
                delete[] pkExtraSets;
            }
        }
    }
    if (kStream.GetFileVersion() >= NiStream::GetVersion(5, 0, 0, 10)) {
        NiBinaryStream* pkIstr = kStream.Istr();
        NiBinaryStream::NIBINARYSTREAMREADFN pfnRead = pkIstr->m_pfnRead;
        uiID = 2;
        pkIstr->m_uiAbsoluteCurrentPos += pfnRead(pkIstr, &m_usDirtyFlags, 2, &uiID, 1);
    }
    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 3, 0, 7)) {
        kStream.ReadLinkID();
    }
}

void NiGeometryData::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
    if (kStream.GetFileVersion() < 0x0A030007) {
        return;
    }

    NiAdditionalGeometryData* pkAdditionalData = static_cast<NiAdditionalGeometryData*>(kStream.GetObjectFromLinkID());
    m_spAdditionalGeomData = pkAdditionalData;
}

bool NiGeometryData::RegisterStreamables(NiStream& kStream) {
    if (!NiObject::RegisterStreamables(kStream)) {
        return false;
    }
    NiAdditionalGeometryData* pkAdditionalData = m_spAdditionalGeomData.m_pObject;
    return !pkAdditionalData || pkAdditionalData->RegisterStreamables(kStream);
}

void NiGeometryData::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
    bool bHasVertex;
    bool bHasNormals;
    bool bHasColor;
    // One stack slot: the component size of the first writes, then the normal count, then the dirty flags written.
    uint32_t uiValue;
    uint32_t uiID_2;
    uint32_t uiSize;
    NiStream* v2 = &kStream;
    uint32_t uiID = v2->GetIDFromGroup(NiGeometryData::m_pkGroup);
    NiBinaryStream* m_pkOstr = v2->Ostr();
    uiID_2 = uiID;
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    uiValue = 4;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &uiID_2, 4, &uiValue, 1);
    NiBinaryStream* v7 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v8 = v7->m_pfnWrite;
    uint16_t* pusVertices = &m_usVertices;
    uiValue = 2;
    v7->m_uiAbsoluteCurrentPos += v8(v7, &m_usVertices, 2, &uiValue, 1);
    NiBinaryStream* v10 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v11 = v10->m_pfnWrite;
    uiValue = 1;
    v10->m_uiAbsoluteCurrentPos += v11(v10, &m_ucKeepFlags, 1, &uiValue, 1);
    NiBinaryStream* v12 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v13 = v12->m_pfnWrite;
    uiValue = 1;
    v12->m_uiAbsoluteCurrentPos += v13(v12, &m_ucCompressFlags, 1, &uiValue, 1);
    NiBinaryStream* v14 = v2->Ostr();
    // Only the low byte of bHasVertex is written out.
    bHasVertex = this->m_pkVertex != nullptr;
    NiBinaryStream::NIBINARYSTREAMWRITEFN v15 = v14->m_pfnWrite;
    uiValue = 1;
    v14->m_uiAbsoluteCurrentPos += v15(v14, &bHasVertex, 1, &uiValue, 1);
    if (bHasVertex && m_bSaveVertexData) {
        NiBinaryStream* v16 = v2->Ostr();
        int32_t v36 = 12 * *pusVertices;
        NiBinaryStream::NIBINARYSTREAMWRITEFN v17 = v16->m_pfnWrite;
        NiPoint3* m_pkVertex = this->m_pkVertex;
        uiValue = 4;
        v16->m_uiAbsoluteCurrentPos += v17(v16, m_pkVertex, v36, &uiValue, 1);
    }

    if (!this->m_pkTexture) {
        m_usDataFlags &= ~BS_TEXTURE_SET_MASK;
    }
    NiBinaryStream* v18 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v19 = v18->m_pfnWrite;
    uiValue = 2;
    v18->m_uiAbsoluteCurrentPos += v19(v18, &m_usDataFlags, 2, &uiValue, 1);
    uint32_t v20 = *pusVertices;
    uiValue = v20;
    if ((m_usDataFlags & NBT_METHOD_MASK) != 0) {
        uiValue = 3 * v20;
    }
    NiBinaryStream* v21 = v2->Ostr();
    bHasNormals = this->m_pkNormal != nullptr;
    NiBinaryStream::NIBINARYSTREAMWRITEFN v22 = v21->m_pfnWrite;
    uiSize = 1;
    v21->m_uiAbsoluteCurrentPos += v22(v21, &bHasNormals, 1, &uiSize, 1);
    if (bHasNormals && m_bSaveVertexData) {
        NiBinaryStream* v23 = v2->Ostr();
        NiBinaryStream::NIBINARYSTREAMWRITEFN v24 = v23->m_pfnWrite;
        NiPoint3* m_pkNormal = this->m_pkNormal;
        uiSize = 4;
        v23->m_uiAbsoluteCurrentPos += v24(v23, m_pkNormal, 12 * uiValue, &uiSize, 1);
    }
    m_kBound.SaveBinary(*v2);
    NiBinaryStream* v25 = v2->Ostr();
    bHasColor = this->m_pkColor != nullptr;
    NiBinaryStream::NIBINARYSTREAMWRITEFN v26 = v25->m_pfnWrite;
    uiSize = 1;
    v25->m_uiAbsoluteCurrentPos += v26(v25, &bHasColor, 1, &uiSize, 1);
    if (bHasColor && m_bSaveVertexData) {
        NiBinaryStream* v27 = v2->Ostr();
        int32_t v37 = 16 * *pusVertices;
        NiBinaryStream::NIBINARYSTREAMWRITEFN v28 = v27->m_pfnWrite;
        NiColorA* m_pkColor = this->m_pkColor;
        uiSize = 4;
        v27->m_uiAbsoluteCurrentPos += v28(v27, m_pkColor, v37, &uiSize, 1);
    }
    uint32_t uiQuantity = *pusVertices * (m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK);
    if (uiQuantity) {
        NiPoint2* m_pkTexture = this->m_pkTexture;
        if (m_pkTexture && m_bSaveVertexData) {
            NiPoint2::SaveBinary(*v2, m_pkTexture, uiQuantity);
        }
    }
    NiBinaryStream* v31 = v2->Ostr();
    uiValue = m_usDirtyFlags & CONSISTENCY_MASK;
    NiBinaryStream::NIBINARYSTREAMWRITEFN v32 = v31->m_pfnWrite;
    uiSize = 2;
    v31->m_uiAbsoluteCurrentPos += v32(v31, &uiValue, 2, &uiSize, 1);
    v2->SaveLinkID(m_spAdditionalGeomData.m_pObject);
}

bool NiGeometryData::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }
    NiGeometryData* pkData = static_cast<NiGeometryData*>(pkObject);
    uint16_t usVertices = m_usVertices;
    if (usVertices != pkData->m_usVertices) {
        return false;
    }
    NiPoint3* pkVertex = m_pkVertex;
    if ((pkVertex && !pkData->m_pkVertex) || (!pkVertex && pkData->m_pkVertex)) {
        return false;
    }
    if (m_ucKeepFlags != pkData->m_ucKeepFlags || m_ucCompressFlags != pkData->m_ucCompressFlags) {
        return false;
    }
    if (pkVertex) {
        NiPoint3* pkOther = pkData->m_pkVertex;
        for (uint32_t i = 0; i < usVertices; i++) {
            if (pkVertex[i] != pkOther[i]) {
                return false;
            }
        }
    }
    NiPoint3* pkNormal = m_pkNormal;
    if ((pkNormal && !pkData->m_pkNormal) || (!pkNormal && pkData->m_pkNormal)) {
        return false;
    }
    if (pkNormal) {
        uint16_t usNBTMethod = m_usDataFlags & NBT_METHOD_MASK;
        if (usNBTMethod != (pkData->m_usDataFlags & NBT_METHOD_MASK)) {
            return false;
        }
        // With binormals and tangents the normal array holds three vectors per vertex.
        uint32_t uiCount = m_usVertices;
        if (usNBTMethod) {
            uiCount = 3 * uiCount;
        }
        NiPoint3* pkOther = pkData->m_pkNormal;
        for (uint32_t i = 0; i < uiCount; i++) {
            if (pkNormal[i] != pkOther[i]) {
                return false;
            }
        }
    }
    if (m_kBound != pkData->m_kBound) {
        return false;
    }
    // Tests the colours one way only and the texture coordinates the other way here, before both loops.
    NiColorA* pkColor = m_pkColor;
    if (pkColor && !pkData->m_pkColor) {
        return false;
    }
    if (!m_pkTexture && pkData->m_pkTexture) {
        return false;
    }
    if (pkColor) {
        uint32_t uiCount = m_usVertices;
        NiColorA* pkOther = pkData->m_pkColor;
        for (uint32_t i = 0; i < uiCount; i++) {
            if (pkColor[i] != pkOther[i]) {
                return false;
            }
        }
    }
    NiPoint2* pkTexture = m_pkTexture;
    if ((pkTexture && !pkData->m_pkTexture) || (!pkTexture && pkData->m_pkTexture)) {
        return false;
    }
    if (pkTexture) {
        // Only the low bit of the texture-set count is compared (Gamebryo's TEXTURE_SET_MASK is
        // 0x3F).
        uint16_t usDataFlags = m_usDataFlags;
        if (((static_cast<uint8_t>(usDataFlags) ^ static_cast<uint8_t>(pkData->m_usDataFlags)) & BS_TEXTURE_SET_MASK) !=
            0) {
            return false;
        }
        uint32_t uiQuantity = m_usVertices * (usDataFlags & BS_TEXTURE_SET_MASK);
        NiPoint2* pkOther = pkData->m_pkTexture;
        for (uint32_t i = 0; i < uiQuantity; i++) {
            if (pkTexture[i] != pkOther[i]) {
                return false;
            }
        }
    }
    NiAdditionalGeometryData* pkAGD = m_spAdditionalGeomData.m_pObject;
    if ((pkAGD && !pkData->m_spAdditionalGeomData.m_pObject) || (!pkAGD && pkData->m_spAdditionalGeomData.m_pObject)) {
        return false;
    }
    if (pkAGD &&
        pkData->m_spAdditionalGeomData.m_pObject &&
        !pkAGD->IsEqual(pkData->m_spAdditionalGeomData.m_pObject)) {
        return false;
    }
    return true;
}

void NiGeometryData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiObject::GetViewerStrings(pkStrings);
    char* ViewerString = NiGetViewerString(NiGeometryData::ms_RTTI.GetName());
    uint32_t m_usSize = pkStrings->m_usSize;
    uint32_t m_usMaxSize = pkStrings->m_usMaxSize;
    char* v41 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        pkStrings->SetSize(m_usSize + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(m_usSize, v41);
    char* ViewerString_4 = NiGetViewerString("m_usVertices", m_usVertices);
    uint32_t v8 = pkStrings->m_usSize;
    uint32_t v9 = pkStrings->m_usMaxSize;
    v41 = ViewerString_4;
    if (v8 >= v9) {
        pkStrings->SetSize(v8 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v8, v41);
    char* ViewerString_8 = NiGetViewerString("m_pkVertex", static_cast<const void*>(m_pkVertex));
    uint32_t v11 = pkStrings->m_usSize;
    v41 = ViewerString_8;
    if (v11 >= pkStrings->m_usMaxSize) {
        pkStrings->SetSize(v11 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v11, v41);
    char* v12 = NiGetViewerString("m_pkNormal", static_cast<const void*>(m_pkNormal));
    uint32_t v13 = pkStrings->m_usSize;
    uint32_t v14 = pkStrings->m_usMaxSize;
    v41 = v12;
    if (v13 >= v14) {
        pkStrings->SetSize(v13 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v13, v41);
    char* ViewerString_5 = NiGetViewerString("NormalBinormalTangent", m_usDataFlags & NBT_METHOD_MASK);
    uint32_t v16 = pkStrings->m_usSize;
    uint32_t v17 = pkStrings->m_usMaxSize;
    v41 = ViewerString_5;
    if (v16 >= v17) {
        pkStrings->SetSize(v16 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v16, v41);
    char* v18 = m_kBound.GetViewerString("m_kBound");
    uint32_t v19 = pkStrings->m_usSize;
    uint32_t v20 = pkStrings->m_usMaxSize;
    v41 = v18;
    if (v19 >= v20) {
        pkStrings->SetSize(v19 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v19, v41);
    char* v21 = NiGetViewerString("m_pkColor", static_cast<const void*>(m_pkColor));
    uint32_t v22 = pkStrings->m_usSize;
    uint32_t v23 = pkStrings->m_usMaxSize;
    v41 = v21;
    if (v22 >= v23) {
        pkStrings->SetSize(v22 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v22, v41);
    char* v24 = NiGetViewerString(
        "Num Texture Sets",
        static_cast<uint16_t>(m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK)
    );
    uint32_t v25 = pkStrings->m_usSize;
    v41 = v24;
    if (v25 >= pkStrings->m_usMaxSize) {
        pkStrings->SetSize(v25 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v25, v41);
    char* v26 = NiGetViewerString("m_pkTexture", static_cast<const void*>(m_pkTexture));
    uint32_t v27 = pkStrings->m_usSize;
    uint32_t v28 = pkStrings->m_usMaxSize;
    v41 = v26;
    if (v27 >= v28) {
        pkStrings->SetSize(v27 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v27, v41);
    char* v29 = NiGetViewerString("m_usDirtyFlags", m_usDirtyFlags);
    uint32_t v30 = pkStrings->m_usSize;
    uint32_t v31 = pkStrings->m_usMaxSize;
    v41 = v29;
    if (v30 >= v31) {
        pkStrings->SetSize(v30 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v30, v41);
    char* ViewerString_2 = NiGetViewerString("m_ucKeepFlags", m_ucKeepFlags);
    uint32_t v33 = pkStrings->m_usSize;
    v41 = ViewerString_2;
    if (v33 >= pkStrings->m_usMaxSize) {
        pkStrings->SetSize(v33 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v33, v41);
    char* v34 = NiGetViewerString("m_ucCompressFlags", m_ucCompressFlags);
    uint32_t v35 = pkStrings->m_usSize;
    uint32_t v36 = pkStrings->m_usMaxSize;
    v41 = v34;
    if (v35 >= v36) {
        pkStrings->SetSize(v35 + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(v35, v41);
    NiAdditionalGeometryData* m_pObject = m_spAdditionalGeomData.m_pObject;
    if (m_pObject) {
        m_pObject->GetViewerStrings(pkStrings);
    }
    else {
        char* v38 = NiGetViewerString("m_spAdditionalGeomData", 0);
        uint32_t v39 = pkStrings->m_usSize;
        uint32_t v40 = pkStrings->m_usMaxSize;
        v41 = v38;
        if (v39 >= v40) {
            pkStrings->SetSize(v39 + pkStrings->m_usGrowBy);
        }
        pkStrings->SetAt(v39, v41);
    }
}

void NiGeometryData::SetBound(BSNiBound* apBound) {
    double dRadius;
    float fZ;
    float fRadius;
    if (apBound) {
        dRadius = m_kBound.m_fRadius;
        apBound->m_kCenter.x = m_kBound.m_kCenter.x;
        fRadius = static_cast<float>(dRadius);
        apBound->m_kCenter.y = m_kBound.m_kCenter.y;
        fZ = m_kBound.m_kCenter.z;
        apBound->m_fRadius = fRadius;
        apBound->m_kCenter.z = fZ;
    }
}

// Bytes of the vertex arrays: normals triple with NBT data, and the texture-set count is masked with 1.
uint32_t NiGeometryData::GetBlockAllocationSize() const {
    uint32_t uiSize = m_pkVertex ? m_usVertices * sizeof(NiPoint3) : 0;
    if (m_pkNormal) {
        uiSize += m_usVertices * ((m_usDataFlags & NBT_METHOD_MASK) != 0 ? 3 : 1) * sizeof(NiPoint3);
    }
    if (m_pkColor) {
        uiSize += m_usVertices * sizeof(NiColorA);
    }
    if (m_pkTexture) {
        uiSize += m_usVertices * (m_usDataFlags & BS_TEXTURE_SET_MASK) * sizeof(NiPoint2);
    }
    return uiSize + NiObject::GetBlockAllocationSize();
}

// The texture-set count is one bit wide, so of the texture coordinates only TEXCOORD0 can be present.
bool NiGeometryData::ContainsVertexData(NiShaderDeclaration::ShaderParameter eParameter) const {
    if (m_pkBuffData && m_pkBuffData->ContainsVertexData(eParameter)) {
        return true;
    }
    switch (eParameter) {
        case NiShaderDeclaration::SHADERPARAM_NI_POSITION: return m_pkVertex != nullptr;
        case NiShaderDeclaration::SHADERPARAM_NI_NORMAL: return m_pkNormal != nullptr;
        case NiShaderDeclaration::SHADERPARAM_NI_COLOR: return m_pkColor != nullptr;
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0:
            return m_pkTexture && (m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK) >= 1;
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD1:
            return m_pkTexture && (m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK) >= 2;
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD2:
            return m_pkTexture && (m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK) >= 3;
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD3:
            return m_pkTexture && (m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK) >= 4;
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD4:
            return m_pkTexture && (m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK) >= 5;
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD5:
            return m_pkTexture && (m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK) >= 6;
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD6:
            return m_pkTexture && (m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK) >= 7;
        case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD7:
            return m_pkTexture && (m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK) >= 8;
        case NiShaderDeclaration::SHADERPARAM_NI_TANGENT:
        case NiShaderDeclaration::SHADERPARAM_NI_BINORMAL:
            return m_pkNormal && (m_usDataFlags & NiGeometryData::NBT_METHOD_MASK) != 0;
        default: return false;
    }
}

void NiGeometryData::SetConsistency(NiGeometryData::Consistency eConsistency) {
    NiAdditionalGeometryData* m_pObject = m_spAdditionalGeomData.m_pObject;
    if (!m_pObject || !m_pObject->GetVBReady()) {
        m_usDirtyFlags = eConsistency | (m_usDirtyFlags & ~CONSISTENCY_MASK);
    }
}

void NiGeometryData::MarkAsChanged(uint16_t usFlags) {
    NiAdditionalGeometryData* pkAdditionalData = m_spAdditionalGeomData.m_pObject;
    if (!pkAdditionalData || !pkAdditionalData->GetVBReady()) {
        m_usDirtyFlags = static_cast<uint16_t>(m_usDirtyFlags | (usFlags & DIRTY_MASK));
    }
}

void NiGeometryData::GetVerticesIterator(NiGeometryDataIteratorNiPoint3& arIterator) {
    if (m_bVBLocked) {
        // The count is a 16-bit word; the second element of ausCount is padding.
        uint16_t ausCount[2];
        NiAdditionalGeometryData* pkAGD = m_spAdditionalGeomData.m_pObject;
        uint32_t uiStride = 0;
        ausCount[0] = 0;
        ausCount[1] = 0;
        uint8_t* pucData = nullptr;
        if (pkAGD && pkAGD->GetVBReady()) {
            uint32_t uiTotalSize;
            uint32_t uiType;
            uint32_t uiUnitSize = 0;
            pkAGD->GetDataStream(0, pucData, uiType, ausCount[0], uiTotalSize, uiUnitSize, uiStride);
        }
        if (pucData) {
            uint32_t v5 = uiStride;
            arIterator.m_pvCurrentPosition = pucData;
            arIterator.m_iStride = v5;
            arIterator.m_bCompressed = false;
        }
        else {
            NiPoint3* m_pkVertex = this->m_pkVertex;
            arIterator.m_bCompressed = false;
            arIterator.m_pvCurrentPosition = m_pkVertex;
            arIterator.m_iStride = 12;
        }
    }
}

void NiGeometryData::GetNormalsIterator(NiGeometryDataIteratorNiPoint3& arIterator) {
    if (m_bVBLocked) {
        // The count is a 16-bit word; the second element of ausCount is padding.
        uint16_t ausCount[2];
        NiAdditionalGeometryData* pkAGD = m_spAdditionalGeomData.m_pObject;
        uint32_t uiStride = 0;
        ausCount[0] = 0;
        ausCount[1] = 0;
        uint8_t* pucData = nullptr;
        if (pkAGD && pkAGD->GetVBReady()) {
            uint32_t uiTotalSize;
            uint32_t uiType;
            // Only the packed data answers GetVBReady with true; skinned data has its two blend streams first.
            uint32_t v6 = 2 *
                    (static_cast<BSPackedAdditionalGeometryData*>(pkAGD)->m_uiShaderDeclarationIndex &
                     BSPackedAdditionalGeometryData::SD_SKINNED) |
                1;
            uint32_t uiUnitSize = 0;
            pkAGD->GetDataStream(v6, pucData, uiType, ausCount[0], uiTotalSize, uiUnitSize, uiStride);
        }
        if (pucData) {
            uint32_t v5 = uiStride;
            arIterator.m_pvCurrentPosition = pucData;
            arIterator.m_iStride = v5;
            arIterator.m_bCompressed = false;
        }
        else {
            NiPoint3* m_pkNormal = this->m_pkNormal;
            arIterator.m_bCompressed = false;
            arIterator.m_pvCurrentPosition = m_pkNormal;
            arIterator.m_iStride = 12;
        }
    }
}

// usSet is unused: only the first texture set is walked.
void NiGeometryData::GetTextureSetIterator(uint16_t usSet, NiGeometryDataIteratorNiPoint2& arIterator) {
    if (m_bVBLocked) {
        // The count is a 16-bit word; the second element of ausCount is padding.
        uint16_t ausCount[2];
        NiAdditionalGeometryData* pkAGD = m_spAdditionalGeomData.m_pObject;
        uint32_t uiStride = 0;
        ausCount[0] = 0;
        ausCount[1] = 0;
        uint8_t* pucData = nullptr;
        if (pkAGD && pkAGD->GetVBReady()) {
            uint32_t uiTotalSize;
            uint32_t uiType;
            // Only the packed data answers GetVBReady with true; the texture stream follows the position and normal
            // streams, the two blend streams of skinned data and the colour stream.
            uint32_t uiFlags = static_cast<BSPackedAdditionalGeometryData*>(pkAGD)->m_uiShaderDeclarationIndex;
            uint32_t uiUnitSize = 0;
            pkAGD->GetDataStream(
                (uiFlags >> 1 & 1) + (2 * ((uiFlags & BSPackedAdditionalGeometryData::SD_SKINNED) != 0) + 2),
                pucData,
                uiType,
                ausCount[0],
                uiTotalSize,
                uiUnitSize,
                uiStride
            );
        }
        if (pucData) {
            uint32_t v7 = uiStride;
            arIterator.m_pvCurrentPosition = pucData;
            arIterator.m_iStride = v7;
            arIterator.m_bCompressed = false;
        }
        else {
            NiPoint2* m_pkTexture = this->m_pkTexture;
            arIterator.m_bCompressed = false;
            arIterator.m_pvCurrentPosition = m_pkTexture;
            arIterator.m_iStride = 8;
        }
    }
}

void NiGeometryData::GetBlendWeightsIterator(NiGeometryDataIteratorNiBlendWeights& arIterator) {
    if (m_bVBLocked) {
        // The count is a 16-bit word; the second element of ausCount is padding.
        uint16_t ausCount[2];
        NiAdditionalGeometryData* pkAGD = m_spAdditionalGeomData.m_pObject;
        uint32_t uiStride = 0;
        ausCount[0] = 0;
        ausCount[1] = 0;
        uint8_t* pucData = nullptr;
        // Tests BSPackedAdditionalGeometryData's flags on any additional data whose VB is ready: only the packed class
        // reports a ready VB.
        if (pkAGD &&
            pkAGD->GetVBReady() &&
            (static_cast<BSPackedAdditionalGeometryData*>(pkAGD)->m_uiShaderDeclarationIndex &
             BSPackedAdditionalGeometryData::SD_SKINNED) != 0) {
            uint32_t uiTotalSize;
            uint32_t uiType;
            uint32_t uiUnitSize = 0;
            pkAGD->GetDataStream(1u, pucData, uiType, ausCount[0], uiTotalSize, uiUnitSize, uiStride);
        }
        arIterator.m_pvCurrentPosition = pucData;
        arIterator.m_iStride = uiStride;
        arIterator.m_bCompressed = false;
    }
}

void NiGeometryData::GetBlendIndicesIterator(NiGeometryDataIteratorNiBlendIndices& arIterator) {
    if (m_bVBLocked) {
        // The count is a 16-bit word; the second element of ausCount is padding.
        uint16_t ausCount[2];
        NiAdditionalGeometryData* pkAGD = m_spAdditionalGeomData.m_pObject;
        uint32_t uiStride = 0;
        ausCount[0] = 0;
        ausCount[1] = 0;
        uint8_t* pucData = nullptr;
        // Only the packed data answers GetVBReady with true; only skinned packed data has blend indices.
        if (pkAGD &&
            pkAGD->GetVBReady() &&
            (static_cast<BSPackedAdditionalGeometryData*>(pkAGD)->m_uiShaderDeclarationIndex &
             BSPackedAdditionalGeometryData::SD_SKINNED) != 0) {
            uint32_t uiTotalSize;
            uint32_t uiType;
            uint32_t uiUnitSize = 0;
            pkAGD->GetDataStream(2u, pucData, uiType, ausCount[0], uiTotalSize, uiUnitSize, uiStride);
        }
        uint32_t v3 = uiStride;
        arIterator.m_pvCurrentPosition = pucData;
        arIterator.m_iStride = v3;
        arIterator.m_bCompressed = false;
    }
}

NiAdditionalGeometryData* NiGeometryData::GetAdditionalGeometryData() const {
    return m_spAdditionalGeomData;
}

void NiGeometryData::SetAdditionalGeometryData(NiAdditionalGeometryData* pkAddData) {
    m_spAdditionalGeomData = pkAddData;
}

void NiGeometryData::ReplaceData(
    uint16_t usVertices,
    NiPoint3* pkVertex,
    NiPoint3* pkNormal,
    NiColorA* pkColor,
    NiPoint2* pkTexture,
    uint16_t usNumTextureSets,
    DataFlags eNBTMethod
) {
    m_usVertices = usVertices;
    m_pkVertex = pkVertex;
    m_pkNormal = pkNormal;
    m_pkColor = pkColor;
    uint16_t usDataFlags = m_usDataFlags;
    m_pkTexture = pkTexture;
    m_usDataFlags = static_cast<uint16_t>(
        (usDataFlags & ~(NBT_METHOD_MASK | BS_TEXTURE_SET_MASK)) | (usNumTextureSets & BS_TEXTURE_SET_MASK) | eNBTMethod
    );
}

bool NiGeometryData::LockPackedVertexData(bool bWriteAccess) {
    if (m_bVBLocked) {
        return false;
    }

    NiAdditionalGeometryData* pkAdditionalData = m_spAdditionalGeomData.m_pObject;
    if (pkAdditionalData && pkAdditionalData->GetVBReady()) {
        if (bWriteAccess) {
            pkAdditionalData->UnlockDataBlock(0);
            m_bVBLockWrite = bWriteAccess;
            m_bVBLocked = true;
            return true;
        }
        uint32_t uiBlockSize = 0;
        pkAdditionalData->LockDataBlock(0, uiBlockSize);
    }
    m_bVBLockWrite = bWriteAccess;
    m_bVBLocked = true;
    return true;
}

void NiGeometryData::UnlockPackedVertexData() {
    if (m_bVBLocked) {
        NiAdditionalGeometryData* pkAdditionalData = m_spAdditionalGeomData.m_pObject;
        if (pkAdditionalData && pkAdditionalData->GetVBReady() && !m_bVBLockWrite) {
            pkAdditionalData->UnlockDataBlock(0);
        }
        m_bVBLocked = false;
    }
}

void NiGeometryData::Replace(
    uint16_t usVertices,
    NiPoint3* pkVertex,
    NiPoint3* pkNormal,
    NiColorA* pkColor,
    NiPoint2* pkTexture,
    uint16_t usNumTextureSets,
    NiGeometryData::DataFlags eNBTMethod
) {
    if (GetGroup()) {
        if (pkVertex != m_pkVertex) {
            NiObjectGroup* pkGroup = GetGroup();
            if (pkGroup->m_uiRefCount-- == 1) {
                pkGroup->FreeVertexBuffer();
                delete pkGroup;
            }
        }
        if (pkNormal != m_pkNormal) {
            NiObjectGroup* pkGroup = GetGroup();
            if (pkGroup->m_uiRefCount-- == 1) {
                pkGroup->FreeVertexBuffer();
                delete pkGroup;
            }
        }
        if (pkColor != m_pkColor) {
            NiObjectGroup* pkGroup = GetGroup();
            if (pkGroup->m_uiRefCount-- == 1) {
                pkGroup->FreeVertexBuffer();
                delete pkGroup;
            }
        }
        if (pkTexture != m_pkTexture) {
            NiObjectGroup* pkGroup = GetGroup();
            if (pkGroup->m_uiRefCount-- == 1) {
                pkGroup->FreeVertexBuffer();
                delete pkGroup;
            }
        }
    }
    else {
        if (pkVertex != m_pkVertex) {
            delete[] m_pkVertex;
        }
        if (pkNormal != m_pkNormal) {
            delete[] m_pkNormal;
        }
        if (pkColor != m_pkColor) {
            delete[] m_pkColor;
        }
        if (pkTexture != m_pkTexture) {
            delete[] m_pkTexture;
        }
    }
    m_pkVertex = pkVertex;
    m_usVertices = usVertices;
    m_pkNormal = pkNormal;
    uint16_t usActiveVertices = GetActiveVertexCount();
    if (usActiveVertices && m_pkVertex) {
        m_kBound.ComputeFromData(usActiveVertices, m_pkVertex);
    }

    uint16_t usDataFlags = m_usDataFlags;
    m_pkColor = pkColor;
    m_pkTexture = pkTexture;
    m_usDataFlags = eNBTMethod |
        ((usNumTextureSets & BS_TEXTURE_SET_MASK) | (usDataFlags & ~(NBT_METHOD_MASK | BS_TEXTURE_SET_MASK)));
}

void NiGeometryData::CreateNormals(bool bInitialize) {
    int32_t v3 = 1;
    if ((m_usDataFlags & NBT_METHOD_MASK) != 0) {
        v3 = 3;
    }
    if (!m_pkNormal) {
        m_pkNormal = new NiPoint3[v3 * m_usVertices];
    }
    if (bInitialize) {
        std::memset(m_pkNormal, 0, sizeof(NiPoint3) * (v3 * m_usVertices));
    }
}

// Deliberately zeroes only one NiPoint3 past the copied normals, rather than the two vertex arrays of
// normals that the Gamebryo source memsets.
void NiGeometryData::StoreNormalBinormalTangent(DataFlags eNBTMethod) {
    if ((m_usDataFlags & NBT_METHOD_MASK) == eNBTMethod) {
        return;
    }
    m_usDataFlags = eNBTMethod | (m_usDataFlags & ~NBT_METHOD_MASK);
    if (!m_pkNormal) {
        return;
    }
    uint32_t uiFactor = 1;
    if (eNBTMethod) {
        uiFactor = 3;
    }
    NiPoint3* pkNormal = new NiPoint3[uiFactor * m_usVertices];
    for (uint32_t uiIndex = 0; uiIndex < m_usVertices; ++uiIndex) {
        pkNormal[uiIndex] = m_pkNormal[uiIndex];
    }
    if (eNBTMethod) {
        pkNormal[m_usVertices].x = 0.0f;
        pkNormal[m_usVertices].y = 0.0f;
        pkNormal[m_usVertices].z = 0.0f;
    }
    delete[] m_pkNormal;
    m_pkNormal = pkNormal;
}

void NiGeometryData::SetKeepFlags(uint32_t uiKeepFlags) {
    m_ucKeepFlags = static_cast<uint8_t>(uiKeepFlags);
}

void NiGeometryData::SetData(
    uint16_t usVertices,
    NiPoint3* pkVertex,
    NiPoint3* pkNormal,
    NiColorA* pkColor,
    NiPoint2* pkTexture,
    uint16_t usNumTextureSets,
    NiGeometryData::DataFlags eNBTMethod
) {
    m_usVertices = usVertices;
    m_pkVertex = pkVertex;
    m_pkNormal = pkNormal;
    uint16_t usActiveVertices = GetActiveVertexCount();
    if (usActiveVertices && m_pkVertex) {
        m_kBound.ComputeFromData(usActiveVertices, m_pkVertex);
    }

    uint16_t usDataFlags = m_usDataFlags;
    m_pkTexture = pkTexture;
    m_pkColor = pkColor;
    m_usDataFlags = eNBTMethod |
        ((usNumTextureSets & BS_TEXTURE_SET_MASK) | (usDataFlags & ~(NBT_METHOD_MASK | BS_TEXTURE_SET_MASK)));
}

// Orders two vertices by the arrays mask selects: positions, normals, colours, texture coordinates, each compared
// float by float (v1's against v0's).
int32_t NiGeometryData::VertexCompare(uint16_t v0, uint16_t v1, int32_t mask) const {
    NiPoint3* pkVertex = m_pkVertex;
    if ((mask & 1) != 0) {
        const float* pf1 = &pkVertex[v1].x;
        const float* pf0 = &pkVertex[v0].x;
        for (uint32_t i = 0; i < 3; ++i) {
            if (pf1[i] > pf0[i]) {
                return -1;
            }
            if (pf1[i] < pf0[i]) {
                return 1;
            }
        }
    }
    NiPoint3* pkNormal = m_pkNormal;
    if (pkNormal && (mask & 2) != 0) {
        int32_t iCmp = NiGeometryData::FloatCompare(&pkNormal[v1].x, &pkNormal[v0].x, 3);
        if (iCmp) {
            return iCmp;
        }
    }
    NiColorA* pkColor = m_pkColor;
    if (pkColor && (mask & 4) != 0) {
        int32_t iCmp = NiGeometryData::FloatCompare(&pkColor[v1].r, &pkColor[v0].r, 4);
        if (iCmp) {
            return iCmp;
        }
    }
    NiPoint2* pkTexture = m_pkTexture;
    if (pkTexture && (mask & 8) != 0) {
        uint32_t uiVertices = m_usVertices;
        uint16_t usSets = m_usDataFlags & NiGeometryData::BS_TEXTURE_SET_MASK;
        uint16_t usBase = 0;
        for (uint16_t usSet = 0; usSet < usSets; ++usSet) {
            const float* pf1 = &pkTexture[usBase + v1].x;
            const float* pf0 = &pkTexture[usBase + v0].x;
            for (uint32_t i = 0; i < 2; ++i) {
                if (pf1[i] > pf0[i]) {
                    return -1;
                }
                if (pf1[i] < pf0[i]) {
                    return 1;
                }
            }
            usBase += uiVertices;
        }
    }
    return 0;
}

// Passes the caller's whole mask on to VertexCompare.
int32_t NiGeometryData::ChoosePivot(uint16_t* pusIndex, int32_t mask, int32_t l, int32_t r) const {
    uint16_t v6 = pusIndex[(l + r) >> 1];
    uint16_t la = pusIndex[l];
    if (VertexCompare(la, v6, mask) < 0) {
        uint16_t v7 = pusIndex[r];
        if (VertexCompare(v6, v7, mask) >= 0) {
            if (VertexCompare(la, v7, mask) >= 0) {
                return la;
            }

            return v7;
        }
        return v6;
    }
    uint16_t v9 = pusIndex[r];
    if (VertexCompare(la, v9, mask) < 0) {
        return la;
    }
    bool v10 = VertexCompare(v6, v9, mask) < 0;
    int32_t result = v9;
    if (!v10) {
        return v6;
    }
    return result;
}

void NiGeometryData::IndexQSort(uint16_t* pusIndex, int32_t mask, int32_t l, int32_t r) const {
    int32_t v5 = l;
    int32_t v7 = r;
    if (r > l) {
        while (true) {
            int32_t v9 = v5 - 1;
            int32_t v10 = v7 + 1;
            uint16_t pusIndexa = ChoosePivot(pusIndex, mask, v5, v7);
            while (true) {
                uint16_t v11;
                uint16_t v12;
                do {
                    v11 = pusIndex[--v10];
                }
                while (VertexCompare(pusIndexa, v11, mask) < 0);
                do {
                    v12 = pusIndex[++v9];
                }
                while (VertexCompare(v12, pusIndexa, mask) < 0);
                if (v9 >= v10) {
                    break;
                }
                uint16_t v13 = pusIndex[v9];
                pusIndex[v9] = pusIndex[v10];
                pusIndex[v10] = v13;
            }
            if (v10 == r) {
                r = v10 - 1;
            }
            else {
                IndexQSort(pusIndex, mask, l, v10);
                l = v10 + 1;
            }
            if (r <= l) {
                break;
            }
            v7 = r;
            v5 = l;
        }
    }
}

void NiGeometryData::IndexSort(uint16_t*& pusIndex0, uint16_t*& pusIndex1, int32_t mask) const {
    uint16_t usVertices = m_usVertices;
    pusIndex0 = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * usVertices));
    pusIndex1 = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * usVertices));
    for (uint16_t i = 0; i < usVertices; i++) {
        pusIndex0[i] = i;
    }
    IndexQSort(pusIndex0, mask, 0, usVertices - 1);
    for (uint16_t i = 0; i < usVertices; i++) {
        pusIndex1[pusIndex0[i]] = i;
    }
}

void NiGeometryData::GetColorsIterator(NiGeometryDataIteratorNiColorA& arIterator) {
    if (m_bVBLocked) {
        void* pvPosition;
        uint32_t uiIterStride;
        // The count is a 16-bit word; the second element of ausCount is padding.
        uint16_t ausCount[2];
        NiAdditionalGeometryData* pkAGD = m_spAdditionalGeomData.m_pObject;
        bool bCompressed = false;
        uint32_t uiStride = 0;
        ausCount[0] = 0;
        ausCount[1] = 0;
        uint8_t* pucData = nullptr;
        if (pkAGD && pkAGD->GetVBReady()) {
            uint32_t uiTotalSize;
            uint32_t uiType;
            // Only the packed data answers GetVBReady with true; its shader declaration flags say which streams it has.
            uint32_t uiFlags = static_cast<BSPackedAdditionalGeometryData*>(pkAGD)->m_uiShaderDeclarationIndex;
            if ((uiFlags & BSPackedAdditionalGeometryData::SD_COLOR) == 0) {
                return;
            }
            uint32_t uiUnitSize = 0;
            pkAGD->GetDataStream(
                2 * ((uiFlags & BSPackedAdditionalGeometryData::SD_SKINNED) != 0) + 2,
                pucData,
                uiType,
                ausCount[0],
                uiTotalSize,
                uiUnitSize,
                uiStride
            );
            bCompressed = (uiFlags & BSPackedAdditionalGeometryData::SD_COMP_COLORS) != 0;
        }
        pvPosition = pucData;
        if (pucData) {
            uiIterStride = uiStride;
        }
        else {
            pvPosition = this->m_pkColor;
            uiIterStride = 16;
        }
        arIterator.m_pvCurrentPosition = pvPosition;
        arIterator.m_iStride = uiIterStride;
        arIterator.m_bCompressed = bCompressed;
    }
}

void NiGeometryData::ClearNormals() {
    if (m_pkNormal) {
        delete[] m_pkNormal;
        m_pkNormal = nullptr;
    }
}

int32_t NiGeometryData::FloatCompare(const float* pf0, const float* pf1, uint32_t uiNum) {
    for (uint32_t i = 0; i < uiNum; ++i) {
        if (pf0[i] > pf1[i]) {
            return -1;
        }
        if (pf0[i] < pf1[i]) {
            return 1;
        }
    }
    return 0;
}

// Reads all the values in one read, as floats.
void NiPoint2::LoadBinary(NiStream& stream, NiPoint2* pkValues, uint32_t uiNumValues) {
    NiStreamLoadBinary(stream, reinterpret_cast<float*>(pkValues), uiNumValues * 2);
}
