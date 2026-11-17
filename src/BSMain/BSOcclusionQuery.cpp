#include "BSMain/BSOcclusionQuery.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"

// The renderer's D3D device creates the query; D3DQUERYTYPE_OCCLUSION is 9.
BSOcclusionQuery::BSOcclusionQuery() : pQuery(nullptr) {
	static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer)->m_pkD3DDevice9->CreateQuery(9, &pQuery);
}

BSOcclusionQuery::~BSOcclusionQuery() {
	if (pQuery) {
		pQuery->Release();
	}
}

// The exe turns the flag into the D3DGETDATA_FLUSH (1) or 0 it passes as dwGetDataFlags.
bool BSOcclusionQuery::GetResult(uint32_t* pResult, bool bFlush) {
	return pQuery->GetData(pResult, 4, bFlush ? 1u : 0u) == 0;
}

bool BSOcclusionQuery::Begin() {
	if (BSOcclusionQuery::bQueryInProgress) {
		return 0;
	}
	BSOcclusionQuery::bQueryInProgress = 1;
	pQuery->Issue(2);
	return 1;
}

void BSOcclusionQuery::End() {
	if (BSOcclusionQuery::bQueryInProgress) {
		BSOcclusionQuery::bQueryInProgress = 0;
		pQuery->Issue(1);
	}
}

// The exe turns the flag into the D3DGETDATA_FLUSH (1) or 0 it passes as dwGetDataFlags.
bool BSOcclusionQuery::GetResultNoWait(uint32_t* pResult, bool bFlush) {
	return pQuery->GetData(pResult, 4, bFlush ? 1u : 0u) == 0;
}
