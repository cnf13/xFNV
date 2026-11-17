#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBBlock.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"

void NiVBBlock::ClearAllData() {
	m_pkSet = nullptr;
	m_pkVB = nullptr;
	m_uiSize = 0;
	m_uiFVF = 0;
	m_uiFlags = 0;
	m_uiIndex = 0;
	m_uiUsage = 0;
	m_ePool = 0;
	m_uiCurrOffset = 0;
	m_uiFreeBytes = 0;
	m_uiTotalFreeBytes = 0;
	for (uint16_t i = 0; i < m_apkChips.m_usSize; ++i) {
		m_apkChips.m_pBase[i] = nullptr;
	}
	m_apkChips.m_usSize = 0;
	m_apkChips.m_usESize = 0;
	m_pkNextBlock = nullptr;
	m_pkPrevBlock = nullptr;
}

// Takes a chip from the chip dump (refilling it when empty) and carves uiSize bytes for it off the block's end.
NiVBChip* NiVBBlock::AllocateChip(uint32_t uiSize) {
	if (GetFreeBytes() < uiSize) {
		return nullptr;
	}
	NiVBChip* pkChip = NiVBBlock::ms_pkChipDump;
	if (!NiVBBlock::ms_pkChipDump) {
		NiVBBlock::AllocateChips();
		pkChip = NiVBBlock::ms_pkChipDump;
	}
	NiVBChip* pkNext = pkChip->GetNext();
	NiVBBlock::ms_pkChipDump = pkNext;
	if (pkNext) {
		pkNext->m_pkPrevChip = nullptr;
	}
	pkChip->m_pkNextChip = nullptr;
	pkChip->m_pkPrevChip = nullptr;
	pkChip->m_pkBlock = this;
	pkChip->m_pkVB = GetVB();
	pkChip->m_uiOffset = GetCurrOffset();
	pkChip->m_uiSize = uiSize;
	m_uiCurrOffset += uiSize;
	m_uiFreeBytes -= uiSize;
	m_uiTotalFreeBytes -= uiSize;
	pkChip->m_uiIndex = m_apkChips.AddFirstEmpty(pkChip);
	return pkChip;
}

uint32_t NiVBBlock::FreeChip(uint32_t uiChipIndex) {
	NiVBChip* pkNullChip;
	NiVBChip** m_pBase = m_apkChips.m_pBase;
	uint32_t v3 = uiChipIndex;
	NiVBChip* pkVBChip = m_pBase[uiChipIndex];
	uint32_t result = 0;
	if (pkVBChip) {
		uint32_t m_uiSize = pkVBChip->GetSize();
		uint32_t v7 = this->GetCurrOffset() - m_uiSize;
		uiChipIndex = m_uiSize;
		if (v7 == pkVBChip->GetOffset()) {
			m_uiCurrOffset = 0;
			if (v3) {
				NiVBChip** v8 = m_pBase;
				uint32_t v9 = v3;
				do {
					if (*v8) {
						uint32_t uiOffset = (*v8)->GetOffset() + (*v8)->GetSize();
						if (uiOffset > this->GetCurrOffset()) {
							m_uiCurrOffset = uiOffset;
						}
					}
					++v8;
					--v9;
				} while (v9);
				m_uiSize = uiChipIndex;
			}
		}
		uint32_t v11 = this->GetSize() - this->GetCurrOffset();
		m_uiTotalFreeBytes += m_uiSize;
		m_uiFreeBytes = v11;
		pkNullChip = nullptr;
		m_apkChips.SetAt(v3, pkNullChip);
		pkVBChip->m_pkPrevChip = 0;
		pkVBChip->m_pkNextChip = NiVBBlock::ms_pkChipDump;
		if (NiVBBlock::ms_pkChipDump) {
			NiVBBlock::ms_pkChipDump->m_pkPrevChip = pkVBChip;
		}
		NiVBBlock::ms_pkChipDump = pkVBChip;
		return m_uiSize;
	}
	return result;
}

void NiVBBlock::DestroyChipArray() {
	FNVD_DESTRUCT_AS(DestructInlined, NiTArray<NiVBChip*, NiTMallocInterface<NiVBChip*>>)(&m_apkChips);
}

// Each chip's vertex buffer is cleared before the chip is freed.
void NiVBBlock::DeleteChips() {
	NiVBChip* pkChip = ms_pkChipDump;
	while (pkChip) {
		NiVBChip* pkNext = pkChip->m_pkNextChip;
		pkChip->m_pkVB = nullptr;
		NiMemObject::operator delete(pkChip, sizeof(NiVBChip));
		pkChip = pkNext;
	}
	ms_pkChipDump = nullptr;
}

void NiVBBlock::AllocateChips() {
	NiVBChip* v1;
	for (uint32_t i = 0; i < NiVBBlock::ms_uiDefClipAllocSize; NiVBBlock::ms_pkChipDump = v1) {
		v1 = static_cast<NiVBChip*>(NiMemObject::operator new(sizeof(NiVBChip)));
		if (v1) {
			v1->m_uiIndex = 0;
			v1->m_pkBlock = 0;
			v1->m_pkVB = 0;
			v1->m_uiOffset = 0;
			v1->m_uiSize = 0;
			v1->m_uiLockFlags = 0;
			v1->m_pkNextChip = 0;
			v1->m_pkPrevChip = 0;
		} else {
			v1 = 0;
		}
		NiVBChip* v2 = NiVBBlock::ms_pkChipDump;
		if (NiVBBlock::ms_pkChipDump) {
			NiVBBlock::ms_pkChipDump->m_pkPrevChip = v1;
			v2 = NiVBBlock::ms_pkChipDump;
		}
		v1->m_pkNextChip = v2;
		v1->m_pkPrevChip = 0;
		++i;
	}
}
