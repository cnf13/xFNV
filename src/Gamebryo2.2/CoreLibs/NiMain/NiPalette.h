#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Reconstruction.hpp"

// A colour table for palettized pixel data. Palettes form a global linked list.
class NiPalette : public NiObject {
public:
	class PaletteEntry {
	public:
		uint8_t m_ucRed;
		uint8_t m_ucGreen;
		uint8_t m_ucBlue;
		uint8_t m_ucAlpha;
	};

	// The renderer's copy of the palette: the palette it was built for. A polymorphic object of its own (PDB:
	// virtual ~RendererData, m_pkPalette at +0x4; IDA: NiPalette::RendererData : NiMemObject). The exe deletes it
	// through its vtable's slot 0 (NiPalette's deleting destructor 0xA8B270 reads [m_pkRendererData] and calls the
	// entry with flag 1).
	class RendererData : public NiMemObject {
	public:
		virtual ~RendererData();

		// The PDB has it protected; public so the offset can be pinned, as NiTexture::RendererData's members are.
		NiPalette* m_pkPalette;
	};

	NiPalette();
	NiPalette(const NiPalette& kPal);
	NiPalette(bool bHasAlpha, uint32_t uiEntries, const PaletteEntry* pkEntries);
	~NiPalette() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

	// Appends this palette to the global list, under its lock.
	void AddPaletteToList();
	// Unlinks this palette from the global list, under its lock.
	void RemovePaletteFromList();
	// Makes this palette a copy of kPal.
	NiPalette& operator=(const NiPalette& kPal);

	static NiObject* CreateObject();

	bool m_bAlpha;
	uint32_t m_uiEntries;
	uint32_t m_uiRevID;
	PaletteEntry* m_pkPalette;
	RendererData* m_pkRendererData;
	NiPalette* m_pkPrev;
	NiPalette* m_pkNext;

	static NiRTTI ms_RTTI;
	// Ends of the global palette list (linked through m_pkPrev / m_pkNext) and its lock.
	static inline NiPalette* ms_pkHead;
	static inline NiPalette* ms_pkTail;
	static NiCriticalSection ms_kPaletteListCriticalSection;
};
inline constexpr fnvd::Vtable NiPalette_vftable{};
