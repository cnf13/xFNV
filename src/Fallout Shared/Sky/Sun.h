#pragma once

#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/Sky/SkyObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class BSShaderAccumulator;
class GameSettingCollection;
class NiAVObject;
class NiBillboardNode;
class NiCamera;
class NiDirectionalLight;
class NiNode;
class NiPick;
class NiTriShape;
class Sky;

// The sun: the base and glare geometry and the light it drives.
class Sun : public SkyObject {
public:
	// The constructor.
	Sun();
	~Sun() override;

	void Initialize(NiNode* apSkyRoot) override;
	void Update(Sky* apSky, float afSecondsPassed) override;

	// The sun base object; the engine keeps a smart pointer, the raw pointer at offset 0 is the same value.
	NiAVObject* GetBase();

	NiDirectionalLight* GetSunLight();
	NiBillboardNode* GetSunGlareNode();
	// The sun base node (thiscall, ret 0, 0x5585E0; the exe's shared +8 NiPointer getter).
	NiBillboardNode* GetSunBaseNode();
	// The glare shape (spSunGlare; thiscall, ret 0, the exe's shared +0x14 NiPointer getter 0x63D040). The PDB's
	// GetSunGlare is another member, a float.
	NiTriShape* GetSunGlareGeo() const;
	// The sun's intensity, 0 without a sky shader property (thiscall, ST0 float, 0x6428B0).
	float GetIntensity();

	// Drops the texture of the sun base's and glare's sky shader properties.
	void RemoveTextures();
	void StartSunOcclusion();
	void StopSunOcclusion();
	void CalculateSunGlare(uint32_t aiTestNumber, BSShaderAccumulator* apAccumulator, NiCamera* apCamera);
	float GetSunOcclusionPercent();

	NiPointer<NiBillboardNode> spSunBaseNode;            // 0x08
	NiPointer<NiBillboardNode> spSunGlareNode;           // 0x0C
	NiPointer<NiTriShape> spSunBase;                     // 0x10
	NiPointer<NiTriShape> spSunGlare;                    // 0x14
	NiPick* pSunGlarePick;                               // 0x18
	NiPointer<NiDirectionalLight> spLight;               // 0x1C
	float fGlareScale;                                   // 0x20
	bool bDoOcclusionTests;                              // 0x24
	NiPointer<BSShaderAccumulator> spSunAccumulator;     // 0x28

	// The half-sizes of the sun base and glare quads.
	static SettingT<INISettingCollection> fSunBaseSize;
	static SettingT<INISettingCollection> fSunGlareSize;
	static SettingT<GameSettingCollection> fSunXExtreme;
	static SettingT<GameSettingCollection> fSunAlphaTransTime;
	static SettingT<GameSettingCollection> fSunMinimumGlareScale;
	static SettingT<GameSettingCollection> fSunReduceGlareSpeed;
	static SettingT<GameSettingCollection> fSunYExtreme;
};
inline constexpr fnvd::Vtable Sun_vftable{};

// GameSettingCollection: 0xf59142 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fSunZExtreme;
