#include "Fallout Shared/TESTerrainLODManager.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSMultiBoundAABB.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/RendererSettingCollection.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/TallGrassShader.h"
#include "BSShader/TallGrassShaderProperty.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESGrass.h"
#include "Fallout Shared/TESGrassAreaParam.h"
#include "Fallout Shared/TESLand.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

// The game settings this file defines, with their IDs and values, in the game's initialization order.
SettingT<INISettingCollection> gs_bDrawShaderGrass_Grass("bDrawShaderGrass:Grass", true);
SettingT<INISettingCollection> gs_bGrassPointLighting_Grass("bGrassPointLighting:Grass", false);
SettingT<INISettingCollection> fGrassMinStartFadeDistance("fGrassMinStartFadeDistance:Grass", 400.0f);
SettingT<INISettingCollection> fGrassMaxStartFadeDistance("fGrassMaxStartFadeDistance:Grass", 7000.0f);
SettingT<INISettingCollection> fGrassDefaultStartFadeDistance("fGrassDefaultStartFadeDistance:Grass", 3500.0f);
SettingT<INISettingCollection> fGrassFadeRange("fGrassFadeRange:Grass", 1000.0f);
SettingT<RendererSettingCollection> fGrassStartFadeDistance("fGrassStartFadeDistance:Grass",
	fGrassDefaultStartFadeDistance.Float());
SettingT<INISettingCollection> gs_iMinGrassSize_Grass("iMinGrassSize:Grass", 80);
SettingT<INISettingCollection> gs_fGrassWindMagnitudeMin_Grass("fGrassWindMagnitudeMin:Grass", 5.0f);
SettingT<INISettingCollection> gs_fGrassWindMagnitudeMax_Grass("fGrassWindMagnitudeMax:Grass", 125.0f);

// The first manager allocates the shared low-detail grass buffers, the last one frees them.
TESTerrainLODManager::TESTerrainLODManager() {
	if (!iManagerCount) {
		int32_t isizecap = (TESObjectLAND::GetGrassEvalSize() << 8) / gs_iMinGrassSize_Grass.Int();
		uint32_t uiCount = static_cast<uint32_t>(isizecap * isizecap);
		if (!pLowpositions) {
			pLowpositions = static_cast<NiPoint3*>(ourMalloc(uiCount * sizeof(NiPoint3)));
		}
		if (!pLowcolors) {
			pLowcolors = static_cast<float*>(ourMalloc(uiCount * sizeof(float)));
		}
	}
	++iManagerCount;
	TallGrassShader::SetWindRanges(gs_fGrassWindMagnitudeMin_Grass.Float(), gs_fGrassWindMagnitudeMax_Grass.Float());
}

TESTerrainLODManager::~TESTerrainLODManager() {
	if (!--iManagerCount) {
		if (pLowpositions) {
			ourFree(pLowpositions);
			pLowpositions = nullptr;
		}
		if (pLowcolors) {
			ourFree(pLowcolors);
			pLowcolors = nullptr;
		}
	}
}

void TESTerrainLODManager::UpdateGrass(NiPoint3 apCameraPosition, NiPoint3 apCameraDirection, float afUpdateAngle) {
	AutoMemContext tempMemContext(MEM_CONTEXT_GRASS, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESTerrainLODManager.cpp", 177);
	if (!TES::pInstance->pInteriorCell && (gs_bDrawShaderGrass_Grass.Bool() && bIsToggleGrassUpdate)) {
		if (!TESTerrainLODManager::bGrassInitialized) {
			TESTerrainLODManager::InitGrass();
		}
		TallGrassShaderProperty::bPointLighting = gs_bGrassPointLighting_Grass.Bool();
		float* v3 = &fGrassStartFadeDistance.Float();
		float fmaxrange = *v3 + fGrassFadeRange.Float();
		float ffaderange = fGrassStartFadeDistance.Float();
		float* v4 = &fGrassStartFadeDistance.Float();
		if (!(*v4 < fGrassMinStartFadeDistance.Float() + 0.5)) {
			if (!TESTerrainLODManager::pGrassNode) {
				NiNode* pbasenode = BSShaderManager::GetShadowSceneNode(0);
				NiFixedString* v5 = &FixedStrings::QGrass();
				NiAVObject* pgrassobj = pbasenode->GetObjectByName(*v5);
				TESTerrainLODManager::pGrassNode = static_cast<NiNode*>(pgrassobj);
				if (!pgrassobj) {
					NiNode* v11;
					NiNode* apMemory = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
					if (apMemory) {
						v11 = ::new (apMemory) NiNode(0);
					} else {
						v11 = 0;
					}
					TESTerrainLODManager::pGrassNode = v11;
					v11->SetName(FixedStrings::QGrass());
					pbasenode->AttachChild(TESTerrainLODManager::pGrassNode, 1);
				}
			}
			TallGrassShaderProperty::SetFadeParameters(0.0f, 0.0f, ffaderange, fmaxrange);
			int32_t icellx = static_cast<int32_t>(apCameraPosition.x) / 0x1000;
			int32_t icelly = static_cast<int32_t>(apCameraPosition.y) / 0x1000;
			if (apCameraPosition.x < 0.0) {
				--icellx;
			}
			if (apCameraPosition.y < 0.0) {
				--icelly;
			}
			TESObjectCELL* pcell = TES::pInstance->GetGridCellCell(icellx, icelly);
			NiPoint2 pcameraCellPos(apCameraPosition.x, apCameraPosition.y);
			pcameraCellPos.x = pcameraCellPos.x - static_cast<double>(icellx << 12);
			pcameraCellPos.y = pcameraCellPos.y - static_cast<double>(icelly << 12);
			for (int32_t icitx = 0; icitx < 5; ++icitx) {
				for (int32_t icity = 0; icity < 5; ++icity) {
					pcell = TES::pInstance->GetGridCellCell(icellx + icitx - 2, icelly + icity - 2);
					if (pcell) {
						bool bremove = 1;
						if (icitx) {
							if (icitx != 4) {
								if (icity) {
									if (icity != 4) {
										double DistanceFromPoint = pcell->GetDistanceFromPoint(apCameraPosition);
										if (fmaxrange > DistanceFromPoint) {
											TESTerrainLODManager::UpdateGrassForCell(pcell,
												TESTerrainLODManager::pGrassNode, apCameraPosition, apCameraDirection,
												ffaderange, fmaxrange, afUpdateAngle);
											bremove = 0;
										}
									}
								}
							}
						}
						if (bremove) {
							int32_t afLowMaxFade = pcell->GetDataY();
							int32_t DataX = pcell->GetDataX();
							TallGrassShaderProperty::RemoveFromCell(DataX, afLowMaxFade);
						}
					}
				}
			}
			if (gs_bGrassPointLighting_Grass.operator bool()) {
				NiNode* iDataY = TESTerrainLODManager::pGrassNode;
				NiNode* pbasenode_2 = BSShaderManager::GetShadowSceneNode(0);
				static_cast<ShadowSceneNode*>(pbasenode_2)->ShadowSceneNode::UpdateObjectLighting(iDataY, 0);
			}
			if (TESTerrainLODManager::pGrassNode) {
				NiUpdateData UpdateData(0.0f, 0, 0);
				TESTerrainLODManager::pGrassNode->Update(UpdateData);
				TESTerrainLODManager::pGrassNode->UpdateProperties();
			}
		} else {
		}
	} else {
		TallGrassShaderProperty::RemoveAllGrass();
	}
}

void TESTerrainLODManager::InitGrass() {
	NiNode* v2;
	AutoMemContext apMemory(MEM_CONTEXT_GRASS, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESTerrainLODManager.cpp", 144);
	float fRadius = 0.0f;
	float* v0 = &fGrassStartFadeDistance.Float();
	if (*v0 >= fGrassMinStartFadeDistance.Float() + 0.5) {
		float* v1 = &fGrassStartFadeDistance.Float();
		fRadius = *v1 + fGrassFadeRange.Float();
	}
	if (fRadius > static_cast<double>(uGridsToLoad.UInt() << 12)) {
		fRadius = static_cast<float>(uGridsToLoad.UInt() << 12);
	}
	TESTerrainLODManager::fGrassRadius = fRadius;
	ShadowSceneNode* ShadowSceneNode_ = BSShaderManager::GetShadowSceneNode(0);
	NiNode* v3 = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
	if (v3) {
		v2 = ::new (v3) NiNode(0);
	} else {
		v2 = 0;
	}
	TESTerrainLODManager::pGrassNode = v2;
	v2->SetName(FixedStrings::QGrass());
	ShadowSceneNode_->SetAt(6u, TESTerrainLODManager::pGrassNode);
	TESTerrainLODManager::pGrassNode->UpdateProperties();
	TESTerrainLODManager::bGrassInitialized = 1;
}

NiNode* TESTerrainLODManager::GetGrassNode() {
	return TESTerrainLODManager::pGrassNode;
}

void TESTerrainLODManager::UpdateGrassForCell(TESObjectCELL* apCell, NiNode* apGrassNode, NiPoint3 apCameraPosition,
	NiPoint3 apCameraDirection, float fFadeRange, float fMaxRange, float afUpdateAngle) {
	NiPoint3 v25;
	NiFrustum kcurrentfrustum;
	if (!bTempTest) {
		int32_t v44 = 0;
		BSSimpleList<TESLandTexture*> plandTextures;
		TESGrass* pGrassForm = 0;
		COORD_DATA coordData;
		uint16_t usHeightLimit = 0;
		uint8_t ucMaxSlope = 0;
		uint8_t ucMinSlope = 0;
		int32_t igrassEvalSize = TESObjectLAND::GetGrassEvalSize();
		int32_t igrassEvalSquared = 2 * igrassEvalSize;
		TESGrass::GRASS_WATER_STATE eWaterState = TESGrass::GWS_COUNT;
		if (apCell && (apCell->GetWorldSpace() && apCell->GetLand())) {
			FNVD_INLINED_LOCAL(NiPlane, kVertexPos, std::nullopt, std::nullopt);
			kVertexPos.m_fConstant = apCell->GetWaterHeight();
			TESObjectLAND* pland = apCell->GetLand();
			int32_t iCellX = apCell->GetDataX();
			int32_t iCellY = apCell->GetDataY();
			int32_t aiVert = 0;
			apCameraDirection.z = 0.0f;
			apCameraDirection.Unitize();
			afUpdateAngle = afUpdateAngle + afUpdateAngle;
			NiFrustum kfrustum(0);
			NiRenderer* Renderer_ = NiRenderer::GetRenderer();
			float fheight = static_cast<float>(Renderer_->GetScreenHeight());
			NiRenderer* v8 = NiRenderer::GetRenderer();
			float fwidth = static_cast<float>(v8->GetScreenWidth());
			float v57 = fheight / fwidth;
			float faspectDelta = 0.75 / (fheight / fwidth);
			SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
			double CameraFOV = WorldSceneGraph->GetCameraFOV();
			float theta = CameraFOV * 0.01745329238474369 * 0.5 + afUpdateAngle;
			double v11 = std::tan(theta);
			kfrustum.m_fLeft = -v11 * faspectDelta;
			double v12 = std::tan(theta);
			kfrustum.m_fRight = v12 * faspectDelta;
			double v13 = std::tan(theta);
			kfrustum.m_fBottom = -v13 * v57 * faspectDelta;
			double v14 = std::tan(theta);
			kfrustum.m_fTop = v14 * v57 * faspectDelta;
			SceneGraph* v15 = Main::GetWorldSceneGraph();
			kcurrentfrustum = v15->GetCamera()->GetViewFrustum();
			kfrustum.m_fFar = kcurrentfrustum.m_fFar;
			kfrustum.m_fNear = kcurrentfrustum.m_fNear;
			NiFrustumPlanes kplanes;
			SceneGraph* v16 = Main::GetWorldSceneGraph();
			kplanes.Set(kfrustum, v16->GetCamera()->GetWorldTransform());
			for (int32_t iQuad = 0; iQuad < 4; ++iQuad) {
				NiNode* plandquad = apCell->GetLandNode(iQuad);
				if (plandquad) {
					int32_t vertIndex;
					NiPoint3 blockstart = pland->GetVertexLocation(0, 0, 0, 0);
					int32_t lseed = 0;
					NiBound kbound = plandquad->GetWorldBound();
					NiPoint3 asdf = NiPoint3_ZERO;
					double v23 = (apCameraPosition - kbound.GetCenter()).Length();
					// Rounded to float before the compare, as above.
					float dist = v23 - kbound.m_fRadius;
					NiNode* ChildNode = apCell->GetChildNode(TESObjectCELL::CN_LAND);
					BSMultiBoundNode* pmbnode = static_cast<BSMultiBoundNode*>(ChildNode->GetAt(iQuad));
					BSMultiBoundShape::BSMBCullResult kcullresult = pmbnode->QMultiBound()->QShape()->kCullResult;
					BSMultiBoundAABB* v22 = static_cast<BSMultiBoundAABB*>(pmbnode->QMultiBound()->QShape());
					bool bvisible = v22->WithinFrustum(kplanes);
					if ((kcullresult == BSMultiBoundShape::BS_CULL_VISIBLE ||
							kcullresult == BSMultiBoundShape::BS_CULL_UNTESTED || bvisible) &&
						fMaxRange > static_cast<double>(dist)) {
						for (int32_t ixvert = igrassEvalSize; ixvert < 16; ixvert += igrassEvalSquared) {
							for (int32_t iyvert = igrassEvalSize; iyvert < 16; iyvert += igrassEvalSquared) {
								vertIndex = ixvert + 17 * iyvert;
								NiPoint3* VertexLocation =
									::new (&v25) NiPoint3(pland->GetVertexLocation(static_cast<uint8_t>(iQuad),
										ixvert + 17 * iyvert, 0, 0));
								kVertexPos.m_kNormal.x = VertexLocation->x;
								kVertexPos.m_kNormal.y = VertexLocation->y;
								float z = VertexLocation->z;
								kVertexPos.m_kNormal.z = apCameraPosition.z;
								kVertexPos.m_kNormal.x = kVertexPos.m_kNormal.x - 256.0;
								kVertexPos.m_kNormal.y = kVertexPos.m_kNormal.y - 256.0;
								lseed = TallGrassShaderProperty::Hash_2DPosition(kVertexPos.m_kNormal);
								TESGrassAreaParam** pparam = pland->GetGrassParameters(iQuad, vertIndex);
								for (int32_t l = 0;
									pparam && (pparam[l] && (l < 16 && pparam[l]->kGrassParam.pcGeomFilename)); ++l) {
									if (pparam[l]) {
										if (!TallGrassShaderProperty::ContainsChunk_00B617B0(pparam[l]
																								 ->kGrassParam.iGrassID,
												lseed)) {
											TESGrass* pgrass =
												static_cast<TESGrass*>(TESForm::GetFormByNumericID(pparam[l]
														->kGrassParam.iGrassID));
											if (pgrass) {
												usHeightLimit = pgrass->GetDistanceFromWaterLevel();
												eWaterState = pgrass->GetUnderwaterState();
												ucMinSlope = pgrass->GetMaxSlopeDegrees();
												ucMaxSlope = pgrass->GetMinSlopeDegrees();
											}
											TESTerrainLODManager::CreateGrass(iCellX, iCellY, iQuad, pland, lseed,
												kVertexPos.m_kNormal, apGrassNode, apCameraPosition,
												&pparam[l]->kGrassParam, pparam[l]->pfDensity, fFadeRange, fMaxRange,
												usHeightLimit, eWaterState, kVertexPos.m_fConstant, ucMaxSlope,
												ucMinSlope);
										}
									}
								}
							}
						}
					} else {
						for (int32_t ixvert_2 = igrassEvalSize; ixvert_2 < 16; ixvert_2 += igrassEvalSquared) {
							for (int32_t iyvert_2 = igrassEvalSize; iyvert_2 < 16; iyvert_2 += igrassEvalSquared) {
								vertIndex = ixvert_2 + 17 * iyvert_2;
								kVertexPos.m_kNormal = pland->GetVertexLocation(static_cast<uint8_t>(iQuad),
									ixvert_2 + 17 * iyvert_2, 0, 0);
								kVertexPos.m_kNormal.x = kVertexPos.m_kNormal.x - 256.0;
								kVertexPos.m_kNormal.y = kVertexPos.m_kNormal.y - 256.0;
								int32_t lseed_2 = TallGrassShaderProperty::Hash_2DPosition(kVertexPos.m_kNormal);
								TallGrassShaderProperty::RemoveFromTerrainChunk(lseed_2);
							}
						}
					}
				}
				plandTextures.RemoveAll();
			}
		} else {
		}
	}
}

void TESTerrainLODManager::CreateGrass(int32_t aiCellx, int32_t aiCelly, int32_t aiBlock, TESObjectLAND* apLand,
	int32_t aiSeed, const NiPoint3& aCoord, NiNode* apGrassNode, NiPoint3 apPosition,
	TallGrassShaderProperty::GrassParam* apGrassParam, float* apfDensity, float fFadeRange, float fMaxRange,
	uint16_t iHeightLimit, TESGrass::GRASS_WATER_STATE iWaterState, float afCellWaterHeight, uint8_t aiMinSlope,
	uint8_t aiMaxSlope) {
	NiPoint3 dst;
	AutoMemContext tempMemContext(MEM_CONTEXT_GRASS, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESTerrainLODManager.cpp", 522);
	NiPointer<NiGeometry> block(static_cast<NiGeometry*>(0));
	float v24 = static_cast<double>(aiMinSlope) * 0.01745329238474369;
	float fminSlope = NiCosQ(v24);
	float v23 = static_cast<double>(aiMaxSlope) * 0.01745329238474369;
	float fmaxSlope = NiCosQ(v23);
	fMaxRange = fMaxRange * fMaxRange;
	bool v17 = TallGrassShaderProperty::ContainsChunk(apGrassParam, aiSeed);
	bool baddlow = !v17;
	if (!v17 && apLand->IsLoaded()) {
		float fgrassArea = static_cast<float>(TESObjectLAND::GetGrassEvalSize() << 8);
		uint16_t igrassSize = static_cast<int32_t>(fgrassArea / apGrassParam->fPositionRange);
		int32_t GrassEvalSize = TESObjectLAND::GetGrassEvalSize();
		uint16_t imaxSize = (GrassEvalSize << 8) / gs_iMinGrassSize_Grass.Int();
		if (igrassSize > static_cast<int32_t>(imaxSize)) {
			igrassSize = imaxSize;
		}
		COORD_DATA aCoordData;
		uint16_t ilowgrasscount = 0;
		for (int32_t i = 0; i < igrassSize; ++i) {
			for (int32_t j = 0; j < igrassSize; ++j) {
				float fY = static_cast<double>(j) * (fgrassArea / static_cast<double>(igrassSize));
				float fX = static_cast<double>(i) * (fgrassArea / static_cast<double>(igrassSize));
				NiPoint3* v19 = ::new (&dst) NiPoint3(fX, fY, 0.0f);
				NiPoint3 pnewpos = aCoord + *v19;
				float fdensy = static_cast<double>(j) * (3.0 / static_cast<double>(igrassSize));
				float fdensx = static_cast<double>(i) * (3.0 / static_cast<double>(igrassSize));
				int32_t idensx = FloatToInt(fdensx);
				int32_t idensy = FloatToInt(fdensy);
				fdensx = fdensx - static_cast<double>(idensx);
				fdensy = fdensy - static_cast<double>(idensy);
				float fd1 = 0.0f;
				float fd2 = 0.0f;
				float fd3 = 0.0f;
				float fd4 = 0.0f;
				fd1 = apfDensity[3 * idensy + idensx];
				if (fdensx > 0.01) {
					if (idensx >= 2) {
						fd2 = apfDensity[3 * idensy + idensx];
					} else {
						fd2 = apfDensity[3 * idensy + 1 + idensx];
					}
					if (fdensy > 0.01) {
						if (idensy >= 2) {
							if (idensx >= 2) {
								fd4 = apfDensity[3 * idensy + idensx];
							} else {
								fd4 = apfDensity[3 * idensy + 1 + idensx];
							}
						} else {
							if (idensx >= 2) {
								fd4 = apfDensity[3 * idensy + 3 + idensx];
							} else {
								fd4 = apfDensity[3 * idensy + 4 + idensx];
							}
						}
					}
				}
				if (fdensy > 0.01) {
					if (idensy >= 2) {
						fd3 = apfDensity[3 * idensy + idensx];
					} else {
						fd3 = apfDensity[3 * idensy + 3 + idensx];
					}
				}
				float fdens = ((1.0 - fdensx) * fd1 + fd2 * fdensx) * (1.0 - fdensy) +
					((1.0 - fdensx) * fd3 + fd4 * fdensx) * fdensy;
				int32_t idens = static_cast<int64_t>(fdens * 32768.0);
				if (idens) {
					int32_t v20 = RandomInt(0, 0x7FFF);
					if (v20 < idens) {
						NiPoint3 vect;
						NiPoint3 plandnormal;
						NiPoint3 pfacenormal;
						float fheight = 0.0f;
						float rndm = RandomFloatNeg1To1();
						pnewpos.x = rndm * apGrassParam->fPositionRange + pnewpos.x;
						rndm = RandomFloatNeg1To1();
						pnewpos.y = rndm * apGrassParam->fPositionRange + pnewpos.y;
						pnewpos.x = FloatFloor(pnewpos.x);
						pnewpos.y = FloatFloor(pnewpos.y);
						pnewpos.z = 0.0f;
						bool bthisCell = apLand->GetCoordData(&aCoordData, pnewpos, 0);
						TES::pInstance->GetLandHeight(pnewpos, fheight);
						switch (iWaterState) {
						case TESGrass::GWS_ABOVE_ONLY_ATLEAST: {
							if (!(fheight < afCellWaterHeight - static_cast<double>(iHeightLimit))) {
								goto LABEL_48;
							}
							break;
						}
						case TESGrass::GWS_ABOVE_ONLY_ATMOST: {
							if (!(afCellWaterHeight > static_cast<double>(fheight)) &&
								!(fheight > static_cast<double>(iHeightLimit) + afCellWaterHeight)) {
								goto LABEL_48;
							}
							break;
						}
						case TESGrass::GWS_BELOW_ONLY_ATLEAST: {
							if (!(fheight > afCellWaterHeight - static_cast<double>(iHeightLimit))) {
								goto LABEL_48;
							}
							break;
						}
						case TESGrass::GWS_BELOW_ONLY_ATMOST: {
							if (!(afCellWaterHeight < static_cast<double>(fheight)) &&
								!(fheight < afCellWaterHeight - static_cast<double>(iHeightLimit))) {
								goto LABEL_48;
							}
							break;
						}
						case TESGrass::GWS_BOTH_ATLEAST: {
							if (!(fheight < static_cast<double>(iHeightLimit) + afCellWaterHeight) ||
								!(fheight > afCellWaterHeight - static_cast<double>(iHeightLimit))) {
								goto LABEL_48;
							}
							break;
						}
						case TESGrass::GWS_BOTH_ATMOST: {
							if (!(fheight > static_cast<double>(iHeightLimit) + afCellWaterHeight) &&
								!(fheight < afCellWaterHeight - static_cast<double>(iHeightLimit))) {
								goto LABEL_48;
							}
							break;
						}
						default: {
						LABEL_48:;
							::new (&vect) NiPoint3(pnewpos - apPosition);
							pnewpos.z = FloatFloor(fheight);
							if (bthisCell) {
								apLand->GetLandNormal(aCoordData, plandnormal, pfacenormal);
							} else {
								TES::pInstance->GetLandNormal(pnewpos, plandnormal, pfacenormal);
							}
							if (apGrassParam->bFitToSlope && apGrassParam->bVertexLighting) {
								plandnormal = pfacenormal;
							} else {
								if (apGrassParam->bFitToSlope && !apGrassParam->bVertexLighting) {
									plandnormal = plandnormal + pfacenormal;
									plandnormal.Unitize();
								}
							}
							if (!(fmaxSlope > static_cast<double>(plandnormal.z)) &&
								!(fminSlope < static_cast<double>(plandnormal.z))) {
								plandnormal.x = plandnormal.x * 0.5 + 0.5;
								if (plandnormal.x >= 0.9700000286102295) {
									plandnormal.x = 0.97f;
								}
								plandnormal.y = plandnormal.y * 0.5 + 0.5;
								if (plandnormal.y >= 0.9700000286102295) {
									plandnormal.y = 0.97f;
								}
								plandnormal.z = plandnormal.z * 0.5 + 0.5;
								if (plandnormal.z >= 0.9700000286102295) {
									plandnormal.z = 0.97f;
								}
								pnewpos.x = pnewpos.x + plandnormal.x;
								pnewpos.y = pnewpos.y + plandnormal.y;
								pnewpos.z = pnewpos.z + plandnormal.z;
								NiColorA kcol(0.0f, 0.0f, 0.0f, 0.0f);
								if (bthisCell) {
									apLand->GetLandColor(aCoordData, kcol);
								} else {
									TES::pInstance->GetLandColor(pnewpos, kcol);
								}
								float flum = kcol.r * 0.31 + kcol.g * 0.37 + kcol.b * 0.32;
								vect = pnewpos - apPosition;
								TESTerrainLODManager::pLowpositions[ilowgrasscount] = pnewpos;
								TESTerrainLODManager::pLowcolors[ilowgrasscount++] = flum;
							}
							break;
						}
						}
					}
				}
			}
		}
		if (ilowgrasscount) {
			TallGrassShaderProperty::AddGrass(aiSeed, aiCellx, aiCelly, apGrassNode, apGrassParam,
				TESTerrainLODManager::pLowpositions, TESTerrainLODManager::pLowcolors, ilowgrasscount);
		}
	} else {
	}
}

float NiCosQ(float afRadians) {
	return s_fCosTableA[static_cast<int32_t>(512.0 / NI_TWO_PI * afRadians) & 511];
}
