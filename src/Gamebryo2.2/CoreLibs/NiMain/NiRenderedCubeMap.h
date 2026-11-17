#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"

// A rendered texture with six face buffers; m_eFace selects the one drawn into.
class NiRenderedCubeMap : public NiRenderedTexture {
public:
	enum FaceID : uint32_t {
		FACE_POS_X = 0,
		FACE_NEG_X = 1,
		FACE_POS_Y = 2,
		FACE_NEG_Y = 3,
		FACE_POS_Z = 4,
		FACE_NEG_Z = 5,
		FACE_NUM = 6,
	};

	NiRenderedCubeMap();

	~NiRenderedCubeMap() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	uint32_t GetWidth() const override;
	uint32_t GetHeight() const override;

	// A cube map of six uiSize-square face buffers in the format kPrefs asks for, or null without a renderer or for a
	// size that is not a power of two.
	static NiRenderedCubeMap* Create(uint32_t uiSize, NiRenderer* apkRenderer, const NiTexture::FormatPrefs& kPrefs);

	FaceID m_eFace;
	NiPointer<Ni2DBuffer> m_aspFaceBuffers[6];

	static NiRTTI ms_RTTI;
};

NiSmartPointer(NiRenderedCubeMap);
