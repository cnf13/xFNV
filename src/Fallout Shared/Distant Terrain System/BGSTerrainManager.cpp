#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSShader/BSShaderManager.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/Distant Terrain System/BGSDistantObjectBlock.h"
#include "Fallout Shared/Distant Terrain System/BGSDistantTreeBlock.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainChunk.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainNode.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSSegmentedTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "SDK/MSVC/stdio.h"

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bUseNewTerrainSystem("bUseNewTerrainSystem:TerrainManager", true);
SettingT<INISettingCollection> bUseDistantObjectBlocks("bUseDistantObjectBlocks:TerrainManager", true);
SettingT<INISettingCollection> bUseDistantTrees("bUseDistantTrees:TerrainManager", true);
NiPointer<NiNode> BGSTerrainManager::spTreeNode;
NiPointer<NiNode> BGSTerrainManager::spRootLandLODNode;
NiPointer<NiNode> BGSTerrainManager::spRootObjectLODNode;

BGSTerrainManager::BGSTerrainManager(TESWorldSpace* apWorld)
	: pWorld(apWorld), spLODRoot(nullptr), spLODWaterRoot(nullptr), bHasLOD(false) {
	AutoMemContext kContext(MEM_CONTEXT_LODMANAGER);
	char LODPath[264];
	std::sprintf(LODPath, "Data\\LODSettings\\%s.DLODSettings", apWorld->GetFormEditorID());
	BSFile* pFile = nullptr;
	if (FileFinder::Locate(LODPath, 0, 6u, 256u)) {
		pFile = static_cast<BSFile*>(FileFinder::GetFile(LODPath, NiFile::READ_ONLY, 100u));
	}
	if (pFile) {
		*pFile >> iMinLevel >> iMaxLevel >> iRootLevel >> sMinCellX >> sMinCellY >> sMaxCellX >> sMaxCellY >>
			iBlockLevel;
		delete pFile;
		pRootNode = new BGSTerrainNode(this, nullptr, sMinCellX, sMinCellY, iRootLevel, false);
		bHasLOD = true;
	} else {
		pRootNode = nullptr;
	}
	spLODRoot = TES::pInstance->pObjLandRoot;
	if (spLODRoot.m_pObject) {
		spLODWaterRoot = TES::pInstance->pObjLODWaterRoot;
		spLODRoot.m_pObject->SetAlwaysDraw(1);
		spLODRoot.m_pObject->m_pkParent->SetAlwaysDraw(1);
	}
	bNeedsImmediateUpdate = false;
	if (!BGSTerrainManager::spRootLandLODNode.m_pObject && spLODRoot.m_pObject) {
		BGSTerrainManager::spRootLandLODNode = new NiNode(0);
		spLODRoot.m_pObject->AttachChild(BGSTerrainManager::spRootLandLODNode.m_pObject, 1);
		BGSTerrainManager::spRootObjectLODNode = new NiNode(0);
		spLODRoot.m_pObject->AttachChild(BGSTerrainManager::spRootObjectLODNode.m_pObject, 1);
	}
}

BGSTerrainManager::~BGSTerrainManager() {
	if (pRootNode) {
		delete pRootNode;
	}
}

void BGSTerrainManager::TriggerApocalypse() {
	pRootNode->TriggerApocalypseRecurse();
}

BGSDistantTreeBlock* BGSTerrainManager::GetTreeBlockForPoint(const NiPoint3& aPoint) {
	if (pRootNode) {
		return pRootNode->GetTreeBlockForPoint(aPoint);
	} else {
		return nullptr;
	}
}

// The size is read once; each removal shrinks the local count and keeps the index.
void BGSTerrainManager::ReleaseTree(TESObjectREFR* apTreeRef) {
	uint32_t uiSize = TreesQueuedToHide.GetSize();
	uint32_t i = 0;
	while (i < uiSize) {
		if (TreesQueuedToHide.GetAt(i) == apTreeRef) {
			TreesQueuedToHide.Remove(i, true);
			--uiSize;
		} else {
			++i;
		}
	}
}

// The form ID is read before the position (GetLocationOnReference).
void BGSTerrainManager::ShowTree(TESObjectREFR* apTreeRef) {
	uint32_t uiID = apTreeRef->GetLODFormID();
	BGSDistantTreeBlock* pBlock = GetTreeBlockForPoint(apTreeRef->GetLocationOnReference());
	if (pBlock) {
		pBlock->ShowTree(uiID);
	}
}

BGSTerrainChunk* BGSTerrainManager::GetChunkForPoint(const NiPoint3& aPoint) {
	if (pRootNode) {
		return pRootNode->GetChunkForPoint(aPoint);
	} else {
		return nullptr;
	}
}

void BGSTerrainManager::Update(const NiPoint3& aViewPosition, BGSTerrainManager::LOD_UPDATE_TYPE aWhatToUpdate) {
	using tempTreeList_t = BSSimpleArray<TESObjectREFR*, 1024>;
	tagRECT kloadedrect;
	if (BGSTerrainManager::bEnabled && bUseNewTerrainSystem.Bool()) {
		NiPoint3 viewPos = aViewPosition;
		char aeUpdateType = aWhatToUpdate | BGSTerrainManager::UPDATE_LAND_AND_BLOCKS;
		TES::pInstance->GetLoadedAreaRect(kloadedrect, 0.0f);
		BSShaderManager::SetLoadedRange(&kloadedrect);
		if (BGSTerrainManager::pActiveManager) {
			if (BGSTerrainManager::pActiveManager != this) {
				BGSTerrainManager* poldManager = BGSTerrainManager::pActiveManager;
				BGSTerrainManager::pActiveManager = this;
				poldManager->UnloadAll();
				if (BGSTerrainManager::spTreeNode.m_pObject) {
					BGSTerrainManager::spTreeNode.m_pObject->DetachAllChildren();
				}
			}
		}
		BGSTerrainManager::pActiveManager = this;
		if (pRootNode) {
			bool bupdateLandAndBlocks = (aeUpdateType & 1) != 0;
			bool bupdateBlockVisibility = (aeUpdateType & 2) != 0;
			bool bupdateWaterVisibility = (aeUpdateType & 4) != 0;
			bool bupdateTrees = (aeUpdateType & 8) != 0;
			if ((aeUpdateType & 1) != 0) {
				pRootNode->UpdateTerrain(viewPos, bupdateWaterVisibility);
				TES::pInstance->InitDistantTextureBlending();
				if (bUseDistantObjectBlocks.Bool()) {
					pRootNode->UpdateBlocks(viewPos, bupdateBlockVisibility);
				}
			}
			if (bupdateTrees && bUseDistantTrees.Bool()) {
				pRootNode->UpdateTrees(viewPos);
				if (BGSDistantTreeBlock::GetBlockCount()) {
					if (TreesQueuedToHide.iSize) {
						tempTreeList_t tempTreeList;
						uint32_t uisize = TreesQueuedToHide.iSize;
						for (uint32_t i = 0; i < uisize; ++i) {
							uint32_t id = TreesQueuedToHide.GetAt(static_cast<int32_t>(i))->GetLODFormID();
							TESObjectREFR* pRef = TreesQueuedToHide.GetAt(static_cast<int32_t>(i));
							const NiPoint3* v3 = &pRef->GetLocationOnReference();
							BGSDistantTreeBlock* ptrees = GetTreeBlockForPoint(*v3);
							if (ptrees) {
								ptrees->HideTree(id, 1);
							} else {
								tempTreeList.Add(TreesQueuedToHide.GetAt(static_cast<int32_t>(i)));
							}
						}
						TreesQueuedToHide.Clear(1);
						if (tempTreeList.iSize) {
							uint32_t uisize_2 = tempTreeList.iSize;
							TreesQueuedToHide.SetSize(uisize_2, 1);
							for (uint32_t i_2 = 0; i_2 < uisize_2; ++i_2) {
								TreesQueuedToHide.SetAt(i_2, tempTreeList.operator[](i_2));
							}
						}
					}
				}
			}
		}
	}
	bNeedsImmediateUpdate = 0;
}

void BGSTerrainManager::UpdateMorphParameters(const NiPoint3& aViewPosition) {
	if (BGSTerrainManager::bEnabled && bUseNewTerrainSystem.Bool()) {
		if (pRootNode) {
			pRootNode->UpdateMorphParameters(aViewPosition);
		}
	}
}

void BGSTerrainManager::UnloadAll() {
	if (pRootNode) {
		pRootNode->UnloadDataRecurse(BGSTerrainNode::UNLOAD_ALL);
	}
}

void BGSTerrainManager::HideTree(TESObjectREFR* apTreeRef, bool abQueue) {
	uint32_t aID = apTreeRef->GetLODFormID();
	const NiPoint3* v3 = &apTreeRef->GetLocationOnReference();
	BGSDistantTreeBlock* TreeBlockForPoint = GetTreeBlockForPoint(*v3);
	if (TreeBlockForPoint) {
		TreeBlockForPoint->HideTree(aID, 1);
	} else {
		if (abQueue) {
			TreesQueuedToHide.Add(apTreeRef);
		}
	}
}

void BGSTerrainManager::SetNeedsImmediateUpdate(bool abUpdate) {
	bNeedsImmediateUpdate = (bool)((uint8_t)abUpdate);
}

void BGSTerrainManager::UpdateMultiBoundVisibility(NiCamera* apCamera) {
	NiCullingProcess aCullProc(0);
	aCullProc.SetCamera(apCamera);
	const NiFrustum* ViewFrustum = &apCamera->GetViewFrustum();
	aCullProc.SetFrustum(*ViewFrustum);
	if (pRootNode) {
		BGSTerrainChunk::UpdateMultiBoundVisibility(apCamera, aCullProc);
		BGSDistantObjectBlock::ResetCullResults();
	}
}

void BGSTerrainManager::ResetMultiBoundResults() {
	if (pRootNode) {
		BGSTerrainChunk::ResetMultiBoundResults();
		BGSDistantObjectBlock::ResetCullResults();
	}
}

bool BGSTerrainManager::HasRootNode() {
	return (pRootNode != 0);
}

NiTexture* BGSTerrainManager::GetDiffuseTextureForPoint(const NiPoint3& aPoint) {
	int32_t v3 = 0;
	BGSTerrainChunk* chunkForPoint = GetChunkForPoint(aPoint);
	if (chunkForPoint) {
		return chunkForPoint->GetTexture(BGSTerrainChunk::CHUNK_TEXTURE_DIFFUSE);
	}
	return reinterpret_cast<NiTexture*>(static_cast<uintptr_t>(v3));
}

NiTexture* BGSTerrainManager::GetNormalTextureForPoint(const NiPoint3& aPoint) {
	int32_t v3 = 0;
	BGSTerrainChunk* chunkForPoint = GetChunkForPoint(aPoint);
	if (chunkForPoint) {
		return chunkForPoint->GetTexture(BGSTerrainChunk::CHUNK_TEXTURE_NORMAL);
	}
	return reinterpret_cast<NiTexture*>(static_cast<uintptr_t>(v3));
}

void BGSTerrainManager::DestroyNodeAndChildren() {
	if (pRootNode) {
		pRootNode->DestroyRecurse();
	}
}

void BGSTerrainManager::EnableLODWaterProcessing(bool abEnable) {
	BGSTerrainChunk::EnableLODWaterProcessing((uint8_t)abEnable);
}

void BGSTerrainManager::DisableBlockVisibility() {
	BSSegmentedTriShape::SetSystemDisabled(1);
}

void BGSTerrainManager::EnableBlockVisibility() {
	BSSegmentedTriShape::SetSystemDisabled(0);
}

uint32_t BGSTerrainManager::GetTreeLevel() {
	return (uint32_t)8;
}

bool BGSTerrainManager::GetEnabled() {
	return (bool)((uint8_t)BGSTerrainManager::bEnabled);
}

void BGSTerrainManager::ShowMinimumDetail() {
	BGSTerrainChunk::ShowMinimumDetail();
}

void BGSTerrainManager::ShowNormalDetail() {
	BGSTerrainChunk::ShowNormalDetail();
}

BGSTerrainManager* BGSTerrainManager::GetActiveManager() {
	return reinterpret_cast<
		BGSTerrainManager*>(static_cast<uintptr_t>(reinterpret_cast<uintptr_t>(BGSTerrainManager::pActiveManager)));
}

bool BGSTerrainManager::GetCurrentWorldHasLOD() {
	TESWorldSpace* pWorldSpace = TES::pInstance->GetWorldSpace();
	uint8_t result = 0;
	if (!pWorldSpace) {
		return (bool)result;
	}
	if (!pWorldSpace->GetTerrainManager()) {
		return (bool)result;
	}
	// Tests the whole bHasLOD byte for nonzero and returns 1, so the byte is read as untyped memory: a bool read would
	// hand back any other byte as it is.
	if (*static_cast<const uint8_t*>(static_cast<const void*>(&pWorldSpace->GetTerrainManager()->bHasLOD)) != 0) {
		return (bool)1;
	}
	return (bool)result;
}

void BGSTerrainManager::operator delete(void* apMemory) {
	ourFree(apMemory);
}

// Reads a 16-bit value into arVal and records the byte count.
BSFile& BSFile::operator>>(int16_t& arVal) {
	iResult = NiBinaryStream::Read(&arVal, 2u);
	return *this;
}

// Reads a 32-bit value into arVal and records the byte count.
BSFile& BSFile::operator>>(uint32_t& arVal) {
	iResult = NiBinaryStream::Read(&arVal, 4u);
	return *this;
}
