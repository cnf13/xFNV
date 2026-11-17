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
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/BGSTextureSet.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
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
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>

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
SettingT<INISettingCollection> gs_sDefaultLandDiffuseTexture_Landscape("sDefaultLandDiffuseTexture:Landscape",
	"DirtWasteland01.dds");
SettingT<INISettingCollection> gs_sDefaultLandNormalTexture_Landscape("sDefaultLandNormalTexture:Landscape",
	"DirtWasteland01_N.dds");
int32_t TESObjectLAND::iGrassEvalSize = gs_iGrassDensityEvalSize_Grass.Int();

// On x64 the m_kBound field is read directly from the model data.
NiBound& NiGeometry::GetModelBound() {
#if FNVD_64BIT
	return this->m_spModelData.operator->()->m_kBound;
#else
	return *static_cast<NiBound*>(static_cast<void*>(static_cast<TESDataHandler*>(static_cast<void*>(this->m_spModelData
																						  .operator->()))
			->TESDataHandler::GetWorldSpaceList()));
#endif
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
		ourFree(TESObjectLAND::pDefColorBLOCK);
		ourFree(TESObjectLAND::pDefNormalBLOCK);
		ourFree(TESObjectLAND::pDefTextureBLOCK);
		ourFree(TESObjectLAND::pDefNormalSetBLOCK);
		for (int32_t i = 0; i < 4; ++i) {
			ourFree(TESObjectLAND::ppDefVertexBLOCK[i]);
		}
		TESObjectLAND::bStaticsDefined = 0;
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
	NiColorA kColor;
	NiPoint2 v35;
	FILE_TEX_DATA texdata_2;
	FILE_TEX_DATA texdata;
	FILE_HEIGHT_DATA kHeightData = {};
	int8_t acNormals[3276] = {};
	uint8_t v48[3272] = {};
	AutoMemContext tempMemContext(MEM_CONTEXT_LAND, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESLand.cpp", 706);
	uint8_t formid = pFile->GetTESForm();
	if (formid == LAND_ID) {
		uint32_t iid = iFormID;
		LoadForm(pFile);
		int32_t ilasttexindex = -1;
		int32_t ilastblock = -1;
		bool bloaddata = 0;
		if ((pFile->GetActive() || TESObjectCELL::GetLoadingTempData()) && !GetLandRemapped()) {
			bloaddata = 1;
		}
		if (bloaddata) {
			CreateLandscape(0);
		}
		bool badjusttextures = 0;
		do {
			CHUNK_ID TESChunk = pFile->GetTESChunk();
			if (TESChunk > VHGT_ID) {
				if (TESChunk == ATXT_ID) {
					if (bloaddata) {
						ourMemset(&texdata_2, 0, sizeof(FILE_TEX_DATA));
						pFile->GetChunkData(&texdata_2, sizeof(FILE_TEX_DATA));
						if (pFile->QEndian()) {
							texdata_2.Endian();
						}
						if (texdata_2.sIndex > 5u) {
							int32_t a6b = texdata_2.cBlock;
							int32_t fBa = texdata_2.sIndex;
							int32_t fGa = GetDataY();
							Warning("MASTERFILE: Land (%i, %i) clamping invalid index %i for block %i.", GetDataX(),
								fGa, fBa, a6b);
							texdata_2.sIndex = 5;
							badjusttextures = 1;
						}
						TESLandTexture* ptex_2 = 0;
						if (texdata_2.iTextureID) {
							TESForm::AddCompileIndex(texdata_2.iTextureID, pFile);
							TESForm* FormByNumericID = TESForm::GetFormByNumericID(texdata_2.iTextureID);
							ptex_2 = dynamic_cast<TESLandTexture*>(FormByNumericID);
							if (!ptex_2) {
								int32_t a6c = texdata_2.cBlock;
								uint32_t fBb = texdata_2.iTextureID;
								int32_t fGb = GetDataY();
								Warning("MASTERFILE: Land (%i, %i) unable to find additional texture ID (%08X) for "
										"block %i.",
									GetDataX(), fGb, fBb, a6c);
								badjusttextures = 1;
							}
						} else {
							ptex_2 = TESObjectLAND::GetDefaultTexture();
						}
						this->pLoadedData->pQuadTextureArray[texdata_2.cBlock][texdata_2.sIndex] = ptex_2;
						if (ptex_2) {
							if (!spQueuedTextures.m_pObject) {
								QueuedFile* v25;
								QueuedFile* result_1 = static_cast<QueuedFile*>(ourMalloc(sizeof(QueuedFile)));
								if (result_1) {
									v25 = ::new (result_1) QueuedFile(IO_TASK_PRIORITY_VERY_LOW);
								} else {
									v25 = 0;
								}
								spQueuedTextures.operator=(v25);
							}
							QueuedFile* a6d = spQueuedTextures.m_pObject;
							ptex_2->QueueTexture(IO_TASK_PRIORITY_VERY_LOW, a6d);
						}
						ilastblock = texdata_2.cBlock;
						ilasttexindex = texdata_2.sIndex;
					}
				} else {
					if (TESChunk == BTXT_ID) {
						if (bloaddata) {
							ourMemset(&texdata, 0, sizeof(FILE_TEX_DATA));
							pFile->GetChunkData(&texdata, sizeof(FILE_TEX_DATA));
							if (pFile->QEndian()) {
								texdata.Endian();
							}
							TESForm::AddCompileIndex(texdata.iTextureID, pFile);
							TESForm* v9 = TESForm::GetFormByNumericID(texdata.iTextureID);
							TESLandTexture* v10 = dynamic_cast<TESLandTexture*>(v9);
							this->pLoadedData->pDefQuadTexture[texdata.cBlock] = v10;
							if (this->pLoadedData->pDefQuadTexture[texdata.cBlock]) {
								if (!spQueuedTextures.m_pObject) {
									QueuedFile* v26;
									QueuedFile* v33 = static_cast<QueuedFile*>(ourMalloc(sizeof(QueuedFile)));
									if (v33) {
										v26 = ::new (v33) QueuedFile(IO_TASK_PRIORITY_VERY_LOW);
									} else {
										v26 = 0;
									}
									spQueuedTextures.operator=(v26);
								}
								TESLandTexture* ptex = this->pLoadedData->pDefQuadTexture[texdata.cBlock];
								QueuedFile* a6a = spQueuedTextures.m_pObject;
								ptex->QueueTexture(IO_TASK_PRIORITY_VERY_LOW, a6a);
							} else {
								int32_t a6 = texdata.cBlock;
								uint32_t fB = texdata.iTextureID;
								int32_t fG = GetDataY();
								Warning("MASTERFILE: Land (%i, %i) unable to find base texture ID (%08X) for block %i.",
									GetDataX(), fG, fB, a6);
								badjusttextures = 1;
							}
						}
					} else {
						if (TESChunk == VTXT_ID && bloaddata) {
							if (ilastblock >= 0 && ilasttexindex >= 0) {
								uint32_t ifilesize = pFile->GetChunkSize();
								if (ifilesize % 8) {
									const char* a6e = pFile->GetFilename();
									int32_t fBc = GetDataY();
									Warning("MASTERFILE: Land (%i, %i) found unrecognized vertex texture data in file "
											"%s.",
										GetDataX(), fBc, a6e);
									badjusttextures = 1;
								} else {
									FILE_TEX_VERT_DATA* pvertdata =
										static_cast<FILE_TEX_VERT_DATA*>(ourMalloc(ifilesize));
									pFile->GetChunkData(pvertdata, ifilesize);
									uint32_t ivertcount = ifilesize >> 3;
									for (uint32_t i_2 = 0; i_2 < ivertcount; ++i_2) {
										if (pFile->QEndian()) {
											pvertdata[i_2].Endian();
										}
										if (pvertdata[i_2].fPercent > 1.0) {
											pvertdata[i_2].fPercent = pvertdata[i_2].fPercent / 100.0;
										}
										SetTexturePercent(ilastblock, pvertdata[i_2].iVertIndex, ilasttexindex,
											pvertdata[i_2].fPercent);
									}
									ourFree(pvertdata);
									pvertdata = 0;
								}
							}
							ilastblock = -1;
							ilasttexindex = -1;
						}
					}
				}
			} else {
				int32_t j;
				int32_t i;
				if (TESChunk == VHGT_ID) {
					if (bloaddata && (Data.iFlags & 1) != 0) {
						float afHeights[1089];
						pFile->GetChunkData(&kHeightData, 0);
						if (pFile->QEndian()) {
							kHeightData.Endian();
						}
						NiPoint2* v3 = ::new (&v35) NiPoint2(3.4028235e+38f, -3.4028235e+38f);
						float x = v3->x;
						float y = v3->y;
						TESObjectLAND::LoadedLandData* pLoadedData = this->pLoadedData;
						pLoadedData->HeightExtents.x = x;
						pLoadedData->HeightExtents.y = y;
						float lastheight = kHeightData.fOffset;
						// Each step adds to the vertex before it in the row; a row's first vertex steps from the first
						// vertex of the row before.
						for (i = 0; i < 1089; ++i) {
							afHeights[i] = static_cast<double>(kHeightData.cHeightData[i]) + lastheight;
							if ((i + 1) % 33) {
								lastheight = afHeights[i];
							} else {
								lastheight = afHeights[i - 32];
							}
						}
						for (i = 0; i < 4; ++i) {
							int32_t v56 = 16 * (i % 2) + 528 * (i / 2);
							for (j = 0; j < 289; ++j) {
								int32_t v54 = j % 17;
								int32_t v55 = j / 17;
								int32_t curvert = j % 17 + (v56 + 33 * (j / 17));
								float v52 = static_cast<float>(8 * FloatToInt(afHeights[curvert]));
								this->pLoadedData->ppVertices[i][j].x =
									static_cast<double>(v54 << 7) + TESObjectLAND::pXOffsetBLOCK[i];
								this->pLoadedData->ppVertices[i][j].y =
									static_cast<double>(v55 << 7) + TESObjectLAND::pYOffsetBLOCK[i];
								this->pLoadedData->ppVertices[i][j].z = v52;
								if (!(this->pLoadedData->HeightExtents.x > static_cast<double>(v52))) {
									if (this->pLoadedData->HeightExtents.y < static_cast<double>(v52)) {
										this->pLoadedData->HeightExtents.y = v52;
									}
								} else {
									this->pLoadedData->HeightExtents.x = v52;
								}
							}
						}
						this->pLoadedData->fBaseHeight =
							(this->pLoadedData->HeightExtents.x + this->pLoadedData->HeightExtents.y) * 0.5;
						for (i = 0; i < 4; ++i) {
							for (j = 0; j < 289; ++j) {
								this->pLoadedData->ppVertices[i][j].z =
									this->pLoadedData->ppVertices[i][j].z - this->pLoadedData->fBaseHeight;
							}
						}
					}
				} else {
					if (TESChunk > VNML_ID) {
						if (TESChunk == VCLR_ID && bloaddata && (Data.iFlags & 2) != 0) {
							pFile->GetChunkData(v48, 0);
							for (i = 0; i < 4; ++i) {
								int32_t orig = 16 * (i % 2) + 528 * (i / 2);
								for (j = 0; j < 289; ++j) {
									int32_t curvert_2 = j % 17 + (orig + 33 * (j / 17));
									float v29 = static_cast<double>(v48[3 * curvert_2 + 2]) / 255.0;
									float v28 = static_cast<double>(v48[3 * curvert_2 + 1]) / 255.0;
									float fR = static_cast<double>(v48[3 * curvert_2]) / 255.0;
									NiColorA* v7 = FNVD_RECONSTRUCT(NiColorA)(&kColor, fR, v28, v29, 1.0f);
									NiColorA* v8 = &this->pLoadedData->ppColorsA[i][j];
									v8->r = v7->r;
									v8->g = v7->g;
									v8->b = v7->b;
									v8->a = v7->a;
								}
							}
						}
					} else {
						if (TESChunk == VNML_ID) {
							if (bloaddata && (Data.iFlags & 1) != 0) {
								pFile->GetChunkData(acNormals, 0);
								for (i = 0; i < 4; ++i) {
									int32_t v50 = 16 * (i % 2) + 528 * (i / 2);
									for (j = 0; j < 289; ++j) {
										int32_t v49 = j % 17 + (v50 + 33 * (j / 17));
										this->pLoadedData->ppNormals[i][j].x =
											static_cast<double>(acNormals[3 * v49]) / 127.0;
										this->pLoadedData->ppNormals[i][j].y =
											static_cast<double>(acNormals[3 * v49 + 1]) / 127.0;
										this->pLoadedData->ppNormals[i][j].z =
											static_cast<double>(acNormals[3 * v49 + 2]) / 127.0;
										this->pLoadedData->ppNormals[i][j].Unitize();
									}
								}
							}
						} else {
							if (TESChunk == DATA_ID) {
								bool bremapped = GetLandRemapped();
								pFile->GetChunkData(&Data, sizeof(OBJ_LAND));
								if (pFile->QEndian()) {
									Data.Endian();
								}
								MarkLoaded(0);
								UpdateUseParentWorldspace(bremapped);
							} else {
								if (TESChunk == MPCD_ID && bloaddata) {
									uint32_t ibuffersize = pFile->GetChunkSize();
									char* pbuffer = static_cast<char*>(ourMalloc(ibuffersize));
									char* pbuffer__pvertdata = pbuffer;
									pFile->GetChunkData(pbuffer, ibuffersize);
									if (this->pLoadedData->pMoppCode) {
										this->pLoadedData->pMoppCode->removeReference();
										this->pLoadedData->pMoppCode = 0;
									}
									ReadMoppCodeFromBuffer(pbuffer__pvertdata, ibuffersize,
										&this->pLoadedData->pMoppCode);
									if (this->pLoadedData->pMoppCode) {
										this->pLoadedData->pMoppCode->addReference();
										SetHasMopp(1);
									}
									ourFree(pbuffer__pvertdata);
								}
							}
						}
					}
				}
			}
		} while (pFile->NextChunk());
		if (badjusttextures) {
			AdjustTextureArrays();
		}
		MarkLoaded(bloaddata);
		return 1;
	} else {
		return 0;
	}
}

// The chunks include padding bytes that are never written from the locals: VHGT's 0x448 bytes end in FILE_HEIGHT_DATA's
// 3 padding bytes, BTXT and ATXT include FILE_TEX_DATA's padding byte at offset 5, and VTXT includes each
// FILE_TEX_VERT_DATA's padding at offsets 2..3.
void TESObjectLAND::Save() {
	FILE_TEX_VERT_DATA atexverts[289];
	FILE_TEX_DATA texdata_2;
	FILE_TEX_DATA texdata;
	if (IsLandAltered() &&
		((Data.iFlags & 8) != 0 ||
			(this->pLoadedData &&
				(this->pLoadedData->ppVertices && (this->pLoadedData->ppNormals && this->pLoadedData->ppColorsA))))) {
		char v20[3272];
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
		if ((Data.iFlags & 1) != 0) {
			int32_t v25[1090];
			char v26[3276];
			FILE_HEIGHT_DATA hgts;
			for (i = 0; i < 4; ++i) {
				int32_t v24 = 16 * (i % 2) + 528 * (i / 2);
				for (int32_t j = 0; j < 289; ++j) {
					int32_t v21 = j % 17 + (v24 + 33 * (j / 17));
					int32_t v22 = 0;
					if (j / 17 == 16) {
						v22 = i / 2 != 1;
					}
					if (j % 17 == 16) {
						v22 = i % 2 != 1;
					}
					if (!v22) {
						v25[v21] = FloatToInt(this->pLoadedData->ppVertices[i][j].z) >> 3;
						this->pLoadedData->ppNormals[i][j].Unitize();
						v26[3 * v21] = static_cast<int32_t>(this->pLoadedData->ppNormals[i][j].x * 127.0);
						v26[3 * v21 + 1] = static_cast<int32_t>(this->pLoadedData->ppNormals[i][j].y * 127.0);
						v26[3 * v21 + 2] = static_cast<int32_t>(this->pLoadedData->ppNormals[i][j].z * 127.0);
					}
				}
			}
			TESForm::AddChunkArray(VNML_ID, v26, 3267u);
			hgts.fOffset = static_cast<float>(v25[0]);
			int32_t lastheight = v25[0];
			char v29 = 0;
			for (i = 0; i < 1089; ++i) {
				if (std::abs(v25[i] - lastheight) < 128) {
					hgts.cHeightData[i] = v25[i] - lastheight;
				} else {
					v29 = 1;
					if (v25[i] <= lastheight) {
						hgts.cHeightData[i] = -127;
					} else {
						hgts.cHeightData[i] = 127;
					}
				}
				if ((i + 1) % 33) {
					if (v29) {
						lastheight += hgts.cHeightData[i];
					} else {
						lastheight = v25[i];
					}
				} else {
					// The first height of the row of 33 just finished.
					lastheight = v25[i - 32];
				}
			}
			if (TESForm::QEndianSwapOnSave()) {
				hgts.Endian();
			}
			TESForm::__AddChunkData(VHGT_ID, &hgts, 1096u);
			if (TESForm::QEndianSwapOnSave()) {
				hgts.Endian();
			}
			if (v29) {
				int32_t someFloat = GetDataY();
				Warning("MASTERFILE: Error saving land height Data for cell (%i, %i). Error correction attempted.\r\n",
					GetDataX(), someFloat);
			}
		}
		if ((Data.iFlags & 2) != 0) {
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
						v20[3 * curvert] = static_cast<int32_t>(this->pLoadedData->ppColorsA[i][k].r * 255.0);
						v20[3 * curvert + 1] = static_cast<int32_t>(this->pLoadedData->ppColorsA[i][k].g * 255.0);
						v20[3 * curvert + 2] = static_cast<int32_t>(this->pLoadedData->ppColorsA[i][k].b * 255.0);
					}
				}
			}
			TESForm::AddChunkArray(VCLR_ID, v20, 3267u);
		}
		if ((Data.iFlags & 4) != 0) {
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
							uint32_t CastingType;
							texdata_2.cBlock = iblock;
							texdata_2.sIndex = itex;
							if (ptex == TESObjectLAND::GetDefaultTexture()) {
								CastingType = 0;
							} else {
								CastingType = ptex->iFormID;
							}
							texdata_2.iTextureID = CastingType;
							if (TESForm::QEndianSwapOnSave()) {
								texdata_2.Endian();
							}
							TESForm::__AddChunkData(ATXT_ID, &texdata_2, 8u);
							if (TESForm::QEndianSwapOnSave()) {
								texdata_2.Endian();
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
									for (uint32_t i_2 = 0; i_2 < itexverts; ++i_2) {
										atexverts[i_2].Endian();
									}
								}
								TESForm::__AddChunkData(VTXT_ID, atexverts, 8 * itexverts);
								if (TESForm::QEndianSwapOnSave()) {
									for (uint32_t i_3 = 0; i_3 < itexverts; ++i_3) {
										atexverts[i_3].Endian();
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
	bool bretval = 0;
	uint8_t FormType = apForm->GetFormType();
	if (TESObjectCELL::IsFormTypeCellChild(FormType)) {
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
				bretval = 1;
				break;
			}
			default: {
				return bretval;
			}
			}
		} else {
			return GetSaveParentCell()->SavesBefore(pformcell);
		}
	} else {
		return GetSaveParentCell()->SavesBefore(apForm);
	}
	return bretval;
}

bool TESObjectLAND::SavesBefore(FORM* apGroupFORM) {
	bool bretval = 0;
	if (apGroupFORM) {
		if (apGroupFORM->form == formEnumString[2].uiFormString) {
			uint32_t iFormID = apGroupFORM->iFormID;
			if (iFormID < 8 || iFormID > 9) {
				TESObjectCELL* pcell_2 = pParentCell;
				return pcell_2->SavesBefore(apGroupFORM);
			} else {
				TESForm* FormByNumericID = TESForm::GetFormByNumericID(apGroupFORM->flags);
				TESObjectCELL* pcell = dynamic_cast<TESObjectCELL*>(FormByNumericID);
				if (pcell) {
					if (pParentCell == pcell) {
						return 0;
					}
				}
			}
		}
	}
	return bretval;
}

TESForm* TESObjectLAND::CreateDuplicateForm(bool abCreateEditorID, NiTPointerMap<TESForm*, TESForm*>* apCopyMap) {
	TESForm* duplicateForm = TESForm::CreateDuplicateForm(0, apCopyMap);
	return dynamic_cast<TESObjectLAND*>(duplicateForm);
}

// Altering the land also marks its parent cell altered.
[[clang::disable_tail_calls]] void TESObjectLAND::SetAltered(bool abVal) {
	TESForm::SetAltered(abVal);
	if (abVal && pParentCell)
		pParentCell->SetAltered(true);
}

void TESObjectLAND::Copy(TESForm* apCopy) {
	TESObjectLAND* pcopyland = dynamic_cast<TESObjectLAND*>(apCopy);
	if (pcopyland) {
		bool bcopyloaded = pcopyland->IsLoaded();
		if (!bcopyloaded) {
			pcopyland->LoadVertices(0);
		}
		pParentCell = pcopyland->pParentCell;
		CreateLandscape(pcopyland);
		for (int32_t i = 0; i < 4; ++i) {
			ourMemcpy(pLoadedData->ppVertices[i], pcopyland->pLoadedData->ppVertices[i], sizeof(NiPoint3) * 289);
			ourMemcpy(pLoadedData->ppNormals[i], pcopyland->pLoadedData->ppNormals[i], sizeof(NiPoint3) * 289);
			ourMemcpy(pLoadedData->ppColorsA[i], pcopyland->pLoadedData->ppColorsA[i], 0x1210u);
			ourMemcpy(pLoadedData->ppNormalsSet[i], pcopyland->pLoadedData->ppNormalsSet[i], 289u);
			ourMemcpy(*reinterpret_cast<void**>(pLoadedData->ppPercentArrays[i]),
				*reinterpret_cast<void**>(pcopyland->pLoadedData->ppPercentArrays[i]), 0x2420u);
			ourMemcpy(pLoadedData->pQuadTextureArray[i], pcopyland->pLoadedData->pQuadTextureArray[i],
				sizeof(TESLandTexture*) * 6);
			pLoadedData->pDefQuadTexture[i] = pcopyland->pLoadedData->pDefQuadTexture[i];
		}
		ourMemcpy(&Data, &pcopyland->Data, sizeof(Data));
		ourMemcpy(&pLoadedData->HeightExtents, &pcopyland->pLoadedData->HeightExtents,
			sizeof(pLoadedData->HeightExtents));
		if (!bcopyloaded) {
			pcopyland->UnLoadVertices();
		}
		MarkLoaded(1);
	}
}

bool TESObjectLAND::BelongsInGroup(FORM* apGroupFORM, bool abAllowParentGroups, bool abCurrentOnly) {
	bool bFalsr = 0;
	if (apGroupFORM && apGroupFORM->form == formEnumString[2].uiFormString) {
		TESForm* pParentCell = GetParentCell();
		uint32_t iFormID = apGroupFORM->iFormID;
		if (iFormID == 6) {
			if (!abAllowParentGroups) {
				return 0;
			}
		} else {
			if (iFormID <= 7 || iFormID > 9) {
				if (abAllowParentGroups) {
					return pParentCell->BelongsInGroup(apGroupFORM, abAllowParentGroups, abCurrentOnly);
				}
				return bFalsr;
			}
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
			} else {
				if (iFormID == 6 && apParentGroup->GroupData.flags == uiParentCellFormID) {
					apOutGroupFORM->form = formEnumString[2].uiFormString;
					apOutGroupFORM->iFormID = 9;
					apOutGroupFORM->flags = this->pParentCell->iFormID;
					apOutGroupFORM->length = 0;
					apOutGroupFORM->iVersionControl = 0;
				}
			}
		}
	}
}

TESObjectCELL* TESObjectLAND::GetSaveParentCell() {
	return pParentCell;
}

void TESObjectLAND::Attach3D() {
	NiPoint3 dst;
	hkMoppBvTreeShapeCinfo cinfo;
	if (pParentCell) {
		if (pParentCell->Get3D()) {
			if (!pLoadedData->ppMesh) {
				NiTriStrips* pquadstrips[4];
				bool pbculled[4];
				NiNode** v17 = static_cast<NiNode**>(ourMalloc(4 * sizeof(NiNode*)));
				pLoadedData->ppMesh = v17;
				ourMemset(pbculled, 0, 4u);
				ourMemset(pquadstrips, 0, sizeof(pquadstrips));
				NiPoint2 landminmax(3.4028235e+38f, -3.4028235e+38f);
				for (int32_t iblock_2 = 0; iblock_2 < 4; ++iblock_2) {
					NiNode* QuadLandNode = GetQuadLandNode(iblock_2);
					pLoadedData->ppMesh[iblock_2] = QuadLandNode;
					if (pLoadedData->ppMesh[iblock_2]) {
						bool AppCulled = pLoadedData->ppMesh[iblock_2]->GetAppCulled();
						pbculled[iblock_2] = AppCulled;
						pLoadedData->ppMesh[iblock_2]->SetAppCulled(1);
						NiTexturingProperty* z = TESObjectLAND::spTexturingProperty.m_pObject;
						pLoadedData->ppMesh[iblock_2]->AttachProperty(z);
						NiTriStrips* ptrigeom = MakeLandTriStrips(iblock_2);
						pquadstrips[iblock_2] = ptrigeom;
						pLoadedData->ppMesh[iblock_2]->SetAt(0, ptrigeom);
						uint16_t I = ptrigeom->GetVertexCount();
						NiPoint3* pTmp = ptrigeom->GetVertices();
						NiPoint3* pTmp_2 = pTmp;
						int32_t I_2 = I;
						NiBound* ModelBound = &ptrigeom->GetModelBound();
						ModelBound->ComputeFromData(I_2, pTmp_2);
						ptrigeom->GetModelData()->MarkAsChanged(15);
						BSMultiBoundNode* pbsmbnode = static_cast<BSMultiBoundNode*>(pLoadedData->ppMesh[iblock_2]);
						BSMultiBoundAABB* pbmbaabb = static_cast<BSMultiBoundAABB*>(pbsmbnode->QMultiBound()->QShape());
						NiPoint2 kminmax = GetMinMaxBlockHeight(iblock_2);
						float v14 = (kminmax.y - kminmax.x) / 2.0;
						NiPoint3* v4 = ::new (&dst) NiPoint3(1024.0f, 1024.0f, v14);
						pbmbaabb->SetHalfExtents(*v4);
						NiPoint3 kcenter = NiPoint3_ZERO;
						double v5 = GetWorldCellX() + 1024.0;
						kcenter.x = v5 + static_cast<double>(iblock_2 % 2 << 11);
						double WorldCellY = GetWorldCellY();
						kcenter.y = WorldCellY + 1024.0 + static_cast<double>(iblock_2 / 2 << 11);
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
				cinfo.~hkMoppBvTreeShapeCinfo();
				::new (&cinfo) hkMoppBvTreeShapeCinfo;
				cinfo.m_code = pLoadedData->pMoppCode;
				pcm->Create(pquadstrips, 4u, pLoadedData->fBaseHeight);
				CalculateGrassParams();
				MergeSameFileTextures();
				InitLandscape();
				for (int32_t iblock_3 = 0; iblock_3 < 4; ++iblock_3) {
					if (pquadstrips[iblock_3]) {
						ShadowLightShader* v13;
						NiGeometryData* pgeodata = pquadstrips[iblock_3]->GetModelData();
						pgeodata->SetConsistency(NiGeometryData::STATIC);
						pgeodata->SetCompressFlags(1u);
						uint32_t ikeep = 23;
						pgeodata->m_ucKeepFlags = 23u;
						BSShader* pshade = pquadstrips[iblock_3]->GetShader();
						if (pshade) {
							ShadowLightShader* pshade_2;
							// Shader types 1 to 5 are the ShadowLightShader family; the type is read as signed.
							if (pshade->GetShaderType() < 1 || pshade->GetShaderType() > 5) {
								pshade_2 = 0;
							} else {
								pshade_2 = static_cast<ShadowLightShader*>(pshade);
							}
							v13 = pshade_2;
						} else {
							v13 = 0;
						}
						ShadowLightShader* pslshader = v13;
						if (v13) {
							NiDX9Renderer* prend = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
							NiD3DShaderDeclaration* pdecl = pslshader->GetLandShaderDecl();
							prend->PrecacheGeometry(pquadstrips[iblock_3], 0, 0, pdecl);
							prend->PerformPrecache();
						}
					}
					if (pLoadedData->ppMesh[iblock_3]) {
						pLoadedData->ppMesh[iblock_3]->SetAppCulled(pbculled[iblock_3]);
						NiUpdateData UpdateData(0.0f, 0, 0);
						pLoadedData->ppMesh[iblock_3]->Update(UpdateData);
						pLoadedData->ppMesh[iblock_3]->UpdateProperties();
					}
				}
			}
			if (pLoadedData->ppMesh) {
				if (pParentCell) {
					bool bforce = 0;
					bool bEmpty = GetLandRemapped() && !pParentCell->TESObjectCELL::HasReferences();
					bforce = bEmpty;
					for (int32_t iblock = 0; iblock < 4; ++iblock) {
						if (bforce || pParentCell->TESObjectCELL::GetHideLand(static_cast<uint32_t>(iblock))) {
							pLoadedData->ppMesh[iblock]->SetAppCulled(1);
						} else {
							pLoadedData->ppMesh[iblock]->SetAppCulled(0);
						}
					}
				}
			}
		}
	}
}

bool TESObjectLAND::LoadVertices(bool abLoad3D) {
	AutoMemContext tempMemContext(MEM_CONTEXT_LAND, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESLand.cpp", 1109);
	if (Interface::GetLoadingMenuVisible()) {
		abLoad3D = 0;
	}
	if (IsLoaded()) {
		if (!pLoadedData->ppMesh && abLoad3D) {
			Attach3D();
		}
	LABEL_15:;
		return 1;
	}
	if (!GetLandRemapped() &&
		(((Data.iFlags & 1) == 0 && ((Data.iFlags & 2) == 0 && (Data.iFlags & 4) == 0)) || !GetFile(-1))) {
		CreateLandscape(0);
		if (abLoad3D) {
			Attach3D();
		}
		goto LABEL_15;
	}
	CreateLandscape(0);
	bool success = LoadVerticesIntoArrays();
	MarkLoaded(1);
	if (abLoad3D) {
		Attach3D();
	}
	return success;
}

// Frees the loaded data's per-quadrant arrays and grass maps, then the data itself through its deleting destructor.
bool TESObjectLAND::UnLoadVertices() {
	NiTMapItem<uint32_t, TESGrassAreaParam**>* pPos;
	MarkLoaded(0);
	Detach3D();
	if (pLoadedData) {
		TESGrassAreaParam** pgrassParam = 0;
		for (int32_t i = 0; i < 4; ++i) {
			if (pLoadedData->ppPercentArrays[i]) {
				ourFree(*pLoadedData->ppPercentArrays[i]);
				ourFree(pLoadedData->ppPercentArrays[i]);
				pLoadedData->ppPercentArrays[i] = 0;
			}
			if (pLoadedData->pQuadTextureArray[i]) {
				ourFree(pLoadedData->pQuadTextureArray[i]);
				pLoadedData->pQuadTextureArray[i] = 0;
			}
			if (pLoadedData->ppVertices) {
				ourFree(pLoadedData->ppVertices[i]);
				pLoadedData->ppVertices[i] = 0;
			}
			if (pLoadedData->ppNormals) {
				ourFree(pLoadedData->ppNormals[i]);
				pLoadedData->ppNormals[i] = 0;
			}
			if (pLoadedData->ppColorsA) {
				ourFree(pLoadedData->ppColorsA[i]);
				pLoadedData->ppColorsA[i] = 0;
			}
			if (pLoadedData->ppNormalsSet) {
				ourFree(pLoadedData->ppNormalsSet[i]);
				pLoadedData->ppNormalsSet[i] = 0;
			}
			if (pLoadedData->pmGrassMap[i].m_kAllocator.m_uiCount) {
				pPos = pLoadedData->pmGrassMap[i].GetFirstPos();
				while (pPos) {
					uint32_t ikey;
					pLoadedData->pmGrassMap[i].GetNext(pPos, ikey, pgrassParam);
					if (pgrassParam) {
						for (int32_t k = 0; k < 16; ++k) {
							if (pgrassParam[k]) {
								pgrassParam[k]->kGrassParam.pcGeomFilename = 0;
								ourFree(pgrassParam[k]);
							}
						}
						ourFree(pgrassParam);
					}
				}
			}
			pLoadedData->pmGrassMap[i].RemoveAll();
		}
		ourFree(pLoadedData->ppNormalsSet);
		ourFree(pLoadedData->ppVertices);
		ourFree(pLoadedData->ppColorsA);
		ourFree(pLoadedData->ppNormals);
		if (pLoadedData->pMoppCode) {
			pLoadedData->pMoppCode->removeReference();
			pLoadedData->pMoppCode = 0;
		}
	}
	TESObjectLAND::LoadedLandData* pData = pLoadedData;
	if (pData) {
		delete pData;
	}
	pLoadedData = 0;
	return 1;
}

// Each block holds 17x17 = 289 vertices, 6 quad textures and 8 texture-percent floats per vertex.
int8_t TESObjectLAND::CreateLandscape(TESObjectLAND* apFrom) {
	TESObjectLAND::LoadedLandData* v6;
	if (pLoadedData && pLoadedData->ppVertices) {
		return 0;
	}
	TESObjectLAND::LoadedLandData* apMemory =
		static_cast<TESObjectLAND::LoadedLandData*>(ourMalloc(sizeof(TESObjectLAND::LoadedLandData)));
	if (apMemory) {
		v6 = ::new (apMemory) TESObjectLAND::LoadedLandData;
	} else {
		v6 = 0;
	}
	pLoadedData = v6;
	pLoadedData->iCellX = pParentCell->GetDataX();
	pLoadedData->iCellY = pParentCell->GetDataY();
	pLoadedData->ppVertices = static_cast<NiPoint3**>(ourMalloc(4 * sizeof(NiPoint3*)));
	pLoadedData->ppColorsA = static_cast<NiColorA**>(ourMalloc(4 * sizeof(NiColorA*)));
	pLoadedData->ppNormals = static_cast<NiPoint3**>(ourMalloc(4 * sizeof(NiPoint3*)));
	pLoadedData->ppNormalsSet = static_cast<bool**>(ourMalloc(4 * sizeof(bool*)));
	float fdefheight = GetDefaultWorldHeight();
	for (int32_t iblock = 0; iblock < 4; ++iblock) {
		NiPoint3* v3;
		NiColorA* v4;
		NiPoint3* v5;
		void* ptr = ourMalloc(289 * sizeof(NiPoint3));
		if (ptr) {
			rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), 289, &emptyPointConstructor);
			v5 = static_cast<NiPoint3*>(ptr);
		} else {
			v5 = 0;
		}
		pLoadedData->ppVertices[iblock] = v5;
		void* v9 = ourMalloc(289 * sizeof(NiColorA));
		if (v9) {
			rt::eh_vector_constructor_iterator(v9, sizeof(NiColorA), 289, &fnvd::constructElement<NiColorA>);
			v4 = static_cast<NiColorA*>(v9);
		} else {
			v4 = 0;
		}
		pLoadedData->ppColorsA[iblock] = v4;
		void* v8 = ourMalloc(289 * sizeof(NiPoint3));
		if (v8) {
			rt::eh_vector_constructor_iterator(v8, sizeof(NiPoint3), 289, &emptyPointConstructor);
			v3 = static_cast<NiPoint3*>(v8);
		} else {
			v3 = 0;
		}
		pLoadedData->ppNormals[iblock] = v3;
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
			for (int32_t ivert_2 = 0; ivert_2 < 289; ++ivert_2) {
				pLoadedData->ppPercentArrays[iblock][ivert_2] = &ppercentArrays[8 * ivert_2];
			}
			ourMemset(pLoadedData->pQuadTextureArray[iblock], 0, 6 * sizeof(TESLandTexture*));
		}
	}
	return 1;
}

void TESObjectLAND::DrawLandBorder(bool bShow) {
	NiPoint3 dst;
	FNVD_INLINED_LOCAL(NiColorA, v28, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiColorA, v29, std::nullopt, std::nullopt);
	int32_t tcount = 32;
	int32_t vcount = 128;
	int32_t lineidx = 0;
	if (HasVertices()) {
		NiNode* pNode = pParentCell->Get3D();
		if (pNode) {
			if (pLoadedData->spBorder.m_pObject) {
				pNode->DetachChild(pLoadedData->spBorder.m_pObject);
				pLoadedData->spBorder.operator=(static_cast<NiLines*>(0));
			}
			if (bShow) {
				BSShaderNoLightingProperty* pshprop;
				NiTexturingProperty* deftext;
				NiLines* pBorder;
				NiColorA* lineclr;
				NiPoint3* lineverts;
				void* ptr = ourMalloc(128 * sizeof(NiPoint3));
				if (ptr) {
					rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), 128, &emptyPointConstructor);
					lineverts = static_cast<NiPoint3*>(ptr);
				} else {
					lineverts = 0;
				}
				void* v31 = ourMalloc(128 * sizeof(NiColorA));
				if (v31) {
					rt::eh_vector_constructor_iterator(v31, sizeof(NiColorA), 128, &fnvd::constructElement<NiColorA>);
					lineclr = static_cast<NiColorA*>(v31);
				} else {
					lineclr = 0;
				}
				uint8_t* lineflags = static_cast<uint8_t*>(ourMalloc(128u));
				float y = GetWorldCellY();
				float x = GetWorldCellX();
				NiPoint3 worldshift(x, y, 0.0f);
				for (int32_t x_2 = 0; x_2 < 33; ++x_2) {
					for (int32_t y_2 = 0; y_2 < 33; ++y_2) {
						if (!x_2 || (!y_2 || (x_2 == 32 || y_2 == 32))) {
							NiColorA* v2;
							float tmphgt;
							if (y_2) {
								if (x_2 == 32) {
									lineidx = y_2 + 32;
								} else {
									if (y_2 == 32) {
										lineidx = 32 - x_2 + 64;
									} else {
										if (!x_2) {
											lineidx = 32 - y_2 + 96;
										}
									}
								}
							} else {
								lineidx = x_2;
							}
							if (lineidx % 2) {
								float v19 = static_cast<double>(gs_iLandBorder1B_Landscape.Int()) / 255.0;
								float v18 = static_cast<double>(gs_iLandBorder1G_Landscape.Int()) / 255.0;
								float fR = static_cast<double>(gs_iLandBorder1R_Landscape.Int()) / 255.0;
								v2 = ::new (&v29) NiColorA(fR, v18, v19, 1.0f);
							} else {
								float v16 = static_cast<double>(gs_iLandBorder2B_Landscape.Int()) / 255.0;
								float v15 = static_cast<double>(gs_iLandBorder2G_Landscape.Int()) / 255.0;
								float v14 = static_cast<double>(gs_iLandBorder2R_Landscape.Int()) / 255.0;
								v2 = ::new (&v28) NiColorA(v14, v15, v16, 1.0f);
							}
							NiColorA* v3 = &lineclr[lineidx];
							v3->r = v2->r;
							v3->g = v2->g;
							v3->b = v2->b;
							v3->a = v2->a;
							lineflags[lineidx] = 1;
							float v13 = static_cast<double>(y_2) * 128.0;
							float v12 = static_cast<double>(x_2) * 128.0;
							NiPoint3* v4 = ::new (&dst) NiPoint3(v12, v13, 0.0f);
							NiPoint3 tmppt = worldshift + *v4;
							GetLandHeight(tmppt, tmphgt);
							tmppt.z = tmphgt + 5.0;
							lineverts[lineidx] = tmppt - worldshift;
						}
					}
				}
				NiLines* v25 = static_cast<NiLines*>(NiMemObject::operator new(sizeof(NiLines)));
				if (v25) {
					pBorder = ::new (v25)
						NiLines(128, lineverts, lineclr, nullptr, 0, NiGeometryData::NBT_METHOD_NONE, lineflags);
				} else {
					pBorder = 0;
				}
				pLoadedData->spBorder.operator=(pBorder);
				pLoadedData->spBorder.m_pObject->SetTranslate(worldshift);
				NiTexturingProperty* v24 =
					static_cast<NiTexturingProperty*>(NiMemObject::operator new(sizeof(NiTexturingProperty)));
				if (v24) {
					deftext = ::new (v24) NiTexturingProperty;
				} else {
					deftext = 0;
				}
				deftext->SetBaseMap(0);
				deftext->SetDecalMap(0, 0);
				pLoadedData->spBorder.m_pObject->AttachProperty(deftext);
				BSShaderNoLightingProperty* apMemory = static_cast<
					BSShaderNoLightingProperty*>(NiMemObject::operator new(sizeof(BSShaderNoLightingProperty)));
				if (apMemory) {
					pshprop = ::new (apMemory) BSShaderNoLightingProperty;
				} else {
					pshprop = 0;
				}
				pshprop->SetShader(33);
				pLoadedData->spBorder.m_pObject->AttachProperty(pshprop);
				BSShaderManager::PrepareObject(pLoadedData->spBorder.m_pObject, 0, 0);
				NiLines* ya = pLoadedData->spBorder.m_pObject;
				pNode->AttachChild(ya, 1);
				pLoadedData->spBorder.m_pObject->UpdateProperties();
				NiUpdateData UpdateData(0.0f, 0, 0);
				pLoadedData->spBorder.m_pObject->Update(UpdateData);
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
	return (double)(GetDataX() << 12);
}

TESLandTexture* TESObjectLAND::GetQuadTexture(uint8_t acBlock, uint16_t aiLayer) {
	int32_t v4 = 0;
	if ((((uint32_t)((int32_t)acBlock)) >= 4u) || (((uint32_t)((int32_t)aiLayer)) >= 6u)) {
		return reinterpret_cast<TESLandTexture*>(static_cast<uintptr_t>(v4));
	}
	if (!pLoadedData) {
		return reinterpret_cast<TESLandTexture*>(static_cast<uintptr_t>(v4));
	}
	if (pLoadedData->pQuadTextureArray[acBlock]) {
		return pLoadedData->pQuadTextureArray[acBlock][aiLayer];
	}
	return reinterpret_cast<TESLandTexture*>(static_cast<uintptr_t>(v4));
}

float TESObjectLAND::GetDefaultWorldHeight() {
	float v5 = -2048.0f;
	TESObjectCELL* ObjectReference = pParentCell;
	if (ObjectReference) {
		TESWorldSpace* WorldSpace = ObjectReference->GetWorldSpace();
		if (WorldSpace) {
			return WorldSpace->GetDefaultLandHeight();
		}
	}
	return v5;
}

bool TESObjectLAND::GetCoordData(COORD_DATA* cdata, const NiPoint3& coord, bool forcetovertex) {
	NiPoint3 dst;
	NiPoint3 arOut;
	float v28 = static_cast<double>(GetDataX()) * 4096.0;
	float v30 = static_cast<double>(GetDataY()) * 4096.0;
	if (!pLoadedData || (!pLoadedData->ppVertices && !pLoadedData->ppMesh)) {
		return 0;
	}
	if (v28 > static_cast<double>(coord.x)) {
		return 0;
	}
	float v11 = v28 + 4096.0;
	if (coord.x > static_cast<double>(v11)) {
		return 0;
	}
	if (v30 > static_cast<double>(coord.y)) {
		return 0;
	}
	float v10 = v30 + 4096.0;
	if (coord.y > static_cast<double>(v10)) {
		return 0;
	}
	cdata->Cellpoint.x = coord.x - v28;
	cdata->Cellpoint.y = coord.y - v30;
	cdata->Block.x = static_cast<int32_t>(cdata->Cellpoint.x / 2048.0);
	if (!(static_cast<int32_t>(cdata->Cellpoint.x) % 2048)) {
		if (static_cast<int32_t>(cdata->Cellpoint.x)) {
			--cdata->Block.x;
		}
	}
	cdata->Block.y = static_cast<int32_t>(cdata->Cellpoint.y / 2048.0);
	if (!(static_cast<int32_t>(cdata->Cellpoint.y) % 2048)) {
		if (static_cast<int32_t>(cdata->Cellpoint.y)) {
			--cdata->Block.y;
		}
	}
	cdata->iBlock = cdata->Block.x + 2 * cdata->Block.y;
	float v27 = static_cast<double>(cdata->Block.x << 11) + v28;
	float v29 = static_cast<double>(cdata->Block.y << 11) + v30;
	cdata->Blockpoint.x = coord.x - v27;
	cdata->Blockpoint.y = coord.y - v29;
	float y = GetWorldCellY() + 2048.0;
	float x = GetWorldCellX() + 2048.0;
	NiPoint3 ecx0 = *::new (&dst) NiPoint3(x, y, static_cast<float>(0.0));
	cdata->Tile.x = static_cast<int32_t>(cdata->Blockpoint.x / 128.0);
	if (!(static_cast<int32_t>(cdata->Blockpoint.x) % 128)) {
		if (static_cast<int32_t>(cdata->Blockpoint.x)) {
			--cdata->Tile.x;
		}
	}
	cdata->Tile.y = static_cast<int32_t>(cdata->Blockpoint.y / 128.0);
	if (!(static_cast<int32_t>(cdata->Blockpoint.y) % 128)) {
		if (static_cast<int32_t>(cdata->Blockpoint.y)) {
			--cdata->Tile.y;
		}
	}
	float v26 = static_cast<double>(cdata->Tile.x << 7) + v27;
	float v24 = static_cast<double>(cdata->Tile.y << 7) + v29;
	cdata->Tilepoint.x = coord.x - v26;
	cdata->Tilepoint.y = coord.y - v24;
	cdata->iShape = cdata->iBlock;
	if ((cdata->Tile.y + cdata->Tile.x) % 2) {
		cdata->bTriright = 1;
		cdata->iTrivertidx[1] = 17 * cdata->Tile.y + cdata->Tile.x + 1;
		cdata->iTrivertidx[2] = cdata->Tile.x + 17 * (cdata->Tile.y + 1);
		if (!(cdata->Tilepoint.x + cdata->Tilepoint.y > 128.0)) {
			cdata->iTrivertidx[0u] = cdata->Tile.x + 17 * cdata->Tile.y;
			cdata->bTriupper = 0;
		} else {
			cdata->iTrivertidx[0u] = 17 * (cdata->Tile.y + 1) + cdata->Tile.x + 1;
			cdata->bTriupper = 1;
		}
	} else {
		cdata->bTriright = 0;
		cdata->iTrivertidx[1] = cdata->Tile.x + 17 * cdata->Tile.y;
		cdata->iTrivertidx[2] = 17 * (cdata->Tile.y + 1) + cdata->Tile.x + 1;
		if (!(cdata->Tilepoint.y < static_cast<double>(cdata->Tilepoint.x))) {
			cdata->iTrivertidx[0u] = cdata->Tile.x + 17 * (cdata->Tile.y + 1);
			cdata->bTriupper = 1;
		} else {
			cdata->iTrivertidx[0u] = 17 * cdata->Tile.y + cdata->Tile.x + 1;
			cdata->bTriupper = 0;
		}
	}
	if (static_cast<int32_t>(coord.x) % 128 || static_cast<int32_t>(coord.y) % 128) {
		float v21 = 0.0;
		int32_t v22 = 0;
		NiPoint2 pt(coord.x, coord.y);
		for (int32_t i = 0; i < 3; ++i) {
			NiPoint2 v17;
			NiPoint3 a4a;
			GetVertex(cdata->iBlock, cdata->iTrivertidx[i], a4a);
			float v7 = ecx0.y + a4a.y;
			float v6 = ecx0.x + a4a.x;
			NiPoint2 v18(v6, v7);
			v17 = v18 - pt;
			float v16 = v17.Length();
			if (!i || v21 > static_cast<double>(v16)) {
				v21 = v16;
				v22 = i;
			}
		}
		cdata->iVertidx = cdata->iTrivertidx[v22];
	} else {
		float z = static_cast<float>(static_cast<int32_t>(cdata->Blockpoint.x) / 128 +
			17 * (static_cast<int32_t>(cdata->Blockpoint.y) / 128));
		cdata->iVertidx = FloatToInt(z);
	}
	GetVertex(cdata->iBlock, cdata->iVertidx, arOut);
	cdata->Vertcoord = ecx0 + arOut;
	return !forcetovertex || GetCoordData(cdata, cdata->Vertcoord, 0);
}

bool TESObjectLAND::GetLandHeight(const COORD_DATA& cdata, float& height) {
	NiPoint3 a4;
	float v11;
	NiPoint3 arOut;
	NiPoint3 ecx0;
	GetVertex(cdata.iBlock, cdata.iTrivertidx[0u], a4);
	GetVertex(cdata.iBlock, cdata.iTrivertidx[1], ecx0);
	GetVertex(cdata.iBlock, cdata.iTrivertidx[2], arOut);
	float z = a4.z;
	float v15 = ecx0.z;
	float v16 = arOut.z;
	if (cdata.bTriright) {
		double v4;
		v11 = (v16 - z) / 128.0;
		if (cdata.bTriupper) {
			float v8 = (z - v16) / 128.0;
			v11 = (z - v15) / 128.0;
			v4 = z - ((128.0 - cdata.Tilepoint.x) * v8 + (128.0 - cdata.Tilepoint.y) * v11);
		} else {
			float v9 = (v15 - z) / 128.0;
			v11 = (v16 - z) / 128.0;
			v4 = v9 * cdata.Tilepoint.x + z + v11 * cdata.Tilepoint.y;
		}
		height = v4;
	} else {
		double v3;
		float v7 = (z - v15) / 128.0;
		v11 = (v16 - z) / 128.0;
		if (cdata.bTriupper) {
			v3 = v7 * cdata.Tilepoint.y + v15 + v11 * cdata.Tilepoint.x;
		} else {
			v3 = v7 * cdata.Tilepoint.x + v15 + v11 * cdata.Tilepoint.y;
		}
		height = v3;
	}
	return 1;
}

NiGeometry* TESObjectLAND::GetGeometry(const NiPoint3& arPoint) {
	COORD_DATA cdata;
	cdata.~COORD_DATA();
	::new (&cdata) COORD_DATA;
	if (!GetCoordData(&cdata, arPoint, false)) {
		return nullptr;
	}
	return GetGeometry(cdata);
}

NiGeometry* TESObjectLAND::GetGeometry(const COORD_DATA& cdata) {
	if (pLoadedData && (pLoadedData->ppMesh && pLoadedData->ppMesh[cdata.iBlock])) {
		return static_cast<NiGeometry*>(pLoadedData->ppMesh[cdata.iBlock]->GetAtChecked(0));
	} else {
		return 0;
	}
}

// The flags are tested in the game's order: 1, then 4, then 2.
bool TESObjectLAND::IsLandAltered() {
	if (Data.iFlags & 1) {
		return true;
	}
	if (Data.iFlags & 4) {
		return true;
	}
	if (Data.iFlags & 2) {
		return true;
	}
	return false;
}

void TESObjectLAND::SetCell(TESObjectCELL* apCell) {
	TESWorldSpace* worldSpace;
	pParentCell = apCell;
	if (pParentCell) {
		worldSpace = pParentCell->GetWorldSpace();
		if (worldSpace && worldSpace->GetParentWorld(TESWorldSpace::PU_LAND)) {
			UpdateUseParentWorldspace(1);
		} else {
			UpdateUseParentWorldspace(0);
		}
	}
}

bool TESObjectLAND::GetLandHeight(const NiPoint3& coord, float& height) {
	COORD_DATA cdata;
	cdata.~COORD_DATA();
	::new (&cdata) COORD_DATA;
	if (!GetCoordData(&cdata, coord, false)) {
		height = GetDefaultWorldHeight();
		return false;
	}
	return GetLandHeight(cdata, height);
}

TESLandTexture* TESObjectLAND::GetMainTexture(const NiPoint3& aCoord) {
	COORD_DATA cdata;
	TESLandTexture* v5 = 0;
	cdata.~COORD_DATA();
	::new (&cdata) COORD_DATA;
	if (GetCoordData(&cdata, aCoord, 1)) {
		return GetMainTexture(cdata.iBlock, cdata.iVertidx);
	}
	return v5;
}

bool TESObjectLAND::GetLandNormal(const NiPoint3& aCoord, NiPoint3& aNormal, NiPoint3& aFaceNormal) {
	COORD_DATA coordData;
	coordData.~COORD_DATA();
	::new (&coordData) COORD_DATA;
	if (GetCoordData(&coordData, aCoord, 0)) {
		return (bool)((int8_t)GetLandNormal(coordData, aNormal, aFaceNormal));
	} else {
		return (bool)0;
	}
}

bool TESObjectLAND::GetLandNormal(const COORD_DATA& cdata, NiPoint3& aNormal, NiPoint3& aFaceNormal) {
	NiPoint3 v41;
	NiPoint3 v44;
	NiPoint3 v47;
	NiPoint3 v50;
	NiPoint3 v53;
	NiPoint3 v56;
	NiPoint3 v59;
	NiPoint3 v62;
	NiPoint3 dst;
	NiPoint3 a4;
	NiPoint3 result;
	NiPoint3 v78;
	result = GetVertexLocation(cdata.iBlock, cdata.iVertidx, 0, 0);
	GetVertex(cdata.iBlock, cdata.iVertidx, a4);
	float v36 = GetWorldCellY() + 2048.0;
	float x = GetWorldCellX() + 2048.0;
	NiPoint3 ecx0 = *::new (&dst) NiPoint3(x, v36, static_cast<float>(0.0));
	v78 = a4 + ecx0;
	v78.z = 0.0;
	if (v78.operator==(result)) {
		GetNormal(cdata.iBlock, cdata.iVertidx, aNormal);
		aFaceNormal = aNormal;
	} else {
		NiPoint3 v66;
		NiPoint3 pt;
		NiPoint3 v68;
		NiPoint3 norm;
		NiPoint3 v70;
		NiPoint3 arOut;
		NiPoint3 vert;
		float v65 = std::fmodf(result.x, 128.0f) / 128.0;
		float v73 = std::fmodf(result.y, 128.0f) / 128.0;
		NiPoint3 v72 = NiPoint3_UNIT_Z;
		GetNormal(cdata.iBlock, cdata.iTrivertidx[0u], norm);
		GetNormal(cdata.iBlock, cdata.iTrivertidx[1], v70);
		GetNormal(cdata.iBlock, cdata.iTrivertidx[2], v66);
		if (!cdata.bTriright && !cdata.bTriupper) {
			NiPoint3* Y = &(v59 = v73 * v66);
			float v34 = 1.0 - v73;
			NiPoint3* v17 = &(v62 = v65 * norm);
			float fScalar = 1.0 - v65;
			v72 = (fScalar * v70 + *v17) * v34 + *Y;
		}
		if (!cdata.bTriright && cdata.bTriupper) {
			float v32 = 1.0 - v73;
			NiPoint3* Ya = &(v53 = v32 * v70);
			NiPoint3* v18 = &(v56 = v65 * v66);
			float v31 = 1.0 - v65;
			v72 = (v31 * norm + *v18) * v73 + *Ya;
		}
		if (cdata.bTriright && !cdata.bTriupper) {
			NiPoint3* Yb = &(v47 = v73 * v66);
			float v30 = 1.0 - v73;
			NiPoint3* v19 = &(v50 = v65 * v70);
			float v29 = 1.0 - v65;
			v72 = (v29 * norm + *v19) * v30 + *Yb;
		}
		if (cdata.bTriright && cdata.bTriupper) {
			float v28 = 1.0 - v73;
			NiPoint3* Yc = &(v41 = v28 * v70);
			NiPoint3* v20 = &(v44 = v65 * norm);
			float v27 = 1.0 - v65;
			v72 = (v27 * v66 + *v20) * v73 + *Yc;
		}
		aNormal = v72;
		aNormal.Unitize();
		GetVertex(cdata.iBlock, cdata.iTrivertidx[0u], arOut);
		GetVertex(cdata.iBlock, cdata.iTrivertidx[1], vert);
		arOut.operator-=(vert);
		GetVertex(cdata.iBlock, cdata.iTrivertidx[0u], v68);
		GetVertex(cdata.iBlock, cdata.iTrivertidx[2], pt);
		v68.operator-=(pt);
		if ((!cdata.bTriright || cdata.bTriupper) && (cdata.bTriright || !cdata.bTriupper)) {
			aFaceNormal = v68.UnitCross(arOut);
		} else {
			aFaceNormal = arOut.UnitCross(v68);
		}
	}
	return 1;
}

bool TESObjectLAND::GetLandColor(const NiPoint3& aCoord, NiColorA& aColor) {
	FNVD_INLINED_LOCAL(NiColorA, v36, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiColorA, v39, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiColorA, v40, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiColorA, v42, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiColorA, v45, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiColorA, v46, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiColorA, v48, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiColorA, v51, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiColorA, v52, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiColorA, apResult, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiColorA, v57, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiColorA, v58, std::nullopt, std::nullopt);
	COORD_DATA cdata;
	cdata.~COORD_DATA();
	::new (&cdata) COORD_DATA;
	if (!GetCoordData(&cdata, aCoord, 0)) {
		return 0;
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
		NiColorA* Y = NiColorA::Multiply(&apResult, fpy, &c2);
		float v33 = 1.0 - fpy;
		NiColorA* v16 = NiColorA::Multiply(&v57, fpx, &c0);
		float v32 = 1.0 - fpx;
		NiColorA* v4 = NiColorA::Multiply(&v58, v32, &c1);
		aColor = (*v4 + *v16) * v33 + *Y;
	}
	if (!cdata.bTriright && cdata.bTriupper) {
		float v31 = 1.0 - fpy;
		NiColorA* Ya = NiColorA::Multiply(&v48, v31, &c1);
		NiColorA* v17 = NiColorA::Multiply(&v51, fpx, &c2);
		float v30 = 1.0 - fpx;
		NiColorA* v7 = NiColorA::Multiply(&v52, v30, &c0);
		aColor = (*v7 + *v17) * fpy + *Ya;
	}
	if (cdata.bTriright && !cdata.bTriupper) {
		NiColorA* Yb = NiColorA::Multiply(&v42, fpy, &c2);
		float v29 = 1.0 - fpy;
		NiColorA* v18 = NiColorA::Multiply(&v45, fpx, &c1);
		float v28 = 1.0 - fpx;
		NiColorA* v10 = NiColorA::Multiply(&v46, v28, &c0);
		aColor = (*v10 + *v18) * v29 + *Yb;
	}
	if (cdata.bTriright && cdata.bTriupper) {
		float v27 = 1.0 - fpy;
		NiColorA* Yc = NiColorA::Multiply(&v36, v27, &c1);
		NiColorA* v19 = NiColorA::Multiply(&v39, fpx, &c0);
		float v26 = 1.0 - fpx;
		NiColorA* v13 = NiColorA::Multiply(&v40, v26, &c2);
		aColor = (*v13 + *v19) * fpy + *Yc;
	}
	return 1;
}

bool TESObjectLAND::GetLandColor(const COORD_DATA& cdata, NiColorA& color) {
	GetColorA(cdata.iBlock, cdata.iVertidx, color);
	return 1;
}

void TESObjectLAND::InitDistantTextureBlending() {
	BGSTerrainManager* pterrainManager;
	NiPoint2 v16;
	NiPoint2 v17;
	NiPoint2 v18;
	NiPoint2 v19;
	NiPoint3 kcellcenter;
	int32_t v30 = 0;
	int32_t v33 = 0;
	if (TES::pInstance->pWorldSpace) {
		pterrainManager = TES::pInstance->pWorldSpace->GetTerrainManager();
	} else {
		pterrainManager = 0;
	}
	kcellcenter = NiPoint3_ZERO;
	kcellcenter.x = (static_cast<double>(pLoadedData->iCellX) + 0.5) * 4096.0;
	kcellcenter.y = (static_cast<double>(pLoadedData->iCellY) + 0.5) * 4096.0;
	NiPoint2 kcellOffset = NiPoint2::ZERO;
	for (int32_t i = 0; i < 4; ++i) {
		if (pLoadedData->ppMesh) {
			if (pterrainManager) {
				int32_t v13;
				NiAVObject* v1 = pLoadedData->ppMesh[i]->GetAtChecked(0);
				NiShadeProperty* pprop = static_cast<NiShadeProperty*>(v1->GetProperty(3));
				if (pprop) {
					int32_t v12 = pprop->iShaderPropertyType >= 8 && pprop->iShaderPropertyType <= 12;
					v13 = v12;
				} else {
					v13 = 0;
				}
				BSShaderPPLightingProperty* pshprop = v13 ? static_cast<BSShaderPPLightingProperty*>(pprop) : 0;
				if (pshprop) {
					if (!pshprop->GetDiffuseTextures(9) || !pshprop->GetNormalTexture(9u)) {
						FNVD_INLINED_LOCAL(NiColorA, kbaseCoord, std::nullopt, std::nullopt);
						NiTexture* pdiffuse = pterrainManager->GetDiffuseTextureForPoint(kcellcenter);
						NiTexture* pnormal = pterrainManager->GetNormalTextureForPoint(kcellcenter);
						pshprop->SetDiffuseTexture(9u, pdiffuse);
						pshprop->SetNormalTexture(9u, pnormal);
						kbaseCoord = NiColorA_BLACK;
						switch (i) {
						case 0: {
							NiPoint2* v2 = ::new (&v19) NiPoint2(0.0f, 0.0f);
							float y = v2->y;
							kcellOffset.x = v2->x;
							kcellOffset.y = y;
							break;
						}
						case 1: {
							NiPoint2* v4 = ::new (&v18) NiPoint2(0.5f, 0.0f);
							float v5 = v4->y;
							kcellOffset.x = v4->x;
							kcellOffset.y = v5;
							break;
						}
						case 2: {
							NiPoint2* v6 = ::new (&v17) NiPoint2(0.0f, 0.5f);
							float v7 = v6->y;
							kcellOffset.x = v6->x;
							kcellOffset.y = v7;
							break;
						}
						case 3: {
							NiPoint2* v8 = ::new (&v16) NiPoint2(0.5f, 0.5f);
							float v9 = v8->y;
							kcellOffset.x = v8->x;
							kcellOffset.y = v9;
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
							int32_t ilevel = pchunk->pNode->iLevel;
							float foriginX = static_cast<double>(pLoadedData->iCellX) - static_cast<double>(sx);
							float foriginY = static_cast<double>(pLoadedData->iCellY) - static_cast<double>(sy);
							double v11 = foriginX + kcellOffset.x;
							kbaseCoord.r = v11 / static_cast<double>(pchunk->pNode->iLevel);
							double v10 = foriginY + kcellOffset.y;
							kbaseCoord.g = v10 / static_cast<double>(pchunk->pNode->iLevel);
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
	}
}

float TESObjectLAND::GetWorldCellY() {
	return (double)(GetDataY() << 12);
}

TESWorldSpace* TESObjectLAND::GetWorldSpace() {
	if (pParentCell) {
		return pParentCell->GetWorldSpace();
	} else {
		return nullptr;
	}
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
	} else {
		return (float)0.0;
	}
}

// aiBlock is never read. The two sums are made with the double 2048.0.
NiPoint3 TESObjectLAND::GetWorldOffsetForBlock(int32_t aiBlock) {
	NiPoint3 kResult;
	kResult.~NiPoint3();
	::new (&kResult) NiPoint3;
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
	kPos.~NiPoint3();
	::new (&kPos) NiPoint3;
	kPos.x = 4096.0f;
	kPos.y = 4096.0f;
	int32_t iCellX = apCellX ? *apCellX : GetDataX();
	kPos.x = (float)iCellX * kPos.x;
	int32_t iCellY = apCellY ? *apCellY : GetDataY();
	kPos.y = (float)iCellY * kPos.y;
	kPos.x = (float)((acBlock & 1) != 0 ? 2048 : 0) + kPos.x;
	kPos.y = (float)((acBlock & 2) != 0 ? 2048 : 0) + kPos.y;
	kPos.x = (float)((asVertIndex % 17) << 7) + kPos.x;
	kPos.y = (float)((asVertIndex / 17) << 7) + kPos.y;
	kPos.z = 0.0f;
	NiPoint3 kResult = kPos;
	return kResult;
}

// The extents are unset when x is +FLT_MAX, or when y is -FLT_MAX: the first request then computes them from the four
// blocks when the land has them, and otherwise sets both to the world's default height. A NaN x falls to the y test, a
// NaN y returns the extents as they are. The compares are against the exact FLT_MAX values widened to double.
NiPoint2 TESObjectLAND::GetMinMaxLandHeight() {
	if (!pLoadedData) {
		return NiPoint2(3.4028235e+38f, -3.4028235e+38f);
	}
	float fMin = pLoadedData->HeightExtents.x;
	float fMax = pLoadedData->HeightExtents.y;
	if (!(fMin == 3.4028234663852886e+38)) {
		if (!(fMax == -3.4028234663852886e+38)) {
			return pLoadedData->HeightExtents;
		}
	}
	if ((Data.iFlags & 1) != 0) {
		for (int32_t i = 0; i < 4; ++i) {
			NiPoint2 kBlock = GetMinMaxBlockHeight(i);
			if (pLoadedData->HeightExtents.x > kBlock.x) {
				pLoadedData->HeightExtents.x = kBlock.x;
			}
			if (pLoadedData->HeightExtents.y < kBlock.y) {
				pLoadedData->HeightExtents.y = kBlock.y;
			}
		}
	} else {
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
	NiPoint2 result;
	NiPoint2 kMinMax(3.4028235e+38f, -3.4028235e+38f);
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
	result = kMinMax;
	return result;
}

bool TESObjectLAND::GetLandRemapped() {
	return ((((int32_t)Data.iFlags) & 1024) != 0);
}

bool TESObjectLAND::HasGoodNormals() {
	return ((((int32_t)Data.iFlags) & 16) != 0);
}

void TESObjectLAND::SetGoodNormals(bool abVal) {
	int32_t v2;
	if (abVal) {
		v2 = ((int32_t)Data.iFlags) | 16;
	} else {
		v2 = (int32_t)(((int32_t)Data.iFlags) & (~0x10u));
	}
	Data.iFlags = (uint32_t)v2;
}

NiNode* TESObjectLAND::GetQuadLandNode(int32_t aiBlock) {
	NiNode* v3 = nullptr;
	if (pParentCell) {
		if (aiBlock < 4) {
			return static_cast<NiNode*>(pParentCell->GetLandNode(aiBlock));
		}
	}
	return v3;
}

void TESObjectLAND::MarkLoaded(bool loaded) {
	int32_t v2;
	if (loaded) {
		v2 = ((int32_t)Data.iFlags) | 8;
	} else {
		v2 = (int32_t)(((int32_t)Data.iFlags) & (~8u));
	}
	Data.iFlags = (uint32_t)v2;
}

void TESObjectLAND::SetTexture(uint8_t acBlock, uint16_t asTexIndex, TESLandTexture* apTexture) {
	if ((((uint32_t)((int32_t)acBlock)) < 4u) && (((uint32_t)((int32_t)asTexIndex)) < 6u)) {
		if (pLoadedData) {
			pLoadedData->pQuadTextureArray[acBlock][asTexIndex] = apTexture;
		}
	}
}

float TESObjectLAND::GetDefaultTexturePercent(uint8_t acBlock, uint16_t asVertIndex) {
	int32_t i;
	float v7 = (float)0.0;
	if ((((uint32_t)((int32_t)acBlock)) < 4u) && (((uint32_t)((int32_t)asVertIndex)) < 0x121u)) {
		if (pLoadedData) {
			if (pLoadedData->ppPercentArrays[acBlock]) {
				for (i = 0; i < 6; ++i) {
					v7 = v7 + pLoadedData->ppPercentArrays[acBlock][asVertIndex][i];
				}
			}
		}
	}
	return (float)(1.0 - v7);
}

float TESObjectLAND::GetTexturePercent(uint8_t acBlock, uint16_t asVertIndex, uint16_t asTexIndex) {
	float v5 = (float)0.0;
	if (((((uint32_t)((int32_t)acBlock)) >= 4u) || (((uint32_t)((int32_t)asVertIndex)) >= 0x121u)) ||
		(((uint32_t)((int32_t)asTexIndex)) >= 6u)) {
		return v5;
	}
	if (!pLoadedData) {
		return v5;
	}
	if (pLoadedData->ppPercentArrays[acBlock]) {
		return pLoadedData->ppPercentArrays[acBlock][asVertIndex][asTexIndex];
	}
	return v5;
}

void TESObjectLAND::SetTexturePercent(uint8_t acBlock, uint16_t asVertIndex, uint16_t asTexIndex, float afPercent) {
	if (((((uint32_t)((int32_t)acBlock)) < 4u) && (((uint32_t)((int32_t)asVertIndex)) < 0x121u)) &&
		(((uint32_t)((int32_t)asTexIndex)) < 6u)) {
		if (pLoadedData) {
			if (!(afPercent > 0.0)) {
				if (pLoadedData->ppPercentArrays[acBlock]) {
					pLoadedData->ppPercentArrays[acBlock][asVertIndex][asTexIndex] = (float)0.0;
				}
			} else {
				pLoadedData->ppPercentArrays[acBlock][asVertIndex][asTexIndex] = afPercent;
			}
		}
	}
}

void TESObjectLAND::RemoveTextureFromArrays(uint8_t acBlock, uint16_t asRemoveIndex) {
	int32_t k;
	int32_t j;
	int32_t i;
	if ((((uint32_t)((int32_t)acBlock)) < 4u) && (((uint32_t)((int32_t)asRemoveIndex)) < 6u)) {
		if (pLoadedData) {
			for (i = (int32_t)asRemoveIndex; i < 5; ++i) {
				pLoadedData->pQuadTextureArray[acBlock][i] = pLoadedData->pQuadTextureArray[acBlock][i + 1];
			}
			pLoadedData->pQuadTextureArray[acBlock][5] = nullptr;
			if (pLoadedData->ppPercentArrays[acBlock]) {
				for (j = 0; j < 289; ++j) {
					for (k = (int32_t)asRemoveIndex; k < 5; ++k) {
						pLoadedData->ppPercentArrays[acBlock][j][k] = pLoadedData->ppPercentArrays[acBlock][j][k + 1];
					}
					pLoadedData->ppPercentArrays[acBlock][j][5] = (float)0.0;
				}
			}
		}
	}
}

bool TESObjectLAND::VertexInRect(tagRECT* apRect, const NiPoint3& aPoint) {
	NiPoint3 dst;
	if (!apRect) {
		return (bool)1;
	}
	float y = (float)(GetWorldCellY() + 2048.0);
	float x = (float)(GetWorldCellX() + 2048.0);
	::new (&dst) NiColor(x, y, 0.0);
	&(&dst)->operator+=(aPoint);
	if ((dst.x > ((double)apRect->right)) || (dst.y > ((double)apRect->top))) {
		return (bool)0;
	}
	return (bool)((uint8_t)((!(dst.x < ((double)apRect->left))) && (!(dst.y < ((double)apRect->bottom)))));
}

bool TESObjectLAND::IsNormalSet(int32_t coord, int32_t a2) {
	return pLoadedData->ppNormalsSet[coord][a2];
}

bool TESObjectLAND::IsNormalSet(const NiPoint3& coord) {
	COORD_DATA cdata;
	cdata.~COORD_DATA();
	::new (&cdata) COORD_DATA;
	return GetCoordData(&cdata, coord, false) && IsNormalSet(cdata.iBlock, cdata.iVertidx);
}

void TESObjectLAND::UpdateUseParentWorldspace(char a2) {
	int32_t v2;
	if (a2) {
		v2 = ((int32_t)Data.iFlags) | 1024;
	} else {
		v2 = (int32_t)(((int32_t)Data.iFlags) & (~0x400u));
	}
	Data.iFlags = (uint32_t)v2;
}

void TESObjectLAND::SetHasMopp(uint8_t a2) {
	int32_t v2;
	if (a2) {
		v2 = ((int32_t)Data.iFlags) | 2048;
	} else {
		v2 = (int32_t)(((int32_t)Data.iFlags) & (~0x800u));
	}
	Data.iFlags = (uint32_t)v2;
}

float TESObjectLAND::GetTextureUsePercent(uint8_t acBlock, uint16_t asTexIndex) {
	int32_t i;
	float v7 = (float)0.0;
	if ((((uint32_t)((int32_t)acBlock)) < 4u) && (((uint32_t)((int32_t)asTexIndex)) < 6u)) {
		if (GetQuadTexture(acBlock, asTexIndex)) {
			for (i = 0; i < 289; ++i) {
				v7 = GetTexturePercent(acBlock, i, asTexIndex) + v7;
			}
		}
	}
	return v7;
}

void TESObjectLAND::GetColorA(int32_t iblock, int32_t index, NiColorA& color) {
	if (HasVertices()) {
		char bGotVertexColors = 0;
		if (pLoadedData->ppColorsA && pLoadedData->ppColorsA[iblock]) {
			color = pLoadedData->ppColorsA[iblock][index];
			bGotVertexColors = 1;
		} else {
			if (pLoadedData->ppMesh) {
				if (pLoadedData->ppMesh[iblock]) {
					NiGeometry* shape = static_cast<NiGeometry*>(pLoadedData->ppMesh[iblock]->GetAt(0));
					if (shape->GetNormals()) {
						color = shape->GetColors()[index];
						bGotVertexColors = 1;
					} else {
						NiGeometryData* pgeodata = shape->GetModelData();
						NiTConvertingStrideIterator<NiColorA> apMemory;
						NiAdditionalGeometryData* pkAGD = pgeodata->GetAdditionalGeometryData();
						if (pkAGD) {
							if (pkAGD->GetVBReady() && pgeodata->LockPackedVertexData(1)) {
								pgeodata->GetColorsIterator(*static_cast<NiGeometryDataIteratorNiColorA*>(&apMemory));
								apMemory.Get(index, color);
								pgeodata->UnlockPackedVertexData();
								bGotVertexColors = 1;
							}
						}
					}
				}
			}
		}
		if (!bGotVertexColors) {
			color = NiColorA_WHITE;
		}
	} else {
		color = NiColorA_WHITE;
	}
}

NiTriStrips* TESObjectLAND::MakeLandTriStrips(uint8_t aiBlock) {
	NiTriStrips* ptrigeom = 0;
	if (aiBlock < 4u) {
		if (pLoadedData) {
			NiTriStrips* ptrigeom_2;
			NiPoint3* pnormalcopy_2;
			NiPoint2* ptextures_2;
			char apDest[264];
			NiPoint2* ptextures = static_cast<NiPoint2*>(ourMalloc(289 * sizeof(NiPoint2)));
			if (ptextures) {
				rt::eh_vector_constructor_iterator(ptextures, sizeof(NiPoint2), 289, &emptyPointConstructor);
				ptextures_2 = ptextures;
			} else {
				ptextures_2 = 0;
			}
			ourMemcpy(ptextures_2, TESObjectLAND::pDefTextureBLOCK, sizeof(NiPoint2) * 289);
			uint16_t* pstriplength = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t)));
			uint16_t* ptriplist = static_cast<uint16_t*>(_NiMalloc(1021 * sizeof(uint16_t)));
			*pstriplength = 1021;
			ourMemcpy(ptriplist, TESObjectLAND::pTriStripListBLOCK, sizeof(uint16_t) * 1021);
			NiPoint3* pnormalcopy = static_cast<NiPoint3*>(ourMalloc(289 * sizeof(NiPoint3)));
			if (pnormalcopy) {
				rt::eh_vector_constructor_iterator(pnormalcopy, sizeof(NiPoint3), 289, &emptyPointConstructor);
				pnormalcopy_2 = pnormalcopy;
			} else {
				pnormalcopy_2 = 0;
			}
			ourMemcpy(pnormalcopy_2, pLoadedData->ppNormals[aiBlock], sizeof(NiPoint3) * 289);
			void* apMemory = NiMemObject::operator new(sizeof(NiTriStrips));
			if (apMemory) {
				ptrigeom_2 = ::new (apMemory) NiTriStrips(289, pLoadedData->ppVertices[aiBlock], pnormalcopy_2,
					pLoadedData->ppColorsA[aiBlock], ptextures_2, 1, 0, *pstriplength - 2, 1, pstriplength, ptriplist);
			} else {
				ptrigeom_2 = 0;
			}
			ptrigeom = ptrigeom_2;
			NiPoint3 blockcenter = GetWorldOffsetForBlock(aiBlock);
			ptrigeom->SetTranslate(blockcenter);
			BSsprintf(apDest, 260, "Block (%i, %i)", aiBlock & 3, aiBlock / 4);
			NiFixedString apString(static_cast<const char*>(apDest));
			NiFixedString* kName = &apString;
			ptrigeom->SetName(*kName);
		}
	}
	return ptrigeom;
}

bool TESObjectLAND::InitLandscape() {
	AutoMemContext tempMemContext(MEM_CONTEXT_LAND, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESLand.cpp", 3236);
	if (pLoadedData && (pLoadedData->ppMesh && *pLoadedData->ppMesh)) {
		int32_t i;
		NiColorA kDefColor(1.0f, 1.0f, 1.0f, 0.0f);
		if ((Data.iFlags & 2) == 0) {
			for (i = 0; i < 4; ++i) {
				for (int32_t j = 0; j < 17; ++j) {
					for (int32_t k = 0; k < 17; ++k) {
						NiColorA* v2 = &pLoadedData->ppColorsA[i][17 * j + k];
						v2->r = kDefColor.r;
						v2->g = kDefColor.g;
						v2->b = kDefColor.b;
						v2->a = kDefColor.a;
					}
				}
			}
		}
		MarkLoaded(1);
		for (i = 0; i < 4; ++i) {
			BSShaderPPLightingProperty* pshprop;
			// The address &pLoadedData->ppPercentArrays is tested here, and it is never null.
			NiBinaryExtraData* ppercentED;
			NiBinaryExtraData* v19 =
				static_cast<NiBinaryExtraData*>(NiMemObject::operator new(sizeof(NiBinaryExtraData)));
			if (v19) {
				// The block's percent buffer, 289 vertices of 8 floats, which the extra data keeps as untyped bytes.
				ppercentED = ::new (v19) NiBinaryExtraData(289 * 8 * sizeof(float),
					reinterpret_cast<char*>(*pLoadedData->ppPercentArrays[i]));
			} else {
				ppercentED = 0;
			}
			NiFixedString* TexPercTag = BSShaderManager::TexPercTag();
			pLoadedData->ppMesh[i]->AddExtraData(*TexPercTag, ppercentED);
			BSShaderPPLightingProperty* v18 =
				static_cast<BSShaderPPLightingProperty*>(NiMemObject::operator new(sizeof(BSShaderPPLightingProperty)));
			if (v18) {
				pshprop = ::new (v18) BSShaderPPLightingProperty;
			} else {
				pshprop = 0;
			}
			pshprop->SetShader(1);
			if (pshprop) {
				uint8_t v9;
				uint8_t v10;
				uint8_t v11;
				uint8_t v12;
				uint8_t v13;
				uint8_t SpecularExponent;
				TESLandTexture* pQuadTex = pLoadedData->pDefQuadTexture[i];
				if (pQuadTex && pQuadTex->pTextureSet) {
					BSTextureSet* AsShaderTextureSet = pQuadTex->pTextureSet->GetAsShaderTextureSet();
					pshprop->SetLandscapeTextureSet(0, AsShaderTextureSet);
				} else {
					TESLandTexture* DefaultTexture = TESObjectLAND::GetDefaultTexture();
					BSTextureSet* v6 = DefaultTexture->pTextureSet->GetAsShaderTextureSet();
					pshprop->SetLandscapeTextureSet(0, v6);
				}
				for (int32_t itex = 1; itex < 7; ++itex) {
					TESLandTexture* pQuadTexa = pLoadedData->pQuadTextureArray[i][itex - 1];
					if (pQuadTexa && pQuadTexa->pTextureSet) {
						BSTextureSet* v7 = pQuadTexa->pTextureSet->GetAsShaderTextureSet();
						pshprop->SetLandscapeTextureSet(static_cast<uint32_t>(itex), v7);
					} else {
						pshprop->SetLandscapeTextureSet(static_cast<uint32_t>(itex), 0);
					}
				}
				if (pLoadedData->pQuadTextureArray[i][5]) {
					SpecularExponent = pLoadedData->pQuadTextureArray[i][5]->GetSpecularExponent();
				} else {
					SpecularExponent = 0;
				}
				if (pLoadedData->pQuadTextureArray[i][4]) {
					v13 = pLoadedData->pQuadTextureArray[i][4]->GetSpecularExponent();
				} else {
					v13 = 0;
				}
				if (pLoadedData->pQuadTextureArray[i][3]) {
					v12 = pLoadedData->pQuadTextureArray[i][3]->GetSpecularExponent();
				} else {
					v12 = 0;
				}
				if (pLoadedData->pQuadTextureArray[i][2]) {
					v11 = pLoadedData->pQuadTextureArray[i][2]->GetSpecularExponent();
				} else {
					v11 = 0;
				}
				if (pLoadedData->pQuadTextureArray[i][1]) {
					v10 = pLoadedData->pQuadTextureArray[i][1]->GetSpecularExponent();
				} else {
					v10 = 0;
				}
				if (*pLoadedData->pQuadTextureArray[i]) {
					v9 = (*pLoadedData->pQuadTextureArray[i])->GetSpecularExponent();
				} else {
					v9 = 0;
				}
				if (pLoadedData->pDefQuadTexture[i]) {
					uint8_t v8 = pLoadedData->pDefQuadTexture[i]->GetSpecularExponent();
					pshprop->SetLandscapeSpecularExponents(v8, v9, v10, v11, v12, v13, SpecularExponent, 0, 0, 0);
				} else {
					pshprop->SetLandscapeSpecularExponents(0, v9, v10, v11, v12, v13, SpecularExponent, 0, 0, 0);
				}
				NiGeometry* pgeom = static_cast<NiGeometry*>(pLoadedData->ppMesh[i]->GetAtChecked(0));
				BSShaderPPLightingProperty::TangentSpaceData* ptangentspace =
					BSShaderManager::CreateTangentSpaceSimple(pgeom);
				pshprop->StoreExtraDataPointers(ptangentspace);
				NiAdditionalGeometryData* pdata = pshprop->CreateRendererSpecificProperty(pgeom);
				pgeom->SetAdditionalGeometryData(pdata);
				pgeom->AttachProperty(pshprop);
				BSShaderManager::PrepareObject(pLoadedData->ppMesh[i], 0, 0);
			}
		}
		return 1;
	} else {
		return 0;
	}
}

// The buffer holds one total per texture and one for the default texture after them.
void TESObjectLAND::AdjustTextureArrays() {
	if (pLoadedData) {
		for (int32_t iblock = 0; iblock < 4; ++iblock) {
			for (uint32_t itex = 0; itex < 6; ++itex) {
				if (pLoadedData->pQuadTextureArray[iblock][itex]) {
					float fblockuse = GetTextureUsePercent(iblock, itex);
					float v9 = fblockuse / 289.0;
					if (v9 < 0.0010000000474974513) {
						pLoadedData->pQuadTextureArray[iblock][itex] = 0;
					}
				}
			}
			uint32_t itexcount = 6;
			for (uint32_t itex_2 = 0; itex_2 < itexcount; ++itex_2) {
				while (!pLoadedData->pQuadTextureArray[iblock][itex_2] && itex_2 < itexcount) {
					RemoveTextureFromArrays(iblock, itex_2);
					--itexcount;
				}
			}
			float* ptotaluse = static_cast<float*>(ourMalloc(sizeof(float) * itexcount + sizeof(float)));
			ourMemset(ptotaluse, 0, sizeof(float) * itexcount + sizeof(float));
			for (int32_t ivert = 0; ivert < 289; ++ivert) {
				float fnondefault = 0.0f;
				for (uint32_t itex_3 = 0; itex_3 < itexcount; ++itex_3) {
					float fuse = GetTexturePercent(iblock, ivert, itex_3);
					ptotaluse[itex_3] = ptotaluse[itex_3] + fuse;
					fnondefault = fnondefault + fuse;
				}
				if (fnondefault < 1.0) {
					ptotaluse[itexcount] = 1.0 - fnondefault + ptotaluse[itexcount];
				}
			}
			int32_t inewdefault = -1;
			float fmax = ptotaluse[itexcount];
			for (uint32_t itex_4 = 0; itex_4 < itexcount; ++itex_4) {
				if (fmax < static_cast<double>(ptotaluse[itex_4])) {
					fmax = ptotaluse[itex_4];
					inewdefault = itex_4;
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
		float apDest[289];
		TESLandTexture* DefaultTexture_0 = GetDefaultTexture(i);
		BaseFormComponent* v20 = static_cast<BaseFormComponent*>(FNVD_DYNAMIC_CAST_VOID(TESTexture)(DefaultTexture_0,
			RTTI_TESLandTexture, RTTI_TESTexture));
		ourMemset(apDest, 0, sizeof(apDest));
		for (int32_t j = 0; j < 6; ++j) {
			TESLandTexture* QuadTexture = GetQuadTexture(i, j);
			TESTexture* v17 = dynamic_cast<TESTexture*>(QuadTexture);
			if (v17) {
				for (int32_t k = 0; k < 6; ++k) {
					if (j != k) {
						TESLandTexture* v14 = GetQuadTexture(i, k);
						BaseFormComponent* apCompareFC =
							static_cast<BaseFormComponent*>(FNVD_DYNAMIC_CAST_VOID(TESTexture)(v14, RTTI_TESLandTexture,
								RTTI_TESTexture));
						if (apCompareFC) {
							if (!v17->TESTexture::CompareComponent(apCompareFC)) {
								for (int32_t m = 0; m < 289; ++m) {
									pLoadedData->ppPercentArrays[i][m][j] =
										pLoadedData->ppPercentArrays[i][m][j] + pLoadedData->ppPercentArrays[i][m][k];
									pLoadedData->ppPercentArrays[i][m][k] = 0.0f;
								}
								SetTexture(i, k, 0);
							}
						}
					}
				}
				if (v20) {
					if (!v17->TESTexture::CompareComponent(v20)) {
						for (int32_t n = 0; n < 289; ++n) {
							pLoadedData->ppPercentArrays[i][n][j] = 0.0f;
						}
						SetTexture(i, j, 0);
					}
				}
			}
			for (int32_t ii = 0; ii < 289; ++ii) {
				apDest[ii] = apDest[ii] + pLoadedData->ppPercentArrays[i][ii][j];
			}
		}
		uint32_t v22 = 6;
		for (uint32_t jj = 0; jj < v22; ++jj) {
			while (!pLoadedData->pQuadTextureArray[i][jj] && jj < v22) {
				RemoveTextureFromArrays(i, jj);
				--v22;
			}
		}
		for (int32_t kk = 0; kk < 289; ++kk) {
			float v7 = 0.0f;
			for (int32_t mm = 6; mm; --mm) {
				pLoadedData->ppPercentArrays[i][kk][mm] = pLoadedData->ppPercentArrays[i][kk][mm - 1];
				v7 = v7 + pLoadedData->ppPercentArrays[i][kk][mm];
			}
			float v3 = 1.0 - v7;
			float a2 = BSMAX(0.0f, v3);
			double v1 = BSMIN(static_cast<float>(1.0), a2);
			*pLoadedData->ppPercentArrays[i][kk] = v1;
			if (apDest[kk] > 1.0) {
				for (int32_t nn = 6; nn; --nn) {
					pLoadedData->ppPercentArrays[i][kk][nn] = pLoadedData->ppPercentArrays[i][kk][nn] / v7;
				}
			}
		}
	}
}

char TESObjectLAND::CalculateGrassParams() {
	double v22;
	double v23;
	if (!pLoadedData) {
		return 0;
	}
	float v40 = gs_fTexturePctThreshold_Grass.Float();
	if (!(v40 < 0.0)) {
		v23 = v40;
	} else {
		v23 = 0.0;
	}
	float v41 = v23;
	if (!(v41 > 1.0)) {
		v22 = v41;
	} else {
		v22 = 0.9;
	}
	float v42 = v22;
	int32_t iMaxGrassTypesPerTexure = gs_iMaxGrassTypesPerTexure_Grass.Int();
	TESObjectLAND::iGrassEvalSize = gs_iGrassDensityEvalSize_Grass.Int();
	if (TESObjectLAND::iGrassEvalSize != 1 && TESObjectLAND::iGrassEvalSize != 2 &&
		TESObjectLAND::iGrassEvalSize != 4 && TESObjectLAND::iGrassEvalSize != 8) {
		TESObjectLAND::iGrassEvalSize = 2;
	}
	for (int32_t i = 0; i < 4; ++i) {
		int32_t iParam = 0;
		for (int32_t j = TESObjectLAND::iGrassEvalSize; j < 16; j += 2 * TESObjectLAND::iGrassEvalSize) {
			for (int32_t k = TESObjectLAND::iGrassEvalSize; k < 16; k += 2 * TESObjectLAND::iGrassEvalSize) {
				TESGrassAreaParam** apParam =
					static_cast<TESGrassAreaParam**>(ourMalloc(16 * sizeof(TESGrassAreaParam*)));
				ourMemset(apParam, 0, sizeof(TESGrassAreaParam*) * 16);
				int32_t aiVertIndex = j + 17 * k;
				if (pLoadedData->pDefQuadTexture[i]) {
					BSSimpleList<TESGrass*>* pList = &pLoadedData->pDefQuadTexture[i]->TextureGrassList;
					int32_t iType = 0;
					while (pList && (iParam < 16 && iType <= iMaxGrassTypesPerTexure)) {
						TESGrass* m_item = pList->m_item;
						if (m_item) {
							float v12;
							float v13;
							float v14;
							float v15;
							float v16;
							float v17;
							float v18;
							float v19;
							float v20;
							TESGrassAreaParam* v21;
							++iType;
							TESGrassAreaParam* apMemory =
								static_cast<TESGrassAreaParam*>(ourMalloc(sizeof(TESGrassAreaParam)));
							if (apMemory) {
								v21 = ::new (apMemory) TESGrassAreaParam;
							} else {
								v21 = 0;
							}
							apParam[iParam] = v21;
							ourMemset(apParam[iParam], 0, sizeof(TESGrassAreaParam));
							apParam[iParam]->kGrassParam.pcGeomFilename = m_item->GetModelPath();
							apParam[iParam]->kGrassParam.iGrassID = m_item->iFormID;
							apParam[iParam]->kGrassParam.fWaveOffsetRange = TESGrass::GetWaveOffsetRange();
							apParam[iParam]->kGrassParam.fWavePeriod = m_item->GetWavePeriod();
							apParam[iParam]->kGrassParam.fColorRange = m_item->GetColorRange();
							apParam[iParam]->kGrassParam.fPositionRange = m_item->GetPositionRange();
							apParam[iParam]->kGrassParam.fHeightRange = m_item->GetHeightRange();
							apParam[iParam]->kGrassParam.bVertexLighting = m_item->GetVertexLighting();
							apParam[iParam]->kGrassParam.bFitToSlope = m_item->GetFitToSlope();
							apParam[iParam]->kGrassParam.bUniformScaing = m_item->GetUniformScaling();
							float v45 = static_cast<double>(m_item->GetDensity()) / 100.0;
							if (!(static_cast<double>(v42) < GetDefaultTexturePercent(i, aiVertIndex - 18))) {
								v20 = 0.0f;
							} else {
								v20 = v45;
							}
							apParam[iParam]->pfDensity[0u] = v20;
							if (!(static_cast<double>(v42) < GetDefaultTexturePercent(i, aiVertIndex - 17))) {
								v19 = 0.0f;
							} else {
								v19 = v45;
							}
							apParam[iParam]->pfDensity[1] = v19;
							if (!(static_cast<double>(v42) < GetDefaultTexturePercent(i, aiVertIndex - 16))) {
								v18 = 0.0f;
							} else {
								v18 = v45;
							}
							apParam[iParam]->pfDensity[2] = v18;
							if (!(static_cast<double>(v42) < GetDefaultTexturePercent(i, aiVertIndex - 1))) {
								v17 = 0.0f;
							} else {
								v17 = v45;
							}
							apParam[iParam]->pfDensity[3] = v17;
							if (!(static_cast<double>(v42) < GetDefaultTexturePercent(i, aiVertIndex))) {
								v16 = 0.0f;
							} else {
								v16 = v45;
							}
							apParam[iParam]->pfDensity[4] = v16;
							if (!(static_cast<double>(v42) < GetDefaultTexturePercent(i, aiVertIndex + 1))) {
								v15 = 0.0f;
							} else {
								v15 = v45;
							}
							apParam[iParam]->pfDensity[5] = v15;
							if (!(static_cast<double>(v42) < GetDefaultTexturePercent(i, aiVertIndex + 16))) {
								v14 = 0.0f;
							} else {
								v14 = v45;
							}
							apParam[iParam]->pfDensity[6] = v14;
							if (!(static_cast<double>(v42) < GetDefaultTexturePercent(i, aiVertIndex + 17))) {
								v13 = 0.0f;
							} else {
								v13 = v45;
							}
							apParam[iParam]->pfDensity[7] = v13;
							if (!(static_cast<double>(v42) < GetDefaultTexturePercent(i, aiVertIndex + 18))) {
								v12 = 0.0f;
							} else {
								v12 = v45;
							}
							apParam[iParam]->pfDensity[8] = v12;
							float v43 = 0.0f;
							for (int32_t m = 0; m < 9; ++m) {
								v43 = v43 + apParam[iParam]->pfDensity[m];
							}
							if (v42 > v43 / 9.0) {
								for (int32_t n = 0; n < 9; ++n) {
									apParam[iParam]->pfDensity[n] = 0.0f;
								}
							}
							++iParam;
						}
						pList = pList->m_pkNext;
					}
				}
				for (int32_t ii = 0; ii < 6; ++ii) {
					if (pLoadedData->pQuadTextureArray[i][ii]) {
						if (pLoadedData->ppPercentArrays[i]) {
							if (pLoadedData->ppPercentArrays[i][aiVertIndex - 18][ii] > 0.10000000149011612 ||
								(pLoadedData->ppPercentArrays[i][aiVertIndex - 17][ii] > 0.10000000149011612 ||
									(pLoadedData->ppPercentArrays[i][aiVertIndex - 16][ii] > 0.10000000149011612 ||
										(pLoadedData->ppPercentArrays[i][aiVertIndex - 1][ii] > 0.10000000149011612 ||
											(pLoadedData->ppPercentArrays[i][aiVertIndex][ii] > 0.10000000149011612 ||
												(pLoadedData->ppPercentArrays[i][aiVertIndex + 1][ii] >
														0.10000000149011612 ||
													(pLoadedData->ppPercentArrays[i][aiVertIndex + 16][ii] >
															0.10000000149011612 ||
														(pLoadedData->ppPercentArrays[i][aiVertIndex + 17][ii] >
																0.10000000149011612 ||
															pLoadedData->ppPercentArrays[i][aiVertIndex + 18][ii] >
																0.10000000149011612)))))))) {
								int32_t iTypea = 0;
								for (BSSimpleList<TESGrass*>* pLista =
										 &pLoadedData->pQuadTextureArray[i][ii]->TextureGrassList;
									pLista && (iParam < 16 && iTypea <= iMaxGrassTypesPerTexure);
									pLista = pLista->m_pkNext) {
									TESGrass* v48 = pLista->m_item;
									if (v48) {
										float v2;
										float v3;
										float v4;
										float v5;
										float v6;
										float v7;
										float v8;
										float v9;
										float v10;
										TESGrassAreaParam* v11;
										TESGrassAreaParam* v25 =
											static_cast<TESGrassAreaParam*>(ourMalloc(sizeof(TESGrassAreaParam)));
										if (v25) {
											v11 = ::new (v25) TESGrassAreaParam;
										} else {
											v11 = 0;
										}
										apParam[iParam] = v11;
										++iTypea;
										ourMemset(apParam[iParam], 0, sizeof(TESGrassAreaParam));
										ourMemset(apParam[iParam]->pfDensity, 0, sizeof(apParam[iParam]->pfDensity));
										apParam[iParam]->kGrassParam.pcGeomFilename = v48->GetModelPath();
										apParam[iParam]->kGrassParam.iGrassID = v48->iFormID;
										apParam[iParam]->kGrassParam.fWaveOffsetRange = TESGrass::GetWaveOffsetRange();
										apParam[iParam]->kGrassParam.fWavePeriod = v48->GetWavePeriod();
										apParam[iParam]->kGrassParam.fColorRange = v48->GetColorRange();
										apParam[iParam]->kGrassParam.fPositionRange = v48->GetPositionRange();
										apParam[iParam]->kGrassParam.fHeightRange = v48->GetHeightRange();
										apParam[iParam]->kGrassParam.bVertexLighting = v48->GetVertexLighting();
										apParam[iParam]->kGrassParam.bFitToSlope = v48->GetFitToSlope();
										apParam[iParam]->kGrassParam.bUniformScaing = v48->GetUniformScaling();
										float v46 = static_cast<double>(v48->GetDensity()) / 100.0;
										if (!(v42 < static_cast<double>(pLoadedData
															->ppPercentArrays[i][aiVertIndex - 16][ii]))) {
											v10 = 0.0f;
										} else {
											v10 = v46;
										}
										apParam[iParam]->pfDensity[2] = v10;
										if (!(v42 < static_cast<double>(pLoadedData
															->ppPercentArrays[i][aiVertIndex - 17][ii]))) {
											v9 = 0.0f;
										} else {
											v9 = v46;
										}
										apParam[iParam]->pfDensity[1] = v9;
										if (!(v42 < static_cast<double>(pLoadedData
															->ppPercentArrays[i][aiVertIndex - 18][ii]))) {
											v8 = 0.0f;
										} else {
											v8 = v46;
										}
										apParam[iParam]->pfDensity[0u] = v8;
										if (!(v42 < static_cast<double>(pLoadedData
															->ppPercentArrays[i][aiVertIndex + 1][ii]))) {
											v7 = 0.0f;
										} else {
											v7 = v46;
										}
										apParam[iParam]->pfDensity[5] = v7;
										if (!(v42 < static_cast<double>(pLoadedData
															->ppPercentArrays[i][aiVertIndex][ii]))) {
											v6 = 0.0f;
										} else {
											v6 = v46;
										}
										apParam[iParam]->pfDensity[4] = v6;
										if (!(v42 < static_cast<double>(pLoadedData
															->ppPercentArrays[i][aiVertIndex - 1][ii]))) {
											v5 = 0.0f;
										} else {
											v5 = v46;
										}
										apParam[iParam]->pfDensity[3] = v5;
										if (!(v42 < static_cast<double>(pLoadedData
															->ppPercentArrays[i][aiVertIndex + 18][ii]))) {
											v4 = 0.0f;
										} else {
											v4 = v46;
										}
										apParam[iParam]->pfDensity[8] = v4;
										if (!(v42 < static_cast<double>(pLoadedData
															->ppPercentArrays[i][aiVertIndex + 17][ii]))) {
											v3 = 0.0f;
										} else {
											v3 = v46;
										}
										apParam[iParam]->pfDensity[7] = v3;
										if (!(v42 < static_cast<double>(pLoadedData
															->ppPercentArrays[i][aiVertIndex + 16][ii]))) {
											v2 = 0.0f;
										} else {
											v2 = v46;
										}
										apParam[iParam]->pfDensity[6] = v2;
										float v44 = 0.0f;
										for (int32_t jj = 0; jj < 9; ++jj) {
											v44 = v44 + apParam[iParam]->pfDensity[jj];
										}
										if (v42 > v44 / 9.0) {
											for (int32_t kk = 0; kk < 9; ++kk) {
												apParam[iParam]->pfDensity[kk] = 0.0f;
											}
										}
										++iParam;
									}
								}
							}
						}
					}
				}
				if (*apParam) {
					pLoadedData->pmGrassMap[i].SetAt(aiVertIndex, apParam);
				} else {
					ourFree(apParam);
				}
				iParam = 0;
			}
		}
	}
	return 1;
}

bool TESObjectLAND::ForceGoodNormals() {
	char ptr[110980];
	LANDS8 L8;
	bool pbloaded[8];
	int32_t i;
	if (!IsLoaded() || GetLandRemapped()) {
		return 0;
	}
	ourMemset(pbloaded, 0, 8u);
	FillLands8(&L8, 1, pbloaded);
	TESObjectLAND** plandarray = &(&L8)->pNWLand;
	rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), 0x2420, &emptyPointConstructor);
	for (i = 0; i < 8; ++i) {
		if (plandarray[i]) {
			for (int32_t j = 0; j < 4; ++j) {
				NiPoint3** NormalArray = plandarray[i]->GetNormalArray();
				ourMemcpy(&ptr[0x3630 * i + 3468 * j], NormalArray[j], 3468u);
			}
			plandarray[i]->InvalidateNormals(0);
		}
	}
	InvalidateNormals(0);
	SetLandNormals(&L8, 0);
	for (i = 0; i < 4; ++i) {
		for (int32_t k = 0; k < 289; ++k) {
			pLoadedData->ppNormals[i][k].Unitize();
			char v8 = static_cast<int32_t>(pLoadedData->ppNormals[i][k].y * 127.0);
			int32_t v3 = static_cast<int32_t>(pLoadedData->ppNormals[i][k].z * 127.0);
			pLoadedData->ppNormals[i][k].x =
				static_cast<double>(static_cast<char>(static_cast<int32_t>(pLoadedData->ppNormals[i][k].x * 127.0))) /
				127.0;
			pLoadedData->ppNormals[i][k].y = static_cast<double>(v8) / 127.0;
			pLoadedData->ppNormals[i][k].z = static_cast<double>(static_cast<char>(v3)) / 127.0;
			pLoadedData->ppNormals[i][k].Unitize();
		}
	}
	for (i = 0; i < 8; ++i) {
		if (plandarray[i]) {
			for (int32_t m = 0; m < 4; ++m) {
				char* v5 = &ptr[0x3630 * i + 3468 * m];
				NiPoint3** v4 = plandarray[i]->GetNormalArray();
				ourMemcpy(v4[m], v5, sizeof(NiPoint3) * 289);
			}
			if (pbloaded[i]) {
				plandarray[i]->UnLoadVertices();
			}
		}
	}
	return 1;
}

void TESObjectLAND::GetVertex(int32_t iblock, int32_t index, NiPoint3& vert) {
	float fHeight;
	NiPoint3 v7;
	NiPoint3 dst;
	if (HasVertices()) {
		if (pLoadedData->ppVertices && pLoadedData->ppVertices[iblock]) {
			vert = pLoadedData->ppVertices[iblock][index];
		} else {
			if (pLoadedData->ppMesh && pLoadedData->ppMesh[iblock]) {
				NiGeometry* v9 = static_cast<NiGeometry*>(pLoadedData->ppMesh[iblock]->GetAt(0));
				vert = v9->GetVertices()[index];
			} else {
				fHeight = GetDefaultWorldHeight();
				vert = *::new (&v7) NiPoint3(static_cast<float>(0.0), static_cast<float>(0.0), fHeight);
			}
		}
		vert.z = vert.z + pLoadedData->fBaseHeight;
	} else {
		fHeight = GetDefaultWorldHeight();
		vert = *::new (&dst) NiPoint3(static_cast<float>(0.0), static_cast<float>(0.0), fHeight);
	}
}

void TESObjectLAND::GetNormal(int32_t iblock, int32_t index, NiPoint3& norm) {
	NiPoint3 v5;
	NiPoint3 dst;
	if (HasVertices()) {
		bool bset = 0;
		if (pLoadedData->ppNormals && pLoadedData->ppNormals[iblock]) {
			norm = pLoadedData->ppNormals[iblock][index];
			bset = 1;
		} else {
			if (pLoadedData->ppMesh) {
				if (pLoadedData->ppMesh[iblock]) {
					NiGeometry* shape = static_cast<NiGeometry*>(pLoadedData->ppMesh[iblock]->GetAt(0));
					if (shape->GetNormals()) {
						norm = shape->GetNormals()[index];
						bset = 1;
					} else {
						NiGeometryData* pgeodata = shape->GetModelData();
						NiTConvertingStrideIterator<NiPoint3> apMemory;
						NiAdditionalGeometryData* pkAGD = pgeodata->GetAdditionalGeometryData();
						if (pkAGD) {
							if (pkAGD->GetVBReady() && pgeodata->LockPackedVertexData(1)) {
								pgeodata->GetNormalsIterator(*static_cast<NiGeometryDataIteratorNiPoint3*>(&apMemory));
								apMemory.Get(index, norm);
								pgeodata->UnlockPackedVertexData();
								bset = 1;
							}
						}
					}
				}
			}
		}
		if (!bset) {
			norm = *::new (&v5) NiPoint3(0.0f, 0.0f, 1.0f);
		}
	} else {
		norm = *::new (&dst) NiPoint3(0.0f, 0.0f, 1.0f);
	}
}

// The height, normal, colour and DATA buffers are zeroed once, at entry, because GetChunkData writes only the chunk's
// bytes and they would otherwise be read uninitialised when a chunk is empty or short (malformed data).
bool TESObjectLAND::LoadVerticesIntoArrays() {
	NiColorA kColor;
	NiPoint2 v42;
	FILE_TEX_DATA texdata_2;
	FILE_TEX_DATA texdata;
	FILE_HEIGHT_DATA kHeightData = {};
	OBJ_LAND tempdata = {};
	int8_t acNormals[3276] = {};
	uint8_t v55[3272] = {};
	if (IsLoaded()) {
		return 1;
	}
	int32_t ilastblock = -1;
	int32_t ilasttexindex = -1;
	bool bfailed = 0;
	TESFile* pFile = 0;
	uint32_t ifindoffset = 0;
	if (GetLandRemapped()) {
		TESWorldSpace* pworld = GetWorldSpace();
		if (!pworld) {
			return 1;
		}
		int32_t iCellY = GetDataY();
		int32_t DataX = GetDataX();
		if (!pworld->FindLandDataInFile(DataX, iCellY, pFile, ifindoffset)) {
			return 1;
		}
	} else {
		if ((Data.iFlags & 1) == 0 && ((Data.iFlags & 2) == 0 && (Data.iFlags & 4) == 0)) {
			return 1;
		}
		TESFile* File = GetFile(-1);
		pFile = File->GetThreadSafeFile();
		if (!pFile->FindForm(this)) {
			bfailed = 1;
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
		const char* Filename;
		MessageHandler::IncDisableWarningCount(0);
		if (pFile) {
			Filename = pFile->GetFilename();
		} else {
			Filename = "UNKNOWN";
		}
		int32_t fB = pParentCell->TESObjectCELL::GetDataY();
		int32_t iCellYa = pParentCell->TESObjectCELL::GetDataX();
		Warning("CELLS: Failed to load landscape data for LAND (%08X) in Cell (%i, %i) from file '%s'.", iFormID,
			iCellYa, fB, Filename);
		MessageHandler::IncDisableWarningCount(1);
		return 0;
	} else {
		TESFile* pFile_2 = pFile;
		if (pFile->SetOffset(ifindoffset)) {
			bool loadedhgt = 0;
			bool loadednorms = 0;
			bool badjusttextures = 0;
			do {
				CHUNK_ID eChunk = pFile_2->GetTESChunk();
				if (eChunk > VHGT_ID) {
					if (eChunk == ATXT_ID) {
						if ((Data.iFlags & 4) != 0) {
							ourMemset(&texdata_2, 0, sizeof(FILE_TEX_DATA));
							pFile_2->GetChunkData(&texdata_2, sizeof(FILE_TEX_DATA));
							if (pFile_2->QEndian()) {
								texdata_2.Endian();
							}
							if (texdata_2.sIndex > 5u) {
								int32_t a6b = texdata_2.cBlock;
								int32_t fBb = texdata_2.sIndex;
								int32_t iCellYc = GetDataY();
								Warning("CELLS: Land (%i, %i) clamping invalid index %i for block %i.", GetDataX(),
									iCellYc, fBb, a6b);
								texdata_2.sIndex = 5;
								badjusttextures = 1;
							}
							TESLandTexture* ptex_2 = 0;
							if (texdata_2.iTextureID) {
								TESForm::AddCompileIndex(texdata_2.iTextureID, pFile_2);
								TESForm* FormByNumericID = TESForm::GetFormByNumericID(texdata_2.iTextureID);
								ptex_2 = dynamic_cast<TESLandTexture*>(FormByNumericID);
								if (!ptex_2) {
									int32_t a6c = texdata_2.cBlock;
									uint32_t fBc = texdata_2.iTextureID;
									int32_t iCellYd = GetDataY();
									Warning("TEXTURES: Land (%i, %i) unable to find additional texture ID (%08X) for "
											"block %i.",
										GetDataX(), iCellYd, fBc, a6c);
									badjusttextures = 1;
								}
							} else {
								ptex_2 = TESObjectLAND::GetDefaultTexture();
							}
							this->pLoadedData->pQuadTextureArray[texdata_2.cBlock][texdata_2.sIndex] = ptex_2;
							if (ptex_2) {
								if (!spQueuedTextures.m_pObject) {
									QueuedFile* v31;
									QueuedFile* v39 = static_cast<QueuedFile*>(ourMalloc(sizeof(QueuedFile)));
									if (v39) {
										v31 = ::new (v39) QueuedFile(IO_TASK_PRIORITY_VERY_LOW);
									} else {
										v31 = 0;
									}
									spQueuedTextures.operator=(v31);
								}
								QueuedFile* a6d = spQueuedTextures.m_pObject;
								ptex_2->QueueTexture(IO_TASK_PRIORITY_VERY_LOW, a6d);
							}
							ilastblock = texdata_2.cBlock;
							ilasttexindex = texdata_2.sIndex;
						}
					} else {
						if (eChunk == BTXT_ID) {
							if ((Data.iFlags & 4) != 0) {
								ourMemset(&texdata, 0, sizeof(FILE_TEX_DATA));
								pFile_2->GetChunkData(&texdata, sizeof(FILE_TEX_DATA));
								if (pFile_2->QEndian()) {
									texdata.Endian();
								}
								TESForm::AddCompileIndex(texdata.iTextureID, pFile_2);
								TESForm* v10 = TESForm::GetFormByNumericID(texdata.iTextureID);
								TESLandTexture* v11 = dynamic_cast<TESLandTexture*>(v10);
								this->pLoadedData->pDefQuadTexture[texdata.cBlock] = v11;
								if (this->pLoadedData->pDefQuadTexture[texdata.cBlock]) {
									TESLandTexture* ptex = this->pLoadedData->pDefQuadTexture[texdata.cBlock];
									if (!spQueuedTextures.m_pObject) {
										QueuedFile* v32;
										QueuedFile* v40 = static_cast<QueuedFile*>(ourMalloc(sizeof(QueuedFile)));
										if (v40) {
											v32 = ::new (v40) QueuedFile(IO_TASK_PRIORITY_VERY_LOW);
										} else {
											v32 = 0;
										}
										spQueuedTextures.operator=(v32);
									}
									QueuedFile* a6a = spQueuedTextures.m_pObject;
									ptex->QueueTexture(IO_TASK_PRIORITY_VERY_LOW, a6a);
								} else {
									int32_t a6 = texdata.cBlock;
									uint32_t fBa = texdata.iTextureID;
									int32_t iCellYb = GetDataY();
									Warning("TEXTURES: Land (%i, %i) unable to find base texture ID (%08X) for block "
											"%i.",
										GetDataX(), iCellYb, fBa, a6);
									badjusttextures = 1;
								}
							}
						} else {
							if (eChunk == VTXT_ID && (Data.iFlags & 4) != 0) {
								if (ilastblock >= 0 && ilasttexindex >= 0) {
									uint32_t ifilesize = pFile_2->GetChunkSize();
									if (ifilesize % 8) {
										const char* a6e = pFile_2->GetFilename();
										int32_t fBd = GetDataY();
										Warning("TEXTURES: Land (%i, %i) found unrecognized vertex texture data in "
												"file %s.",
											GetDataX(), fBd, a6e);
										badjusttextures = 1;
									} else {
										FILE_TEX_VERT_DATA* pvertdata =
											static_cast<FILE_TEX_VERT_DATA*>(ourMalloc(ifilesize));
										pFile_2->GetChunkData(pvertdata, ifilesize);
										uint32_t ivertcount = ifilesize >> 3;
										for (uint32_t i_2 = 0; i_2 < ivertcount; ++i_2) {
											if (pFile_2->QEndian()) {
												pvertdata[i_2].Endian();
											}
											SetTexturePercent(ilastblock, pvertdata[i_2].iVertIndex, ilasttexindex,
												pvertdata[i_2].fPercent);
										}
										ourFree(pvertdata);
										pvertdata = 0;
									}
								}
								ilastblock = -1;
								ilasttexindex = -1;
							}
						}
					}
				} else {
					int32_t j;
					int32_t i;
					if (eChunk == VHGT_ID) {
						if ((Data.iFlags & 1) != 0) {
							float afHeights[1089];
							pFile_2->GetChunkData(&kHeightData, sizeof(FILE_HEIGHT_DATA));
							if (pFile_2->QEndian()) {
								kHeightData.Endian();
							}
							NiPoint2* v4 = ::new (&v42) NiPoint2(3.4028235e+38f, -3.4028235e+38f);
							float x = v4->x;
							float y = v4->y;
							TESObjectLAND::LoadedLandData* pLoadedData = this->pLoadedData;
							pLoadedData->HeightExtents.x = x;
							pLoadedData->HeightExtents.y = y;
							float lastheight = kHeightData.fOffset;
							// Each step adds to the vertex before it in the row; a row's first vertex steps from the
							// first vertex of the row before.
							for (i = 0; i < 1089; ++i) {
								afHeights[i] = static_cast<double>(kHeightData.cHeightData[i]) + lastheight;
								if ((i + 1) % 33) {
									lastheight = afHeights[i];
								} else {
									lastheight = afHeights[i - 32];
								}
							}
							for (i = 0; i < 4; ++i) {
								int32_t v63 = 16 * (i % 2) + 528 * (i / 2);
								for (j = 0; j < 289; ++j) {
									int32_t v61 = j % 17;
									int32_t v62 = j / 17;
									int32_t curvert = j % 17 + (v63 + 33 * (j / 17));
									float v59 = static_cast<float>(8 * FloatToInt(afHeights[curvert]));
									this->pLoadedData->ppVertices[i][j].x =
										static_cast<double>(v61 << 7) + TESObjectLAND::pXOffsetBLOCK[i];
									this->pLoadedData->ppVertices[i][j].y =
										static_cast<double>(v62 << 7) + TESObjectLAND::pYOffsetBLOCK[i];
									this->pLoadedData->ppVertices[i][j].z = v59;
									if (!(this->pLoadedData->HeightExtents.x > static_cast<double>(v59))) {
										if (this->pLoadedData->HeightExtents.y < static_cast<double>(v59)) {
											this->pLoadedData->HeightExtents.y = v59;
										}
									} else {
										this->pLoadedData->HeightExtents.x = v59;
									}
								}
							}
							this->pLoadedData->fBaseHeight =
								(this->pLoadedData->HeightExtents.x + this->pLoadedData->HeightExtents.y) * 0.5;
							for (i = 0; i < 4; ++i) {
								for (j = 0; j < 289; ++j) {
									this->pLoadedData->ppVertices[i][j].z =
										this->pLoadedData->ppVertices[i][j].z - this->pLoadedData->fBaseHeight;
								}
							}
							loadedhgt = 1;
						}
					} else {
						if (eChunk > VNML_ID) {
							if (eChunk == VCLR_ID && (Data.iFlags & 2) != 0) {
								pFile_2->GetChunkData(v55, 3267u);
								for (i = 0; i < 4; ++i) {
									int32_t orig = 16 * (i % 2) + 528 * (i / 2);
									for (j = 0; j < 289; ++j) {
										int32_t curvert_2 = j % 17 + (orig + 33 * (j / 17));
										float v35 = static_cast<double>(v55[3 * curvert_2 + 2]) / 255.0;
										float v34 = static_cast<double>(v55[3 * curvert_2 + 1]) / 255.0;
										float fR = static_cast<double>(v55[3 * curvert_2]) / 255.0;
										NiColorA* v8 = FNVD_RECONSTRUCT(NiColorA)(&kColor, fR, v34, v35, 1.0f);
										NiColorA* v9 = &this->pLoadedData->ppColorsA[i][j];
										v9->r = v8->r;
										v9->g = v8->g;
										v9->b = v8->b;
										v9->a = v8->a;
									}
								}
							}
						} else {
							switch (eChunk) {
							case VNML_ID: {
								if ((Data.iFlags & 1) != 0) {
									pFile_2->GetChunkData(acNormals, 3267u);
									for (i = 0; i < 4; ++i) {
										int32_t v57 = 16 * (i % 2) + 528 * (i / 2);
										for (j = 0; j < 289; ++j) {
											int32_t v56 = j % 17 + (v57 + 33 * (j / 17));
											this->pLoadedData->ppNormals[i][j].x =
												static_cast<double>(acNormals[3 * v56]) / 127.0;
											this->pLoadedData->ppNormals[i][j].y =
												static_cast<double>(acNormals[3 * v56 + 1]) / 127.0;
											this->pLoadedData->ppNormals[i][j].z =
												static_cast<double>(acNormals[3 * v56 + 2]) / 127.0;
											this->pLoadedData->ppNormals[i][j].Unitize();
										}
									}
									loadednorms = 1;
								}
								break;
							}
							case DATA_ID: {
								// A remapped land keeps its own flags but for the three data bits, taken from the file.
								if (GetLandRemapped()) {
									pFile_2->GetChunkData(&tempdata, sizeof(OBJ_LAND));
									if (pFile_2->QEndian()) {
										tempdata.Endian();
									}
									tempdata.iFlags = tempdata.iFlags & 7;
									Data.iFlags &= ~7u;
									Data.iFlags |= tempdata.iFlags;
								}
								break;
							}
							case MPCD_ID: {
								uint32_t ibuffersize = pFile_2->GetChunkSize();
								char* pbuffer = static_cast<char*>(ourMalloc(ibuffersize));
								char* pbuffer__pvertdata = pbuffer;
								pFile_2->GetChunkData(pbuffer, ibuffersize);
								if (this->pLoadedData->pMoppCode) {
									this->pLoadedData->pMoppCode->removeReference();
									this->pLoadedData->pMoppCode = 0;
								}
								ReadMoppCodeFromBuffer(pbuffer__pvertdata, ibuffersize, &this->pLoadedData->pMoppCode);
								if (this->pLoadedData->pMoppCode) {
									this->pLoadedData->pMoppCode->addReference();
									SetHasMopp(1);
								}
								ourFree(pbuffer__pvertdata);
								break;
							}
							}
						}
					}
				}
			} while (pFile_2->NextChunk());
			if (badjusttextures) {
				AdjustTextureArrays();
			}
			if (loadedhgt && !loadednorms) {
				const char* a6f = pFile_2->GetFilename();
				int32_t fBe = GetDataY();
				Warning("CELLS: Land for cell (%i, %i) in file '%s' does not contain Normal Data.", GetDataX(), fBe,
					a6f);
			}
			return 1;
		} else {
			return 0;
		}
	}
}

void TESObjectLAND::Detach3D() {
	spQueuedTextures.operator=(static_cast<QueuedFile*>(0));
	if (pLoadedData) {
		if (pLoadedData->ppMesh) {
			bhkWorld* phw;
			if (pParentCell) {
				phw = pParentCell->GetbhkWorld();
			} else {
				phw = 0;
			}
			if (phw) {
				if (pLoadedData->spLandRB.operator!=(nullptr)) {
					phw->RemoveBody(pLoadedData->spLandRB.m_pObject);
				}
			}
			pLoadedData->spLandRB.operator=(static_cast<bhkRigidBody*>(0));
			for (int32_t iblock = 0; iblock < 4; ++iblock) {
				if (pLoadedData->ppMesh[iblock]) {
					pLoadedData->ppVertices[iblock] = 0;
					pLoadedData->ppColorsA[iblock] = 0;
					ourFree(pLoadedData->ppNormals[iblock]);
					pLoadedData->ppNormals[iblock] = 0;
					NiNode* v2 = pLoadedData->ppMesh[iblock];
					ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
					ShadowSceneNode_->RemoveObject(v2);
					pLoadedData->ppMesh[iblock]->DetachChildAt(0u);
				}
				pLoadedData->ppMesh[iblock] = 0;
			}
			ourFree(pLoadedData->ppMesh);
			pLoadedData->ppMesh = 0;
		}
		if (pLoadedData->spBorder.m_pObject) {
			NiNode* pParent = pLoadedData->spBorder.m_pObject->m_pkParent;
			if (pParent) {
				pParent->DetachChild(pLoadedData->spBorder.m_pObject);
			}
			pLoadedData->spBorder.operator=(static_cast<NiLines*>(0));
		}
	}
}

bool TESObjectLAND::ReadMoppCodeFromBuffer(char* apBuffer, uint32_t aiBufferSize, hkpMoppCode** apOutCode) {
	bool bretval = 0;
	if (apBuffer) {
		if (aiBufferSize) {
			if (apOutCode) {
				hkpMoppCode* pcode;
				hkpMoppCode* apMemory = static_cast<hkpMoppCode*>(hkReferencedObject::newObject(sizeof(hkpMoppCode)));
				if (apMemory) {
					pcode = ::new (apMemory) hkpMoppCode;
				} else {
					pcode = 0;
				}
				*apOutCode = pcode;
				if (aiBufferSize > 16) {
					ourMemcpy(&pcode->m_info, apBuffer, sizeof(pcode->m_info));
					// The remaining byte count is passed as is: hkArray::setSize takes Havok's int.
					pcode->m_data.setSize(static_cast<int32_t>(aiBufferSize - 16));
					ourMemcpy(pcode->m_data.m_data, apBuffer + 16, aiBufferSize - 16);
					bretval = 1;
				}
				if (!bretval) {
					if (pcode) {
						delete pcode;
					}
					*apOutCode = 0;
				}
			}
		}
	}
	return bretval;
}

TESLandTexture* TESObjectLAND::GetMainTexture(uint8_t acBlock, uint16_t asVertIndex) {
	TESLandTexture* v9 = 0;
	if (acBlock < 4u && asVertIndex < 289u) {
		float v7 = -3.4028235e+38f;
		uint32_t v8 = 0;
		for (uint32_t i = 0; i < 6; ++i) {
			float TexturePercent = GetTexturePercent(acBlock, asVertIndex, i);
			if (v7 < static_cast<double>(TexturePercent)) {
				v7 = TexturePercent;
				v8 = i;
			}
		}
		if (v8) {
			return GetQuadTexture(acBlock, v8 - 1);
		} else {
			return GetDefaultTexture(acBlock);
		}
	}
	return v9;
}

void TESObjectLAND::SetNewDefaultTexture(uint8_t acBlock, uint16_t asNewDefaultIndex) {
	if (acBlock < 4u && asNewDefaultIndex < 6u) {
		if (pLoadedData) {
			TESLandTexture* v8 = pLoadedData->pDefQuadTexture[acBlock];
			pLoadedData->pDefQuadTexture[acBlock] = pLoadedData->pQuadTextureArray[acBlock][asNewDefaultIndex];
			pLoadedData->pQuadTextureArray[acBlock][asNewDefaultIndex] = v8;
			if (pLoadedData->ppPercentArrays[acBlock]) {
				for (int32_t i = 0; i < 289; ++i) {
					float v6 = 0.0;
					for (uint32_t j = 0; j < 6; ++j) {
						v6 = GetTexturePercent(acBlock, i, j) + v6;
					}
					float v5 = 1.0 - v6;
					if (v5 < 0.0) {
						v5 = 0.0;
					}
					pLoadedData->ppPercentArrays[acBlock][i][asNewDefaultIndex] = v5;
				}
			}
		}
	}
}

TESGrassAreaParam** TESObjectLAND::GetGrassParameters(int32_t aiBlock, int32_t aiVert) {
	TESGrassAreaParam** pparam;
	if (!pLoadedData || (aiBlock >= 4 || (aiVert + 18 >= 289 || aiVert - 18 < 0))) {
		return 0;
	}
	// The grass map is keyed by unsigned int; the vertex index goes in as its key.
	if (pLoadedData->pmGrassMap[aiBlock].GetAt(static_cast<uint32_t>(aiVert), pparam)) {
		return pparam;
	}
	return 0;
}

NiPoint3** TESObjectLAND::GetNormalArray() {
	if (pLoadedData) {
		return pLoadedData->ppNormals;
	} else {
		return 0;
	}
}

void TESObjectLAND::InvalidateNormals(tagRECT* apInvalidateRect) {
	if (IsLoaded()) {
		for (int32_t i = 0; i < 4; ++i) {
			for (int32_t k = 0; k < 289; ++k) {
				if (!apInvalidateRect || VertexInRect(apInvalidateRect, pLoadedData->ppVertices[i][k])) {
					pLoadedData->ppNormals[i][k] = NiPoint3_ZERO;
					pLoadedData->ppNormalsSet[i][k] = 0;
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
					pLoadedData->ppNormals[i][i0].operator+=(normal);
				}
				if (!apInvalidateRect || VertexInRect(apInvalidateRect, pLoadedData->ppVertices[i][i1])) {
					pLoadedData->ppNormals[i][i1].operator+=(normal);
				}
				if (!apInvalidateRect || VertexInRect(apInvalidateRect, pLoadedData->ppVertices[i][i2])) {
					pLoadedData->ppNormals[i][i2].operator+=(normal);
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
					} else {
						ptrigeom = 0;
					}
					if (ptrigeom) {
						int32_t v18;
						uint16_t I = ptrigeom->GetVertexCount();
						NiPoint3* Vertices = ptrigeom->GetVertices();
						NiBound* ModelBound = &ptrigeom->GetModelBound();
						ModelBound->ComputeFromData(I, Vertices);
						ptrigeom->GetModelData()->MarkAsChanged(15);
						NiShadeProperty* pprop = static_cast<NiShadeProperty*>(ptrigeom->GetProperty(3));
						if (pprop) {
							int32_t v17 = pprop->iShaderPropertyType >= 8 && pprop->iShaderPropertyType <= 12;
							v18 = v17;
						} else {
							v18 = 0;
						}
						BSShaderPPLightingProperty* pshprop = v18 ? static_cast<BSShaderPPLightingProperty*>(pprop) : 0;
						if (pshprop) {
							uint8_t v11;
							uint8_t v12;
							uint8_t v13;
							uint8_t v14;
							uint8_t v15;
							uint8_t SpecularExponent;
							NiPoint3* pnormals = ptrigeom->GetModelData()->m_pkNormal;
							ourMemcpy(pnormals, pLoadedData->ppNormals[iblock], sizeof(NiPoint3) * 289);
							ptrigeom->GetModelData()->StoreNormalBinormalTangent(NiGeometryData::NBT_METHOD_NONE);
							pshprop->SetupGeometry(ptrigeom);
							TESLandTexture* v23 = pLoadedData->pDefQuadTexture[iblock];
							if (v23 && v23->pTextureSet) {
								BSTextureSet* AsShaderTextureSet =
									v23->pTextureSet->BGSTextureSet::GetAsShaderTextureSet();
								pshprop->SetLandscapeTextureSet(0, AsShaderTextureSet);
							} else {
								TESLandTexture* DefaultTexture = TESObjectLAND::GetDefaultTexture();
								BSTextureSet* v6 = DefaultTexture->pTextureSet->GetAsShaderTextureSet();
								pshprop->SetLandscapeTextureSet(0, v6);
							}
							for (int32_t itex = 1; itex < 7; ++itex) {
								TESLandTexture* v24 = pLoadedData->pQuadTextureArray[iblock][itex - 1];
								if (v24 && v24->pTextureSet) {
									BSTextureSet* v7 = v24->pTextureSet->BGSTextureSet::GetAsShaderTextureSet();
									pshprop->SetLandscapeTextureSet(static_cast<uint32_t>(itex), v7);
								} else {
									pshprop->SetLandscapeTextureSet(static_cast<uint32_t>(itex), 0);
								}
							}
							if (pLoadedData->pQuadTextureArray[iblock][5]) {
								SpecularExponent = pLoadedData->pQuadTextureArray[iblock][5]->GetSpecularExponent();
							} else {
								SpecularExponent = 0;
							}
							if (pLoadedData->pQuadTextureArray[iblock][4]) {
								v15 = pLoadedData->pQuadTextureArray[iblock][4]->GetSpecularExponent();
							} else {
								v15 = 0;
							}
							if (pLoadedData->pQuadTextureArray[iblock][3]) {
								v14 = pLoadedData->pQuadTextureArray[iblock][3]->GetSpecularExponent();
							} else {
								v14 = 0;
							}
							if (pLoadedData->pQuadTextureArray[iblock][2]) {
								v13 = pLoadedData->pQuadTextureArray[iblock][2]->GetSpecularExponent();
							} else {
								v13 = 0;
							}
							if (pLoadedData->pQuadTextureArray[iblock][1]) {
								v12 = pLoadedData->pQuadTextureArray[iblock][1]->GetSpecularExponent();
							} else {
								v12 = 0;
							}
							if (*pLoadedData->pQuadTextureArray[iblock]) {
								v11 = (*pLoadedData->pQuadTextureArray[iblock])->GetSpecularExponent();
							} else {
								v11 = 0;
							}
							if (pLoadedData->pDefQuadTexture[iblock]) {
								uint8_t v10 = pLoadedData->pDefQuadTexture[iblock]->GetSpecularExponent();
								pshprop->SetLandscapeSpecularExponents(v10, v11, v12, v13, v14, v15, SpecularExponent,
									0, 0, 0);
							} else {
								pshprop->SetLandscapeSpecularExponents(0, v11, v12, v13, v14, v15, SpecularExponent, 0,
									0, 0);
							}
							pshprop->FreeRenderPasses();
						}
					}
				}
				NiUpdateData UpdateData(0.0f, 0, 0);
				pNode->Update(UpdateData);
				if (updatescene) {
					pNode->UpdateProperties();
				}
			}
		}
		if (updatescene) {
			bool ShowLandBorders = TES::pInstance->GetShowLandBorders();
			DrawLandBorder(ShowLandBorders);
		}
	} else {
		CreateLandscape(0);
		Attach3D();
	}
}

void TESObjectLAND::FillLands8(LANDS8* pL8, bool abForceLoad, bool* apLoaded) {
	if (pL8) {
		TESObjectLAND* v5;
		TESObjectLAND* v8;
		TESObjectLAND* v11;
		TESObjectLAND* v14;
		TESObjectLAND* v17;
		TESObjectLAND* v20;
		TESObjectLAND* v23;
		TESObjectLAND* v26;
		TESWorldSpace* WorldSpace = GetWorldSpace();
		int32_t v28 = GetDataY() + 1;
		int32_t DataX = GetDataX();
		TESObjectCELL* CellFromCellCoord = pDataHandler->GetCellFromCellCoord(DataX - 1, v28, WorldSpace, 0);
		if (CellFromCellCoord && (v5 = CellFromCellCoord->GetLand(), v5->HasVertices())) {
			pL8->pNWLand = CellFromCellCoord->GetLand();
		} else {
			TESObjectLAND* Land;
			if (CellFromCellCoord &&
				((Land = CellFromCellCoord->GetLand(), !Land->HasVertices()) && (abForceLoad && apLoaded))) {
				pL8->pNWLand = CellFromCellCoord->GetLand();
				pL8->pNWLand->LoadVertices(0);
				*apLoaded = 1;
			} else {
				pL8->pNWLand = 0;
			}
		}
		TESWorldSpace* v37 = GetWorldSpace();
		int32_t v29 = GetDataY() + 1;
		int32_t v7 = GetDataX();
		TESObjectCELL* v46 = pDataHandler->GetCellFromCellCoord(v7, v29, v37, 0);
		if (v46 && (v8 = v46->GetLand(), v8->HasVertices())) {
			pL8->pNLand = v46->GetLand();
		} else {
			TESObjectLAND* v9;
			if (v46 && ((v9 = v46->GetLand(), !v9->HasVertices()) && (abForceLoad && apLoaded))) {
				pL8->pNLand = v46->GetLand();
				pL8->pNLand->LoadVertices(0);
				apLoaded[1] = 1;
			} else {
				pL8->pNLand = 0;
			}
		}
		TESWorldSpace* v38 = GetWorldSpace();
		int32_t v30 = GetDataY() + 1;
		int32_t v10 = GetDataX();
		TESObjectCELL* v47 = pDataHandler->GetCellFromCellCoord(v10 + 1, v30, v38, 0);
		if (v47 && (v11 = v47->GetLand(), v11->HasVertices())) {
			pL8->pNELand = v47->GetLand();
		} else {
			TESObjectLAND* v12;
			if (v47 && ((v12 = v47->GetLand(), !v12->HasVertices()) && (abForceLoad && apLoaded))) {
				pL8->pNELand = v47->GetLand();
				pL8->pNELand->LoadVertices(0);
				apLoaded[2] = 1;
			} else {
				pL8->pNELand = 0;
			}
		}
		TESWorldSpace* v39 = GetWorldSpace();
		int32_t DataY = GetDataY();
		int32_t v13 = GetDataX();
		TESObjectCELL* v48 = pDataHandler->GetCellFromCellCoord(v13 - 1, DataY, v39, 0);
		if (v48 && (v14 = v48->GetLand(), v14->HasVertices())) {
			pL8->pWLand = v48->GetLand();
		} else {
			TESObjectLAND* v15;
			if (v48 && ((v15 = v48->GetLand(), !v15->HasVertices()) && (abForceLoad && apLoaded))) {
				pL8->pWLand = v48->GetLand();
				pL8->pWLand->LoadVertices(0);
				apLoaded[3] = 1;
			} else {
				pL8->pWLand = 0;
			}
		}
		TESWorldSpace* v40 = GetWorldSpace();
		int32_t v32 = GetDataY();
		int32_t v16 = GetDataX();
		TESObjectCELL* v49 = pDataHandler->GetCellFromCellCoord(v16 + 1, v32, v40, 0);
		if (v49 && (v17 = v49->GetLand(), v17->HasVertices())) {
			pL8->pELand = v49->GetLand();
		} else {
			TESObjectLAND* v18;
			if (v49 && ((v18 = v49->GetLand(), !v18->HasVertices()) && (abForceLoad && apLoaded))) {
				pL8->pELand = v49->GetLand();
				pL8->pELand->LoadVertices(0);
				apLoaded[4] = 1;
			} else {
				pL8->pELand = 0;
			}
		}
		TESWorldSpace* v41 = GetWorldSpace();
		int32_t v33 = GetDataY() - 1;
		int32_t v19 = GetDataX();
		TESObjectCELL* v50 = pDataHandler->GetCellFromCellCoord(v19 - 1, v33, v41, 0);
		if (v50 && (v20 = v50->GetLand(), v20->HasVertices())) {
			pL8->pSWLand = v50->GetLand();
		} else {
			TESObjectLAND* v21;
			if (v50 && ((v21 = v50->GetLand(), !v21->HasVertices()) && (abForceLoad && apLoaded))) {
				pL8->pSWLand = v50->GetLand();
				pL8->pSWLand->LoadVertices(0);
				apLoaded[5] = 1;
			} else {
				pL8->pSWLand = 0;
			}
		}
		TESWorldSpace* v42 = GetWorldSpace();
		int32_t v34 = GetDataY() - 1;
		int32_t v22 = GetDataX();
		TESObjectCELL* v51 = pDataHandler->GetCellFromCellCoord(v22, v34, v42, 0);
		if (v51 && (v23 = v51->GetLand(), v23->HasVertices())) {
			pL8->pSLand = v51->GetLand();
		} else {
			TESObjectLAND* v24;
			if (v51 && ((v24 = v51->GetLand(), !v24->HasVertices()) && (abForceLoad && apLoaded))) {
				pL8->pSLand = v51->GetLand();
				pL8->pSLand->LoadVertices(0);
				apLoaded[6] = 1;
			} else {
				pL8->pSLand = 0;
			}
		}
		TESWorldSpace* v43 = GetWorldSpace();
		int32_t v35 = GetDataY() - 1;
		int32_t v25 = GetDataX();
		TESObjectCELL* v52 = pDataHandler->GetCellFromCellCoord(v25 + 1, v35, v43, 0);
		if (v52 && (v26 = v52->GetLand(), v26->HasVertices())) {
			pL8->pSELand = v52->GetLand();
		} else {
			TESObjectLAND* v27;
			if (v52 && ((v27 = v52->GetLand(), !v27->HasVertices()) && (abForceLoad && apLoaded))) {
				pL8->pSELand = v52->GetLand();
				pL8->pSELand->LoadVertices(0);
				apLoaded[7] = 1;
			} else {
				pL8->pSELand = 0;
			}
		}
	}
}

void TESObjectLAND::SetLandNormals(LANDS8* pL8, tagRECT* apCalcRect) {
	LANDS8 v6;
	if (IsLoaded()) {
		if (!pL8) {
			pL8 = &v6;
			FillLands8(&v6, 0, 0);
		}
		for (int32_t i = 0; i < 4; ++i) {
			for (int32_t j = 0; j < 289; ++j) {
				if ((j < 17 || (j > 272 || (!(j % 17) || !((j + 1) % 17)))) &&
					(!apCalcRect || VertexInRect(apCalcRect, pLoadedData->ppVertices[i][j]))) {
					CalculateEdgeNormal(i, j, pLoadedData->ppNormals[i][j], pL8);
				}
			}
		}
		if (pL8->pNELand) {
			if (pL8->pNLand) {
				if (pL8->pNWLand) {
					if (pL8->pSELand) {
						if (pL8->pSLand) {
							if (pL8->pSWLand) {
								if (pL8->pELand) {
									if (pL8->pWLand) {
										SetGoodNormals(1);
									}
								}
							}
						}
					}
				}
			}
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
	NiPoint3 kTemp;
	float fY = GetWorldCellY() + 2048.0;
	float fX = GetWorldCellX() + 2048.0;
	offset = *::new (&kTemp) NiPoint3(fX, fY, 0.0f);
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
			tempnormal = *::new (&kTemp) NiPoint3(0.0f, 0.0f, 1.0f);
		}
		if (pland && pland->IsNormalSet(scaledvert)) {
			normal = tempnormal;
			pLoadedData->ppNormalsSet[iblock][vertidx] = true;
			return;
		}
		retnormal += tempnormal;
		divisor = divisor + 1.0;
	} else if (iblock >= 2 && vertidx >= 272) {
		cellup = true;
		pland = pL8->pNLand;
		NiPoint3 pfacenorm;
		if (!pland || !pland->GetLandNormal(scaledvert, tempnormal, pfacenorm)) {
			tempnormal = *::new (&kTemp) NiPoint3(0.0f, 0.0f, 1.0f);
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
			tempnormal = *::new (&kTemp) NiPoint3(0.0f, 0.0f, 1.0f);
		}
		if (pland && pland->IsNormalSet(scaledvert)) {
			normal = tempnormal;
			pLoadedData->ppNormalsSet[iblock][vertidx] = true;
			return;
		}
		retnormal += tempnormal;
		divisor = divisor + 1.0;
	} else if ((iblock + 1) % 2 == 0 && (vertidx + 1) % 17 == 0) {
		cellright = true;
		pland = pL8->pELand;
		NiPoint3 pfacenorm;
		if (!pland || !pland->GetLandNormal(scaledvert, tempnormal, pfacenorm)) {
			tempnormal = *::new (&kTemp) NiPoint3(0.0f, 0.0f, 1.0f);
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
	} else if (cellright && celldown) {
		pland = pL8->pSELand;
	} else if (cellleft && cellup) {
		pland = pL8->pNWLand;
	} else if (cellright && cellup) {
		pland = pL8->pNELand;
	}
	if ((cellleft || cellright) && (celldown || cellup)) {
		NiPoint3 pfacenorm;
		if (!pland || !pland->GetLandNormal(scaledvert, tempnormal, pfacenorm)) {
			tempnormal = *::new (&kTemp) NiPoint3(0.0f, 0.0f, 1.0f);
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
	} else if (vertidx >= 272 && !cellup) {
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
	} else if ((vertidx + 1) % 17 == 0 && !cellright) {
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
	} else if (blockleft && blockup) {
		iDiagBlock = iblock + 1;
		iDiagVert = 16;
	} else if (blockright && blockdown) {
		iDiagBlock = iblock - 1;
		iDiagVert = 272;
	} else if (blockright && blockup) {
		iDiagBlock = iblock + 3;
		iDiagVert = 0;
	} else {
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
	NiColorA* pColorBlock;
	NiTexturingProperty* NiTexturingProperty_;
	NiTexturingProperty::Map* pdefmap;
	TESLandTexture* TESLandTexture_;
	BGSTextureSet* v21;
	NiPoint3 v22;
	NiPoint2 v25;
	NiPoint3 dst;
	char apDest[268];
	int32_t i;
	int32_t y;
	int32_t x;
	AutoMemContext tempMemContext(MEM_CONTEXT_LAND, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESLand.cpp", 198);
	BGSTextureSet* v42 = static_cast<BGSTextureSet*>(NiMemObject::operator new(sizeof(BGSTextureSet)));
	if (v42) {
		v21 = ::new (v42) BGSTextureSet;
	} else {
		v21 = 0;
	}
	TESObjectLAND::pDefTextTexSet = v21;
	BSsprintf(apDest, 260, "Landscape\\%s", gs_sDefaultLandDiffuseTexture_Landscape.String());
	TESObjectLAND::pDefTextTexSet->SetBGSTextureFilename(BGSTextureSet::TEXTYPE_BASE, apDest);
	BSsprintf(apDest, 260, "Landscape\\%s", gs_sDefaultLandNormalTexture_Landscape.String());
	TESObjectLAND::pDefTextTexSet->SetBGSTextureFilename(BGSTextureSet::TEXTYPE_NORMAL, apDest);
	pDataHandler->pObjectList->lnk_Remove(TESObjectLAND::pDefTextTexSet);
	TESLandTexture* v40 = static_cast<TESLandTexture*>(ourMalloc(sizeof(TESLandTexture)));
	if (v40) {
		TESLandTexture_ = ::new (v40) TESLandTexture;
	} else {
		TESLandTexture_ = 0;
	}
	TESObjectLAND::pDefText = TESLandTexture_;
	TESLandTexture_->pTextureSet = TESObjectLAND::pDefTextTexSet;
	NiPointer<NiTexture> spTempTexture(static_cast<NiTexture*>(0));
	TESObjectLAND::pDefTextTexSet->GetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, spTempTexture);
	NiTexturingProperty::Map* v38 = static_cast<NiTexturingProperty::Map*>(ourMalloc(sizeof(NiTexturingProperty::Map)));
	if (v38) {
		pdefmap = ::new (v38) NiTexturingProperty::Map(spTempTexture.m_pObject, 0, 3, 5, nullptr);
	} else {
		pdefmap = 0;
	}
	NiTexturingProperty* v36 =
		static_cast<NiTexturingProperty*>(NiMemObject::operator new(sizeof(NiTexturingProperty)));
	if (v36) {
		NiTexturingProperty_ = ::new (v36) NiTexturingProperty;
	} else {
		NiTexturingProperty_ = 0;
	}
	TESObjectLAND::spTexturingProperty.operator=(NiTexturingProperty_);
	TESObjectLAND::spTexturingProperty.m_pObject->SetApplyMode(NiTexturingProperty::APPLY_MODULATE);
	TESObjectLAND::spTexturingProperty.m_pObject->SetBaseMap(pdefmap);
	// Six indices for each of the 16x16 quads.
	uint16_t* v35 = static_cast<uint16_t*>(ourMalloc(16 * 16 * 6 * sizeof(uint16_t)));
	TESObjectLAND::pDefTriListBLOCK = v35;
	int32_t v49 = 0;
	for (x = 0; x < 16; ++x) {
		for (y = 0; y < 16; ++y) {
			uint16_t v0;
			if (y % 2 != x % 2) {
				TESObjectLAND::pDefTriListBLOCK[v49++] = y + 17 * (x + 1);
				TESObjectLAND::pDefTriListBLOCK[v49++] = y + 17 * x;
				TESObjectLAND::pDefTriListBLOCK[v49++] = 17 * x + y + 1;
				TESObjectLAND::pDefTriListBLOCK[v49++] = 17 * x + y + 1;
				TESObjectLAND::pDefTriListBLOCK[v49++] = 17 * (x + 1) + y + 1;
				uint16_t ival = y + 17 * (x + 1);
				v0 = ival;
			} else {
				TESObjectLAND::pDefTriListBLOCK[v49++] = 17 * (x + 1) + y + 1;
				TESObjectLAND::pDefTriListBLOCK[v49++] = y + 17 * (x + 1);
				TESObjectLAND::pDefTriListBLOCK[v49++] = y + 17 * x;
				TESObjectLAND::pDefTriListBLOCK[v49++] = y + 17 * x;
				TESObjectLAND::pDefTriListBLOCK[v49++] = 17 * x + y + 1;
				uint16_t ival_2 = 17 * (x + 1) + y + 1;
				v0 = ival_2;
			}
			TESObjectLAND::pDefTriListBLOCK[v49++] = v0;
		}
	}
	float ftilescalar = gs_fLandTextureTilingMult_Landscape.Float();
	if (ftilescalar == 0.0) {
		Warning("DEFAULT: INI setting fLandTextureTilingMult cannot be 0.");
		ftilescalar = 16.0f;
	}
	ftilescalar = 4.0 / ftilescalar;
	// The vertex blocks hold 17x17 = 289 vertices.
	void* ptr = ourMalloc(289 * sizeof(NiColorA));
	if (ptr) {
		rt::eh_vector_constructor_iterator(ptr, sizeof(NiColorA), 289, &fnvd::constructElement<NiColorA>);
		pColorBlock = static_cast<NiColorA*>(ptr);
	} else {
		pColorBlock = 0;
	}
	TESObjectLAND::pDefColorBLOCK = pColorBlock;
	void* v31 = ourMalloc(289 * sizeof(NiPoint3));
	if (v31) {
		rt::eh_vector_constructor_iterator(v31, sizeof(NiPoint3), 289, &emptyPointConstructor);
		pNormalBlock = static_cast<NiPoint3*>(v31);
	} else {
		pNormalBlock = 0;
	}
	TESObjectLAND::pDefNormalBLOCK = pNormalBlock;
	bool* v30 = static_cast<bool*>(ourMalloc(289 * sizeof(bool)));
	TESObjectLAND::pDefNormalSetBLOCK = v30;
	void* v28 = ourMalloc(289 * sizeof(NiPoint2));
	if (v28) {
		rt::eh_vector_constructor_iterator(v28, sizeof(NiPoint2), 289, &emptyPointConstructor);
		pTextureBlock = static_cast<NiPoint2*>(v28);
	} else {
		pTextureBlock = 0;
	}
	TESObjectLAND::pDefTextureBLOCK = pTextureBlock;
	int32_t index = 0;
	for (x = 0; x < 17; ++x) {
		for (y = 0; y < 17; ++y) {
			NiColorA kColor(1.0f, 1.0f, 1.0f, 0.0f);
			TESObjectLAND::pDefColorBLOCK[index] = kColor;
			NiPoint3* v3 = ::new (&dst) NiPoint3(0.0f, 0.0f, 1.0f);
			TESObjectLAND::pDefNormalBLOCK[index] = *v3;
			TESObjectLAND::pDefNormalSetBLOCK[index] = 1;
			float v14 = static_cast<double>(x) / ftilescalar;
			float v13 = static_cast<double>(y) / ftilescalar;
			NiPoint2* v4 = ::new (&v25) NiPoint2(v13, v14);
			float v5 = v4->x;
			float v6 = v4->y;
			int32_t v7 = index;
			NiPoint2* v8 = TESObjectLAND::pDefTextureBLOCK;
			TESObjectLAND::pDefTextureBLOCK[index].x = v5;
			v8[v7].y = v6;
			++index;
		}
	}
	for (i = 0; i < 4; ++i) {
		TESObjectLAND::pXOffsetBLOCK[i] = static_cast<float>((i % 2 << 11) - 2048);
		TESObjectLAND::pYOffsetBLOCK[i] = static_cast<float>((i / 2 << 11) - 2048);
	}
	for (i = 0; i < 4; ++i) {
		NiPoint3* v12;
		index = 0;
		void* v23 = ourMalloc(289 * sizeof(NiPoint3));
		if (v23) {
			rt::eh_vector_constructor_iterator(v23, sizeof(NiPoint3), 289, &emptyPointConstructor);
			v12 = static_cast<NiPoint3*>(v23);
		} else {
			v12 = 0;
		}
		TESObjectLAND::ppDefVertexBLOCK[i] = v12;
		for (x = 0; x < 17; ++x) {
			for (y = 0; y < 17; ++y) {
				float v11 = static_cast<double>(x << 7) + TESObjectLAND::pYOffsetBLOCK[i];
				float v10 = static_cast<double>(y << 7) + TESObjectLAND::pXOffsetBLOCK[i];
				NiPoint3* v9 = ::new (&v22) NiPoint3(v10, v11, -2048.0f);
				TESObjectLAND::ppDefVertexBLOCK[i][index++] = *v9;
			}
		}
	}
	TESObjectLAND::bStaticsDefined = 1;
}

TESObjectLAND::LoadedLandData::LoadedLandData()
	: ppMesh(nullptr), ppVertices(nullptr), ppNormals(nullptr), ppColorsA(nullptr), ppNormalsSet(nullptr),
	  spBorder(nullptr), HeightExtents(3.4028235e+38f, -3.4028235e+38f), pQuadTextureArray(), ppPercentArrays(),
	  pMoppCode(nullptr), pmGrassMap(), spLandRB(nullptr), fBaseHeight(0.0f) {
	for (int32_t i = 0; i < 4; ++i) {
		pDefQuadTexture[i] = TESObjectLAND::pDefText;
	}
}

TESObjectLAND::LoadedLandData::~LoadedLandData() {}

COORD_DATA::COORD_DATA() {}

hkMoppBvTreeShapeCinfo::hkMoppBvTreeShapeCinfo() : m_code(nullptr), fScale(1.0f) {}

NiDX9ShaderDeclaration* ShadowLightShader::GetLandShaderDecl() {
	return spSplatTexturingShaderDecl.operator->();
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

NiVBChip* NiVBChip::Create(NiVBBlock* pkBlock, IDirect3DVertexBuffer9* pkVB, uint32_t uiOffset, uint32_t uiSize,
	uint32_t uiIndex) {
	NiVBChip* pkChip;
	NiVBChip* apMemory = static_cast<NiVBChip*>(NiMemObject::operator new(sizeof(NiVBChip)));
	if (apMemory) {
		pkChip = ::new (apMemory) NiVBChip;
	} else {
		pkChip = 0;
	}
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
