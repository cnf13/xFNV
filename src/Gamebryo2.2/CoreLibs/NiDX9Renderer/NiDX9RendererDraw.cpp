#include "BSShader/BSShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9IndexBufferManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9VertexBufferManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSResizableTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSSegmentedTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLinesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticlesData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/BSStripParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/BSStripPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DGeometryGroupManager.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DUtility.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiDynamicGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiVBChip.h"

// Inline NiSinCos, used by PackPoints and PackPointsSubTexture; NiSinCos in NiMath.h is a separate function.
static inline void InlineNiSinCos(float afRadians, float& arfSin, float& arfCos) {
	arfSin = std::sin(afRadians);
	arfCos = std::cos(afRadians);
}

// A particle color as a D3DCOLOR: each channel times 255 (a double), rounded to float, then converted to an integer.
static inline uint32_t PackPointColor(const NiColorA& arColor) {
	int32_t iA = NiFastFloatToInt(arColor.a * 255.0);
	int32_t iR = NiFastFloatToInt(arColor.r * 255.0);
	int32_t iG = NiFastFloatToInt(arColor.g * 255.0);
	int32_t iB = NiFastFloatToInt(arColor.b * 255.0);
	return (((static_cast<uint32_t>(iA) << 8 | iR) << 8 | iG) << 8) | iB;
}

// The two corner offsets of a rotated point sprite: with fA = (sin + cos) * afSize and fB = (cos - sin) * afSize,
// arCorner0 = right * fA + up * fB and arCorner1 = right * -fB + up * fA. sin + cos and cos - sin stay unrounded;
// fA, fB and every other product and sum are rounded to float.
static inline void GetRotatedPointCorners(const NiPoint3& arRight, const NiPoint3& arUp, float afSize, float afAngle,
	NiPoint3& arCorner0, NiPoint3& arCorner1) {
	float fSin;
	float fCos;
	InlineNiSinCos(afAngle, fSin, fCos);
	float fA = (fSin + fCos) * afSize;
	float fB = (fCos - fSin) * afSize;

	NiPoint3 kUpB;
	kUpB.x = arUp.x * fB;
	kUpB.y = arUp.y * fB;
	kUpB.z = arUp.z * fB;
	NiPoint3 kRightA;
	kRightA.x = arRight.x * fA;
	kRightA.y = arRight.y * fA;
	kRightA.z = arRight.z * fA;
	arCorner0.x = kRightA.x + kUpB.x;
	arCorner0.y = kRightA.y + kUpB.y;
	arCorner0.z = kRightA.z + kUpB.z;

	NiPoint3 kUpA;
	kUpA.x = arUp.x * fA;
	kUpA.y = arUp.y * fA;
	kUpA.z = arUp.z * fA;
	fB = -fB;
	NiPoint3 kRightNegB;
	kRightNegB.x = arRight.x * fB;
	kRightNegB.y = arRight.y * fB;
	kRightNegB.z = arRight.z * fB;
	arCorner1.x = kRightNegB.x + kUpA.x;
	arCorner1.y = kRightNegB.y + kUpA.y;
	arCorner1.z = kRightNegB.z + kUpA.z;
}

// One point-sprite vertex: position, normal, D3DCOLOR, u, v (0x24 bytes; PackPoints appends the texture index).
static inline void PackPointVertex(float* apfVert, float afX, float afY, float afZ, const NiPoint3& arNormal,
	uint32_t auiColor, float afU, float afV) {
	apfVert[0] = afX;
	apfVert[1] = afY;
	apfVert[2] = afZ;
	apfVert[3] = arNormal.x;
	apfVert[4] = arNormal.y;
	apfVert[5] = arNormal.z;
	*reinterpret_cast<uint32_t*>(&apfVert[6]) = auiColor;
	apfVert[7] = afU;
	apfVert[8] = afV;
}

// The vertex component flags of a geometry buffer, as used by PackSkinnedGeometryBuffer, ReservePrepackedBufferSpace
// and PackGeometryBuffer: bit 0 of the data flags as the texture set count, capped at
// NiD3DPass::ms_uiMaxTextureBlendStages, in the top byte; colors 0x400000, normals 0x800000, and 0x200000 for normals
// with an NBT method.
static uint32_t GetGeometryBufferFlags(const NiGeometryData* apData) {
	bool bColors = apData->m_pkColor != nullptr;
	bool bNormals = apData->m_pkNormal != nullptr;
	bool bNBTs = bNormals && (apData->m_usDataFlags & 0xF000);
	uint32_t uiTextureSets = apData->m_usDataFlags & 1;
	if (uiTextureSets > NiD3DPass::ms_uiMaxTextureBlendStages)
		uiTextureSets = NiD3DPass::ms_uiMaxTextureBlendStages;
	uint32_t uiFlags = 0;
	if (bColors)
		uiFlags = 0x400000;
	if (bNormals)
		uiFlags |= 0x800000;
	if (bNBTs)
		uiFlags |= 0x200000;
	return (uiTextureSets << 24) | uiFlags;
}

// Draws an already set-up triangle shape: per skin partition when hardware skinned, else per index array (per enabled
// segment of a segmented shape), else unindexed. The geometry data's dirty flags are cleared after the draw.
void NiDX9Renderer::Do_OnlyRenderShape(NiTriShape* apShape) {
	if (m_bDeviceLost)
		return;

	NiGeometryData* pkModelData = apShape->m_spModelData.m_pObject;
	if (!pkModelData->GetActiveVertexCount())
		return;

	NiSkinInstance* pkSkin = apShape->m_spSkinInstance.m_pObject;
	bool bHWSkinned = IsHardwareSkinned(apShape, nullptr);
	NiGeometryBufferData* pkBuffData = apShape->m_spModelData.m_pObject->m_pkBuffData;
	apShape->m_spShader->FirstPass();

	if (bHWSkinned) {
		NiSkinPartition* pkSkinPartition = pkSkin->m_spSkinPartition.m_pObject;
		uint32_t uiPartitions = pkSkinPartition->m_uiPartitions;
		NiSkinPartition::Partition* pkPartition = pkSkinPartition->m_pkPartitions;
		uint32_t ePrimitiveType = pkPartition->m_usStrips ? D3DPT_TRIANGLESTRIP : D3DPT_TRIANGLELIST;
		for (uint32_t i = 0; i < uiPartitions; ++i, ++pkPartition) {
			NiGeometryBufferData* pkPartBuffData = pkPartition->m_pkBuffData;
			uint32_t uiStartIndex = 0;
			for (uint32_t j = 0; j < pkPartBuffData->m_uiNumArrays; ++j) {
				uint32_t uiPrimitives;
				if (pkPartBuffData->m_pusArrayLengths) {
					uiPrimitives = pkPartBuffData->m_pusArrayLengths[j] - 2;
				} else {
					uiPrimitives = pkPartBuffData->m_uiTriCount;
					if (apShape->IsResizableTriShape())
						uiPrimitives = apShape->IsResizableTriShape()->uiNumPrimitives;
				}
				m_pkD3DDevice9->DrawIndexedPrimitive(ePrimitiveType, pkPartBuffData->m_uiBaseVertexIndex, 0,
					pkPartition->m_usVertices, uiStartIndex, uiPrimitives);
				uiStartIndex += uiPrimitives + 2;
			}
		}
	} else if (pkBuffData->m_pkIB) {
		uint32_t uiStartIndex = 0;
		for (uint32_t i = 0; i < pkBuffData->m_uiNumArrays; ++i) {
			uint32_t uiPrimitives;
			if (pkBuffData->m_pusArrayLengths)
				uiPrimitives = pkBuffData->m_pusArrayLengths[i] - 2;
			else
				uiPrimitives = pkBuffData->m_uiTriCount;

			if (apShape->IsSegmentedTriShape()) {
				// One draw per enabled segment; with bIgnoreSegments, one draw of the whole shape.
				BSSegmentedTriShape* pkSegmented = apShape->IsSegmentedTriShape();
				uint32_t uiSegments = pkSegmented->bIgnoreSegments ? 1 : pkSegmented->iNumSegments;
				for (uint32_t j = 0; j < uiSegments; ++j) {
					BSSegmentedTriShape::Segment* pkSegment = &pkSegmented->pSegments[j];
					if (pkSegment->bEnabledPostMerge) {
						bool bIgnoreSegments = pkSegmented->bIgnoreSegments;
						uint32_t uiSegmentPrimitives = bIgnoreSegments
							? static_cast<NiTriBasedGeomData*>(pkSegmented->m_spModelData.m_pObject)->m_usTriangles
							: pkSegment->iNumPrimitivesMerged;
						uint32_t uiSegmentStart = bIgnoreSegments ? 0 : pkSegment->iStartIndex;
						m_pkD3DDevice9->DrawIndexedPrimitive(pkBuffData->m_eType, pkBuffData->m_uiBaseVertexIndex, 0,
							pkBuffData->m_uiVertCount, uiSegmentStart, uiSegmentPrimitives);
					}
				}
			} else {
				m_pkD3DDevice9->DrawIndexedPrimitive(pkBuffData->m_eType, pkBuffData->m_uiBaseVertexIndex, 0,
					pkBuffData->m_uiVertCount, uiStartIndex, uiPrimitives);
			}
			uiStartIndex += uiPrimitives + 2;
		}
	} else {
		m_pkD3DDevice9->DrawPrimitive(pkBuffData->m_eType, pkBuffData->m_uiBaseVertexIndex, pkBuffData->m_uiTriCount);
	}

	pkModelData->m_usDirtyFlags &= 0xF000;
}

// Draws already set-up triangle strips like Do_OnlyRenderShape (without segments or resizing), after taking the
// strips' world bound as the renderer's current bound.
void NiDX9Renderer::Do_OnlyRenderTristrips(NiTriStrips* apStrips) {
	if (m_bDeviceLost)
		return;

	NiGeometryData* pkModelData = apStrips->m_spModelData.m_pObject;
	if (!pkModelData->GetActiveVertexCount())
		return;

	NiSkinInstance* pkSkin = apStrips->m_spSkinInstance.m_pObject;
	const NiBound* pkBound = apStrips->m_kWorldBound ? apStrips->m_kWorldBound : &NiAVObject::NullBoundS;
	NiPoint3 kCenter = pkBound->m_kCenter;
	float fRadius = pkBound->m_fRadius;
	bool bHWSkinned = IsHardwareSkinned(apStrips, nullptr);
	NiGeometryBufferData* pkBuffData = apStrips->m_spModelData.m_pObject->m_pkBuffData;
	apStrips->m_spShader->FirstPass();
	m_kWorldBound.m_fRadius = fRadius;
	m_kWorldBound.m_kCenter = kCenter;

	if (bHWSkinned) {
		NiSkinPartition* pkSkinPartition = pkSkin->m_spSkinPartition.m_pObject;
		uint32_t uiPartitions = pkSkinPartition->m_uiPartitions;
		NiSkinPartition::Partition* pkPartition = pkSkinPartition->m_pkPartitions;
		uint32_t ePrimitiveType = pkPartition->m_usStrips ? D3DPT_TRIANGLESTRIP : D3DPT_TRIANGLELIST;
		for (uint32_t i = 0; i < uiPartitions; ++i, ++pkPartition) {
			NiGeometryBufferData* pkPartBuffData = pkPartition->m_pkBuffData;
			uint32_t uiStartIndex = 0;
			for (uint32_t j = 0; j < pkPartBuffData->m_uiNumArrays; ++j) {
				uint32_t uiPrimitives = pkPartBuffData->m_pusArrayLengths ? pkPartBuffData->m_pusArrayLengths[j] - 2
																		  : pkPartBuffData->m_uiTriCount;
				m_pkD3DDevice9->DrawIndexedPrimitive(ePrimitiveType, pkPartBuffData->m_uiBaseVertexIndex, 0,
					pkPartition->m_usVertices, uiStartIndex, uiPrimitives);
				uiStartIndex += uiPrimitives + 2;
			}
		}
	} else if (pkBuffData->m_pkIB) {
		uint32_t uiStartIndex = 0;
		for (uint32_t i = 0; i < pkBuffData->m_uiNumArrays; ++i) {
			uint32_t uiPrimitives =
				pkBuffData->m_pusArrayLengths ? pkBuffData->m_pusArrayLengths[i] - 2 : pkBuffData->m_uiTriCount;
			m_pkD3DDevice9->DrawIndexedPrimitive(pkBuffData->m_eType, pkBuffData->m_uiBaseVertexIndex, 0,
				pkBuffData->m_uiVertCount, uiStartIndex, uiPrimitives);
			uiStartIndex += uiPrimitives + 2;
		}
	} else {
		m_pkD3DDevice9->DrawPrimitive(pkBuffData->m_eType, pkBuffData->m_uiBaseVertexIndex, pkBuffData->m_uiTriCount);
	}

	pkModelData->m_usDirtyFlags &= 0xF000;
}

// Point sprites: four vertices and two triangles per active particle, drawn once per shader pass. Fully transparent
// alpha-blended points are skipped. A failed index buffer lock releases the buffer and leaves the vertex setup as is.
void NiDX9Renderer::Do_RenderPoints(NiParticles* apPoints) {
	if (m_bDeviceLost)
		return;

	NiPropertyState* pkProps = m_pkCurrProp;
	if ((static_cast<NiAlphaProperty*>(pkProps->m_aspProps[NiProperty::ALPHA].m_pObject)->m_uFlags &
			NiAlphaProperty::ALPHA_BLEND_MASK) &&
		static_cast<NiMaterialProperty*>(pkProps->m_aspProps[NiProperty::MATERIAL].m_pObject)->m_fAlpha <= 0.0f)
		return;

	NiParticlesData* pkData = static_cast<NiParticlesData*>(apPoints->m_spModelData.m_pObject);
	NiTransform kWorld = apPoints->m_kWorld;
	const NiBound* pkBound = apPoints->m_kWorldBound ? apPoints->m_kWorldBound : &NiAVObject::NullBoundS;
	NiPoint3 kCenter = pkBound->m_kCenter;
	float fRadius = pkBound->m_fRadius;
	uint16_t usActive = pkData->GetActiveVertexCount();
	uint32_t uiMaxPoints = pkData->m_usVertices;
	if (!usActive)
		return;

	BSShader* pkShader = apPoints->m_spShader;
	m_pkGeometryGroupManager->AddObjectToGroup(m_pkDynamicGroup, pkData, nullptr, false, 0, 0);
	uint32_t uiVertCount = usActive * 4;
	NiGeometryBufferData* pkBuffData = pkData->m_pkBuffData;
	pkBuffData->SetStreamCount(1);
	pkBuffData->m_uiVertCount = uiVertCount;
	pkBuffData->m_uiMaxVertCount = uiMaxPoints * 4;
	pkBuffData->m_uiTriCount = usActive * 2;
	pkBuffData->m_uiMaxTriCount = uiMaxPoints * 2;
	pkBuffData->m_pusIndexArray = nullptr;
	pkBuffData->m_pusArrayLengths = nullptr;
	pkBuffData->m_uiNumArrays = 1;
	m_kWorldBound.m_kCenter = kCenter;
	m_kWorldBound.m_fRadius = fRadius;
	pkBuffData->m_uiFlags = 0x1C00000;

	IDirect3DIndexBuffer9* pkIB;
	if (pkData->m_ausRenderOrder) {
		// Two triangles per particle in render order, in the geometry's own index buffer.
		pkIB = pkBuffData->m_pkIB;
		uint32_t uiIBSize = uiMaxPoints * 12;
		if (!pkIB) {
			pkIB = m_pkIBManager->GetIndexBuffer(uiIBSize, 0, D3DFMT_INDEX16,
				bUseDirect3D9Ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED);
			pkBuffData->m_pkIB = pkIB;
			pkBuffData->m_uiIndexCount = uiMaxPoints * 6;
			pkBuffData->m_uiIBSize = uiIBSize;
			if (!pkIB)
				return;
		}

		uint16_t* pusIndices = nullptr;
		if (pkIB->Lock(0, usActive * 12, reinterpret_cast<void**>(&pusIndices), 0) < 0) {
			pkIB->Unlock();
			pkIB->Release();
			return;
		}

		const uint16_t* pusOrder = pkData->m_ausRenderOrder;
		for (uint32_t i = 0; i < usActive; ++i) {
			uint16_t usBase = static_cast<uint16_t>(pusOrder[i] * 4);
			pusIndices[0] = usBase;
			pusIndices[1] = static_cast<uint16_t>(usBase + 1);
			pusIndices[2] = static_cast<uint16_t>(usBase + 2);
			pusIndices[3] = usBase;
			pusIndices[4] = static_cast<uint16_t>(usBase + 2);
			pusIndices[5] = static_cast<uint16_t>(usBase + 3);
			pusIndices += 6;
		}
		pkIB->Unlock();
	} else {
		pkIB = m_pkIBManager->ResizeTempPointConnectivity(usActive, uiMaxPoints, false, 0,
			bUseDirect3D9Ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED);
	}
	if (!pkIB)
		return;

	const NiPoint3* pkVertex = pkData->m_pkVertex;
	const NiColorA* pkColor = pkData->m_pkColor;
	const float* pfRadii = pkData->m_pfRadii;
	const float* pfSizes = pkData->m_pfSizes;
	const float* pfRotationAngles = pkData->m_pfRotationAngles;
	const uint8_t* pucTextureIndices = pkData->m_pucTextureIndices;

	m_pkRenderState->SetBoneCount(0);
	NiD3DUtility::GetD3DFromNi(m_kD3DMat, kWorld.m_Rotate, kWorld.m_Translate, kWorld.m_fScale);
	m_pkD3DDevice9->SetTransform(D3DTS_WORLD, &m_kD3DMat);
	m_pkRenderState->vSetModelTransform(&kWorld);
	m_kModelCamRight = m_kCamRight * kWorld.m_Rotate;
	m_kModelCamUp = m_kCamUp * kWorld.m_Rotate;

	NiVBChip* pkChip;
	if (pucTextureIndices) {
		pkBuffData->SetVertexDeclaration(GetDecl_Particles());
		if (pkBuffData->m_uiStreamCount > 0)
			pkBuffData->m_puiVertexStride[0] = 0x28;
		m_pkVBManager->AllocateBufferSpace(pkBuffData, 0);
		pkChip = pkBuffData->m_uiStreamCount > 0 ? pkBuffData->m_ppkVBChip[0] : nullptr;
		if (!pkChip->m_pkVB)
			return;
		void* pvVerts =
			m_pkVBManager->LockVB(pkChip->m_pkVB, pkChip->m_uiOffset, uiVertCount * 0x28, pkChip->m_uiLockFlags);
		if (!pvVerts)
			return;
		PackPoints(pvVerts, pkVertex, pkColor, pfRadii, pfSizes, pfRotationAngles, pucTextureIndices, usActive);
	} else {
		pkBuffData->SetVertexDeclaration(GetDecl_ParticlesSubTexture());
		if (pkBuffData->m_uiStreamCount > 0)
			pkBuffData->m_puiVertexStride[0] = 0x24;
		m_pkVBManager->AllocateBufferSpace(pkBuffData, 0);
		pkChip = pkBuffData->m_uiStreamCount > 0 ? pkBuffData->m_ppkVBChip[0] : nullptr;
		if (!pkChip->m_pkVB)
			return;
		void* pvVerts =
			m_pkVBManager->LockVB(pkChip->m_pkVB, pkChip->m_uiOffset, uiVertCount * 0x24, pkChip->m_uiLockFlags);
		if (!pvVerts)
			return;
		PackPointsSubTexture(pvVerts, pkVertex, pkColor, pfRadii, pfSizes, pfRotationAngles, usActive);
	}
	m_pkVBManager->UnlockVB(pkChip->m_pkVB);

	uint32_t uiPasses = pkShader->FirstPass();
	NiPropertyState* pkState = &apPoints->m_PropertyState;
	pkShader->SetupGeometryTextures(pkState);
	pkShader->SetupGeometryConstants(pkState);
	pkShader->SetupGeometryAlphaBlending(pkState);
	pkShader->SetupGeometryAlphaTesting(pkState);
	pkShader->SetupGeometryRenderStates(pkState, true);
	m_pkRenderState->SetDeclaration(pkBuffData->m_hDeclaration, false);
	m_pkD3DDevice9->SetStreamSource(0, pkChip->m_pkVB, 0,
		pkBuffData->m_uiStreamCount > 0 ? pkBuffData->m_puiVertexStride[0] : 0);
	m_pkD3DDevice9->SetIndices(pkIB);
	if (uiPasses) {
		do {
			m_pkD3DDevice9->DrawIndexedPrimitive(pkBuffData->m_eType, pkBuffData->m_uiBaseVertexIndex, 0,
				pkBuffData->m_uiVertCount, 0, pkBuffData->m_uiTriCount);
		} while (pkShader->NextPass());
	}
	pkShader->PostGeometry(pkState);
}

// Particle strips: one triangle strip through every strip of two or more points, the strips joined by degenerate
// triangles, drawn in one call without shader passes. A failed index buffer lock releases the buffer and returns with
// the vertex buffer still locked, as the game does.
void NiDX9Renderer::Do_RenderParticleStrips(BSStripParticleSystem* apParticles) {
	if (m_bDeviceLost)
		return;

	NiPropertyState* pkProps = m_pkCurrProp;
	if ((static_cast<NiAlphaProperty*>(pkProps->m_aspProps[NiProperty::ALPHA].m_pObject)->m_uFlags &
			NiAlphaProperty::ALPHA_BLEND_MASK) &&
		static_cast<NiMaterialProperty*>(pkProps->m_aspProps[NiProperty::MATERIAL].m_pObject)->m_fAlpha <= 0.0f)
		return;

	BSStripPSysData* pkData = static_cast<BSStripPSysData*>(apParticles->m_spModelData.m_pObject);
	NiTransform kWorld = apParticles->m_kWorld;
	const NiBound* pkBound = apParticles->m_kWorldBound ? apParticles->m_kWorldBound : &NiAVObject::NullBoundS;
	uint32_t uiActive = pkData->m_usActive;
	NiPoint3 kCenter = pkBound->m_kCenter;
	float fRadius = pkBound->m_fRadius;
	const uint8_t* pucTextureIndices = pkData->m_pucTextureIndices;

	// Four vertices per strip point; the index count adds two per joint between strips.
	uint32_t uiVerts = 0;
	uint32_t uiIndices = 0;
	bool bStripStarted = false;
	for (uint32_t i = 0; i < uiActive; ++i) {
		uint16_t usPoints = pkData->GetPointCount(static_cast<uint16_t>(i));
		if (usPoints >= 2) {
			uiVerts += usPoints * 4;
			if (bStripStarted)
				uiIndices += 2;
			else
				bStripStarted = true;
			uiIndices += usPoints * 4 + 2;
		}
	}

	BSShader* pkShader = apParticles->m_spShader;
	if (!uiVerts || !pkShader)
		return;

	m_pkGeometryGroupManager->AddObjectToGroup(m_pkDynamicGroup, pkData, nullptr, false, 0, 0);
	NiGeometryBufferData* pkBuffData = pkData->m_pkBuffData;
	pkBuffData->SetStreamCount(1);
	pkBuffData->m_uiTriCount = uiIndices - 2;
	pkBuffData->m_uiMaxTriCount = uiIndices - 2;
	pkBuffData->m_uiVertCount = uiVerts;
	pkBuffData->m_uiMaxVertCount = uiVerts;
	pkBuffData->m_pusIndexArray = nullptr;
	pkBuffData->m_pusArrayLengths = nullptr;
	pkBuffData->m_uiNumArrays = 1;
	m_kWorldBound.m_kCenter = kCenter;
	m_kWorldBound.m_fRadius = fRadius;
	pkBuffData->m_uiFlags = 0x1C00000;

	m_pkRenderState->SetBoneCount(0);
	m_pkRenderState->vSetModelTransform(&kWorld);

	IDirect3DVertexDeclaration9* hDecl;
	if (pucTextureIndices) {
		if (pkBuffData->m_uiStreamCount > 0)
			pkBuffData->m_puiVertexStride[0] = 0x28;
		m_pkRenderState->SetDeclaration(GetDecl_Particles(), false);
		hDecl = GetDecl_Particles();
	} else {
		if (pkBuffData->m_uiStreamCount > 0)
			pkBuffData->m_puiVertexStride[0] = 0x24;
		m_pkRenderState->SetDeclaration(GetDecl_ParticlesSubTexture(), false);
		hDecl = GetDecl_ParticlesSubTexture();
	}
	pkBuffData->SetVertexDeclaration(hDecl);
	pkBuffData->m_eType = D3DPT_TRIANGLESTRIP;

	pkShader->FirstPass();
	pkShader->SetupGeometryTextures(m_pkCurrProp);
	pkShader->SetupGeometryConstants(m_pkCurrProp);
	pkShader->SetupGeometryAlphaBlending(m_pkCurrProp);
	pkShader->SetupGeometryAlphaTesting(m_pkCurrProp);
	pkShader->SetupGeometryRenderStates(m_pkCurrProp, true);

	m_pkVBManager->AllocateBufferSpace(pkBuffData, 0);
	NiVBChip* pkChip = pkBuffData->m_uiStreamCount > 0 ? pkBuffData->m_ppkVBChip[0] : nullptr;
	if (!pkChip->m_pkVB)
		return;
	void* pvVerts = m_pkVBManager->LockVB(pkChip->m_pkVB, pkChip->m_uiOffset,
		pucTextureIndices ? uiVerts * 0x28 : uiVerts * 0x24, pkChip->m_uiLockFlags);
	if (!pvVerts)
		return;

	uint32_t uiIBSize = uiIndices * 2;
	IDirect3DIndexBuffer9* pkIB = pkBuffData->m_pkIB;
	if (!pkIB) {
		// Sized for every particle at the maximum strip length.
		uint32_t uiMaxIndices = 0;
		bool bMaxStarted = false;
		for (uint32_t i = pkData->m_usVertices; i; --i) {
			uint16_t usMaxPoints = pkData->GetMaxPointCount();
			if (usMaxPoints >= 2) {
				if (bMaxStarted)
					uiMaxIndices += 2;
				else
					bMaxStarted = true;
				uiMaxIndices += usMaxPoints * 4 + 2;
			}
		}
		pkIB = m_pkIBManager->GetIndexBuffer(uiMaxIndices * 2, 0, D3DFMT_INDEX16,
			bUseDirect3D9Ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED);
		pkBuffData->m_pkIB = pkIB;
		pkBuffData->m_uiIndexCount = uiMaxIndices;
		pkBuffData->m_uiIBSize = uiMaxIndices * 2;
	}

	uint32_t uiPackedVerts = 0;
	uint32_t uiPackedIndices = 0;
	if (!pkIB) {
		m_pkVBManager->UnlockVB(pkChip->m_pkVB);
		return;
	}

	uint16_t* pusIndices = nullptr;
	if (pkIB->Lock(0, uiIBSize, reinterpret_cast<void**>(&pusIndices), 0) < 0) {
		pkIB->Unlock();
		pkIB->Release();
		return;
	}

	PackParticleStrips(pvVerts, pusIndices, pkData, &uiPackedVerts, &uiPackedIndices);
	m_pkVBManager->UnlockVB(pkChip->m_pkVB);
	pkIB->Unlock();
	if (uiPackedVerts < 1)
		return;

	pkBuffData->m_uiVertCount = uiPackedVerts;
	pkBuffData->m_uiMaxVertCount = uiPackedVerts;
	pkBuffData->m_uiTriCount = uiPackedIndices - 2;
	pkBuffData->m_uiMaxTriCount = uiPackedIndices - 2;
	pkBuffData->m_pusIndexArray = nullptr;
	pkBuffData->m_pusArrayLengths = nullptr;
	pkBuffData->m_uiNumArrays = 1;
	m_pkD3DDevice9->SetStreamSource(0, pkChip->m_pkVB, 0,
		pkBuffData->m_uiStreamCount > 0 ? pkBuffData->m_puiVertexStride[0] : 0);
	m_pkD3DDevice9->SetIndices(pkIB);
	m_pkD3DDevice9->DrawIndexedPrimitive(pkBuffData->m_eType, pkBuffData->m_uiBaseVertexIndex, 0,
		pkBuffData->m_uiVertCount, 0, pkBuffData->m_uiTriCount);
}

// Lines: the positions (plus packed colors and texture coordinates when present) go to a dynamic vertex buffer and
// the shared line-list connectivity built from the per-point flags is drawn once per shader pass.
void NiDX9Renderer::Do_RenderLines(NiLines* apLines) {
	if (m_bDeviceLost)
		return;

	NiLinesData* pkData = static_cast<NiLinesData*>(apLines->m_spModelData.m_pObject);
	uint32_t uiVertices = pkData->m_usVertices;
	if (!uiVertices)
		return;

	const NiBound* pkBound = apLines->m_kWorldBound ? apLines->m_kWorldBound : &NiAVObject::NullBoundS;
	NiPoint3 kCenter = pkBound->m_kCenter;
	float fRadius = pkBound->m_fRadius;
	m_pkGeometryGroupManager->AddObjectToGroup(m_pkDynamicGroup, pkData, nullptr, false, 0, 0);
	m_kWorldBound.m_fRadius = fRadius;
	m_kWorldBound.m_kCenter = kCenter;

	bool* pbFlags = reinterpret_cast<bool*>(pkData->m_pkFlags);
	const NiPoint3* pkVertex = pkData->m_pkVertex;
	const NiPoint2* pkTexture = pkData->m_pkTexture;
	const NiColorA* pkColor = pkData->m_pkColor;
	uint32_t uiConn = 0;
	IDirect3DIndexBuffer9* pkIB = m_pkIBManager->ResizeTempLineConnectivity(uiVertices, pbFlags, &uiConn, 0,
		bUseDirect3D9Ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED);
	if (!pkIB || !uiConn)
		return;

	uint32_t uiFVF = D3DFVF_XYZ;
	uint32_t uiStride = 0xC;
	if (pkTexture) {
		uiFVF = D3DFVF_XYZ | D3DFVF_TEX1;
		uiStride = 0x14;
	}
	if (pkColor) {
		uiFVF |= D3DFVF_DIFFUSE;
		uiStride += 4;
	}

	BSShader* pkShader = apLines->m_spShader;
	NiGeometryBufferData* pkBuffData = pkData->m_pkBuffData;
	pkBuffData->SetStreamCount(1);
	pkBuffData->m_uiVertCount = uiVertices;
	pkBuffData->m_uiMaxVertCount = uiVertices;
	pkBuffData->SetFVF(uiFVF);
	if (pkBuffData->m_uiStreamCount > 0)
		pkBuffData->m_puiVertexStride[0] = uiStride;
	m_pkVBManager->AllocateBufferSpace(pkBuffData, 0);
	NiVBChip* pkChip = pkBuffData->m_uiStreamCount > 0 ? pkBuffData->m_ppkVBChip[0] : nullptr;
	m_pkRenderState->SetFVF(pkBuffData->m_uiFVF, false);
	if (!pkChip->m_pkVB)
		return;
	void* pvVerts = m_pkVBManager->LockVB(pkChip->m_pkVB, pkChip->m_uiOffset, pkChip->m_uiSize, pkChip->m_uiLockFlags);
	if (!pvVerts)
		return;

	// Vertex layout per D3DFVF order: position, then the D3DCOLOR (A8R8G8B8), then the texture coordinates.
	float* pfDest = static_cast<float*>(pvVerts);
	for (uint32_t i = 0; i < uiVertices; ++i) {
		pfDest[0] = pkVertex->x;
		pfDest[1] = pkVertex->y;
		pfDest[2] = pkVertex->z;
		pfDest += 3;
		++pkVertex;
		if (pkColor) {
			uint32_t uiA = static_cast<uint32_t>(NiFastFloatToInt(static_cast<float>(pkColor->a * 255.0)));
			uint32_t uiR = static_cast<uint32_t>(NiFastFloatToInt(static_cast<float>(pkColor->r * 255.0)));
			uint32_t uiG = static_cast<uint32_t>(NiFastFloatToInt(static_cast<float>(pkColor->g * 255.0)));
			uint32_t uiB = static_cast<uint32_t>(NiFastFloatToInt(static_cast<float>(pkColor->b * 255.0)));
			*reinterpret_cast<uint32_t*>(pfDest) = (((uiA << 8 | uiR) << 8 | uiG) << 8) | uiB;
			pfDest += 1;
			++pkColor;
		}
		if (pkTexture) {
			pfDest[0] = pkTexture->x;
			pfDest[1] = pkTexture->y;
			pfDest += 2;
			++pkTexture;
		}
	}
	m_pkVBManager->UnlockVB(pkChip->m_pkVB);

	pkShader->SetupGeometryTextures(m_pkCurrProp);
	pkShader->SetupGeometryConstants(m_pkCurrProp);
	pkShader->SetupGeometryAlphaBlending(m_pkCurrProp);
	pkShader->SetupGeometryAlphaTesting(m_pkCurrProp);
	pkShader->SetupGeometryRenderStates(m_pkCurrProp, true);
	uint32_t uiPasses = pkShader->FirstPass();
	m_pkD3DDevice9->SetStreamSource(0, pkChip->m_pkVB, 0,
		pkBuffData->m_uiStreamCount > 0 ? pkBuffData->m_puiVertexStride[0] : 0);
	m_pkD3DDevice9->SetIndices(pkIB);
	if (uiPasses) {
		do {
			m_pkD3DDevice9->DrawIndexedPrimitive(D3DPT_LINELIST, pkBuffData->m_uiBaseVertexIndex, 0,
				pkBuffData->m_uiVertCount, 0, uiConn >> 1);
		} while (pkShader->NextPass());
	}
	pkShader->PostGeometry(m_pkCurrProp);
}

// Hardware skinned: geometry without model buffer data whose skin has a partition, when the first partition already
// has buffer data or the renderer forces it (flag 2); else when the shader (apShader, or the geometry's) has vertex
// shaders in all passes on a vertex shader 2.0+ device, or, on a device that can switch to software vertex processing,
// when the shader's declaration asks for software vertex processing.
bool NiDX9Renderer::IsHardwareSkinned(const NiGeometry* apGeometry, BSShader* apShader) {
	if (apGeometry->m_spModelData.m_pObject->m_pkBuffData)
		return false;
	NiSkinInstance* pkSkin = apGeometry->m_spSkinInstance.m_pObject;
	if (!pkSkin)
		return false;
	NiSkinPartition* pkSkinPartition = pkSkin->m_spSkinPartition.m_pObject;
	if (!pkSkinPartition)
		return false;
	if (pkSkinPartition->m_pkPartitions->m_pkBuffData || (GetFlags() & 2))
		return true;

	BSShader* pkShader = apShader ? apShader : apGeometry->m_spShader;
	if (m_uiMaxVertexShaderVersion > 0xFFFE0000 && pkShader && pkShader->GetVSPresentAllPasses())
		return true;
	if (!m_bSWVertexSwitchable || !pkShader || !pkShader->GetShaderDecl())
		return false;
	return pkShader->GetShaderDecl()->GetSoftwareVertexProcessing();
}

// GetDecl_ParticlesSubTexture's layout plus the texture index as texture coordinate 1, the layout PackPoints writes.
IDirect3DVertexDeclaration9* NiDX9Renderer::GetDecl_Particles() {
	IDirect3DVertexDeclaration9*& rhDecl = hParticleVertexDecls[1];
	if (!rhDecl) {
		const D3DVERTEXELEMENT9 akDecl[] = {
			{0, 0x00, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
			{0, 0x0C, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
			{0, 0x18, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
			{0, 0x1C, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
			{0, 0x24, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
			// D3DDECL_END()
			{0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0},
		};
		if (m_pkD3DDevice9->CreateVertexDeclaration(akDecl, &rhDecl) < 0) {
			rhDecl = nullptr;
			return nullptr;
		}
	}
	return rhDecl;
}

// Position, normal, color and texture coordinate 0, the layout PackPointsSubTexture writes.
IDirect3DVertexDeclaration9* NiDX9Renderer::GetDecl_ParticlesSubTexture() {
	IDirect3DVertexDeclaration9*& rhDecl = hParticleVertexDecls[0];
	if (!rhDecl) {
		const D3DVERTEXELEMENT9 akDecl[] = {
			{0, 0x00, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
			{0, 0x0C, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
			{0, 0x18, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
			{0, 0x1C, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
			// D3DDECL_END()
			{0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0},
		};
		if (m_pkD3DDevice9->CreateVertexDeclaration(akDecl, &rhDecl) < 0) {
			rhDecl = nullptr;
			return nullptr;
		}
	}
	return rhDecl;
}

// PackPointsSubTexture with the point's texture index (as a float) after each vertex.
void NiDX9Renderer::PackPoints(void* apVerts, const NiPoint3* apPositions, const NiColorA* apColors,
	const float* apRadii, const float* apSizes, const float* apRotationAngles, const uint8_t* apTextureIndices,
	uint16_t ausCount) {
	NiPoint3 kNormal;
	kNormal.x = m_kModelCamRight.y * m_kModelCamUp.z - m_kModelCamRight.z * m_kModelCamUp.y;
	kNormal.y = m_kModelCamRight.z * m_kModelCamUp.x - m_kModelCamRight.x * m_kModelCamUp.z;
	kNormal.z = m_kModelCamRight.x * m_kModelCamUp.y - m_kModelCamRight.y * m_kModelCamUp.x;

	NiPoint3 kUpMinusRight;
	NiPoint3 kUpPlusRight;
	if (!apRotationAngles) {
		kUpMinusRight.x = m_kModelCamUp.x - m_kModelCamRight.x;
		kUpMinusRight.y = m_kModelCamUp.y - m_kModelCamRight.y;
		kUpMinusRight.z = m_kModelCamUp.z - m_kModelCamRight.z;
		kUpPlusRight.x = m_kModelCamUp.x + m_kModelCamRight.x;
		kUpPlusRight.y = m_kModelCamUp.y + m_kModelCamRight.y;
		kUpPlusRight.z = m_kModelCamUp.z + m_kModelCamRight.z;
	}

	float* pfVert = static_cast<float*>(apVerts);
	for (uint16_t i = 0; i < ausCount; ++i) {
		uint32_t uiColor = apColors ? PackPointColor(apColors[i]) : 0xFFFFFFFF;
		float fSize = apRadii[i] * apSizes[i];
		NiPoint3 kCorner0;
		NiPoint3 kCorner1;
		if (apRotationAngles) {
			GetRotatedPointCorners(m_kModelCamRight, m_kModelCamUp, fSize, apRotationAngles[i], kCorner0, kCorner1);
		} else {
			kCorner0.x = kUpPlusRight.x * fSize;
			kCorner0.y = kUpPlusRight.y * fSize;
			kCorner0.z = kUpPlusRight.z * fSize;
			kCorner1.x = kUpMinusRight.x * fSize;
			kCorner1.y = kUpMinusRight.y * fSize;
			kCorner1.z = kUpMinusRight.z * fSize;
		}

		const NiPoint3& kPos = apPositions[i];
		PackPointVertex(pfVert, kPos.x - kCorner0.x, kPos.y - kCorner0.y, kPos.z - kCorner0.z, kNormal, uiColor, 0.0f,
			1.0f);
		pfVert[9] = apTextureIndices[i];
		PackPointVertex(pfVert + 10, kPos.x - kCorner1.x, kPos.y - kCorner1.y, kPos.z - kCorner1.z, kNormal, uiColor,
			1.0f, 1.0f);
		pfVert[19] = apTextureIndices[i];
		PackPointVertex(pfVert + 20, kPos.x + kCorner0.x, kPos.y + kCorner0.y, kPos.z + kCorner0.z, kNormal, uiColor,
			1.0f, 0.0f);
		pfVert[29] = apTextureIndices[i];
		PackPointVertex(pfVert + 30, kPos.x + kCorner1.x, kPos.y + kCorner1.y, kPos.z + kCorner1.z, kNormal, uiColor,
			0.0f, 0.0f);
		pfVert[39] = apTextureIndices[i];
		pfVert += 40;
	}
}

// Four vertices per point, facing the camera: position - corner 0 (uv 0,1), position - corner 1 (1,1),
// position + corner 0 (1,0) and position + corner 1 (0,0), all with the normal right x up and the point's color
// (white without colors). The corners span radius * size along up + right and up - right, turned by the rotation
// angle when there are angles.
void NiDX9Renderer::PackPointsSubTexture(void* apVerts, const NiPoint3* apPositions, const NiColorA* apColors,
	const float* apRadii, const float* apSizes, const float* apRotationAngles, uint16_t ausCount) {
	NiPoint3 kNormal;
	kNormal.x = m_kModelCamRight.y * m_kModelCamUp.z - m_kModelCamRight.z * m_kModelCamUp.y;
	kNormal.y = m_kModelCamRight.z * m_kModelCamUp.x - m_kModelCamRight.x * m_kModelCamUp.z;
	kNormal.z = m_kModelCamRight.x * m_kModelCamUp.y - m_kModelCamRight.y * m_kModelCamUp.x;

	NiPoint3 kUpMinusRight;
	NiPoint3 kUpPlusRight;
	if (!apRotationAngles) {
		kUpMinusRight.x = m_kModelCamUp.x - m_kModelCamRight.x;
		kUpMinusRight.y = m_kModelCamUp.y - m_kModelCamRight.y;
		kUpMinusRight.z = m_kModelCamUp.z - m_kModelCamRight.z;
		kUpPlusRight.x = m_kModelCamUp.x + m_kModelCamRight.x;
		kUpPlusRight.y = m_kModelCamUp.y + m_kModelCamRight.y;
		kUpPlusRight.z = m_kModelCamUp.z + m_kModelCamRight.z;
	}

	float* pfVert = static_cast<float*>(apVerts);
	for (uint16_t i = 0; i < ausCount; ++i) {
		uint32_t uiColor = apColors ? PackPointColor(apColors[i]) : 0xFFFFFFFF;
		float fSize = apRadii[i] * apSizes[i];
		NiPoint3 kCorner0;
		NiPoint3 kCorner1;
		if (apRotationAngles) {
			GetRotatedPointCorners(m_kModelCamRight, m_kModelCamUp, fSize, apRotationAngles[i], kCorner0, kCorner1);
		} else {
			kCorner0.x = kUpPlusRight.x * fSize;
			kCorner0.y = kUpPlusRight.y * fSize;
			kCorner0.z = kUpPlusRight.z * fSize;
			kCorner1.x = kUpMinusRight.x * fSize;
			kCorner1.y = kUpMinusRight.y * fSize;
			kCorner1.z = kUpMinusRight.z * fSize;
		}

		const NiPoint3& kPos = apPositions[i];
		PackPointVertex(pfVert, kPos.x - kCorner0.x, kPos.y - kCorner0.y, kPos.z - kCorner0.z, kNormal, uiColor, 0.0f,
			1.0f);
		PackPointVertex(pfVert + 9, kPos.x - kCorner1.x, kPos.y - kCorner1.y, kPos.z - kCorner1.z, kNormal, uiColor,
			1.0f, 1.0f);
		PackPointVertex(pfVert + 18, kPos.x + kCorner0.x, kPos.y + kCorner0.y, kPos.z + kCorner0.z, kNormal, uiColor,
			1.0f, 0.0f);
		PackPointVertex(pfVert + 27, kPos.x + kCorner1.x, kPos.y + kCorner1.y, kPos.z + kCorner1.z, kNormal, uiColor,
			0.0f, 0.0f);
		pfVert += 36;
	}
}

// One triangle strip through every strip of two or more points: per point, its four vertices (u advancing by the
// strip's texture coordinate step) and the indices of vertices 0 and 1, then the indices of vertex 1 of the last
// point and vertex 2 of the first point, then of vertices 2 and 3 of every point. Consecutive strips are joined by
// two repeated indices.
// *apuiVerts receives the vertex count, *apuiIndices the index count.
void NiDX9Renderer::PackParticleStrips(void* apVerts, uint16_t* apusIndices, BSStripPSysData* apData,
	uint32_t* apuiVerts, uint32_t* apuiIndices) {
	const uint8_t* pucTextureIndices = apData->m_pucTextureIndices;
	uint16_t usActive = apData->m_usActive;
	uint32_t uiVerts = 0;
	uint32_t uiExtraIndices = 0;
	uint32_t uiVertex = 0;
	bool bJoin = false;
	for (uint16_t i = 0; i < usActive; ++i) {
		uint16_t usPoints = apData->GetPointCount(i);
		if (usPoints < 2)
			continue;

		if (bJoin) {
			*apusIndices++ = static_cast<uint16_t>(uiVertex - 1);
			*apusIndices++ = static_cast<uint16_t>(uiVertex);
			uiExtraIndices += 2;
		} else {
			bJoin = true;
		}

		apData->GetMaxPointCount();
		apData->GetTexCoordDelta(i);
		float fU = 0.0f;
		float fUDelta = apData->GetTexCoordDelta(i);
		uint16_t usBack = static_cast<uint16_t>(uiVertex + 2);
		uint16_t usPoint = static_cast<uint16_t>(apData->pStripList[i].iFirstIndex);
		uiVerts += usPoints * 4;
		for (uint16_t j = 0; j < usPoints; ++j) {
			const NiPoint3* pkVerts = apData->GetPointVerts(i, usPoint);
			const NiPoint3* pkNorms = apData->GetPointNorms(i, usPoint);
			int32_t iA = NiFastFloatToInt(apData->GetPointColor(i, usPoint)->a * 255.0);
			int32_t iR = NiFastFloatToInt(apData->GetPointColor(i, usPoint)->r * 255.0);
			int32_t iG = NiFastFloatToInt(apData->GetPointColor(i, usPoint)->g * 255.0);
			int32_t iB = NiFastFloatToInt(apData->GetPointColor(i, usPoint)->b * 255.0);
			uint32_t uiColor = (((static_cast<uint32_t>(iA) << 8 | iR) << 8 | iG) << 8) | iB;
			float fTextureIndex = pucTextureIndices ? pucTextureIndices[i] : -1.0f;
			apVerts = PackStripParticleVertex(apVerts, &pkVerts[0], &pkNorms[0], uiColor, fU, 0.0f, fTextureIndex);
			apVerts = PackStripParticleVertex(apVerts, &pkVerts[1], &pkNorms[0], uiColor, fU, 1.0f, fTextureIndex);
			apVerts = PackStripParticleVertex(apVerts, &pkVerts[2], &pkNorms[1], uiColor, fU, 0.0f, fTextureIndex);
			apVerts = PackStripParticleVertex(apVerts, &pkVerts[3], &pkNorms[1], uiColor, fU, 1.0f, fTextureIndex);
			fU = fUDelta + fU;
			*apusIndices++ = static_cast<uint16_t>(uiVertex);
			*apusIndices++ = static_cast<uint16_t>(uiVertex + 1);
			uiVertex += 4;
			usPoint = static_cast<uint16_t>(apData->pStripList[i].NormalizeIndex(usPoint + 1));
		}

		uiExtraIndices += 2;
		*apusIndices++ = static_cast<uint16_t>(uiVertex - 3);
		*apusIndices++ = usBack;
		for (uint16_t j = 0; j < usPoints; ++j) {
			*apusIndices++ = usBack;
			*apusIndices++ = static_cast<uint16_t>(usBack + 1);
			usBack += 4;
		}
	}
	*apuiVerts = uiVerts;
	*apuiIndices = uiExtraIndices + uiVerts;
}

// Draws apGeometry through its shader's passes: per pass the geometry is prepared, then drawn per index array (per
// enabled segment of a segmented shape; a resizable shape's primitive count replaces the triangle count), or
// unindexed. A nonzero PreProcessPipeline skips the draw. apData is not read.
void NiDX9Renderer::DrawPrimitive(NiGeometry* apGeometry, NiGeometryData*, NiSkinInstance* apSkinInstance,
	const NiTransform& arWorld, const NiBound& arWorldBound, NiGeometryBufferData* apBuffData) {
	BSShader* pkShader = apGeometry ? apGeometry->m_spShader : nullptr;
	if (pkShader->PreProcessPipeline(apGeometry, apSkinInstance, apBuffData, m_pkCurrProp, m_pkCurrEffects, arWorld,
			arWorldBound))
		return;
	pkShader->UpdatePipeline(apGeometry, apSkinInstance, apBuffData, m_pkCurrProp, m_pkCurrEffects, arWorld,
		arWorldBound);

	NiGeometryBufferData* pkBuffData = apBuffData;
	if (pkShader->FirstPass()) {
		do {
			pkShader->SetupRenderingPass(apGeometry, apSkinInstance, pkBuffData, m_pkCurrProp, m_pkCurrEffects, arWorld,
				arWorldBound);
			pkShader->SetupTransformations(apGeometry, apSkinInstance, nullptr, pkBuffData, m_pkCurrProp,
				m_pkCurrEffects, arWorld, arWorldBound);
			pkBuffData = static_cast<NiGeometryBufferData*>(pkShader->PrepareGeometryForRendering(apGeometry, nullptr,
				pkBuffData, m_pkCurrProp));
			pkShader->SetupShaderPrograms(apGeometry, apSkinInstance, nullptr, pkBuffData, m_pkCurrProp,
				m_pkCurrEffects, arWorld, arWorldBound);

			if (pkBuffData->m_pkIB) {
				uint32_t uiStartIndex = 0;
				for (uint32_t i = 0; i < pkBuffData->m_uiNumArrays; ++i) {
					uint32_t uiPrimitives;
					if (pkBuffData->m_pusArrayLengths) {
						uiPrimitives = pkBuffData->m_pusArrayLengths[i] - 2;
					} else {
						uiPrimitives = pkBuffData->m_uiTriCount;
						if (apGeometry->IsResizableTriShape())
							uiPrimitives = apGeometry->IsResizableTriShape()->uiNumPrimitives;
					}

					if (apGeometry->IsSegmentedTriShape()) {
						// One draw per enabled segment; with bIgnoreSegments, one draw of the whole shape.
						BSSegmentedTriShape* pkSegmented = apGeometry->IsSegmentedTriShape();
						uint32_t uiSegments = pkSegmented->bIgnoreSegments ? 1 : pkSegmented->iNumSegments;
						for (uint32_t j = 0; j < uiSegments; ++j) {
							BSSegmentedTriShape::Segment* pkSegment = &pkSegmented->pSegments[j];
							if (pkSegment->bEnabledPostMerge) {
								bool bIgnoreSegments = pkSegmented->bIgnoreSegments;
								uint32_t uiSegmentPrimitives = bIgnoreSegments
									? static_cast<NiTriBasedGeomData*>(pkSegmented->m_spModelData.m_pObject)
										  ->m_usTriangles
									: pkSegment->iNumPrimitivesMerged;
								uint32_t uiSegmentStart = bIgnoreSegments ? 0 : pkSegment->iStartIndex;
								m_pkD3DDevice9->DrawIndexedPrimitive(pkBuffData->m_eType,
									pkBuffData->m_uiBaseVertexIndex, 0, pkBuffData->m_uiVertCount, uiSegmentStart,
									uiSegmentPrimitives);
							}
						}
					} else {
						m_pkD3DDevice9->DrawIndexedPrimitive(pkBuffData->m_eType, pkBuffData->m_uiBaseVertexIndex, 0,
							pkBuffData->m_uiVertCount, uiStartIndex, uiPrimitives);
					}
					uiStartIndex += uiPrimitives + 2;
				}
			} else {
				m_pkD3DDevice9->DrawPrimitive(pkBuffData->m_eType, pkBuffData->m_uiBaseVertexIndex,
					pkBuffData->m_uiTriCount);
			}

			pkShader->PostRender(apGeometry, apSkinInstance, nullptr, pkBuffData, m_pkCurrProp, m_pkCurrEffects,
				arWorld, arWorldBound);
		} while (pkShader->NextPass());
	}

	pkShader->PostProcessPipeline(apGeometry, apSkinInstance, pkBuffData, m_pkCurrProp, m_pkCurrEffects, arWorld,
		arWorldBound);
	m_pkRenderState->SetInternalNormalizeNormals(false);
}

// Draws the skin partitions through the shader's passes. Per pass, the first drawn partition sets up the
// transformations and shader programs, later ones only their bone matrices. With dismemberment data, disabled
// partitions are skipped and the partitions that follow without a state change are drawn under the same setup. A
// partition the shader cannot prepare ends the call (without the post-processing). The data's dirty flags are cleared
// after each pass.
void NiDX9Renderer::DrawSkinnedPrimitive(NiGeometry* apGeometry, NiGeometryData* apData, NiSkinInstance* apSkinInstance,
	const NiTransform& arWorld, const NiBound& arWorldBound) {
	if (!apSkinInstance)
		return;

	NiSkinPartition* pkSkinPartition = apSkinInstance->m_spSkinPartition.m_pObject;
	NiSkinPartition::Partition* pkPartitions = pkSkinPartition->m_pkPartitions;
	uint32_t ePrimitiveType = pkPartitions->m_usStrips ? D3DPT_TRIANGLESTRIP : D3DPT_TRIANGLELIST;
	BSShader* pkShader = apGeometry ? apGeometry->m_spShader : nullptr;
	NiGeometryBufferData* pkBuffData = pkPartitions->m_pkBuffData;
	if (pkShader->PreProcessPipeline(apGeometry, apSkinInstance, pkBuffData, m_pkCurrProp, m_pkCurrEffects, arWorld,
			arWorldBound))
		return;
	pkShader->UpdatePipeline(apGeometry, apSkinInstance, pkBuffData, m_pkCurrProp, m_pkCurrEffects, arWorld,
		arWorldBound);

	uint32_t uiPartitions = pkSkinPartition->m_uiPartitions;
	uint32_t uiPasses = pkShader->FirstPass();
	BSDismemberSkinInstance::PartitionData* pkPartitionData = nullptr;
	if (apSkinInstance->GetRTTI() == &BSDismemberSkinInstance::ms_RTTI)
		pkPartitionData = static_cast<BSDismemberSkinInstance*>(apSkinInstance)->pPartitionDataArray;

	if (uiPasses) {
		do {
			pkShader->SetupRenderingPass(apGeometry, apSkinInstance, pkBuffData, m_pkCurrProp, m_pkCurrEffects, arWorld,
				arWorldBound);
			m_kWorldBound.m_kCenter = arWorldBound.m_kCenter;
			m_kWorldBound.m_fRadius = arWorldBound.m_fRadius;

			bool bFirst = true;
			for (uint32_t i = 0; i < uiPartitions; ++i) {
				if (pkPartitionData && !pkPartitionData[i].bEnabled)
					continue;

				NiSkinPartition::Partition* pkPartition = &pkPartitions[i];
				pkBuffData = pkPartition->m_pkBuffData;
				if (bFirst) {
					bFirst = false;
					pkShader->SetupTransformations(apGeometry, apSkinInstance, pkPartition, pkBuffData, m_pkCurrProp,
						m_pkCurrEffects, arWorld, arWorldBound);
					pkBuffData = static_cast<NiGeometryBufferData*>(pkShader->PrepareGeometryForRendering(apGeometry,
						pkPartition, pkBuffData, m_pkCurrProp));
					if (!pkBuffData)
						return;
					pkShader->SetupShaderPrograms(apGeometry, apSkinInstance, pkPartition, pkBuffData, m_pkCurrProp,
						m_pkCurrEffects, arWorld, arWorldBound);
				} else {
					pkBuffData = static_cast<NiGeometryBufferData*>(pkShader->PrepareGeometryForRendering(apGeometry,
						pkPartition, pkBuffData, m_pkCurrProp));
					if (!pkBuffData)
						return;
					pkShader->SetupBoneMatrices(apGeometry, apSkinInstance, pkPartition, pkBuffData, m_pkCurrProp,
						m_pkCurrEffects, arWorld, arWorldBound);
				}

				uint32_t uiStartIndex = 0;
				for (uint32_t j = 0; j < pkBuffData->m_uiNumArrays; ++j) {
					uint32_t uiPrimitives =
						pkBuffData->m_pusArrayLengths ? pkBuffData->m_pusArrayLengths[j] - 2 : pkBuffData->m_uiTriCount;
					m_pkD3DDevice9->DrawIndexedPrimitive(ePrimitiveType, pkBuffData->m_uiBaseVertexIndex, 0,
						pkPartition->m_usVertices, uiStartIndex, uiPrimitives);
					uiStartIndex += uiPrimitives + 2;
				}

				if (pkPartitionData) {
					NiSkinPartition::Partition* pkNext = pkPartition;
					while (i + 1 < uiPartitions && !pkPartitionData[i + 1].bStateChange) {
						++i;
						++pkNext;
						if (!pkPartitionData[i].bEnabled)
							continue;

						pkPartition = pkNext;
						pkBuffData =
							static_cast<NiGeometryBufferData*>(pkShader->PrepareGeometryForRendering(apGeometry,
								pkPartition, pkPartition->m_pkBuffData, m_pkCurrProp));
						if (!pkBuffData)
							return;
						uiStartIndex = 0;
						for (uint32_t j = 0; j < pkBuffData->m_uiNumArrays; ++j) {
							uint32_t uiPrimitives = pkBuffData->m_pusArrayLengths ? pkBuffData->m_pusArrayLengths[j] - 2
																				  : pkBuffData->m_uiTriCount;
							m_pkD3DDevice9->DrawIndexedPrimitive(ePrimitiveType, pkBuffData->m_uiBaseVertexIndex, 0,
								pkPartition->m_usVertices, uiStartIndex, uiPrimitives);
							uiStartIndex += uiPrimitives + 2;
						}
					}
				}

				pkShader->PostRender(apGeometry, apSkinInstance, pkPartition, pkBuffData, m_pkCurrProp, m_pkCurrEffects,
					arWorld, arWorldBound);
			}

			apData->m_usDirtyFlags &= 0xF000;
		} while (pkShader->NextPass());
	}

	pkShader->PostProcessPipeline(apGeometry, apSkinInstance, pkBuffData, m_pkCurrProp, m_pkCurrEffects, arWorld,
		arWorldBound);
	m_pkRenderState->SetInternalNormalizeNormals(false);
}

// One indexed draw per array of the partition's buffer: strips of length - 2 triangles, else the triangle count.
void NiDX9Renderer::Do_DrawPartition(NiGeometryBufferData* apBuffData, const NiSkinPartition::Partition* apPartition,
	NiGeometryData*) {
	uint32_t uiStartIndex = 0;
	for (uint32_t i = 0; i < apBuffData->m_uiNumArrays; ++i) {
		uint32_t uiPrimitives =
			apBuffData->m_pusArrayLengths ? apBuffData->m_pusArrayLengths[i] - 2 : apBuffData->m_uiTriCount;
		m_pkD3DDevice9->DrawIndexedPrimitive(apPartition->m_usStrips ? D3DPT_TRIANGLESTRIP : D3DPT_TRIANGLELIST,
			apBuffData->m_uiBaseVertexIndex, 0, apPartition->m_usVertices, uiStartIndex, uiPrimitives);
		uiStartIndex += uiPrimitives + 2;
	}
}

// Once per frame (or when the register count changes): the skin's bone matrices, from bone world space through the
// skin's root parent to akWorld (bone space only with palettized bones), as 4x4 Direct3D matrices (transposed when
// asked) or as 3x4 rows with the camera offset NiRenderer::PosAdjust taken off the translation. The buffer grows,
// 16-byte aligned, as needed. Normal renormalization is requested when a scale is below 0.99 and above 1.01 at once,
// which never holds, so it is turned off.
void NiDX9Renderer::CalculateBoneMatrices(const NiSkinInstance* apSkinInstance, const NiTransform& akWorld,
	bool abTranspose, uint32_t auiBoneMatrixRegisters, bool abPalettizedBones) {
	NiSkinInstance* pkSkin = const_cast<NiSkinInstance*>(apSkinInstance);
	uint32_t uiSkinFrameID = pkSkin->m_uiFrameID;
	if (uiSkinFrameID == m_pkVBManager->GetFrameID() && auiBoneMatrixRegisters == pkSkin->m_uiNumRegisters)
		return;

	pkSkin->m_uiFrameID = m_pkVBManager->GetFrameID();
	NiSkinData* pkSkinData = pkSkin->m_spSkinData.m_pObject;
	uint32_t uiBones = pkSkinData->m_uiBones;
	void* pvBoneMatrices = pkSkin->m_pvBoneMatrices;
	if (auiBoneMatrixRegisters == 3)
		abTranspose = true;

	uint32_t uiSize = uiBones * auiBoneMatrixRegisters * 16;
	if (uiSize > pkSkin->m_uiAllocatedSize) {
		_NiAlignedFree(pvBoneMatrices);
		pvBoneMatrices = _NiAlignedMalloc(uiSize, 16);
		pkSkin->m_pvBoneMatrices = pvBoneMatrices;
		pkSkin->m_uiAllocatedSize = uiSize;
	}
	pkSkin->m_uiNumMatrices = uiBones;
	pkSkin->m_uiNumRegisters = auiBoneMatrixRegisters;

	D3DMATRIX* pkMatrices = static_cast<D3DMATRIX*>(pvBoneMatrices);
	if (auiBoneMatrixRegisters == 4) {
		for (uint32_t i = 0; i < uiBones; ++i) {
			if (abTranspose) {
				pkMatrices[i]._43 = 0.0f;
				pkMatrices[i]._42 = 0.0f;
				pkMatrices[i]._41 = 0.0f;
			} else {
				pkMatrices[i]._34 = 0.0f;
				pkMatrices[i]._24 = 0.0f;
				pkMatrices[i]._14 = 0.0f;
			}
			pkMatrices[i]._44 = 1.0f;
		}
	}

	NiTransform kTransform;
	pkSkin->m_pkRootParent->m_kWorld.Invert(kTransform);
	kTransform = pkSkin->m_spSkinData.m_pObject->m_kRootParentToSkin * kTransform;
	NiTransform kSkinToWorld = akWorld * kTransform;
	bool bNormalize = false;
	if (kTransform.m_fScale < 0.9900000095367432 && kTransform.m_fScale > 1.01f)
		bNormalize = true;

	NiAVObject** ppkBones = pkSkin->m_ppkBones;
	NiSkinData::BoneData* pkBoneData = pkSkinData->m_pkBoneData;
	float* pfRows = static_cast<float*>(pvBoneMatrices);
	for (uint32_t i = 0; i < uiBones; ++i) {
		kTransform = ppkBones[i]->m_kWorld * pkBoneData[i].m_kSkinToBone;
		if (ppkBones[i]->m_kWorld.m_fScale < 0.9900000095367432 && 1.01f < ppkBones[i]->m_kWorld.m_fScale)
			bNormalize = true;
		if (!abPalettizedBones)
			kTransform = kSkinToWorld * kTransform;

		if (auiBoneMatrixRegisters == 4) {
			if (abTranspose) {
				NiD3DUtility::GetD3DTransposeFromNi(pkMatrices[i], kTransform.m_Rotate, kTransform.m_Translate,
					kTransform.m_fScale);
			} else {
				NiD3DUtility::GetD3DFromNi(pkMatrices[i], kTransform.m_Rotate, kTransform.m_Translate,
					kTransform.m_fScale);
			}
		} else {
			float* pfRow = pfRows + i * 12;
			float fScale = kTransform.m_fScale;
			pfRow[0] = kTransform.m_Rotate.m_pEntry[0][0] * fScale;
			pfRow[4] = kTransform.m_Rotate.m_pEntry[1][0] * fScale;
			pfRow[8] = kTransform.m_Rotate.m_pEntry[2][0] * fScale;
			pfRow[1] = kTransform.m_Rotate.m_pEntry[0][1] * fScale;
			pfRow[5] = kTransform.m_Rotate.m_pEntry[1][1] * fScale;
			pfRow[9] = kTransform.m_Rotate.m_pEntry[2][1] * fScale;
			pfRow[2] = kTransform.m_Rotate.m_pEntry[0][2] * fScale;
			pfRow[6] = kTransform.m_Rotate.m_pEntry[1][2] * fScale;
			pfRow[10] = kTransform.m_Rotate.m_pEntry[2][2] * fScale;
			pfRow[3] = kTransform.m_Translate.x - NiRenderer::PosAdjust.x;
			pfRow[7] = kTransform.m_Translate.y - NiRenderer::PosAdjust.y;
			pfRow[11] = kTransform.m_Translate.z - NiRenderer::PosAdjust.z;
		}
	}

	m_pkRenderState->SetInternalNormalizeNormals(bNormalize);
}

// Takes akWorld as the model transform; with at most four bones in the partition, the device's world matrices 0 to 3
// get the partition's first four bone matrices.
void NiDX9Renderer::SetSkinnedModelTransforms(const NiSkinInstance* apSkin,
	const NiSkinPartition::Partition* apPartition, const NiTransform& akWorld) {
	m_pkRenderState->vSetModelTransform(&akWorld);
	NiD3DUtility::GetD3DFromNi(m_kD3DMat, akWorld.m_Rotate, akWorld.m_Translate, akWorld.m_fScale);
	if (apPartition->m_usBones > 4)
		return;

	const D3DMATRIX* pkBoneMatrices = static_cast<const D3DMATRIX*>(apSkin->m_pvBoneMatrices);
	for (uint32_t i = 0; i < 4; ++i)
		m_pkD3DDevice9->SetTransform(D3DTS_WORLD + i, &pkBoneMatrices[apPartition->m_pusBones[i]]);
}

// Takes akXform as the model transform (pushed to the device's world matrix when asked) and expresses the camera's
// right and up vectors through its scaled rotation.
void NiDX9Renderer::SetModelTransform(const NiTransform& akXform, bool abPushToDevice) {
	NiD3DUtility::GetD3DFromNi(m_kD3DMat, akXform.m_Rotate, akXform.m_Translate, akXform.m_fScale);
	if (abPushToDevice)
		m_pkD3DDevice9->SetTransform(D3DTS_WORLD, &m_kD3DMat);
	m_pkRenderState->vSetModelTransform(&akXform);

	float fScale = akXform.m_fScale;
	float afM[3][3];
	for (uint32_t i = 0; i < 3; ++i) {
		for (uint32_t j = 0; j < 3; ++j)
			afM[i][j] = akXform.m_Rotate.m_pEntry[i][j] * fScale;
	}

	m_kModelCamRight.x = (m_kCamRight.x * afM[0][0] + m_kCamRight.y * afM[1][0]) + m_kCamRight.z * afM[2][0];
	m_kModelCamRight.y = (m_kCamRight.x * afM[0][1] + m_kCamRight.y * afM[1][1]) + m_kCamRight.z * afM[2][1];
	m_kModelCamRight.z = (m_kCamRight.x * afM[0][2] + m_kCamRight.y * afM[1][2]) + m_kCamRight.z * afM[2][2];
	m_kModelCamUp.x = (m_kCamUp.x * afM[0][0] + m_kCamUp.y * afM[1][0]) + m_kCamUp.z * afM[2][0];
	m_kModelCamUp.y = (m_kCamUp.x * afM[0][1] + m_kCamUp.y * afM[1][1]) + m_kCamUp.z * afM[2][1];
	m_kModelCamUp.z = (m_kCamUp.x * afM[0][2] + m_kCamUp.y * afM[1][2]) + m_kCamUp.z * afM[2][2];
}

// The Direct3D declaration of shader declaration auiIndex; index 0 has none.
IDirect3DVertexDeclaration9* NiDX9Renderer::GetD3DDeclarationFromIndex(uint32_t auiIndex) {
	if (!auiIndex)
		return nullptr;
	return ms_aspDeclarations[auiIndex].m_pObject->GetD3DDeclaration();
}

// Packs one skin partition: a valid buffer whose data is static and already packed is left alone. A stream count
// change, an invalid buffer (its chips released) or abForce repacks everything. The vertices go through the shader
// declaration, else (one stream only) the vertex buffer manager; the partition's indices are packed when they are
// dirty. False when a pack fails.
bool NiDX9Renderer::PackSkinnedGeometryBuffer(NiGeometryBufferData* apBuffData, NiGeometryData* apData,
	const NiSkinInstance* apSkin, NiSkinPartition::Partition* apPartition, NiD3DShaderDeclaration* apShaderDecl,
	bool abForce) {
	if (!apBuffData || !apData)
		return false;

	bool bValid = apBuffData->IsVBChipValid();
	if (!abForce && bValid && (apData->m_usDirtyFlags & 0x7000) == 0x4000)
		return true;

	apBuffData->m_uiFlags = GetGeometryBufferFlags(apData);

	uint16_t usTriangles = apPartition->m_usTriangles;
	const uint16_t* pusTriList = apPartition->m_pusTriList;
	uint16_t usVertices = apPartition->m_usVertices;
	const uint16_t* pusStripLengths = apPartition->m_pusStripLengths;
	uint16_t usArrays = apPartition->m_usStrips;
	uint32_t uiIndices;
	if (!usArrays) {
		uiIndices = usTriangles * 3;
		usArrays = 1;
	} else {
		uiIndices = usTriangles + usArrays * 2;
	}

	uint32_t uiStreams = apShaderDecl ? apShaderDecl->m_uiStreamCount : 1;
	uint32_t uiDirtyFlags = abForce ? 0xFFF : (apData->m_usDirtyFlags & 0xFFF);
	if (bValid) {
		if (uiStreams != apBuffData->m_uiStreamCount) {
			apBuffData->SetStreamCount(uiStreams);
			uiDirtyFlags = 0xFFF;
		}
	} else {
		NiGeometryGroup* pkGroup = apBuffData->m_pkGeometryGroup;
		for (uint32_t i = 0; i < uiStreams; ++i)
			pkGroup->ReleaseChip(apBuffData, i);
		apBuffData->SetStreamCount(uiStreams);
		apBuffData->m_uiBaseVertexIndex = 0;
		uiDirtyFlags = 0xFFF;
	}

	apBuffData->m_uiVertCount = usVertices;
	apBuffData->m_uiMaxVertCount = usVertices;
	apBuffData->m_pusArrayLengths = pusStripLengths;
	apBuffData->m_pusIndexArray = pusTriList;
	apBuffData->m_uiTriCount = usTriangles;
	apBuffData->m_uiMaxTriCount = usTriangles;
	apBuffData->m_uiNumArrays = usArrays;

	NiSkinInstance* pkSkin = const_cast<NiSkinInstance*>(apSkin);
	if (uiDirtyFlags & ~0x10u) {
		for (uint32_t i = 0; i < uiStreams; ++i) {
			NiVBChip* pkChip = i < apBuffData->m_uiStreamCount ? apBuffData->m_ppkVBChip[i] : nullptr;
			if (!apShaderDecl ||
				!apShaderDecl->PackUntransformedVB(apData, pkSkin, apPartition, static_cast<uint16_t>(uiDirtyFlags),
					pkChip, i, nullptr, nullptr, nullptr)) {
				if (uiStreams != 1 ||
					!m_pkVBManager->PackSkinnedVB(apData, pkSkin, apPartition, uiDirtyFlags, pkChip, nullptr))
					return false;
			}
		}
	}

	if (!(uiDirtyFlags & 0x30))
		return true;

	IDirect3DIndexBuffer9* pkOldIB = apBuffData->m_pkIB;
	uint32_t uiIBSize = apBuffData->m_uiIBSize;
	IDirect3DIndexBuffer9* pkIB = m_pkIBManager->PackBuffer(pusTriList, uiIndices, uiIndices, pkOldIB, &uiIBSize,
		bUseDirect3D9Ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED,
		apBuffData->m_bSoftwareVP ? D3DUSAGE_SOFTWAREPROCESSING : 0);
	if (!pkIB)
		return false;
	if (pkIB != pkOldIB) {
		apBuffData->RemoveIB();
		apBuffData->m_pkIB = pkIB;
		apBuffData->m_uiIndexCount = uiIndices;
		apBuffData->m_uiIBSize = uiIBSize;
	}
	return true;
}

// Sets up the buffer's streams, format and strides from the shader declaration, else (at most four bones per
// partition, one stream) from the vertex buffer manager, then reserves vertex buffer space for each stream. When a
// stream fails, the chips of the streams from it down to 1 are released (stream 0 keeps its chip) and false returned.
bool NiDX9Renderer::ReservePrepackedBufferSpace(NiGeometryBufferData* apBuffData, NiGeometryData* apData,
	NiSkinPartition::Partition* apPartition, NiD3DShaderDeclaration* apShaderDecl) {
	uint32_t uiFVF = 0;
	IDirect3DVertexDeclaration9* hDeclaration = nullptr;
	uint32_t uiStreams = 0;
	uint32_t uiVertexStride = 0;
	if (!apShaderDecl || !apShaderDecl->GenerateVertexParameters(&hDeclaration, &uiStreams)) {
		if (apPartition && apPartition->m_usBones > 4)
			return false;
		uiStreams = 1;
		// Only the format and the stride are read back.
		uint32_t uiPosOffset;
		uint32_t uiWeightOffset;
		uint32_t uiNormOffset;
		uint32_t uiColorOffset;
		uint32_t uiTexOffset;
		m_pkVBManager->GenerateVertexParameters(apData, apPartition, uiFVF, uiVertexStride, uiPosOffset, uiWeightOffset,
			uiNormOffset, uiColorOffset, uiTexOffset);
	}

	apBuffData->SetStreamCount(uiStreams);
	if (uiFVF) {
		apBuffData->SetFVF(uiFVF);
		if (apBuffData->m_uiStreamCount > 0)
			apBuffData->m_puiVertexStride[0] = uiVertexStride;
	} else {
		apBuffData->SetVertexDeclaration(hDeclaration);
		for (uint32_t i = 0; i < uiStreams; ++i) {
			uint32_t uiStride = apShaderDecl->GetVertexStride(i);
			if (i < apBuffData->m_uiStreamCount)
				apBuffData->m_puiVertexStride[i] = uiStride;
		}
	}

	apBuffData->m_uiFlags = GetGeometryBufferFlags(apData);
	if (apShaderDecl && m_bSWVertexCapable)
		apBuffData->m_bSoftwareVP = apShaderDecl->GetSoftwareVertexProcessing();

	for (uint32_t i = 0; i < uiStreams; ++i) {
		if (!m_pkVBManager->AllocateBufferSpace(apBuffData, i)) {
			for (; i; --i)
				apBuffData->m_pkGeometryGroup->ReleaseChip(apBuffData, i);
			return false;
		}
	}
	return true;
}

// Packs a triangle shape's or strips' data like PackSkinnedGeometryBuffer packs a partition (all triangles, the
// active vertex and triangle counts), with the vertices also dirty (3) when the skin was not yet updated this frame.
// Other data gets no indices. The data's dirty flags are cleared unless a pack fails.
bool NiDX9Renderer::PackGeometryBuffer(NiGeometryBufferData* apBuffData, NiGeometryData* apData, NiSkinInstance* apSkin,
	NiD3DShaderDeclaration* apShaderDecl, bool abForce) {
	if (!apBuffData || !apData)
		return false;

	bool bValid = apBuffData->IsVBChipValid();
	if (!abForce && bValid && (apData->m_usDirtyFlags & 0x7000) == 0x4000)
		return true;

	apBuffData->m_uiFlags = GetGeometryBufferFlags(apData);

	uint16_t usVertices = apData->GetActiveVertexCount();
	uint16_t usMaxVertices = apData->m_usVertices;
	uint16_t usTriangles = 0;
	uint16_t usMaxTriangles = 0;
	uint32_t uiIndices = 0;
	const uint16_t* pusIndices = nullptr;
	const uint16_t* pusArrayLengths = nullptr;
	uint16_t usArrays = 0;
	if (apData->IsKindOf(&NiTriShapeData::ms_RTTI)) {
		NiTriShapeData* pkShapeData = static_cast<NiTriShapeData*>(apData);
		usTriangles = pkShapeData->GetActiveTriangleCount();
		usMaxTriangles = pkShapeData->m_usTriangles;
		pusIndices = pkShapeData->GetTriList();
		pusArrayLengths = nullptr;
		usArrays = 1;
		uiIndices = usMaxTriangles * 3;
	} else if (NiObject::IsKindOf(&NiTriStripsData::ms_RTTI, apData)) {
		NiTriStripsData* pkStripsData = static_cast<NiTriStripsData*>(apData);
		usTriangles = pkStripsData->GetActiveTriangleCount();
		usMaxTriangles = pkStripsData->m_usTriangles;
		pusIndices = pkStripsData->GetStripLists();
		pusArrayLengths = pkStripsData->m_pusStripLengths;
		usArrays = pkStripsData->m_usStrips;
		uiIndices = usMaxTriangles + usArrays * 2;
	}

	uint32_t uiStreams = apShaderDecl ? apShaderDecl->m_uiStreamCount : 1;
	uint32_t uiDirtyFlags = apData->m_usDirtyFlags & 0xFFF;
	if (apSkin) {
		uint32_t uiSkinFrameID = apSkin->m_uiFrameID;
		if (uiSkinFrameID != m_pkVBManager->GetFrameID()) {
			uiDirtyFlags |= 3;
			apSkin->m_uiFrameID = m_pkVBManager->GetFrameID();
		}
	}
	if (abForce)
		uiDirtyFlags = 0xFFF;

	if (bValid) {
		if (uiStreams != apBuffData->m_uiStreamCount) {
			apBuffData->SetStreamCount(uiStreams);
			uiDirtyFlags = 0xFFF;
		}
	} else {
		NiGeometryGroup* pkGroup = apBuffData->m_pkGeometryGroup;
		for (uint32_t i = 0; i < uiStreams; ++i)
			pkGroup->ReleaseChip(apBuffData, i);
		apBuffData->SetStreamCount(uiStreams);
		apBuffData->m_uiBaseVertexIndex = 0;
		uiDirtyFlags = 0xFFF;
	}

	apBuffData->m_uiVertCount = usVertices;
	apBuffData->m_uiMaxVertCount = usMaxVertices;
	apBuffData->m_pusArrayLengths = pusArrayLengths;
	apBuffData->m_uiMaxTriCount = usMaxTriangles;
	apBuffData->m_uiTriCount = usTriangles;
	apBuffData->m_pusIndexArray = pusIndices;
	apBuffData->m_uiNumArrays = usArrays;

	if (uiDirtyFlags & ~0x10u) {
		for (uint32_t i = 0; i < uiStreams; ++i) {
			NiVBChip* pkChip = i < apBuffData->m_uiStreamCount ? apBuffData->m_ppkVBChip[i] : nullptr;
			if (!apShaderDecl ||
				!apShaderDecl->PackUntransformedVB(apData, apSkin, nullptr, static_cast<uint16_t>(uiDirtyFlags), pkChip,
					i, nullptr, nullptr, nullptr)) {
				if (uiStreams != 1 ||
					!m_pkVBManager->PackUntransformedVB(apData, apSkin, uiDirtyFlags, pkChip, nullptr))
					return false;
			}
		}
	}

	if (pusIndices && (uiDirtyFlags & 0x30)) {
		IDirect3DIndexBuffer9* pkOldIB = apBuffData->m_pkIB;
		uint32_t uiIBSize = apBuffData->m_uiIBSize;
		IDirect3DIndexBuffer9* pkIB = m_pkIBManager->PackBuffer(pusIndices, uiIndices, uiIndices, pkOldIB, &uiIBSize,
			bUseDirect3D9Ex ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED,
			apBuffData->m_bSoftwareVP ? D3DUSAGE_SOFTWAREPROCESSING : 0);
		if (!pkIB)
			return false;
		if (pkIB != pkOldIB) {
			apBuffData->RemoveIB();
			apBuffData->m_pkIB = pkIB;
			apBuffData->m_uiIndexCount = uiIndices;
			apBuffData->m_uiIBSize = uiIBSize;
		}
	}

	apData->m_usDirtyFlags &= 0xF000;
	return true;
}

// PrePackGeometryBuffer per skin partition whose chips are not valid (all triangles of the partition); false when a
// partition's space cannot be reserved (the other partitions are still queued). apGeometry is not read.
bool NiDX9Renderer::PrePackSkinnedGeometryBuffer(NiGeometry*, NiGeometryData* apData, NiSkinInstance* apSkin,
	NiD3DShaderDeclaration* apShaderDecl, uint32_t auiBonesPerPartition, uint32_t auiBonesPerVertex) {
	NiSkinPartition* pkSkinPartition = apSkin->m_spSkinPartition.m_pObject;
	NiSkinPartition::Partition* pkPartition = pkSkinPartition->m_pkPartitions;
	uint32_t uiPartitions = pkSkinPartition->m_uiPartitions;
	bool bResult = true;
	for (uint32_t p = 0; p < uiPartitions; ++p, ++pkPartition) {
		NiGeometryBufferData* pkBuffData = pkPartition->m_pkBuffData;
		if (pkBuffData->IsVBChipValid())
			continue;

		pkBuffData->m_uiVertCount = pkPartition->m_usVertices;
		pkBuffData->m_uiMaxVertCount = pkPartition->m_usVertices;
		pkBuffData->m_uiNumArrays = pkPartition->m_usStrips ? pkPartition->m_usStrips : 1;
		pkBuffData->m_uiTriCount = pkPartition->m_usTriangles;
		pkBuffData->m_uiMaxTriCount = pkPartition->m_usTriangles;
		pkBuffData->m_pusIndexArray = pkPartition->m_pusTriList;
		pkBuffData->m_pusArrayLengths = pkPartition->m_pusStripLengths;
		if (!ReservePrepackedBufferSpace(pkBuffData, apData, pkPartition, apShaderDecl)) {
			bResult = false;
			continue;
		}

		uint32_t uiStreams = pkBuffData->m_uiStreamCount;
		for (uint32_t i = 0; i < uiStreams; ++i) {
			NiVBChip* pkChip = i < pkBuffData->m_uiStreamCount ? pkBuffData->m_ppkVBChip[i] : nullptr;
			NiVBBlock* pkBlock = pkChip->m_pkBlock;
			PrePackObject* pkObject = new PrePackObject;
			pkObject->m_pkPartition = pkPartition;
			pkObject->m_pkData = apData;
			pkObject->m_pkShaderDecl = apShaderDecl;
			pkObject->m_uiBonesPerPartition = auiBonesPerPartition;
			pkObject->m_pkSkin = apSkin;
			pkObject->m_uiBonesPerVertex = auiBonesPerVertex;
			// Clears the link before the map lookup, then stores the lookup's result over it.
			pkObject->m_pkNext = nullptr;
			pkObject->m_pkBuffData = pkBuffData;
			pkObject->m_uiStream = i;
			PrePackObject* pkHead;
			pkObject->m_pkNext = m_kPrePackObjects.GetAt(pkBlock, pkHead) ? pkHead : nullptr;
			m_kPrePackObjects.SetAt(pkBlock, pkObject);
		}
	}
	return bResult;
}

// Sets up the data's buffer for packing at the next precache (unless its chips are valid) and queues one object per
// stream under the stream's vertex buffer block, in front of the block's queue. apGeometry is not read.
bool NiDX9Renderer::PrePackGeometryBuffer(NiGeometry*, NiGeometryData* apData, uint16_t ausTriangles,
	uint16_t ausMaxTriangles, const uint16_t* apusIndices, const uint16_t* apusStripLengths, uint16_t ausStripCount,
	NiD3DShaderDeclaration* apShaderDecl) {
	NiGeometryBufferData* pkBuffData = apData->m_pkBuffData;
	if (pkBuffData->IsVBChipValid())
		return true;

	uint16_t usVertices = apData->GetActiveVertexCount();
	pkBuffData->m_uiMaxVertCount = apData->m_usVertices;
	pkBuffData->m_uiVertCount = usVertices;
	pkBuffData->m_pusIndexArray = apusIndices;
	pkBuffData->m_uiMaxTriCount = ausMaxTriangles;
	pkBuffData->m_uiTriCount = ausTriangles;
	pkBuffData->m_pusArrayLengths = apusStripLengths;
	pkBuffData->m_uiNumArrays = ausStripCount;
	if (!ReservePrepackedBufferSpace(pkBuffData, apData, nullptr, apShaderDecl))
		return false;

	uint32_t uiStreams = pkBuffData->m_uiStreamCount;
	for (uint32_t i = 0; i < uiStreams; ++i) {
		NiVBChip* pkChip = i < pkBuffData->m_uiStreamCount ? pkBuffData->m_ppkVBChip[i] : nullptr;
		NiVBBlock* pkBlock = pkChip->m_pkBlock;
		PrePackObject* pkObject = new PrePackObject;
		pkObject->m_pkData = apData;
		pkObject->m_pkShaderDecl = apShaderDecl;
		pkObject->m_pkSkin = nullptr;
		pkObject->m_pkPartition = nullptr;
		pkObject->m_uiBonesPerPartition = 0;
		pkObject->m_uiBonesPerVertex = 0;
		pkObject->m_pkBuffData = pkBuffData;
		pkObject->m_uiStream = i;
		PrePackObject* pkHead;
		pkObject->m_pkNext = m_kPrePackObjects.GetAt(pkBlock, pkHead) ? pkHead : nullptr;
		m_kPrePackObjects.SetAt(pkBlock, pkObject);
	}
	return true;
}
