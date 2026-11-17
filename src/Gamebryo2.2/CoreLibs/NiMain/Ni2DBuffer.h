#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"

class NiDX92DBufferData;
class NiPixelFormat;

// A 2D render buffer (colour or depth); the renderer keeps its surface in RendererData.
class Ni2DBuffer : public NiObject {
public:
	enum CopyFilterPreference : uint32_t {
		COPY_FILTER_NONE = 0,
		COPY_FILTER_POINT = 1,
		COPY_FILTER_LINEAR = 2,
		COPY_FILTER_MAX = 3,
	};

	enum MultiSamplePreference : uint32_t {
		MULTISAMPLE_NONE = 0,
		MULTISAMPLE_2 = 1,
		MULTISAMPLE_4 = 2,
		MULTISAMPLE_8 = 3,
	};

	// The renderer's surface of the buffer: the buffer, its pixel format and multisample preference. The class has no
	// rtti and only the renderer's subclasses are created; its methods are not declared.
	class RendererData : public NiRefObject {
	public:
		~RendererData() override;

		// Every PC subclass leaves it returning false.
		virtual NiDX92DBufferData* IsNiXenon2DBufferData();

		const NiPixelFormat* GetPixelFormat() const { return m_pkPixelFormat; }

		Ni2DBuffer* m_pkBuffer;
		const NiPixelFormat* m_pkPixelFormat;
		MultiSamplePreference m_eMSAAPref;
	};

	Ni2DBuffer();
	~Ni2DBuffer() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	virtual bool FastCopy(Ni2DBuffer* pkDest, const NiRect<uint32_t>* pkSrcRect, uint32_t uiDestX, uint32_t uiDestY);
	virtual bool Copy(Ni2DBuffer* pkDest, const NiRect<uint32_t>* pkSrcRect, const NiRect<uint32_t>* pkDestRect,
		Ni2DBuffer::CopyFilterPreference ePref);
	virtual bool CreateRendererData(const NiPixelFormat& kFormat, Ni2DBuffer::MultiSamplePreference eMSAAPref);

	// The width, m_uiWidth.
	uint32_t GetWidth() const { return m_uiWidth; }
	// The height, m_uiHeight.
	uint32_t GetHeight() const { return m_uiHeight; }
	// The renderer data's pixel format; null without renderer data.
	const NiPixelFormat* GetPixelFormat() const;
	void ResetDimensions(uint32_t uiWidth, uint32_t uiHeight);
	// Hands the buffer its renderer data (a smart pointer assignment).
	void SetRendererData(RendererData* pkRendererData);

	// Creates a buffer of the given size whose renderer data is pkData.
	static Ni2DBuffer* Create(uint32_t uiWidth, uint32_t uiHeight, RendererData* pkData);
	static Ni2DBuffer* Create_(uint32_t uiWidth, uint32_t uiHeight);

	uint32_t m_uiWidth;
	uint32_t m_uiHeight;
	NiPointer<RendererData> m_spRendererData;

	static NiRTTI ms_RTTI;
};
