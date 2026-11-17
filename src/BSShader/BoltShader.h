#pragma once

#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"

#include <stdint.h>

class BoltShaderProperty;
class NiD3DPixelShader;
class NiD3DVertexShader;
class NiDX9ShaderDeclaration;
class NiTriShape;

void au_re_GameHeapFree_36();

class BoltShader : public BSShader {
public:
	class SegmentData;

	BoltShader(NiDX9ShaderDeclaration* apShaderDecl);
	~BoltShader() override;

	const NiRTTI* GetRTTI() const override;
	bool Initialize() override;
	void RecreateRendererData() override;
	void CreateConstantMaps() override;
	void ReloadShaders() override;
	void ClearPassStages() override;
	void SetCurrentPass(uint32_t aPass, bool aFirstPass) override;

protected:
	virtual void LoadShaders();
	// Creates the bolt pass when there is none, then sets up its stages; always true.
	virtual bool LoadStagesAndPasses();

public:
	void BC1210();

	// Releases the shared bolt geometry.
	static void Free();
	// The segment count a bolt is built with, chosen on first use from the shader level.
	static uint32_t GetMaxSegments();
	// Sets the sub-segment count bolts are built with (defined in MagicBoltProjectile.cpp).
	static void SetSubSegmentCount(int32_t aiSubSegments);
	// Builds the shared bolt geometry on first use and returns it.
	static NiGeometry* GetBoltGeometry();
	// The per-vertex segment index data of the bolt geometry.
	static float* GetSegmentIndices();
	// Attaches a clone of the bolt geometry with its shader property to apNode. abStartFromPoint is not read by this
	// body.
	static BoltShaderProperty* Start(NiNode* apNode, bool abStartFromPoint);
	static BSShader* CreateShader();
	static void Stop(NiNode* apNode, BoltShaderProperty* apProp);

	BSShaderManager::eBSSMPassEnum ePass;
	// The bolt geometry's segment array, the source of AddEntry's "bolt data" entry (uiSegments * 0x10 bytes).
	SegmentData* pSegmentData;
	float fTimer;
	float fTortuosity[40];
	NiPoint4 EyePosition;
	NiPoint4 fVars[4];
	NiPointer<NiD3DPass> spBoltPass;
	NiColorA CoreColor;
	NiColorA GlowColor;
	NiPointer<NiD3DVertexShader> spBoltVertexShader;
	NiPointer<NiD3DPixelShader> spBoltPixelShader;

	static NiRTTI ms_RTTI;
	static NiPointer<NiTriShape> spBaseGeometry;
	static inline uint32_t iMaxSegments;
	static inline int32_t iSubSegments = 1;
	static inline float* pSegmentIndex;
	static inline int32_t iLoadedCount;
};

// One bolt segment's position, uploaded as one vertex shader constant (16 bytes per segment).
class BoltShader::SegmentData {
public:
	float fXPos;
	float fYPos;
	float fZPos;
	float fUnused;
};
