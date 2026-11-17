#include "Fallout Shared/TESObjectTREE.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderTextureSet.h"
#include "BSShader/DistantLODShaderProperty.h"
#include "BSShader/SpeedTreeBillboardShaderProperty.h"
#include "BSSystem/Archive.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIconTree.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTree.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTexture1024.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

// Byte swaps every field, in field order.
void OBJ_TREE::Endian() {
	ENDIAN(fCurveScalar, false);
	ENDIAN(fMinimumLeafAngle, false);
	ENDIAN(fMaximumLeafAngle, false);
	ENDIAN(fBranchDimming, false);
	ENDIAN(fLeafDimming, false);
	ENDIAN(iCanopyShadowRadius, false);
	ENDIAN(fRockSpeed, false);
	ENDIAN(fRustleSpeed, false);
}

TREE_BILLBOARD_DATA::TREE_BILLBOARD_DATA(TESObjectTREE* apTree, uint32_t aiCellChunk, uint32_t aiCellKey,
	NiNode* apInstancedNode, uint32_t aiArraySize, NiPoint3* apLocArray, float* apColorArray)
	: pTree(apTree), iCellChunk(aiCellChunk), iCellKey(aiCellKey), pInstancedNode(apInstancedNode),
	  iArraySize(aiArraySize), pLocArray(static_cast<NiPoint3*>(ourMalloc(sizeof(NiPoint3) * aiArraySize))),
	  pColorArray(static_cast<float*>(ourMalloc(sizeof(float) * aiArraySize))) {
	ourMemcpy(pLocArray, apLocArray, sizeof(NiPoint3) * iArraySize);
	ourMemcpy(pColorArray, apColorArray, sizeof(float) * iArraySize);
}

TREE_BILLBOARD_DATA::~TREE_BILLBOARD_DATA() {
	ourFree(pLocArray);
	ourFree(pColorArray);
}

TESObjectTREE::TESObjectTREE() : SeedArray(0, 1), BillboardSize(NiPoint2::ZERO) {
	cFormType = TREE_ID;
	Data.fCurveScalar = 2.5f;
	Data.fMinimumLeafAngle = 5.0f;
	Data.fMaximumLeafAngle = 85.0f;
	Data.fBranchDimming = 0.5f;
	Data.fLeafDimming = 0.7f;
	Data.fRockSpeed = 1.0f;
	Data.fRustleSpeed = 1.0f;
	Data.iCanopyShadowRadius = 128;
}

TESObjectTREE::~TESObjectTREE() {
	TESForm::InitializeDataComponent();
}

// Reads the TREE record's chunks; only a TREE record (type 0x25) is accepted.
bool TESObjectTREE::Load(TESFile* apFile) {
	if ((uint8_t)apFile->GetTESForm() != TREE_ID) {
		return false;
	}
	LoadForm(apFile);
	for (;;) {
		CHUNK_ID eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
		if (eChunk == NO_CHUNK) {
			break;
		}
		switch (eChunk) {
		case CNAM_ID: {
			uint32_t uiChunkSize = apFile->GetChunkSize();
			if (uiChunkSize == 0x20) {
				apFile->GetChunkData(&Data, uiChunkSize);
				if (apFile->QEndian()) {
					Data.Endian();
				}
				if (Data.iCanopyShadowRadius < 0) {
					Data.iCanopyShadowRadius = 128;
				}
			}
			break;
		}
		case MODL_ID:
		case MODT_ID:
			TESModel::LoadModelChunk(static_cast<TESModelTree*>(this), apFile);
			break;
		case ICON_ID:
			TESTexture::LoadTextureChunk(static_cast<TESIconTree*>(this), apFile);
			break;
		case DEST_ID:
		case DSTD_ID:
			BGSDestructibleObjectForm::LoadChunk(static_cast<BGSDestructibleObjectForm*>(this), apFile);
			break;
		case OBND_ID: {
			TESBoundObject* pObject = this;
			pObject->LoadObjectBound(apFile);
			break;
		}
		case EDID_ID: {
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			uint32_t uiChunkSize = apFile->GetChunkSize();
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
			apFile->GetChunkData(pBuffer, 0x200);
			TESForm* pForm = this;
			pForm->SetFormEditorID(pBuffer);
			break;
		}
		case SNAM_ID: {
			uint32_t uiChunkSize = apFile->GetChunkSize();
			if (uiChunkSize == 0) {
				break;
			}
			if ((uiChunkSize % 4) != 0) {
				break;
			}
			uint32_t uiCount = uiChunkSize >> 2;
			uint32_t* pSeeds = (uint32_t*)ourMalloc(sizeof(uint32_t) * uiCount);
			ourMemset(pSeeds, 0, uiChunkSize);
			apFile->GetChunkData(pSeeds, uiChunkSize);
			SeedArray.RemoveAll();
			for (uint32_t i = 0; i < uiCount; ++i) {
				if (apFile->QEndian()) {
					ENDIAN(pSeeds[i], false);
				}
				if (pSeeds[i] > 0) {
					SeedArray.Add(pSeeds[i]);
				}
			}
			ourFree(pSeeds);
			break;
		}
		case BNAM_ID: {
			uint32_t uiChunkSize = apFile->GetChunkSize();
			if (uiChunkSize == 8) {
				apFile->GetChunkData(&BillboardSize, uiChunkSize);
				if (apFile->QEndian()) {
					ENDIAN(BillboardSize.x, false);
					ENDIAN(BillboardSize.y, false);
				}
			}
			break;
		}
		default:
			break;
		}
		if (!apFile->NextChunk()) {
			break;
		}
	}
	return true;
}

void TESObjectTREE::Save() {
	StartForm();
	TESModel::Save(MODL_ID, MODT_ID);
	TESTexture::Save(ICON_ID);
	BGSDestructibleObjectForm::Save();
	if (SeedArray.m_usSize) {
		uint32_t inumSeeds = SeedArray.m_usSize;
		uint32_t* pdata =
			static_cast<uint32_t*>(ourMalloc(inumSeeds <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * inumSeeds
																					  : SIZE_MAX));
		ourMemset(pdata, 0, sizeof(uint32_t) * inumSeeds);
		for (uint32_t i = 0; i < inumSeeds; ++i) {
			uint32_t ival = SeedArray.operator[](i);
			if (ival) {
				pdata[i] = ival;
			}
		}
		TESForm::AddChunkArray(SNAM_ID, pdata, inumSeeds);
	}
	if (Data.iCanopyShadowRadius < 0) {
		Data.iCanopyShadowRadius = 128;
	}
	if (TESForm::QEndianSwapOnSave()) {
		Data.Endian();
	}
	TESForm::__AddChunkData(CNAM_ID, &Data, 32u);
	if (TESForm::QEndianSwapOnSave()) {
		Data.Endian();
	}
	// The PDB's float overload (folded into 0x485710): the billboard's width and height.
	TESForm::AddChunkArray(BNAM_ID, &BillboardSize.x, 2u);
	CloseForm();
}

void TESObjectTREE::InitItem() {
	if (!TESForm::GetInitialized()) {
		static_cast<BGSDestructibleObjectForm*>(this)->InitItem(this);
		TESForm::SetInitialized(1);
	}
}

void TESObjectTREE::Copy(TESForm* apCopy) {
	TESObjectTREE* ptree = dynamic_cast<TESObjectTREE*>(apCopy);
	if (ptree) {
		CopyAllComponents(apCopy);
		uint32_t isize = ptree->SeedArray.m_usSize;
		SeedArray.RemoveAll();
		for (uint32_t i = 0; i < isize; ++i) {
			SeedArray.AddFirstEmpty(ptree->SeedArray.operator[](i));
		}
		Data = *ptree->GetData();
		float y = ptree->BillboardSize.y;
		BillboardSize.x = ptree->BillboardSize.x;
		BillboardSize.y = y;
	}
}

bool TESObjectTREE::Compare(TESForm* apCompare) {
	TESObjectTREE* pcomp = dynamic_cast<TESObjectTREE*>(apCompare);
	if (!pcomp) {
		return 1;
	}
	OBJ_TREE* v3 = pcomp->GetData();
	if (std::memcmp(&Data, v3, sizeof(Data))) {
		return 1;
	}
	if (BillboardSize.operator!=(pcomp->BillboardSize)) {
		return 1;
	}
	uint32_t isize = SeedArray.m_usSize;
	if (isize != pcomp->SeedArray.m_usSize) {
		return 1;
	}
	for (uint32_t i = 0; i < isize; ++i) {
		uint32_t* v4 = &SeedArray.operator[](i);
		if (*v4 != pcomp->SeedArray.operator[](i)) {
			return 1;
		}
	}
	return CompareAllComponents(pcomp);
}

uint8_t TESObjectTREE::GetIndexForSeed(uint32_t aiSeed) {
	uint32_t i;
	uint8_t v5 = (uint8_t)((-1));
	for (i = 0; i < ((uint32_t)((int32_t)(&SeedArray)->NiTArray<uint32_t, NiTMallocInterface<uint32_t>>::GetSize()));
		++i) {
		if ((&SeedArray)->NiTArray<uint32_t, NiTMallocInterface<uint32_t>>::operator[](i) == aiSeed) {
			return (uint8_t)i;
		}
	}
	return v5;
}

uint32_t TESObjectTREE::GetSeedAtIndex(uint8_t aiIndex) {
	if ((aiIndex == 255) || (!(&SeedArray)->NiTArray<uint32_t, NiTMallocInterface<uint32_t>>::GetSize())) {
		return GetRandomSeed();
	}
	if (((uint32_t)((int32_t)aiIndex)) >= (&SeedArray)->NiTArray<uint32_t, NiTMallocInterface<uint32_t>>::GetSize()) {
		aiIndex %= (&SeedArray)->NiTArray<uint32_t, NiTMallocInterface<uint32_t>>::GetSize();
	}
	return (&SeedArray)->NiTArray<uint32_t, NiTMallocInterface<uint32_t>>::operator[](aiIndex);
}

uint32_t TESObjectTREE::GetRandomSeed() {
	uint32_t size = (&SeedArray)->NiTArray<uint32_t, NiTMallocInterface<uint32_t>>::GetSize();
	if (!size) {
		return (uint32_t)0;
	}
	uint32_t v3 = Random() % size;
	return (&SeedArray)->NiTArray<uint32_t, NiTMallocInterface<uint32_t>>::operator[](v3);
}

float TESObjectTREE::GetCurveScalar() {
	return Data.fCurveScalar;
}

void TESObjectTREE::SetCurveScalar(float afCurveScalar) {
	Data.fCurveScalar = afCurveScalar;
}

void TESObjectTREE::SetMinimumLeafAngle(float afAngle) {
	Data.fMinimumLeafAngle = afAngle;
}

float TESObjectTREE::GetMaximumLeafAngle() {
	return Data.fMaximumLeafAngle;
}

void TESObjectTREE::SetMaximumLeafAngle(float afAngle) {
	Data.fMaximumLeafAngle = afAngle;
}

float TESObjectTREE::GetBranchDimming() {
	return Data.fBranchDimming;
}

void TESObjectTREE::SetBranchDimming(float afDimming) {
	Data.fBranchDimming = afDimming;
}

float TESObjectTREE::GetLeafDimming() {
	return Data.fLeafDimming;
}

void TESObjectTREE::SetLeafDimming(float afDimming) {
	Data.fLeafDimming = afDimming;
}

float TESObjectTREE::GetShadowRadius() {
	return static_cast<float>(Data.iCanopyShadowRadius);
}

void TESObjectTREE::SetShadowRadius(float aiRadius) {
	Data.iCanopyShadowRadius = (int32_t)aiRadius;
}

float TESObjectTREE::GetRockSpeed() {
	return Data.fRockSpeed;
}

void TESObjectTREE::SetRockSpeed(float afRockSpeed) {
	Data.fRockSpeed = afRockSpeed;
}

float TESObjectTREE::GetRustleSpeed() {
	return Data.fRustleSpeed;
}

void TESObjectTREE::SetRustleSpeed(float afRustleSpeed) {
	Data.fRustleSpeed = afRustleSpeed;
}

// The exe's cleanup-only C++ EH frame is left out.
void TESObjectTREE::CreateDistant3D(NiPoint3* apLocArray, NiPoint3* apRotArray, float* apColorArray,
	uint32_t aiArraySize, uint32_t aiCellChunk, uint32_t aiCellKey, NiNode* apInstancedNode, NiNode* apFullNode) {
	FNVD_INLINED_LOCAL(NiPointer<NiTexture>, pkSelf, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(AutoMemContext, apMemory, std::nullopt, std::nullopt);
	::new (&apMemory) AutoMemContext(MEM_CONTEXT_LODLANDSCAPE, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectTREE.cpp", 552);
	if (!apInstancedNode ||
		(!apLocArray ||
			(!apColorArray || (!aiArraySize || (!SpeedTree::bEnableTrees::Get() || !IsLargeEnoughForDistantLOD()))))) {
		apMemory.~AutoMemContext();
		return;
	}
	NiBillboardNode* v18 = 0;
	uint32_t CastingType = iFormID;
	if (!DistantLODShaderProperty::HasCachedGeometry(CastingType)) {
		char cFormattedPath[268];
		char cTexturePath[264];
		GetBillboardTextureFilename(cTexturePath);
		StandardizeTexturePath(cTexturePath, cFormattedPath, 260u);
		::new (&pkSelf) NiPointer<NiTexture>(nullptr);
		BSTexturePalette::GetTexture(cFormattedPath, pkSelf);
		if (!pkSelf.m_pObject) {
			TREE_BILLBOARD_DATA* v10;
			TREE_BILLBOARD_DATA* v12 = static_cast<TREE_BILLBOARD_DATA*>(ourMalloc(sizeof(TREE_BILLBOARD_DATA)));
			if (v12) {
				v10 = ::new (v12) TREE_BILLBOARD_DATA(this, aiCellChunk, aiCellKey, apInstancedNode, aiArraySize,
					apLocArray, apColorArray);
			} else {
				v10 = 0;
			}
			pModelLoader->QueueTreeBillboard(cFormattedPath, v10);
			pkSelf.~NiPointer<NiTexture>();
			apMemory.~AutoMemContext();
			return;
		}
		v18 = static_cast<NiBillboardNode*>(BuildDistant3D(1));
		if (!v18) {
			pkSelf.~NiPointer<NiTexture>();
			apMemory.~AutoMemContext();
			return;
		}
		pkSelf.~NiPointer<NiTexture>();
	}
	FNVD_CONSTRUCTED_LOCAL(DistantLODShaderProperty::DistantLODParam, apParam, nullptr);
	apParam.iBaseObjectID = iFormID;
	apParam.p3DData = v18;
	apParam.pcGeomFilename = 0;
	DistantLODShaderProperty::AddDistantLOD(aiCellChunk, aiCellKey, apInstancedNode, &apParam, apLocArray, apColorArray,
		aiArraySize);
	if (apParam.p3DData) {
		delete apParam.p3DData;
	}
	apMemory.~AutoMemContext();
}

bool TESObjectTREE::IsLargeEnoughForDistantLOD() {
	return (bool)((uint8_t)(((!(BillboardSize.x > 0.0)) || (!(BillboardSize.x <= 200.0))) &&
		((!(BillboardSize.y > 0.0)) || (!(BillboardSize.y <= 350.0)))));
}

void TESObjectTREE::GetBillboardTextureFilename(char* apOutString) {
	const char* pmodel;
	for (const char* ppath = "Textures\\Trees\\Billboards"; *ppath; ++ppath) {
		*apOutString++ = *ppath;
	}
	for (pmodel = GetModel(); *pmodel != '.' && *pmodel; ++pmodel) {
		*apOutString++ = *pmodel;
	}
	if (*pmodel == '.') {
		*apOutString++ = '.';
		*apOutString++ = 'd';
		*apOutString++ = 'd';
		*apOutString++ = 's';
	}
	*apOutString = 0;
}

// The exe's cleanup-only C++ EH frame is left out.
void TESObjectTREE::CreateBillboardGeomData(bool abZUp, NiPointer<NiTriShapeData>& aspTriShapeData) {
	NiPoint3* v7;
	void* v31;
	NiColorA* pcol;
	NiPoint2* ptext;
	NiPoint3* pnorm_2;
	NiPoint3* pverts;
	FNVD_INLINED_LOCAL(NiColorA, v38, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiColorA, v39, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiColorA, v40, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiColorA, v41, std::nullopt, std::nullopt);
	NiPoint2 v44;
	NiPoint2 v45;
	NiPoint2 v46;
	NiPoint2 v47;
	NiPoint3 v52;
	NiPoint3 v53;
	NiPoint3 v54;
	NiPoint3 v55;
	NiPoint3 v56;
	NiPoint3 dst;
	float STBBNBT[36];
	FNVD_INLINED_LOCAL(NiPointer<NiTriShapeData>, spgeomData, std::nullopt, std::nullopt);
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, tempMemContext, &AutoMemContext::Destruct, MEM_CONTEXT_LODLANDSCAPE, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectTREE.cpp", 409);
	STBBNBT[0] = 0.0f;
	STBBNBT[1] = 1.0f;
	STBBNBT[2] = 0.0f;
	STBBNBT[3] = 0.0f;
	STBBNBT[4] = 1.0f;
	STBBNBT[5] = 0.0f;
	STBBNBT[6] = 0.0f;
	STBBNBT[7] = 1.0f;
	STBBNBT[8] = 0.0f;
	STBBNBT[9] = 0.0f;
	STBBNBT[10] = 1.0f;
	STBBNBT[11] = 0.0f;
	STBBNBT[12] = 0.0f;
	STBBNBT[13] = 0.0f;
	STBBNBT[14] = -1.0f;
	STBBNBT[15] = 0.0f;
	STBBNBT[16] = 0.0f;
	STBBNBT[17] = -1.0f;
	STBBNBT[18] = 0.0f;
	STBBNBT[19] = 0.0f;
	STBBNBT[20] = -1.0f;
	STBBNBT[21] = 0.0f;
	STBBNBT[22] = 0.0f;
	STBBNBT[23] = -1.0f;
	STBBNBT[24] = 1.0f;
	STBBNBT[25] = 0.0f;
	STBBNBT[26] = 0.0f;
	STBBNBT[27] = 1.0f;
	STBBNBT[28] = 0.0f;
	STBBNBT[29] = 0.0f;
	STBBNBT[30] = 1.0f;
	STBBNBT[31] = 0.0f;
	STBBNBT[32] = 0.0f;
	STBBNBT[33] = 1.0f;
	STBBNBT[34] = 0.0f;
	STBBNBT[35] = 0.0f;
	void* ptr = ourMalloc(4 * sizeof(NiPoint3));
	if (ptr) {
		rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), 4, &emptyPointConstructor);
		pverts = static_cast<NiPoint3*>(ptr);
	} else {
		pverts = 0;
	}
	float y = BillboardSize.y;
	float v66 = BillboardSize.x;
	float v67 = y;
	if (y <= 0.0) {
		v67 = 1400.0f;
	}
	v66 = v67 / 2.0;
	NiPoint3* v4 = ::new (&dst) NiPoint3(-v66, 0.0f, 0.0f);
	*pverts = *v4;
	NiPoint3* v5 = ::new (&v56) NiPoint3(v66, 0.0f, 0.0f);
	pverts[3] = *v5;
	float x = -v66;
	if (abZUp) {
		NiPoint3* v6 = ::new (&v55) NiPoint3(x, 0.0f, v67);
		pverts[1] = *v6;
		v7 = ::new (&v54) NiPoint3(v66, 0.0f, v67);
	} else {
		NiPoint3* v8 = ::new (&v53) NiPoint3(x, v67, 0.0f);
		pverts[1] = *v8;
		v7 = ::new (&v52) NiPoint3(v66, v67, 0.0f);
	}
	pverts[2] = *v7;
	NiPoint3* pnorm = static_cast<NiPoint3*>(ourMalloc(12 * sizeof(NiPoint3)));
	if (pnorm) {
		rt::eh_vector_constructor_iterator(pnorm, sizeof(NiPoint3), 12, &emptyPointConstructor);
		pnorm_2 = pnorm;
	} else {
		pnorm_2 = 0;
	}
	ourMemcpy(pnorm_2, static_cast<void*>(STBBNBT), sizeof(NiPoint3) * 12);
	void* v48 = ourMalloc(4 * sizeof(NiPoint2));
	if (v48) {
		rt::eh_vector_constructor_iterator(v48, sizeof(NiPoint2), 4, &emptyPointConstructor);
		ptext = static_cast<NiPoint2*>(v48);
	} else {
		ptext = 0;
	}
	NiPoint2* v9 = ::new (&v47) NiPoint2(1.0f, 1.0f);
	float v10 = v9->x;
	float v11 = v9->y;
	ptext->x = v10;
	ptext->y = v11;
	NiPoint2* v13 = ::new (&v46) NiPoint2(1.0f, 0.0f);
	float v14 = v13->x;
	float v15 = v13->y;
	ptext[1].x = v14;
	ptext[1].y = v15;
	NiPoint2* v17 = ::new (&v45) NiPoint2(0.0f, 0.0f);
	float v18 = v17->x;
	float v19 = v17->y;
	ptext[2].x = v18;
	ptext[2].y = v19;
	NiPoint2* v21 = ::new (&v44) NiPoint2(0.0f, 1.0f);
	float v22 = v21->x;
	float v23 = v21->y;
	ptext[3].x = v22;
	ptext[3].y = v23;
	void* v42 = ourMalloc(4 * sizeof(NiColorA));
	if (v42) {
		rt::eh_vector_constructor_iterator(v42, sizeof(NiColorA), 4, &fnvd::constructElement<NiColorA>);
		pcol = static_cast<NiColorA*>(v42);
	} else {
		pcol = 0;
	}
	NiColorA* v25 = ::new (&v41) NiColorA(1.0f, 1.0f, 1.0f, 0.0f);
	*pcol = *v25;
	NiColorA* v26 = ::new (&v40) NiColorA(1.0f, 1.0f, 1.0f, 0.0f);
	pcol[1] = *v26;
	NiColorA* v27 = ::new (&v39) NiColorA(1.0f, 1.0f, 1.0f, 0.0f);
	pcol[2] = *v27;
	NiColorA* v28 = ::new (&v38) NiColorA(1.0f, 1.0f, 1.0f, 0.0f);
	pcol[3] = *v28;
	uint16_t* ptrilist = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
	*ptrilist = 0;
	ptrilist[1] = 2;
	ptrilist[2] = 1;
	ptrilist[3] = 0;
	ptrilist[4] = 3;
	ptrilist[5] = 2;
	NiTriShapeData* v37 = static_cast<NiTriShapeData*>(NiMemObject::operator new(sizeof(NiTriShapeData)));
	if (v37) {
		v31 =
			::new (v37) NiTriShapeData(4, pverts, pnorm_2, pcol, ptext, 1, NiGeometryData::NBT_METHOD_NDL, 2, ptrilist);
	} else {
		v31 = 0;
	}
	::new (&spgeomData) NiPointer<NiTriShapeData>(static_cast<NiTriShapeData*>(v31));
	aspTriShapeData.operator=(spgeomData);
	spgeomData.~NiPointer<NiTriShapeData>();
}

// The exe's cleanup-only C++ EH frame is left out.
NiNode* TESObjectTREE::BuildDistant3D(bool abZUp) {
	BSShaderTextureSet* ptextureSet;
	SpeedTreeBillboardShaderProperty* pbillboardProp;
	NiTriShape* p3D;
	char a2[268];
	FNVD_INLINED_LOCAL(AutoMemContext, tempMemContext, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiPointer<NiTriShapeData>, spgeomData, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiPointer<NiTexture>, spTempTexture, std::nullopt, std::nullopt);
	if (!IsLargeEnoughForDistantLOD()) {
		return 0;
	}
	::new (&tempMemContext) AutoMemContext(MEM_CONTEXT_LODLANDSCAPE, 1,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectTREE.cpp", 470);
	::new (&spgeomData) NiPointer<NiTriShapeData>(nullptr);
	CreateBillboardGeomData(abZUp, spgeomData);
	NiTriShape* v12 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
	if (v12) {
		p3D = ::new (v12) NiTriShape(spgeomData.m_pObject);
	} else {
		p3D = 0;
	}
	NiAlphaProperty* paprop = BSTreeManager::GetDistantBillboardAlphaProperty();
	if (paprop) {
		p3D->AttachProperty(paprop);
	}
	SpeedTreeBillboardShaderProperty* v11 = static_cast<
		SpeedTreeBillboardShaderProperty*>(NiMemObject::operator new(sizeof(SpeedTreeBillboardShaderProperty)));
	if (v11) {
		pbillboardProp = ::new (v11) SpeedTreeBillboardShaderProperty;
	} else {
		pbillboardProp = 0;
	}
	p3D->AttachProperty(pbillboardProp);
	GetBillboardTextureFilename(a2);
	BSShaderTextureSet* v10 = static_cast<BSShaderTextureSet*>(NiMemObject::operator new(sizeof(BSShaderTextureSet)));
	if (v10) {
		ptextureSet = ::new (v10) BSShaderTextureSet;
	} else {
		ptextureSet = 0;
	}
	ptextureSet->SetTextureFilename(BSShaderProperty::TEXTYPE_DIFFUSE, a2);
	pbillboardProp->LoadTextureSet("STBB", ptextureSet);
	pbillboardProp->SetClampNI(NiTexturingProperty::CLAMP_S_CLAMP_T);
	::new (&spTempTexture) NiPointer<NiTexture>(nullptr);
	ptextureSet->GetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, spTempTexture);
	if (spTempTexture.operator==(nullptr)) {
		spTempTexture.~NiPointer<NiTexture>();
		spgeomData.~NiPointer<NiTriShapeData>();
		tempMemContext.~AutoMemContext();
		return 0;
	} else {
		NiBillboardNode* pnode;
		NiBillboardNode* v9 = static_cast<NiBillboardNode*>(NiMemObject::operator new(sizeof(NiBillboardNode)));
		if (v9) {
			pnode = ::new (v9) NiBillboardNode;
		} else {
			pnode = 0;
		}
		pnode->SetMode(NiBillboardNode::BSROTATE_ABOUT_UP);
		pnode->AttachChild(p3D, 1);
		spTempTexture.~NiPointer<NiTexture>();
		spgeomData.~NiPointer<NiTriShapeData>();
		tempMemContext.~AutoMemContext();
		return pnode;
	}
}

BSTextureSet::BSTextureSet() {}

// The exe's destructor is NiObject's alone.
BSTextureSet::~BSTextureSet() {}

bool SpeedTree::bEnableTrees::Get() {
	return gs_bEnableTrees_SpeedTree.Bool();
}

TESIconTree::TESIconTree() : TESTexture1024() {}

TESIconTree::~TESIconTree() {}

const char* TESIconTree::GetDefaultPath() const {
	return "Data\\Textures\\Trees\\Leaves\\";
}

TESModelTree::TESModelTree() {}

TESModelTree::~TESModelTree() {}

TESTexture1024::TESTexture1024() : TESTexture() {}

TESTexture1024::~TESTexture1024() {}

NiBillboardNode::~NiBillboardNode() {}
