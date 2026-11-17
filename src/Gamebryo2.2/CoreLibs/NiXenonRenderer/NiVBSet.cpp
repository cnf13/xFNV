#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBSet.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBBlock.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"

// The block size and free-space threshold come from the exe's defaults.
NiVBSet::NiVBSet()
	: m_uiIndex(0), m_uiFVF(0), m_uiFlags(0), m_uiBlockSize(ms_uiDefBlockSize), m_pkD3DDevice(nullptr), m_uiUsage(0),
	  m_ePool(0), m_apkBlocks(0, 1), m_pkBlockList(nullptr), m_uiFreeSpaceThreshold(ms_uiDefFreeSpaceThreshold) {
	m_apkBlocks.RemoveAll();
}

void NiVBSet::RemoveBlockFromFreeSpaceList(NiVBBlock* pkVBBlock) {
	NiVBBlock* m_pkBlockList;
	NiVBBlock* m_pkNextBlock;
	NiVBBlock* m_pkPrevBlock;
	m_pkBlockList = this->m_pkBlockList;
	if (m_pkBlockList) {
		while (m_pkBlockList != pkVBBlock) {
			m_pkBlockList = m_pkBlockList->m_pkNextBlock;
			if (!m_pkBlockList) {
				return;
			}
		}
		m_pkNextBlock = m_pkBlockList->m_pkNextBlock;
		if (m_pkNextBlock) {
			m_pkNextBlock->m_pkPrevBlock = m_pkBlockList->m_pkPrevBlock;
		}
		m_pkPrevBlock = m_pkBlockList->m_pkPrevBlock;
		if (m_pkPrevBlock) {
			m_pkPrevBlock->m_pkNextBlock = m_pkBlockList->m_pkNextBlock;
		}
		if (m_pkBlockList == this->m_pkBlockList) {
			this->m_pkBlockList = m_pkBlockList->m_pkNextBlock;
		}
	}
}

void NiVBSet::InsertBlockIntoFreeSpaceList(NiVBBlock* pkVBBlock) {
	NiVBBlock* m_pkBlockList;
	NiVBBlock* v3;
	m_pkBlockList = this->m_pkBlockList;
	if (m_pkBlockList) {
		v3 = nullptr;
		while (m_pkBlockList->m_uiFreeBytes <= pkVBBlock->m_uiFreeBytes) {
			v3 = m_pkBlockList;
			m_pkBlockList = m_pkBlockList->m_pkNextBlock;
			if (!m_pkBlockList) {
				v3->m_pkNextBlock = pkVBBlock;
				pkVBBlock->m_pkPrevBlock = v3;
				pkVBBlock->m_pkNextBlock = nullptr;
				return;
			}
		}
		if (v3) {
			v3->m_pkNextBlock = pkVBBlock;
		}
		m_pkBlockList->m_pkPrevBlock = pkVBBlock;
		pkVBBlock->m_pkNextBlock = m_pkBlockList;
		pkVBBlock->m_pkPrevBlock = v3;
		if (m_pkBlockList == this->m_pkBlockList) {
			this->m_pkBlockList = pkVBBlock;
		}
	} else {
		this->m_pkBlockList = pkVBBlock;
		pkVBBlock->m_pkNextBlock = nullptr;
		this->m_pkBlockList->m_pkPrevBlock = nullptr;
	}
}

// Releases the block's vertex buffer, clears it, unlinks it and puts it on the dump list. The exe ignores this.
void NiVBSet::FreeBlock(NiVBBlock* pkBlock) {
	if (pkBlock) {
		if (pkBlock->m_pkVB) {
			pkBlock->m_pkVB->Release();
			pkBlock->m_pkVB = nullptr;
		}
		pkBlock->ClearAllData();
		NiVBBlock* pkNext = pkBlock->m_pkNextBlock;
		NiVBBlock* pkPrev = pkBlock->m_pkPrevBlock;
		if (pkNext) {
			pkNext->m_pkPrevBlock = pkPrev;
		}
		if (pkPrev) {
			pkPrev->m_pkNextBlock = pkNext;
		}
		if (ms_pkBlockDump) {
			ms_pkBlockDump->m_pkPrevBlock = pkBlock;
		}
		pkBlock->m_pkNextBlock = ms_pkBlockDump;
		pkBlock->m_pkPrevBlock = nullptr;
		ms_pkBlockDump = pkBlock;
	}
}

// The PDB does not name flag 0x10000000: a set with it makes each block exactly as large as asked.
NiVBBlock* NiVBSet::CreateBlock(uint32_t uiSize) {
	uint32_t uiVBSize = uiSize;
	if ((GetFlags() & 0x10000000) == 0 && uiSize <= GetBlockSize()) {
		uiVBSize = GetBlockSize();
	}
	IDirect3DVertexBuffer9* pkVB = nullptr;
	if (m_pkD3DDevice->CreateVertexBuffer(uiVBSize, GetUsage(), GetFVF(), GetPool(), &pkVB, nullptr) < 0) {
		return nullptr;
	}
	NiVBBlock* pkVBBlock = NiVBSet::ms_pkBlockDump;
	if (!NiVBSet::ms_pkBlockDump) {
		NiVBSet::AllocateBlocks();
		pkVBBlock = NiVBSet::ms_pkBlockDump;
	}
	NiVBBlock* pkNext = pkVBBlock->m_pkNextBlock;
	NiVBSet::ms_pkBlockDump = pkNext;
	if (pkNext) {
		pkNext->m_pkPrevBlock = nullptr;
	}
	pkVBBlock->m_pkNextBlock = nullptr;
	pkVBBlock->m_pkPrevBlock = nullptr;
	pkVBBlock->m_pkSet = this;
	pkVBBlock->m_pkVB = pkVB;
	pkVBBlock->m_uiSize = uiVBSize;
	pkVBBlock->m_uiFreeBytes = uiVBSize;
	pkVBBlock->m_uiTotalFreeBytes = uiVBSize;
	pkVBBlock->m_uiFVF = GetFVF();
	pkVBBlock->m_uiFlags = GetFlags();
	pkVBBlock->m_ePool = GetPool();
	pkVBBlock->m_uiUsage = GetUsage();
	pkVBBlock->m_uiIndex = m_apkBlocks.AddFirstEmpty(pkVBBlock);
	return pkVBBlock;
}

NiVBChip* NiVBSet::AllocateChip(uint32_t uiSize, bool bForceNewBlock) {
	NiVBBlock* pkBlock = nullptr;
	if (!bForceNewBlock) {
		pkBlock = FindBlockWithFreeSpace(uiSize);
	}
	if (!pkBlock) {
		pkBlock = CreateBlock(uiSize);
		if (!pkBlock) {
			return nullptr;
		}
	}
	NiVBChip* pkChip = pkBlock->AllocateChip(uiSize);
	if (pkBlock->m_uiFreeBytes > m_uiFreeSpaceThreshold) {
		InsertBlockIntoFreeSpaceList(pkBlock);
	}
	return pkChip;
}

void NiVBSet::FreeChip(uint32_t uiBlockIndex, uint32_t uiChipIndex) {
	NiVBBlock* pkBlock = m_apkBlocks.m_pBase[uiBlockIndex];
	if (!pkBlock) {
		return;
	}
	uint32_t uiPreFreeSpace = pkBlock->m_uiFreeBytes;
	pkBlock->FreeChip(uiChipIndex);
	if (uiPreFreeSpace != pkBlock->m_uiFreeBytes) {
		RemoveBlockFromFreeSpaceList(pkBlock);
		if (pkBlock->m_uiTotalFreeBytes == pkBlock->m_uiSize) {
			m_apkBlocks.RemoveAt(uiBlockIndex);
			FreeBlock(pkBlock);
		} else {
			InsertBlockIntoFreeSpaceList(pkBlock);
		}
	}
}

void NiVBSet::E943C0() {
	for (uint32_t i = 0; i < m_apkBlocks.m_usSize; ++i) {
		if (i < m_apkBlocks.m_usSize) {
			NiVBBlock** m_pBase = m_apkBlocks.m_pBase;
			NiVBBlock* v4 = m_pBase[i];
			m_pBase[i] = 0;
			if (v4) {
				--m_apkBlocks.m_usESize;
			}
			uint16_t v5 = m_apkBlocks.m_usSize - 1;
			if (i == v5) {
				m_apkBlocks.m_usSize = v5;
			}
			if (v4) {
				IDirect3DVertexBuffer9* m_pkVB = v4->GetVB();
				if (m_pkVB) {
					v4->GetVB()->Release();
					v4->m_pkVB = 0;
				}
				v4->ClearAllData();
				NiVBBlock* m_pkNextBlock = v4->m_pkNextBlock;
				NiVBBlock* m_pkPrevBlock = v4->m_pkPrevBlock;
				if (m_pkNextBlock) {
					m_pkNextBlock->m_pkPrevBlock = m_pkPrevBlock;
				}
				if (m_pkPrevBlock) {
					m_pkPrevBlock->m_pkNextBlock = m_pkNextBlock;
				}
				NiVBBlock* v9 = NiVBSet::ms_pkBlockDump;
				if (NiVBSet::ms_pkBlockDump) {
					NiVBSet::ms_pkBlockDump->m_pkPrevBlock = v4;
					v9 = NiVBSet::ms_pkBlockDump;
				}
				v4->m_pkNextBlock = v9;
				v4->m_pkPrevBlock = 0;
				NiVBSet::ms_pkBlockDump = v4;
			}
		}
	}
	IDirect3DDevice9* m_pkD3DDevice = this->m_pkD3DDevice;
	if (m_pkD3DDevice) {
		this->m_pkD3DDevice->Release();
	}
	this->m_pkD3DDevice = 0;
	// The block array's destructor, inlined.
	FNVD_DESTRUCT_AS(DestructInlined, NiTArray<NiVBBlock*, NiTMallocInterface<NiVBBlock*>>)(&m_apkBlocks);
}

NiVBBlock* NiVBSet::FindBlockWithFreeSpace(uint32_t uiFreeSize) {
	NiVBBlock* m_pkNextBlock;
	NiVBBlock* m_pkPrevBlock;
	NiVBBlock* result = m_pkBlockList;
	if (!result) {
		return nullptr;
	}
	while (result->m_uiFreeBytes < uiFreeSize) {
		result = result->m_pkNextBlock;
		if (!result) {
			return nullptr;
		}
	}
	m_pkNextBlock = result->m_pkNextBlock;
	if (m_pkNextBlock) {
		m_pkNextBlock->m_pkPrevBlock = result->m_pkPrevBlock;
	}
	m_pkPrevBlock = result->m_pkPrevBlock;
	if (m_pkPrevBlock) {
		m_pkPrevBlock->m_pkNextBlock = result->m_pkNextBlock;
	}
	if (result == m_pkBlockList) {
		m_pkBlockList = result->m_pkNextBlock;
	}
	return result;
}

void NiVBSet::SetPoolAndUsage(NiVBSet* apSet) {
	apSet->m_uiUsage = 8;
	if (apSet->m_uiFlags & 0x80000000) {
		apSet->m_uiUsage = 0x18;
	}
	// The exe tests the whole byte against zero (cmp; sete), so the flag is read as a byte, not a 0/1 bool.
	apSet->m_ePool = *reinterpret_cast<const uint8_t*>(&NiDX9Renderer::bUseDirect3D9Ex) == 0;
}

void NiVBSet::DeleteBlocks() {
	NiVBBlock::DeleteChips();
	NiVBBlock* v0 = NiVBSet::ms_pkBlockDump;
	if (NiVBSet::ms_pkBlockDump) {
		NiVBBlock* m_pkNextBlock;
		do {
			m_pkNextBlock = v0->m_pkNextBlock;
			v0->DestroyChipArray();
			NiMemObject::operator delete(v0, 68u);
			v0 = m_pkNextBlock;
		} while (m_pkNextBlock);
	}
	NiVBSet::ms_pkBlockDump = 0;
}

void NiVBSet::AllocateBlocks() {
	NiVBBlock* v1;
	for (uint32_t ui = 0; ui < NiVBSet::ms_uiDefBlockAllocSize; NiVBSet::ms_pkBlockDump = v1) {
		v1 = static_cast<NiVBBlock*>(NiMemObject::operator new(sizeof(NiVBBlock)));
		if (v1) {
			::new (&v1->m_apkChips) NiTPrimitiveArray<NiVBChip*>(0, 1);
			v1->ClearAllData();
		} else {
			v1 = 0;
		}
		NiVBBlock* m_pkNextBlock = v1->m_pkNextBlock;
		NiVBBlock* m_pkPrevBlock = v1->m_pkPrevBlock;
		if (m_pkNextBlock) {
			m_pkNextBlock->m_pkPrevBlock = m_pkPrevBlock;
		}
		if (m_pkPrevBlock) {
			m_pkPrevBlock->m_pkNextBlock = m_pkNextBlock;
		}
		NiVBBlock* v4 = NiVBSet::ms_pkBlockDump;
		if (NiVBSet::ms_pkBlockDump) {
			NiVBSet::ms_pkBlockDump->m_pkPrevBlock = v1;
			v4 = NiVBSet::ms_pkBlockDump;
		}
		v1->m_pkNextBlock = v4;
		v1->m_pkPrevBlock = 0;
		++ui;
	}
}
