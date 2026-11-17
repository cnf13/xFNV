#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX92DBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9IndexBufferManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9LightManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9VertexBufferManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderTargetGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTAbstractPoolAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DGeometryGroupManager.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderInterface.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiUnsharedGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"

// Restores a lost device, packs the pending precache, starts the light manager's frame and the device scene, and
// drops the buffers left queued for presentation. False only when BeginScene fails.
bool NiDX9Renderer::Do_BeginFrame() {
	if (LostDeviceRestore()) {
		PerformPrecache();
		m_pkLightManager->NextFrame();
		if (m_pkD3DDevice9->BeginScene() < 0) {
			return false;
		}
		while (m_kBuffersToUseAtDisplayFrame.GetSize()) {
			m_kBuffersToUseAtDisplayFrame.RemoveHead();
		}
	}
	return true;
}

// Ends the device scene and closes the vertex buffer manager's frame; false when EndScene fails.
bool NiDX9Renderer::Do_EndFrame() {
	if (!m_bDeviceLost) {
		if (m_pkD3DDevice9->EndScene() < 0) {
			return false;
		}
		m_pkVBManager->EndFrame();
	}
	return true;
}

// Unless the device is lost, presents every buffer queued for presentation, emptying the queue. Always true.
bool NiDX9Renderer::Do_DisplayFrame() {
	if (!m_bDeviceLost) {
		using PoolAllocator = NiTAbstractPoolAllocator<uint32_t>;
		using BufferItem = NiTListItem<NiPointer<NiDX92DBufferData>>;
		while (m_kBuffersToUseAtDisplayFrame.m_uiCount) {
			// NiTPointerListBase::RemoveHead with DeleteItem (0xE6E830) and the pool free
			// (NiTPointerAllocator<uint32_t>::Deallocate, 0x45CEE0) inlined: the exe calls neither.
			BufferItem* pkNode = m_kBuffersToUseAtDisplayFrame.m_pkHead;
			m_kBuffersToUseAtDisplayFrame.m_pkHead = pkNode->m_pkNext;
			if (m_kBuffersToUseAtDisplayFrame.m_pkHead)
				m_kBuffersToUseAtDisplayFrame.m_pkHead->m_pkPrev = nullptr;
			else
				m_kBuffersToUseAtDisplayFrame.m_pkTail = nullptr;
			// NiPointer's copy and destructor inlined: the exe increments only a non-null element (0xE75045), but
			// releases it after DisplayFrame with no null test (0xE750B5): the call through it already needed it.
			NiDX92DBufferData* pkBuffer = pkNode->m_element.m_pObject;
			if (pkBuffer)
				pkBuffer->IncRefCount();
			pkNode->m_element = nullptr;
			PoolAllocator::Lock();
			PoolAllocator::AllocNode* pkFreed = reinterpret_cast<PoolAllocator::AllocNode*>(pkNode);
			pkFreed->m_pkData = nullptr;
			pkFreed->m_pkNext = PoolAllocator::ms_pkFreeMem;
			PoolAllocator::ms_pkFreeMem = pkFreed;
			PoolAllocator::Unlock();
			m_kBuffersToUseAtDisplayFrame.m_uiCount--;

			pkBuffer->DisplayFrame();
			pkBuffer->DecRefCount();
		}
	}
	return true;
}

// Clears the current group's buffers named by auiMode (NiRenderer::ClearFlags) inside apRect, a [0,1] rectangle
// whose top and bottom count from the bottom edge (null: the whole target). Depth and stencil are cleared only when
// the depth/stencil buffer has them.
void NiDX9Renderer::Do_ClearBuffer(const NiRect<float>* apRect, uint32_t auiMode) {
	if (!auiMode || m_bDeviceLost || !m_pkCurrRenderTargetGroup) {
		return;
	}

	uint32_t uiWidth = m_pkCurrRenderTargetGroup->GetWidth(0);
	uint32_t uiHeight = m_pkCurrRenderTargetGroup->GetHeight(0);
	D3DRECT kRect;
	if (apRect) {
		float fWidth = static_cast<float>(uiWidth);
		float fHeight = static_cast<float>(uiHeight);
		kRect.x1 = static_cast<int32_t>(apRect->m_left * fWidth);
		kRect.y1 = static_cast<int32_t>((1.0f - apRect->m_top) * fHeight);
		kRect.x2 = static_cast<int32_t>(apRect->m_right * fWidth);
		kRect.y2 = static_cast<int32_t>((1.0f - apRect->m_bottom) * fHeight);
	} else {
		kRect.x1 = 0;
		kRect.y1 = 0;
		kRect.x2 = uiWidth;
		kRect.y2 = uiHeight;
	}

	uint32_t uiD3DFlags = auiMode & CLEAR_BACKBUFFER;
	Ni2DBuffer::RendererData* pkDSData = m_pkCurrRenderTargetGroup->GetDepthStencilBufferRendererData();
	if (pkDSData) {
		NiDX9DepthStencilBufferData* pkDepthData =
			static_cast<NiDX9DepthStencilBufferData*>(NiDX92DBufferData::DynamicCast(pkDSData,
				&NiDX9DepthStencilBufferData::ms_RTTI));
		if (pkDepthData) {
			uiD3DFlags |= ((auiMode & CLEAR_ZBUFFER) && pkDepthData->HasValidDepthBuffer()) ? D3DCLEAR_ZBUFFER : 0;
			uiD3DFlags |= ((auiMode & CLEAR_STENCIL) && pkDepthData->HasValidStencilBuffer()) ? D3DCLEAR_STENCIL : 0;
		}
	}

	m_kD3DPort.Width = m_pkCurrRenderTargetGroup->GetWidth(0);
	m_kD3DPort.Height = m_pkCurrRenderTargetGroup->GetHeight(0);
	m_pkD3DDevice9->SetViewport(&m_kD3DPort);
	m_pkD3DDevice9->Clear(1, &kRect, uiD3DFlags, m_uiBackground, m_fZClear, m_uiStencilClear);
}

// Binds the group's colour buffers and depth/stencil buffer, then clears it. When a binding fails the default group
// is bound instead and the call fails.
bool NiDX9Renderer::Do_BeginUsingRenderTargetGroup(NiRenderTargetGroup* apTarget, NiRenderer::ClearFlags aeClearMode) {
	if (m_bDeviceLost) {
		return true;
	}

	NiDX92DBufferData::ClearRenderTarget(m_pkD3DDevice9, 1);
	NiDX92DBufferData::ClearRenderTarget(m_pkD3DDevice9, 2);
	NiDX92DBufferData::ClearRenderTarget(m_pkD3DDevice9, 3);

	for (uint32_t i = 0; i < apTarget->GetBufferCount(); i++) {
		NiDX92DBufferData* pkBuffData = static_cast<NiDX92DBufferData*>(apTarget->GetBufferRendererData(i));
		if (pkBuffData && pkBuffData->IsKindOf(&NiDX92DBufferData::ms_RTTI) &&
			!pkBuffData->SetRenderTarget(m_pkD3DDevice9, i)) {
			Do_BeginUsingRenderTargetGroup(m_pkDefaultRenderTargetGroup, aeClearMode);
			Error("NiDX9Renderer::BeginUsingRenderTargetGroup> Failed - %s",
				"Setting an output target failed. Resetting to default target.");
			return false;
		}
	}

	NiDX9DepthStencilBufferData* pkDepthData =
		static_cast<NiDX9DepthStencilBufferData*>(apTarget->GetDepthStencilBufferRendererData());
	if (pkDepthData && pkDepthData->IsKindOf(&NiDX9DepthStencilBufferData::ms_RTTI)) {
		if (!pkDepthData->SetDepthTarget(m_pkD3DDevice9)) {
			Do_BeginUsingRenderTargetGroup(m_pkDefaultRenderTargetGroup, aeClearMode);
			Error("NiDX9Renderer::BeginUsingRenderTargetGroup> Failed - %s",
				"Setting the depth/stencil buffer failed. Resetting to default target.");
			return false;
		}
	} else if (!NiDX9DepthStencilBufferData::SetNULLDepthStencilTarget(m_pkD3DDevice9)) {
		Do_BeginUsingRenderTargetGroup(m_pkDefaultRenderTargetGroup, aeClearMode);
		Error("NiDX9Renderer::BeginUsingRenderTargetGroup> Failed - %s",
			"Setting the depth/stencil buffer to NULL failed. Resetting to default target.");
		return false;
	}

	NiDX92DBufferData* pkFirstData = static_cast<NiDX92DBufferData*>(apTarget->GetBufferRendererData(0));
	bool bOnscreen = pkFirstData && pkFirstData->IsKindOf(&NiDX9OnscreenBufferData::ms_RTTI);
	m_pkCurrRenderTargetGroup = apTarget;
	if (bOnscreen) {
		m_pkCurrOnscreenRenderTargetGroup = apTarget;
	}
	Do_ClearBuffer(nullptr, aeClearMode);
	return true;
}

// Resolves each multisampled colour buffer of the current group into its surface and queues the buffers that are
// presented at DisplayFrame, then leaves the group.
bool NiDX9Renderer::Do_EndUsingRenderTargetGroup() {
	if (m_bDeviceLost) {
		return true;
	}

	for (uint32_t i = 0; i < m_pkCurrRenderTargetGroup->GetBufferCount(); i++) {
		Ni2DBuffer::RendererData* pkData = m_pkCurrRenderTargetGroup->GetBufferRendererData(i);
		NiDX92DBufferData* pkBuffData =
			(pkData && static_cast<NiDX92DBufferData*>(pkData)->IsKindOf(&NiDX92DBufferData::ms_RTTI))
			? static_cast<NiDX92DBufferData*>(pkData)
			: nullptr;
		if (pkBuffData->GetMSAALevel()) {
			// filter 0: D3DTEXF_NONE
			m_pkD3DDevice9->StretchRect(pkBuffData->m_pkSurface2, nullptr, pkBuffData->m_pkSurface, nullptr, 0);
		}
		if (pkBuffData->CanDisplayFrame()) {
			if (!m_kBuffersToUseAtDisplayFrame.FindPos(pkBuffData)) {
				m_kBuffersToUseAtDisplayFrame.AddTail(pkBuffData);
			}
		}
	}

	m_pkCurrRenderTargetGroup = nullptr;
	m_pkCurrOnscreenRenderTargetGroup = nullptr;
	return true;
}

// Draws the batch: the batched shader's pipeline runs once for the batch (its first geometry's inputs), every pass
// sets up from the first geometry and then draws each batched geometry. The batch is emptied afterwards, or at once
// when there is nothing to draw or the shader's PreProcessPipeline fails.
void NiDX9Renderer::Do_EndBatch() {
	BatchedObject* pkHead = m_pkBatchHead;
	if (!pkHead || m_bDeviceLost) {
		m_spBatchedShader = nullptr;
		m_pkBatchedPropertyState = nullptr;
		m_pkBatchedEffectState = nullptr;
		EmptyBatch();
		return;
	}

	NiGeometryBufferData* pkBuffData = pkHead->m_pkBuffData;
	NiGeometry* pkGeometry = pkHead->m_pkGeometry;
	NiSkinInstance* pkSkin = pkGeometry->m_spSkinInstance;
	const NiBound* pkBound = pkGeometry->m_kWorldBound ? pkGeometry->m_kWorldBound : &NiAVObject::NullBoundS;
	if (m_spBatchedShader->PreProcessPipeline(pkGeometry, pkSkin, pkBuffData, m_pkBatchedPropertyState,
			m_pkBatchedEffectState, pkGeometry->m_kWorld, *pkBound)) {
		m_spBatchedShader = nullptr;
		m_pkBatchedPropertyState = nullptr;
		m_pkBatchedEffectState = nullptr;
		EmptyBatch();
		return;
	}

	m_spBatchedShader->UpdatePipeline(pkGeometry, pkSkin, pkBuffData, m_pkBatchedPropertyState, m_pkBatchedEffectState,
		pkGeometry->m_kWorld, *pkBound);

	uint32_t uiRemainingPasses = m_spBatchedShader->FirstPass();
	while (uiRemainingPasses) {
		BatchedObject* pkCurr = m_pkBatchHead;
		NiGeometry* pkPassGeometry = pkCurr->m_pkGeometry;
		const NiBound* pkPassBound =
			pkPassGeometry->m_kWorldBound ? pkPassGeometry->m_kWorldBound : &NiAVObject::NullBoundS;
		m_spBatchedShader->SetupRenderingPass(pkPassGeometry, pkPassGeometry->m_spSkinInstance, pkCurr->m_pkBuffData,
			m_pkBatchedPropertyState, m_pkBatchedEffectState, pkPassGeometry->m_kWorld, *pkPassBound);

		do {
			pkGeometry = pkCurr->m_pkGeometry;
			pkBuffData = pkCurr->m_pkBuffData;
			const NiSkinPartition::Partition* pkPartition =
				static_cast<const NiSkinPartition::Partition*>(pkCurr->m_pkCurrPartition);
			pkSkin = pkGeometry->m_spSkinInstance;
			pkBound = pkGeometry->m_kWorldBound ? pkGeometry->m_kWorldBound : &NiAVObject::NullBoundS;

			m_spBatchedShader->SetupTransformations(pkGeometry, pkSkin, pkPartition, pkBuffData,
				m_pkBatchedPropertyState, m_pkBatchedEffectState, pkGeometry->m_kWorld, *pkBound);
			pkBuffData = static_cast<NiGeometryBufferData*>(m_spBatchedShader->PrepareGeometryForRendering(pkGeometry,
				pkPartition, pkBuffData, m_pkBatchedPropertyState));
			m_spBatchedShader->SetupShaderPrograms(pkGeometry, pkSkin, pkPartition, pkBuffData,
				m_pkBatchedPropertyState, m_pkBatchedEffectState, pkGeometry->m_kWorld, *pkBound);

			if (pkBuffData->m_pkIB) {
				uint32_t uiStartIndex = 0;
				for (uint32_t i = 0; i < pkBuffData->m_uiNumArrays; i++) {
					uint32_t uiPrimitiveCount =
						pkBuffData->m_pusArrayLengths ? pkBuffData->m_pusArrayLengths[i] - 2 : pkBuffData->m_uiTriCount;
					m_pkD3DDevice9->DrawIndexedPrimitive(pkBuffData->m_eType, pkBuffData->m_uiBaseVertexIndex, 0,
						pkBuffData->m_uiVertCount, uiStartIndex, uiPrimitiveCount);
					uiStartIndex += uiPrimitiveCount + 2;
				}
			} else {
				m_pkD3DDevice9->DrawPrimitive(pkBuffData->m_eType, pkBuffData->m_uiBaseVertexIndex,
					pkBuffData->m_uiTriCount);
			}

			m_spBatchedShader->PostRender(pkGeometry, pkSkin, pkPartition, pkBuffData, m_pkBatchedPropertyState,
				m_pkBatchedEffectState, pkGeometry->m_kWorld, *pkBound);
			pkCurr = pkCurr->m_pkNext;
		} while (pkCurr);

		uiRemainingPasses = m_spBatchedShader->NextPass();
	}

	// Keep only the high four bits (the consistency type) of each batched geometry's dirty flags.
	for (BatchedObject* pkObject = m_pkBatchHead; pkObject; pkObject = pkObject->m_pkNext) {
		pkObject->m_pkGeometry->m_spModelData->m_usDirtyFlags &= 0xF000;
	}

	m_spBatchedShader->PostProcessPipeline(pkGeometry, pkSkin, pkBuffData, m_pkBatchedPropertyState,
		m_pkBatchedEffectState, pkGeometry->m_kWorld, *pkBound);

	m_spBatchedShader = nullptr;
	m_pkBatchedPropertyState = nullptr;
	m_pkBatchedEffectState = nullptr;
	EmptyBatch();
	m_pkRenderState->SetInternalNormalizeNormals(false);
}

// Do_RenderScreenTexture (0xE750E0) stays with the exe: it passes DrawPrimitive a world bound it never initialises
// (the NiBound local at [esp+0x14]), an uninitialised read.
// Queues apShape in the open batch.
void NiDX9Renderer::Do_BatchRenderShape(NiTriShape* apShape) {
	AddToBatch(apShape);
}

// Queues apStrips in the open batch.
void NiDX9Renderer::Do_BatchRenderStrips(NiTriStrips* apStrips) {
	AddToBatch(apStrips);
}

// Draws apShape at once in the default geometry group, skinned on the hardware when it can be.
void NiDX9Renderer::Do_RenderShape(NiTriShape* apShape) {
	if (m_bDeviceLost) {
		return;
	}

	NiGeometryData* pkData = apShape->m_spModelData;
	if (!pkData->GetActiveVertexCount()) {
		return;
	}

	NiSkinInstance* pkSkin = apShape->m_spSkinInstance;
	NiTransform kWorld = apShape->m_kWorld;
	NiBound kWorldBound = apShape->m_kWorldBound ? *apShape->m_kWorldBound : NiAVObject::NullBoundS;
	bool bHWSkinned = IsHardwareSkinned(apShape, nullptr);
	m_pkGeometryGroupManager->AddObjectToGroup(m_pkDefaultGroup, pkData, pkSkin, bHWSkinned, 0, 0);
	if (bHWSkinned) {
		DrawSkinnedPrimitive(apShape, pkData, pkSkin, kWorld, kWorldBound);
	} else {
		DrawPrimitive(apShape, pkData, pkSkin, kWorld, kWorldBound, pkData->m_pkBuffData);
	}
}

// Draws apStrips at once in the default geometry group, skinned on the hardware when it can be.
void NiDX9Renderer::Do_RenderTristrips(NiTriStrips* apStrips) {
	if (m_bDeviceLost) {
		return;
	}

	NiGeometryData* pkData = apStrips->m_spModelData;
	if (!pkData->GetActiveVertexCount()) {
		return;
	}

	NiSkinInstance* pkSkin = apStrips->m_spSkinInstance;
	NiTransform kWorld = apStrips->m_kWorld;
	NiBound kWorldBound = apStrips->m_kWorldBound ? *apStrips->m_kWorldBound : NiAVObject::NullBoundS;
	bool bHWSkinned = IsHardwareSkinned(apStrips, nullptr);
	m_pkGeometryGroupManager->AddObjectToGroup(m_pkDefaultGroup, pkData, pkSkin, bHWSkinned, 0, 0);
	if (bHWSkinned) {
		DrawSkinnedPrimitive(apStrips, pkData, pkSkin, kWorld, kWorldBound);
	} else {
		DrawPrimitive(apStrips, pkData, pkSkin, kWorld, kWorldBound, pkData->m_pkBuffData);
	}
}

// Four vertices (pixel corners offset by -0.5, the rectangle's color, texel coordinates scaled by the inverse
// texture size) and two triangles per screen rectangle. The positions wrap into the current target's size. With the
// rectangle-count dirty bit (8), the CPU arrays grow as needed, a vertex chip too small for the vertices forces a full
// repack (0xF), and the indices are rebuilt and packed. ausDirtyFlags reaches PackTransformedVB zero-extended (as the
// only caller pushes it) or as 0xF.
void NiDX9Renderer::PackScreenTextureBuffers(NiScreenTexture* apScreenTexture, uint16_t ausDirtyFlags) {
	uint32_t uiDirtyFlags = ausDirtyFlags;
	uint16_t usRects = apScreenTexture->m_kScreenRects.m_uiUsed;
	NiGeometryBufferData* pkBuffData = apScreenTexture->m_pkBuffData;
	uint16_t usVerts = static_cast<uint16_t>(usRects * 4);
	uint32_t uiIndices = (usVerts * 3) / 2;
	pkBuffData->m_uiTriCount = usRects * 2;
	pkBuffData->m_uiMaxTriCount = usRects * 2;
	pkBuffData->m_uiVertCount = usVerts;
	pkBuffData->m_uiMaxVertCount = usVerts;
	pkBuffData->m_pusIndexArray = nullptr;
	pkBuffData->m_pusArrayLengths = nullptr;
	pkBuffData->m_uiNumArrays = 1;
	NiVBChip* pkVBChip = pkBuffData->m_uiStreamCount ? pkBuffData->m_ppkVBChip[0] : nullptr;

	bool bRectCountChanged = (ausDirtyFlags >> 3) & 1;
	if (bRectCountChanged) {
		if (usVerts > m_usNumScreenTextureVerts) {
			// delete[] and new[] of the three arrays: Bethesda's operators on MemoryManager; only NiColorA has a
			// constructor (zero), run through the eh vector constructor iterator when the block is not null.
			MemoryManager_Singleton.Deallocate(m_pkScreenTextureVerts);
			MemoryManager_Singleton.Deallocate(m_pkScreenTextureColors);
			MemoryManager_Singleton.Deallocate(m_pkScreenTextureTexCoords);
			m_pkScreenTextureVerts =
				static_cast<NiPoint2*>(MemoryManager_Singleton.Allocate(usVerts * sizeof(NiPoint2)));
			NiColorA* pkColors = static_cast<NiColorA*>(MemoryManager_Singleton.Allocate(usVerts * sizeof(NiColorA)));
			if (pkColors) {
				for (uint32_t i = 0; i < usVerts; ++i) {
					::new (static_cast<void*>(&pkColors[i])) NiColorA();
				}
			}
			m_pkScreenTextureColors = pkColors;
			m_pkScreenTextureTexCoords =
				static_cast<NiPoint2*>(MemoryManager_Singleton.Allocate(usVerts * sizeof(NiPoint2)));
			m_usNumScreenTextureVerts = usVerts;
		}
		if (pkVBChip) {
			uint32_t uiStride = pkBuffData->m_uiStreamCount ? pkBuffData->m_puiVertexStride[0] : 0;
			if (usVerts > pkVBChip->m_uiSize / uiStride) {
				uiDirtyFlags = 0xF;
			}
		}
	}

	// The exe then tests (dirty | ~8) == 0, which never holds: the vertices are always packed.
	int32_t iWidth = static_cast<int32_t>(m_pkCurrRenderTargetGroup->GetWidth(0));
	int32_t iHeight = static_cast<int32_t>(m_pkCurrRenderTargetGroup->GetHeight(0));
	NiTexturingProperty::Map* pkMap = apScreenTexture->m_spTexProp.m_pObject->m_kMaps.m_pBase[0];
	NiTexture* pkTexture = pkMap ? pkMap->m_spTexture.m_pObject : nullptr;
	uint16_t usTexWidth = static_cast<uint16_t>(pkTexture->GetWidth());
	pkMap = apScreenTexture->m_spTexProp.m_pObject->m_kMaps.m_pBase[0];
	pkTexture = pkMap ? pkMap->m_spTexture.m_pObject : nullptr;
	float fInvTexWidth = 1.0f / static_cast<int32_t>(usTexWidth);
	uint16_t usTexHeight = static_cast<uint16_t>(pkTexture->GetHeight());
	float fInvTexHeight = 1.0f / static_cast<int32_t>(usTexHeight);

	for (uint16_t r = 0; r < usRects; ++r) {
		const NiScreenTexture::ScreenRect& rkRect = apScreenTexture->m_kScreenRects.m_pBase[r];
		int16_t sPixLeft = rkRect.m_sPixLeft;
		while (sPixLeft < 0) {
			sPixLeft = static_cast<int16_t>(sPixLeft + iWidth);
		}
		while (sPixLeft >= iWidth) {
			sPixLeft = static_cast<int16_t>(sPixLeft - iWidth);
		}
		int16_t sPixTop = rkRect.m_sPixTop;
		while (sPixTop < 0) {
			sPixTop = static_cast<int16_t>(sPixTop + iHeight);
		}
		while (sPixTop >= iHeight) {
			sPixTop = static_cast<int16_t>(sPixTop - iHeight);
		}

		float fLeft = static_cast<int32_t>(sPixLeft) - 0.5;
		float fTop = static_cast<int32_t>(sPixTop) - 0.5;
		float fRight = fLeft + static_cast<int32_t>(rkRect.m_usPixWidth);
		float fBottom = fTop + static_cast<int32_t>(rkRect.m_usPixHeight);
		float fTexLeft = static_cast<int32_t>(rkRect.m_usTexLeft) * fInvTexWidth;
		float fTexTop = static_cast<int32_t>(rkRect.m_usTexTop) * fInvTexHeight;
		// The width and height times the inverse texture size are not rounded to float before the sum.
		float fTexRight = fTexLeft + static_cast<int32_t>(rkRect.m_usPixWidth) * fInvTexWidth;
		float fTexBottom = fTexTop + static_cast<int32_t>(rkRect.m_usPixHeight) * fInvTexHeight;

		uint16_t usVertex = static_cast<uint16_t>(r * 4);
		for (uint16_t j = 0; j < 4; ++j, ++usVertex) {
			m_pkScreenTextureVerts[usVertex].x = (j >> 1) ? fRight : fLeft;
			m_pkScreenTextureVerts[usVertex].y = (j % 2) ? fBottom : fTop;
			m_pkScreenTextureColors[usVertex] = rkRect.m_kColor;
			m_pkScreenTextureTexCoords[usVertex].x = (j >> 1) ? fTexRight : fTexLeft;
			m_pkScreenTextureTexCoords[usVertex].y = (j % 2) ? fTexBottom : fTexTop;
		}
	}

	if (!m_pkVBManager->PackTransformedVB(pkBuffData, usVerts, m_pkScreenTextureVerts, m_pkScreenTextureColors,
			m_pkScreenTextureTexCoords, uiDirtyFlags, pkVBChip, nullptr)) {
		return;
	}
	if (!bRectCountChanged) {
		return;
	}

	if (uiIndices > m_uiNumScreenTextureIndices) {
		_NiFree(m_pusScreenTextureIndices);
		m_pusScreenTextureIndices = static_cast<uint16_t*>(_NiMalloc(uiIndices * sizeof(uint16_t)));
		m_uiNumScreenTextureIndices = uiIndices;
		uint16_t usIndex = 0;
		for (uint32_t v = 0; v < usVerts; v += 4) {
			uint16_t usV = static_cast<uint16_t>(v);
			m_pusScreenTextureIndices[usIndex++] = usV;
			m_pusScreenTextureIndices[usIndex++] = static_cast<uint16_t>(usV + 1);
			m_pusScreenTextureIndices[usIndex++] = static_cast<uint16_t>(usV + 2);
			m_pusScreenTextureIndices[usIndex++] = static_cast<uint16_t>(usV + 2);
			m_pusScreenTextureIndices[usIndex++] = static_cast<uint16_t>(usV + 1);
			m_pusScreenTextureIndices[usIndex++] = static_cast<uint16_t>(usV + 3);
		}
	}

	// No null check on the new index buffer here.
	IDirect3DIndexBuffer9* pkOldIB = pkBuffData->m_pkIB;
	uint32_t uiIBSize = pkBuffData->m_uiIBSize;
	IDirect3DIndexBuffer9* pkIB = m_pkIBManager->PackBuffer(m_pusScreenTextureIndices, uiIndices, uiIndices, pkOldIB,
		&uiIBSize, bUseDirect3D9Ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED,
		pkBuffData->m_bSoftwareVP ? D3DUSAGE_SOFTWAREPROCESSING : 0);
	if (pkIB != pkOldIB) {
		pkBuffData->RemoveIB();
		pkBuffData->m_pkIB = pkIB;
		pkBuffData->m_uiIndexCount = uiIndices;
		pkBuffData->m_uiIBSize = uiIBSize;
	}
}

// One stream of position, color and one texture set. An invalid chip is released and everything repacked (0xF), a
// dynamic group always repacks everything, otherwise only the dirty parts are packed; the dirty flags are cleared.
void NiDX9Renderer::PackScreenTextureGeomBuffer(NiScreenTexture* apScreenTexture) {
	if (!apScreenTexture) {
		return;
	}

	NiGeometryBufferData* pkBuffData = apScreenTexture->m_pkBuffData;
	bool bValid = pkBuffData->IsVBChipValid();
	pkBuffData->m_uiFlags = 0x1400000;
	pkBuffData->SetStreamCount(1);
	NiGeometryGroup* pkGroup = pkBuffData->m_pkGeometryGroup;
	uint16_t usDirtyFlags = apScreenTexture->m_usDirtyFlags;
	if (!bValid) {
		pkGroup->ReleaseChip(pkBuffData, 0);
		pkBuffData->m_uiBaseVertexIndex = 0;
		PackScreenTextureBuffers(apScreenTexture, 0xF);
	} else if (pkGroup->IsDynamic()) {
		PackScreenTextureBuffers(apScreenTexture, 0xF);
	} else if (usDirtyFlags) {
		PackScreenTextureBuffers(apScreenTexture, usDirtyFlags);
	}
	apScreenTexture->m_usDirtyFlags = 0;
}
