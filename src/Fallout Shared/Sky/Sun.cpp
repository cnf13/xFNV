#include "Fallout Shared/Sky/Sun.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderData.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/SkyShaderProperty.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyObject.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWeather.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <cmath>

SettingT<GameSettingCollection> Sun::fSunMinimumGlareScale("fSunMinimumGlareScale", 0.25f);
SettingT<GameSettingCollection> Sun::fSunReduceGlareSpeed("fSunReduceGlareSpeed", 4.0f);
SettingT<GameSettingCollection> Sun::fSunAlphaTransTime("fSunAlphaTransTime", 2.0f);
SettingT<GameSettingCollection> Sun::fSunXExtreme("fSunXExtreme", -400.0f);
SettingT<GameSettingCollection> Sun::fSunYExtreme("fSunYExtreme", 25.0f);
SettingT<GameSettingCollection> gs_fSunZExtreme("fSunZExtreme", -100.0f);
SettingT<INISettingCollection> Sun::fSunBaseSize("fSunBaseSize:Weather", 250.0f);
SettingT<INISettingCollection> Sun::fSunGlareSize("fSunGlareSize:Weather", 350.0f);

Sun::Sun()
	: spSunBaseNode(nullptr), spSunGlareNode(nullptr), spSunBase(nullptr), spSunGlare(nullptr), pSunGlarePick(nullptr),
	  spLight(nullptr), fGlareScale(0.0f), bDoOcclusionTests(false), spSunAccumulator(nullptr) {
	spSunAccumulator = new BSShaderAccumulator(100, 1u, 1u);
}

Sun::~Sun() {
	delete pSunGlarePick;
}

// Builds the sun base and glare quads (each under a billboard node facing the camera), the sun light, the glare's
// occlusion pick, and gives both quads sky shader properties. The quads are fSunBaseSize and fSunGlareSize half-wide.
// The arrays are new[]'d through ourMalloc with the compiler's constructor iterator. Each NiPoint3 argument list reads
// its y before its x, as the exe evaluates arguments right to left; the negated halves round to float locals as the
// exe's fstp dword does.
void Sun::Initialize(NiNode* apSkyRoot) {
	SkyObject::Initialize(apSkyRoot);

	void* pMemory = ourMalloc(4 * sizeof(NiPoint3));
	NiPoint3* psunVertices = nullptr;
	if (pMemory) {
		rt::eh_vector_constructor_iterator(pMemory, sizeof(NiPoint3), 4, &emptyPointConstructor);
		psunVertices = static_cast<NiPoint3*>(pMemory);
	}
	float fY = fSunBaseSize.Float();
	float fX = static_cast<float>(fSunBaseSize.Float() * -1.0);
	psunVertices[0] = NiPoint3(fX, fY, 0.0f);
	fY = static_cast<float>(fSunBaseSize.Float() * -1.0);
	fX = static_cast<float>(fSunBaseSize.Float() * -1.0);
	psunVertices[1] = NiPoint3(fX, fY, 0.0f);
	fY = fSunBaseSize.Float();
	fX = fSunBaseSize.Float();
	psunVertices[2] = NiPoint3(fX, fY, 0.0f);
	fY = static_cast<float>(fSunBaseSize.Float() * -1.0);
	fX = fSunBaseSize.Float();
	psunVertices[3] = NiPoint3(fX, fY, 0.0f);

	pMemory = ourMalloc(4 * sizeof(NiPoint3));
	NiPoint3* psunNormals = nullptr;
	if (pMemory) {
		rt::eh_vector_constructor_iterator(pMemory, sizeof(NiPoint3), 4, &emptyPointConstructor);
		psunNormals = static_cast<NiPoint3*>(pMemory);
	}
	psunNormals[0] = NiPoint3_ZERO;
	psunNormals[1] = NiPoint3_ZERO;
	psunNormals[2] = NiPoint3_ZERO;
	psunNormals[3] = NiPoint3_ZERO;

	pMemory = ourMalloc(4 * sizeof(NiPoint3));
	NiPoint3* pglareVertices = nullptr;
	if (pMemory) {
		rt::eh_vector_constructor_iterator(pMemory, sizeof(NiPoint3), 4, &emptyPointConstructor);
		pglareVertices = static_cast<NiPoint3*>(pMemory);
	}
	fY = fSunGlareSize.Float();
	fX = static_cast<float>(fSunGlareSize.Float() * -1.0);
	pglareVertices[0] = NiPoint3(fX, fY, 0.0f);
	fY = static_cast<float>(fSunGlareSize.Float() * -1.0);
	fX = static_cast<float>(fSunGlareSize.Float() * -1.0);
	pglareVertices[1] = NiPoint3(fX, fY, 0.0f);
	fY = fSunGlareSize.Float();
	fX = fSunGlareSize.Float();
	pglareVertices[2] = NiPoint3(fX, fY, 0.0f);
	fY = static_cast<float>(fSunGlareSize.Float() * -1.0);
	fX = fSunGlareSize.Float();
	pglareVertices[3] = NiPoint3(fX, fY, 0.0f);

	pMemory = ourMalloc(4 * sizeof(NiPoint3));
	NiPoint3* pglareNormals = nullptr;
	if (pMemory) {
		rt::eh_vector_constructor_iterator(pMemory, sizeof(NiPoint3), 4, &emptyPointConstructor);
		pglareNormals = static_cast<NiPoint3*>(pMemory);
	}
	pglareNormals[0] = NiPoint3_ZERO;
	pglareNormals[1] = NiPoint3_ZERO;
	pglareNormals[2] = NiPoint3_ZERO;
	pglareNormals[3] = NiPoint3_ZERO;

	{
		// The exe builds the colour with NiColorA's constructor (0x414430).
		alignas(NiColorA) uint8_t aucDefaultColor[sizeof(NiColorA)];
		const NiColorA& defaultColor = *::new (aucDefaultColor) NiColorA(1.0f, 0.0f, 0.0f, 1.0f);

		pMemory = ourMalloc(4 * sizeof(NiColorA));
		NiColorA* psunColors = nullptr;
		if (pMemory) {
			rt::eh_vector_constructor_iterator(pMemory, sizeof(NiColorA), 4, &fnvd::constructElement<NiColorA>);
			psunColors = static_cast<NiColorA*>(pMemory);
		}
		psunColors[0] = defaultColor;
		psunColors[1] = defaultColor;
		psunColors[2] = defaultColor;
		psunColors[3] = defaultColor;

		pMemory = ourMalloc(4 * sizeof(NiColorA));
		NiColorA* pglareColors = nullptr;
		if (pMemory) {
			rt::eh_vector_constructor_iterator(pMemory, sizeof(NiColorA), 4, &fnvd::constructElement<NiColorA>);
			pglareColors = static_cast<NiColorA*>(pMemory);
		}
		pglareColors[0] = defaultColor;
		pglareColors[1] = defaultColor;
		pglareColors[2] = defaultColor;
		pglareColors[3] = defaultColor;

		pMemory = ourMalloc(4 * sizeof(NiPoint2));
		NiPoint2* psunTexture = nullptr;
		if (pMemory) {
			rt::eh_vector_constructor_iterator(pMemory, sizeof(NiPoint2), 4, &emptyPointConstructor);
			psunTexture = static_cast<NiPoint2*>(pMemory);
		}
		psunTexture[0] = NiPoint2(0.0f, 0.0f);
		psunTexture[1] = NiPoint2(0.0f, 1.0f);
		psunTexture[2] = NiPoint2(1.0f, 0.0f);
		psunTexture[3] = NiPoint2(1.0f, 1.0f);

		pMemory = ourMalloc(4 * sizeof(NiPoint2));
		NiPoint2* pglareTexture = nullptr;
		if (pMemory) {
			rt::eh_vector_constructor_iterator(pMemory, sizeof(NiPoint2), 4, &emptyPointConstructor);
			pglareTexture = static_cast<NiPoint2*>(pMemory);
		}
		pglareTexture[0] = NiPoint2(0.0f, 0.0f);
		pglareTexture[1] = NiPoint2(0.0f, 1.0f);
		pglareTexture[2] = NiPoint2(1.0f, 0.0f);
		pglareTexture[3] = NiPoint2(1.0f, 1.0f);

		// Two triangles per quad.
		uint16_t* ssunVertOrder = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
		ssunVertOrder[0] = 0;
		ssunVertOrder[1] = 1;
		ssunVertOrder[2] = 2;
		ssunVertOrder[3] = 2;
		ssunVertOrder[4] = 1;
		ssunVertOrder[5] = 3;
		uint16_t* sglareVertOrder = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
		sglareVertOrder[0] = 0;
		sglareVertOrder[1] = 1;
		sglareVertOrder[2] = 2;
		sglareVertOrder[3] = 2;
		sglareVertOrder[4] = 1;
		sglareVertOrder[5] = 3;

		pMemory = NiMemObject::operator new(sizeof(NiTriShape));
		NiTriShape* pSunBase = pMemory ? ::new (pMemory) NiTriShape(4, psunVertices, psunNormals, psunColors,
											 psunTexture, 1, NiGeometryData::NBT_METHOD_NONE, 2, ssunVertOrder)
									   : nullptr;
		spSunBase = pSunBase;
		spSunBase->SetSelectiveUpdate(true);

		pMemory = NiMemObject::operator new(sizeof(NiBillboardNode));
		NiBillboardNode* pSunBaseNode = pMemory ? ::new (pMemory) NiBillboardNode : nullptr;
		spSunBaseNode = pSunBaseNode;
		spSunBaseNode->SetSelectiveUpdate(true);
		spSunBaseNode->SetMode(NiBillboardNode::RIGID_FACE_CAMERA);
		NiNode* pParent = spSunBaseNode.operator->();
		pParent->AttachChild(spSunBase.operator->(), true);

		pMemory = NiMemObject::operator new(sizeof(NiTriShape));
		NiTriShape* pSunGlare = pMemory ? ::new (pMemory) NiTriShape(4, pglareVertices, pglareNormals, pglareColors,
											  pglareTexture, 1, NiGeometryData::NBT_METHOD_NONE, 2, sglareVertOrder)
										: nullptr;
		spSunGlare = pSunGlare;
		spSunGlare->SetSelectiveUpdate(true);
	}

	pMemory = NiMemObject::operator new(sizeof(NiBillboardNode));
	NiBillboardNode* pSunGlareNode = pMemory ? ::new (pMemory) NiBillboardNode : nullptr;
	spSunGlareNode = pSunGlareNode;
	spSunGlareNode->SetSelectiveUpdate(true);
	spSunGlareNode->SetMode(NiBillboardNode::RIGID_FACE_CAMERA);
	float fScale = fGlareScale;
	spSunGlareNode->SetScale(fScale);
	NiNode* pParent = spSunGlareNode.operator->();
	pParent->AttachChild(spSunGlare.operator->(), true);
	// The exe reads spRoot through NiPointer::operator-> (0x559450); src's SkyObject keeps it as a raw pointer.
	pParent = spRoot;
	pParent->AttachChild(spSunBaseNode.operator->(), true);

	pMemory = NiMemObject::operator new(sizeof(NiDirectionalLight));
	NiDirectionalLight* pLight = pMemory ? ::new (pMemory) NiDirectionalLight : nullptr;
	spLight = pLight;
	spLight->SetDimmer(1.0f);
	spLight->SetAmbientColor(NiColor_WHITE);
	spLight->SetDiffuseColor(NiColor_WHITE);
	// The exe reads spRoot through NiPointer::operator-> (0x559450); src's SkyObject keeps it as a raw pointer.
	pParent = spRoot;
	pParent->AttachChild(spLight.operator->(), true);

	pMemory = ourMalloc(sizeof(NiPick));
	pSunGlarePick = pMemory ? ::new (pMemory) NiPick(0, 8) : nullptr;
	pSunGlarePick->SetPickType(NiPick::FIND_FIRST);
	pSunGlarePick->SetIntersectType(NiPick::TRIANGLE_INTERSECT);
	pSunGlarePick->SetFrontOnly(true);
	pSunGlarePick->SetObserveAppCullFlag(true);

	alignas(BSShaderData) uint8_t aShaderDataStorage[sizeof(BSShaderData)];
	BSShaderData* pbsshaderdata = ::new (aShaderDataStorage) BSShaderData;
	pbsshaderdata->eLightingShader = BSShaderData::LS_SKY;
	pbsshaderdata->iSkyTYpe = SkyShaderProperty::SO_SUN;
	pbsshaderdata->Convert(spSunBase.operator->(), BSShaderData::PROP_SHADER);
	pbsshaderdata->iSkyTYpe = SkyShaderProperty::SO_SUN_GLARE;
	pbsshaderdata->Convert(spSunGlare.operator->(), BSShaderData::PROP_SHADER);
}

// The exe's cleanup-only C++ EH frame is left out.
void Sun::Update(Sky* apSky, float afSecondsPassed) {
	NiMatrix3 rotmat_4;
	NiMatrix3 rotmat_2;
	NiMatrix3 newSunRot;
	NiMatrix3 lightDir_2;
	NiMatrix3 lightDir;
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, tempMemContext, &AutoMemContext::Destruct, MEM_CONTEXT_SKY, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Sky\\Sun.cpp", 300);
	NiPoint3 newLoc = NiPoint3_ZERO;
	float fhour = apSky->GetCurrentGameHour();
	SkyObject::Update(apSky, afSecondsPassed);
	if (apSky->eMode) {
		if (apSky->eMode == Sky::SM_INTERIOR) {
			TESObjectCELL* pcell_3;
			lightDir_2 = NiMatrix3_IDENTITY;
			if (TES::pInstance) {
				pcell_3 = TES::pInstance->pInteriorCell;
			} else {
				pcell_3 = 0;
			}
			if (pcell_3) {
				NiMatrix3 rotmat_3;
				float v44 = static_cast<double>(pcell_3->GetDataDirectionalXY()) * 0.01745329238474369;
				lightDir_2.MakeZRotation(v44);
				float v43 = static_cast<double>(pcell_3->GetDataDirectionalZ()) * 0.01745329238474369;
				rotmat_3.MakeYRotation(v43);
				lightDir_2 = *::new (&rotmat_4) NiMatrix3(lightDir_2 * rotmat_3);
			} else {
				lightDir_2.SetEntry(1u, 0, -0.7853982f);
				lightDir_2.SetEntry(2u, 0, -0.7853982f);
			}
			spLight.m_pObject->SetRotate(lightDir_2);
			// The exe's 0x4EA950 (a fold owned by TESDataHandler::GetRaceList) returns &SkyColor[3] (Sky + 0x60), the
			// ambient light color (the PDB's Sky::GetAmbientLightColor).
			NiColor* RaceList = &apSky->SkyColor[3];
			spLight.m_pObject->SetAmbientColor(*RaceList);
			spLight.m_pObject->SetDiffuseColor(*apSky->GetSunLightColor());
			spLight.m_pObject->SetSpecularColor(*&apSky->SunSpecularColor);
		} else {
			if (apSky->eMode == Sky::SM_FULL || apSky->eMode == Sky::SM_FAKEEXTERIOR) {
				SkyShaderProperty* pshadeProperty_2;
				int32_t v34;
				SkyShaderProperty* pshadeProperty;
				int32_t v36;
				float v37;
				float WeatherData;
				float ftemp;
				bool DisplayObject = spRoot.m_pObject->GetDisplayObject();
				spRoot.m_pObject->SetAppCulled(!DisplayObject);
				double SunriseEnd = apSky->GetSunriseEnd();
				float fSunriseBegin = (apSky->GetSunriseBegin() + SunriseEnd) / 2.0;
				double SunsetEnd = apSky->GetSunsetEnd();
				float fSunsetEnd = (apSky->GetSunsetBegin() + SunsetEnd) / 2.0;
				float fhalftrans = Sun::fSunAlphaTransTime.Float() * 0.5;
				float fAlphaInBegin = fSunriseBegin - fhalftrans;
				float fAlphaInEnd = fSunriseBegin + fhalftrans;
				float fAlphaOutBegin = fSunsetEnd - fhalftrans;
				float fAlphaOutEnd = fSunsetEnd + fhalftrans;
				float fDiscAlpha = 1.0f;
				if (fhour >= static_cast<double>(fAlphaInBegin) && fAlphaOutEnd >= static_cast<double>(fhour)) {
					if (!(fhour > static_cast<double>(fAlphaInBegin)) || !(fAlphaInEnd > static_cast<double>(fhour))) {
						if (fhour > static_cast<double>(fAlphaOutBegin) && fAlphaOutEnd > static_cast<double>(fhour)) {
							fDiscAlpha = 1.0 - (fhour - fAlphaOutBegin) / (fAlphaOutEnd - fAlphaOutBegin);
						}
					} else {
						fDiscAlpha = (fhour - fAlphaInBegin) / (fAlphaInEnd - fAlphaInBegin);
					}
				} else {
					fDiscAlpha = 0.0f;
				}
				TESWeather* pCurr = apSky->pCurrentWeather;
				TESWeather* pLast = apSky->pLastWeather;
				float fCurrPct = apSky->GetWeatherPercent();
				if (pCurr) {
					float v40 = fCurrPct * fDiscAlpha;
					WeatherData = pCurr->GetWeatherData(TESWeather::WD_SUN_GLARE, v40, 0.0f);
				} else {
					WeatherData = fDiscAlpha;
				}
				if (pLast) {
					float v38 = (1.0 - fCurrPct) * fDiscAlpha;
					v37 = pLast->GetWeatherData(TESWeather::WD_SUN_GLARE, v38, 0.0f);
				} else {
					v37 = 0.0f;
				}
				float fGlareAlpha = WeatherData + v37;
				if (spSunBase.m_pObject->GetProperty(3)) {
					NiProperty* Property = spSunBase.m_pObject->GetProperty(3);
					// The exe's 0x441110 is NiShadeProperty::GetShaderPropertyType, a field getter.
					v36 = static_cast<NiShadeProperty*>(Property)->iShaderPropertyType ==
						BSShaderManager::BSSM_PROPERTYTYPE_SKY;
				} else {
					v36 = 0;
				}
				if (v36) {
					pshadeProperty = static_cast<SkyShaderProperty*>(spSunBase.m_pObject->GetProperty(3));
				} else {
					pshadeProperty = 0;
				}
				SkyShaderProperty* v66 = pshadeProperty;
				if (pshadeProperty) {
					// The exe's 0x461110 (a fold owned by TESDataHandler::GetCameraShotList) returns &SkyColor[5]
					// (Sky + 0x78), the sun color; the exe builds the colour with its constructor (0x414430) and
					// copies it into the property's vertex colour.
					FNVD_CONSTRUCTED_LOCAL(NiColorA, kSunColor, nullptr, apSky->SkyColor[5].r, apSky->SkyColor[5].g,
						apSky->SkyColor[5].b, fDiscAlpha);
					v66->VertexColor = kSunColor;
				}
				if (spSunGlare.m_pObject->GetProperty(3)) {
					NiProperty* v10 = spSunGlare.m_pObject->GetProperty(3);
					v34 = static_cast<NiShadeProperty*>(v10)->iShaderPropertyType ==
						BSShaderManager::BSSM_PROPERTYTYPE_SKY;
				} else {
					v34 = 0;
				}
				if (v34) {
					pshadeProperty_2 = static_cast<SkyShaderProperty*>(spSunGlare.m_pObject->GetProperty(3));
				} else {
					pshadeProperty_2 = 0;
				}
				v66 = pshadeProperty_2;
				if (pshadeProperty_2) {
					FNVD_CONSTRUCTED_LOCAL(NiColorA, kGlareColor, nullptr, apSky->SkyColor[5].r, apSky->SkyColor[5].g,
						apSky->SkyColor[5].b, fGlareAlpha);
					v66->VertexColor = kGlareColor;
				}
				float fxNewAim = 0.0f;
				newSunRot = NiMatrix3_IDENTITY;
				if (!(fhour > static_cast<double>(fAlphaInBegin)) || !(fAlphaOutEnd > static_cast<double>(fhour))) {
					if (!(fAlphaOutEnd <= static_cast<double>(fhour))) {
						if (fAlphaInBegin >= static_cast<double>(fhour)) {
							ftemp = (24.0 - fAlphaOutEnd + fhour) / (24.0 - (fAlphaOutEnd - fAlphaInBegin));
							double v30 = ftemp * 2.0 - 1.0;
							fxNewAim = Sun::fSunXExtreme.Float() * v30;
						}
					} else {
						ftemp = (fhour - fAlphaOutEnd) / (24.0 - (fAlphaOutEnd - fAlphaInBegin));
						double v31 = ftemp * 2.0 - 1.0;
						fxNewAim = Sun::fSunXExtreme.Float() * v31;
					}
				} else {
					ftemp = (fhour - fAlphaInBegin) / (fAlphaOutEnd - fAlphaInBegin);
					double v32 = 1.0 - ftemp * 2.0;
					fxNewAim = Sun::fSunXExtreme.Float() * v32;
				}
				newLoc.x = fxNewAim;
				newLoc.y = Sun::fSunYExtreme.operator float();
				float v24 = Sun::fSunXExtreme.operator float();
				double v29 = std::fabsf(v24);
				newLoc.z = v29 - std::fabsf(fxNewAim);
				spRoot.m_pObject->SetTranslate(newLoc);
				spSunGlareNode.m_pObject->SetTranslate(newLoc);
				float v25 = gs_fSunZExtreme.operator float();
				float fMaxb = Sun::fSunYExtreme.operator float();
				NiPoint3 ksundirection(-fxNewAim, fMaxb, v25);
				ksundirection.Unitize();
				newSunRot.SetCol(0, ksundirection);
				spLight.m_pObject->SetRotate(newSunRot);
				NiColor* v14 = &apSky->SkyColor[3];
				spLight.m_pObject->SetAmbientColor(*v14);
				spLight.m_pObject->SetDiffuseColor(*apSky->GetSunLightColor());
				spLight.m_pObject->SetSpecularColor(*&apSky->SunSpecularColor);
				if (TES::pInstance) {
					if (TES::pInstance->pObjRoot) {
						float* v16;
						SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
						NiPoint3 v54 = WorldSceneGraph->GetCameraRoot()->GetTranslate();
						NiPoint3 direction = newLoc;
						float fscaleChange = Sun::fSunReduceGlareSpeed.Float() * afSecondsPassed;
						direction.Unitize();
						bool v28 = true;
						if (!(GetSunOcclusionPercent() >= 1.0)) {
							if (!TESWaterSystem::GetPlayerUnderwater()) {
								v28 = false;
							}
						}
						float v27 = 1.0 - GetSunOcclusionPercent();
						float fscale = std::pow(v27, 2.0f);
						StopSunOcclusion();
						if (v28 || (v16 = &Sun::fSunMinimumGlareScale.Float(), *v16 > static_cast<double>(fscale))) {
							double v26 = fGlareScale - fscaleChange;
							if (!(Sun::fSunMinimumGlareScale.Float() > v26)) {
								spSunGlareNode.m_pObject->SetAppCulled(0);
								fGlareScale = fGlareScale - fscaleChange;
								spSunGlareNode.m_pObject->SetScale(fGlareScale);
							} else {
								spSunGlareNode.m_pObject->SetAppCulled(1);
								fGlareScale = 0.0f;
							}
						} else {
							if (!(fscale <= fGlareScale + fscaleChange) &&
								!(fscale <= static_cast<double>(fGlareScale))) {
								fGlareScale = fGlareScale + fscaleChange;
							} else {
								fGlareScale = fscale;
							}
							spSunGlareNode.m_pObject->SetAppCulled(0);
							spSunGlareNode.m_pObject->SetScale(fGlareScale);
						}
						if (!spSunGlareNode.m_pObject->GetDisplayObject()) {
							spSunGlareNode.m_pObject->SetAppCulled(1);
						}
					}
				}
				if (apSky->IsUnderwater()) {
					GetSunBaseNode()->SetAppCulled(1);
				} else {
					GetSunBaseNode()->SetAppCulled(0);
				}
			}
		}
	} else {
		TESObjectCELL* pcell;
		lightDir = NiMatrix3_IDENTITY;
		if (TES::pInstance) {
			pcell = TES::pInstance->pInteriorCell;
		} else {
			pcell = 0;
		}
		if (pcell) {
			NiMatrix3 rotmat;
			float v47 = static_cast<double>(pcell->GetDataDirectionalXY()) * 0.01745329238474369;
			lightDir.MakeZRotation(v47);
			float v46 = static_cast<double>(pcell->GetDataDirectionalZ()) * 0.01745329238474369;
			rotmat.MakeYRotation(v46);
			lightDir = *::new (&rotmat_2) NiMatrix3(lightDir * rotmat);
		} else {
			lightDir.SetEntry(1u, 0, -0.7853982f);
			lightDir.SetEntry(2u, 0, -0.7853982f);
		}
		spLight.m_pObject->SetRotate(lightDir);
		NiColor* v3 = &apSky->SkyColor[3];
		spLight.m_pObject->SetAmbientColor(*v3);
		spLight.m_pObject->SetDiffuseColor(*apSky->GetSunLightColor());
		spLight.m_pObject->SetSpecularColor(*&apSky->SunSpecularColor);
	}
}

NiAVObject* Sun::GetBase() {
	return spSunBase;
}

// Same function as NiAVObject::GetCollisionObject (0x6838B0), registered there: the exe's Sky::GetLight calls it out
// of line, and its body reads spLight through the out-of-line NiPointer::operator->.
NiDirectionalLight* Sun::GetSunLight() {
	return spLight;
}

NiBillboardNode* Sun::GetSunGlareNode() {
	return spSunGlareNode;
}

// Same function as BGSBodyPart::GetIKStartNodeName (0x63D040), registered there: its body
// reads spSunGlare through the out-of-line NiPointer::operator->.
NiTriShape* Sun::GetSunGlareGeo() const {
	return spSunGlare;
}

float Sun::GetIntensity() {
	NiTriShape* v2;
	NiShadeProperty* property;
	NiTriShape* v4;
	SkyShaderProperty* v7;
	int32_t v8;
	NiTriShape* v1 = (&spSunGlare)->m_pObject;
	if (v1->GetProperty(3)) {
		v2 = (&spSunGlare)->m_pObject;
		property = static_cast<NiShadeProperty*>(v2->GetProperty(3));
		v8 = (int32_t)(property->GetShaderPropertyType() == 13);
	} else {
		v8 = 0;
	}
	if (v8) {
		v4 = (&spSunGlare)->m_pObject;
		v7 = static_cast<SkyShaderProperty*>(v4->GetProperty(3));
	} else {
		v7 = nullptr;
	}
	if (v7) {
		return v7->VertexColor.a;
	} else {
		return 0.0;
	}
}

// Clears the texture and texture file of the sky shader property on the sun base and on the glare.
void Sun::RemoveTextures() {
	NiProperty* pProperty = spSunBase->GetProperty(NiProperty::SHADE);
	SkyShaderProperty* pShaderProperty;
	// EXE-CALLED 0x653270 (the out-of-line static DynamicCast)
	pShaderProperty =
		static_cast<SkyShaderProperty*>(NiObject::DynamicCastStatic(&SkyShaderProperty::ms_RTTI, pProperty));
	if (pShaderProperty) {
		pShaderProperty->SetTexture(nullptr);
		pShaderProperty->SetTextureFile(EmptyFS);
	}
	pProperty = spSunGlare->GetProperty(NiProperty::SHADE);
	pShaderProperty =
		static_cast<SkyShaderProperty*>(NiObject::DynamicCastStatic(&SkyShaderProperty::ms_RTTI, pProperty));
	if (pShaderProperty) {
		pShaderProperty->SetTexture(nullptr);
		pShaderProperty->SetTextureFile(EmptyFS);
	}
}

void Sun::StartSunOcclusion() {
	bDoOcclusionTests = (bool)1;
	BSShaderAccumulator* v1 = (&spSunAccumulator)->operator->();
	if (v1) {
		v1->ResetSunOcclusion();
	}
}

void Sun::StopSunOcclusion() {
	bDoOcclusionTests = (bool)0;
}

// The exe's cleanup-only C++ EH frame is left out.
void Sun::CalculateSunGlare(uint32_t aiTestNumber, BSShaderAccumulator* apAccumulator, NiCamera* apCamera) {
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, apMemory, &AutoMemContext::Destruct, MEM_CONTEXT_SKY, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Sky\\Sun.cpp", 546);
	if (GetBase()) {
		if (bDoOcclusionTests) {
			if (apAccumulator) {
				spSunAccumulator.operator=(apAccumulator);
				NiTriShape* pSunBase = static_cast<NiTriShape*>(GetBase());
				float worldScale = pSunBase->GetWorldScale() / 25.0;
				pSunBase->SetWorldScale(worldScale);
				apAccumulator->DoSunOcclusionQuery(pSunBase, aiTestNumber, apCamera);
				float worldScale2 = pSunBase->GetWorldScale() * 25.0;
				pSunBase->SetWorldScale(worldScale2);
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
float Sun::GetSunOcclusionPercent() {
	FNVD_INLINED_LOCAL(AutoMemContext, tempMemContext, std::nullopt, std::nullopt);
	::new (&tempMemContext)
		AutoMemContext(MEM_CONTEXT_SKY, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Sky\\Sun.cpp", 565);
	BSShaderAccumulator* paccum = spSunAccumulator.m_pObject;
	if (paccum) {
		float WeaponMaxRangeMult = paccum->fPercentSunOccludedStored;
		tempMemContext.~AutoMemContext();
		return WeaponMaxRangeMult;
	} else {
		tempMemContext.~AutoMemContext();
		return 0.0f;
	}
}
