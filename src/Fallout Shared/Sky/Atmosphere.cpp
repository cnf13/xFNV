#include "Fallout Shared/Sky/Atmosphere.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSStream.h"
#include "BSShader/BSFogProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/SkyShader.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/Sky/SkyObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"

Atmosphere::Atmosphere()
	: spAtmosphere(nullptr), spFog(nullptr), spSkyQuadNode(nullptr), spSkyQuad(nullptr), bUpdateFogDistance(true) {}

// The exe's destructor sequence (Destruct) only releases the pointers, which their own destructors do.
Atmosphere::~Atmosphere() {}

void Atmosphere::Update(Sky* apSky, float afSecondsPassed) {
	if (!apSky) {
		return;
	}
	if (BSShaderManager::GetFogProperty(0) != spFog.m_pObject) {
		BSShaderManager::SetFogProperty(0, spFog.m_pObject);
	}
	SkyObject::Update(apSky, afSecondsPassed);
	if (apSky->eMode == Sky::SM_FULL || apSky->eMode == Sky::SM_FAKEEXTERIOR) {
		if (!spAtmosphere.m_pObject) {
			return;
		}
		const NiPoint3* pCameraPos;
		if (pPlayer && pPlayer->GetParentCell() &&
			(pCameraPos = &Main::GetWorldSceneGraph()->GetCameraRoot()->GetWorldTranslate(),
				static_cast<double>(pCameraPos->z) < pPlayer->GetRelevantWaterHeight())) {
			// Under water: all three blend colours are SkyColor[1] (the exe's getter at +0x48).
			FNVD_CONSTRUCTED_LOCAL(NiColorA, kColor0, nullptr, apSky->SkyColor[1].r, apSky->SkyColor[1].g,
				apSky->SkyColor[1].b, 1.0f);
			Sky::BlendColor[0] = kColor0;
			FNVD_CONSTRUCTED_LOCAL(NiColorA, kColor1, nullptr, apSky->SkyColor[1].r, apSky->SkyColor[1].g,
				apSky->SkyColor[1].b, 1.0f);
			Sky::BlendColor[1] = kColor1;
			FNVD_CONSTRUCTED_LOCAL(NiColorA, kColor2, nullptr, apSky->SkyColor[1].r, apSky->SkyColor[1].g,
				apSky->SkyColor[1].b, 1.0f);
			Sky::BlendColor[2] = kColor2;
		} else {
			// SkyColor[8], SkyColor[7] and SkyColor[0] (the exe's getters at +0x9C, +0x90 and +0x3C).
			FNVD_CONSTRUCTED_LOCAL(NiColorA, kColor0, nullptr, apSky->SkyColor[8].r, apSky->SkyColor[8].g,
				apSky->SkyColor[8].b, 1.0f);
			Sky::BlendColor[0] = kColor0;
			FNVD_CONSTRUCTED_LOCAL(NiColorA, kColor1, nullptr, apSky->SkyColor[7].r, apSky->SkyColor[7].g,
				apSky->SkyColor[7].b, 1.0f);
			Sky::BlendColor[1] = kColor1;
			FNVD_CONSTRUCTED_LOCAL(NiColorA, kColor2, nullptr, apSky->SkyColor[0].r, apSky->SkyColor[0].g,
				apSky->SkyColor[0].b, 1.0f);
			Sky::BlendColor[2] = kColor2;
		}
	}
	if (spFog.m_pObject) {
		const float* pfFogDistances = apSky->fFogDistances;
		spFog.m_pObject->SetFog(pfFogDistances[0] < pfFogDistances[1]);
		if (bUpdateFogDistance) {
			spFog.m_pObject->SetDistance(pfFogDistances[0], pfFogDistances[1]);
			spFog.m_pObject->SetPower(apSky->GetCellFogPower());
		}
		if (BSShaderManager::bHDR) {
			spFog.m_pObject->SetColor(&apSky->SkyColor[1].r);
		} else {
			float fDimmer = BSShaderManager::fSunlightDimmer / 3.75;
			NiColor kFogColor = apSky->SkyColor[1] * fDimmer;
			spFog.m_pObject->SetColor(&kFogColor.r);
		}
		// The exe reads the fog height once into a local it never uses, then again for the call.
		[[maybe_unused]] float fUnusedHeight = apSky->GetFogHeight();
		spFog.m_pObject->SetFogHeight(apSky->GetFogHeight());
		spFog.m_pObject->SetWaterDistance(pfFogDistances[2], pfFogDistances[3]);
		spFog.m_pObject->SetWaterColor(apSky->WaterFogColor);
	}
	if (bUpdateFogDistance) {
		float fFar = Main::GetWorldSceneGraph()->GetFarDistance();
		NiCamera* pCamera = Main::GetWorldSceneGraph()->GetCamera();
		if (pCamera) {
			NiFrustum kFrustum = pCamera->GetViewFrustum();
			if (kFrustum.m_fFar != fFar) {
				kFrustum.m_fFar = fFar;
				float fRatio = fFar / kFrustum.m_fNear;
				pCamera->SetMaxFarNearRatio(fRatio);
				pCamera->SetViewFrustum(kFrustum);
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
// The exe's cleanup-only C++ EH frame is left out.
void Atmosphere::Initialize(NiNode* apSkyRoot, BSFogProperty* apFog) {
	FNVD_INLINED_LOCAL(Model, model, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSStream, stream, std::nullopt, std::nullopt);
	SkyObject::Initialize(apSkyRoot);
	if (!BSShaderManager::bHDR) {
		NiNode* v8;
		SkyShader::CreateSkyQuad(spSkyQuad);
		spSkyQuad.m_pObject->SetSelectiveUpdate(1);
		SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
		NiNode* CameraRoot = WorldSceneGraph->GetCameraRoot();
		spSkyQuad.m_pObject->SetTranslate(CameraRoot->GetTranslate());
		NiNode* apMemory = static_cast<NiNode*>(NiMemObject::operator new(sizeof(NiNode)));
		if (apMemory) {
			v8 = ::new (apMemory) NiNode(0);
		} else {
			v8 = 0;
		}
		spSkyQuadNode.operator=(v8);
		spSkyQuadNode.m_pObject->SetSelectiveUpdate(1);
		NiNode* v7 = spSkyQuadNode.m_pObject;
		v7->AttachChild(spSkyQuad.m_pObject, 1);
		NiNode* v6 = spRoot.m_pObject;
		v6->AttachChild(spSkyQuadNode.m_pObject, 1);
	}
	::new (&stream) BSStream;
	if (stream.BSStream::Load("Meshes\\Sky\\Atmosphere.nif", 0)) {
		::new (&model) Model("Meshes\\Sky\\Atmosphere.nif", &stream, 0, 0);
		NiNode* pnode = model.GetModel();
		NiTriShape* ptrishape = 0;
		NiObject* pobj = model.GetModel();
		if (pobj) {
			if (pobj->IsNode()) {
				ptrishape = static_cast<NiTriShape*>(static_cast<NiNode*>(pobj)->NiNode::GetAtChecked(0));
			} else {
				if (NiObject::IsKindOf_0(&NiTriShape::ms_RTTI, pobj)) {
					ptrishape = static_cast<NiTriShape*>(pobj);
				}
			}
		}
		if (!ptrishape) {
			Warning("Cannot load the atmosphere model.");
			model.~Model();
			stream.~BSStream();
			return;
		}
		spAtmosphere.operator=(ptrishape);
		spAtmosphere.m_pObject->SetSelectiveUpdate(1);
		NiNode* v5 = spRoot.m_pObject;
		v5->AttachChild(spAtmosphere.m_pObject, 1);
		model.~Model();
	}
	spFog.operator=(apFog);
	BSShaderManager::SetFogProperty(0, apFog);
	stream.~BSStream();
}

BSFogProperty* Atmosphere::GetFog() {
	return spFog;
}

void Atmosphere::SetUpdateFogDistance(bool a2) {
	bUpdateFogDistance = (bool)((uint8_t)a2);
}
