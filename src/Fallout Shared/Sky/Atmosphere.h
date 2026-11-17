#pragma once

#include "Fallout Shared/Sky/SkyObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class BSFogProperty;
class NiNode;
class NiTriShape;
class Sky;

// The sky's atmosphere: the atmosphere geometry and the fog property the weather drives.
class Atmosphere : public SkyObject {
public:
	// The constructor.
	Atmosphere();
	~Atmosphere() override;

	void Update(Sky* apSky, float afSecondsPassed) override;

	virtual void Initialize(NiNode* apSkyRoot, BSFogProperty* apFog);

	using SkyObject::Initialize;

	BSFogProperty* GetFog();
	NiTriShape* GetAthmosphereGeo() { return spAtmosphere; }
	void SetUpdateFogDistance(bool a2);

	// The atmosphere geometry; the exe reads it through NiPointer's operator-> (0x5585E0 from 0x63BCE0).
	NiPointer<NiTriShape> spAtmosphere;
	// The fog property.
	NiPointer<BSFogProperty> spFog;
	// The non-HDR sky quad and the node it hangs from (Initialize 0x633260; SkyShader::CreateSkyQuad builds the quad).
	NiPointer<NiNode> spSkyQuadNode;
	NiPointer<NiTriShape> spSkyQuad;
	bool bUpdateFogDistance;
	uint8_t padding19[3];
};
inline constexpr fnvd::Vtable Atmosphere_vftable{};
