#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/FaceGen/NiGeometryDataIteratorNiPoint3.h"
#include "Fallout Shared/NiGeometryDataIteratorNiColorA.h"
#include "Fallout Shared/TempEffects/NiGeometryDataIteratorNiBlendIndices.h"
#include "Fallout Shared/TempEffects/NiGeometryDataIteratorNiBlendWeights.h"
#include "Fallout Shared/TempEffects/NiGeometryDataIteratorNiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSPackedAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"

NiRTTI NiGeometryData::ms_RTTI("NiGeometryData", &NiObject::ms_RTTI);

NiCriticalSection NiGeometryData::ms_kCritSec;

NiGeometryData::NiGeometryData()
	: m_usVertices(0), m_usDataFlags(0), m_usDirtyFlags(0), m_pkVertex(nullptr), m_pkNormal(nullptr),
	  m_pkColor(nullptr), m_pkTexture(nullptr), m_pkBuffData(nullptr), m_ucKeepFlags(0), m_ucCompressFlags(0),
	  m_bVBLocked(false), m_bVBLockWrite(false), m_bSaveVertexData(VERTEX_MASK) {
	if (!m_spAdditionalGeomData || !m_spAdditionalGeomData->GetVBReady()) {
		m_usDirtyFlags &= ~CONSISTENCY_MASK;
	}
}

NiGeometryData::NiGeometryData(int32_t usVertices, NiPoint3* pkVertex, NiPoint3* pkNormal, NiColorA* pkColor,
	NiPoint2* pkTexture, int32_t usNumTextureSets, NiGeometryData::DataFlags eNBTMethod)
	: m_usVertices(static_cast<uint16_t>(usVertices)), m_usID(ms_usNextID++),
	  m_usDataFlags(static_cast<uint16_t>(eNBTMethod | (usNumTextureSets & 1))), m_usDirtyFlags(0),
	  m_pkVertex(pkVertex), m_pkNormal(pkNormal), m_pkColor(pkColor), m_pkTexture(pkTexture), m_pkBuffData(nullptr),
	  m_ucKeepFlags(0), m_ucCompressFlags(0), m_bVBLocked(false), m_bVBLockWrite(false),
	  m_bSaveVertexData(VERTEX_MASK) {
	if (m_usVertices && m_pkVertex) {
		m_kBound.ComputeFromData(m_usVertices, m_pkVertex);
	}
	if (!m_spAdditionalGeomData || !m_spAdditionalGeomData->GetVBReady()) {
		m_usDirtyFlags &= ~CONSISTENCY_MASK;
	}
}

NiGeometryData::~NiGeometryData() {
	if (NiRenderer::ms_pkRenderer) {
		NiRenderer::ms_pkRenderer->PurgeGeometryData(this);
	}
	m_spAdditionalGeomData = nullptr;
	if (m_pkGroup) {
		// Each array held a reference on the shared group; the last one frees its vertex buffer and the group.
		const void* apvArrays[] = {m_pkVertex, m_pkNormal, m_pkColor, m_pkTexture};
		for (const void* pvArray : apvArrays) {
			if (pvArray && --m_pkGroup->m_uiRefCount == 0) {
				m_pkGroup->FreeVertexBuffer();
				delete m_pkGroup;
			}
		}
	} else {
		MemoryManager_Singleton.Deallocate(m_pkVertex);
		MemoryManager_Singleton.Deallocate(m_pkNormal);
		MemoryManager_Singleton.Deallocate(m_pkColor);
		MemoryManager_Singleton.Deallocate(m_pkTexture);
	}
}

// KEEP (niread:): the has-vertices, has-normals and has-colors bools are read from the stream into stack slots
// nothing initialised (the first into arStream's own argument slot), so a short read leaves them unwritten and the
// body tests them.
void NiGeometryData::LoadBinary(NiStream& kStream) {
	bool bHasVertex;
	bool bHasNormal;
	bool bHasColor;
	uint32_t v69;
	uint32_t uiID;
	NiStream* v2 = &kStream;
	NiObject::LoadBinary(kStream);
	if (v2->GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 114)) {
		NiBinaryStream* m_pkIstr = v2->Istr();
		NiBinaryStream::NIBINARYSTREAMREADFN m_pfnRead = m_pkIstr->m_pfnRead;
		v69 = 4;
		m_pkIstr->m_uiAbsoluteCurrentPos += m_pfnRead(m_pkIstr, &uiID, 4, &v69, 1);
		NiGeometryData::m_pkGroup = v2->GetGroupFromID(uiID);
	}
	NiBinaryStream* v6 = v2->Istr();
	NiBinaryStream::NIBINARYSTREAMREADFN v7 = v6->m_pfnRead;
	uint16_t* p_m_usVertices = &this->m_usVertices;
	uiID = 2;
	v6->m_uiAbsoluteCurrentPos += v7(v6, &this->m_usVertices, 2, &uiID, 1);
	if (v2->GetFileVersion() >= NiStream::GetVersion(10, 0, 1, 16)) {
		NiBinaryStream* v9 = v2->Istr();
		NiBinaryStream::NIBINARYSTREAMREADFN v10 = v9->m_pfnRead;
		uiID = 1;
		v9->m_uiAbsoluteCurrentPos += v10(v9, &m_ucKeepFlags, 1, &uiID, 1);
		NiBinaryStream* v11 = v2->Istr();
		NiBinaryStream::NIBINARYSTREAMREADFN v12 = v11->m_pfnRead;
		uiID = 1;
		v11->m_uiAbsoluteCurrentPos += v12(v11, &m_ucCompressFlags, 1, &uiID, 1);
	}
	NiBinaryStream* v13 = v2->Istr();
	NiBinaryStream::NIBINARYSTREAMREADFN v14 = v13->m_pfnRead;
	if (v2->GetFileVersion() >= NiStream::GetVersion(4, 1, 0, 0)) {
		uiID = 1;
		v13->m_uiAbsoluteCurrentPos += v14(v13, &bHasVertex, 1, &uiID, 1);
	} else {
		uiID = 4;
		v13->m_uiAbsoluteCurrentPos += v14(v13, &this->m_pkVertex, 4, &uiID, 1);
		bHasVertex = this->m_pkVertex != 0;
	}
	if (bHasVertex) {
		NiObjectGroup* v15 = GetGroup();
		int32_t v16 = *p_m_usVertices;
		if (v15) {
			NiObjectGroup* v17 = GetGroup();
			NiPoint3* m_pvFree = static_cast<NiPoint3*>(v17->m_pvFree);
			++v17->m_uiRefCount;
			v17->m_pvFree = (void*)&m_pvFree[v16];
			this->m_pkVertex = m_pvFree;
		} else {
			this->m_pkVertex =
				static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(*p_m_usVertices <= SIZE_MAX / sizeof(NiPoint3)
						? sizeof(NiPoint3) * *p_m_usVertices
						: SIZE_MAX));
		}
		if (v2->m_BSStreamHeader.uiVersion < 19 || m_bSaveVertexData) {
			NiBinaryStream* v19 = v2->Istr();
			int32_t v65 = 12 * *p_m_usVertices;
			NiBinaryStream::NIBINARYSTREAMREADFN v20 = v19->m_pfnRead;
			NiPoint3* m_pkVertex = this->m_pkVertex;
			uiID = 4;
			v19->m_uiAbsoluteCurrentPos += v20(v19, m_pkVertex, v65, &uiID, 1);
		} else {
			std::memset(this->m_pkVertex, 0, sizeof(NiPoint3) * *p_m_usVertices);
		}
	}
	bool v21 = v2->GetFileVersion() < NiStream::GetVersion(10, 0, 0, 2);
	uint32_t v22 = *p_m_usVertices;
	v69 = v22;
	if (!v21) {
		NiBinaryStream* v23 = v2->Istr();
		NiBinaryStream::NIBINARYSTREAMREADFN v24 = v23->m_pfnRead;
		uiID = 2;
		v23->m_uiAbsoluteCurrentPos += v24(v23, &this->m_usDataFlags, 2, &uiID, 1);
		if ((this->m_usDataFlags & NBT_METHOD_MASK) != 0) {
			v69 = 3 * v22;
			v22 *= 3;
		}
	}
	NiBinaryStream* v25 = v2->Istr();
	NiBinaryStream::NIBINARYSTREAMREADFN v26 = v25->m_pfnRead;
	if (v2->GetFileVersion() >= NiStream::GetVersion(4, 1, 0, 0)) {
		uiID = 1;
		v25->m_uiAbsoluteCurrentPos += v26(v25, &bHasNormal, 1, &uiID, 1);
	} else {
		uiID = 4;
		v25->m_uiAbsoluteCurrentPos += v26(v25, &this->m_pkNormal, 4, &uiID, 1);
		v22 = v69;
		bHasNormal = this->m_pkNormal != 0;
	}
	if (bHasNormal) {
		if (GetGroup()) {
			NiObjectGroup* v27 = GetGroup();
			NiPoint3* v28 = static_cast<NiPoint3*>(v27->m_pvFree);
			++v27->m_uiRefCount;
			v27->m_pvFree = (void*)&v28[v22];
			this->m_pkNormal = v28;
		} else {
			this->m_pkNormal =
				static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(v22 <= SIZE_MAX / sizeof(NiPoint3)
						? sizeof(NiPoint3) * v22
						: SIZE_MAX));
		}
		if (v2->m_BSStreamHeader.uiVersion < 19 || m_bSaveVertexData) {
			NiBinaryStream* v29 = v2->Istr();
			NiBinaryStream::NIBINARYSTREAMREADFN v30 = v29->m_pfnRead;
			NiPoint3* m_pkNormal = this->m_pkNormal;
			uiID = 4;
			v29->m_uiAbsoluteCurrentPos += v30(v29, m_pkNormal, 12 * v22, &uiID, 1);
		}
	}
	m_kBound.LoadBinary(*v2);
	if (v2->GetFileVersion() >= NiStream::GetVersion(4, 1, 0, 0)) {
		NiBinaryStream* v33 = v2->Istr();
		NiBinaryStream::NIBINARYSTREAMREADFN v34 = v33->m_pfnRead;
		uiID = 1;
		v33->m_uiAbsoluteCurrentPos += v34(v33, &bHasColor, 1, &uiID, 1);
	} else {
		NiBinaryStream* v31 = v2->Istr();
		NiBinaryStream::NIBINARYSTREAMREADFN v32 = v31->m_pfnRead;
		uiID = 4;
		v31->m_uiAbsoluteCurrentPos += v32(v31, &this->m_pkColor, 4, &uiID, 1);
		bHasColor = this->m_pkColor != 0;
	}
	if (bHasColor) {
		if (GetGroup()) {
			int32_t m_usVertices = this->m_usVertices;
			NiObjectGroup* v36 = GetGroup();
			NiColorA* v37 = static_cast<NiColorA*>(v36->m_pvFree);
			++v36->m_uiRefCount;
			v36->m_pvFree = (void*)&v37[m_usVertices];
			this->m_pkColor = v37;
		} else {
			uint32_t uiNumberOfNormals = this->m_usVertices;
			NiColorA* v39 = static_cast<NiColorA*>(MemoryManager_Singleton.Allocate(this->m_usVertices <=
						SIZE_MAX / sizeof(NiColorA)
					? sizeof(NiColorA) * uiNumberOfNormals
					: SIZE_MAX));
			NiColorA* v40 = v39;
			if (v39) {
				rt::eh_vector_constructor_iterator(v39, sizeof(NiColorA), static_cast<int32_t>(uiNumberOfNormals),
					&fnvd::constructElement<NiColorA>);
			} else {
				v40 = 0;
			}
			this->m_pkColor = v40;
		}
		if (v2->m_BSStreamHeader.uiVersion < 19 || m_bSaveVertexData) {
			NiBinaryStream* v41 = v2->Istr();
			int32_t v66 = 16 * this->m_usVertices;
			NiBinaryStream::NIBINARYSTREAMREADFN v42 = v41->m_pfnRead;
			NiColorA* m_pkColor = this->m_pkColor;
			uiID = 4;
			v41->m_uiAbsoluteCurrentPos += v42(v41, m_pkColor, v66, &uiID, 1);
		}
	}
	uint32_t m_uiNifFileVersion = v2->GetFileVersion();
	if (m_uiNifFileVersion >= NiStream::GetVersion(5, 0, 0, 10)) {
		if (m_uiNifFileVersion < NiStream::GetVersion(10, 0, 0, 2)) {
			NiBinaryStream* v46 = v2->Istr();
			NiBinaryStream::NIBINARYSTREAMREADFN v47 = v46->m_pfnRead;
			uiID = 2;
			v46->m_uiAbsoluteCurrentPos += v47(v46, &this->m_usDataFlags, 2, &uiID, 1);
		}
	} else {
		NiBinaryStream* v44 = v2->Istr();
		NiBinaryStream::NIBINARYSTREAMREADFN v45 = v44->m_pfnRead;
		uiID = 2;
		v44->m_uiAbsoluteCurrentPos += v45(v44, &v69, 2, &uiID, 1);
		this->m_usDataFlags ^= (static_cast<uint8_t>(v69) ^ static_cast<uint8_t>(this->m_usDataFlags)) & 1;
	}
	if (v2->GetFileVersion() < NiStream::GetVersion(4, 1, 0, 0)) {
		NiBinaryStream* v48 = v2->Istr();
		NiBinaryStream::NIBINARYSTREAMREADFN v49 = v48->m_pfnRead;
		uiID = 4;
		v48->m_uiAbsoluteCurrentPos += v49(v48, &this->m_pkTexture, 4, &uiID, 1);
	}
	if (v2->m_BSStreamHeader.uiVersion >= 27) {
		v69 = 0;
	} else {
		uint16_t m_usDataFlags = this->m_usDataFlags;
		v69 = m_usDataFlags & 31;
		if (static_cast<uint16_t>(v69) > 1u) {
			this->m_usDataFlags = m_usDataFlags | 1;
		}
	}
	uint16_t v51 = this->m_usDataFlags;
	if ((v51 & 1) != 0) {
		uint32_t v52 = this->m_usVertices * (v51 & 1);
		if (GetGroup()) {
			NiObjectGroup* v53 = GetGroup();
			NiPoint2* v54 = static_cast<NiPoint2*>(v53->m_pvFree);
			++v53->m_uiRefCount;
			v53->m_pvFree = (void*)&v54[v52];
			this->m_pkTexture = v54;
		} else {
			this->m_pkTexture =
				static_cast<NiPoint2*>(MemoryManager_Singleton.Allocate(v52 <= SIZE_MAX / sizeof(NiPoint2)
						? sizeof(NiPoint2) * v52
						: SIZE_MAX));
		}
		if (v2->m_BSStreamHeader.uiVersion < 19 || m_bSaveVertexData) {
			NiBinaryStream* v55 = v2->Istr();
			NiBinaryStream::NIBINARYSTREAMREADFN v56 = v55->m_pfnRead;
			NiPoint2* m_pkTexture = this->m_pkTexture;
			uiID = 4;
			v55->m_uiAbsoluteCurrentPos += v56(v55, m_pkTexture, 8 * v52, &uiID, 1);
			// Older files' extra texture sets are read into a scratch array and dropped.
			if (static_cast<uint16_t>(v69) > 1u) {
				uint32_t v57 = this->m_usVertices * (static_cast<uint16_t>(v69) - 1);
				NiPoint2* pkExtraSets =
					static_cast<NiPoint2*>(MemoryManager_Singleton.Allocate(v57 <= SIZE_MAX / sizeof(NiPoint2)
							? sizeof(NiPoint2) * v57
							: SIZE_MAX));
				NiPoint2::LoadBinary(*v2, pkExtraSets, v57);
				MemoryManager_Singleton.Deallocate(pkExtraSets);
			}
		}
	}
	if (v2->GetFileVersion() >= NiStream::GetVersion(5, 0, 0, 10)) {
		NiBinaryStream* v59 = v2->Istr();
		NiBinaryStream::NIBINARYSTREAMREADFN v60 = v59->m_pfnRead;
		uiID = 2;
		v59->m_uiAbsoluteCurrentPos += v60(v59, &m_usDirtyFlags, 2, &uiID, 1);
	}
	if (v2->GetFileVersion() >= NiStream::GetVersion(10, 3, 0, 7)) {
		v2->ReadLinkID();
	}
}

// The asm calls the out-of-line empty nullsub_3 (0xA89AF0, NiObject::LinkObject's fold); it does not call a
// shader-property helper.
void NiGeometryData::LinkObject(NiStream& kStream) {
	NiObject::LinkObject(kStream);
	if (kStream.GetFileVersion() < 0x0A030007)
		return;

	NiAdditionalGeometryData* pkAdditionalData = static_cast<NiAdditionalGeometryData*>(kStream.GetObjectFromLinkID());
	m_spAdditionalGeomData = pkAdditionalData;
}

bool NiGeometryData::RegisterStreamables(NiStream& kStream) {
	if (!NiObject::RegisterStreamables(kStream))
		return false;
	NiAdditionalGeometryData* pkAdditionalData = m_spAdditionalGeomData.m_pObject;
	return !pkAdditionalData || pkAdditionalData->RegisterStreamables(kStream);
}

void NiGeometryData::SaveBinary(NiStream& kStream) {
	NiObject::SaveBinary(kStream);
	bool bHasVertex;
	bool bHasNormals;
	bool bHasColor;
	// One stack slot: the component size of the first writes, then the normal count, then the dirty flags written.
	uint32_t uiValue;
	uint32_t uiID_2;
	uint32_t uiSize;
	NiStream* v2 = &kStream;
	uint32_t uiID = v2->GetIDFromGroup(NiGeometryData::m_pkGroup);
	NiBinaryStream* m_pkOstr = v2->Ostr();
	uiID_2 = uiID;
	NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
	uiValue = 4;
	m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &uiID_2, 4, &uiValue, 1);
	NiBinaryStream* v7 = v2->Ostr();
	NiBinaryStream::NIBINARYSTREAMWRITEFN v8 = v7->m_pfnWrite;
	uint16_t* p_m_usVertices = &m_usVertices;
	uiValue = 2;
	v7->m_uiAbsoluteCurrentPos += v8(v7, &m_usVertices, 2, &uiValue, 1);
	NiBinaryStream* v10 = v2->Ostr();
	NiBinaryStream::NIBINARYSTREAMWRITEFN v11 = v10->m_pfnWrite;
	uiValue = 1;
	v10->m_uiAbsoluteCurrentPos += v11(v10, &m_ucKeepFlags, 1, &uiValue, 1);
	NiBinaryStream* v12 = v2->Ostr();
	NiBinaryStream::NIBINARYSTREAMWRITEFN v13 = v12->m_pfnWrite;
	uiValue = 1;
	v12->m_uiAbsoluteCurrentPos += v13(v12, &m_ucCompressFlags, 1, &uiValue, 1);
	NiBinaryStream* v14 = v2->Ostr();
	// The exe keeps this bool in the low byte of arStream's argument slot; only that byte is written out.
	bHasVertex = this->m_pkVertex != 0;
	NiBinaryStream::NIBINARYSTREAMWRITEFN v15 = v14->m_pfnWrite;
	uiValue = 1;
	v14->m_uiAbsoluteCurrentPos += v15(v14, &bHasVertex, 1, &uiValue, 1);
	if (bHasVertex) {
		if (m_bSaveVertexData) {
			NiBinaryStream* v16 = v2->Ostr();
			int32_t v36 = 12 * *p_m_usVertices;
			NiBinaryStream::NIBINARYSTREAMWRITEFN v17 = v16->m_pfnWrite;
			NiPoint3* m_pkVertex = this->m_pkVertex;
			uiValue = 4;
			v16->m_uiAbsoluteCurrentPos += v17(v16, m_pkVertex, v36, &uiValue, 1);
		}
	}
	if (!this->m_pkTexture) {
		m_usDataFlags &= ~1u;
	}
	NiBinaryStream* v18 = v2->Ostr();
	NiBinaryStream::NIBINARYSTREAMWRITEFN v19 = v18->m_pfnWrite;
	uiValue = 2;
	v18->m_uiAbsoluteCurrentPos += v19(v18, &m_usDataFlags, 2, &uiValue, 1);
	uint32_t v20 = *p_m_usVertices;
	uiValue = v20;
	if ((m_usDataFlags & NBT_METHOD_MASK) != 0) {
		uiValue = 3 * v20;
	}
	NiBinaryStream* v21 = v2->Ostr();
	bHasNormals = this->m_pkNormal != 0;
	NiBinaryStream::NIBINARYSTREAMWRITEFN v22 = v21->m_pfnWrite;
	uiSize = 1;
	v21->m_uiAbsoluteCurrentPos += v22(v21, &bHasNormals, 1, &uiSize, 1);
	if (bHasNormals && m_bSaveVertexData) {
		NiBinaryStream* v23 = v2->Ostr();
		NiBinaryStream::NIBINARYSTREAMWRITEFN v24 = v23->m_pfnWrite;
		NiPoint3* m_pkNormal = this->m_pkNormal;
		uiSize = 4;
		v23->m_uiAbsoluteCurrentPos += v24(v23, m_pkNormal, 12 * uiValue, &uiSize, 1);
	}
	m_kBound.SaveBinary(*v2);
	NiBinaryStream* v25 = v2->Ostr();
	bHasColor = this->m_pkColor != 0;
	NiBinaryStream::NIBINARYSTREAMWRITEFN v26 = v25->m_pfnWrite;
	uiSize = 1;
	v25->m_uiAbsoluteCurrentPos += v26(v25, &bHasColor, 1, &uiSize, 1);
	if (bHasColor && m_bSaveVertexData) {
		NiBinaryStream* v27 = v2->Ostr();
		int32_t v37 = 16 * *p_m_usVertices;
		NiBinaryStream::NIBINARYSTREAMWRITEFN v28 = v27->m_pfnWrite;
		NiColorA* m_pkColor = this->m_pkColor;
		uiSize = 4;
		v27->m_uiAbsoluteCurrentPos += v28(v27, m_pkColor, v37, &uiSize, 1);
	}
	uint32_t uiQuantity = *p_m_usVertices * (m_usDataFlags & NiGeometryData::VERTEX_MASK);
	if (uiQuantity) {
		NiPoint2* m_pkTexture = this->m_pkTexture;
		if (m_pkTexture) {
			if (m_bSaveVertexData) {
				NiPoint2::SaveBinary(*v2, m_pkTexture, uiQuantity);
			}
		}
	}
	NiBinaryStream* v31 = v2->Ostr();
	uiValue = m_usDirtyFlags & CONSISTENCY_MASK;
	NiBinaryStream::NIBINARYSTREAMWRITEFN v32 = v31->m_pfnWrite;
	uiSize = 2;
	v31->m_uiAbsoluteCurrentPos += v32(v31, &uiValue, 2, &uiSize, 1);
	v2->SaveLinkID(m_spAdditionalGeomData.m_pObject);
}

bool NiGeometryData::IsEqual(NiObject* pkObject) {
	if (!NiObject::IsEqual(pkObject)) {
		return false;
	}
	NiGeometryData* pkData = static_cast<NiGeometryData*>(pkObject);
	uint16_t usVertices = m_usVertices;
	if (usVertices != pkData->m_usVertices) {
		return false;
	}
	NiPoint3* pkVertex = m_pkVertex;
	if ((pkVertex && !pkData->m_pkVertex) || (!pkVertex && pkData->m_pkVertex)) {
		return false;
	}
	if (m_ucKeepFlags != pkData->m_ucKeepFlags || m_ucCompressFlags != pkData->m_ucCompressFlags) {
		return false;
	}
	if (pkVertex) {
		NiPoint3* pkOther = pkData->m_pkVertex;
		for (uint32_t i = 0; i < usVertices; i++) {
			if (pkVertex[i] != pkOther[i]) {
				return false;
			}
		}
	}
	NiPoint3* pkNormal = m_pkNormal;
	if ((pkNormal && !pkData->m_pkNormal) || (!pkNormal && pkData->m_pkNormal)) {
		return false;
	}
	if (pkNormal) {
		uint16_t usNBTMethod = m_usDataFlags & NBT_METHOD_MASK;
		if (usNBTMethod != (pkData->m_usDataFlags & NBT_METHOD_MASK)) {
			return false;
		}
		// With binormals and tangents the normal array holds three vectors per vertex.
		uint32_t uiCount = m_usVertices;
		if (usNBTMethod) {
			uiCount = 3 * uiCount;
		}
		NiPoint3* pkOther = pkData->m_pkNormal;
		for (uint32_t i = 0; i < uiCount; i++) {
			if (pkNormal[i] != pkOther[i]) {
				return false;
			}
		}
	}
	if (m_kBound != pkData->m_kBound) {
		return false;
	}
	// The exe tests the colours one way only and the texture coordinates the other way here, before both loops.
	NiColorA* pkColor = m_pkColor;
	if (pkColor && !pkData->m_pkColor) {
		return false;
	}
	if (!m_pkTexture && pkData->m_pkTexture) {
		return false;
	}
	if (pkColor) {
		uint32_t uiCount = m_usVertices;
		NiColorA* pkOther = pkData->m_pkColor;
		for (uint32_t i = 0; i < uiCount; i++) {
			if (pkColor[i] != pkOther[i]) {
				return false;
			}
		}
	}
	NiPoint2* pkTexture = m_pkTexture;
	if ((pkTexture && !pkData->m_pkTexture) || (!pkTexture && pkData->m_pkTexture)) {
		return false;
	}
	if (pkTexture) {
		// The exe keeps only the low bit of the texture-set count (Gamebryo's TEXTURE_SET_MASK is 0x3F).
		uint16_t usDataFlags = m_usDataFlags;
		if (((static_cast<uint8_t>(usDataFlags) ^ static_cast<uint8_t>(pkData->m_usDataFlags)) & 1) != 0) {
			return false;
		}
		uint32_t uiQuantity = m_usVertices * (usDataFlags & 1);
		NiPoint2* pkOther = pkData->m_pkTexture;
		for (uint32_t i = 0; i < uiQuantity; i++) {
			if (pkTexture[i] != pkOther[i]) {
				return false;
			}
		}
	}
	NiAdditionalGeometryData* pkAGD = m_spAdditionalGeomData.m_pObject;
	if ((pkAGD && !pkData->m_spAdditionalGeomData.m_pObject) || (!pkAGD && pkData->m_spAdditionalGeomData.m_pObject)) {
		return false;
	}
	if (pkAGD && pkData->m_spAdditionalGeomData.m_pObject &&
		!pkAGD->IsEqual(pkData->m_spAdditionalGeomData.m_pObject)) {
		return false;
	}
	return true;
}

void NiGeometryData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiObject::GetViewerStrings(pkStrings);
	char* ViewerString = NiGetViewerString(NiGeometryData::ms_RTTI.GetName());
	uint32_t m_usSize = pkStrings->m_usSize;
	uint32_t m_usMaxSize = pkStrings->m_usMaxSize;
	char* v41 = ViewerString;
	if (m_usSize >= m_usMaxSize) {
		pkStrings->SetSize(m_usSize + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(m_usSize, v41);
	char* ViewerString_4 = NiGetViewerString("m_usVertices", m_usVertices);
	uint32_t v8 = pkStrings->m_usSize;
	uint32_t v9 = pkStrings->m_usMaxSize;
	v41 = ViewerString_4;
	if (v8 >= v9) {
		pkStrings->SetSize(v8 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v8, v41);
	char* ViewerString_8 = NiGetViewerString("m_pkVertex", static_cast<const void*>(m_pkVertex));
	uint32_t v11 = pkStrings->m_usSize;
	v41 = ViewerString_8;
	if (v11 >= pkStrings->m_usMaxSize) {
		pkStrings->SetSize(v11 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v11, v41);
	char* v12 = NiGetViewerString("m_pkNormal", static_cast<const void*>(m_pkNormal));
	uint32_t v13 = pkStrings->m_usSize;
	uint32_t v14 = pkStrings->m_usMaxSize;
	v41 = v12;
	if (v13 >= v14) {
		pkStrings->SetSize(v13 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v13, v41);
	char* ViewerString_5 = NiGetViewerString("NormalBinormalTangent", m_usDataFlags & NBT_METHOD_MASK);
	uint32_t v16 = pkStrings->m_usSize;
	uint32_t v17 = pkStrings->m_usMaxSize;
	v41 = ViewerString_5;
	if (v16 >= v17) {
		pkStrings->SetSize(v16 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v16, v41);
	char* v18 = m_kBound.GetViewerString("m_kBound");
	uint32_t v19 = pkStrings->m_usSize;
	uint32_t v20 = pkStrings->m_usMaxSize;
	v41 = v18;
	if (v19 >= v20) {
		pkStrings->SetSize(v19 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v19, v41);
	char* v21 = NiGetViewerString("m_pkColor", static_cast<const void*>(m_pkColor));
	uint32_t v22 = pkStrings->m_usSize;
	uint32_t v23 = pkStrings->m_usMaxSize;
	v41 = v21;
	if (v22 >= v23) {
		pkStrings->SetSize(v22 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v22, v41);
	char* v24 =
		NiGetViewerString("Num Texture Sets", static_cast<uint16_t>(m_usDataFlags & NiGeometryData::VERTEX_MASK));
	uint32_t v25 = pkStrings->m_usSize;
	v41 = v24;
	if (v25 >= pkStrings->m_usMaxSize) {
		pkStrings->SetSize(v25 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v25, v41);
	char* v26 = NiGetViewerString("m_pkTexture", static_cast<const void*>(m_pkTexture));
	uint32_t v27 = pkStrings->m_usSize;
	uint32_t v28 = pkStrings->m_usMaxSize;
	v41 = v26;
	if (v27 >= v28) {
		pkStrings->SetSize(v27 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v27, v41);
	char* v29 = NiGetViewerString("m_usDirtyFlags", m_usDirtyFlags);
	uint32_t v30 = pkStrings->m_usSize;
	uint32_t v31 = pkStrings->m_usMaxSize;
	v41 = v29;
	if (v30 >= v31) {
		pkStrings->SetSize(v30 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v30, v41);
	char* ViewerString_2 = NiGetViewerString("m_ucKeepFlags", m_ucKeepFlags);
	uint32_t v33 = pkStrings->m_usSize;
	v41 = ViewerString_2;
	if (v33 >= pkStrings->m_usMaxSize) {
		pkStrings->SetSize(v33 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v33, v41);
	char* v34 = NiGetViewerString("m_ucCompressFlags", m_ucCompressFlags);
	uint32_t v35 = pkStrings->m_usSize;
	uint32_t v36 = pkStrings->m_usMaxSize;
	v41 = v34;
	if (v35 >= v36) {
		pkStrings->SetSize(v35 + pkStrings->m_usGrowBy);
	}
	pkStrings->SetAt(v35, v41);
	NiAdditionalGeometryData* m_pObject = m_spAdditionalGeomData.m_pObject;
	if (m_pObject) {
		m_pObject->GetViewerStrings(pkStrings);
	} else {
		char* v38 = NiGetViewerString("m_spAdditionalGeomData", 0);
		uint32_t v39 = pkStrings->m_usSize;
		uint32_t v40 = pkStrings->m_usMaxSize;
		v41 = v38;
		if (v39 >= v40) {
			pkStrings->SetSize(v39 + pkStrings->m_usGrowBy);
		}
		pkStrings->SetAt(v39, v41);
	}
}

void NiGeometryData::SetBound(BSNiBound* apBound) {
	double m_fRadius;
	float z;
	float a2a;
	if (apBound) {
		m_fRadius = (double)m_kBound.m_fRadius;
		apBound->m_kCenter.x = m_kBound.m_kCenter.x;
		a2a = (float)m_fRadius;
		apBound->m_kCenter.y = m_kBound.m_kCenter.y;
		z = m_kBound.m_kCenter.z;
		apBound->m_fRadius = a2a;
		apBound->m_kCenter.z = z;
	}
}

// Bytes of the vertex arrays: normals triple with NBT data, and the exe masks the texture-set count with 1. The base
// is the exe's shared return-0 function (0xE7CD40), called out of line.
uint32_t NiGeometryData::GetBlockAllocationSize() const {
	uint32_t uiSize = m_pkVertex ? m_usVertices * sizeof(NiPoint3) : 0;
	if (m_pkNormal) {
		uiSize += m_usVertices * ((m_usDataFlags & NBT_METHOD_MASK) != 0 ? 3 : 1) * sizeof(NiPoint3);
	}
	if (m_pkColor) {
		uiSize += m_usVertices * sizeof(NiColorA);
	}
	if (m_pkTexture) {
		uiSize += m_usVertices * (m_usDataFlags & 1) * sizeof(NiPoint2);
	}
	return uiSize + NiObject::GetBlockAllocationSize();
}

// The exe masks the texture-set count with VERTEX_MASK (`and al, 1`), so of the texture coordinates only TEXCOORD0
// can be present.
bool NiGeometryData::ContainsVertexData(NiShaderDeclaration::ShaderParameter eParameter) const {
	if (m_pkBuffData && m_pkBuffData->ContainsVertexData(eParameter)) {
		return true;
	}
	switch (eParameter) {
	case NiShaderDeclaration::SHADERPARAM_NI_POSITION:
		return m_pkVertex != 0;
	case NiShaderDeclaration::SHADERPARAM_NI_NORMAL:
		return m_pkNormal != 0;
	case NiShaderDeclaration::SHADERPARAM_NI_COLOR:
		return m_pkColor != 0;
	case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0:
		return m_pkTexture && static_cast<uint8_t>(m_usDataFlags & NiGeometryData::VERTEX_MASK) >= 1;
	case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD1:
		return m_pkTexture && static_cast<uint8_t>(m_usDataFlags & NiGeometryData::VERTEX_MASK) >= 2;
	case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD2:
		return m_pkTexture && static_cast<uint8_t>(m_usDataFlags & NiGeometryData::VERTEX_MASK) >= 3;
	case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD3:
		return m_pkTexture && static_cast<uint8_t>(m_usDataFlags & NiGeometryData::VERTEX_MASK) >= 4;
	case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD4:
		return m_pkTexture && static_cast<uint8_t>(m_usDataFlags & NiGeometryData::VERTEX_MASK) >= 5;
	case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD5:
		return m_pkTexture && static_cast<uint8_t>(m_usDataFlags & NiGeometryData::VERTEX_MASK) >= 6;
	case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD6:
		return m_pkTexture && static_cast<uint8_t>(m_usDataFlags & NiGeometryData::VERTEX_MASK) >= 7;
	case NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD7:
		return m_pkTexture && static_cast<uint8_t>(m_usDataFlags & NiGeometryData::VERTEX_MASK) >= 8;
	case NiShaderDeclaration::SHADERPARAM_NI_TANGENT:
	case NiShaderDeclaration::SHADERPARAM_NI_BINORMAL:
		return m_pkNormal && (m_usDataFlags & NiGeometryData::NBT_METHOD_MASK) != 0;
	default:
		return false;
	}
}

void NiGeometryData::SetConsistency(NiGeometryData::Consistency eConsistency) {
	NiAdditionalGeometryData* m_pObject = m_spAdditionalGeomData.m_pObject;
	if (!m_pObject || !m_pObject->GetVBReady()) {
		m_usDirtyFlags = eConsistency | (m_usDirtyFlags & 0x8FFF);
	}
}

void NiGeometryData::MarkAsChanged(uint16_t usFlags) {
	NiAdditionalGeometryData* pkAdditionalData = m_spAdditionalGeomData.m_pObject;
	if (!pkAdditionalData || !pkAdditionalData->GetVBReady())
		m_usDirtyFlags = uint16_t(m_usDirtyFlags | (usFlags & 0x0FFF));
}

void NiGeometryData::GetVerticesIterator(NiGeometryDataIteratorNiPoint3& arIterator) {
	if (m_bVBLocked) {
		// The count is a word; the exe zeroes its whole 4-byte stack slot, the second word being the slot's padding.
		uint16_t ausCount[2];
		NiAdditionalGeometryData* pkAGD = m_spAdditionalGeomData.m_pObject;
		uint32_t uiStride = 0;
		ausCount[0] = 0;
		ausCount[1] = 0;
		uint8_t* pucData = 0;
		if (!pkAGD) {
			goto LABEL_6;
		}
		if (pkAGD->GetVBReady()) {
			uint32_t uiTotalSize;
			uint32_t uiType;
			uint32_t uiUnitSize = 0;
			pkAGD->GetDataStream(0, pucData, uiType, ausCount[0], uiTotalSize, uiUnitSize, uiStride);
		}
		if (pucData) {
			uint32_t v5 = uiStride;
			arIterator.m_pvCurrentPosition = (void*)pucData;
			arIterator.m_iStride = v5;
			arIterator.m_bCompressed = 0;
		} else {
		LABEL_6:;
			NiPoint3* m_pkVertex = this->m_pkVertex;
			arIterator.m_bCompressed = 0;
			arIterator.m_pvCurrentPosition = (void*)m_pkVertex;
			arIterator.m_iStride = 12;
		}
	}
}

void NiGeometryData::GetNormalsIterator(NiGeometryDataIteratorNiPoint3& arIterator) {
	if (m_bVBLocked) {
		// The count is a word; the exe zeroes its whole 4-byte stack slot, the second word being the slot's padding.
		uint16_t ausCount[2];
		NiAdditionalGeometryData* pkAGD = m_spAdditionalGeomData.m_pObject;
		uint32_t uiStride = 0;
		ausCount[0] = 0;
		ausCount[1] = 0;
		uint8_t* pucData = 0;
		if (!pkAGD) {
			goto LABEL_6;
		}
		if (pkAGD->GetVBReady()) {
			uint32_t uiTotalSize;
			uint32_t uiType;
			// Only the packed data answers GetVBReady with true; skinned data has its two blend streams first.
			uint32_t v6 = 2 *
					(static_cast<BSPackedAdditionalGeometryData*>(pkAGD)->m_uiShaderDeclarationIndex &
						BSPackedAdditionalGeometryData::SD_SKINNED) |
				1;
			uint32_t uiUnitSize = 0;
			pkAGD->GetDataStream(v6, pucData, uiType, ausCount[0], uiTotalSize, uiUnitSize, uiStride);
		}
		if (pucData) {
			uint32_t v5 = uiStride;
			arIterator.m_pvCurrentPosition = (void*)pucData;
			arIterator.m_iStride = v5;
			arIterator.m_bCompressed = 0;
		} else {
		LABEL_6:;
			NiPoint3* m_pkNormal = this->m_pkNormal;
			arIterator.m_bCompressed = 0;
			arIterator.m_pvCurrentPosition = (void*)m_pkNormal;
			arIterator.m_iStride = 12;
		}
	}
}

// usSet is unused: the exe walks the first texture set only.
void NiGeometryData::GetTextureSetIterator(uint16_t usSet, NiGeometryDataIteratorNiPoint2& arIterator) {
	if (m_bVBLocked) {
		// The count is a word; the exe zeroes its whole 4-byte stack slot, the second word being the slot's padding.
		uint16_t ausCount[2];
		NiAdditionalGeometryData* pkAGD = m_spAdditionalGeomData.m_pObject;
		uint32_t uiStride = 0;
		ausCount[0] = 0;
		ausCount[1] = 0;
		uint8_t* pucData = 0;
		if (!pkAGD) {
			goto LABEL_6;
		}
		if (pkAGD->GetVBReady()) {
			uint32_t uiTotalSize;
			uint32_t uiType;
			// Only the packed data answers GetVBReady with true; the texture stream follows the position and normal
			// streams, the two blend streams of skinned data and the colour stream.
			uint32_t uiFlags = static_cast<BSPackedAdditionalGeometryData*>(pkAGD)->m_uiShaderDeclarationIndex;
			uint32_t uiUnitSize = 0;
			pkAGD->GetDataStream((uiFlags >> 1 & 1) +
					(2 * ((uiFlags & BSPackedAdditionalGeometryData::SD_SKINNED) != 0) + 2),
				pucData, uiType, ausCount[0], uiTotalSize, uiUnitSize, uiStride);
		}
		if (pucData) {
			uint32_t v7 = uiStride;
			arIterator.m_pvCurrentPosition = (void*)pucData;
			arIterator.m_iStride = v7;
			arIterator.m_bCompressed = 0;
		} else {
		LABEL_6:;
			NiPoint2* m_pkTexture = this->m_pkTexture;
			arIterator.m_bCompressed = 0;
			arIterator.m_pvCurrentPosition = (void*)m_pkTexture;
			arIterator.m_iStride = 8;
		}
	}
}

void NiGeometryData::GetBlendWeightsIterator(NiGeometryDataIteratorNiBlendWeights& arIterator) {
	if (m_bVBLocked) {
		// The count is a word; the exe zeroes its whole 4-byte stack slot, the second word being the slot's padding.
		uint16_t ausCount[2];
		NiAdditionalGeometryData* pkAGD = m_spAdditionalGeomData.m_pObject;
		uint32_t uiStride = 0;
		ausCount[0] = 0;
		ausCount[1] = 0;
		uint8_t* pucData = 0;
		// The exe tests BSPackedAdditionalGeometryData's flags (+0x2C) on any additional data whose VB is ready:
		// only the packed class reports a ready VB.
		if (pkAGD && pkAGD->GetVBReady() &&
			(static_cast<BSPackedAdditionalGeometryData*>(pkAGD)->m_uiShaderDeclarationIndex &
				BSPackedAdditionalGeometryData::SD_SKINNED) != 0) {
			uint32_t uiTotalSize;
			uint32_t uiType;
			uint32_t uiUnitSize = 0;
			pkAGD->GetDataStream(1u, pucData, uiType, ausCount[0], uiTotalSize, uiUnitSize, uiStride);
		}
		arIterator.m_pvCurrentPosition = (void*)pucData;
		arIterator.m_iStride = uiStride;
		arIterator.m_bCompressed = 0;
	}
}

void NiGeometryData::GetBlendIndicesIterator(NiGeometryDataIteratorNiBlendIndices& arIterator) {
	if (m_bVBLocked) {
		// The count is a word; the exe zeroes its whole 4-byte stack slot, the second word being the slot's padding.
		uint16_t ausCount[2];
		NiAdditionalGeometryData* pkAGD = m_spAdditionalGeomData.m_pObject;
		uint32_t uiStride = 0;
		ausCount[0] = 0;
		ausCount[1] = 0;
		uint8_t* pucData = 0;
		// Only the packed data answers GetVBReady with true; only skinned packed data has blend indices.
		if (pkAGD && pkAGD->GetVBReady() &&
			(static_cast<BSPackedAdditionalGeometryData*>(pkAGD)->m_uiShaderDeclarationIndex &
				BSPackedAdditionalGeometryData::SD_SKINNED) != 0) {
			uint32_t uiTotalSize;
			uint32_t uiType;
			uint32_t uiUnitSize = 0;
			pkAGD->GetDataStream(2u, pucData, uiType, ausCount[0], uiTotalSize, uiUnitSize, uiStride);
		}
		uint32_t v3 = uiStride;
		arIterator.m_pvCurrentPosition = (void*)pucData;
		arIterator.m_iStride = v3;
		arIterator.m_bCompressed = 0;
	}
}

NiAdditionalGeometryData* NiGeometryData::GetAdditionalGeometryData() const {
	return (&m_spAdditionalGeomData)->operator->();
}

void NiGeometryData::SetAdditionalGeometryData(NiAdditionalGeometryData* pkAddData) {
	m_spAdditionalGeomData = pkAddData;
}

void NiGeometryData::ReplaceData(uint16_t usVertices, NiPoint3* pkVertex, NiPoint3* pkNormal, NiColorA* pkColor,
	NiPoint2* pkTexture, uint16_t usNumTextureSets, DataFlags eNBTMethod) {
	m_usVertices = usVertices;
	m_pkVertex = pkVertex;
	m_pkNormal = pkNormal;
	m_pkColor = pkColor;
	uint16_t usDataFlags = m_usDataFlags;
	m_pkTexture = pkTexture;
	m_usDataFlags = uint16_t((usDataFlags & 0x0FFE) | (usNumTextureSets & 1) | eNBTMethod);
}

bool NiGeometryData::LockPackedVertexData(bool bWriteAccess) {
	if (m_bVBLocked)
		return false;

	NiAdditionalGeometryData* pkAdditionalData = m_spAdditionalGeomData.m_pObject;
	if (pkAdditionalData && pkAdditionalData->GetVBReady()) {
		if (bWriteAccess) {
			pkAdditionalData->UnlockDataBlock(0);
			m_bVBLockWrite = bWriteAccess;
			m_bVBLocked = true;
			return true;
		}
		uint32_t uiBlockSize = 0;
		pkAdditionalData->LockDataBlock(0, uiBlockSize);
	}
	m_bVBLockWrite = bWriteAccess;
	m_bVBLocked = true;
	return true;
}

void NiGeometryData::UnlockPackedVertexData() {
	if (m_bVBLocked) {
		NiAdditionalGeometryData* pkAdditionalData = m_spAdditionalGeomData.m_pObject;
		if (pkAdditionalData && pkAdditionalData->GetVBReady() && !m_bVBLockWrite)
			pkAdditionalData->UnlockDataBlock(0);
		m_bVBLocked = false;
	}
}

void NiGeometryData::Replace(uint16_t usVertices, NiPoint3* pkVertex, NiPoint3* pkNormal, NiColorA* pkColor,
	NiPoint2* pkTexture, uint16_t usNumTextureSets, NiGeometryData::DataFlags eNBTMethod) {
	if (GetGroup()) {
		if (pkVertex != m_pkVertex) {
			NiObjectGroup* pkGroup = GetGroup();
			if (pkGroup->m_uiRefCount-- == 1) {
				pkGroup->FreeVertexBuffer();
				NiMemObject::operator delete(pkGroup, 16u);
			}
		}
		if (pkNormal != m_pkNormal) {
			NiObjectGroup* pkGroup = GetGroup();
			if (pkGroup->m_uiRefCount-- == 1) {
				pkGroup->FreeVertexBuffer();
				NiMemObject::operator delete(pkGroup, 16u);
			}
		}
		if (pkColor != m_pkColor) {
			NiObjectGroup* pkGroup = GetGroup();
			if (pkGroup->m_uiRefCount-- == 1) {
				pkGroup->FreeVertexBuffer();
				NiMemObject::operator delete(pkGroup, 16u);
			}
		}
		if (pkTexture != m_pkTexture) {
			NiObjectGroup* pkGroup = GetGroup();
			if (pkGroup->m_uiRefCount-- == 1) {
				pkGroup->FreeVertexBuffer();
				NiMemObject::operator delete(pkGroup, 16u);
			}
		}
	} else {
		if (pkVertex != m_pkVertex) {
			MemoryManager_Singleton.Deallocate(m_pkVertex);
		}
		if (pkNormal != m_pkNormal) {
			MemoryManager_Singleton.Deallocate(m_pkNormal);
		}
		if (pkColor != m_pkColor) {
			MemoryManager_Singleton.Deallocate(m_pkColor);
		}
		if (pkTexture != m_pkTexture) {
			MemoryManager_Singleton.Deallocate(m_pkTexture);
		}
	}
	m_pkVertex = pkVertex;
	m_usVertices = usVertices;
	m_pkNormal = pkNormal;
	uint16_t usActiveVertices = GetActiveVertexCount();
	if (usActiveVertices) {
		if (m_pkVertex) {
			m_kBound.ComputeFromData(usActiveVertices, m_pkVertex);
		}
	}
	uint16_t usDataFlags = m_usDataFlags;
	m_pkColor = pkColor;
	m_pkTexture = pkTexture;
	m_usDataFlags = eNBTMethod | ((usNumTextureSets & 1) | (usDataFlags & 4094));
}

void NiGeometryData::CreateNormals(bool bInitialize) {
	int32_t v3 = 1;
	if ((m_usDataFlags & 61440) != 0) {
		v3 = 3;
	}
	if (!m_pkNormal) {
		m_pkNormal = static_cast<NiPoint3*>((&MemoryManager_Singleton)
				->Allocate((((12 * ((uint64_t)(v3 * ((uint32_t)m_usVertices)))) >> 32) != 0)
						? ((-1))
						: (12 * (v3 * m_usVertices))));
	}
	if (bInitialize) {
		std::memset(m_pkNormal, 0, sizeof(NiPoint3) * (v3 * m_usVertices));
	}
}

// The asm zeroes one NiPoint3 past the copied normals, not the two vertex arrays of normals the reference source
// memsets. The allocation size is the exe's 32x32 multiply with its overflow test.
void NiGeometryData::StoreNormalBinormalTangent(DataFlags eNBTMethod) {
	if ((m_usDataFlags & NBT_METHOD_MASK) == eNBTMethod) {
		return;
	}
	m_usDataFlags = static_cast<uint16_t>(eNBTMethod | (m_usDataFlags & 0xFFF));
	if (!m_pkNormal) {
		return;
	}
	uint32_t uiFactor = 1;
	if (eNBTMethod) {
		uiFactor = 3;
	}
	NiPoint3* pkNormal = static_cast<NiPoint3*>((&MemoryManager_Singleton)
			->Allocate((((12 * ((uint64_t)(uiFactor * ((uint32_t)m_usVertices)))) >> 32) != 0)
					? ((-1))
					: (12 * (uiFactor * m_usVertices))));
	for (uint32_t uiIndex = 0; uiIndex < m_usVertices; ++uiIndex) {
		pkNormal[uiIndex] = m_pkNormal[uiIndex];
	}
	if (eNBTMethod) {
		pkNormal[m_usVertices].x = 0.0f;
		pkNormal[m_usVertices].y = 0.0f;
		pkNormal[m_usVertices].z = 0.0f;
	}
	(&MemoryManager_Singleton)->Deallocate(m_pkNormal);
	m_pkNormal = pkNormal;
}

void NiGeometryData::SetKeepFlags(uint32_t uiKeepFlags) {
	m_ucKeepFlags = (uint8_t)uiKeepFlags;
}

void NiGeometryData::SetData(uint16_t usVertices, NiPoint3* pkVertex, NiPoint3* pkNormal, NiColorA* pkColor,
	NiPoint2* pkTexture, uint16_t usNumTextureSets, NiGeometryData::DataFlags eNBTMethod) {
	m_usVertices = usVertices;
	m_pkVertex = pkVertex;
	m_pkNormal = pkNormal;
	uint16_t usActiveVertices = GetActiveVertexCount();
	if (usActiveVertices) {
		if (m_pkVertex) {
			m_kBound.ComputeFromData(usActiveVertices, m_pkVertex);
		}
	}
	uint16_t usDataFlags = m_usDataFlags;
	m_pkTexture = pkTexture;
	m_pkColor = pkColor;
	m_usDataFlags = eNBTMethod | ((usNumTextureSets & 1) | (usDataFlags & 4094));
}

// Orders two vertices by the arrays mask selects: positions, normals, colours, texture coordinates, each compared
// float by float (v1's against v0's).
int32_t NiGeometryData::VertexCompare(uint16_t v0, uint16_t v1, int32_t mask) const {
	NiPoint3* pkVertex = m_pkVertex;
	if ((mask & 1) != 0) {
		const float* pf1 = &pkVertex[v1].x;
		const float* pf0 = &pkVertex[v0].x;
		for (uint32_t i = 0; i < 3; ++i) {
			if (pf1[i] > pf0[i]) {
				return -1;
			}
			if (pf1[i] < pf0[i]) {
				return 1;
			}
		}
	}
	NiPoint3* pkNormal = m_pkNormal;
	if (pkNormal && (mask & 2) != 0) {
		int32_t iCmp = NiGeometryData::FloatCompare(&pkNormal[v1].x, &pkNormal[v0].x, 3);
		if (iCmp) {
			return iCmp;
		}
	}
	NiColorA* pkColor = m_pkColor;
	if (pkColor && (mask & 4) != 0) {
		int32_t iCmp = NiGeometryData::FloatCompare(&pkColor[v1].r, &pkColor[v0].r, 4);
		if (iCmp) {
			return iCmp;
		}
	}
	NiPoint2* pkTexture = m_pkTexture;
	if (pkTexture && (mask & 8) != 0) {
		uint32_t uiVertices = m_usVertices;
		uint16_t usSets = m_usDataFlags & NiGeometryData::VERTEX_MASK;
		uint16_t usBase = 0;
		for (uint16_t usSet = 0; usSet < usSets; ++usSet) {
			const float* pf1 = &pkTexture[usBase + v1].x;
			const float* pf0 = &pkTexture[usBase + v0].x;
			for (uint32_t i = 0; i < 2; ++i) {
				if (pf1[i] > pf0[i]) {
					return -1;
				}
				if (pf1[i] < pf0[i]) {
					return 1;
				}
			}
			usBase += uiVertices;
		}
	}
	return 0;
}

// The exe passes the caller's whole mask dword on to VertexCompare (push ebx).
int32_t NiGeometryData::ChoosePivot(uint16_t* pusIndex, int32_t mask, int32_t l, int32_t r) const {
	uint16_t v6 = pusIndex[(l + r) >> 1];
	uint16_t la = pusIndex[l];
	if (VertexCompare(la, v6, mask) < 0) {
		uint16_t v7 = pusIndex[r];
		if (VertexCompare(v6, v7, mask) >= 0) {
			if (VertexCompare(la, v7, mask) >= 0) {
				return la;
			} else {
				return v7;
			}
		}
		return v6;
	}
	uint16_t v9 = pusIndex[r];
	if (VertexCompare(la, v9, mask) < 0) {
		return la;
	}
	bool v10 = VertexCompare(v6, v9, mask) < 0;
	int32_t result = v9;
	if (!v10) {
		return v6;
	}
	return result;
}

void NiGeometryData::IndexQSort(uint16_t* pusIndex, int32_t mask, int32_t l, int32_t r) const {
	int32_t v5 = l;
	int32_t v7 = r;
	if (r > l) {
		while (1) {
			int32_t v9 = v5 - 1;
			int32_t v10 = v7 + 1;
			uint16_t pusIndexa = ChoosePivot(pusIndex, mask, v5, v7);
			while (1) {
				uint16_t v11;
				uint16_t v12;
				do {
					v11 = pusIndex[--v10];
				} while (VertexCompare(pusIndexa, v11, mask) < 0);
				do {
					v12 = pusIndex[++v9];
				} while (VertexCompare(v12, pusIndexa, mask) < 0);
				if (v9 >= v10) {
					break;
				}
				uint16_t v13 = pusIndex[v9];
				pusIndex[v9] = pusIndex[v10];
				pusIndex[v10] = v13;
			}
			if (v10 == r) {
				r = v10 - 1;
			} else {
				IndexQSort(pusIndex, mask, l, v10);
				l = v10 + 1;
			}
			if (r <= l) {
				break;
			}
			v7 = r;
			v5 = l;
		}
	}
}

void NiGeometryData::IndexSort(uint16_t*& pusIndex0, uint16_t*& pusIndex1, int32_t mask) const {
	uint16_t usVertices = m_usVertices;
	pusIndex0 = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * usVertices));
	pusIndex1 = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * usVertices));
	for (uint16_t i = 0; i < usVertices; i++) {
		pusIndex0[i] = i;
	}
	IndexQSort(pusIndex0, mask, 0, usVertices - 1);
	for (uint16_t i = 0; i < usVertices; i++) {
		pusIndex1[pusIndex0[i]] = i;
	}
}

void NiGeometryData::GetColorsIterator(NiGeometryDataIteratorNiColorA& arIterator) {
	if (m_bVBLocked) {
		void* pvPosition;
		uint32_t uiIterStride;
		// The count is a word; the exe zeroes its whole 4-byte stack slot, the second word being the slot's padding.
		uint16_t ausCount[2];
		NiAdditionalGeometryData* pkAGD = m_spAdditionalGeomData.m_pObject;
		bool bCompressed = false;
		uint32_t uiStride = 0;
		ausCount[0] = 0;
		ausCount[1] = 0;
		uint8_t* pucData = 0;
		if (!pkAGD) {
			goto LABEL_7;
		}
		if (pkAGD->GetVBReady()) {
			uint32_t uiTotalSize;
			uint32_t uiType;
			// Only the packed data answers GetVBReady with true; its shader declaration flags say which streams it has.
			uint32_t uiFlags = static_cast<BSPackedAdditionalGeometryData*>(pkAGD)->m_uiShaderDeclarationIndex;
			if ((uiFlags & BSPackedAdditionalGeometryData::SD_COLOR) == 0) {
				return;
			}
			uint32_t uiUnitSize = 0;
			pkAGD->GetDataStream(2 * ((uiFlags & BSPackedAdditionalGeometryData::SD_SKINNED) != 0) + 2, pucData, uiType,
				ausCount[0], uiTotalSize, uiUnitSize, uiStride);
			bCompressed = (uiFlags & BSPackedAdditionalGeometryData::SD_COMP_COLORS) != 0;
		}
		pvPosition = (void*)pucData;
		if (pucData) {
			uiIterStride = uiStride;
		} else {
		LABEL_7:;
			pvPosition = (void*)this->m_pkColor;
			uiIterStride = 16;
		}
		arIterator.m_pvCurrentPosition = pvPosition;
		arIterator.m_iStride = uiIterStride;
		arIterator.m_bCompressed = bCompressed;
	}
}

void NiGeometryData::ClearNormals() {
	if (m_pkNormal) {
		MemoryManager_Singleton.Deallocate(m_pkNormal);
		m_pkNormal = 0;
	}
}

int32_t NiGeometryData::FloatCompare(const float* pf0, const float* pf1, uint32_t uiNum) {
	for (uint32_t i = 0; i < uiNum; ++i) {
		if (pf0[i] > pf1[i]) {
			return -1;
		}
		if (pf0[i] < pf1[i]) {
			return 1;
		}
	}
	return 0;
}

// Gamebryo's NiStreamLoadBinary of the floats, inlined: one read of every value, 4-byte components.
void NiPoint2::LoadBinary(NiStream& stream, NiPoint2* pkValues, uint32_t uiNumValues) {
	uint32_t uiSize = sizeof(float);
	// EXE-INLINED none: NiStreamLoadBinary
	stream.Istr()->BinaryRead(pkValues, sizeof(NiPoint2) * uiNumValues, &uiSize, 1);
}
