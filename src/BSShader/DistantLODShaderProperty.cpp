#include "BSShader/DistantLODShaderProperty.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSStream.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/DistantLODGroup.h"
#include "BSShader/ParticleShader.h"
#include "BSShader/ParticleShaderProperty.h"
#include "BSShader/ShaderBuffer.h"
#include "BSShader/SpeedTreeBillboardShaderProperty.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeDynamicData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsDynamicData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

// The game settings this file defines, with their IDs and values, in initialization order.
NiRTTI DistantLODShaderProperty::ms_RTTI("DistantLODShaderProperty", &BSShaderLightingProperty::ms_RTTI);
SettingT<INISettingCollection> iDistantLODGroupWidth("iDistantLODGroupWidth:DistantLOD", 8);

// Deletes every cached distant LOD in DistantLODShaderProperty::GetDistantLODMap's map, empties the map and clears the
// two LOD counts.
void ClearDistantLODMap() {
    NiTMapItem<uint32_t, DistantLODShaderProperty::CachedDistantLOD*>* pos_1 = nullptr;
    NiTMapItem<uint32_t, DistantLODShaderProperty::CachedDistantLOD*>* pos;
    NiTPointerMap<uint32_t, DistantLODShaderProperty::CachedDistantLOD*>*
        v0 = DistantLODShaderProperty::GetDistantLODMap();
    uint32_t m_uiHashSize = v0->m_uiHashSize;
    uint32_t m_uiHashSize_1 = 0;
    if (m_uiHashSize) {
        NiTMapItem<uint32_t, DistantLODShaderProperty::CachedDistantLOD*>** m_ppkHashTable = v0->m_ppkHashTable;
        NiTMapItem<uint32_t, DistantLODShaderProperty::CachedDistantLOD*>** m_ppkHashTable_1 = m_ppkHashTable;
        for (;;) {
            if (*m_ppkHashTable_1) {
                pos_1 = m_ppkHashTable[m_uiHashSize_1];
                break;
            }
            ++m_uiHashSize_1;
            ++m_ppkHashTable_1;
            if (m_uiHashSize_1 >= m_uiHashSize) {
                break;
            }
        }
    }
    pos = pos_1;
    DistantLODShaderProperty::CachedDistantLOD* val_ = nullptr;
    if (DistantLODShaderProperty::GetDistantLODMap()->m_kAllocator.m_uiCount && pos_1) {
        do {
            uint32_t key;
            DistantLODShaderProperty::GetDistantLODMap()->GetNext(pos, key, val_);
            delete val_;
        }
        while (pos);
    }

    DistantLODShaderProperty::GetDistantLODMap()->RemoveAll();
    DistantLODShaderProperty::uiLODGeometryCount = 0;
    DistantLODShaderProperty::uiLODCount = 0;
}

DistantLODShaderProperty::DistantLODParam::DistantLODParam()
  : pcGeomFilename(nullptr), p3DData(nullptr), iBaseObjectID(0) {}

DistantLODShaderProperty::DistantLODShaderProperty() : pDistantLODGroup(nullptr), spBaseTexture(nullptr) {
    iShader = 3;
    iShaderPropertyType = 2;
}

DistantLODShaderProperty::~DistantLODShaderProperty() {
    pDistantLODGroup = nullptr;
}

// The clone is built in an engine block by the class's constructor; the copy body fills in the members.
NiObject* DistantLODShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
    DistantLODShaderProperty* pClone = new DistantLODShaderProperty;
    CopyMembers(pClone, arCloneData);
    return pClone;
}

void DistantLODShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderLightingProperty::GetViewerStrings(apStrings);
}

void DistantLODShaderProperty::SetClampD3D(D3DTEXTUREADDRESS aClampMode) {
    TextureClampMode = (aClampMode != D3DTADDRESS_CLAMP) ? NiTexturingProperty::WRAP_S_WRAP_T
                                                         : NiTexturingProperty::CLAMP_S_CLAMP_T;
}

// The class's own copy body.
void DistantLODShaderProperty::CopyMembers(DistantLODShaderProperty* pDest, NiCloningProcess& arCloneData) {
    BSShaderLightingProperty::CopyMembers(pDest, arCloneData);
    pDest->spBaseTexture = spBaseTexture;
    pDest->TextureClampMode = TextureClampMode;
    pDest->pDistantLODParam = pDistantLODParam;
    pDest->pDistantLODGroup = pDistantLODGroup;
}

void DistantLODShaderProperty::SetLODTreeParent(NiNode* apLODTreeParent) {
    pLODTreeParent = apLODTreeParent;
}

// Finds the cached geometry under the base object's form ID, or builds it: the first geometry child of the object's
// node (loaded from pcGeomFilename when there is no node yet) is copied inuminstances times into one dynamic tri shape
// or tri strips data, each copy's vertices tagged with its instance number in pInstanceIndex, and cached with the
// billboard texture, two alpha properties and the stencil property. The cache keeps the only reference to the returned
// geometry. When the node has no geometry child, its children are searched forever.
DistantLODShaderProperty::CachedGeometry*
DistantLODShaderProperty::GetCachedGeometry(DistantLODParam* apDistantLODParam) {
    if (!BSShaderManager::eShaderLevel) {
        return nullptr;
    }
    NiGeometry* plodgeom = nullptr;
    BSStream kStream;
    BSStream* pStream = &kStream;
    uint32_t uiBaseObjectID = apDistantLODParam->iBaseObjectID;
    if (!uiBaseObjectID) {
        return nullptr;
    }
    NiPointer<CachedGeometry> spCachedGeom;
    if (!GetCachedGeometryMap()->GetAt(uiBaseObjectID, spCachedGeom)) {
        if ((!apDistantLODParam->p3DData && apDistantLODParam->pcGeomFilename) &&
            pStream->BSStream::Load(apDistantLODParam->pcGeomFilename, nullptr))
        // BSStream's Load is called directly because the stream is a local object.
        {
            NiObject* pobj = pStream->m_kTopObjects.m_pBase[0].m_pObject;
            if (pobj) {
                apDistantLODParam->p3DData = pobj->IsNode();
            }
        }

        NiNode* parent = apDistantLODParam->p3DData;
        if (parent) {
            while (!plodgeom) {
                for (NiAVObject* pavobj : parent->m_kChildren) {
                    if (pavobj) {
                        plodgeom = pavobj->IsGeometry();
                        if (plodgeom) {
                            break;
                        }
                    }
                }
            }
            NiObject* ptristrips = plodgeom->DynamicCastMember(&NiTriStrips::ms_RTTI);
            NiGeometryData* pgeomdata = plodgeom->m_spModelData.m_pObject;
            uint16_t inuminstances = (BSShaderManager::eShaderLevel < 2) ? 80 : 228;
            bool bstrips;
            int32_t ivertices;
            float* pinstance;
            uint16_t usinstanceTriCount;
            NiTriBasedGeomData* pnewTriData = nullptr;
            if (!ptristrips) {
                bstrips = false;
                NiTriShapeData* ptrishapedata = pgeomdata
                    ? static_cast<NiTriShapeData*>(pgeomdata->DynamicCastMember(&NiTriShapeData::ms_RTTI))
                    : nullptr;
                NiPoint3* pvertex = ptrishapedata->m_pkVertex;
                NiPoint3* pnormal = ptrishapedata->m_pkNormal;
                uint16_t usVertices = ptrishapedata->m_usVertices;
                uint32_t uiTriListLength = ptrishapedata->m_uiTriListLength;
                NiColorA* pcolor = ptrishapedata->m_pkColor;
                NiPoint2* ptex = ptrishapedata->m_pkTexture;
                uint16_t* pindex = ptrishapedata->GetTriList();
                ivertices = usVertices;
                // All the copies together stay within 0xFFFF vertices and 0xFFFF indices (signed divisions).
                uint16_t usLimit = static_cast<uint16_t>(0xFFFF / ivertices);
                if (inuminstances >= usLimit) {
                    inuminstances = usLimit;
                }
                int32_t iindices = static_cast<uint16_t>(uiTriListLength);
                usLimit = static_cast<uint16_t>(0xFFFF / iindices);
                if (inuminstances >= usLimit) {
                    inuminstances = usLimit;
                }
                int32_t iInstances = inuminstances;
                uint32_t uiCount = static_cast<uint32_t>(iInstances * ivertices);
                NiPoint3* pgvertex = new NiPoint3[uiCount];
                NiPoint3* pgnormal = new NiPoint3[uiCount];
                NiColorA* pgcolor = new NiColorA[uiCount];
                NiPoint2* pgtex = new NiPoint2[uiCount];
                pinstance = new float[uiCount];
                uint16_t* pgindex = new uint16_t[static_cast<uint32_t>(iindices * iInstances)];
                uint32_t uiIndex = 0;
                for (int32_t i = 0; i < inuminstances; ++i) {
                    int32_t ivertoff = i * ivertices;
                    if (ivertices > 0) {
                        // Rounds the instance number to a float once per copy.
                        float fInstance = static_cast<float>(i);
                        for (int32_t j = 0; j < ivertices; ++j) {
                            pgvertex[ivertoff + j] = pvertex[j];
                            pgnormal[ivertoff + j] = pnormal[j];
                            pgcolor[ivertoff + j] = pcolor[j];
                            pgtex[ivertoff + j] = ptex[j];
                            pinstance[ivertoff + j] = fInstance;
                        }
                    }
                    for (int32_t j = 0; j < iindices; ++j) {
                        pgindex[uiIndex++] = static_cast<uint16_t>(pindex[j] + ivertoff);
                    }
                }
                int32_t iTriangles = iindices / 3;
                usinstanceTriCount = static_cast<uint16_t>(iTriangles);
                pnewTriData = new NiTriShapeDynamicData(
                    static_cast<uint16_t>(inuminstances * ivertices),
                    pgvertex,
                    pgnormal,
                    pgcolor,
                    pgtex,
                    1,
                    NiGeometryData::NBT_METHOD_NONE,
                    static_cast<uint16_t>(iTriangles * inuminstances),
                    pgindex,
                    static_cast<uint16_t>(inuminstances * ivertices),
                    0
                );
            }
            else {
                bstrips = true;
                NiTriStripsData* ptristripsdata = pgeomdata
                    ? static_cast<NiTriStripsData*>(pgeomdata->DynamicCastMember(&NiTriStripsData::ms_RTTI))
                    : nullptr;
                NiPoint3* pnormal = ptristripsdata->m_pkNormal;
                NiPoint3* pvertex = ptristripsdata->m_pkVertex;
                uint16_t usVertices = ptristripsdata->m_usVertices;
                // Only the first strip is replicated.
                uint16_t* pusStripLengths = ptristripsdata->m_pusStripLengths;
                NiPoint2* ptex = ptristripsdata->m_pkTexture;
                NiColorA* pcolor = ptristripsdata->m_pkColor;
                uint16_t* pindex = ptristripsdata->GetStripListsCall();
                ivertices = usVertices;
                uint16_t usLimit = static_cast<uint16_t>(0xFFFF / ivertices);
                if (inuminstances >= usLimit) {
                    inuminstances = usLimit;
                }
                // Each copy after the first joins the strip through two repeated indices, plus one more after a
                // strip of odd length (which keeps the winding).
                int32_t striplength = *pusStripLengths;
                usLimit = static_cast<uint16_t>(0xFFFF / (striplength + ((striplength % 2) != 0) + 2));
                if (inuminstances >= usLimit) {
                    inuminstances = usLimit;
                }
                int32_t iInstances = inuminstances;
                uint32_t uiCount = static_cast<uint32_t>(iInstances * ivertices);
                NiPoint3* pgvertex = new NiPoint3[uiCount];
                NiPoint3* pgnormal = new NiPoint3[uiCount];
                NiColorA* pgcolor = new NiColorA[uiCount];
                NiPoint2* pgtex = new NiPoint2[uiCount];
                pinstance = new float[uiCount];
                // Reads the strip length again and keeps the stride to 16 bits here.
                striplength = *pusStripLengths;
                uint16_t usStride = static_cast<uint16_t>(striplength + (((striplength % 2) != 0) + 2));
                uint16_t* pgindex = new uint16_t[static_cast<uint32_t>((iInstances - 1) * usStride + striplength)];
                uint32_t newinstanceindexcount = 0;
                int32_t iLastInstance = iInstances - 1;
                for (int32_t i = 0; i < inuminstances; ++i) {
                    int32_t ivertoff = i * ivertices;
                    rt::memcpy(pgvertex + ivertoff, pvertex, ivertices * sizeof(NiPoint3));
                    rt::memcpy(pgnormal + ivertoff, pnormal, ivertices * sizeof(NiPoint3));
                    rt::memcpy(pgcolor + ivertoff, pcolor, ivertices * sizeof(NiColorA));
                    rt::memcpy(pgtex + ivertoff, ptex, ivertices * sizeof(NiPoint2));
                    if (ivertices > 0) {
                        // Rounds the instance number to a float once per copy.
                        float fInstance = static_cast<float>(i);
                        for (int32_t j = 0; j < ivertices; ++j) {
                            pinstance[ivertoff + j] = fInstance;
                        }
                    }
                    for (int32_t j = 0; j < *pusStripLengths; ++j) {
                        pgindex[newinstanceindexcount++] = static_cast<uint16_t>(pindex[j] + ivertoff);
                    }
                    if (i < iLastInstance) {
                        // Truncates the copy number to 16 bits for the joining indices.
                        uint16_t usInstance = static_cast<uint16_t>(i);
                        pgindex[newinstanceindexcount++] = static_cast<uint16_t>(
                            usInstance * ivertices + pindex[*pusStripLengths - 1]
                        );
                        int32_t iNextOffset = (usInstance + 1) * ivertices;
                        pgindex[newinstanceindexcount++] = static_cast<uint16_t>(pindex[0] + iNextOffset);
                        if (*pusStripLengths % 2 == 1) {
                            pgindex[newinstanceindexcount++] = static_cast<uint16_t>(pindex[0] + iNextOffset);
                        }
                    }
                }
                uint16_t* pusNewStripLengths = new uint16_t(static_cast<uint16_t>(newinstanceindexcount));
                striplength = *pusStripLengths;
                usinstanceTriCount = (striplength % 2 == 1) ? static_cast<uint16_t>(striplength + 3)
                                                            : static_cast<uint16_t>(striplength + 2);
                pnewTriData = new NiTriStripsDynamicData(
                    static_cast<uint16_t>(inuminstances * ivertices),
                    pgvertex,
                    pgnormal,
                    pgcolor,
                    pgtex,
                    1,
                    0,
                    static_cast<uint16_t>(newinstanceindexcount - 2),
                    1,
                    pusNewStripLengths,
                    pgindex,
                    static_cast<uint16_t>(inuminstances * ivertices),
                    0
                );
            }
            NiPointer<NiTexture> pbasetexture;
            if (plodgeom->GetProperty(NiProperty::SHADE) &&
                static_cast<NiShadeProperty*>(plodgeom->GetProperty(NiProperty::SHADE))->iShaderPropertyType ==
                    BSShaderManager::BSSM_PROPERTYTYPE_SPEEDTREEBILLBOARD) {
                SpeedTreeBillboardShaderProperty* pprop = static_cast<SpeedTreeBillboardShaderProperty*>(
                    plodgeom->GetProperty(NiProperty::SHADE)
                );
                if (pprop) {
                    NiTexture* pTexture = pprop->GetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0);
                    if (pTexture) {
                        pbasetexture = pTexture;
                    }
                }
            }
            plodgeom->RemoveProperty(NiProperty::ALPHA);
            NiAlphaProperty* palphablend = new NiAlphaProperty;
            palphablend->m_ucAlphaTestRef = 0;
            palphablend->m_uFlags = (palphablend->m_uFlags & ~NiAlphaProperty::TEST_FUNC_MASK) |
                NiAlphaProperty::ALPHA_BLEND_MASK | NiAlphaProperty::TEST_ENABLE_MASK |
                (NiAlphaProperty::TEST_GREATER << NiAlphaProperty::TEST_FUNC_POS);
            NiAlphaProperty* palphanoblend = new NiAlphaProperty;
            palphanoblend->m_uFlags = (palphanoblend->m_uFlags &
                                       ~(NiAlphaProperty::ALPHA_BLEND_MASK | NiAlphaProperty::TEST_FUNC_MASK)) |
                NiAlphaProperty::TEST_ENABLE_MASK | (NiAlphaProperty::TEST_GREATER << NiAlphaProperty::TEST_FUNC_POS);
            palphanoblend->m_ucAlphaTestRef = 0;
            NiStencilProperty* pstencil = static_cast<NiStencilProperty*>(plodgeom->GetProperty(NiProperty::STENCIL));
            float fBoundRadius = plodgeom->m_spModelData->m_kBound.m_fRadius;
            CachedGeometry* pCachedGeom = new CachedGeometry;
            spCachedGeom = pCachedGeom;
            pCachedGeom->bTristrip = bstrips;
            pCachedGeom->spTriGeomData = pnewTriData;
            pCachedGeom->pInstanceIndex = pinstance;
            pCachedGeom->iVertices = ivertices;
            pCachedGeom->spBaseTexture = pbasetexture;
            pCachedGeom->spAlphaNoBlend = palphanoblend;
            pCachedGeom->spAlphaBlend = palphablend;
            pCachedGeom->spStencil = pstencil;
            pCachedGeom->fBoundRadius = fBoundRadius;
            pCachedGeom->iMaxInstanceCount = inuminstances;
            pCachedGeom->usInstanceTriCount = usinstanceTriCount;
            pCachedGeom->bBillboard = false;
            if (NiObject::DynamicCast(&NiBillboardNode::ms_RTTI, apDistantLODParam->p3DData)) {
                pCachedGeom->bBillboard = true;
            }
            GetCachedGeometryMap()->SetAt(apDistantLODParam->iBaseObjectID, NiPointer<CachedGeometry>(pCachedGeom));
        }
    }
    NiRefObject* pResult = spCachedGeom.m_pObject;
    if (!pResult) {
        char message[200];
        BSsprintf(
            message,
            sizeof(message),
            "DISTANT LOD ERROR : could not load DistantLOD nif %s",
            apDistantLODParam->pcGeomFilename
        );
    }
    if (pResult) {
        pResult->DecRefCount();
    }
    // Released above: the smart pointer's own destructor has nothing left to do.
    spCachedGeom.m_pObject = nullptr;
    // The stream's destructor runs on return.
    return static_cast<CachedGeometry*>(pResult);
}

NiTPointerMap<uint32_t, NiPointer<DistantLODShaderProperty::CachedGeometry>>*
DistantLODShaderProperty::GetCachedGeometryMap() {
    static pmCachedGeometryMap_t pmCachedGeometryMap;
    return &pmCachedGeometryMap;
}

bool DistantLODShaderProperty::SetDistantLODCulled(bool abDistLODStatus) {
    bool result = (pLODTreeParent->m_uFlags & NiAVObject::APP_CULLED) != 0;
    if (abDistLODStatus) {
        pLODTreeParent->m_uFlags |= NiAVObject::APP_CULLED;
    }
    else {
        pLODTreeParent->m_uFlags &= ~NiAVObject::APP_CULLED;
    }
    return result;
}

bool DistantLODShaderProperty::HasCachedGeometry(uint32_t aiFormID) {
    bool bRetVal = false;
    if (aiFormID) {
        NiPointer<CachedGeometry> spCachedGeom;
        if (GetCachedGeometryMap()->GetAt(aiFormID, spCachedGeom) && spCachedGeom) {
            bRetVal = true;
        }
    }
    return bRetVal;
}

// Finds a group of the object's cell chunk with room for an instance in *apDistantLODGroup, creating the object's map
// entry and a new group as needed; returns the free instance count.
uint16_t DistantLODShaderProperty::GetDistantLODGroup(
    DistantLODShaderProperty::DistantLODParam* apDistantLODParam,
    uint32_t aiCellChunk,
    DistantLODGroup** apDistantLODGroup,
    NiNode* apDistantLODNode
) {
    // With a group width above one, the chunk's cell coordinates (high and low half) are moved to their group's.
    int32_t iWidth = iDistantLODGroupWidth.uValue.i;
    uint32_t uiCellChunk;
    if (iWidth > 1) {
        int32_t iHalf = iWidth / 2;
        int16_t sX = static_cast<int16_t>(aiCellChunk >> 16);
        int16_t sY = static_cast<int16_t>(aiCellChunk);
        uiCellChunk = static_cast<uint32_t>(static_cast<uint16_t>((aiCellChunk >> 16) + iHalf - sX % iWidth)) << 16 |
            static_cast<uint16_t>(aiCellChunk + iHalf - sY % iWidth);
    }
    else {
        uiCellChunk = iWidth;
    }
    *apDistantLODGroup = nullptr;
    CachedGeometry* pCachedGeom = GetCachedGeometry(apDistantLODParam);
    if (!pCachedGeom) {
        return 0;
    }
    CachedDistantLOD* pCachedLOD = nullptr;
    if (!DistantLODShaderProperty::GetDistantLODMap()->GetAt(apDistantLODParam->iBaseObjectID, pCachedLOD)) {
        pCachedLOD = new CachedDistantLOD;
        pCachedLOD->spCachedGeometry.SetNew(pCachedGeom);
        pCachedLOD->kDistantLODParam = *apDistantLODParam;
        DistantLODShaderProperty::GetDistantLODMap()->SetAt(apDistantLODParam->iBaseObjectID, pCachedLOD);
    }
    NiTPointerList<DistantLODGroup*>* pGroupList = &pCachedLOD->pGroupList;
    if (!*apDistantLODGroup) {
        // The first group of the same cell chunk with room left.
        NiTListItem<DistantLODGroup*>* pPos = pGroupList->m_pkHead;
        while (pPos) {
            DistantLODGroup* pGroup = pPos->m_element;
            pPos = pPos->m_pkNext;
            if (pGroup->iMaxInstanceCount != pGroup->iInstanceCount && uiCellChunk == pGroup->iCellChunk) {
                *apDistantLODGroup = pGroup;
                break;
            }
            if (*apDistantLODGroup) {
                break;
            }
        }
    }
    if (!*apDistantLODGroup || (*apDistantLODGroup)->iMaxInstanceCount == (*apDistantLODGroup)->iInstanceCount) {
        *apDistantLODGroup = new DistantLODGroup(pCachedGeom, uiCellChunk, false);
        ++uiLODGeometryCount;
        (*apDistantLODGroup)->SetDistantLODParam(apDistantLODParam);
        pGroupList->AddTail(*apDistantLODGroup);
        pLODTreeParent->AttachChild((*apDistantLODGroup)->spDistantLODGeometry, true);
    }
    return (*apDistantLODGroup)->iMaxInstanceCount - (*apDistantLODGroup)->iInstanceCount;
}

void DistantLODShaderProperty::ClearStaticMaps() {
    ClearDistantLODMap();
    NiTPointerMap<uint32_t, NiPointer<DistantLODShaderProperty::CachedGeometry>>*
        pMap = DistantLODShaderProperty::GetCachedGeometryMap();
    // The map's GetFirstPos: the first non-empty bucket.
    NiTMapItem<uint32_t, NiPointer<DistantLODShaderProperty::CachedGeometry>>* pPos = nullptr;
    uint32_t uiHashSize = pMap->m_uiHashSize;
    for (uint32_t i = 0; i < uiHashSize; ++i) {
        if (pMap->m_ppkHashTable[i]) {
            pPos = pMap->m_ppkHashTable[i];
            break;
        }
    }
    DistantLODShaderProperty::CachedGeometry* pCachedGeom = nullptr;
    NiPointer<DistantLODShaderProperty::CachedGeometry> spCachedGeom;
    if (DistantLODShaderProperty::GetCachedGeometryMap()->m_kAllocator.m_uiCount) {
        if (pPos) {
            do {
                uint32_t uiIgnore;
                DistantLODShaderProperty::GetCachedGeometryMap()->GetNext(pPos, uiIgnore, spCachedGeom);
                pCachedGeom = spCachedGeom.m_pObject;
                if (pCachedGeom) {
                    pCachedGeom->spTriGeomData = nullptr;
                    delete[] pCachedGeom->pInstanceIndex;
                    pCachedGeom->pInstanceIndex = nullptr;
                    pCachedGeom->spBaseTexture = nullptr;
                    pCachedGeom->spAlphaNoBlend = nullptr;
                    pCachedGeom->spAlphaBlend = nullptr;
                    pCachedGeom->spStencil = nullptr;
                }
            }
            while (pPos);
        }
        DistantLODShaderProperty::GetCachedGeometryMap()->RemoveAll();
    }
}

void DistantLODShaderProperty::AddDistantLOD(
    uint32_t aiCellChunk,
    uint32_t aiCellKey,
    NiNode* apDistantLODNode,
    DistantLODShaderProperty::DistantLODParam* apDistantLODParam,
    NiPoint3* apPositions,
    float* afColors,
    uint16_t aiNumPositions
) {
    DistantLODGroup::DistantLODInstanceData lodinstance;
    DistantLODGroup* pGroup;
    uint16_t v7 = aiNumPositions;
    DistantLODShaderProperty::uiLODCount += aiNumPositions;
    int32_t i = 0;
    int32_t v9 = 0;
    if (aiNumPositions) {
        while (true) {
            pGroup = nullptr;
            uint16_t ilen = DistantLODShaderProperty::GetDistantLODGroup(
                apDistantLODParam,
                aiCellChunk,
                &pGroup,
                apDistantLODNode
            );
            if (!pGroup) {
                return;
            }
            int32_t ilen_2 = ilen;
            if (ilen) {
                float* p_z = &apPositions[v9].z;
                do {
                    if (!v7) {
                        break;
                    }
                    lodinstance.fXPosNorm = *(p_z - 2);
                    lodinstance.fYPosNorm = *(p_z - 1);
                    lodinstance.fZPosNorm = *p_z;
                    lodinstance.fScaleColor = afColors[v9];
                    pGroup->Add(&lodinstance, aiCellKey);
                    ++i;
                    --v7;
                    ++v9;
                    p_z += 3;
                }
                while (i < ilen_2);
            }
            pGroup->UpdateGroupBound();
            i = 0;
            if (!v7) {
                break;
            }
        }
    }
    apDistantLODNode->UpdateWorldBound();
    DistantLODShaderProperty::CachedDistantLOD* pCachedLOD = nullptr;
    DistantLODShaderProperty::GetDistantLODMap()->GetAt(apDistantLODParam->iBaseObjectID, pCachedLOD);
    // Searches the cell key list so that the cell key is added once.
    NiTListItem<uint32_t>* pPos = pCachedLOD->pCellKeyList.m_pkHead;
    while (pPos && pPos->m_element != aiCellKey) {
        pPos = pPos->m_pkNext;
    }
    if (!pPos) {
        pCachedLOD->pCellKeyList.AddTail(aiCellKey);
    }
}

NiTPointerMap<uint32_t, DistantLODShaderProperty::CachedDistantLOD*>* DistantLODShaderProperty::GetDistantLODMap() {
    static DistantLODShaderProperty::pmDistantLODMap_t pmDistantLODMap;
    return &pmDistantLODMap;
}

DistantLODShaderProperty::CachedGeometry::CachedGeometry() {}

// The smart pointers release their objects themselves.
DistantLODShaderProperty::CachedGeometry::~CachedGeometry() {}

// The groups are destroyed and freed; the lists and the cached geometry's pointer are emptied by their own
// destructors.
DistantLODShaderProperty::CachedDistantLOD::~CachedDistantLOD() {
    spCachedGeometry = nullptr;
    pCellKeyList.RemoveAll();
    for (NiTListItem<DistantLODGroup*>* pItem = pGroupList.m_pkHead; pItem; pItem = pItem->m_pkNext) {
        delete pItem->m_element;
    }
    pGroupList.RemoveAll();
}
