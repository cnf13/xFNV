#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceManager.h"

#include <stdint.h>

class TLSData;

float randnormal(float mean, float stdev);

// The water FFT image-space effect and its texture state.
class ImageSpaceEffectWaterFFT : public ImageSpaceEffect {
public:
	using FrequencyTable = float[4];

	ImageSpaceEffectWaterFFT();
	~ImageSpaceEffectWaterFFT() override;

	void Render(NiScreenElements* apGeometry, NiDX9Renderer* apRenderer, ImageSpaceEffectParam* apParam,
		int32_t abEndFrame) override;
	void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
	bool IsActive() override { return true; }
	// Advances the FFT time from the shader timer.
	bool UpdateParams(ImageSpaceEffectParam* pParam) override;

	// Returns the rendered textures the effect holds and marks it unloaded.
	void FreeUpTextureMemory();
	// Borrows the utility textures the effect has not got yet, then builds the utility buffers.
	void InitializeUtilityBuffers();
	// The Phillips wave spectrum at wave vector (kx, ky), from the wind angle, amplitude and speed in fFrequency. Does
	// not use `this`.
	float Phillips(float kx, float ky);
	// Fills the scramble map with a bit-reversed ramp and the IFFT butterfly map with the twiddle factors, into the
	// locked D3D textures of the borrowed rendered textures. Returns the thread's TLS block.
	TLSData* FillFFTUtilityTextures(int32_t iResolution, int32_t iLog2Resolution);
	// Fills the high-amplitude texture; the first time through also fills the high-W texture and clears the flag.
	void FillWaterUtilityTextures(bool* abFirstTimeThrough);
	void FillWaterUtilityBuffers();

protected:
	// Allocates the utility buffers, fills them and marks the effect loaded.
	void Do_InitializeUtilityBuffers();

public:
	static ImageSpaceEffectWaterFFT* GetInstance();

	ImageSpaceTexture pHighWTex;
	ImageSpaceTexture pHighAmplitude0Tex;
	ImageSpaceTexture pIFFTButterflyMap;
	ImageSpaceTexture pScrambleMap;
	ImageSpaceTexture pFourierSpectrum;
	float** HighW;
	float** HighAmplitude0;
	float fTime;
	float fPassNumber;
	bool bLoaded;
	uint8_t paddingB9[3];

	// The width and height of the FFT grid textures.
	static inline int32_t iGridResolution;
	static inline ImageSpaceEffectWaterFFT* pInstance;
	static inline FrequencyTable fFrequency{};
	static inline float fInvResolution;
	static inline int32_t iLog2GridResolution;
	static inline bool bNormals;
};

inline bool WaterHighResolution;

// randnormal's table of the standard normal distribution: pairs of
// x and its cumulative probability, x rising from 0 to 99.
using NormalDistributionTable = float[53][2];
inline NormalDistributionTable kNormalDistributionTable{{0.0f, 0.5f}, {0.01f, 0.504f}, {0.02f, 0.508f}, {0.03f, 0.512f},
	{0.06f, 0.5239f}, {0.08f, 0.5319f}, {0.11f, 0.5438f}, {0.13f, 0.5517f}, {0.16f, 0.5636f}, {0.18f, 0.5714f},
	{0.21f, 0.5832f}, {0.23f, 0.591f}, {0.26f, 0.6026f}, {0.28f, 0.6103f}, {0.31f, 0.6217f}, {0.34f, 0.6331f},
	{0.36f, 0.6406f}, {0.39f, 0.6517f}, {0.42f, 0.6628f}, {0.44f, 0.67f}, {0.47f, 0.6808f}, {0.5f, 0.6915f},
	{0.53f, 0.7019f}, {0.56f, 0.7123f}, {0.59f, 0.7224f}, {0.62f, 0.7324f}, {0.65f, 0.7422f}, {0.68f, 0.7517f},
	{0.71f, 0.7611f}, {0.74f, 0.7703f}, {0.78f, 0.7823f}, {0.81f, 0.791f}, {0.85f, 0.8023f}, {0.88f, 0.8106f},
	{0.92f, 0.8212f}, {0.96f, 0.8315f}, {1.0f, 0.8413f}, {1.04f, 0.8508f}, {1.09f, 0.8621f}, {1.13f, 0.8708f},
	{1.18f, 0.881f}, {1.23f, 0.8907f}, {1.29f, 0.9015f}, {1.35f, 0.9115f}, {1.41f, 0.9207f}, {1.48f, 0.9306f},
	{1.56f, 0.9406f}, {1.65f, 0.9505f}, {1.76f, 0.9608f}, {1.89f, 0.9706f}, {2.06f, 0.9803f}, {2.33f, 0.9901f},
	{99.0f, 1.0f}};
