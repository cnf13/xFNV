#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/Sky/SkyObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiGeometry;
class NiTexture;
class Sky;

// The sky cloud layers: one cloud texture and one colour per layer, plus a transition set.
class Clouds : public SkyObject {
public:
	static const int32_t MAX_LAYERS = 4;

	// The constructor.
	Clouds();
	~Clouds() override;

	void Initialize(NiNode* apSkyRoot) override;
	void Update(Sky* apSky, float afSecondsPassed) override;

	int32_t GetNumLayers() const;
	void SetNumLayers(int32_t aiNumLayers);

	NiColor& GetColor(int32_t aiLayer);
	void SetColor(int32_t aiLayer, NiColor& aColor);

	// The geometry of layer aiLayer.
	NiGeometry* GetCloudsGeo(int32_t aiLayer);

	void ZeroGeo();
	// Clears the transition texture of every layer and its sky shader's blend texture and value.
	void ClearTransTextures();
	// Clears the texture and texture file of every layer's sky shader.
	void RemoveTextures();

	// Makes the next update push the transition textures again.
	void ForceTransTextureUpdate();

	// The cloud geometry of each layer; every assignment goes through NiPointer's operator=.
	NiPointer<NiGeometry> spClouds[MAX_LAYERS];       // 0x08

	// The texture each layer blends toward during a weather transition (PDB: NiPointer<NiTexture>; the exe hands
	// its address to SkyTextureManager::LoadTextureInBackground 0x63FB70, which takes a NiPointer<NiTexture>*).
	NiPointer<NiTexture> spTransTexture[MAX_LAYERS];

	// The colour stored per layer (stride sizeof(NiColor)).
	NiColor pColors[MAX_LAYERS];           // 0x28

	uint16_t usNumLayers;                  // 0x58

	// 0x5A: the colour is stale and has to be pushed to the layer again.
	bool bForceUpdate;                     // 0x5A
};
inline constexpr fnvd::Vtable Clouds_vftable{};

// The game settings Clouds.cpp defines (PDB), at their exe addresses.
extern SettingT<GameSettingCollection> fWeatherCloudSpeedMax;
