#include "Fallout Shared/TESLand.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkBvTreeShape.h"
#include "BSHavok/bhkMoppBvTreeShape.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSMultiBoundAABB.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderNoLightingProperty.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/TallGrassShaderProperty.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainChunk.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainNode.h"
#include "Fallout Shared/FaceGen/NiGeometryDataIteratorNiPoint3.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/HavokError.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/NiGeometryDataIteratorNiColorA.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESChildCell.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGrass.h"
#include "Fallout Shared/TESGrassAreaParam.h"
#include "Fallout Shared/TESLandTexture.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSPackedAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBinaryExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBBlock.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <limits>

NiPointer<NiTexturingProperty> TESObjectLAND::spTexturingProperty;
SettingT<INISettingCollection> gs_fLandTextureTilingMult_Landscape("fLandTextureTilingMult:Landscape", 2.0f);
static SettingT<INISettingCollection> bPreventSafetyCheck("bPreventSafetyCheck:Landscape", false);
static SettingT<INISettingCollection> bCurrentCellOnly("bCurrentCellOnly:Landscape", false);
SettingT<INISettingCollection> gs_iMaxGrassTypesPerTexure_Grass("iMaxGrassTypesPerTexure:Grass", 2);
SettingT<INISettingCollection> gs_iGrassDensityEvalSize_Grass("iGrassDensityEvalSize:Grass", 2);
SettingT<INISettingCollection> gs_fTexturePctThreshold_Grass("fTexturePctThreshold:Grass", 0.3f);
SettingT<INISettingCollection> gs_iLandBorder1R_Landscape("iLandBorder1R:Landscape", 255);
SettingT<INISettingCollection> gs_iLandBorder1G_Landscape("iLandBorder1G:Landscape", 255);
SettingT<INISettingCollection> gs_iLandBorder1B_Landscape("iLandBorder1B:Landscape", 0);
SettingT<INISettingCollection> gs_iLandBorder2R_Landscape("iLandBorder2R:Landscape", 0);
SettingT<INISettingCollection> gs_iLandBorder2G_Landscape("iLandBorder2G:Landscape", 0);
SettingT<INISettingCollection> gs_iLandBorder2B_Landscape("iLandBorder2B:Landscape", 0);
SettingT<INISettingCollection>
    gs_sDefaultLandDiffuseTexture_Landscape("sDefaultLandDiffuseTexture:Landscape", "DirtWasteland01.dds");
SettingT<INISettingCollection>
    gs_sDefaultLandNormalTexture_Landscape("sDefaultLandNormalTexture:Landscape", "DirtWasteland01_N.dds");
int32_t TESObjectLAND::iGrassEvalSize = gs_iGrassDensityEvalSize_Grass.Int();

// The m_kBound field is read directly from the model data (the exe's shared getter has another class's offset, which
// differs with 8-byte pointers).
NiBound& NiGeometry::GetModelBound() {
    return this->m_spModelData->m_kBound;
}

// The quadrant byte needs no swap.
void FILE_TEX_DATA::Endian() {
    ENDIAN(iTextureID, false);
    ENDIAN(sIndex, false);
}

void FILE_TEX_VERT_DATA::Endian() {
    ENDIAN(iVertIndex, false);
    ENDIAN(fPercent, false);
}

void OBJ_LAND::Endian() {
    ENDIAN(iFlags, false);
}

void FILE_HEIGHT_DATA::Endian() {
    ENDIAN(fOffset, false);
}

TESObjectLAND::TESObjectLAND()
  : TESForm(), TESChildCell(), pParentCell(nullptr), spQueuedTextures(nullptr), pLoadedData(nullptr) {
    if (!TESObjectLAND::bStaticsDefined) {
        TESObjectLAND::InitializeStatics();
    }
    ourMemset(&Data, 0, sizeof(Data));
    cFormType = 66;
    ++TESObjectLAND::iLANDsinuse;
}

TESObjectLAND::~TESObjectLAND() {
    UnLoadVertices();
    if (!--TESObjectLAND::iLANDsinuse) {
        ourFree(TESObjectLAND::pDefTriListBLOCK);
        delete[] TESObjectLAND::pDefColorBLOCK;
        delete[] TESObjectLAND::pDefNormalBLOCK;
        delete[] TESObjectLAND::pDefTextureBLOCK;
        ourFree(TESObjectLAND::pDefNormalSetBLOCK);
        for (NiPoint3* pVertexBlock : TESObjectLAND::ppDefVertexBLOCK) {
            delete[] pVertexBlock;
        }
        TESObjectLAND::bStaticsDefined = false;
        TESObjectLAND::spTexturingProperty = static_cast<NiTexturingProperty*>(nullptr);
        if (TESObjectLAND::pDefText) {
            delete TESObjectLAND::pDefText;
        }
        TESObjectLAND::pDefText = nullptr;
        if (TESObjectLAND::pDefTextTexSet) {
            delete TESObjectLAND::pDefTextTexSet;
        }
        TESObjectLAND::pDefTextTexSet = nullptr;
    }
}

// The height, normal and colour buffers are the chunk's data, which GetChunkData copies whole (size 0: the chunk's own
// size). The three buffers are zeroed once, at entry, so that an empty or short chunk (malformed data) does not leave
// them uninitialised, as GetChunkData writes only the chunk's bytes.
bool TESObjectLAND::Load(TESFile* pFile) {
    FILE_TEX_DATA addtexdata;
    FILE_TEX_DATA texdata;
    FILE_HEIGHT_DATA kHeightData = {};
    int8_t acNormals[3276] = {};
    uint8_t acColors[3272] = {};
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LAND,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESLand.cpp",
        706
    );
    uint8_t formid = pFile->GetTESForm();
    if (formid == LAND_ID) {
        LoadForm(pFile);
        int32_t ilasttexindex = -1;
        int32_t ilastblock = -1;
        bool bloaddata = false;
        if ((pFile->GetActive() || TESObjectCELL::GetLoadingTempData()) && !GetLandRemapped()) {
            bloaddata = true;
        }
        if (bloaddata) {
            CreateLandscape(nullptr);
        }
        bool badjusttextures = false;
        do {
            CHUNK_ID eChunk = pFile->GetTESChunk();
            if (eChunk > VHGT_ID) {
                if (eChunk == ATXT_ID) {
                    if (bloaddata) {
                        ourMemset(&addtexdata, 0, sizeof(FILE_TEX_DATA));
                        pFile->GetChunkData(&addtexdata, sizeof(FILE_TEX_DATA));
                        if (pFile->QEndian()) {
                            addtexdata.Endian();
                        }
                        if (addtexdata.sIndex > 5u) {
                            int32_t iblock = addtexdata.cBlock;
                            int32_t iindex = addtexdata.sIndex;
                            int32_t iy = GetDataY();
                            Warning(
                                "MASTERFILE: Land (%i, %i) clamping invalid index %i for block %i.",
                                GetDataX(),
                                iy,
                                iindex,
                                iblock
                            );
                            addtexdata.sIndex = 5;
                            badjusttextures = true;
                        }
                        TESLandTexture* ptex = nullptr;
                        if (addtexdata.iTextureID) {
                            TESForm::AddCompileIndex(addtexdata.iTextureID, pFile);
                            TESForm* pForm = TESForm::GetFormByNumericID(addtexdata.iTextureID);
                            ptex = dynamic_cast<TESLandTexture*>(pForm);
                            if (!ptex) {
                                int32_t iblock = addtexdata.cBlock;
                                uint32_t itextureid = addtexdata.iTextureID;
                                int32_t iy = GetDataY();
                                Warning(
                                    "MASTERFILE: Land (%i, %i) unable to find additional texture ID (%08X) for "
                                    "block %i.",
                                    GetDataX(),
                                    iy,
                                    itextureid,
                                    iblock
                                );
                                badjusttextures = true;
                            }
                        }
                        else {
                            ptex = TESObjectLAND::GetDefaultTexture();
                        }
                        this->pLoadedData->pQuadTextureArray[addtexdata.cBlock][addtexdata.sIndex] = ptex;
                        if (ptex) {
                            if (!spQueuedTextures.m_pObject) {
                                QueuedFile* pQueuedFile;
                                pQueuedFile = new QueuedFile(IO_TASK_PRIORITY_VERY_LOW);
                                spQueuedTextures = pQueuedFile;
                            }
                            ptex->QueueTexture(IO_TASK_PRIORITY_VERY_LOW, spQueuedTextures.m_pObject);
                        }
                        ilastblock = addtexdata.cBlock;
                        ilasttexindex = addtexdata.sIndex;
                    }
                }
                else if (eChunk == BTXT_ID) {
                    if (bloaddata) {
                        ourMemset(&texdata, 0, sizeof(FILE_TEX_DATA));
                        pFile->GetChunkData(&texdata, sizeof(FILE_TEX_DATA));
                        if (pFile->QEndian()) {
                            texdata.Endian();
                        }
                        TESForm::AddCompileIndex(texdata.iTextureID, pFile);
                        TESForm* pForm = TESForm::GetFormByNumericID(texdata.iTextureID);
                        this->pLoadedData->pDefQuadTexture[texdata.cBlock] = dynamic_cast<TESLandTexture*>(pForm);
                        if (this->pLoadedData->pDefQuadTexture[texdata.cBlock]) {
                            if (!spQueuedTextures.m_pObject) {
                                QueuedFile* pQueuedFile;
                                pQueuedFile = new QueuedFile(IO_TASK_PRIORITY_VERY_LOW);
                                spQueuedTextures = pQueuedFile;
                            }
                            TESLandTexture* ptex = this->pLoadedData->pDefQuadTexture[texdata.cBlock];
                            ptex->QueueTexture(IO_TASK_PRIORITY_VERY_LOW, spQueuedTextures.m_pObject);
                        }
                        else {
                            int32_t iblock = texdata.cBlock;
                            uint32_t itextureid = texdata.iTextureID;
                            int32_t iy = GetDataY();
                            Warning(
                                "MASTERFILE: Land (%i, %i) unable to find base texture ID (%08X) for block %i.",
                                GetDataX(),
                                iy,
                                itextureid,
                                iblock
                            );
                            badjusttextures = true;
                        }
                    }
                }
                else if (eChunk == VTXT_ID && bloaddata) {
                    if (ilastblock >= 0 && ilasttexindex >= 0) {
                        uint32_t ifilesize = pFile->GetChunkSize();
                        if (ifilesize % 8) {
                            const char* pfilename = pFile->GetFilename();
                            int32_t iy = GetDataY();
                            Warning(
                                "MASTERFILE: Land (%i, %i) found unrecognized vertex texture data in file "
                                "%s.",
                                GetDataX(),
                                iy,
                                pfilename
                            );
                            badjusttextures = true;
                        }
                        else {
                            FILE_TEX_VERT_DATA* pvertdata = static_cast<FILE_TEX_VERT_DATA*>(ourMalloc(ifilesize));
                            pFile->GetChunkData(pvertdata, ifilesize);
                            uint32_t ivertcount = ifilesize >> 3;
                            for (uint32_t i = 0; i < ivertcount; ++i) {
                                if (pFile->QEndian()) {
                                    pvertdata[i].Endian();
                                }
                                if (pvertdata[i].fPercent > 1.0) {
                                    pvertdata[i].fPercent = pvertdata[i].fPercent / 100.0;
                                }
                                SetTexturePercent(
                                    ilastblock,
                                    pvertdata[i].iVertIndex,
                                    ilasttexindex,
                                    pvertdata[i].fPercent
                                );
                            }
                            ourFree(pvertdata);
                            pvertdata = nullptr;
                        }
                    }
                    ilastblock = -1;
                    ilasttexindex = -1;
                }
            }
            else {
                int32_t j;
                int32_t i;
                if (eChunk == VHGT_ID) {
                    if (bloaddata && (Data.iFlags & LAND_DATA_HEIGHTS_NORMALS) != 0) {
                        float afHeights[1089];
                        pFile->GetChunkData(&kHeightData, 0);
                        if (pFile->QEndian()) {
                            kHeightData.Endian();
                        }
                        NiPoint2 kExtents(std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
                        this->pLoadedData->HeightExtents.x = kExtents.x;
                        this->pLoadedData->HeightExtents.y = kExtents.y;
                        float lastheight = kHeightData.fOffset;
                        // Each step adds to the vertex before it in the row; a row's first vertex steps from the first
                        // vertex of the row before.
                        for (i = 0; i < 1089; ++i) {
                            afHeights[i] = static_cast<double>(kHeightData.cHeightData[i]) + lastheight;
                            if ((i + 1) % 33) {
                                lastheight = afHeights[i];
                            }
                            else {
                                lastheight = afHeights[i - 32];
                            }
                        }
                        for (i = 0; i < 4; ++i) {
                            int32_t orig = 16 * (i % 2) + 528 * (i / 2);
                            for (j = 0; j < 289; ++j) {
                                int32_t icolumn = j % 17;
                                int32_t irow = j / 17;
                                int32_t curvert = j % 17 + (orig + 33 * (j / 17));
                                float fvertheight = static_cast<float>(8 * FloatToInt(afHeights[curvert]));
                                this->pLoadedData->ppVertices[i][j].x = static_cast<double>(icolumn << 7) +
                                    TESObjectLAND::pXOffsetBLOCK[i];
                                this->pLoadedData->ppVertices[i][j].y = static_cast<double>(irow << 7) +
                                    TESObjectLAND::pYOffsetBLOCK[i];
                                this->pLoadedData->ppVertices[i][j].z = fvertheight;
                                if (this->pLoadedData->HeightExtents.x > static_cast<double>(fvertheight)) {
                                    this->pLoadedData->HeightExtents.x = fvertheight;
                                }
                                else if (this->pLoadedData->HeightExtents.y < static_cast<double>(fvertheight)) {
                                    this->pLoadedData->HeightExtents.y = fvertheight;
                                }
                            }
                        }
                        this->pLoadedData->fBaseHeight = (this->pLoadedData->HeightExtents.x +
                                                          this->pLoadedData->HeightExtents.y) *
                            0.5;
                        for (i = 0; i < 4; ++i) {
                            for (j = 0; j < 289; ++j) {
                                this->pLoadedData->ppVertices[i][j].z = this->pLoadedData->ppVertices[i][j].z -
                                    this->pLoadedData->fBaseHeight;
                            }
                        }
                    }
                }
                else if (eChunk > VNML_ID) {
                    if (eChunk == VCLR_ID && bloaddata && (Data.iFlags & LAND_DATA_COLORS) != 0) {
                        pFile->GetChunkData(acColors, 0);
                        for (i = 0; i < 4; ++i) {
                            int32_t orig = 16 * (i % 2) + 528 * (i / 2);
                            for (j = 0; j < 289; ++j) {
                                int32_t curvert = j % 17 + (orig + 33 * (j / 17));
                                float fB = static_cast<double>(acColors[3 * curvert + 2]) / 255.0;
                                float fG = static_cast<double>(acColors[3 * curvert + 1]) / 255.0;
                                float fR = static_cast<double>(acColors[3 * curvert]) / 255.0;
                                NiColorA kColor(fR, fG, fB, 1.0f);
                                NiColorA* pColor = &this->pLoadedData->ppColorsA[i][j];
                                pColor->r = kColor.r;
                                pColor->g = kColor.g;
                                pColor->b = kColor.b;
                                pColor->a = kColor.a;
                            }
                        }
                    }
                }
                else if (eChunk == VNML_ID) {
                    if (bloaddata && (Data.iFlags & LAND_DATA_HEIGHTS_NORMALS) != 0) {
                        pFile->GetChunkData(acNormals, 0);
                        for (i = 0; i < 4; ++i) {
                            int32_t orig = 16 * (i % 2) + 528 * (i / 2);
                            for (j = 0; j < 289; ++j) {
                                int32_t curvert = j % 17 + (orig + 33 * (j / 17));
                                this->pLoadedData->ppNormals[i][j].x = static_cast<double>(acNormals[3 * curvert]) /
                                    127.0;
                                this->pLoadedData->ppNormals[i][j].y = static_cast<double>(acNormals[3 * curvert + 1]) /
                                    127.0;
                                this->pLoadedData->ppNormals[i][j].z = static_cast<double>(acNormals[3 * curvert + 2]) /
                                    127.0;
                                this->pLoadedData->ppNormals[i][j].Unitize();
                            }
                        }
                    }
                }
                else if (eChunk == DATA_ID) {
                    bool bremapped = GetLandRemapped();
                    pFile->GetChunkData(&Data, sizeof(OBJ_LAND));
                    if (pFile->QEndian()) {
                        Data.Endian();
                    }
                    MarkLoaded(false);
                    UpdateUseParentWorldspace(bremapped);
                }
                else if (eChunk == MPCD_ID && bloaddata) {
                    uint32_t ibuffersize = pFile->GetChunkSize();
                    char* pbuffer = static_cast<char*>(ourMalloc(ibuffersize));
                    pFile->GetChunkData(pbuffer, ibuffersize);
                    if (this->pLoadedData->pMoppCode) {
                        this->pLoadedData->pMoppCode->removeReference();
                        this->pLoadedData->pMoppCode = nullptr;
                    }
                    ReadMoppCodeFromBuffer(pbuffer, ibuffersize, &this->pLoadedData->pMoppCode);
                    if (this->pLoadedData->pMoppCode) {
                        this->pLoadedData->pMoppCode->addReference();
                        SetHasMopp(1);
                    }
                    ourFree(pbuffer);
                }
            }
        }
        while (pFile->NextChunk());
        if (badjusttextures) {
            AdjustTextureArrays();
        }
        MarkLoaded(bloaddata);
        return true;
    }

    return false;
}

// The chunks include padding bytes that are never written from the locals: VHGT's 0x448 bytes end in FILE_HEIGHT_DATA's
// 3 padding bytes, BTXT and ATXT include FILE_TEX_DATA's padding byte at offset 5, and VTXT includes each
// FILE_TEX_VERT_DATA's padding at offsets 2..3.
void TESObjectLAND::Save() {
    FILE_TEX_VERT_DATA atexverts[289];
    FILE_TEX_DATA addtexdata;
    FILE_TEX_DATA texdata;
    if (IsLandAltered() &&
        ((Data.iFlags & LAND_DATA_LOADED) != 0 ||
         (this->pLoadedData &&
          this->pLoadedData->ppVertices &&
          this->pLoadedData->ppNormals &&
          this->pLoadedData->ppColorsA))) {
        char acColors[3272];
        int32_t i;
        if (!HasGoodNormals()) {
            ForceGoodNormals();
        }
        StartForm();
        if (TESForm::QEndianSwapOnSave()) {
            Data.Endian();
        }
        TESForm::__AddChunkData(DATA_ID, &Data, 4u);
        if (TESForm::QEndianSwapOnSave()) {
            Data.Endian();
        }
        if ((Data.iFlags & LAND_DATA_HEIGHTS_NORMALS) != 0) {
            int32_t aiHeights[1090];
            char acNormals[3276];
            FILE_HEIGHT_DATA hgts;
            for (i = 0; i < 4; ++i) {
                int32_t orig = 16 * (i % 2) + 528 * (i / 2);
                for (int32_t j = 0; j < 289; ++j) {
                    int32_t curvert = j % 17 + (orig + 33 * (j / 17));
                    int32_t skipvert = 0;
                    if (j / 17 == 16) {
                        skipvert = i / 2 != 1;
                    }
                    if (j % 17 == 16) {
                        skipvert = i % 2 != 1;
                    }
                    if (!skipvert) {
                        aiHeights[curvert] = FloatToInt(this->pLoadedData->ppVertices[i][j].z) >> 3;
                        this->pLoadedData->ppNormals[i][j].Unitize();
                        acNormals[3 * curvert] = static_cast<int32_t>(this->pLoadedData->ppNormals[i][j].x * 127.0);
                        acNormals[3 * curvert + 1] = static_cast<int32_t>(this->pLoadedData->ppNormals[i][j].y * 127.0);
                        acNormals[3 * curvert + 2] = static_cast<int32_t>(this->pLoadedData->ppNormals[i][j].z * 127.0);
                    }
                }
            }
            TESForm::AddChunkArray(VNML_ID, acNormals, 3267u);
            hgts.fOffset = static_cast<float>(aiHeights[0]);
            int32_t lastheight = aiHeights[0];
            char berror = 0;
            for (i = 0; i < 1089; ++i) {
                if (std::abs(aiHeights[i] - lastheight) < 128) {
                    hgts.cHeightData[i] = aiHeights[i] - lastheight;
                }
                else {
                    berror = 1;
                    if (aiHeights[i] <= lastheight) {
                        hgts.cHeightData[i] = -127;
                    }
                    else {
                        hgts.cHeightData[i] = 127;
                    }
                }
                if ((i + 1) % 33) {
                    if (berror) {
                        lastheight += hgts.cHeightData[i];
                    }
                    else {
                        lastheight = aiHeights[i];
                    }
                }
                else {
                    // The first height of the row of 33 just finished.
                    lastheight = aiHeights[i - 32];
                }
            }
            if (TESForm::QEndianSwapOnSave()) {
                hgts.Endian();
            }
            TESForm::__AddChunkData(VHGT_ID, &hgts, 1096u);
            if (TESForm::QEndianSwapOnSave()) {
                hgts.Endian();
            }
            if (berror) {
                int32_t iy = GetDataY();
                Warning(
                    "MASTERFILE: Error saving land height Data for cell (%i, %i). Error correction attempted.\r\n",
                    GetDataX(),
                    iy
                );
            }
        }
        if ((Data.iFlags & LAND_DATA_COLORS) != 0) {
            for (i = 0; i < 4; ++i) {
                int32_t orig = 16 * (i % 2) + 528 * (i / 2);
                for (int32_t k = 0; k < 289; ++k) {
                    int32_t curvert = k % 17 + (orig + 33 * (k / 17));
                    int32_t skipvert = 0;
                    if (k / 17 == 16) {
                        skipvert = i / 2 != 1;
                    }
                    if (k % 17 == 16) {
                        skipvert = i % 2 != 1;
                    }
                    if (!skipvert) {
                        acColors[3 * curvert] = static_cast<int32_t>(this->pLoadedData->ppColorsA[i][k].r * 255.0);
                        acColors[3 * curvert + 1] = static_cast<int32_t>(this->pLoadedData->ppColorsA[i][k].g * 255.0);
                        acColors[3 * curvert + 2] = static_cast<int32_t>(this->pLoadedData->ppColorsA[i][k].b * 255.0);
                    }
                }
            }
            TESForm::AddChunkArray(VCLR_ID, acColors, 3267u);
        }
        if ((Data.iFlags & LAND_DATA_TEXTURES) != 0) {
            AdjustTextureArrays();
            for (int32_t iblock = 0; iblock < 4; ++iblock) {
                if (this->pLoadedData->pDefQuadTexture[iblock]) {
                    TESObjectLAND::LoadedLandData* pLoadedData = this->pLoadedData;
                    if (pLoadedData->pDefQuadTexture[iblock] != TESObjectLAND::GetDefaultTexture()) {
                        texdata.cBlock = iblock;
                        texdata.sIndex = -1;
                        texdata.iTextureID = this->pLoadedData->pDefQuadTexture[iblock]->iFormID;
                        if (TESForm::QEndianSwapOnSave()) {
                            texdata.Endian();
                        }
                        TESForm::__AddChunkData(BTXT_ID, &texdata, 8u);
                        if (TESForm::QEndianSwapOnSave()) {
                            texdata.Endian();
                        }
                    }
                }
                if (this->pLoadedData->pQuadTextureArray[iblock]) {
                    int32_t itexsize = 6;
                    for (int32_t itex = 0; itex < itexsize; ++itex) {
                        TESLandTexture* ptex = this->pLoadedData->pQuadTextureArray[iblock][itex];
                        if (ptex) {
                            addtexdata.cBlock = iblock;
                            addtexdata.sIndex = itex;
                            addtexdata.iTextureID = ptex == TESObjectLAND::GetDefaultTexture() ? 0 : ptex->iFormID;
                            if (TESForm::QEndianSwapOnSave()) {
                                addtexdata.Endian();
                            }
                            TESForm::__AddChunkData(ATXT_ID, &addtexdata, 8u);
                            if (TESForm::QEndianSwapOnSave()) {
                                addtexdata.Endian();
                            }
                            int32_t itexverts = 0;
                            for (int32_t ivert = 0; ivert < 289; ++ivert) {
                                if (this->pLoadedData->ppPercentArrays[iblock]) {
                                    float ftexpercent = GetTexturePercent(iblock, ivert, itex);
                                    if (ftexpercent > 0.0) {
                                        atexverts[itexverts].fPercent = ftexpercent;
                                        atexverts[itexverts++].iVertIndex = ivert;
                                    }
                                }
                            }
                            if (itexverts) {
                                if (TESForm::QEndianSwapOnSave()) {
                                    for (uint32_t ivert = 0; ivert < itexverts; ++ivert) {
                                        atexverts[ivert].Endian();
                                    }
                                }
                                TESForm::__AddChunkData(VTXT_ID, atexverts, 8 * itexverts);
                                if (TESForm::QEndianSwapOnSave()) {
                                    for (uint32_t ivert = 0; ivert < itexverts; ++ivert) {
                                        atexverts[ivert].Endian();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        CloseForm();
        TESForm::CompressSaveBuffer();
    }
}

bool TESObjectLAND::SavesBefore(TESForm* apForm) {
    bool bretval = false;
    uint8_t uiformtype = apForm->GetFormType();
    if (TESObjectCELL::IsFormTypeCellChild(uiformtype)) {
        TESChildCell* pcellchild = dynamic_cast<TESChildCell*>(apForm);
        TESObjectCELL* pformcell = pcellchild->GetSaveParentCell();
        if (pformcell == GetSaveParentCell()) {
            switch (apForm->GetFormType()) {
                case REFR_ID:
                case ACHR_ID:
                case ACRE_ID:
                case PMIS_ID:
                case PGRE_ID:
                case PBEA_ID:
                case PFLA_ID:
                case PCBE_ID: {
                    TESObjectREFR* pref = dynamic_cast<TESObjectREFR*>(apForm);
                    bretval = pref && !pref->GetRefPersists();
                    break;
                }
                case NAVM_ID: {
                    bretval = true;
                    break;
                }
                default: {
                    break;
                }
            }
        }
        else {
            return GetSaveParentCell()->SavesBefore(pformcell);
        }
    }
    else {
        return GetSaveParentCell()->SavesBefore(apForm);
    }
    return bretval;
}

bool TESObjectLAND::SavesBefore(FORM* apGroupFORM) {
    bool bretval = false;
    if (apGroupFORM && apGroupFORM->form == formEnumString[2].uiFormString) {
        uint32_t iFormID = apGroupFORM->iFormID;
        if (iFormID < 8 || iFormID > 9) {
            return pParentCell->SavesBefore(apGroupFORM);
        }

        TESForm* pForm = TESForm::GetFormByNumericID(apGroupFORM->flags);
        TESObjectCELL* pcell = dynamic_cast<TESObjectCELL*>(pForm);
        if (pcell && pParentCell == pcell) {
            return false;
        }
    }

    return bretval;
}

TESForm* TESObjectLAND::CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) {
    TESForm* duplicateForm = TESForm::CreateDuplicateForm(false, apCopyMap);
    return dynamic_cast<TESObjectLAND*>(duplicateForm);
}

// Altering the land also marks its parent cell altered.
[[clang::disable_tail_calls]] void TESObjectLAND::SetAltered(bool abVal) {
    TESForm::SetAltered(abVal);
    if (abVal && pParentCell) {
        pParentCell->SetAltered(true);
    }
}

void TESObjectLAND::Copy(TESForm* apCopy) {
    TESObjectLAND* pcopyland = dynamic_cast<TESObjectLAND*>(apCopy);
    if (pcopyland) {
        bool bcopyloaded = pcopyland->IsLoaded();
        if (!bcopyloaded) {
            pcopyland->LoadVertices(false);
        }
        pParentCell = pcopyland->pParentCell;
        CreateLandscape(pcopyland);
        for (int32_t i = 0; i < 4; ++i) {
            ourMemcpy(pLoadedData->ppVertices[i], pcopyland->pLoadedData->ppVertices[i], sizeof(NiPoint3) * 289);
            ourMemcpy(pLoadedData->ppNormals[i], pcopyland->pLoadedData->ppNormals[i], sizeof(NiPoint3) * 289);
            ourMemcpy(pLoadedData->ppColorsA[i], pcopyland->pLoadedData->ppColorsA[i], sizeof(NiColorA) * 289);
            ourMemcpy(pLoadedData->ppNormalsSet[i], pcopyland->pLoadedData->ppNormalsSet[i], sizeof(bool) * 289);
            ourMemcpy(
                pLoadedData->ppPercentArrays[i][0],
                pcopyland->pLoadedData->ppPercentArrays[i][0],
                sizeof(float) * 8 * 289
            );
            ourMemcpy(
                pLoadedData->pQuadTextureArray[i],
                pcopyland->pLoadedData->pQuadTextureArray[i],
                sizeof(TESLandTexture*) * 6
            );
            pLoadedData->pDefQuadTexture[i] = pcopyland->pLoadedData->pDefQuadTexture[i];
        }
        ourMemcpy(&Data, &pcopyland->Data, sizeof(Data));
        ourMemcpy(
            &pLoadedData->HeightExtents,
            &pcopyland->pLoadedData->HeightExtents,
            sizeof(pLoadedData->HeightExtents)
        );
        if (!bcopyloaded) {
            pcopyland->UnLoadVertices();
        }
        MarkLoaded(true);
    }
}

bool TESObjectLAND::BelongsInGroup(FORM* apGroupFORM, bool abAllowParentGroups, bool abCurrentOnly) {
    bool bFalsr = false;
    if (apGroupFORM && apGroupFORM->form == formEnumString[2].uiFormString) {
        TESForm* pParentCell = GetParentCell();
        uint32_t iFormID = apGroupFORM->iFormID;
        if (iFormID == 6) {
            if (!abAllowParentGroups) {
                return false;
            }
        }
        else if (iFormID <= 7 || iFormID > 9) {
            if (abAllowParentGroups) {
                return pParentCell->BelongsInGroup(apGroupFORM, abAllowParentGroups, abCurrentOnly);
            }
            return bFalsr;
        }
        if (pParentCell->FormIDMatchesWithoutIndex(apGroupFORM->flags)) {
            return apGroupFORM->iFormID != 8;
        }
    }
    return bFalsr;
}

void TESObjectLAND::CreateGroupData(FORM* apOutGroupFORM, FORM_GROUP* apParentGroup) {
    if (apOutGroupFORM) {
        apOutGroupFORM->form = 0;
        if (apParentGroup) {
            TESObjectCELL* pParentCell = this->pParentCell;
            uint32_t uiParentCellFormID = this->pParentCell->iFormID;
            uint32_t iFormID = apParentGroup->GroupData.iFormID;
            if (iFormID == 3 || iFormID == 5) {
                if (apParentGroup->GroupData.flags == pParentCell->GetGroupSubBlockKey()) {
                    apOutGroupFORM->form = formEnumString[2].uiFormString;
                    apOutGroupFORM->iFormID = 6;
                    apOutGroupFORM->flags = uiParentCellFormID;
                    apOutGroupFORM->length = 0;
                    apOutGroupFORM->iVersionControl = 0;
                }
            }
            else if (iFormID == 6 && apParentGroup->GroupData.flags == uiParentCellFormID) {
                apOutGroupFORM->form = formEnumString[2].uiFormString;
                apOutGroupFORM->iFormID = 9;
                apOutGroupFORM->flags = this->pParentCell->iFormID;
                apOutGroupFORM->length = 0;
                apOutGroupFORM->iVersionControl = 0;
            }
        }
    }
}

TESObjectCELL* TESObjectLAND::GetSaveParentCell() {
    return pParentCell;
}

void TESObjectLAND::Attach3D() {
    if (pParentCell && pParentCell->Get3D()) {
        if (!pLoadedData->ppMesh) {
            NiTriStrips* pquadstrips[4];
            bool pbculled[4];
            pLoadedData->ppMesh = static_cast<NiNode**>(ourMalloc(4 * sizeof(NiNode*)));
            ourMemset(pbculled, 0, 4u);
            ourMemset(pquadstrips, 0, sizeof(pquadstrips));
            NiPoint2 landminmax(std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
            for (int32_t iblock = 0; iblock < 4; ++iblock) {
                pLoadedData->ppMesh[iblock] = GetQuadLandNode(iblock);
                if (pLoadedData->ppMesh[iblock]) {
                    pbculled[iblock] = pLoadedData->ppMesh[iblock]->GetAppCulled();
                    pLoadedData->ppMesh[iblock]->SetAppCulled(true);
                    pLoadedData->ppMesh[iblock]->AttachProperty(TESObjectLAND::spTexturingProperty.m_pObject);
                    NiTriStrips* ptrigeom = MakeLandTriStrips(iblock);
                    pquadstrips[iblock] = ptrigeom;
                    pLoadedData->ppMesh[iblock]->SetAt(0, ptrigeom);
                    uint16_t usvertcount = ptrigeom->GetVertexCount();
                    NiPoint3* pverts = ptrigeom->GetVertices();
                    ptrigeom->GetModelBound().ComputeFromData(usvertcount, pverts);
                    ptrigeom->GetModelData()->MarkAsChanged(15);
                    BSMultiBoundNode* pbsmbnode = static_cast<BSMultiBoundNode*>(pLoadedData->ppMesh[iblock]);
                    BSMultiBoundAABB* pbmbaabb = static_cast<BSMultiBoundAABB*>(pbsmbnode->QMultiBound()->QShape());
                    NiPoint2 kminmax = GetMinMaxBlockHeight(iblock);
                    float fhalfheight = (kminmax.y - kminmax.x) / 2.0;
                    pbmbaabb->SetHalfExtents(NiPoint3(1024.0f, 1024.0f, fhalfheight));
                    NiPoint3 kcenter = NiPoint3::ZERO;
                    kcenter.x = GetWorldCellX() + 1024.0 + static_cast<double>(iblock % 2 << 11);
                    kcenter.y = GetWorldCellY() + 1024.0 + static_cast<double>(iblock / 2 << 11);
                    kcenter.z = (kminmax.x + kminmax.y) / 2.0;
                    pbmbaabb->SetCenter(kcenter);
                    if (kminmax.x < static_cast<double>(landminmax.x)) {
                        landminmax.x = kminmax.x;
                    }
                    if (kminmax.y > static_cast<double>(landminmax.y)) {
                        landminmax.y = kminmax.y;
                    }
                }
            }
            CellMopp* pcm = pParentCell->GetCellMopp();
            pcm->Kill();
            hkMoppBvTreeShapeCinfo cinfo;
            cinfo.m_code = pLoadedData->pMoppCode;
            pcm->Create(pquadstrips, 4u, pLoadedData->fBaseHeight);
            CalculateGrassParams();
            MergeSameFileTextures();
            InitLandscape();
            for (int32_t iblock = 0; iblock < 4; ++iblock) {
                if (pquadstrips[iblock]) {
                    NiGeometryData* pgeodata = pquadstrips[iblock]->GetModelData();
                    pgeodata->SetConsistency(NiGeometryData::STATIC);
                    pgeodata->SetCompressFlags(NiGeometryData::COMPRESS_NORM);
                    pgeodata->m_ucKeepFlags = 23u;
                    BSShader* pshade = pquadstrips[iblock]->GetShader();
                    ShadowLightShader* pslshader = nullptr;
                    // Shader types 1 to 5 are the ShadowLightShader family; the type is read as signed.
                    if (pshade && pshade->GetShaderType() >= 1 && pshade->GetShaderType() <= 5) {
                        pslshader = static_cast<ShadowLightShader*>(pshade);
                    }
                    if (pslshader) {
                        NiDX9Renderer* prend = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                        NiD3DShaderDeclaration* pdecl = pslshader->GetLandShaderDecl();
                        prend->PrecacheGeometry(pquadstrips[iblock], 0, 0, pdecl);
                        prend->PerformPrecache();
                    }
                }
                if (pLoadedData->ppMesh[iblock]) {
                    pLoadedData->ppMesh[iblock]->SetAppCulled(pbculled[iblock]);
                    NiUpdateData UpdateData(0.0f, false, false);
                    pLoadedData->ppMesh[iblock]->Update(UpdateData);
                    pLoadedData->ppMesh[iblock]->UpdateProperties();
                }
            }
        }
        if (pLoadedData->ppMesh && pParentCell) {
            bool bforce = GetLandRemapped() && !pParentCell->TESObjectCELL::HasReferences();
            for (int32_t iblock = 0; iblock < 4; ++iblock) {
                if (bforce || pParentCell->TESObjectCELL::GetHideLand(static_cast<uint32_t>(iblock))) {
                    pLoadedData->ppMesh[iblock]->SetAppCulled(true);
                }
                else {
                    pLoadedData->ppMesh[iblock]->SetAppCulled(false);
                }
            }
        }
    }
}

bool TESObjectLAND::LoadVertices(bool abLoad3D) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LAND,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESLand.cpp",
        1109
    );
    if (Interface::GetLoadingMenuVisible()) {
        abLoad3D = false;
    }
    if (IsLoaded()) {
        if (!pLoadedData->ppMesh && abLoad3D) {
            Attach3D();
        }
        return true;
    }
    if (!GetLandRemapped() &&
        (((Data.iFlags & LAND_DATA_HEIGHTS_NORMALS) == 0 &&
          (Data.iFlags & LAND_DATA_COLORS) == 0 &&
          (Data.iFlags & LAND_DATA_TEXTURES) == 0) ||
         !GetFile(-1))) {
        CreateLandscape(nullptr);
        if (abLoad3D) {
            Attach3D();
        }
        return true;
    }
    CreateLandscape(nullptr);
    bool success = LoadVerticesIntoArrays();
    MarkLoaded(true);
    if (abLoad3D) {
        Attach3D();
    }
    return success;
}

// Frees the loaded data's per-quadrant arrays and grass maps, then the data itself through its deleting destructor.
bool TESObjectLAND::UnLoadVertices() {
    NiTMapItem<uint32_t, TESGrassAreaParam**>* pPos;
    MarkLoaded(false);
    Detach3D();
    if (pLoadedData) {
        TESGrassAreaParam** pgrassParam = nullptr;
        for (int32_t i = 0; i < 4; ++i) {
            if (pLoadedData->ppPercentArrays[i]) {
                ourFree(*pLoadedData->ppPercentArrays[i]);
                ourFree(pLoadedData->ppPercentArrays[i]);
                pLoadedData->ppPercentArrays[i] = nullptr;
            }
            if (pLoadedData->pQuadTextureArray[i]) {
                ourFree(pLoadedData->pQuadTextureArray[i]);
                pLoadedData->pQuadTextureArray[i] = nullptr;
            }
            if (pLoadedData->ppVertices) {
                delete[] pLoadedData->ppVertices[i];
                pLoadedData->ppVertices[i] = nullptr;
            }
            if (pLoadedData->ppNormals) {
                delete[] pLoadedData->ppNormals[i];
                pLoadedData->ppNormals[i] = nullptr;
            }
            if (pLoadedData->ppColorsA) {
                delete[] pLoadedData->ppColorsA[i];
                pLoadedData->ppColorsA[i] = nullptr;
            }
            if (pLoadedData->ppNormalsSet) {
                ourFree(pLoadedData->ppNormalsSet[i]);
                pLoadedData->ppNormalsSet[i] = nullptr;
            }
            if (pLoadedData->pmGrassMap[i].m_kAllocator.m_uiCount) {
                pPos = pLoadedData->pmGrassMap[i].GetFirstPos();
                while (pPos) {
                    uint32_t ikey;
                    pLoadedData->pmGrassMap[i].GetNext(pPos, ikey, pgrassParam);
                    if (pgrassParam) {
                        for (int32_t k = 0; k < 16; ++k) {
                            if (pgrassParam[k]) {
                                pgrassParam[k]->kGrassParam.pcGeomFilename = nullptr;
                                ourFree(pgrassParam[k]);
                            }
                        }
                        ourFree(pgrassParam);
                    }
                }
            }
            pLoadedData->pmGrassMap[i].RemoveAll();
        }
        delete[] pLoadedData->ppNormalsSet;
        delete[] pLoadedData->ppVertices;
        delete[] pLoadedData->ppColorsA;
        delete[] pLoadedData->ppNormals;
        if (pLoadedData->pMoppCode) {
            pLoadedData->pMoppCode->removeReference();
            pLoadedData->pMoppCode = nullptr;
        }
    }
    if (pLoadedData) {
        delete pLoadedData;
    }
    pLoadedData = nullptr;
    return true;
}

// Each block holds 17x17 = 289 vertices, 6 quad textures and 8 texture-percent floats per vertex.
int8_t TESObjectLAND::CreateLandscape(TESObjectLAND* apFrom) {
    TESObjectLAND::LoadedLandData* pData;
    if (pLoadedData && pLoadedData->ppVertices) {
        return 0;
    }
    pData = new TESObjectLAND::LoadedLandData;
    pLoadedData = pData;
    pLoadedData->iCellX = pParentCell->GetDataX();
    pLoadedData->iCellY = pParentCell->GetDataY();
    pLoadedData->ppVertices = new NiPoint3*[4];
    pLoadedData->ppColorsA = new NiColorA*[4];
    pLoadedData->ppNormals = new NiPoint3*[4];
    pLoadedData->ppNormalsSet = new bool*[4];
    float fdefheight = GetDefaultWorldHeight();
    for (int32_t iblock = 0; iblock < 4; ++iblock) {
        pLoadedData->ppVertices[iblock] = new NiPoint3[289];
        pLoadedData->ppColorsA[iblock] = new NiColorA[289];
        pLoadedData->ppNormals[iblock] = new NiPoint3[289];
        pLoadedData->ppNormalsSet[iblock] = static_cast<bool*>(ourMalloc(289 * sizeof(bool)));
        pLoadedData->ppPercentArrays[iblock] = static_cast<float**>(ourMalloc(289 * sizeof(float*)));
        pLoadedData->pDefQuadTexture[iblock] = TESObjectLAND::GetDefaultTexture();
        pLoadedData->pQuadTextureArray[iblock] = static_cast<TESLandTexture**>(ourMalloc(6 * sizeof(TESLandTexture*)));
        if (!apFrom || !apFrom->pLoadedData) {
            ourMemcpy(pLoadedData->ppVertices[iblock], TESObjectLAND::ppDefVertexBLOCK[iblock], 289 * sizeof(NiPoint3));
            ourMemcpy(pLoadedData->ppColorsA[iblock], TESObjectLAND::pDefColorBLOCK, 289 * sizeof(NiColorA));
            ourMemcpy(pLoadedData->ppNormals[iblock], TESObjectLAND::pDefNormalBLOCK, 289 * sizeof(NiPoint3));
            ourMemcpy(pLoadedData->ppNormalsSet[iblock], TESObjectLAND::pDefNormalSetBLOCK, 289 * sizeof(bool));
            if (-2048.0f != fdefheight) {
                for (int32_t ivert = 0; ivert < 289; ++ivert) {
                    pLoadedData->ppVertices[iblock][ivert].z = fdefheight;
                }
            }
            float* ppercentArrays = static_cast<float*>(ourMalloc(289 * 8 * sizeof(float)));
            ourMemset(ppercentArrays, 0, 289 * 8 * sizeof(float));
            for (int32_t ivert = 0; ivert < 289; ++ivert) {
                pLoadedData->ppPercentArrays[iblock][ivert] = &ppercentArrays[8 * ivert];
            }
            ourMemset(pLoadedData->pQuadTextureArray[iblock], 0, 6 * sizeof(TESLandTexture*));
        }
    }
    return 1;
}

void TESObjectLAND::DrawLandBorder(bool bShow) {
    int32_t lineidx = 0;
    if (HasVertices()) {
        NiNode* pNode = pParentCell->Get3D();
        if (pNode) {
            if (pLoadedData->spBorder.m_pObject) {
                pNode->DetachChild(pLoadedData->spBorder.m_pObject);
                pLoadedData->spBorder = static_cast<NiLines*>(nullptr);
            }
            if (bShow) {
                NiPoint3* lineverts = new NiPoint3[128];
                NiColorA* lineclr = new NiColorA[128];
                uint8_t* lineflags = static_cast<uint8_t*>(ourMalloc(128u));
                float fworldy = GetWorldCellY();
                float fworldx = GetWorldCellX();
                NiPoint3 worldshift(fworldx, fworldy, 0.0f);
                for (int32_t ix = 0; ix < 33; ++ix) {
                    for (int32_t iy = 0; iy < 33; ++iy) {
                        if (!ix || !iy || ix == 32 || iy == 32) {
                            NiColorA kOddColor;
                            NiColorA kEvenColor;
                            NiColorA* pColor;
                            float tmphgt;
                            if (iy) {
                                if (ix == 32) {
                                    lineidx = iy + 32;
                                }
                                else if (iy == 32) {
                                    lineidx = 32 - ix + 64;
                                }
                                else if (!ix) {
                                    lineidx = 32 - iy + 96;
                                }
                            }
                            else {
                                lineidx = ix;
                            }
                            if (lineidx % 2) {
                                float fB = static_cast<double>(gs_iLandBorder1B_Landscape.Int()) / 255.0;
                                float fG = static_cast<double>(gs_iLandBorder1G_Landscape.Int()) / 255.0;
                                float fR = static_cast<double>(gs_iLandBorder1R_Landscape.Int()) / 255.0;
                                kOddColor = NiColorA(fR, fG, fB, 1.0f);
                                pColor = &kOddColor;
                            }
                            else {
                                float fB = static_cast<double>(gs_iLandBorder2B_Landscape.Int()) / 255.0;
                                float fG = static_cast<double>(gs_iLandBorder2G_Landscape.Int()) / 255.0;
                                float fR = static_cast<double>(gs_iLandBorder2R_Landscape.Int()) / 255.0;
                                kEvenColor = NiColorA(fR, fG, fB, 1.0f);
                                pColor = &kEvenColor;
                            }
                            NiColorA* pLineColor = &lineclr[lineidx];
                            pLineColor->r = pColor->r;
                            pLineColor->g = pColor->g;
                            pLineColor->b = pColor->b;
                            pLineColor->a = pColor->a;
                            lineflags[lineidx] = 1;
                            float fy = static_cast<double>(iy) * 128.0;
                            float fx = static_cast<double>(ix) * 128.0;
                            NiPoint3 tmppt = worldshift + NiPoint3(fx, fy, 0.0f);
                            GetLandHeight(tmppt, tmphgt);
                            tmppt.z = tmphgt + 5.0;
                            lineverts[lineidx] = tmppt - worldshift;
                        }
                    }
                }
                NiLines* pBorder = new NiLines(
                    128,
                    lineverts,
                    lineclr,
                    nullptr,
                    0,
                    NiGeometryData::NBT_METHOD_NONE,
                    lineflags
                );
                pLoadedData->spBorder = pBorder;
                pLoadedData->spBorder->SetTranslate(worldshift);
                NiTexturingProperty* deftext = new NiTexturingProperty;
                deftext->SetBaseMap(nullptr);
                deftext->SetDecalMap(0, nullptr);
                pLoadedData->spBorder->AttachProperty(deftext);
                BSShaderNoLightingProperty* pshprop = new BSShaderNoLightingProperty;
                pshprop->SetShader(33);
                pLoadedData->spBorder->AttachProperty(pshprop);
                BSShaderManager::PrepareObject(pLoadedData->spBorder.m_pObject, 0, 0);
                pNode->AttachChild(pLoadedData->spBorder.m_pObject, 1);
                pLoadedData->spBorder->UpdateProperties();
                NiUpdateData UpdateData(0.0f, false, false);
                pLoadedData->spBorder->Update(UpdateData);
            }
        }
    }
}

int32_t TESObjectLAND::GetDataX() {
    if (pLoadedData) {
        return pLoadedData->iCellX;
    }
    if (pParentCell) {
        return pParentCell->GetDataX();
    }
    return 0;
}

int32_t TESObjectLAND::GetDataY() {
    if (pLoadedData) {
        return pLoadedData->iCellY;
    }
    if (pParentCell) {
        return pParentCell->GetDataY();
    }
    return 0;
}

float TESObjectLAND::GetWorldCellX() {
    return static_cast<double>(GetDataX() << 12);
}

TESLandTexture* TESObjectLAND::GetQuadTexture(uint8_t acBlock, uint16_t aiLayer) {
    if (acBlock >= 4u || aiLayer >= 6u) {
        return nullptr;
    }
    if (!pLoadedData) {
        return nullptr;
    }
    if (pLoadedData->pQuadTextureArray[acBlock]) {
        return pLoadedData->pQuadTextureArray[acBlock][aiLayer];
    }
    return nullptr;
}

float TESObjectLAND::GetDefaultWorldHeight() {
    if (pParentCell) {
        TESWorldSpace* pWorldSpace = pParentCell->GetWorldSpace();
        if (pWorldSpace) {
            return pWorldSpace->GetDefaultLandHeight();
        }
    }
    return -2048.0f;
}

bool TESObjectLAND::GetCoordData(COORD_DATA* cdata, const NiPoint3& coord, bool forcetovertex) {
    NiPoint3 vert;
    float fcellx = static_cast<double>(GetDataX()) * 4096.0;
    float fcelly = static_cast<double>(GetDataY()) * 4096.0;
    if (!pLoadedData || (!pLoadedData->ppVertices && !pLoadedData->ppMesh)) {
        return false;
    }
    if (fcellx > static_cast<double>(coord.x)) {
        return false;
    }
    float fcellmaxx = fcellx + 4096.0;
    if (coord.x > static_cast<double>(fcellmaxx)) {
        return false;
    }
    if (fcelly > static_cast<double>(coord.y)) {
        return false;
    }
    float fcellmaxy = fcelly + 4096.0;
    if (coord.y > static_cast<double>(fcellmaxy)) {
        return false;
    }
    cdata->Cellpoint.x = coord.x - fcellx;
    cdata->Cellpoint.y = coord.y - fcelly;
    cdata->Block.x = static_cast<int32_t>(cdata->Cellpoint.x / 2048.0);
    if (!(static_cast<int32_t>(cdata->Cellpoint.x) % 2048) && static_cast<int32_t>(cdata->Cellpoint.x)) {
        --cdata->Block.x;
    }

    cdata->Block.y = static_cast<int32_t>(cdata->Cellpoint.y / 2048.0);
    if (!(static_cast<int32_t>(cdata->Cellpoint.y) % 2048) && static_cast<int32_t>(cdata->Cellpoint.y)) {
        --cdata->Block.y;
    }

    cdata->iBlock = cdata->Block.x + 2 * cdata->Block.y;
    float fblockx = static_cast<double>(cdata->Block.x << 11) + fcellx;
    float fblocky = static_cast<double>(cdata->Block.y << 11) + fcelly;
    cdata->Blockpoint.x = coord.x - fblockx;
    cdata->Blockpoint.y = coord.y - fblocky;
    float fcentery = GetWorldCellY() + 2048.0;
    float fcenterx = GetWorldCellX() + 2048.0;
    NiPoint3 kcellcenter(fcenterx, fcentery, static_cast<float>(0.0));
    cdata->Tile.x = static_cast<int32_t>(cdata->Blockpoint.x / 128.0);
    if (!(static_cast<int32_t>(cdata->Blockpoint.x) % 128) && static_cast<int32_t>(cdata->Blockpoint.x)) {
        --cdata->Tile.x;
    }

    cdata->Tile.y = static_cast<int32_t>(cdata->Blockpoint.y / 128.0);
    if (!(static_cast<int32_t>(cdata->Blockpoint.y) % 128) && static_cast<int32_t>(cdata->Blockpoint.y)) {
        --cdata->Tile.y;
    }

    float ftilex = static_cast<double>(cdata->Tile.x << 7) + fblockx;
    float ftiley = static_cast<double>(cdata->Tile.y << 7) + fblocky;
    cdata->Tilepoint.x = coord.x - ftilex;
    cdata->Tilepoint.y = coord.y - ftiley;
    cdata->iShape = cdata->iBlock;
    if ((cdata->Tile.y + cdata->Tile.x) % 2) {
        cdata->bTriright = true;
        cdata->iTrivertidx[1] = 17 * cdata->Tile.y + cdata->Tile.x + 1;
        cdata->iTrivertidx[2] = cdata->Tile.x + 17 * (cdata->Tile.y + 1);
        if (cdata->Tilepoint.x + cdata->Tilepoint.y > 128.0) {
            cdata->iTrivertidx[0] = 17 * (cdata->Tile.y + 1) + cdata->Tile.x + 1;
            cdata->bTriupper = true;
        }
        else {
            cdata->iTrivertidx[0] = cdata->Tile.x + 17 * cdata->Tile.y;
            cdata->bTriupper = false;
        }
    }
    else {
        cdata->bTriright = false;
        cdata->iTrivertidx[1] = cdata->Tile.x + 17 * cdata->Tile.y;
        cdata->iTrivertidx[2] = 17 * (cdata->Tile.y + 1) + cdata->Tile.x + 1;
        if (cdata->Tilepoint.y < static_cast<double>(cdata->Tilepoint.x)) {
            cdata->iTrivertidx[0] = 17 * cdata->Tile.y + cdata->Tile.x + 1;
            cdata->bTriupper = false;
        }
        else {
            cdata->iTrivertidx[0] = cdata->Tile.x + 17 * (cdata->Tile.y + 1);
            cdata->bTriupper = true;
        }
    }
    if (static_cast<int32_t>(coord.x) % 128 || static_cast<int32_t>(coord.y) % 128) {
        float fmindist = 0.0;
        int32_t iclosest = 0;
        NiPoint2 pt(coord.x, coord.y);
        for (int32_t i = 0; i < 3; ++i) {
            NiPoint2 kdelta;
            NiPoint3 trivert;
            GetVertex(cdata->iBlock, cdata->iTrivertidx[i], trivert);
            float fverty = kcellcenter.y + trivert.y;
            float fvertx = kcellcenter.x + trivert.x;
            NiPoint2 kvert(fvertx, fverty);
            kdelta = kvert - pt;
            float fdist = kdelta.Length();
            if (!i || fmindist > static_cast<double>(fdist)) {
                fmindist = fdist;
                iclosest = i;
            }
        }
        cdata->iVertidx = cdata->iTrivertidx[iclosest];
    }
    else {
        float fvertidx = static_cast<float>(
            static_cast<int32_t>(cdata->Blockpoint.x) / 128 + 17 * (static_cast<int32_t>(cdata->Blockpoint.y) / 128)
        );
        cdata->iVertidx = FloatToInt(fvertidx);
    }
    GetVertex(cdata->iBlock, cdata->iVertidx, vert);
    cdata->Vertcoord = kcellcenter + vert;
    return !forcetovertex || GetCoordData(cdata, cdata->Vertcoord, false);
}

bool TESObjectLAND::GetLandHeight(const COORD_DATA& cdata, float& height) {
    NiPoint3 vert0;
    float fslopeb;
    NiPoint3 vert2;
    NiPoint3 vert1;
    GetVertex(cdata.iBlock, cdata.iTrivertidx[0], vert0);
    GetVertex(cdata.iBlock, cdata.iTrivertidx[1], vert1);
    GetVertex(cdata.iBlock, cdata.iTrivertidx[2], vert2);
    float fz0 = vert0.z;
    float fz1 = vert1.z;
    float fz2 = vert2.z;
    if (cdata.bTriright) {
        double dheight;
        fslopeb = (fz2 - fz0) / 128.0;
        if (cdata.bTriupper) {
            float fslopea = (fz0 - fz2) / 128.0;
            fslopeb = (fz0 - fz1) / 128.0;
            dheight = fz0 - ((128.0 - cdata.Tilepoint.x) * fslopea + (128.0 - cdata.Tilepoint.y) * fslopeb);
        }
        else {
            float fslopea = (fz1 - fz0) / 128.0;
            fslopeb = (fz2 - fz0) / 128.0;
            dheight = fslopea * cdata.Tilepoint.x + fz0 + fslopeb * cdata.Tilepoint.y;
        }
        height = dheight;
    }
    else {
        double dheight;
        float fslopea = (fz0 - fz1) / 128.0;
        fslopeb = (fz2 - fz0) / 128.0;
        if (cdata.bTriupper) {
            dheight = fslopea * cdata.Tilepoint.y + fz1 + fslopeb * cdata.Tilepoint.x;
        }
        else {
            dheight = fslopea * cdata.Tilepoint.x + fz1 + fslopeb * cdata.Tilepoint.y;
        }
        height = dheight;
    }
    return true;
}

NiGeometry* TESObjectLAND::GetGeometry(const NiPoint3& arPoint) {
    COORD_DATA cdata;
    if (!GetCoordData(&cdata, arPoint, false)) {
        return nullptr;
    }
    return GetGeometry(cdata);
}

NiGeometry* TESObjectLAND::GetGeometry(const COORD_DATA& cdata) {
    if (pLoadedData && pLoadedData->ppMesh && pLoadedData->ppMesh[cdata.iBlock]) {
        return static_cast<NiGeometry*>(pLoadedData->ppMesh[cdata.iBlock]->GetAtChecked(0));
    }

    return nullptr;
}

bool TESObjectLAND::IsLandAltered() {
    if (Data.iFlags & LAND_DATA_HEIGHTS_NORMALS) {
        return true;
    }
    if (Data.iFlags & LAND_DATA_TEXTURES) {
        return true;
    }
    if (Data.iFlags & LAND_DATA_COLORS) {
        return true;
    }
    return false;
}

void TESObjectLAND::SetCell(TESObjectCELL* apCell) {
    pParentCell = apCell;
    if (pParentCell) {
        TESWorldSpace* worldSpace = pParentCell->GetWorldSpace();
        if (worldSpace && worldSpace->GetParentWorld(TESWorldSpace::PU_LAND)) {
            UpdateUseParentWorldspace(1);
        }
        else {
            UpdateUseParentWorldspace(0);
        }
    }
}

bool TESObjectLAND::GetLandHeight(const NiPoint3& coord, float& height) {
    COORD_DATA cdata;
    if (!GetCoordData(&cdata, coord, false)) {
        height = GetDefaultWorldHeight();
        return false;
    }
    return GetLandHeight(cdata, height);
}

TESLandTexture* TESObjectLAND::GetMainTexture(const NiPoint3& aCoord) {
    COORD_DATA cdata;
    if (GetCoordData(&cdata, aCoord, true)) {
        return GetMainTexture(cdata.iBlock, cdata.iVertidx);
    }
    return nullptr;
}

bool TESObjectLAND::GetLandNormal(const NiPoint3& aCoord, NiPoint3& aNormal, NiPoint3& aFaceNormal) {
    COORD_DATA coordData;
    if (GetCoordData(&coordData, aCoord, false)) {
        return GetLandNormal(coordData, aNormal, aFaceNormal);
    }

    return false;
}

bool TESObjectLAND::GetLandNormal(const COORD_DATA& cdata, NiPoint3& aNormal, NiPoint3& aFaceNormal) {
    NiPoint3 kyterm4;
    NiPoint3 kxterm4;
    NiPoint3 kyterm3;
    NiPoint3 kxterm3;
    NiPoint3 kyterm2;
    NiPoint3 kxterm2;
    NiPoint3 kyterm1;
    NiPoint3 kxterm1;
    NiPoint3 kvert;
    NiPoint3 kloc;
    NiPoint3 kworldvert;
    kloc = GetVertexLocation(cdata.iBlock, cdata.iVertidx, nullptr, nullptr);
    GetVertex(cdata.iBlock, cdata.iVertidx, kvert);
    float fcentery = GetWorldCellY() + 2048.0;
    float fcenterx = GetWorldCellX() + 2048.0;
    NiPoint3 kcellcenter(fcenterx, fcentery, static_cast<float>(0.0));
    kworldvert = kvert + kcellcenter;
    kworldvert.z = 0.0;
    if (kworldvert == kloc) {
        GetNormal(cdata.iBlock, cdata.iVertidx, aNormal);
        aFaceNormal = aNormal;
    }
    else {
        NiPoint3 norm2;
        NiPoint3 pt;
        NiPoint3 kedge2;
        NiPoint3 norm;
        NiPoint3 norm1;
        NiPoint3 kedge1;
        NiPoint3 vert;
        float fpx = std::fmodf(kloc.x, 128.0f) / 128.0;
        float fpy = std::fmodf(kloc.y, 128.0f) / 128.0;
        NiPoint3 knormal = NiPoint3::UNIT_Z;
        GetNormal(cdata.iBlock, cdata.iTrivertidx[0], norm);
        GetNormal(cdata.iBlock, cdata.iTrivertidx[1], norm1);
        GetNormal(cdata.iBlock, cdata.iTrivertidx[2], norm2);
        if (!cdata.bTriright && !cdata.bTriupper) {
            kyterm1 = fpy * norm2;
            float finvy = 1.0 - fpy;
            kxterm1 = fpx * norm;
            float finvx = 1.0 - fpx;
            knormal = (finvx * norm1 + kxterm1) * finvy + kyterm1;
        }
        if (!cdata.bTriright && cdata.bTriupper) {
            float finvy = 1.0 - fpy;
            kyterm2 = finvy * norm1;
            kxterm2 = fpx * norm2;
            float finvx = 1.0 - fpx;
            knormal = (finvx * norm + kxterm2) * fpy + kyterm2;
        }
        if (cdata.bTriright && !cdata.bTriupper) {
            kyterm3 = fpy * norm2;
            float finvy = 1.0 - fpy;
            kxterm3 = fpx * norm1;
            float finvx = 1.0 - fpx;
            knormal = (finvx * norm + kxterm3) * finvy + kyterm3;
        }
        if (cdata.bTriright && cdata.bTriupper) {
            float finvy = 1.0 - fpy;
            kyterm4 = finvy * norm1;
            kxterm4 = fpx * norm;
            float finvx = 1.0 - fpx;
            knormal = (finvx * norm2 + kxterm4) * fpy + kyterm4;
        }
        aNormal = knormal;
        aNormal.Unitize();
        GetVertex(cdata.iBlock, cdata.iTrivertidx[0], kedge1);
        GetVertex(cdata.iBlock, cdata.iTrivertidx[1], vert);
        kedge1 -= vert;
        GetVertex(cdata.iBlock, cdata.iTrivertidx[0], kedge2);
        GetVertex(cdata.iBlock, cdata.iTrivertidx[2], pt);
        kedge2 -= pt;
        if ((!cdata.bTriright || cdata.bTriupper) && (cdata.bTriright || !cdata.bTriupper)) {
            aFaceNormal = kedge2.UnitCross(kedge1);
        }
        else {
            aFaceNormal = kedge1.UnitCross(kedge2);
        }
    }
    return true;
}

bool TESObjectLAND::GetLandColor(const NiPoint3& aCoord, NiColorA& aColor) {
    COORD_DATA cdata;
    if (!GetCoordData(&cdata, aCoord, false)) {
        return false;
    }
    NiColorA c0(0.0f, 0.0f, 0.0f, 0.0f);
    NiColorA c1(0.0f, 0.0f, 0.0f, 0.0f);
    NiColorA c2(0.0f, 0.0f, 0.0f, 0.0f);
    float fpx = std::fmodf(aCoord.x, 128.0f) / 128.0;
    float fpy = std::fmodf(aCoord.y, 128.0f) / 128.0;
    GetColorA(cdata.iBlock, cdata.iTrivertidx[0], c0);
    GetColorA(cdata.iBlock, cdata.iTrivertidx[1], c1);
    GetColorA(cdata.iBlock, cdata.iTrivertidx[2], c2);
    if (!cdata.bTriright && !cdata.bTriupper) {
        float finvy = 1.0 - fpy;
        float finvx = 1.0 - fpx;
        aColor = (finvx * c1 + fpx * c0) * finvy + fpy * c2;
    }
    if (!cdata.bTriright && cdata.bTriupper) {
        float finvy = 1.0 - fpy;
        float finvx = 1.0 - fpx;
        aColor = (finvx * c0 + fpx * c2) * fpy + finvy * c1;
    }
    if (cdata.bTriright && !cdata.bTriupper) {
        float finvy = 1.0 - fpy;
        float finvx = 1.0 - fpx;
        aColor = (finvx * c0 + fpx * c1) * finvy + fpy * c2;
    }
    if (cdata.bTriright && cdata.bTriupper) {
        float finvy = 1.0 - fpy;
        float finvx = 1.0 - fpx;
        aColor = (finvx * c2 + fpx * c0) * fpy + finvy * c1;
    }
    return true;
}

bool TESObjectLAND::GetLandColor(const COORD_DATA& cdata, NiColorA& color) {
    GetColorA(cdata.iBlock, cdata.iVertidx, color);
    return true;
}

void TESObjectLAND::InitDistantTextureBlending() {
    BGSTerrainManager* pterrainManager;
    NiPoint3 kcellcenter;
    if (TES::pInstance->pWorldSpace) {
        pterrainManager = TES::pInstance->pWorldSpace->GetTerrainManager();
    }
    else {
        pterrainManager = nullptr;
    }
    kcellcenter = NiPoint3::ZERO;
    kcellcenter.x = (static_cast<double>(pLoadedData->iCellX) + 0.5) * 4096.0;
    kcellcenter.y = (static_cast<double>(pLoadedData->iCellY) + 0.5) * 4096.0;
    NiPoint2 kcellOffset = NiPoint2::ZERO;
    for (int32_t i = 0; i < 4; ++i) {
        if (pLoadedData->ppMesh && pterrainManager) {
            NiAVObject* pgeom = pLoadedData->ppMesh[i]->GetAtChecked(0);
            NiShadeProperty* pprop = static_cast<NiShadeProperty*>(pgeom->GetProperty(3));
            int32_t ipplighting = pprop && pprop->iShaderPropertyType >= 8 && pprop->iShaderPropertyType <= 12;
            BSShaderPPLightingProperty* pshprop = nullptr;
            if (ipplighting) {
                pshprop = static_cast<BSShaderPPLightingProperty*>(pprop);
            }
            if (pshprop && (!pshprop->GetDiffuseTextures(9) || !pshprop->GetNormalTexture(9u))) {
                NiTexture* pdiffuse = pterrainManager->GetDiffuseTextureForPoint(kcellcenter);
                NiTexture* pnormal = pterrainManager->GetNormalTextureForPoint(kcellcenter);
                pshprop->SetDiffuseTexture(9u, pdiffuse);
                pshprop->SetNormalTexture(9u, pnormal);
                NiColorA kbaseCoord = NiColorA::BLACK;
                switch (i) {
                    case 0: {
                        kcellOffset = NiPoint2(0.0f, 0.0f);
                        break;
                    }
                    case 1: {
                        kcellOffset = NiPoint2(0.5f, 0.0f);
                        break;
                    }
                    case 2: {
                        kcellOffset = NiPoint2(0.0f, 0.5f);
                        break;
                    }
                    case 3: {
                        kcellOffset = NiPoint2(0.5f, 0.5f);
                        break;
                    }
                    default: {
                        break;
                    }
                }
                BGSTerrainChunk* pchunk = pterrainManager->GetChunkForPoint(kcellcenter);
                if (pchunk) {
                    int16_t sx = pchunk->pNode->sBaseCellX;
                    int16_t sy = pchunk->pNode->sBaseCellY;
                    float foriginX = static_cast<double>(pLoadedData->iCellX) - static_cast<double>(sx);
                    float foriginY = static_cast<double>(pLoadedData->iCellY) - static_cast<double>(sy);
                    double doffsetX = foriginX + kcellOffset.x;
                    double doffsetY = foriginY + kcellOffset.y;
                    kbaseCoord.r = doffsetX / static_cast<double>(pchunk->pNode->iLevel);
                    kbaseCoord.g = doffsetY / static_cast<double>(pchunk->pNode->iLevel);
                    while (kbaseCoord.r > 1.0) {
                        kbaseCoord.r = kbaseCoord.r - 1.0;
                    }
                    while (kbaseCoord.r < 0.0) {
                        kbaseCoord.r = kbaseCoord.r + 1.0;
                    }
                    while (kbaseCoord.g > 1.0) {
                        kbaseCoord.g = kbaseCoord.g - 1.0;
                    }
                    while (kbaseCoord.g < 0.0) {
                        kbaseCoord.g = kbaseCoord.g + 1.0;
                    }
                    pshprop->LandBlendParamsA.x = kbaseCoord.r;
                    pshprop->LandBlendParamsA.y = kbaseCoord.g;
                    pshprop->LandBlendParamsA.z = kbaseCoord.b;
                    pshprop->LandBlendParamsA.w = kbaseCoord.a;
                }
            }
        }
    }
}

float TESObjectLAND::GetWorldCellY() {
    return static_cast<double>(GetDataY() << 12);
}

TESWorldSpace* TESObjectLAND::GetWorldSpace() {
    if (pParentCell) {
        return pParentCell->GetWorldSpace();
    }

    return nullptr;
}

TESLandTexture* TESObjectLAND::GetDefaultTexture(uint8_t acBlock) {
    TESLandTexture* pTexture = nullptr;
    if (acBlock < 4 && pLoadedData) {
        pTexture = pLoadedData->pDefQuadTexture[acBlock];
    }
    return pTexture;
}

float TESObjectLAND::GetBaseHeight() {
    if (pLoadedData) {
        return pLoadedData->fBaseHeight;
    }

    return 0.0f;
}

// aiBlock is never read. The two sums are made with the double 2048.0.
NiPoint3 TESObjectLAND::GetWorldOffsetForBlock(int32_t aiBlock) {
    NiPoint3 kResult;
    kResult.x = GetWorldCellX() + 2048.0;
    kResult.y = GetWorldCellY() + 2048.0;
    kResult.z = GetBaseHeight();
    return static_cast<const NiPoint3&>(kResult);
}

// The vertex starts a 4096-unit cell apart, is pushed half a cell along the axes its index's low bits name, and is
// moved by the block's 128-unit tiles. The sums are kept in floats, in a local point copied to the result only at the
// end.
NiPoint3 TESObjectLAND::GetVertexLocation(uint8_t acBlock, uint16_t asVertIndex, int32_t* apCellX, int32_t* apCellY) {
    NiPoint3 kPos;
    kPos.x = 4096.0f;
    kPos.y = 4096.0f;
    int32_t iCellX = apCellX ? *apCellX : GetDataX();
    kPos.x = static_cast<float>(iCellX) * kPos.x;
    int32_t iCellY = apCellY ? *apCellY : GetDataY();
    kPos.y = static_cast<float>(iCellY) * kPos.y;
    kPos.x = static_cast<float>((acBlock & 1) != 0 ? 2048 : 0) + kPos.x;
    kPos.y = static_cast<float>((acBlock & 2) != 0 ? 2048 : 0) + kPos.y;
    kPos.x = static_cast<float>((asVertIndex % 17) << 7) + kPos.x;
    kPos.y = static_cast<float>((asVertIndex / 17) << 7) + kPos.y;
    kPos.z = 0.0f;
    NiPoint3 kResult = kPos;
    return kResult;
}

// The extents are unset when x is +FLT_MAX, or when y is -FLT_MAX: the first request then computes them from the four
// blocks when the land has them, and otherwise sets both to the world's default height. A NaN x falls to the y test, a
// NaN y returns the extents as they are. The compares are against the exact FLT_MAX values widened to double.
NiPoint2 TESObjectLAND::GetMinMaxLandHeight() {
    if (!pLoadedData) {
        return NiPoint2(std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    }
    float fMin = pLoadedData->HeightExtents.x;
    float fMax = pLoadedData->HeightExtents.y;
    if (!(fMin == 3.4028234663852886e+38) && !(fMax == -3.4028234663852886e+38)) {
        return pLoadedData->HeightExtents;
    }

    if ((Data.iFlags & LAND_DATA_HEIGHTS_NORMALS) != 0) {
        for (int32_t i = 0; i < 4; ++i) {
            NiPoint2 kBlock = GetMinMaxBlockHeight(i);
            if (pLoadedData->HeightExtents.x > kBlock.x) {
                pLoadedData->HeightExtents.x = kBlock.x;
            }
            if (pLoadedData->HeightExtents.y < kBlock.y) {
                pLoadedData->HeightExtents.y = kBlock.y;
            }
        }
    }
    else {
        float fHeight = GetDefaultWorldHeight();
        float fHeight2 = GetDefaultWorldHeight();
        NiPoint2 kExtents(fHeight2, fHeight);
        pLoadedData->HeightExtents.x = kExtents.x;
        pLoadedData->HeightExtents.y = kExtents.y;
    }
    return pLoadedData->HeightExtents;
}

// The block's lowest and highest vertex height.
NiPoint2 TESObjectLAND::GetMinMaxBlockHeight(int32_t iblock) {
    NiPoint2 kResult;
    NiPoint2 kMinMax(std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    if (iblock < 16) {
        for (int32_t i = 0; i < 289; ++i) {
            NiPoint3 kVertex;
            GetVertex(iblock, i, kVertex);
            if (kMinMax.x > static_cast<double>(kVertex.z)) {
                kMinMax.x = kVertex.z;
            }
            if (kMinMax.y < static_cast<double>(kVertex.z)) {
                kMinMax.y = kVertex.z;
            }
        }
    }
    kResult = kMinMax;
    return kResult;
}

bool TESObjectLAND::GetLandRemapped() {
    return (Data.iFlags & LAND_DATA_USE_PARENT_WORLDSPACE) != 0;
}

bool TESObjectLAND::HasGoodNormals() {
    return (Data.iFlags & LAND_DATA_GOOD_NORMALS) != 0;
}

void TESObjectLAND::SetGoodNormals(bool abVal) {
    if (abVal) {
        Data.iFlags |= LAND_DATA_GOOD_NORMALS;
    }
    else {
        Data.iFlags &= ~LAND_DATA_GOOD_NORMALS;
    }
}

NiNode* TESObjectLAND::GetQuadLandNode(int32_t aiBlock) {
    if (pParentCell && aiBlock < 4) {
        return static_cast<NiNode*>(pParentCell->GetLandNode(aiBlock));
    }

    return nullptr;
}

void TESObjectLAND::MarkLoaded(bool loaded) {
    if (loaded) {
        Data.iFlags |= LAND_DATA_LOADED;
    }
    else {
        Data.iFlags &= ~LAND_DATA_LOADED;
    }
}

void TESObjectLAND::SetTexture(uint8_t acBlock, uint16_t asTexIndex, TESLandTexture* apTexture) {
    if (acBlock < 4u && asTexIndex < 6u && pLoadedData) {
        pLoadedData->pQuadTextureArray[acBlock][asTexIndex] = apTexture;
    }
}

float TESObjectLAND::GetDefaultTexturePercent(uint8_t acBlock, uint16_t asVertIndex) {
    int32_t i;
    float ftotal = 0.0f;
    if (acBlock < 4u && asVertIndex < 0x121u && pLoadedData && pLoadedData->ppPercentArrays[acBlock]) {
        for (i = 0; i < 6; ++i) {
            ftotal = ftotal + pLoadedData->ppPercentArrays[acBlock][asVertIndex][i];
        }
    }

    return static_cast<float>(1.0 - ftotal);
}

float TESObjectLAND::GetTexturePercent(uint8_t acBlock, uint16_t asVertIndex, uint16_t asTexIndex) {
    if (acBlock >= 4u || asVertIndex >= 0x121u || asTexIndex >= 6u) {
        return 0.0f;
    }
    if (!pLoadedData) {
        return 0.0f;
    }
    if (pLoadedData->ppPercentArrays[acBlock]) {
        return pLoadedData->ppPercentArrays[acBlock][asVertIndex][asTexIndex];
    }
    return 0.0f;
}

void TESObjectLAND::SetTexturePercent(uint8_t acBlock, uint16_t asVertIndex, uint16_t asTexIndex, float afPercent) {
    if (acBlock < 4u && asVertIndex < 0x121u && asTexIndex < 6u && pLoadedData) {
        if (afPercent > 0.0) {
            pLoadedData->ppPercentArrays[acBlock][asVertIndex][asTexIndex] = afPercent;
        }
        else if (pLoadedData->ppPercentArrays[acBlock]) {
            pLoadedData->ppPercentArrays[acBlock][asVertIndex][asTexIndex] = 0.0f;
        }
    }
}

void TESObjectLAND::RemoveTextureFromArrays(uint8_t acBlock, uint16_t asRemoveIndex) {
    int32_t k;
    int32_t j;
    int32_t i;
    if (acBlock < 4u && asRemoveIndex < 6u && pLoadedData) {
        for (i = asRemoveIndex; i < 5; ++i) {
            pLoadedData->pQuadTextureArray[acBlock][i] = pLoadedData->pQuadTextureArray[acBlock][i + 1];
        }
        pLoadedData->pQuadTextureArray[acBlock][5] = nullptr;
        if (pLoadedData->ppPercentArrays[acBlock]) {
            for (j = 0; j < 289; ++j) {
                for (k = asRemoveIndex; k < 5; ++k) {
                    pLoadedData->ppPercentArrays[acBlock][j][k] = pLoadedData->ppPercentArrays[acBlock][j][k + 1];
                }
                pLoadedData->ppPercentArrays[acBlock][j][5] = 0.0f;
            }
        }
    }
}

bool TESObjectLAND::VertexInRect(tagRECT* apRect, const NiPoint3& aPoint) {
    NiPoint3 kpoint;
    if (!apRect) {
        return true;
    }
    float fcentery = static_cast<float>(GetWorldCellY() + 2048.0);
    float fcenterx = static_cast<float>(GetWorldCellX() + 2048.0);
    ::new (&kpoint) NiColor(fcenterx, fcentery, 0.0);
    kpoint += aPoint;
    if (kpoint.x > static_cast<double>(apRect->right) || kpoint.y > static_cast<double>(apRect->top)) {
        return false;
    }
    return !(kpoint.x < static_cast<double>(apRect->left)) && !(kpoint.y < static_cast<double>(apRect->bottom));
}

bool TESObjectLAND::IsNormalSet(int32_t coord, int32_t aiIndex) {
    return pLoadedData->ppNormalsSet[coord][aiIndex];
}

bool TESObjectLAND::IsNormalSet(const NiPoint3& coord) {
    COORD_DATA cdata;
    return GetCoordData(&cdata, coord, false) && IsNormalSet(cdata.iBlock, cdata.iVertidx);
}

void TESObjectLAND::UpdateUseParentWorldspace(char abUse) {
    if (abUse) {
        Data.iFlags |= LAND_DATA_USE_PARENT_WORLDSPACE;
    }
    else {
        Data.iFlags &= ~LAND_DATA_USE_PARENT_WORLDSPACE;
    }
}

void TESObjectLAND::SetHasMopp(uint8_t abHasMopp) {
    if (abHasMopp) {
        Data.iFlags |= LAND_DATA_HAS_MOPP;
    }
    else {
        Data.iFlags &= ~LAND_DATA_HAS_MOPP;
    }
}

float TESObjectLAND::GetTextureUsePercent(uint8_t acBlock, uint16_t asTexIndex) {
    int32_t i;
    float ftotal = 0.0f;
    if (acBlock < 4u && asTexIndex < 6u && GetQuadTexture(acBlock, asTexIndex)) {
        for (i = 0; i < 289; ++i) {
            ftotal = GetTexturePercent(acBlock, i, asTexIndex) + ftotal;
        }
    }

    return ftotal;
}

void TESObjectLAND::GetColorA(int32_t iblock, int32_t index, NiColorA& color) {
    if (HasVertices()) {
        char bGotVertexColors = 0;
        if (pLoadedData->ppColorsA && pLoadedData->ppColorsA[iblock]) {
            color = pLoadedData->ppColorsA[iblock][index];
            bGotVertexColors = 1;
        }
        else if (pLoadedData->ppMesh && pLoadedData->ppMesh[iblock]) {
            NiGeometry* shape = static_cast<NiGeometry*>(pLoadedData->ppMesh[iblock]->GetAt(0));
            if (shape->GetNormals()) {
                color = shape->GetColors()[index];
                bGotVertexColors = 1;
            }
            else {
                NiGeometryData* pgeodata = shape->GetModelData();
                NiTConvertingStrideIterator<NiColorA> kIterator;
                NiAdditionalGeometryData* pkAGD = pgeodata->GetAdditionalGeometryData();
                if (pkAGD && pkAGD->GetVBReady() && pgeodata->LockPackedVertexData(true)) {
                    pgeodata->GetColorsIterator(*static_cast<NiGeometryDataIteratorNiColorA*>(&kIterator));
                    kIterator.Get(index, color);
                    pgeodata->UnlockPackedVertexData();
                    bGotVertexColors = 1;
                }
            }
        }
        if (!bGotVertexColors) {
            color = NiColorA::WHITE;
        }
    }
    else {
        color = NiColorA::WHITE;
    }
}

NiTriStrips* TESObjectLAND::MakeLandTriStrips(uint8_t aiBlock) {
    NiTriStrips* ptrigeom = nullptr;
    if (aiBlock < 4u && pLoadedData) {
        char acName[264];
        NiPoint2* ptextures = new NiPoint2[289];
        ourMemcpy(ptextures, TESObjectLAND::pDefTextureBLOCK, sizeof(NiPoint2) * 289);
        uint16_t* pstriplength = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t)));
        uint16_t* ptriplist = static_cast<uint16_t*>(_NiMalloc(1021 * sizeof(uint16_t)));
        *pstriplength = 1021;
        ourMemcpy(ptriplist, TESObjectLAND::pTriStripListBLOCK, sizeof(uint16_t) * 1021);
        NiPoint3* pnormalcopy = new NiPoint3[289];
        ourMemcpy(pnormalcopy, pLoadedData->ppNormals[aiBlock], sizeof(NiPoint3) * 289);
        ptrigeom = new NiTriStrips(
            289,
            pLoadedData->ppVertices[aiBlock],
            pnormalcopy,
            pLoadedData->ppColorsA[aiBlock],
            ptextures,
            1,
            0,
            *pstriplength - 2,
            1,
            pstriplength,
            ptriplist
        );
        NiPoint3 blockcenter = GetWorldOffsetForBlock(aiBlock);
        ptrigeom->SetTranslate(blockcenter);
        BSsprintf(acName, 260, "Block (%i, %i)", aiBlock & 3, aiBlock / 4);
        NiFixedString kName(static_cast<const char*>(acName));
        ptrigeom->SetName(kName);
    }

    return ptrigeom;
}

bool TESObjectLAND::InitLandscape() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LAND,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESLand.cpp",
        3236
    );
    if (pLoadedData && pLoadedData->ppMesh && *pLoadedData->ppMesh) {
        int32_t i;
        NiColorA kDefColor(1.0f, 1.0f, 1.0f, 0.0f);
        if ((Data.iFlags & LAND_DATA_COLORS) == 0) {
            for (i = 0; i < 4; ++i) {
                for (int32_t j = 0; j < 17; ++j) {
                    for (int32_t k = 0; k < 17; ++k) {
                        NiColorA* pColor = &pLoadedData->ppColorsA[i][17 * j + k];
                        pColor->r = kDefColor.r;
                        pColor->g = kDefColor.g;
                        pColor->b = kDefColor.b;
                        pColor->a = kDefColor.a;
                    }
                }
            }
        }
        MarkLoaded(true);
        for (i = 0; i < 4; ++i) {
            // The address &pLoadedData->ppPercentArrays is tested here, and it is never null.
            // The block's percent buffer, 289 vertices of 8 floats, which the extra data keeps as untyped bytes.
            NiBinaryExtraData* ppercentED = new NiBinaryExtraData(
                289 * 8 * sizeof(float),
                reinterpret_cast<char*>(*pLoadedData->ppPercentArrays[i])
            );
            NiFixedString* pTexPercTag = BSShaderManager::TexPercTag();
            pLoadedData->ppMesh[i]->AddExtraData(*pTexPercTag, ppercentED);
            BSShaderPPLightingProperty* pshprop = new BSShaderPPLightingProperty;
            pshprop->SetShader(1);
            if (pshprop) {
                uint8_t ucexp1;
                uint8_t ucexp2;
                uint8_t ucexp3;
                uint8_t ucexp4;
                uint8_t ucexp5;
                uint8_t ucexp6;
                TESLandTexture* pQuadTex = pLoadedData->pDefQuadTexture[i];
                if (pQuadTex && pQuadTex->pTextureSet) {
                    BSTextureSet* pTextureSet = pQuadTex->pTextureSet->GetAsShaderTextureSet();
                    pshprop->SetLandscapeTextureSet(0, pTextureSet);
                }
                else {
                    TESLandTexture* pDefault = TESObjectLAND::GetDefaultTexture();
                    BSTextureSet* pTextureSet = pDefault->pTextureSet->GetAsShaderTextureSet();
                    pshprop->SetLandscapeTextureSet(0, pTextureSet);
                }
                for (int32_t itex = 1; itex < 7; ++itex) {
                    pQuadTex = pLoadedData->pQuadTextureArray[i][itex - 1];
                    if (pQuadTex && pQuadTex->pTextureSet) {
                        BSTextureSet* pTextureSet = pQuadTex->pTextureSet->GetAsShaderTextureSet();
                        pshprop->SetLandscapeTextureSet(static_cast<uint32_t>(itex), pTextureSet);
                    }
                    else {
                        pshprop->SetLandscapeTextureSet(static_cast<uint32_t>(itex), nullptr);
                    }
                }
                if (pLoadedData->pQuadTextureArray[i][5]) {
                    ucexp6 = pLoadedData->pQuadTextureArray[i][5]->GetSpecularExponent();
                }
                else {
                    ucexp6 = 0;
                }
                if (pLoadedData->pQuadTextureArray[i][4]) {
                    ucexp5 = pLoadedData->pQuadTextureArray[i][4]->GetSpecularExponent();
                }
                else {
                    ucexp5 = 0;
                }
                if (pLoadedData->pQuadTextureArray[i][3]) {
                    ucexp4 = pLoadedData->pQuadTextureArray[i][3]->GetSpecularExponent();
                }
                else {
                    ucexp4 = 0;
                }
                if (pLoadedData->pQuadTextureArray[i][2]) {
                    ucexp3 = pLoadedData->pQuadTextureArray[i][2]->GetSpecularExponent();
                }
                else {
                    ucexp3 = 0;
                }
                if (pLoadedData->pQuadTextureArray[i][1]) {
                    ucexp2 = pLoadedData->pQuadTextureArray[i][1]->GetSpecularExponent();
                }
                else {
                    ucexp2 = 0;
                }
                if (pLoadedData->pQuadTextureArray[i][0]) {
                    ucexp1 = pLoadedData->pQuadTextureArray[i][0]->GetSpecularExponent();
                }
                else {
                    ucexp1 = 0;
                }
                if (pLoadedData->pDefQuadTexture[i]) {
                    uint8_t ucexp0 = pLoadedData->pDefQuadTexture[i]->GetSpecularExponent();
                    pshprop->SetLandscapeSpecularExponents(
                        ucexp0,
                        ucexp1,
                        ucexp2,
                        ucexp3,
                        ucexp4,
                        ucexp5,
                        ucexp6,
                        0,
                        0,
                        0
                    );
                }
                else {
                    pshprop->SetLandscapeSpecularExponents(0, ucexp1, ucexp2, ucexp3, ucexp4, ucexp5, ucexp6, 0, 0, 0);
                }
                NiGeometry* pgeom = static_cast<NiGeometry*>(pLoadedData->ppMesh[i]->GetAtChecked(0));
                BSShaderPPLightingProperty::TangentSpaceData* ptangentspace = BSShaderManager::CreateTangentSpaceSimple(
                    pgeom
                );
                pshprop->StoreExtraDataPointers(ptangentspace);
                NiAdditionalGeometryData* pdata = pshprop->CreateRendererSpecificProperty(pgeom);
                pgeom->SetAdditionalGeometryData(pdata);
                pgeom->AttachProperty(pshprop);
                BSShaderManager::PrepareObject(pLoadedData->ppMesh[i], 0, 0);
            }
        }
        return true;
    }

    return false;
}

// The buffer holds one total per texture and one for the default texture after them.
void TESObjectLAND::AdjustTextureArrays() {
    if (pLoadedData) {
        for (int32_t iblock = 0; iblock < 4; ++iblock) {
            for (uint32_t itex = 0; itex < 6; ++itex) {
                if (pLoadedData->pQuadTextureArray[iblock][itex]) {
                    float fblockuse = GetTextureUsePercent(iblock, itex);
                    float fblockpercent = fblockuse / 289.0;
                    if (fblockpercent < 0.0010000000474974513) {
                        pLoadedData->pQuadTextureArray[iblock][itex] = nullptr;
                    }
                }
            }
            uint32_t itexcount = 6;
            for (uint32_t itex = 0; itex < itexcount; ++itex) {
                while (!pLoadedData->pQuadTextureArray[iblock][itex] && itex < itexcount) {
                    RemoveTextureFromArrays(iblock, itex);
                    --itexcount;
                }
            }
            float* ptotaluse = static_cast<float*>(ourMalloc(sizeof(float) * itexcount + sizeof(float)));
            ourMemset(ptotaluse, 0, sizeof(float) * itexcount + sizeof(float));
            for (int32_t ivert = 0; ivert < 289; ++ivert) {
                float fnondefault = 0.0f;
                for (uint32_t itex = 0; itex < itexcount; ++itex) {
                    float fuse = GetTexturePercent(iblock, ivert, itex);
                    ptotaluse[itex] = ptotaluse[itex] + fuse;
                    fnondefault = fnondefault + fuse;
                }
                if (fnondefault < 1.0) {
                    ptotaluse[itexcount] = 1.0 - fnondefault + ptotaluse[itexcount];
                }
            }
            int32_t inewdefault = -1;
            float fmax = ptotaluse[itexcount];
            for (uint32_t itex = 0; itex < itexcount; ++itex) {
                if (fmax < static_cast<double>(ptotaluse[itex])) {
                    fmax = ptotaluse[itex];
                    inewdefault = itex;
                }
            }
            if (inewdefault != -1) {
                SetNewDefaultTexture(iblock, inewdefault);
            }
            ourFree(ptotaluse);
        }
    }
}

void TESObjectLAND::MergeSameFileTextures() {
    for (int32_t i = 0; i < 4; ++i) {
        float atotalpercent[289];
        TESLandTexture* pdefault = GetDefaultTexture(i);
        BaseFormComponent* pdeftexture = dynamic_cast<TESTexture*>(pdefault);
        ourMemset(atotalpercent, 0, sizeof(atotalpercent));
        for (int32_t j = 0; j < 6; ++j) {
            TESLandTexture* pquadtex = GetQuadTexture(i, j);
            TESTexture* ptexture = dynamic_cast<TESTexture*>(pquadtex);
            if (ptexture) {
                for (int32_t k = 0; k < 6; ++k) {
                    if (j != k) {
                        TESLandTexture* pcompare = GetQuadTexture(i, k);
                        BaseFormComponent* apCompareFC = dynamic_cast<TESTexture*>(pcompare);
                        if (apCompareFC && !ptexture->TESTexture::CompareComponent(apCompareFC)) {
                            for (int32_t m = 0; m < 289; ++m) {
                                pLoadedData->ppPercentArrays[i][m][j] = pLoadedData->ppPercentArrays[i][m][j] +
                                    pLoadedData->ppPercentArrays[i][m][k];
                                pLoadedData->ppPercentArrays[i][m][k] = 0.0f;
                            }
                            SetTexture(i, k, nullptr);
                        }
                    }
                }
                if (pdeftexture && !ptexture->TESTexture::CompareComponent(pdeftexture)) {
                    for (int32_t n = 0; n < 289; ++n) {
                        pLoadedData->ppPercentArrays[i][n][j] = 0.0f;
                    }
                    SetTexture(i, j, nullptr);
                }
            }
            for (int32_t ii = 0; ii < 289; ++ii) {
                atotalpercent[ii] = atotalpercent[ii] + pLoadedData->ppPercentArrays[i][ii][j];
            }
        }
        uint32_t itexcount = 6;
        for (uint32_t jj = 0; jj < itexcount; ++jj) {
            while (!pLoadedData->pQuadTextureArray[i][jj] && jj < itexcount) {
                RemoveTextureFromArrays(i, jj);
                --itexcount;
            }
        }
        for (int32_t kk = 0; kk < 289; ++kk) {
            float ftotal = 0.0f;
            for (int32_t mm = 6; mm; --mm) {
                pLoadedData->ppPercentArrays[i][kk][mm] = pLoadedData->ppPercentArrays[i][kk][mm - 1];
                ftotal = ftotal + pLoadedData->ppPercentArrays[i][kk][mm];
            }
            float fremaining = 1.0 - ftotal;
            float fnonnegative = BSMAX(0.0f, fremaining);
            double dclamped = BSMIN(static_cast<float>(1.0), fnonnegative);
            *pLoadedData->ppPercentArrays[i][kk] = dclamped;
            if (atotalpercent[kk] > 1.0) {
                for (int32_t nn = 6; nn; --nn) {
                    pLoadedData->ppPercentArrays[i][kk][nn] = pLoadedData->ppPercentArrays[i][kk][nn] / ftotal;
                }
            }
        }
    }
}

char TESObjectLAND::CalculateGrassParams() {
    if (!pLoadedData) {
        return 0;
    }
    float fthreshold = gs_fTexturePctThreshold_Grass.Float();
    double dclamped = fthreshold < 0.0 ? 0.0 : fthreshold;
    fthreshold = dclamped;
    dclamped = fthreshold > 1.0 ? 0.9 : fthreshold;
    fthreshold = dclamped;
    int32_t iMaxGrassTypesPerTexure = gs_iMaxGrassTypesPerTexure_Grass.Int();
    TESObjectLAND::iGrassEvalSize = gs_iGrassDensityEvalSize_Grass.Int();
    if (TESObjectLAND::iGrassEvalSize != 1 &&
        TESObjectLAND::iGrassEvalSize != 2 &&
        TESObjectLAND::iGrassEvalSize != 4 &&
        TESObjectLAND::iGrassEvalSize != 8) {
        TESObjectLAND::iGrassEvalSize = 2;
    }
    for (int32_t i = 0; i < 4; ++i) {
        int32_t iParam = 0;
        for (int32_t j = TESObjectLAND::iGrassEvalSize; j < 16; j += 2 * TESObjectLAND::iGrassEvalSize) {
            for (int32_t k = TESObjectLAND::iGrassEvalSize; k < 16; k += 2 * TESObjectLAND::iGrassEvalSize) {
                TESGrassAreaParam** apParam = static_cast<TESGrassAreaParam**>(
                    ourMalloc(16 * sizeof(TESGrassAreaParam*))
                );
                ourMemset(apParam, 0, sizeof(TESGrassAreaParam*) * 16);
                int32_t aiVertIndex = j + 17 * k;
                if (pLoadedData->pDefQuadTexture[i]) {
                    BSSimpleList<TESGrass*>* pList = &pLoadedData->pDefQuadTexture[i]->TextureGrassList;
                    int32_t iType = 0;
                    while (pList && iParam < 16 && iType <= iMaxGrassTypesPerTexure) {
                        TESGrass* pGrass = pList->m_item;
                        if (pGrass) {
                            float fdensity;
                            TESGrassAreaParam* pAreaParam;
                            ++iType;
                            pAreaParam = new TESGrassAreaParam;
                            apParam[iParam] = pAreaParam;
                            ourMemset(apParam[iParam], 0, sizeof(TESGrassAreaParam));
                            apParam[iParam]->kGrassParam.pcGeomFilename = pGrass->GetModelPath();
                            apParam[iParam]->kGrassParam.iGrassID = pGrass->iFormID;
                            apParam[iParam]->kGrassParam.fWaveOffsetRange = TESGrass::GetWaveOffsetRange();
                            apParam[iParam]->kGrassParam.fWavePeriod = pGrass->GetWavePeriod();
                            apParam[iParam]->kGrassParam.fColorRange = pGrass->GetColorRange();
                            apParam[iParam]->kGrassParam.fPositionRange = pGrass->GetPositionRange();
                            apParam[iParam]->kGrassParam.fHeightRange = pGrass->GetHeightRange();
                            apParam[iParam]->kGrassParam.bVertexLighting = pGrass->GetVertexLighting();
                            apParam[iParam]->kGrassParam.bFitToSlope = pGrass->GetFitToSlope();
                            apParam[iParam]->kGrassParam.bUniformScaing = pGrass->GetUniformScaling();
                            float fgrassdensity = static_cast<double>(pGrass->GetDensity()) / 100.0;
                            if (static_cast<double>(fthreshold) < GetDefaultTexturePercent(i, aiVertIndex - 18)) {
                                fdensity = fgrassdensity;
                            }
                            else {
                                fdensity = 0.0f;
                            }
                            apParam[iParam]->pfDensity[0] = fdensity;
                            if (static_cast<double>(fthreshold) < GetDefaultTexturePercent(i, aiVertIndex - 17)) {
                                fdensity = fgrassdensity;
                            }
                            else {
                                fdensity = 0.0f;
                            }
                            apParam[iParam]->pfDensity[1] = fdensity;
                            if (static_cast<double>(fthreshold) < GetDefaultTexturePercent(i, aiVertIndex - 16)) {
                                fdensity = fgrassdensity;
                            }
                            else {
                                fdensity = 0.0f;
                            }
                            apParam[iParam]->pfDensity[2] = fdensity;
                            if (static_cast<double>(fthreshold) < GetDefaultTexturePercent(i, aiVertIndex - 1)) {
                                fdensity = fgrassdensity;
                            }
                            else {
                                fdensity = 0.0f;
                            }
                            apParam[iParam]->pfDensity[3] = fdensity;
                            if (static_cast<double>(fthreshold) < GetDefaultTexturePercent(i, aiVertIndex)) {
                                fdensity = fgrassdensity;
                            }
                            else {
                                fdensity = 0.0f;
                            }
                            apParam[iParam]->pfDensity[4] = fdensity;
                            if (static_cast<double>(fthreshold) < GetDefaultTexturePercent(i, aiVertIndex + 1)) {
                                fdensity = fgrassdensity;
                            }
                            else {
                                fdensity = 0.0f;
                            }
                            apParam[iParam]->pfDensity[5] = fdensity;
                            if (static_cast<double>(fthreshold) < GetDefaultTexturePercent(i, aiVertIndex + 16)) {
                                fdensity = fgrassdensity;
                            }
                            else {
                                fdensity = 0.0f;
                            }
                            apParam[iParam]->pfDensity[6] = fdensity;
                            if (static_cast<double>(fthreshold) < GetDefaultTexturePercent(i, aiVertIndex + 17)) {
                                fdensity = fgrassdensity;
                            }
                            else {
                                fdensity = 0.0f;
                            }
                            apParam[iParam]->pfDensity[7] = fdensity;
                            if (static_cast<double>(fthreshold) < GetDefaultTexturePercent(i, aiVertIndex + 18)) {
                                fdensity = fgrassdensity;
                            }
                            else {
                                fdensity = 0.0f;
                            }
                            apParam[iParam]->pfDensity[8] = fdensity;
                            float ftotaldensity = 0.0f;
                            for (float fDensity : apParam[iParam]->pfDensity) {
                                ftotaldensity = ftotaldensity + fDensity;
                            }
                            if (fthreshold > ftotaldensity / 9.0) {
                                for (float& fDensity : apParam[iParam]->pfDensity) {
                                    fDensity = 0.0f;
                                }
                            }
                            ++iParam;
                        }
                        pList = pList->m_pkNext;
                    }
                }
                for (int32_t ii = 0; ii < 6; ++ii) {
                    if (pLoadedData->pQuadTextureArray[i][ii] &&
                        pLoadedData->ppPercentArrays[i] &&
                        (pLoadedData->ppPercentArrays[i][aiVertIndex - 18][ii] > 0.10000000149011612 ||
                         pLoadedData->ppPercentArrays[i][aiVertIndex - 17][ii] > 0.10000000149011612 ||
                         pLoadedData->ppPercentArrays[i][aiVertIndex - 16][ii] > 0.10000000149011612 ||
                         pLoadedData->ppPercentArrays[i][aiVertIndex - 1][ii] > 0.10000000149011612 ||
                         pLoadedData->ppPercentArrays[i][aiVertIndex][ii] > 0.10000000149011612 ||
                         pLoadedData->ppPercentArrays[i][aiVertIndex + 1][ii] > 0.10000000149011612 ||
                         pLoadedData->ppPercentArrays[i][aiVertIndex + 16][ii] > 0.10000000149011612 ||
                         pLoadedData->ppPercentArrays[i][aiVertIndex + 17][ii] > 0.10000000149011612 ||
                         pLoadedData->ppPercentArrays[i][aiVertIndex + 18][ii] > 0.10000000149011612)) {
                        int32_t iType = 0;
                        for (BSSimpleList<TESGrass*>* pList = &pLoadedData->pQuadTextureArray[i][ii]->TextureGrassList;
                             pList && iParam < 16 && iType <= iMaxGrassTypesPerTexure;
                             pList = pList->m_pkNext) {
                            TESGrass* pGrass = pList->m_item;
                            if (pGrass) {
                                float fdensity;
                                TESGrassAreaParam* pAreaParam;
                                pAreaParam = new TESGrassAreaParam;
                                apParam[iParam] = pAreaParam;
                                ++iType;
                                ourMemset(apParam[iParam], 0, sizeof(TESGrassAreaParam));
                                ourMemset(apParam[iParam]->pfDensity, 0, sizeof(apParam[iParam]->pfDensity));
                                apParam[iParam]->kGrassParam.pcGeomFilename = pGrass->GetModelPath();
                                apParam[iParam]->kGrassParam.iGrassID = pGrass->iFormID;
                                apParam[iParam]->kGrassParam.fWaveOffsetRange = TESGrass::GetWaveOffsetRange();
                                apParam[iParam]->kGrassParam.fWavePeriod = pGrass->GetWavePeriod();
                                apParam[iParam]->kGrassParam.fColorRange = pGrass->GetColorRange();
                                apParam[iParam]->kGrassParam.fPositionRange = pGrass->GetPositionRange();
                                apParam[iParam]->kGrassParam.fHeightRange = pGrass->GetHeightRange();
                                apParam[iParam]->kGrassParam.bVertexLighting = pGrass->GetVertexLighting();
                                apParam[iParam]->kGrassParam.bFitToSlope = pGrass->GetFitToSlope();
                                apParam[iParam]->kGrassParam.bUniformScaing = pGrass->GetUniformScaling();
                                float fgrassdensity = static_cast<double>(pGrass->GetDensity()) / 100.0;
                                if (fthreshold <
                                    static_cast<double>(pLoadedData->ppPercentArrays[i][aiVertIndex - 16][ii])) {
                                    fdensity = fgrassdensity;
                                }
                                else {
                                    fdensity = 0.0f;
                                }
                                apParam[iParam]->pfDensity[2] = fdensity;
                                if (fthreshold <
                                    static_cast<double>(pLoadedData->ppPercentArrays[i][aiVertIndex - 17][ii])) {
                                    fdensity = fgrassdensity;
                                }
                                else {
                                    fdensity = 0.0f;
                                }
                                apParam[iParam]->pfDensity[1] = fdensity;
                                if (fthreshold <
                                    static_cast<double>(pLoadedData->ppPercentArrays[i][aiVertIndex - 18][ii])) {
                                    fdensity = fgrassdensity;
                                }
                                else {
                                    fdensity = 0.0f;
                                }
                                apParam[iParam]->pfDensity[0] = fdensity;
                                if (fthreshold <
                                    static_cast<double>(pLoadedData->ppPercentArrays[i][aiVertIndex + 1][ii])) {
                                    fdensity = fgrassdensity;
                                }
                                else {
                                    fdensity = 0.0f;
                                }
                                apParam[iParam]->pfDensity[5] = fdensity;
                                if (fthreshold <
                                    static_cast<double>(pLoadedData->ppPercentArrays[i][aiVertIndex][ii])) {
                                    fdensity = fgrassdensity;
                                }
                                else {
                                    fdensity = 0.0f;
                                }
                                apParam[iParam]->pfDensity[4] = fdensity;
                                if (fthreshold <
                                    static_cast<double>(pLoadedData->ppPercentArrays[i][aiVertIndex - 1][ii])) {
                                    fdensity = fgrassdensity;
                                }
                                else {
                                    fdensity = 0.0f;
                                }
                                apParam[iParam]->pfDensity[3] = fdensity;
                                if (fthreshold <
                                    static_cast<double>(pLoadedData->ppPercentArrays[i][aiVertIndex + 18][ii])) {
                                    fdensity = fgrassdensity;
                                }
                                else {
                                    fdensity = 0.0f;
                                }
                                apParam[iParam]->pfDensity[8] = fdensity;
                                if (fthreshold <
                                    static_cast<double>(pLoadedData->ppPercentArrays[i][aiVertIndex + 17][ii])) {
                                    fdensity = fgrassdensity;
                                }
                                else {
                                    fdensity = 0.0f;
                                }
                                apParam[iParam]->pfDensity[7] = fdensity;
                                if (fthreshold <
                                    static_cast<double>(pLoadedData->ppPercentArrays[i][aiVertIndex + 16][ii])) {
                                    fdensity = fgrassdensity;
                                }
                                else {
                                    fdensity = 0.0f;
                                }
                                apParam[iParam]->pfDensity[6] = fdensity;
                                float ftotaldensity = 0.0f;
                                for (float fDensity : apParam[iParam]->pfDensity) {
                                    ftotaldensity = ftotaldensity + fDensity;
                                }
                                if (fthreshold > ftotaldensity / 9.0) {
                                    for (float& fDensity : apParam[iParam]->pfDensity) {
                                        fDensity = 0.0f;
                                    }
                                }
                                ++iParam;
                            }
                        }
                    }
                }
                if (*apParam) {
                    pLoadedData->pmGrassMap[i].SetAt(aiVertIndex, apParam);
                }
                else {
                    ourFree(apParam);
                }
                iParam = 0;
            }
        }
    }
    return 1;
}

bool TESObjectLAND::ForceGoodNormals() {
    NiPoint3 akSavedNormals[8][4][289];
    LANDS8 L8;
    bool pbloaded[8];
    int32_t i;
    if (!IsLoaded() || GetLandRemapped()) {
        return false;
    }
    ourMemset(pbloaded, 0, 8u);
    FillLands8(&L8, true, pbloaded);
    TESObjectLAND** plandarray = &L8.pNWLand;
    for (i = 0; i < 8; ++i) {
        if (plandarray[i]) {
            for (int32_t j = 0; j < 4; ++j) {
                NiPoint3** ppnormals = plandarray[i]->GetNormalArray();
                ourMemcpy(akSavedNormals[i][j], ppnormals[j], sizeof(akSavedNormals[i][j]));
            }
            plandarray[i]->InvalidateNormals(nullptr);
        }
    }
    InvalidateNormals(nullptr);
    SetLandNormals(&L8, nullptr);
    for (i = 0; i < 4; ++i) {
        for (int32_t k = 0; k < 289; ++k) {
            pLoadedData->ppNormals[i][k].Unitize();
            char cnormaly = static_cast<int32_t>(pLoadedData->ppNormals[i][k].y * 127.0);
            int32_t inormalz = static_cast<int32_t>(pLoadedData->ppNormals[i][k].z * 127.0);
            pLoadedData->ppNormals[i][k].x = static_cast<double>(static_cast<char>(
                                                 static_cast<int32_t>(pLoadedData->ppNormals[i][k].x * 127.0)
                                             )) /
                127.0;
            pLoadedData->ppNormals[i][k].y = static_cast<double>(cnormaly) / 127.0;
            pLoadedData->ppNormals[i][k].z = static_cast<double>(static_cast<char>(inormalz)) / 127.0;
            pLoadedData->ppNormals[i][k].Unitize();
        }
    }
    for (i = 0; i < 8; ++i) {
        if (plandarray[i]) {
            for (int32_t m = 0; m < 4; ++m) {
                NiPoint3** ppnormals = plandarray[i]->GetNormalArray();
                ourMemcpy(ppnormals[m], akSavedNormals[i][m], sizeof(akSavedNormals[i][m]));
            }
            if (pbloaded[i]) {
                plandarray[i]->UnLoadVertices();
            }
        }
    }
    return true;
}

void TESObjectLAND::GetVertex(int32_t iblock, int32_t index, NiPoint3& vert) {
    float fHeight;
    if (HasVertices()) {
        if (pLoadedData->ppVertices && pLoadedData->ppVertices[iblock]) {
            vert = pLoadedData->ppVertices[iblock][index];
        }
        else if (pLoadedData->ppMesh && pLoadedData->ppMesh[iblock]) {
            NiGeometry* pgeom = static_cast<NiGeometry*>(pLoadedData->ppMesh[iblock]->GetAt(0));
            vert = pgeom->GetVertices()[index];
        }
        else {
            fHeight = GetDefaultWorldHeight();
            vert = NiPoint3(static_cast<float>(0.0), static_cast<float>(0.0), fHeight);
        }
        vert.z = vert.z + pLoadedData->fBaseHeight;
    }
    else {
        fHeight = GetDefaultWorldHeight();
        vert = NiPoint3(static_cast<float>(0.0), static_cast<float>(0.0), fHeight);
    }
}

void TESObjectLAND::GetNormal(int32_t iblock, int32_t index, NiPoint3& norm) {
    if (HasVertices()) {
        bool bset = false;
        if (pLoadedData->ppNormals && pLoadedData->ppNormals[iblock]) {
            norm = pLoadedData->ppNormals[iblock][index];
            bset = true;
        }
        else if (pLoadedData->ppMesh && pLoadedData->ppMesh[iblock]) {
            NiGeometry* shape = static_cast<NiGeometry*>(pLoadedData->ppMesh[iblock]->GetAt(0));
            if (shape->GetNormals()) {
                norm = shape->GetNormals()[index];
                bset = true;
            }
            else {
                NiGeometryData* pgeodata = shape->GetModelData();
                NiTConvertingStrideIterator<NiPoint3> kIterator;
                NiAdditionalGeometryData* pkAGD = pgeodata->GetAdditionalGeometryData();
                if (pkAGD && pkAGD->GetVBReady() && pgeodata->LockPackedVertexData(true)) {
                    pgeodata->GetNormalsIterator(*static_cast<NiGeometryDataIteratorNiPoint3*>(&kIterator));
                    kIterator.Get(index, norm);
                    pgeodata->UnlockPackedVertexData();
                    bset = true;
                }
            }
        }
        if (!bset) {
            norm = NiPoint3(0.0f, 0.0f, 1.0f);
        }
    }
    else {
        norm = NiPoint3(0.0f, 0.0f, 1.0f);
    }
}

// The height, normal, colour and DATA buffers are zeroed once, at entry, because GetChunkData writes only the chunk's
// bytes and they would otherwise be read uninitialised when a chunk is empty or short (malformed data).
bool TESObjectLAND::LoadVerticesIntoArrays() {
    FILE_TEX_DATA addtexdata;
    FILE_TEX_DATA texdata;
    FILE_HEIGHT_DATA kHeightData = {};
    OBJ_LAND tempdata = {};
    int8_t acNormals[3276] = {};
    uint8_t acColors[3272] = {};
    if (IsLoaded()) {
        return true;
    }
    int32_t ilastblock = -1;
    int32_t ilasttexindex = -1;
    bool bfailed = false;
    TESFile* pFile = nullptr;
    uint32_t ifindoffset = 0;
    if (GetLandRemapped()) {
        TESWorldSpace* pworld = GetWorldSpace();
        if (!pworld) {
            return true;
        }
        int32_t iCellY = GetDataY();
        int32_t iCellX = GetDataX();
        if (!pworld->FindLandDataInFile(iCellX, iCellY, pFile, ifindoffset)) {
            return true;
        }
    }
    else {
        if ((Data.iFlags & LAND_DATA_HEIGHTS_NORMALS) == 0 &&
            (Data.iFlags & LAND_DATA_COLORS) == 0 &&
            (Data.iFlags & LAND_DATA_TEXTURES) == 0) {
            return true;
        }
        pFile = GetFile(-1)->GetThreadSafeFile();
        if (!pFile->FindForm(this)) {
            bfailed = true;
        }
        if (!bfailed) {
            uint8_t formid = pFile->GetTESForm();
            bfailed = formid != LAND_ID;
        }
        if (!bfailed) {
            ifindoffset = pFile->GetOffset();
        }
    }
    if (bfailed) {
        const char* pfilename;
        MessageHandler::IncDisableWarningCount(false);
        if (pFile) {
            pfilename = pFile->GetFilename();
        }
        else {
            pfilename = "UNKNOWN";
        }
        int32_t iCellY = pParentCell->TESObjectCELL::GetDataY();
        int32_t iCellX = pParentCell->TESObjectCELL::GetDataX();
        Warning(
            "CELLS: Failed to load landscape data for LAND (%08X) in Cell (%i, %i) from file '%s'.",
            iFormID,
            iCellX,
            iCellY,
            pfilename
        );
        MessageHandler::IncDisableWarningCount(true);
        return false;
    }

    TESFile* pLandFile = pFile;
    if (pFile->SetOffset(ifindoffset)) {
        bool loadedhgt = false;
        bool loadednorms = false;
        bool badjusttextures = false;
        do {
            CHUNK_ID eChunk = pLandFile->GetTESChunk();
            if (eChunk > VHGT_ID) {
                if (eChunk == ATXT_ID) {
                    if ((Data.iFlags & LAND_DATA_TEXTURES) != 0) {
                        ourMemset(&addtexdata, 0, sizeof(FILE_TEX_DATA));
                        pLandFile->GetChunkData(&addtexdata, sizeof(FILE_TEX_DATA));
                        if (pLandFile->QEndian()) {
                            addtexdata.Endian();
                        }
                        if (addtexdata.sIndex > 5u) {
                            int32_t iblock = addtexdata.cBlock;
                            int32_t iindex = addtexdata.sIndex;
                            int32_t iy = GetDataY();
                            Warning(
                                "CELLS: Land (%i, %i) clamping invalid index %i for block %i.",
                                GetDataX(),
                                iy,
                                iindex,
                                iblock
                            );
                            addtexdata.sIndex = 5;
                            badjusttextures = true;
                        }
                        TESLandTexture* ptex = nullptr;
                        if (addtexdata.iTextureID) {
                            TESForm::AddCompileIndex(addtexdata.iTextureID, pLandFile);
                            TESForm* pForm = TESForm::GetFormByNumericID(addtexdata.iTextureID);
                            ptex = dynamic_cast<TESLandTexture*>(pForm);
                            if (!ptex) {
                                int32_t iblock = addtexdata.cBlock;
                                uint32_t itextureid = addtexdata.iTextureID;
                                int32_t iy = GetDataY();
                                Warning(
                                    "TEXTURES: Land (%i, %i) unable to find additional texture ID (%08X) for "
                                    "block %i.",
                                    GetDataX(),
                                    iy,
                                    itextureid,
                                    iblock
                                );
                                badjusttextures = true;
                            }
                        }
                        else {
                            ptex = TESObjectLAND::GetDefaultTexture();
                        }
                        this->pLoadedData->pQuadTextureArray[addtexdata.cBlock][addtexdata.sIndex] = ptex;
                        if (ptex) {
                            if (!spQueuedTextures.m_pObject) {
                                QueuedFile* pQueuedFile;
                                pQueuedFile = new QueuedFile(IO_TASK_PRIORITY_VERY_LOW);
                                spQueuedTextures = pQueuedFile;
                            }
                            ptex->QueueTexture(IO_TASK_PRIORITY_VERY_LOW, spQueuedTextures.m_pObject);
                        }
                        ilastblock = addtexdata.cBlock;
                        ilasttexindex = addtexdata.sIndex;
                    }
                }
                else if (eChunk == BTXT_ID) {
                    if ((Data.iFlags & LAND_DATA_TEXTURES) != 0) {
                        ourMemset(&texdata, 0, sizeof(FILE_TEX_DATA));
                        pLandFile->GetChunkData(&texdata, sizeof(FILE_TEX_DATA));
                        if (pLandFile->QEndian()) {
                            texdata.Endian();
                        }
                        TESForm::AddCompileIndex(texdata.iTextureID, pLandFile);
                        TESForm* pForm = TESForm::GetFormByNumericID(texdata.iTextureID);
                        this->pLoadedData->pDefQuadTexture[texdata.cBlock] = dynamic_cast<TESLandTexture*>(pForm);
                        if (this->pLoadedData->pDefQuadTexture[texdata.cBlock]) {
                            TESLandTexture* ptex = this->pLoadedData->pDefQuadTexture[texdata.cBlock];
                            if (!spQueuedTextures.m_pObject) {
                                QueuedFile* pQueuedFile;
                                pQueuedFile = new QueuedFile(IO_TASK_PRIORITY_VERY_LOW);
                                spQueuedTextures = pQueuedFile;
                            }
                            ptex->QueueTexture(IO_TASK_PRIORITY_VERY_LOW, spQueuedTextures.m_pObject);
                        }
                        else {
                            int32_t iblock = texdata.cBlock;
                            uint32_t itextureid = texdata.iTextureID;
                            int32_t iy = GetDataY();
                            Warning(
                                "TEXTURES: Land (%i, %i) unable to find base texture ID (%08X) for block "
                                "%i.",
                                GetDataX(),
                                iy,
                                itextureid,
                                iblock
                            );
                            badjusttextures = true;
                        }
                    }
                }
                else if (eChunk == VTXT_ID && (Data.iFlags & LAND_DATA_TEXTURES) != 0) {
                    if (ilastblock >= 0 && ilasttexindex >= 0) {
                        uint32_t ifilesize = pLandFile->GetChunkSize();
                        if (ifilesize % 8) {
                            const char* pfilename = pLandFile->GetFilename();
                            int32_t iy = GetDataY();
                            Warning(
                                "TEXTURES: Land (%i, %i) found unrecognized vertex texture data in "
                                "file %s.",
                                GetDataX(),
                                iy,
                                pfilename
                            );
                            badjusttextures = true;
                        }
                        else {
                            FILE_TEX_VERT_DATA* pvertdata = static_cast<FILE_TEX_VERT_DATA*>(ourMalloc(ifilesize));
                            pLandFile->GetChunkData(pvertdata, ifilesize);
                            uint32_t ivertcount = ifilesize >> 3;
                            for (uint32_t i = 0; i < ivertcount; ++i) {
                                if (pLandFile->QEndian()) {
                                    pvertdata[i].Endian();
                                }
                                SetTexturePercent(
                                    ilastblock,
                                    pvertdata[i].iVertIndex,
                                    ilasttexindex,
                                    pvertdata[i].fPercent
                                );
                            }
                            ourFree(pvertdata);
                            pvertdata = nullptr;
                        }
                    }
                    ilastblock = -1;
                    ilasttexindex = -1;
                }
            }
            else {
                int32_t j;
                int32_t i;
                if (eChunk == VHGT_ID) {
                    if ((Data.iFlags & LAND_DATA_HEIGHTS_NORMALS) != 0) {
                        float afHeights[1089];
                        pLandFile->GetChunkData(&kHeightData, sizeof(FILE_HEIGHT_DATA));
                        if (pLandFile->QEndian()) {
                            kHeightData.Endian();
                        }
                        NiPoint2 kExtents(std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
                        this->pLoadedData->HeightExtents.x = kExtents.x;
                        this->pLoadedData->HeightExtents.y = kExtents.y;
                        float lastheight = kHeightData.fOffset;
                        // Each step adds to the vertex before it in the row; a row's first vertex steps from the
                        // first vertex of the row before.
                        for (i = 0; i < 1089; ++i) {
                            afHeights[i] = static_cast<double>(kHeightData.cHeightData[i]) + lastheight;
                            if ((i + 1) % 33) {
                                lastheight = afHeights[i];
                            }
                            else {
                                lastheight = afHeights[i - 32];
                            }
                        }
                        for (i = 0; i < 4; ++i) {
                            int32_t orig = 16 * (i % 2) + 528 * (i / 2);
                            for (j = 0; j < 289; ++j) {
                                int32_t icolumn = j % 17;
                                int32_t irow = j / 17;
                                int32_t curvert = j % 17 + (orig + 33 * (j / 17));
                                float fvertheight = static_cast<float>(8 * FloatToInt(afHeights[curvert]));
                                this->pLoadedData->ppVertices[i][j].x = static_cast<double>(icolumn << 7) +
                                    TESObjectLAND::pXOffsetBLOCK[i];
                                this->pLoadedData->ppVertices[i][j].y = static_cast<double>(irow << 7) +
                                    TESObjectLAND::pYOffsetBLOCK[i];
                                this->pLoadedData->ppVertices[i][j].z = fvertheight;
                                if (this->pLoadedData->HeightExtents.x > static_cast<double>(fvertheight)) {
                                    this->pLoadedData->HeightExtents.x = fvertheight;
                                }
                                else if (this->pLoadedData->HeightExtents.y < static_cast<double>(fvertheight)) {
                                    this->pLoadedData->HeightExtents.y = fvertheight;
                                }
                            }
                        }
                        this->pLoadedData->fBaseHeight = (this->pLoadedData->HeightExtents.x +
                                                          this->pLoadedData->HeightExtents.y) *
                            0.5;
                        for (i = 0; i < 4; ++i) {
                            for (j = 0; j < 289; ++j) {
                                this->pLoadedData->ppVertices[i][j].z = this->pLoadedData->ppVertices[i][j].z -
                                    this->pLoadedData->fBaseHeight;
                            }
                        }
                        loadedhgt = true;
                    }
                }
                else if (eChunk > VNML_ID) {
                    if (eChunk == VCLR_ID && (Data.iFlags & LAND_DATA_COLORS) != 0) {
                        pLandFile->GetChunkData(acColors, 3267u);
                        for (i = 0; i < 4; ++i) {
                            int32_t orig = 16 * (i % 2) + 528 * (i / 2);
                            for (j = 0; j < 289; ++j) {
                                int32_t curvert = j % 17 + (orig + 33 * (j / 17));
                                float fB = static_cast<double>(acColors[3 * curvert + 2]) / 255.0;
                                float fG = static_cast<double>(acColors[3 * curvert + 1]) / 255.0;
                                float fR = static_cast<double>(acColors[3 * curvert]) / 255.0;
                                NiColorA kColor(fR, fG, fB, 1.0f);
                                NiColorA* pColor = &this->pLoadedData->ppColorsA[i][j];
                                pColor->r = kColor.r;
                                pColor->g = kColor.g;
                                pColor->b = kColor.b;
                                pColor->a = kColor.a;
                            }
                        }
                    }
                }
                else {
                    switch (eChunk) {
                        case VNML_ID: {
                            if ((Data.iFlags & LAND_DATA_HEIGHTS_NORMALS) != 0) {
                                pLandFile->GetChunkData(acNormals, 3267u);
                                for (i = 0; i < 4; ++i) {
                                    int32_t orig = 16 * (i % 2) + 528 * (i / 2);
                                    for (j = 0; j < 289; ++j) {
                                        int32_t curvert = j % 17 + (orig + 33 * (j / 17));
                                        this->pLoadedData->ppNormals[i][j].x = static_cast<double>(
                                                                                   acNormals[3 * curvert]
                                                                               ) /
                                            127.0;
                                        this->pLoadedData->ppNormals[i][j].y = static_cast<double>(
                                                                                   acNormals[3 * curvert + 1]
                                                                               ) /
                                            127.0;
                                        this->pLoadedData->ppNormals[i][j].z = static_cast<double>(
                                                                                   acNormals[3 * curvert + 2]
                                                                               ) /
                                            127.0;
                                        this->pLoadedData->ppNormals[i][j].Unitize();
                                    }
                                }
                                loadednorms = true;
                            }
                            break;
                        }
                        case DATA_ID: {
                            // A remapped land keeps its own flags but for the three data bits, taken from the
                            // file.
                            if (GetLandRemapped()) {
                                pLandFile->GetChunkData(&tempdata, sizeof(OBJ_LAND));
                                if (pLandFile->QEndian()) {
                                    tempdata.Endian();
                                }
                                tempdata.iFlags &= LAND_DATA_HEIGHTS_NORMALS | LAND_DATA_COLORS | LAND_DATA_TEXTURES;
                                Data.iFlags &= ~(LAND_DATA_HEIGHTS_NORMALS | LAND_DATA_COLORS | LAND_DATA_TEXTURES);
                                Data.iFlags |= tempdata.iFlags;
                            }
                            break;
                        }
                        case MPCD_ID: {
                            uint32_t ibuffersize = pLandFile->GetChunkSize();
                            char* pbuffer = static_cast<char*>(ourMalloc(ibuffersize));
                            pLandFile->GetChunkData(pbuffer, ibuffersize);
                            if (this->pLoadedData->pMoppCode) {
                                this->pLoadedData->pMoppCode->removeReference();
                                this->pLoadedData->pMoppCode = nullptr;
                            }
                            ReadMoppCodeFromBuffer(pbuffer, ibuffersize, &this->pLoadedData->pMoppCode);
                            if (this->pLoadedData->pMoppCode) {
                                this->pLoadedData->pMoppCode->addReference();
                                SetHasMopp(1);
                            }
                            ourFree(pbuffer);
                            break;
                        }
                    }
                }
            }
        }
        while (pLandFile->NextChunk());
        if (badjusttextures) {
            AdjustTextureArrays();
        }
        if (loadedhgt && !loadednorms) {
            const char* pfilename = pLandFile->GetFilename();
            int32_t iy = GetDataY();
            Warning(
                "CELLS: Land for cell (%i, %i) in file '%s' does not contain Normal Data.",
                GetDataX(),
                iy,
                pfilename
            );
        }
        return true;
    }

    return false;
}

void TESObjectLAND::Detach3D() {
    spQueuedTextures = static_cast<QueuedFile*>(nullptr);
    if (pLoadedData) {
        if (pLoadedData->ppMesh) {
            bhkWorld* phw;
            if (pParentCell) {
                phw = pParentCell->GetbhkWorld();
            }
            else {
                phw = nullptr;
            }
            if (phw && pLoadedData->spLandRB != nullptr) {
                phw->RemoveBody(pLoadedData->spLandRB.m_pObject);
            }

            pLoadedData->spLandRB = static_cast<bhkRigidBody*>(nullptr);
            for (int32_t iblock = 0; iblock < 4; ++iblock) {
                if (pLoadedData->ppMesh[iblock]) {
                    pLoadedData->ppVertices[iblock] = nullptr;
                    pLoadedData->ppColorsA[iblock] = nullptr;
                    delete[] pLoadedData->ppNormals[iblock];
                    pLoadedData->ppNormals[iblock] = nullptr;
                    NiNode* pMesh = pLoadedData->ppMesh[iblock];
                    BSShaderManager::GetShadowSceneNode(0)->RemoveObject(pMesh);
                    pLoadedData->ppMesh[iblock]->DetachChildAt(0u);
                }
                pLoadedData->ppMesh[iblock] = nullptr;
            }
            ourFree(pLoadedData->ppMesh);
            pLoadedData->ppMesh = nullptr;
        }
        if (pLoadedData->spBorder.m_pObject) {
            NiNode* pParent = pLoadedData->spBorder->m_pkParent;
            if (pParent) {
                pParent->DetachChild(pLoadedData->spBorder.m_pObject);
            }
            pLoadedData->spBorder = static_cast<NiLines*>(nullptr);
        }
    }
}

bool TESObjectLAND::ReadMoppCodeFromBuffer(char* apBuffer, uint32_t aiBufferSize, hkpMoppCode** apOutCode) {
    bool bretval = false;
    if (apBuffer && aiBufferSize && apOutCode) {
        hkpMoppCode* pcode;
        hkpMoppCode* pMemory = static_cast<hkpMoppCode*>(hkReferencedObject::newObject(sizeof(hkpMoppCode)));
        if (pMemory) {
            pcode = ::new (pMemory) hkpMoppCode;
        }
        else {
            pcode = nullptr;
        }
        *apOutCode = pcode;
        if (aiBufferSize > 16) {
            ourMemcpy(&pcode->m_info, apBuffer, sizeof(pcode->m_info));
            // The remaining byte count is passed as is: hkArray::setSize takes Havok's int.
            pcode->m_data.setSize(static_cast<int32_t>(aiBufferSize - 16));
            ourMemcpy(pcode->m_data.m_data, apBuffer + 16, aiBufferSize - 16);
            bretval = true;
        }
        if (!bretval) {
            if (pcode) {
                delete pcode;
            }
            *apOutCode = nullptr;
        }
    }

    return bretval;
}

TESLandTexture* TESObjectLAND::GetMainTexture(uint8_t acBlock, uint16_t asVertIndex) {
    if (acBlock < 4u && asVertIndex < 289u) {
        float fmaxpercent = -std::numeric_limits<float>::max();
        uint32_t uimaintex = 0;
        for (uint32_t i = 0; i < 6; ++i) {
            float fpercent = GetTexturePercent(acBlock, asVertIndex, i);
            if (fmaxpercent < static_cast<double>(fpercent)) {
                fmaxpercent = fpercent;
                uimaintex = i;
            }
        }
        if (uimaintex) {
            return GetQuadTexture(acBlock, uimaintex - 1);
        }

        return GetDefaultTexture(acBlock);
    }
    return nullptr;
}

void TESObjectLAND::SetNewDefaultTexture(uint8_t acBlock, uint16_t asNewDefaultIndex) {
    if (acBlock < 4u && asNewDefaultIndex < 6u && pLoadedData) {
        TESLandTexture* polddefault = pLoadedData->pDefQuadTexture[acBlock];
        pLoadedData->pDefQuadTexture[acBlock] = pLoadedData->pQuadTextureArray[acBlock][asNewDefaultIndex];
        pLoadedData->pQuadTextureArray[acBlock][asNewDefaultIndex] = polddefault;
        if (pLoadedData->ppPercentArrays[acBlock]) {
            for (int32_t i = 0; i < 289; ++i) {
                float ftotal = 0.0;
                for (uint32_t j = 0; j < 6; ++j) {
                    ftotal = GetTexturePercent(acBlock, i, j) + ftotal;
                }
                float fdefault = 1.0 - ftotal;
                if (fdefault < 0.0) {
                    fdefault = 0.0;
                }
                pLoadedData->ppPercentArrays[acBlock][i][asNewDefaultIndex] = fdefault;
            }
        }
    }
}

TESGrassAreaParam** TESObjectLAND::GetGrassParameters(int32_t aiBlock, int32_t aiVert) {
    TESGrassAreaParam** pparam;
    if (!pLoadedData || aiBlock >= 4 || aiVert + 18 >= 289 || aiVert - 18 < 0) {
        return nullptr;
    }
    // The grass map is keyed by unsigned int; the vertex index goes in as its key.
    if (pLoadedData->pmGrassMap[aiBlock].GetAt(static_cast<uint32_t>(aiVert), pparam)) {
        return pparam;
    }
    return nullptr;
}

NiPoint3** TESObjectLAND::GetNormalArray() {
    if (pLoadedData) {
        return pLoadedData->ppNormals;
    }

    return nullptr;
}

void TESObjectLAND::InvalidateNormals(tagRECT* apInvalidateRect) {
    if (IsLoaded()) {
        for (int32_t i = 0; i < 4; ++i) {
            for (int32_t k = 0; k < 289; ++k) {
                if (!apInvalidateRect || VertexInRect(apInvalidateRect, pLoadedData->ppVertices[i][k])) {
                    pLoadedData->ppNormals[i][k] = NiPoint3::ZERO;
                    pLoadedData->ppNormalsSet[i][k] = false;
                }
            }
            uint16_t* pTriList = TESObjectLAND::pDefTriListBLOCK;
            for (uint16_t j = 0; j < 512u; ++j) {
                NiPoint3 e1;
                NiPoint3 normal;
                NiPoint3 e2;
                uint16_t i0 = *pTriList++;
                uint16_t i1 = *pTriList++;
                uint16_t i2 = *pTriList++;
                e1 = pLoadedData->ppVertices[i][i1] - pLoadedData->ppVertices[i][i0];
                e2 = pLoadedData->ppVertices[i][i2] - pLoadedData->ppVertices[i][i1];
                normal = e1.Cross(e2);
                NiPoint3::UnitizeVector(normal);
                if (!apInvalidateRect || VertexInRect(apInvalidateRect, pLoadedData->ppVertices[i][i0])) {
                    pLoadedData->ppNormals[i][i0] += normal;
                }
                if (!apInvalidateRect || VertexInRect(apInvalidateRect, pLoadedData->ppVertices[i][i1])) {
                    pLoadedData->ppNormals[i][i1] += normal;
                }
                if (!apInvalidateRect || VertexInRect(apInvalidateRect, pLoadedData->ppVertices[i][i2])) {
                    pLoadedData->ppNormals[i][i2] += normal;
                }
            }
            NiPoint3::UnitizeVectors(pLoadedData->ppNormals[i], 289u, 12u);
        }
    }
}

// The 16-bit vertex count is zero-extended into ComputeFromData's int.
void TESObjectLAND::UpdateMesh(bool updatescene, LANDS8* pL8) {
    if (pLoadedData->ppMesh) {
        for (int32_t iblock = 0; iblock < 4; ++iblock) {
            NiNode* pNode = GetQuadLandNode(iblock);
            if (pNode) {
                if (updatescene) {
                    NiGeometry* ptrigeom;
                    NiAVObject* pobj = pNode->GetAtChecked(0);
                    if (pobj) {
                        ptrigeom = pobj->IsGeometry();
                    }
                    else {
                        ptrigeom = nullptr;
                    }
                    if (ptrigeom) {
                        uint16_t usvertcount = ptrigeom->GetVertexCount();
                        NiPoint3* pverts = ptrigeom->GetVertices();
                        ptrigeom->GetModelBound().ComputeFromData(usvertcount, pverts);
                        ptrigeom->GetModelData()->MarkAsChanged(15);
                        NiShadeProperty* pprop = static_cast<NiShadeProperty*>(ptrigeom->GetProperty(3));
                        int32_t ipplighting = pprop &&
                            pprop->iShaderPropertyType >= 8 &&
                            pprop->iShaderPropertyType <= 12;
                        BSShaderPPLightingProperty* pshprop = nullptr;
                        if (ipplighting) {
                            pshprop = static_cast<BSShaderPPLightingProperty*>(pprop);
                        }
                        if (pshprop) {
                            uint8_t ucexp1;
                            uint8_t ucexp2;
                            uint8_t ucexp3;
                            uint8_t ucexp4;
                            uint8_t ucexp5;
                            uint8_t ucexp6;
                            NiPoint3* pnormals = ptrigeom->GetModelData()->m_pkNormal;
                            ourMemcpy(pnormals, pLoadedData->ppNormals[iblock], sizeof(NiPoint3) * 289);
                            ptrigeom->GetModelData()->StoreNormalBinormalTangent(NiGeometryData::NBT_METHOD_NONE);
                            pshprop->SetupGeometry(ptrigeom);
                            TESLandTexture* pQuadTex = pLoadedData->pDefQuadTexture[iblock];
                            if (pQuadTex && pQuadTex->pTextureSet) {
                                BSTextureSet* pTextureSet = pQuadTex->pTextureSet
                                                                ->BGSTextureSet::GetAsShaderTextureSet();
                                pshprop->SetLandscapeTextureSet(0, pTextureSet);
                            }
                            else {
                                TESLandTexture* pDefault = TESObjectLAND::GetDefaultTexture();
                                BSTextureSet* pTextureSet = pDefault->pTextureSet->GetAsShaderTextureSet();
                                pshprop->SetLandscapeTextureSet(0, pTextureSet);
                            }
                            for (int32_t itex = 1; itex < 7; ++itex) {
                                pQuadTex = pLoadedData->pQuadTextureArray[iblock][itex - 1];
                                if (pQuadTex && pQuadTex->pTextureSet) {
                                    BSTextureSet* pTextureSet = pQuadTex->pTextureSet
                                                                    ->BGSTextureSet::GetAsShaderTextureSet();
                                    pshprop->SetLandscapeTextureSet(static_cast<uint32_t>(itex), pTextureSet);
                                }
                                else {
                                    pshprop->SetLandscapeTextureSet(static_cast<uint32_t>(itex), nullptr);
                                }
                            }
                            if (pLoadedData->pQuadTextureArray[iblock][5]) {
                                ucexp6 = pLoadedData->pQuadTextureArray[iblock][5]->GetSpecularExponent();
                            }
                            else {
                                ucexp6 = 0;
                            }
                            if (pLoadedData->pQuadTextureArray[iblock][4]) {
                                ucexp5 = pLoadedData->pQuadTextureArray[iblock][4]->GetSpecularExponent();
                            }
                            else {
                                ucexp5 = 0;
                            }
                            if (pLoadedData->pQuadTextureArray[iblock][3]) {
                                ucexp4 = pLoadedData->pQuadTextureArray[iblock][3]->GetSpecularExponent();
                            }
                            else {
                                ucexp4 = 0;
                            }
                            if (pLoadedData->pQuadTextureArray[iblock][2]) {
                                ucexp3 = pLoadedData->pQuadTextureArray[iblock][2]->GetSpecularExponent();
                            }
                            else {
                                ucexp3 = 0;
                            }
                            if (pLoadedData->pQuadTextureArray[iblock][1]) {
                                ucexp2 = pLoadedData->pQuadTextureArray[iblock][1]->GetSpecularExponent();
                            }
                            else {
                                ucexp2 = 0;
                            }
                            if (pLoadedData->pQuadTextureArray[iblock][0]) {
                                ucexp1 = pLoadedData->pQuadTextureArray[iblock][0]->GetSpecularExponent();
                            }
                            else {
                                ucexp1 = 0;
                            }
                            if (pLoadedData->pDefQuadTexture[iblock]) {
                                uint8_t ucexp0 = pLoadedData->pDefQuadTexture[iblock]->GetSpecularExponent();
                                pshprop->SetLandscapeSpecularExponents(
                                    ucexp0,
                                    ucexp1,
                                    ucexp2,
                                    ucexp3,
                                    ucexp4,
                                    ucexp5,
                                    ucexp6,
                                    0,
                                    0,
                                    0
                                );
                            }
                            else {
                                pshprop->SetLandscapeSpecularExponents(
                                    0,
                                    ucexp1,
                                    ucexp2,
                                    ucexp3,
                                    ucexp4,
                                    ucexp5,
                                    ucexp6,
                                    0,
                                    0,
                                    0
                                );
                            }
                            pshprop->FreeRenderPasses();
                        }
                    }
                }
                NiUpdateData UpdateData(0.0f, false, false);
                pNode->Update(UpdateData);
                if (updatescene) {
                    pNode->UpdateProperties();
                }
            }
        }
        if (updatescene) {
            DrawLandBorder(TES::pInstance->GetShowLandBorders());
        }
    }
    else {
        CreateLandscape(nullptr);
        Attach3D();
    }
}

void TESObjectLAND::FillLands8(LANDS8* pL8, bool abForceLoad, bool* apLoaded) {
    if (pL8) {
        TESWorldSpace* pWorld = GetWorldSpace();
        int32_t iCellY = GetDataY() + 1;
        int32_t iCellX = GetDataX();
        TESObjectCELL* pCell = pDataHandler->GetCellFromCellCoord(iCellX - 1, iCellY, pWorld, false);
        if (pCell && pCell->GetLand()->HasVertices()) {
            pL8->pNWLand = pCell->GetLand();
        }
        else if (pCell && !pCell->GetLand()->HasVertices() && abForceLoad && apLoaded) {
            pL8->pNWLand = pCell->GetLand();
            pL8->pNWLand->LoadVertices(false);
            apLoaded[0] = true;
        }
        else {
            pL8->pNWLand = nullptr;
        }
        pWorld = GetWorldSpace();
        iCellY = GetDataY() + 1;
        iCellX = GetDataX();
        pCell = pDataHandler->GetCellFromCellCoord(iCellX, iCellY, pWorld, false);
        if (pCell && pCell->GetLand()->HasVertices()) {
            pL8->pNLand = pCell->GetLand();
        }
        else if (pCell && !pCell->GetLand()->HasVertices() && abForceLoad && apLoaded) {
            pL8->pNLand = pCell->GetLand();
            pL8->pNLand->LoadVertices(false);
            apLoaded[1] = true;
        }
        else {
            pL8->pNLand = nullptr;
        }
        pWorld = GetWorldSpace();
        iCellY = GetDataY() + 1;
        iCellX = GetDataX();
        pCell = pDataHandler->GetCellFromCellCoord(iCellX + 1, iCellY, pWorld, false);
        if (pCell && pCell->GetLand()->HasVertices()) {
            pL8->pNELand = pCell->GetLand();
        }
        else if (pCell && !pCell->GetLand()->HasVertices() && abForceLoad && apLoaded) {
            pL8->pNELand = pCell->GetLand();
            pL8->pNELand->LoadVertices(false);
            apLoaded[2] = true;
        }
        else {
            pL8->pNELand = nullptr;
        }
        pWorld = GetWorldSpace();
        iCellY = GetDataY();
        iCellX = GetDataX();
        pCell = pDataHandler->GetCellFromCellCoord(iCellX - 1, iCellY, pWorld, false);
        if (pCell && pCell->GetLand()->HasVertices()) {
            pL8->pWLand = pCell->GetLand();
        }
        else if (pCell && !pCell->GetLand()->HasVertices() && abForceLoad && apLoaded) {
            pL8->pWLand = pCell->GetLand();
            pL8->pWLand->LoadVertices(false);
            apLoaded[3] = true;
        }
        else {
            pL8->pWLand = nullptr;
        }
        pWorld = GetWorldSpace();
        iCellY = GetDataY();
        iCellX = GetDataX();
        pCell = pDataHandler->GetCellFromCellCoord(iCellX + 1, iCellY, pWorld, false);
        if (pCell && pCell->GetLand()->HasVertices()) {
            pL8->pELand = pCell->GetLand();
        }
        else if (pCell && !pCell->GetLand()->HasVertices() && abForceLoad && apLoaded) {
            pL8->pELand = pCell->GetLand();
            pL8->pELand->LoadVertices(false);
            apLoaded[4] = true;
        }
        else {
            pL8->pELand = nullptr;
        }
        pWorld = GetWorldSpace();
        iCellY = GetDataY() - 1;
        iCellX = GetDataX();
        pCell = pDataHandler->GetCellFromCellCoord(iCellX - 1, iCellY, pWorld, false);
        if (pCell && pCell->GetLand()->HasVertices()) {
            pL8->pSWLand = pCell->GetLand();
        }
        else if (pCell && !pCell->GetLand()->HasVertices() && abForceLoad && apLoaded) {
            pL8->pSWLand = pCell->GetLand();
            pL8->pSWLand->LoadVertices(false);
            apLoaded[5] = true;
        }
        else {
            pL8->pSWLand = nullptr;
        }
        pWorld = GetWorldSpace();
        iCellY = GetDataY() - 1;
        iCellX = GetDataX();
        pCell = pDataHandler->GetCellFromCellCoord(iCellX, iCellY, pWorld, false);
        if (pCell && pCell->GetLand()->HasVertices()) {
            pL8->pSLand = pCell->GetLand();
        }
        else if (pCell && !pCell->GetLand()->HasVertices() && abForceLoad && apLoaded) {
            pL8->pSLand = pCell->GetLand();
            pL8->pSLand->LoadVertices(false);
            apLoaded[6] = true;
        }
        else {
            pL8->pSLand = nullptr;
        }
        pWorld = GetWorldSpace();
        iCellY = GetDataY() - 1;
        iCellX = GetDataX();
        pCell = pDataHandler->GetCellFromCellCoord(iCellX + 1, iCellY, pWorld, false);
        if (pCell && pCell->GetLand()->HasVertices()) {
            pL8->pSELand = pCell->GetLand();
        }
        else if (pCell && !pCell->GetLand()->HasVertices() && abForceLoad && apLoaded) {
            pL8->pSELand = pCell->GetLand();
            pL8->pSELand->LoadVertices(false);
            apLoaded[7] = true;
        }
        else {
            pL8->pSELand = nullptr;
        }
    }
}

void TESObjectLAND::SetLandNormals(LANDS8* pL8, tagRECT* apCalcRect) {
    LANDS8 klands;
    if (IsLoaded()) {
        if (!pL8) {
            pL8 = &klands;
            FillLands8(&klands, false, nullptr);
        }
        for (int32_t i = 0; i < 4; ++i) {
            for (int32_t j = 0; j < 289; ++j) {
                if ((j < 17 || j > 272 || !(j % 17) || !((j + 1) % 17)) &&
                    (!apCalcRect || VertexInRect(apCalcRect, pLoadedData->ppVertices[i][j]))) {
                    CalculateEdgeNormal(i, j, pLoadedData->ppNormals[i][j], pL8);
                }
            }
        }
        if (pL8->pNELand &&
            pL8->pNLand &&
            pL8->pNWLand &&
            pL8->pSELand &&
            pL8->pSLand &&
            pL8->pSWLand &&
            pL8->pELand &&
            pL8->pWLand) {
            SetGoodNormals(true);
        }
    }
}

// An edge vertex's normal averages its own with the matching vertex's of each neighbouring cell (from LANDS8) and
// each neighbouring block of this land. A neighbour whose normal is already final is taken as it is. A neighbouring
// cell without land, or whose GetLandNormal fails, adds the up vector.
void TESObjectLAND::CalculateEdgeNormal(int32_t iblock, int32_t vertidx, NiPoint3& normal, LANDS8* pL8) {
    if (pLoadedData->ppNormalsSet[iblock][vertidx] || !pL8) {
        return;
    }
    NiPoint3 offset;
    float fY = GetWorldCellY() + 2048.0;
    float fX = GetWorldCellX() + 2048.0;
    offset = NiPoint3(fX, fY, 0.0f);
    NiPoint3 scaledvert = pLoadedData->ppVertices[iblock][vertidx] + offset;
    bool blockdown = false;
    bool blockup = false;
    bool blockleft = false;
    bool blockright = false;
    bool celldown = false;
    bool cellup = false;
    bool cellleft = false;
    bool cellright = false;
    NiPoint3 tempnormal;
    NiPoint3 retnormal;
    float divisor = 1.0f;
    TESObjectLAND* pland;
    retnormal = pLoadedData->ppNormals[iblock][vertidx];

    // The neighbouring cells across the edges.
    if (iblock < 2 && vertidx < 17) {
        celldown = true;
        pland = pL8->pSLand;
        NiPoint3 pfacenorm;
        if (!pland || !pland->GetLandNormal(scaledvert, tempnormal, pfacenorm)) {
            tempnormal = NiPoint3(0.0f, 0.0f, 1.0f);
        }
        if (pland && pland->IsNormalSet(scaledvert)) {
            normal = tempnormal;
            pLoadedData->ppNormalsSet[iblock][vertidx] = true;
            return;
        }
        retnormal += tempnormal;
        divisor = divisor + 1.0;
    }
    else if (iblock >= 2 && vertidx >= 272) {
        cellup = true;
        pland = pL8->pNLand;
        NiPoint3 pfacenorm;
        if (!pland || !pland->GetLandNormal(scaledvert, tempnormal, pfacenorm)) {
            tempnormal = NiPoint3(0.0f, 0.0f, 1.0f);
        }
        if (pland && pland->IsNormalSet(scaledvert)) {
            normal = tempnormal;
            pLoadedData->ppNormalsSet[iblock][vertidx] = true;
            return;
        }
        retnormal += tempnormal;
        divisor = divisor + 1.0;
    }
    if (iblock % 2 == 0 && vertidx % 17 == 0) {
        cellleft = true;
        pland = pL8->pWLand;
        NiPoint3 pfacenorm;
        if (!pland || !pland->GetLandNormal(scaledvert, tempnormal, pfacenorm)) {
            tempnormal = NiPoint3(0.0f, 0.0f, 1.0f);
        }
        if (pland && pland->IsNormalSet(scaledvert)) {
            normal = tempnormal;
            pLoadedData->ppNormalsSet[iblock][vertidx] = true;
            return;
        }
        retnormal += tempnormal;
        divisor = divisor + 1.0;
    }
    else if ((iblock + 1) % 2 == 0 && (vertidx + 1) % 17 == 0) {
        cellright = true;
        pland = pL8->pELand;
        NiPoint3 pfacenorm;
        if (!pland || !pland->GetLandNormal(scaledvert, tempnormal, pfacenorm)) {
            tempnormal = NiPoint3(0.0f, 0.0f, 1.0f);
        }
        if (pland && pland->IsNormalSet(scaledvert)) {
            normal = tempnormal;
            pLoadedData->ppNormalsSet[iblock][vertidx] = true;
            return;
        }
        retnormal += tempnormal;
        divisor = divisor + 1.0;
    }

    // The diagonal cell at a corner.
    pland = nullptr;
    if (cellleft && celldown) {
        pland = pL8->pSWLand;
    }
    else if (cellright && celldown) {
        pland = pL8->pSELand;
    }
    else if (cellleft && cellup) {
        pland = pL8->pNWLand;
    }
    else if (cellright && cellup) {
        pland = pL8->pNELand;
    }
    if ((cellleft || cellright) && (celldown || cellup)) {
        NiPoint3 pfacenorm;
        if (!pland || !pland->GetLandNormal(scaledvert, tempnormal, pfacenorm)) {
            tempnormal = NiPoint3(0.0f, 0.0f, 1.0f);
        }
        if (pland && pland->IsNormalSet(scaledvert)) {
            normal = tempnormal;
            pLoadedData->ppNormalsSet[iblock][vertidx] = true;
            return;
        }
        retnormal += tempnormal;
        divisor = divisor + 1.0;
    }

    // The neighbouring blocks of this land.
    if (vertidx < 17 && !celldown) {
        blockdown = true;
        tempnormal = pLoadedData->ppNormals[iblock - 2][vertidx + 272];
        if (pLoadedData->ppNormalsSet[iblock - 2][vertidx + 272]) {
            normal = tempnormal;
            pLoadedData->ppNormalsSet[iblock][vertidx] = true;
            return;
        }
        retnormal += tempnormal;
        divisor = divisor + 1.0;
    }
    else if (vertidx >= 272 && !cellup) {
        blockup = true;
        tempnormal = pLoadedData->ppNormals[iblock + 2][vertidx - 272];
        if (pLoadedData->ppNormalsSet[iblock + 2][vertidx - 272]) {
            normal = tempnormal;
            pLoadedData->ppNormalsSet[iblock][vertidx] = true;
            return;
        }
        retnormal += tempnormal;
        divisor = divisor + 1.0;
    }
    if (vertidx % 17 == 0 && !cellleft) {
        blockleft = true;
        tempnormal = pLoadedData->ppNormals[iblock - 1][vertidx + 16];
        if (pLoadedData->ppNormalsSet[iblock - 1][vertidx + 16]) {
            normal = tempnormal;
            pLoadedData->ppNormalsSet[iblock][vertidx] = true;
            return;
        }
        retnormal += tempnormal;
        divisor = divisor + 1.0;
    }
    else if ((vertidx + 1) % 17 == 0 && !cellright) {
        blockright = true;
        tempnormal = pLoadedData->ppNormals[iblock + 1][vertidx - 16];
        if (pLoadedData->ppNormalsSet[iblock + 1][vertidx - 16]) {
            normal = tempnormal;
            pLoadedData->ppNormalsSet[iblock][vertidx] = true;
            return;
        }
        retnormal += tempnormal;
        divisor = divisor + 1.0;
    }

    // The diagonal block at the land's centre.
    bool bDiagBlock = true;
    int32_t iDiagBlock = 0;
    int32_t iDiagVert = 0;
    if (blockleft && blockdown) {
        iDiagBlock = iblock - 3;
        iDiagVert = 288;
    }
    else if (blockleft && blockup) {
        iDiagBlock = iblock + 1;
        iDiagVert = 16;
    }
    else if (blockright && blockdown) {
        iDiagBlock = iblock - 1;
        iDiagVert = 272;
    }
    else if (blockright && blockup) {
        iDiagBlock = iblock + 3;
        iDiagVert = 0;
    }
    else {
        bDiagBlock = false;
    }
    if (bDiagBlock) {
        tempnormal = pLoadedData->ppNormals[iDiagBlock][iDiagVert];
        if (pLoadedData->ppNormalsSet[iDiagBlock][iDiagVert]) {
            normal = tempnormal;
            pLoadedData->ppNormalsSet[iblock][vertidx] = true;
            return;
        }
        retnormal += tempnormal;
        divisor = divisor + 1.0;
    }
    normal = retnormal / divisor;
    pLoadedData->ppNormalsSet[iblock][vertidx] = true;
}

bool TESObjectLAND::IsLoaded() {
    return (Data.iFlags & LAND_DATA_LOADED) != 0;
}

TESLandTexture* TESObjectLAND::GetDefaultTexture() {
    return TESObjectLAND::pDefText;
}

int32_t TESObjectLAND::GetGrassEvalSize() {
    return TESObjectLAND::iGrassEvalSize;
}

void TESObjectLAND::InitializeStatics() {
    NiPoint2* pTextureBlock;
    NiPoint3* pNormalBlock;
    NiTexturingProperty::Map* pdefmap;
    TESLandTexture* pTexture;
    char acPath[268];
    int32_t i;
    int32_t y;
    int32_t x;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LAND,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESLand.cpp",
        198
    );
    TESObjectLAND::pDefTextTexSet = new BGSTextureSet;
    BSsprintf(acPath, 260, "Landscape\\%s", gs_sDefaultLandDiffuseTexture_Landscape.String());
    TESObjectLAND::pDefTextTexSet->SetBGSTextureFilename(BGSTextureSet::TEXTYPE_BASE, acPath);
    BSsprintf(acPath, 260, "Landscape\\%s", gs_sDefaultLandNormalTexture_Landscape.String());
    TESObjectLAND::pDefTextTexSet->SetBGSTextureFilename(BGSTextureSet::TEXTYPE_NORMAL, acPath);
    pDataHandler->pObjectList->lnk_Remove(TESObjectLAND::pDefTextTexSet);
    pTexture = new TESLandTexture;
    TESObjectLAND::pDefText = pTexture;
    pTexture->pTextureSet = TESObjectLAND::pDefTextTexSet;
    NiPointer<NiTexture> spTempTexture(static_cast<NiTexture*>(nullptr));
    TESObjectLAND::pDefTextTexSet->GetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, spTempTexture);
    pdefmap = ::new NiTexturingProperty::Map(spTempTexture.m_pObject, 0, 3, 5, nullptr);
    TESObjectLAND::spTexturingProperty = new NiTexturingProperty;
    TESObjectLAND::spTexturingProperty->SetApplyMode(NiTexturingProperty::APPLY_MODULATE);
    TESObjectLAND::spTexturingProperty->SetBaseMap(pdefmap);
    // Six indices for each of the 16x16 quads.
    TESObjectLAND::pDefTriListBLOCK = static_cast<uint16_t*>(ourMalloc(16 * 16 * 6 * sizeof(uint16_t)));
    int32_t itrivert = 0;
    for (x = 0; x < 16; ++x) {
        for (y = 0; y < 16; ++y) {
            uint16_t ival;
            if (y % 2 != x % 2) {
                TESObjectLAND::pDefTriListBLOCK[itrivert++] = y + 17 * (x + 1);
                TESObjectLAND::pDefTriListBLOCK[itrivert++] = y + 17 * x;
                TESObjectLAND::pDefTriListBLOCK[itrivert++] = 17 * x + y + 1;
                TESObjectLAND::pDefTriListBLOCK[itrivert++] = 17 * x + y + 1;
                TESObjectLAND::pDefTriListBLOCK[itrivert++] = 17 * (x + 1) + y + 1;
                ival = y + 17 * (x + 1);
            }
            else {
                TESObjectLAND::pDefTriListBLOCK[itrivert++] = 17 * (x + 1) + y + 1;
                TESObjectLAND::pDefTriListBLOCK[itrivert++] = y + 17 * (x + 1);
                TESObjectLAND::pDefTriListBLOCK[itrivert++] = y + 17 * x;
                TESObjectLAND::pDefTriListBLOCK[itrivert++] = y + 17 * x;
                TESObjectLAND::pDefTriListBLOCK[itrivert++] = 17 * x + y + 1;
                ival = 17 * (x + 1) + y + 1;
            }
            TESObjectLAND::pDefTriListBLOCK[itrivert++] = ival;
        }
    }
    float ftilescalar = gs_fLandTextureTilingMult_Landscape.Float();
    if (ftilescalar == 0.0) {
        Warning("DEFAULT: INI setting fLandTextureTilingMult cannot be 0.");
        ftilescalar = 16.0f;
    }
    ftilescalar = 4.0 / ftilescalar;
    // The vertex blocks hold 17x17 = 289 vertices.
    TESObjectLAND::pDefColorBLOCK = new NiColorA[289];
    pNormalBlock = new NiPoint3[289];
    TESObjectLAND::pDefNormalBLOCK = pNormalBlock;
    TESObjectLAND::pDefNormalSetBLOCK = static_cast<bool*>(ourMalloc(289 * sizeof(bool)));
    pTextureBlock = new NiPoint2[289];
    TESObjectLAND::pDefTextureBLOCK = pTextureBlock;
    int32_t index = 0;
    for (x = 0; x < 17; ++x) {
        for (y = 0; y < 17; ++y) {
            NiColorA kColor(1.0f, 1.0f, 1.0f, 0.0f);
            TESObjectLAND::pDefColorBLOCK[index] = kColor;
            TESObjectLAND::pDefNormalBLOCK[index] = NiPoint3(0.0f, 0.0f, 1.0f);
            TESObjectLAND::pDefNormalSetBLOCK[index] = true;
            float ftexv = static_cast<double>(x) / ftilescalar;
            float ftexu = static_cast<double>(y) / ftilescalar;
            NiPoint2 kTexCoord(ftexu, ftexv);
            NiPoint2* pTexCoords = TESObjectLAND::pDefTextureBLOCK;
            pTexCoords[index].x = kTexCoord.x;
            pTexCoords[index].y = kTexCoord.y;
            ++index;
        }
    }
    for (i = 0; i < 4; ++i) {
        TESObjectLAND::pXOffsetBLOCK[i] = static_cast<float>((i % 2 << 11) - 2048);
        TESObjectLAND::pYOffsetBLOCK[i] = static_cast<float>((i / 2 << 11) - 2048);
    }
    for (i = 0; i < 4; ++i) {
        index = 0;
        TESObjectLAND::ppDefVertexBLOCK[i] = new NiPoint3[289];
        for (x = 0; x < 17; ++x) {
            for (y = 0; y < 17; ++y) {
                float fverty = static_cast<double>(x << 7) + TESObjectLAND::pYOffsetBLOCK[i];
                float fvertx = static_cast<double>(y << 7) + TESObjectLAND::pXOffsetBLOCK[i];
                TESObjectLAND::ppDefVertexBLOCK[i][index++] = NiPoint3(fvertx, fverty, -2048.0f);
            }
        }
    }
    TESObjectLAND::bStaticsDefined = true;
}

TESObjectLAND::LoadedLandData::LoadedLandData()
  : ppMesh(nullptr), ppVertices(nullptr), ppNormals(nullptr), ppColorsA(nullptr), ppNormalsSet(nullptr),
    spBorder(nullptr), HeightExtents(std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()),
    pQuadTextureArray(), ppPercentArrays(), pMoppCode(nullptr), pmGrassMap(), spLandRB(nullptr), fBaseHeight(0.0f) {
    for (TESLandTexture*& pTexture : pDefQuadTexture) {
        pTexture = TESObjectLAND::pDefText;
    }
}

TESObjectLAND::LoadedLandData::~LoadedLandData() {}

COORD_DATA::COORD_DATA() {}

hkMoppBvTreeShapeCinfo::hkMoppBvTreeShapeCinfo() : m_code(nullptr), fScale(1.0f) {}

NiDX9ShaderDeclaration* ShadowLightShader::GetLandShaderDecl() {
    return spSplatTexturingShaderDecl;
}

TESGrassAreaParam::TESGrassAreaParam() : kGrassParam() {}

// The cross product, unitized; a length not above 1e-6 (or NaN) gives the zero vector. The length is compared against
// the double widening of 1e-6f.
NiPoint3 NiPoint3::UnitCross(const NiPoint3& pt) const {
    NiPoint3 kCross(y * pt.z - z * pt.y, z * pt.x - x * pt.z, x * pt.y - y * pt.x);
    float fLength = kCross.Length();
    if (fLength > 9.999999974752427e-07) {
        return kCross / fLength;
    }
    return NiPoint3(0.0f, 0.0f, 0.0f);
}

void NiTexturingProperty::SetBaseMap(NiTexturingProperty::Map* pkMap) {
    NiTexturingProperty::Map* pkOld = m_kMaps[BASE_INDEX];
    if (pkMap != pkOld) {
        delete pkOld;
        m_kMaps.SetAt(BASE_INDEX, pkMap);
    }
}

NiVBChip* NiVBChip::Create(
    NiVBBlock* pkBlock,
    IDirect3DVertexBuffer9* pkVB,
    uint32_t uiOffset,
    uint32_t uiSize,
    uint32_t uiIndex
) {
    NiVBChip* pkChip;
    pkChip = new NiVBChip;
    if (pkChip) {
        pkChip->m_uiIndex = uiIndex;
        pkChip->m_pkBlock = pkBlock;
        pkChip->m_pkVB = pkVB;
        pkChip->m_uiOffset = uiOffset;
        pkChip->m_uiSize = uiSize;
        pkChip->m_uiLockFlags = 0;
    }
    return pkChip;
}

// An empty code at a zero offset and scale, not built yet.
hkpMoppCode::hkpMoppCode() {
    m_info.m_offset.setZero4();
    m_buildType.m_storage = BUILD_NOT_SET;
}

// The code's bytes go with m_data.
hkpMoppCode::~hkpMoppCode() {}

const hkClass& hkpMoppCode::staticClass() {
    return hkpMoppCodeClass;
}

void hkMemoryAllocator::bufFreeArray(void* p, int32_t numElem) {
    bufFree(p, numElem);
}

hkBaseObject::hkBaseObject() {}

hkBaseObject::~hkBaseObject() {}

void hkBaseObject::operator delete(void* apMemory) {
    ourFree(apMemory);
}
