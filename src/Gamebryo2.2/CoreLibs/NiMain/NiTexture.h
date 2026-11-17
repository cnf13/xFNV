#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/BSDegradePriority.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

#include <stdint.h>

class NiColorA;

NiSmartPointer(NiTexture);

// An image used for texturing; the renderer keeps its own copy in RendererData. Textures form a global linked list.
class NiTexture : public NiObjectNET {
public:
	// How the texture should be stored when it is loaded.
	class FormatPrefs {
	public:
		enum PixelLayout : uint32_t {
			PALETTIZED_8 = 0,
			HIGH_COLOR_16 = 1,
			TRUE_COLOR_32 = 2,
			COMPRESSED = 3,
			BUMPMAP = 4,
			PALETTIZED_4 = 5,
			PIX_DEFAULT = 6,
			SINGLE_COLOR_8 = 7,
			SINGLE_COLOR_16 = 8,
			SINGLE_COLOR_32 = 9,
			DOUBLE_COLOR_32 = 10,
			DOUBLE_COLOR_64 = 11,
			FLOAT_COLOR_32 = 12,
			FLOAT_COLOR_64 = 13,
			FLOAT_COLOR_128 = 14,
		};

		enum AlphaFormat : uint32_t {
			NONE = 0,
			BINARY = 1,
			SMOOTH = 2,
			ALPHA_DEFAULT = 3,
		};

		enum MipFlag : uint32_t {
			NO = 0,
			YES = 1,
			MIP_DEFAULT = 2,
		};

		FormatPrefs();

		// Initialises the three fields, as used for the classes' ms_kDefaultPrefs.
		FormatPrefs(PixelLayout ePixelLayout, AlphaFormat eAlphaFmt, MipFlag eMipMapped)
			: m_ePixelLayout(ePixelLayout), m_eAlphaFmt(eAlphaFmt), m_eMipMapped(eMipMapped) {}

		PixelLayout m_ePixelLayout;
		AlphaFormat m_eAlphaFmt;
		MipFlag m_eMipMapped;
	};

	// The renderer's copy of a texture.
	class RendererData : public NiObject {
	public:
		// Why the renderer could not create a texture's data (the texture failure callback's argument).
		enum CreationFailure : int32_t {
			NONE = 0,
			GENERIC = INT32_MIN,
			NO2DMATCH = INT32_MIN + 1,
			NOCUBEMAPMATCH = INT32_MIN + 2,
			NOCONVERTER = INT32_MIN + 3,
			FAILEDCONVERT = INT32_MIN + 4,
			FILENOTFOUND = INT32_MIN + 5,
			BADDIMENSIONS = INT32_MIN + 6,
			CUBEMAPFAILEDCONVERT = INT32_MIN + 7,
			CUBEMAPFILENOTFOUND = INT32_MIN + 8,
			CUBEMAPBADDIMENSIONS = INT32_MIN + 9,
			CREATEFAIL_COUNT = 10,
		};

		using TEXTUREFAILUREFUNC = NiPixelData* (*)(NiTexture* pkTexture, CreationFailure eFailure);
		using ReplacementDataArray = NiPointer<NiPixelData>[10];

		// Base construction used by the derived constructors: the texture, a zero size, the priorities 0xFF and state
		// 0; cFlags is left alone.
		explicit RendererData(NiTexture* pkTexture)
			: m_pkTexture(pkTexture), m_uiWidth(0), m_uiHeight(0), DegradePriorityRange{0xFF, 0xFF, 0xFF, 0} {}

		~RendererData() override;

		virtual void Upgrade(NiTexture* apTexture) {}
		virtual void Degrade(NiTexture* apTexture) {}
		virtual uint32_t ComputeAliasSize();

		// Returns &DegradePriorityRange.
		BSDegradePriority* QDegradePriority();
		// Returns &m_kPixelFormat.
		const NiPixelFormat* GetPixelFormat();

		// Fills ms_kReplacementFormat with RGBA32 (the static data manager's initialisation).
		static void _SDMInit();
		// Drops the replacement textures (the static data manager's shutdown).
		static void _SDMShutdown();
		// The format textures are replaced with when their data is missing; setting it drops the replacement data
		// made in the old format.
		static void SetTextureReplacementFormat(const NiPixelFormat& kFormat);
		// The calling thread's iLoadMethod (a thread-local static).
		static int32_t& GetThreadLoadMethod();
		static NiPixelData* GetReplacementData(NiTexture* pkTexture, CreationFailure eFailure);
		static NiPixelData* MakeDataFromColors(const NiColorA& kTopLeftColor, const NiColorA& kBottomLeftColor,
			const NiPixelFormat& kDesc, uint32_t uiNumFaces);
		static NiPixelData* DefaultTextureFailureCallback(NiTexture* pkTexture,
			NiTexture::RendererData::CreationFailure eFailure);

		NiTexture* m_pkTexture;
		uint32_t m_uiWidth;
		uint32_t m_uiHeight;
		NiPixelFormat m_kPixelFormat;
		BSDegradePriority DegradePriorityRange;
		uint8_t cFlags;

		static TEXTUREFAILUREFUNC ms_pfnTextureFailureCallback;
		static ReplacementDataArray ms_aspReplacementData;
		static NiPixelFormat ms_kReplacementFormat;
	};

	// Default format preferences, no renderer data, appended to the global texture list.
	NiTexture();
	~NiTexture() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	virtual void Upgrade() {}
	virtual void Degrade() {}
	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;
	virtual const NiFixedString& GetFilename() const { return m_kName; }
	virtual uint32_t GetMemorySize();

	// Appends this texture to the global list, under its lock.
	void AddTextureToList();
	// Unlinks this texture from the global list, under its lock.
	void RemoveTextureFromList();
	// The degrade state of the renderer data's DegradePriorityRange, PS_NONE when the texture has no renderer data.
	PRIORITY_STATE QState();
	// The renderer data's pixel format, null without renderer data.
	const NiPixelFormat* GetPixelFormat();
	RendererData* GetRendererData();

	static void SetMipmapByDefault(bool bMipmap);
	static NiTexture* GetListHead();
	// Set and read the calling thread's RendererData::iLoadMethod.
	static void SetLoadMethod(uint32_t aiLoadMethod);
	static uint32_t QLoadMethod();

	FormatPrefs m_kFormatPrefs;
	RendererData* m_pkRendererData;
	NiTexture* m_pkPrev;
	NiTexture* m_pkNext;

	static NiRTTI ms_RTTI;
	// First texture of the global texture list (linked through m_pkNext).
	static inline NiTexture* ms_pkHead;
	static inline bool ms_bMipmapByDefault;
	// Last texture of the global texture list, and the list's lock.
	static inline NiTexture* ms_pkTail;
	static NiCriticalSection ms_kTextureListCriticalSection;
	static inline bool bLoadDirectToRenderer;
};
