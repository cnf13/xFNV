#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Reconstruction.hpp"

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
	// rtti and only the renderer's subclasses are created; its methods are not reconstructed yet.
	class RendererData : public NiRefObject {
	public:
		~RendererData() override;

		// Slot 2; the Xbox PDB's name. Every PC subclass leaves it returning false (0xE7CD40).
		virtual NiDX92DBufferData* IsNiXenon2DBufferData();

		// m_pkPixelFormat. Same function as TESEnchantableForm::GetCastingType (0x84E3A0), registered there.
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

	// The width, m_uiWidth. Same function as TESForm::GetFormFlags (0x44DDC0), registered there.
	uint32_t GetWidth() const { return m_uiWidth; }
	// The height, m_uiHeight. Same function as TESEnchantableForm::GetCastingType (0x84E3A0), registered there.
	uint32_t GetHeight() const { return m_uiHeight; }
	// The renderer data's pixel format; null without renderer data.
	const NiPixelFormat* GetPixelFormat() const;
	void ResetDimensions(uint32_t uiWidth, uint32_t uiHeight);
	// Hands the buffer its renderer data (the smart pointer assignment, inlined in the exe).
	void SetRendererData(RendererData* pkRendererData);

	// cdecl, 0xA8F2C0. Creates a buffer of the given size whose renderer data is pkData.
	static Ni2DBuffer* Create(uint32_t uiWidth, uint32_t uiHeight, RendererData* pkData);
	static Ni2DBuffer* Create_(uint32_t uiWidth, uint32_t uiHeight);

	uint32_t m_uiWidth;
	uint32_t m_uiHeight;
	NiPointer<RendererData> m_spRendererData;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable Ni2DBuffer_vftable{};
