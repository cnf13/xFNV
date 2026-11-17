#pragma once

#include "BSCore/BSStringT.h"
#include "Fallout Shared/Sky/SkyObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Reconstruction.hpp"

class Sky;

// A moon (Masser or Secunda): its mesh and shadow mesh, the phase textures and the orbit settings.
class Moon : public SkyObject {
public:
	// When the moon reloads its phase texture.
	enum UpdateStatus : int32_t {
		US_NOT_REQUIRED = 0,
		US_WHEN_CULLED = 1,
		US_INITIALIZE = 2,
	};

	Moon(const char* apMoonName, float afAngleFadeStart, float afAngleFadeEnd, float afAngleShadowEarlyFade,
		float afSpeed, float afZOffset, uint32_t aiSize);
	~Moon() override;

	// Moves the moon along its orbit and fades it.
	void Update(Sky* apSky, float afSecondsPassed) override;
	// Builds the moon and shadow meshes under apSkyRoot with the textures named after apName.
	virtual void Initialize(NiNode* apSkyRoot, const char* apName);
	using SkyObject::Initialize;

	// Clears the moon's and its shadow's sky shader textures.
	void RemoveTextures();
	float GetMoonVisibility() const;
	float GetMoonShadowVisibility() const;

	// Advances the shared moon phase from the days passed; true when the phase changed.
	static bool UpdatePhase(const Sky* apSky);

	NiPointer<NiNode> spMoonNode;
	NiPointer<NiNode> spShadowNode;
	NiPointer<NiTriShape> spMoonMesh;
	NiPointer<NiTriShape> spShadowMesh;
	BSStringT<char> strTextures[8];
	float fAngleFadeStart;
	float fAngleFadeEnd;
	float fAngleShadowEarlyFade;
	float fSpeed;
	float fZOffset;
	uint32_t iSize;
	UpdateStatus eUpdateMoonTexture;
	float fPos;
	float fLastPosUpdate;

	static inline int32_t eCurrentPhase;
};
inline constexpr fnvd::Vtable Moon_vftable{};
