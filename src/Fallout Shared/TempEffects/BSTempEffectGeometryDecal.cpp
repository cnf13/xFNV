#include "Fallout Shared/TempEffects/BSTempEffectGeometryDecal.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/GeometryDecalShaderProperty.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDecalNode.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/FaceGen/NiGeometryDataIteratorNiPoint3.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout Shared/TempEffects/NiGeometryDataIteratorNiBlendIndices.h"
#include "Fallout Shared/TempEffects/NiGeometryDataIteratorNiBlendWeights.h"
#include "Fallout Shared/TempEffects/NiGeometryDataIteratorNiPoint2.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/FixedStrings.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeomData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/System.h"
#include "SDK/MSVC/ehvec.h"

#include <cmath>

NiRTTI BSTempEffectGeometryDecal::ms_RTTI("BSTempEffectGeometryDecal", &BSTempEffect::ms_RTTI);

BSTempEffectGeometryDecal::BSTempEffectGeometryDecal(TESObjectCELL* apCell, float afLifetime,
	DECAL_CREATION_DATA* apDecalData, NiTriBasedGeom* apGeometry, NiPoint3 apOrigin, NiPoint3 apVector, float afScale,
	float afRotation, uint32_t aiBodyParts)
	: BSTempEffect(apCell, afLifetime), spDecalGeometry(nullptr), pOldIndex(nullptr), bFinished(false),
	  spTargetGeom(apGeometry), spParent(apGeometry->m_pkParent), spDecalParent(nullptr), pOrigin(apOrigin),
	  pVector(apVector), fScale(afScale), fRotation(afRotation), pTextureSet(apDecalData->pTexSet), iNumVerts(0),
	  iBodyParts(aiBodyParts) {}

BSTempEffectGeometryDecal::~BSTempEffectGeometryDecal() {
	if (spDecalGeometry.m_pObject) {
		if (spDecalGeometry.m_pObject->NiAVObject::GetParent()) {
			NiNode* pParent = spDecalGeometry.m_pObject->NiAVObject::GetParent();
			pParent->DetachChild(spDecalGeometry.m_pObject);
		}
		spDecalGeometry = static_cast<NiTriBasedGeom*>(nullptr);
	}
	if (pOldIndex) {
		ourFree(pOldIndex);
	}
	spTargetGeom = static_cast<NiTriBasedGeom*>(nullptr);
	spParent = static_cast<NiNode*>(nullptr);
}

void BSTempEffectGeometryDecal::Initialize() {
	NiTriBasedGeom* v1 = spTargetGeom.m_pObject;
	if (v1->GetSkinInstance()) {
		if (!bInitialized) {
			NiNode* v2 = spParent.m_pObject;
			if (v2->GetRefCount() > 1) {
				NiTriBasedGeom* v3 = spTargetGeom.m_pObject;
				if (v3->GetRefCount() > 1) {
					NiGeometry* v4 = spTargetGeom.m_pObject;
					NiAdditionalGeometryData* AdditionalGeometryData = v4->GetAdditionalGeometryData();
					if (AdditionalGeometryData && AdditionalGeometryData->GetVBReady()) {
						InitializeUsingAGD();
					} else {
						InitializeUsingData();
					}
				}
			}
		}
		BSTempEffect::Initialize();
		spTargetGeom.operator=(static_cast<NiTriBasedGeom*>(0));
		spParent.operator=(static_cast<NiNode*>(0));
	}
}

bool BSTempEffectGeometryDecal::Update(float afTimeDelta) {
	if (bFinished) {
		return 0;
	}
	bool ret = BSTempEffect::Update(afTimeDelta);
	NiAVObject* pobj = Get3D();
	// The shade property of the decal geometry is its GeometryDecalShaderProperty.
	static_cast<GeometryDecalShaderProperty*>(pobj->GetProperty(NiProperty::SHADE))->fTimer = fAge / fLifetime;
	if (fLifetime < static_cast<double>(fAge)) {
		BGSDecalNode* pdecalnode = 0;
		if (spDecalParent.m_pObject) {
			NiNode* v4 = spDecalParent.m_pObject;
			v4->DetachChild(spDecalGeometry.m_pObject);
			if (spDecalParent.m_pObject->m_pkParent) {
				pdecalnode = static_cast<BGSDecalNode*>(spDecalParent.m_pObject->m_pkParent);
				pdecalnode->RemoveFromDecalList(this);
			}
		}
		if (pdecalnode) {
			if (!spDecalParent.m_pObject->GetChildCount()) {
				NiNode* Parent = spDecalParent.m_pObject->m_pkParent;
				Parent->DetachChild(spDecalParent.m_pObject);
				--BGSDecalManager::iSkinnedDecalCount;
			}
		}
		bFinished = 1;
	}
	return ret;
}

NiAVObject* BSTempEffectGeometryDecal::Get3D() {
	return spDecalGeometry;
}

uint16_t BSTempEffectGeometryDecal::GetSaveSize() {
	uint16_t isize = 0;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		isize = 6;
	}
	if (bDebugSaveBuffer.Bool()) {
		SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pheader) {
			TESForm* FormByNumericID = TESForm::GetFormByNumericID(pheader->iFormID);
			InitializationFlow("GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				isize, pheader->iFormID, FormByNumericID->GetFormEditorID(), pheader->iFlags, 1839,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp");
		} else {
			InitializationFlow("GetSaveSize(): %-5i ending at line %i in file %s", isize, 1839,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp");
		}
	}
	return isize;
}

void BSTempEffectGeometryDecal::SaveGame() {
	uint32_t iblockHeader;
	uint16_t iblockSize;
	iblockSize = 0;
	void* pblockSize = 0;
	uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
	if (bDebugSaveBuffer.Bool()) {
		itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		// 'BLOK'.
		iblockHeader = 0x424C4F4B;
		pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
		pblockSize = pSaveLoadGameOLD->m_pBuffer;
		pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
	}
	if (bDebugSaveBuffer.Bool()) {
		uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pheader) {
			TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
			uint32_t iFlags = pheader->iFlags;
			const char* v1 = pform->GetFormEditorID();
			InitializationFlow("SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				BufferPointer - itempStartPosition, pheader->iFormID, v1, iFlags, 1923,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp");
		} else {
			InitializationFlow("SaveGame(): %-5i ending at line %i in file %s", BufferPointer - itempStartPosition,
				1923,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp");
		}
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		// The save buffer is raw bytes: the 2-byte size placeholder is patched in place.
		uint8_t* pcurrent = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		uint8_t* pblockStart = static_cast<uint8_t*>(pblockSize);
		if (pcurrent > pblockStart + 0xFFFF) {
			Warning("SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp",
				1923);
		}
		*static_cast<uint16_t*>(pblockSize) = static_cast<uint16_t>(pcurrent - pblockStart);
	}
}

bool BSTempEffectGeometryDecal::LoadGame() {
	uint32_t iblockHeader;
	uint16_t v17[3];
	bool bvalid = 1;
	v17[0] = 0;
	uint8_t* istartBufferPosition = 0;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
		if (iblockHeader != 0x424C4F4B) {
			LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
			if (pheader) {
				TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
						"loading form is %08X %s with version %i and flags %08X",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout "
					"Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp",
					1932, pheader->iFormID, pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags);
			} else {
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
						"version is %i",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout "
					"Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp",
					1932, pSaveLoadGameOLD->GetCurrentVersion());
			}
		}
		istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		pSaveLoadGameOLD->LoadGameDataOLD(v17, 2u);
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
		if (pheader_2) {
			TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
			if (icurrentBufferPosition <= &istartBufferPosition[v17[0]]) {
				if (icurrentBufferPosition < &istartBufferPosition[v17[0]]) {
					uint32_t iFlags = pheader_2->iFlags;
					int32_t cVersion = pheader_2->cVersion;
					const char* v3 = pform_2->GetFormEditorID();
					Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
							"form is %08X %s with version %i and flags %08X",
						&istartBufferPosition[v17[0]] - icurrentBufferPosition,
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout "
						"Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp",
						2104, pheader_2->iFormID, v3, cVersion, iFlags);
				}
			} else {
				uint32_t v6 = pheader_2->iFlags;
				int32_t v4 = pheader_2->cVersion;
				const char* v2 = pform_2->GetFormEditorID();
				Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
						"is %08X %s with version %i and flags %08X",
					icurrentBufferPosition - &istartBufferPosition[v17[0]],
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout "
					"Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp",
					2104, pheader_2->iFormID, v2, v4, v6);
			}
		} else {
			if (icurrentBufferPosition <= &istartBufferPosition[v17[0]]) {
				if (icurrentBufferPosition < &istartBufferPosition[v17[0]]) {
					int32_t CurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
					Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
							"%i",
						&istartBufferPosition[v17[0]] - icurrentBufferPosition,
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout "
						"Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp",
						2104, CurrentVersion);
				}
			} else {
				int32_t v8 = pSaveLoadGameOLD->GetCurrentVersion();
				Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
					icurrentBufferPosition - &istartBufferPosition[v17[0]],
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout "
					"Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp",
					2104, v8);
			}
		}
	}
	return bvalid;
}

bool BSTempEffectGeometryDecal::ShouldAddTriangle(uint32_t aiIndex, int32_t aiBaseVertex, bool abStrip, uint16_t* apIB,
	NiPoint3* apTexCoords, uint16_t* apusVertexMap) {
	uint16_t v7;
	uint16_t v9;
	int32_t v10;
	int32_t i;
	uint8_t v12 = 0;
	int32_t v13 = 0;
	for (i = 0; i < 3; ++i) {
		v10 = 0;
		if (abStrip) {
			v7 = apIB[i + aiIndex];
		} else {
			v7 = apIB[(3 * aiIndex) + i];
		}
		v9 = (uint16_t)(aiBaseVertex + v7);
		if (apusVertexMap) {
			v9 = apusVertexMap[v9];
		}
		if (!(apTexCoords[v9].x < 0.0)) {
			if (!(apTexCoords[v9].x > 1.0)) {
				v10 = 1;
			} else {
				v13 |= 2u;
			}
		} else {
			v13 |= 1u;
		}
		if (!(apTexCoords[v9].y < 0.0)) {
			if (!(apTexCoords[v9].y > 1.0)) {
				++v10;
			} else {
				v13 |= 8u;
			}
		} else {
			v13 |= 4u;
		}
		if (apTexCoords[v9].z > 0.10000000149011612) {
			v13 |= 0x10u;
			++v10;
		}
		if (v10 == 3) {
			return (bool)1;
		}
	}
	if ((((v13 & 3) == 3) || ((v13 & 12) == 12)) && ((v13 & 16) != 0)) {
		return (bool)1;
	}
	return (bool)v12;
}

// Projects the decal onto a dismemberable skinned target whose vertices come through the model data's packed streams:
// the vertices are skinned (Deform is empty in this build), moved into the decal's texture space, and every triangle of
// an enabled body part's partition that the decal touches is copied into the decal geometry, with a new skin partition
// per body part and new bone weights. When no partition is enabled it deliberately returns without unlocking the packed
// vertex data, and when no vertex is kept it leaks the new partitions, as the game does. The new[] allocations of
// 16-bit counts keep a size overflow check only where a count can overflow it.
void BSTempEffectGeometryDecal::InitializeUsingAGD() {
	NiSkinInstance* pSkinInstance = spTargetGeom.operator->()->GetSkinInstance();
	if (!NiObject::IsExactKindOf(&BSDismemberSkinInstance::ms_RTTI, pSkinInstance)) {
		return;
	}
	BSDismemberSkinInstance* pDismemberSkinInstance = static_cast<BSDismemberSkinInstance*>(pSkinInstance);
	int32_t iPartitionCount = static_cast<int32_t>(pSkinInstance->GetSkinPartition()->GetPartitionCount());

	// The projection: the decal's rotation about Z, then the rotation of Z onto the decal's direction.
	NiMatrix3 kProjection;
	kProjection.~NiMatrix3();
	::new (&kProjection) NiMatrix3;
	NiTriBasedGeom* pTargetGeom = spTargetGeom.operator->();
	const NiTransform* pkWorldTransform;
	pkWorldTransform = &pTargetGeom->GetWorldTransform();
	NiMatrix3 kWorldRotate = pkWorldTransform->m_Rotate;
	NiMatrix3 kInverseRotate;
	kWorldRotate.Inverse(&kInverseRotate);
	NiPoint3 kLocalVector;
	kLocalVector.~NiPoint3();
	::new (&kLocalVector) NiPoint3;
	kLocalVector = kInverseRotate * pVector;
	NiMatrix3 kZRotation;
	kZRotation.~NiMatrix3();
	::new (&kZRotation) NiMatrix3;
	kZRotation.MakeZRotation(fRotation);
	kProjection = kZRotation * BSUtilities::GetRotationZToVector(pVector);
	pTargetGeom = spTargetGeom.operator->();
	pkWorldTransform = &pTargetGeom->GetWorldTransform();
	NiTransform kWorld = *pkWorldTransform;
	kWorld.m_Translate -= pOrigin;

	NiGeometryDataIteratorNiPoint3 kVertexIter;
	NiGeometryDataIteratorNiPoint3 kNormalIter;
	NiGeometryDataIteratorNiPoint2 kTextureIter;
	NiGeometryDataIteratorNiBlendWeights kWeightIter;
	NiGeometryDataIteratorNiBlendIndices kIndexIter;
	FNVD_RECONSTRUCT(NiTConvertingStrideIterator<NiPoint3>)(&kVertexIter);
	FNVD_RECONSTRUCT(NiTConvertingStrideIterator<NiPoint3>)(&kNormalIter);
	FNVD_RECONSTRUCT(NiTConvertingStrideIterator<NiPoint2>)(&kTextureIter);
	FNVD_RECONSTRUCT(NiTConvertingStrideIterator<NiBlendWeights>)(&kWeightIter);
	FNVD_RECONSTRUCT(NiTConvertingStrideIterator<NiBlendIndices>)(&kIndexIter);
	NiAdditionalGeometryData* pAdditionalData = spTargetGeom.operator->()->GetModelData()->GetAdditionalGeometryData();
	int32_t iBones = 0;
	NiSkinData::BoneData* pSkinBoneData = nullptr;
	NiSkinData::BoneData* pBoneData = nullptr;
	NiSkinData* pSkinData = nullptr;
	NiSkinPartition::Partition* pPartitions = nullptr;
	pSkinBoneData = pSkinInstance->GetSkinData()->GetBoneData();
	if (!pSkinBoneData) {
		return;
	}
	int32_t iVertexCount = pAdditionalData->GetVertexCount();
	uint32_t uiVertexCount = static_cast<uint32_t>(iVertexCount);
	bool bLocked = spTargetGeom.operator->()->GetModelData()->LockPackedVertexData(false);
	spTargetGeom.operator->()->GetModelData()->GetVerticesIterator(kVertexIter);
	spTargetGeom.operator->()->GetModelData()->GetNormalsIterator(kNormalIter);
	spTargetGeom.operator->()->GetModelData()->GetTextureSetIterator(0, kTextureIter);
	spTargetGeom.operator->()->GetModelData()->GetBlendWeightsIterator(kWeightIter);
	spTargetGeom.operator->()->GetModelData()->GetBlendIndicesIterator(kIndexIter);

	// Counts the triangles and partitions of the body parts the decal covers. A body part above 1000 is divided by 1000
	// and one above 100 reduced below 101; iBodyParts holds bit (part + 1) for each covered part, with the shift count
	// taken modulo 32.
	uint16_t usTriangleCount = 0;
	bool bAnyPartition = false;
	int32_t iPartitions = 0;
	pPartitions = pSkinInstance->GetSkinPartition()->GetPartitions();
	int32_t bStrips = pPartitions->m_usStrips > 0;
	for (int32_t i = 0; i < iPartitionCount; ++i) {
		bool bEnabled = pDismemberSkinInstance->GetPartitionDataArray()[i].bEnabled;
		uint16_t usBodyPart = pDismemberSkinInstance->GetPartitionDataArray()[i].usBodyPartNum;
		if (usBodyPart > 1000) {
			usBodyPart = static_cast<uint16_t>(usBodyPart / 1000);
		} else if (usBodyPart > 100) {
			while (usBodyPart > 100) {
				usBodyPart = static_cast<uint16_t>(usBodyPart - 100);
			}
		}
		int32_t iBodyPart = usBodyPart;
		if (bEnabled && (iBodyParts & (1u << ((iBodyPart + 1) & 31))) != 0) {
			bAnyPartition = true;
			usTriangleCount = static_cast<uint16_t>(usTriangleCount + pPartitions[i].m_usTriangles);
			++iPartitions;
		}
	}
	if (!bAnyPartition) {
		return;
	}

	// EXE-INLINED none: the new[] of each array (ourMalloc of the count times the element size)
	int32_t iIndexCount = usTriangleCount * 3;
	uint16_t* pNewIndex = static_cast<uint16_t*>(ourMalloc(static_cast<uint32_t>(iIndexCount) * sizeof(uint16_t)));
	int32_t* piVertexMap = static_cast<int32_t*>(ourMalloc(uiVertexCount * sizeof(int32_t)));
	uint16_t usNewVertexCount = 0;
	uint16_t* pPartIndex = static_cast<uint16_t*>(ourMalloc(static_cast<uint32_t>(iIndexCount) * sizeof(uint16_t)));
	uint16_t* pPartVertexMap = static_cast<uint16_t*>(ourMalloc(uiVertexCount * sizeof(uint16_t)));
	int32_t* piPartVertexMap = static_cast<int32_t*>(ourMalloc(uiVertexCount * sizeof(int32_t)));
	uint32_t uiMaxPartitions = static_cast<uint32_t>(iPartitions);
	uint32_t uiCount = static_cast<uint32_t>(iPartitions);
	uint16_t* pBodyPartIndex =
		static_cast<uint16_t*>(ourMalloc(uiCount > SIZE_MAX / sizeof(uint16_t) ? SIZE_MAX
																			   : sizeof(uint16_t) * uiCount));
	float* pfPartWeights = static_cast<float*>(ourMalloc((uiVertexCount << 2) * sizeof(float)));
	// A byte array's new[] has no multiplication to check.
	uint8_t* pucPartBones = static_cast<uint8_t*>(ourMalloc(uiVertexCount << 2));
	ourMemset(piVertexMap, -1, uiVertexCount << 2);

	// A real new[]: clang gives the class a vector deleting destructor only where its new[] is seen, and
	// ~NiSkinPartition's delete[] of the partitions goes through it.
	NiSkinPartition::Partition* pNewPartitions = new NiSkinPartition::Partition[static_cast<uint32_t>(iPartitions)];
	void* pvMemory;
	NiPoint3* pTexCoords = nullptr;
	pvMemory = ourMalloc(uiVertexCount * sizeof(NiPoint3));
	if (pvMemory) {
		rt::eh_vector_constructor_iterator(pvMemory, sizeof(NiPoint3), iVertexCount, &emptyPointConstructor);
		pTexCoords = static_cast<NiPoint3*>(pvMemory);
	}
	NiBlendWeights* pWeights = nullptr;
	pvMemory = ourMalloc(uiVertexCount * sizeof(NiBlendWeights));
	if (pvMemory) {
		rt::eh_vector_constructor_iterator(pvMemory, sizeof(NiBlendWeights), iVertexCount,
			&fnvd::constructElement<NiBlendWeights>);
		pWeights = static_cast<NiBlendWeights*>(pvMemory);
	}
	NiPoint3* pSkinVertices = nullptr;
	NiPoint3* pSkinNormals = nullptr;
	pvMemory = ourMalloc(uiVertexCount * sizeof(NiPoint3));
	if (pvMemory) {
		rt::eh_vector_constructor_iterator(pvMemory, sizeof(NiPoint3), iVertexCount, &emptyPointConstructor);
		pSkinVertices = static_cast<NiPoint3*>(pvMemory);
	}
	pvMemory = ourMalloc(uiVertexCount * sizeof(NiPoint3));
	if (pvMemory) {
		rt::eh_vector_constructor_iterator(pvMemory, sizeof(NiPoint3), iVertexCount, &emptyPointConstructor);
		pSkinNormals = static_cast<NiPoint3*>(pvMemory);
	}
	pSkinInstance->Deform(&kVertexIter, &kWeightIter, &kIndexIter, &kNormalIter, uiVertexCount, pSkinVertices,
		pSkinNormals, sizeof(NiPoint3));

	// A copy of the skin's bones, each with room for the vertex weights the kept vertices bring (the first bone for
	// four per vertex).
	pSkinData = pSkinInstance->GetSkinData();
	pSkinBoneData = pSkinData->GetBoneData();
	iBones = static_cast<int32_t>(pSkinData->GetBoneCount());
	uiCount = static_cast<uint32_t>(iBones);
	pvMemory = ourMalloc(uiCount > SIZE_MAX / sizeof(NiSkinData::BoneData) ? SIZE_MAX
																		   : sizeof(NiSkinData::BoneData) * uiCount);
	if (pvMemory) {
		rt::eh_vector_constructor_iterator(pvMemory, sizeof(NiSkinData::BoneData), iBones,
			&fnvd::constructElement<NiSkinData::BoneData>);
		pBoneData = static_cast<NiSkinData::BoneData*>(pvMemory);
	}
	for (int32_t j = 0; j < iBones; ++j) {
		NiSkinData::BoneData* pBone = &pBoneData[j];
		pBone->m_kSkinToBone = pSkinBoneData[j].m_kSkinToBone;
		NiSkinData::BoneVertData* pBoneVertData = nullptr;
		if (j == 0) {
			uiCount = uiVertexCount << 2;
			pvMemory = ourMalloc(uiCount > SIZE_MAX / sizeof(NiSkinData::BoneVertData)
					? SIZE_MAX
					: sizeof(NiSkinData::BoneVertData) * uiCount);
			if (pvMemory) {
				rt::eh_vector_constructor_iterator(pvMemory, sizeof(NiSkinData::BoneVertData),
					static_cast<int32_t>(uiCount), &fnvd::constructElement<NiSkinData::BoneVertData>);
				pBoneVertData = static_cast<NiSkinData::BoneVertData*>(pvMemory);
			}
		} else {
			pvMemory = ourMalloc(uiVertexCount * sizeof(NiSkinData::BoneVertData));
			if (pvMemory) {
				rt::eh_vector_constructor_iterator(pvMemory, sizeof(NiSkinData::BoneVertData), iVertexCount,
					&fnvd::constructElement<NiSkinData::BoneVertData>);
				pBoneVertData = static_cast<NiSkinData::BoneVertData*>(pvMemory);
			}
		}
		pBone->m_pkBoneVertData = pBoneVertData;
	}

	// Each vertex's texture coordinates: x and y in the decal's plane scaled to [0, 1], z the weight of the decal
	// there, falling off with the angle between the normal and the decal's direction and with the depth.
	NiPoint3 kDirection = kLocalVector;
	NiPoint3 kSkinVertex;
	kSkinVertex.~NiPoint3();
	::new (&kSkinVertex) NiPoint3;
	NiPoint3 kSkinNormal;
	kSkinNormal.~NiPoint3();
	::new (&kSkinNormal) NiPoint3;
	for (int32_t k = 0; k < iVertexCount; ++k) {
		NiPoint3 kProjected = NiPoint3_ZERO;
		kSkinVertex = pSkinVertices[k];
		kSkinNormal = pSkinNormals[k];
		kWeightIter.Get(k, pWeights[k]);
		// Sums the three weights in double precision and stores the sum as a float.
		float fSum = static_cast<float>(static_cast<double>(pWeights[k].fWeights[0]) + pWeights[k].fWeights[1] +
			pWeights[k].fWeights[2]);
		pWeights[k].fWeights[3] = static_cast<float>(1.0 - fSum);
		if (pWeights[k].fWeights[3] < 0.0001) {
			pWeights[k].fWeights[3] = 0.0f;
		}
		kSkinVertex *= kWorld.m_fScale;
		NiPoint3 kWorldVertex = NiPoint3_ZERO;
		NiMatrix3::TransformVertices(kWorld.m_Rotate, kWorld.m_Translate, 1, &kSkinVertex, &kWorldVertex);
		NiMatrix3::TransformVertices(kProjection, NiPoint3_ZERO, 1, &kWorldVertex, &kProjected);
		pTexCoords[k].x = static_cast<float>(static_cast<double>(kProjected.x) / fScale + 0.5);
		pTexCoords[k].y = static_cast<float>(static_cast<double>(kProjected.y) / fScale + 0.5);
		float fFacing = static_cast<float>(1.0 - BSMAX(0.0f, kDirection.Dot(kSkinNormal)));
		float fFalloff = std::pow(fFacing, 10);
		pTexCoords[k].z = static_cast<float>((1.0 - std::fabs(kProjected.z) / 256.0) * fFalloff);
	}

	// Walks every partition's triangles (strips or lists) from its first vertex in the model's vertex order, and
	// keeps those of the covered body parts the decal touches: the decal's own vertex and index lists, and per body
	// part a partition over the kept vertices with the source partition's bones.
	uint16_t usNewIndexCount = 0;
	int32_t iBaseVertex = 0;
	iPartitions = 0;
	for (int32_t m = 0; m < iPartitionCount; ++m) {
		uint16_t* pTriList = nullptr;
		bool bEnabled = pDismemberSkinInstance->GetPartitionDataArray()[m].bEnabled;
		uint16_t usBodyPart = pDismemberSkinInstance->GetPartitionDataArray()[m].usBodyPartNum;
		if (usBodyPart > 1000) {
			usBodyPart = static_cast<uint16_t>(usBodyPart / 1000);
		} else if (usBodyPart > 100) {
			while (usBodyPart > 100) {
				usBodyPart = static_cast<uint16_t>(usBodyPart - 100);
			}
		}
		int32_t iBodyPart = usBodyPart;
		if (bEnabled && (iBodyParts & (1u << ((iBodyPart + 1) & 31))) != 0) {
			usTriangleCount = pPartitions[m].m_usTriangles;
			if (!pTriList) {
				pTriList = pPartitions[m].m_pusTriList;
			}
			uint16_t usPartIndexCount = 0;
			uint16_t usPartVertexCount = 0;
			ourMemset(piPartVertexMap, -1, uiVertexCount << 2);
			ourMemset(pfPartWeights, 0, (uiVertexCount << 2) << 2);
			ourMemset(pucPartBones, 0, uiVertexCount << 2);
			int32_t bFlip = 0;
			for (int32_t t = 0; t < usTriangleCount; ++t) {
				bool bAdd = false;
				// Always set; it is tested with bAdd.
				bool bKeep = true;
				uint16_t usVertex0 = 0;
				uint16_t usVertex1 = 0;
				uint16_t usVertex2 = 0;
				if (bStrips) {
					usVertex0 = static_cast<uint16_t>(pTriList[t] + iBaseVertex);
					usVertex1 = static_cast<uint16_t>(pTriList[t + 1] + iBaseVertex);
					usVertex2 = static_cast<uint16_t>(pTriList[t + 2] + iBaseVertex);
				} else {
					usVertex0 = static_cast<uint16_t>(pTriList[3 * t] + iBaseVertex);
					usVertex1 = static_cast<uint16_t>(pTriList[3 * t + 1] + iBaseVertex);
					usVertex2 = static_cast<uint16_t>(pTriList[3 * t + 2] + iBaseVertex);
				}
				if (usVertex0 == usVertex1 || usVertex0 == usVertex2 || usVertex1 == usVertex2) {
					// A degenerate strip triangle still flips the winding.
					if (bStrips) {
						bFlip = bFlip == 0;
					}
					continue;
				}
				bAdd = ShouldAddTriangle(t, iBaseVertex, bStrips != 0, pTriList, pTexCoords, nullptr);
				if (bAdd && bKeep) {
					for (int32_t v = 0; v < 3; ++v) {
						uint16_t usIndex = bStrips ? pTriList[t + v] : pTriList[3 * t + v];
						uint16_t usVertex = static_cast<uint16_t>(usIndex + iBaseVertex);
						if (piVertexMap[usVertex] == -1) {
							// A new decal vertex: its weights go to the bones the source partition maps its
							// indices to.
							piVertexMap[usVertex] = usNewVertexCount;
							pNewIndex[usNewIndexCount] = usNewVertexCount;
							usNewIndexCount = static_cast<uint16_t>(usNewIndexCount + 1);
							NiBlendIndices kIndices;
							kIndexIter.Get(usVertex, kIndices);
							for (int32_t w = 0; w < 4; ++w) {
								if (pWeights[usVertex].fWeights[w] != 0.0) {
									uint16_t usBone = kIndices.Indices[3 - w];
									usBone = pPartitions[m].m_pusBones[usBone];
									NiSkinData::BoneData* pBone = &pBoneData[usBone];
									pBone->m_pkBoneVertData[pBone->m_usVerts].m_usVert = usNewVertexCount;
									pBone->m_pkBoneVertData[pBone->m_usVerts].m_fWeight =
										pWeights[usVertex].fWeights[w];
									pBone->m_usVerts = static_cast<uint16_t>(pBone->m_usVerts + 1);
								}
							}
							usNewVertexCount = static_cast<uint16_t>(usNewVertexCount + 1);
						} else {
							pNewIndex[usNewIndexCount] = static_cast<uint16_t>(piVertexMap[usVertex]);
							usNewIndexCount = static_cast<uint16_t>(usNewIndexCount + 1);
						}
						if (piPartVertexMap[usVertex] == -1) {
							// A new vertex of this partition: its weights and palette bone indices, four slots.
							NiBlendIndices kPartIndices;
							kIndexIter.Get(usVertex, kPartIndices);
							for (int32_t w = 0; w < 4; ++w) {
								if (pWeights[usVertex].fWeights[w] != 0.0) {
									uint16_t usBone = kPartIndices.Indices[3 - w];
									int32_t iSlot = w + usPartVertexCount * 4;
									pfPartWeights[iSlot] = pWeights[usVertex].fWeights[w];
									pucPartBones[iSlot] = static_cast<uint8_t>(usBone);
								}
							}
							piPartVertexMap[usVertex] = usPartVertexCount;
							pPartIndex[usPartIndexCount] = usPartVertexCount;
							usPartIndexCount = static_cast<uint16_t>(usPartIndexCount + 1);
							pPartVertexMap[usPartVertexCount] = static_cast<uint16_t>(piVertexMap[usVertex]);
							usPartVertexCount = static_cast<uint16_t>(usPartVertexCount + 1);
						} else {
							pPartIndex[usPartIndexCount] = static_cast<uint16_t>(piPartVertexMap[usVertex]);
							usPartIndexCount = static_cast<uint16_t>(usPartIndexCount + 1);
						}
					}
					// A strip triangle of odd winding is turned around in both index lists.
					if (bStrips && bFlip) {
						int32_t iFirst = pNewIndex[usNewIndexCount - 3];
						int32_t iLast = pNewIndex[usNewIndexCount - 1];
						pNewIndex[usNewIndexCount - 3] = static_cast<uint16_t>(iLast);
						pNewIndex[usNewIndexCount - 1] = static_cast<uint16_t>(iFirst);
						iFirst = pPartIndex[usPartIndexCount - 3];
						iLast = pPartIndex[usPartIndexCount - 1];
						pPartIndex[usPartIndexCount - 3] = static_cast<uint16_t>(iLast);
						pPartIndex[usPartIndexCount - 1] = static_cast<uint16_t>(iFirst);
					}
				}
				if (bStrips) {
					bFlip = bFlip == 0;
				}
			}
			if (usPartVertexCount > 0) {
				NiSkinPartition::Partition& rSource = pPartitions[m];
				NiSkinPartition::Partition& rPartition = pNewPartitions[iPartitions];
				rPartition.m_usTriangles = static_cast<uint16_t>(usPartIndexCount / 3);
				rPartition.m_pusTriList = static_cast<uint16_t*>(_NiMalloc(usPartIndexCount * sizeof(uint16_t)));
				ourMemcpy(rPartition.m_pusTriList, pPartIndex, usPartIndexCount * sizeof(uint16_t));
				rPartition.m_usVertices = usPartVertexCount;
				rPartition.m_pusVertexMap = static_cast<uint16_t*>(_NiMalloc(usPartVertexCount * sizeof(uint16_t)));
				ourMemcpy(rPartition.m_pusVertexMap, pPartVertexMap, usPartVertexCount * sizeof(uint16_t));
				rPartition.m_usBones = rSource.m_usBones;
				rPartition.m_pusBones = static_cast<uint16_t*>(_NiMalloc(rSource.m_usBones * sizeof(uint16_t)));
				ourMemcpy(rPartition.m_pusBones, rSource.m_pusBones, rSource.m_usBones * sizeof(uint16_t));
				rPartition.m_usBonesPerVertex = rSource.m_usBonesPerVertex;
				int32_t iWeightCount = rPartition.m_usBonesPerVertex * usPartVertexCount;
				rPartition.m_pfWeights =
					static_cast<float*>(_NiMalloc((static_cast<uint32_t>(iWeightCount)) * sizeof(float)));
				rPartition.m_pucBonePalette = static_cast<uint8_t*>(_NiMalloc(static_cast<uint32_t>(iWeightCount)));
				ourMemcpy(rPartition.m_pfWeights, pfPartWeights,
					(rPartition.m_usBonesPerVertex << 2) * rPartition.m_usVertices);
				ourMemcpy(rPartition.m_pucBonePalette, pucPartBones,
					rPartition.m_usBonesPerVertex * rPartition.m_usVertices);
				if (static_cast<uint32_t>(iPartitions) >= uiMaxPartitions) {
					platform::DebuggerBreak();
				}
				pBodyPartIndex[iPartitions] = usBodyPart;
				++iPartitions;
			}
		}
		iBaseVertex += pPartitions[m].m_usVertices;
	}
	ourFree(pWeights);

	NiSkinPartition* pNewSkinPartition = nullptr;
	if (iPartitions > 0) {
		pNewSkinPartition = new NiSkinPartition;
		pNewSkinPartition->ReplacePartitions(pNewPartitions, static_cast<uint32_t>(iPartitions));
	}
	if (usNewVertexCount > 0) {
		uint16_t* pGeomIndex = static_cast<uint16_t*>(ourMalloc(usNewIndexCount * sizeof(uint16_t)));
		ourMemcpy(pGeomIndex, pNewIndex, usNewIndexCount * sizeof(uint16_t));
		NiPoint3* pGeomVertex = nullptr;
		pvMemory = ourMalloc(usNewVertexCount * sizeof(NiPoint3));
		if (pvMemory) {
			rt::eh_vector_constructor_iterator(pvMemory, sizeof(NiPoint3), usNewVertexCount, &emptyPointConstructor);
			pGeomVertex = static_cast<NiPoint3*>(pvMemory);
		}
		NiPoint3* pGeomTexCoord = nullptr;
		pvMemory = ourMalloc(usNewVertexCount * sizeof(NiPoint3));
		if (pvMemory) {
			rt::eh_vector_constructor_iterator(pvMemory, sizeof(NiPoint3), usNewVertexCount, &emptyPointConstructor);
			pGeomTexCoord = static_cast<NiPoint3*>(pvMemory);
		}
		for (int32_t q = 0; q < iVertexCount; ++q) {
			NiPoint3 kVertex;
			kVertex.~NiPoint3();
			::new (&kVertex) NiPoint3;
			NiPoint3 kNormal;
			kNormal.~NiPoint3();
			::new (&kNormal) NiPoint3;
			if (piVertexMap[q] != -1) {
				kVertexIter.Get(q, kVertex);
				kNormalIter.Get(q, kNormal);
				pGeomVertex[piVertexMap[q]] = kVertex;
				pGeomTexCoord[piVertexMap[q]] = pTexCoords[q];
			}
		}
		// Each bone's vertex weights move to an array of the size used.
		for (int32_t b = 0; b < iBones; ++b) {
			NiSkinData::BoneData* pBone = &pBoneData[b];
			NiSkinData::BoneVertData* pOldVertData = pBone->m_pkBoneVertData;
			uint32_t uiVerts = pBone->m_usVerts;
			NiSkinData::BoneVertData* pBoneVertData = nullptr;
			pvMemory = ourMalloc(uiVerts * sizeof(NiSkinData::BoneVertData));
			if (pvMemory) {
				rt::eh_vector_constructor_iterator(pvMemory, sizeof(NiSkinData::BoneVertData),
					static_cast<int32_t>(uiVerts), &fnvd::constructElement<NiSkinData::BoneVertData>);
				pBoneVertData = static_cast<NiSkinData::BoneVertData*>(pvMemory);
			}
			pBoneData[b].m_pkBoneVertData = pBoneVertData;
			ourMemcpy(pBoneData[b].m_pkBoneVertData, pOldVertData, pBone->m_usVerts * sizeof(NiSkinData::BoneVertData));
			ourFree(pOldVertData);
		}
		if (pNewSkinPartition) {
			CreateGeometry(spTargetGeom.operator->(), usNewVertexCount, usNewIndexCount, pGeomVertex, pGeomTexCoord,
				pGeomIndex, pBoneData, pNewSkinPartition, pBodyPartIndex);
		} else {
			CreateGeometry(spTargetGeom.operator->(), usNewVertexCount, usNewIndexCount, pGeomVertex, pGeomTexCoord,
				pGeomIndex, pBoneData, nullptr, nullptr);
		}
		iNumVerts = usNewVertexCount;
	} else {
		for (int32_t b = 0; b < iBones; ++b) {
			ourFree(pBoneData[b].m_pkBoneVertData);
			pBoneData[b].m_pkBoneVertData = nullptr;
		}
		ourFree(pBoneData);
	}
	ourFree(pSkinVertices);
	ourFree(pSkinNormals);
	ourFree(pNewIndex);
	ourFree(piVertexMap);
	ourFree(pTexCoords);
	ourFree(pPartIndex);
	ourFree(pPartVertexMap);
	ourFree(piPartVertexMap);
	ourFree(pfPartWeights);
	ourFree(pucPartBones);
	ourFree(pBodyPartIndex);
	if (bLocked) {
		spTargetGeom.operator->()->GetModelData()->UnlockPackedVertexData();
	}
}

// Projects the decal onto a dismemberable skinned target from its model data's plain vertex arrays: the vertices are
// skinned (Deform), moved into the decal's texture space, and every triangle of an enabled body part's partition that
// the decal touches is copied into the decal geometry, with a new skin partition per body part and new bone weights.
// When no vertex is kept the new bones are freed but the new partitions are leaked, deliberately, as the game does.
void BSTempEffectGeometryDecal::InitializeUsingData() {
	if (!iBodyParts) {
		return;
	}
	NiSkinInstance* pSkinInstance = spTargetGeom.operator->()->GetSkinInstance();
	BSDismemberSkinInstance* pDismemberSkinInstance = nullptr;
	if (!NiObject::IsExactKindOf(&BSDismemberSkinInstance::ms_RTTI, pSkinInstance)) {
		return;
	}
	pDismemberSkinInstance = static_cast<BSDismemberSkinInstance*>(pSkinInstance);
	int32_t iPartitionCount = static_cast<int32_t>(pSkinInstance->GetSkinPartition()->GetPartitionCount());

	// The projection: the decal's rotation about Z, then the rotation of Z onto the decal's direction.
	NiMatrix3 kProjection;
	kProjection.~NiMatrix3();
	::new (&kProjection) NiMatrix3;
	NiTriBasedGeom* pTargetGeom = spTargetGeom.operator->();
	const NiTransform* pkWorldTransform;
	pkWorldTransform = &pTargetGeom->GetWorldTransform();
	NiMatrix3 kWorldRotate = pkWorldTransform->m_Rotate;
	NiMatrix3 kInverseRotate;
	kWorldRotate.Inverse(&kInverseRotate);
	NiPoint3 kLocalVector;
	kLocalVector.~NiPoint3();
	::new (&kLocalVector) NiPoint3;
	kLocalVector = kInverseRotate * pVector;
	NiMatrix3 kZRotation;
	kZRotation.~NiMatrix3();
	::new (&kZRotation) NiMatrix3;
	kZRotation.MakeZRotation(fRotation);
	kProjection = kZRotation * BSUtilities::GetRotationZToVector(pVector);
	pTargetGeom = spTargetGeom.operator->();
	pkWorldTransform = &pTargetGeom->GetWorldTransform();
	NiTransform kWorld = *pkWorldTransform;
	kWorld.m_Translate -= pOrigin;
	NiSkinData::BoneData* pSkinBoneData = pSkinInstance->GetSkinData()->GetBoneData();
	if (!pSkinBoneData || !pSkinBoneData->m_pkBoneVertData) {
		return;
	}

	int32_t iBones = 0;
	pSkinBoneData = nullptr;
	NiSkinData::BoneData* pBoneData = nullptr;
	NiSkinData* pSkinData = nullptr;
	NiSkinPartition::Partition* pPartitions = nullptr;
	int32_t iVertexCount = spTargetGeom.operator->()->GetModelData()->GetVertexCount();
	uint32_t uiVertexCount = static_cast<uint32_t>(iVertexCount);
	uint16_t usTriangles = 0;
	uint16_t* pusTriList = nullptr;
	uint16_t* pusVertexMap = nullptr;
	bool bAnyPartition = false;
	int32_t iPartitions = 0;

	// Counts the triangles and partitions of the body parts the decal covers. A body part above 1000 is divided by 1000
	// and one above 100 reduced below 101; iBodyParts holds bit (part + 1) for each covered part, with the shift count
	// taken modulo 32.
	pPartitions = pSkinInstance->GetSkinPartition()->GetPartitions();
	int32_t bStrips = pPartitions->m_usStrips > 0;
	for (int32_t i = 0; i < iPartitionCount; ++i) {
		bool bEnabled = pDismemberSkinInstance->GetPartitionDataArray()[i].bEnabled;
		uint16_t usBodyPart = pDismemberSkinInstance->GetPartitionDataArray()[i].usBodyPartNum;
		if (usBodyPart > 1000) {
			usBodyPart = static_cast<uint16_t>(usBodyPart / 1000);
		} else if (usBodyPart > 100) {
			while (usBodyPart > 100) {
				usBodyPart = static_cast<uint16_t>(usBodyPart - 100);
			}
		}
		int32_t iBodyPart = usBodyPart;
		if (bEnabled && (iBodyParts & (1u << ((iBodyPart + 1) & 31))) != 0) {
			bAnyPartition = true;
			usTriangles = static_cast<uint16_t>(usTriangles + pPartitions[i].m_usTriangles);
			++iPartitions;
		}
	}
	if (!bAnyPartition) {
		return;
	}

	// The scratch arrays, each a new[] whose byte count becomes all ones when it overflows 32 bits.
	// EXE-INLINED none: the new[] size checks
	uint32_t uiIndexCount = static_cast<uint32_t>(usTriangles * 3);
	uint16_t* pusNewIndex =
		static_cast<uint16_t*>(ourMalloc(uiIndexCount <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * uiIndexCount
																					 : SIZE_MAX));
	int32_t* piVertexMap =
		static_cast<int32_t*>(ourMalloc(uiVertexCount <= SIZE_MAX / sizeof(int32_t) ? sizeof(int32_t) * uiVertexCount
																					: SIZE_MAX));
	uint16_t* pusPartIndex =
		static_cast<uint16_t*>(ourMalloc(uiIndexCount <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * uiIndexCount
																					 : SIZE_MAX));
	uint16_t* pusPartVertexMap =
		static_cast<uint16_t*>(ourMalloc(uiVertexCount <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * uiVertexCount
																					  : SIZE_MAX));
	int32_t* piPartVertexMap =
		static_cast<int32_t*>(ourMalloc(uiVertexCount <= SIZE_MAX / sizeof(int32_t) ? sizeof(int32_t) * uiVertexCount
																					: SIZE_MAX));
	uint32_t uiBodyPartCount = static_cast<uint32_t>(iPartitionCount);
	uint16_t* pusBodyParts = static_cast<uint16_t*>(ourMalloc(uiBodyPartCount <= SIZE_MAX / sizeof(uint16_t)
			? sizeof(uint16_t) * uiBodyPartCount
			: SIZE_MAX));
	uint32_t uiInteractions = uiVertexCount << 2;
	float* pfPartWeights =
		static_cast<float*>(ourMalloc(uiInteractions <= SIZE_MAX / sizeof(float) ? sizeof(float) * uiInteractions
																				 : SIZE_MAX));
	uint8_t* pucPartBonePalette = static_cast<uint8_t*>(ourMalloc(uiVertexCount << 2));
	ourMemset(piVertexMap, -1, uiVertexCount << 2);

	// A real new[], as in the first decal's partitions above.
	NiSkinPartition::Partition* pNewPartitions = new NiSkinPartition::Partition[static_cast<uint32_t>(iPartitions)];

	void* pMemory =
		ourMalloc(uiVertexCount > SIZE_MAX / sizeof(NiPoint3) ? SIZE_MAX : sizeof(NiPoint3) * uiVertexCount);
	NiPoint3* pTexCoords = nullptr;
	if (pMemory) {
		rt::eh_vector_constructor_iterator(pMemory, sizeof(NiPoint3), static_cast<int32_t>(uiVertexCount),
			&emptyPointConstructor);
		pTexCoords = static_cast<NiPoint3*>(pMemory);
	}
	NiPoint3* pVertices = nullptr;
	NiPoint3* pNormals = nullptr;
	pMemory = ourMalloc(uiVertexCount > SIZE_MAX / sizeof(NiPoint3) ? SIZE_MAX : sizeof(NiPoint3) * uiVertexCount);
	pVertices = nullptr;
	if (pMemory) {
		rt::eh_vector_constructor_iterator(pMemory, sizeof(NiPoint3), static_cast<int32_t>(uiVertexCount),
			&emptyPointConstructor);
		pVertices = static_cast<NiPoint3*>(pMemory);
	}
	pMemory = ourMalloc(uiVertexCount > SIZE_MAX / sizeof(NiPoint3) ? SIZE_MAX : sizeof(NiPoint3) * uiVertexCount);
	pNormals = nullptr;
	if (pMemory) {
		rt::eh_vector_constructor_iterator(pMemory, sizeof(NiPoint3), static_cast<int32_t>(uiVertexCount),
			&emptyPointConstructor);
		pNormals = static_cast<NiPoint3*>(pMemory);
	}
	pSkinInstance->Deform(spTargetGeom.operator->()->GetModelData(), pVertices, pNormals,
		static_cast<NiPoint3*>(nullptr), static_cast<NiPoint3*>(nullptr), sizeof(NiPoint3));

	// The new bones: the skin-to-bone transforms copied, room for four weights per vertex on bone 0 and one per
	// vertex on the others.
	pSkinData = pSkinInstance->GetSkinData();
	pSkinBoneData = pSkinData->GetBoneData();
	iBones = static_cast<int32_t>(pSkinData->GetBoneCount());
	uint32_t uiBones = static_cast<uint32_t>(iBones);
	pMemory = ourMalloc(uiBones > SIZE_MAX / sizeof(NiSkinData::BoneData) ? SIZE_MAX
																		  : sizeof(NiSkinData::BoneData) * uiBones);
	NiSkinData::BoneData* pNewBones = nullptr;
	if (pMemory) {
		rt::eh_vector_constructor_iterator(pMemory, sizeof(NiSkinData::BoneData), static_cast<int32_t>(uiBones),
			&fnvd::constructElement<NiSkinData::BoneData>);
		pNewBones = static_cast<NiSkinData::BoneData*>(pMemory);
	}
	pBoneData = pNewBones;
	for (int32_t i = 0; i < iBones; ++i) {
		NiSkinData::BoneData* pBone = &pBoneData[i];
		pBone->m_kSkinToBone = pSkinBoneData[i].m_kSkinToBone;
		uint32_t uiCount = i == 0 ? uiVertexCount << 2 : uiVertexCount;
		pMemory = ourMalloc(uiCount > SIZE_MAX / sizeof(NiSkinData::BoneVertData)
				? SIZE_MAX
				: sizeof(NiSkinData::BoneVertData) * uiCount);
		NiSkinData::BoneVertData* pVertData = nullptr;
		if (pMemory) {
			rt::eh_vector_constructor_iterator(pMemory, sizeof(NiSkinData::BoneVertData), static_cast<int32_t>(uiCount),
				&fnvd::constructElement<NiSkinData::BoneVertData>);
			pVertData = static_cast<NiSkinData::BoneVertData*>(pMemory);
		}
		pBone->m_pkBoneVertData = pVertData;
	}

	// Each skinned vertex in the decal's texture space: x and y scaled by fScale around 0.5, z the weight that falls
	// off with the normal's angle to the decal direction and with the depth.
	NiPoint3 kDirection = kLocalVector;
	NiPoint3 kVertex;
	kVertex.~NiPoint3();
	::new (&kVertex) NiPoint3;
	NiPoint3 kNormal;
	kNormal.~NiPoint3();
	::new (&kNormal) NiPoint3;
	for (int32_t i = 0; i < iVertexCount; ++i) {
		NiPoint3 kTexCoord = NiPoint3_ZERO;
		kVertex = pVertices[i];
		kNormal = pNormals[i];
		NiPoint3 kWorldVertex = NiPoint3_ZERO;
		kVertex *= kWorld.m_fScale;
		NiMatrix3::TransformVertices(kWorld.m_Rotate, kWorld.m_Translate, 1, &kVertex, &kWorldVertex);
		NiMatrix3::TransformVertices(kProjection, NiPoint3_ZERO, 1, &kWorldVertex, &kTexCoord);
		pTexCoords[i].x = static_cast<float>(static_cast<double>(kTexCoord.x) / fScale + 0.5);
		pTexCoords[i].y = static_cast<float>(static_cast<double>(kTexCoord.y) / fScale + 0.5);
		float fFacing = static_cast<float>(1.0 - BSMAX(0.0f, kDirection.Dot(kNormal)));
		// pow's result is kept as a double across the fabs call.
		double dFalloff = std::pow(fFacing, 10);
		pTexCoords[i].z = static_cast<float>((1.0 - static_cast<double>(std::fabs(kTexCoord.z)) / 256.0) * dFalloff);
	}

	// Copies the covered triangles of each covered partition: the decal's own indices and vertex map, and the
	// partition's indices, vertex map, bone weights and bone palette.
	uint16_t usNewIndices = 0;
	uint16_t usNewVertices = 0;
	iPartitions = 0;
	for (int32_t i = 0; i < iPartitionCount; ++i) {
		bool bEnabled = pDismemberSkinInstance->GetPartitionDataArray()[i].bEnabled;
		uint16_t usBodyPart = pDismemberSkinInstance->GetPartitionDataArray()[i].usBodyPartNum;
		if (usBodyPart > 1000) {
			usBodyPart = static_cast<uint16_t>(usBodyPart / 1000);
		} else if (usBodyPart > 100) {
			while (usBodyPart > 100) {
				usBodyPart = static_cast<uint16_t>(usBodyPart - 100);
			}
		}
		int32_t iBodyPart = usBodyPart;
		if (!bEnabled || (iBodyParts & (1u << ((iBodyPart + 1) & 31))) == 0) {
			continue;
		}
		usTriangles = pPartitions[i].m_usTriangles;
		pusTriList = pPartitions[i].m_pusTriList;
		pusVertexMap = pPartitions[i].m_pusVertexMap;
		bStrips = pPartitions[i].m_usStrips > 0;
		uint16_t usPartIndices = 0;
		uint16_t usPartVertices = 0;
		ourMemset(piPartVertexMap, -1, uiVertexCount << 2);
		ourMemset(pfPartWeights, 0, (uiVertexCount << 2) << 2);
		ourMemset(pucPartBonePalette, 0, uiVertexCount << 2);
		int32_t bFlip = 0;
		int32_t iIndex = 0;
		for (int32_t j = 0; j < usTriangles; ++j) {
			bool bAdd = false;
			bool bAllowed = true;
			uint16_t usVertex0 = pusTriList[iIndex];
			uint16_t usVertex1 = pusTriList[iIndex + 1];
			uint16_t usVertex2 = pusTriList[iIndex + 2];
			usVertex0 = pusVertexMap[usVertex0];
			usVertex1 = pusVertexMap[usVertex1];
			usVertex2 = pusVertexMap[usVertex2];
			// A degenerate triangle (a strip's joint) is skipped.
			if (usVertex0 == usVertex1 || usVertex0 == usVertex2 || usVertex1 == usVertex2) {
				if (bStrips) {
					bFlip = bFlip == 0;
					++iIndex;
				} else {
					iIndex += 3;
				}
				continue;
			}
			bAdd = ShouldAddTriangle(static_cast<uint32_t>(j), 0, bStrips != 0, pusTriList, pTexCoords, pusVertexMap);
			if (bAdd && bAllowed) {
				for (int32_t k = 0; k < 3; ++k) {
					uint16_t usIndex = pusTriList[iIndex + k];
					uint16_t usVertex = pusVertexMap[usIndex];
					if (piVertexMap[usVertex] == -1) {
						piVertexMap[usVertex] = usNewVertices;
						pusNewIndex[usNewIndices] = usNewVertices;
						++usNewIndices;
						++usNewVertices;
					} else {
						pusNewIndex[usNewIndices] = static_cast<uint16_t>(piVertexMap[usVertex]);
						++usNewIndices;
					}
					if (piPartVertexMap[usVertex] == -1) {
						for (int32_t l = 0; l < pPartitions[i].m_usBonesPerVertex; ++l) {
							int32_t iSrc = usIndex * pPartitions[i].m_usBonesPerVertex + l;
							int32_t iDst = usPartVertices * pPartitions[i].m_usBonesPerVertex + l;
							pfPartWeights[iDst] = pPartitions[i].m_pfWeights[iSrc];
							pucPartBonePalette[iDst] = pPartitions[i].m_pucBonePalette[iSrc];
						}
						piPartVertexMap[usVertex] = usPartVertices;
						pusPartIndex[usPartIndices] = usPartVertices;
						++usPartIndices;
						pusPartVertexMap[usPartVertices] = static_cast<uint16_t>(piVertexMap[usVertex]);
						++usPartVertices;
					} else {
						pusPartIndex[usPartIndices] = static_cast<uint16_t>(piPartVertexMap[usVertex]);
						++usPartIndices;
					}
				}
				// A flipped strip triangle is wound the other way round.
				if (bStrips && bFlip) {
					int32_t iFirst = pusNewIndex[usNewIndices - 3];
					int32_t iLast = pusNewIndex[usNewIndices - 1];
					pusNewIndex[usNewIndices - 3] = static_cast<uint16_t>(iLast);
					pusNewIndex[usNewIndices - 1] = static_cast<uint16_t>(iFirst);
					iFirst = pusPartIndex[usPartIndices - 3];
					iLast = pusPartIndex[usPartIndices - 1];
					pusPartIndex[usPartIndices - 3] = static_cast<uint16_t>(iLast);
					pusPartIndex[usPartIndices - 1] = static_cast<uint16_t>(iFirst);
				}
			}
			if (bStrips) {
				bFlip = bFlip == 0;
				++iIndex;
			} else {
				iIndex += 3;
			}
		}
		if (usPartVertices > 0) {
			NiSkinPartition::Partition* pNew = pNewPartitions;
			pNew[iPartitions].m_usTriangles = static_cast<uint16_t>(usPartIndices / 3);
			pNew[iPartitions].m_pusTriList = static_cast<uint16_t*>(_NiMalloc(usPartIndices * sizeof(uint16_t)));
			ourMemcpy(pNew[iPartitions].m_pusTriList, pusPartIndex, usPartIndices * sizeof(uint16_t));
			pNew[iPartitions].m_usVertices = usPartVertices;
			pNew[iPartitions].m_pusVertexMap = static_cast<uint16_t*>(_NiMalloc(usPartVertices * sizeof(uint16_t)));
			ourMemcpy(pNew[iPartitions].m_pusVertexMap, pusPartVertexMap, usPartVertices * sizeof(uint16_t));
			pNew[iPartitions].m_usBones = pPartitions[i].m_usBones;
			pNew[iPartitions].m_pusBones =
				static_cast<uint16_t*>(_NiMalloc(pPartitions[i].m_usBones * sizeof(uint16_t)));
			ourMemcpy(pNew[iPartitions].m_pusBones, pPartitions[i].m_pusBones,
				pPartitions[i].m_usBones * sizeof(uint16_t));
			pNew[iPartitions].m_usBonesPerVertex = pPartitions[i].m_usBonesPerVertex;
			uint32_t uiPartInteractions = pNew[iPartitions].m_usBonesPerVertex * usPartVertices;
			pNew[iPartitions].m_pfWeights = static_cast<float*>(_NiMalloc(uiPartInteractions * sizeof(float)));
			pNew[iPartitions].m_pucBonePalette = static_cast<uint8_t*>(_NiMalloc(uiPartInteractions));
			ourMemcpy(pNew[iPartitions].m_pfWeights, pfPartWeights,
				(pNew[iPartitions].m_usBonesPerVertex * 4u) * pNew[iPartitions].m_usVertices);
			ourMemcpy(pNew[iPartitions].m_pucBonePalette, pucPartBonePalette,
				static_cast<uint32_t>(pNew[iPartitions].m_usBonesPerVertex * pNew[iPartitions].m_usVertices));
			if (static_cast<uint32_t>(iPartitions) >= uiBodyPartCount) {
				platform::DebuggerBreak();
			}
			pusBodyParts[iPartitions] = usBodyPart;
			++iPartitions;
		}
	}

	NiSkinPartition* pSkinPartition = nullptr;
	if (iPartitions > 0) {
		pSkinPartition = new NiSkinPartition;
		pSkinPartition->ReplacePartitions(pNewPartitions, static_cast<uint32_t>(iPartitions));
	}
	if (usNewVertices > 0) {
		uint32_t uiCount = usNewIndices;
		uint16_t* pusGeomIndex =
			static_cast<uint16_t*>(ourMalloc(uiCount <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * uiCount
																					: SIZE_MAX));
		ourMemcpy(pusGeomIndex, pusNewIndex, usNewIndices * sizeof(uint16_t));
		NiPoint3* pModelVertices = spTargetGeom.operator->()->GetModelData()->GetVertices();
		// The normals are read but the texture coordinates are used as the decal's second array, as the game does.
		[[maybe_unused]] NiPoint3* pModelNormals = spTargetGeom.operator->()->GetModelData()->GetNormals();
		uiCount = usNewVertices;
		pMemory = ourMalloc(uiCount > SIZE_MAX / sizeof(NiPoint3) ? SIZE_MAX : sizeof(NiPoint3) * uiCount);
		NiPoint3* pGeomVertex = nullptr;
		if (pMemory) {
			rt::eh_vector_constructor_iterator(pMemory, sizeof(NiPoint3), static_cast<int32_t>(uiCount),
				&emptyPointConstructor);
			pGeomVertex = static_cast<NiPoint3*>(pMemory);
		}
		uiCount = usNewVertices;
		pMemory = ourMalloc(uiCount > SIZE_MAX / sizeof(NiPoint3) ? SIZE_MAX : sizeof(NiPoint3) * uiCount);
		NiPoint3* pGeomTexCoord = nullptr;
		if (pMemory) {
			rt::eh_vector_constructor_iterator(pMemory, sizeof(NiPoint3), static_cast<int32_t>(uiCount),
				&emptyPointConstructor);
			pGeomTexCoord = static_cast<NiPoint3*>(pMemory);
		}
		for (int32_t i = 0; i < iVertexCount; ++i) {
			if (piVertexMap[i] != -1) {
				pGeomVertex[piVertexMap[i]] = pModelVertices[i];
				pGeomTexCoord[piVertexMap[i]] = pTexCoords[i];
			}
		}
		// Each new bone keeps the weights of the kept vertices, renumbered, in an array of the kept size.
		for (int32_t i = 0; i < iBones; ++i) {
			NiSkinData::BoneData* pOldBone = &pSkinBoneData[i];
			NiSkinData::BoneData* pNewBone = &pBoneData[i];
			for (int32_t j = 0; j < pOldBone->m_usVerts; ++j) {
				uint16_t usVert = pOldBone->m_pkBoneVertData[j].m_usVert;
				if (piVertexMap[usVert] != -1) {
					pNewBone->m_pkBoneVertData[pNewBone->m_usVerts].m_usVert =
						static_cast<uint16_t>(piVertexMap[usVert]);
					pNewBone->m_pkBoneVertData[pNewBone->m_usVerts].m_fWeight = pOldBone->m_pkBoneVertData[j].m_fWeight;
					++pNewBone->m_usVerts;
				}
			}
			NiSkinData::BoneVertData* pOldVertData = pNewBone->m_pkBoneVertData;
			uint32_t uiVerts = pNewBone->m_usVerts;
			pMemory = ourMalloc(uiVerts > SIZE_MAX / sizeof(NiSkinData::BoneVertData)
					? SIZE_MAX
					: sizeof(NiSkinData::BoneVertData) * uiVerts);
			NiSkinData::BoneVertData* pVertData = nullptr;
			if (pMemory) {
				rt::eh_vector_constructor_iterator(pMemory, sizeof(NiSkinData::BoneVertData),
					static_cast<int32_t>(uiVerts), &fnvd::constructElement<NiSkinData::BoneVertData>);
				pVertData = static_cast<NiSkinData::BoneVertData*>(pMemory);
			}
			pBoneData[i].m_pkBoneVertData = pVertData;
			ourMemcpy(pBoneData[i].m_pkBoneVertData, pOldVertData,
				pNewBone->m_usVerts * sizeof(NiSkinData::BoneVertData));
			ourFree(pOldVertData);
		}
		if (pSkinPartition) {
			CreateGeometry(spTargetGeom.operator->(), usNewVertices, usNewIndices, pGeomVertex, pGeomTexCoord,
				pusGeomIndex, pBoneData, pSkinPartition, pusBodyParts);
		} else {
			CreateGeometry(spTargetGeom.operator->(), usNewVertices, usNewIndices, pGeomVertex, pGeomTexCoord,
				pusGeomIndex, pBoneData, nullptr, nullptr);
		}
		iNumVerts = usNewVertices;
	} else {
		for (int32_t i = 0; i < iBones; ++i) {
			ourFree(pBoneData[i].m_pkBoneVertData);
			pBoneData[i].m_pkBoneVertData = nullptr;
		}
		ourFree(pBoneData);
	}
	ourFree(pVertices);
	ourFree(pNormals);
	ourFree(pusNewIndex);
	ourFree(pusPartIndex);
	ourFree(pusPartVertexMap);
	ourFree(piVertexMap);
	ourFree(piPartVertexMap);
	ourFree(pfPartWeights);
	ourFree(pucPartBonePalette);
	ourFree(pTexCoords);
	ourFree(pusBodyParts);
}

// Deliberately keeps an uninitialised read, as the game does: with a partition and no body part index array, the
// partition data's body parts are filled from ibodyPartsA, which only the no-partition branch writes, for up to the
// partition count's entries (past its 15). Both callers pass the two together.
void BSTempEffectGeometryDecal::CreateGeometry(NiTriBasedGeom* apGeometry, uint16_t aiNewVertex, uint16_t aiNewIndex,
	NiPoint3* apGeomVertex, NiPoint3* apGeomTexCoord, uint16_t* apGeomIndex, NiSkinData::BoneData* apGeomBoneData,
	NiSkinPartition* apPartition, uint16_t* apBodyPartIndex) {
	BSDismemberSkinInstance* pnewSkinInstance;
	NiSkinData* pnewSkinData;
	GeometryDecalShaderProperty* pshaderprop;
	NiTriShape* pnewDecal;
	uint32_t ibodyPartsA[15];
	NiTriShape* v31 = static_cast<NiTriShape*>(NiMemObject::operator new(sizeof(NiTriShape)));
	if (v31) {
		pnewDecal = ::new (v31) NiTriShape(aiNewVertex, apGeomVertex, apGeomTexCoord, nullptr, nullptr, 0,
			NiGeometryData::NBT_METHOD_NONE, aiNewIndex / 3, apGeomIndex);
	} else {
		pnewDecal = 0;
	}
	pnewDecal->SetName(*FixedStrings::GetDecal());
	GeometryDecalShaderProperty* apMemory =
		static_cast<GeometryDecalShaderProperty*>(NiMemObject::operator new(sizeof(GeometryDecalShaderProperty)));
	if (apMemory) {
		pshaderprop = ::new (apMemory) GeometryDecalShaderProperty;
	} else {
		pshaderprop = 0;
	}
	pnewDecal->AttachProperty(pshaderprop);
	pshaderprop->SetDepthBias(1.0f);
	NiPointer<NiTexture> sptexture(static_cast<NiTexture*>(0));
	pTextureSet->GetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, sptexture);
	NiTexture* ptexture = sptexture.m_pObject;
	if (NiObject::IsKindOf_0(&NiSourceTexture::ms_RTTI, ptexture)) {
		pshaderprop->spDecalTexture.operator=(static_cast<NiSourceTexture*>(ptexture));
	}
	NiSkinInstance* pskin = apGeometry->GetSkinInstance();
	int32_t iboneCount = pskin->GetSkinData()->m_uiBones;
	NiSkinData* v29 = static_cast<NiSkinData*>(NiMemObject::operator new(sizeof(NiSkinData)));
	if (v29) {
		NiTransform* v13 = &pskin->GetSkinData()->m_kRootParentToSkin;
		pnewSkinData = ::new (v29) NiSkinData(iboneCount, apGeomBoneData, *v13, apGeomVertex);
	} else {
		pnewSkinData = 0;
	}
	NiAVObject** ppbones = pskin->m_ppkBones;
	// Checked array allocation: the count is taken unsigned.
	NiAVObject** ppnewBones =
		static_cast<NiAVObject**>(ourMalloc(static_cast<uint32_t>(iboneCount) <= SIZE_MAX / sizeof(NiAVObject*)
				? sizeof(NiAVObject*) * static_cast<uint32_t>(iboneCount)
				: SIZE_MAX));
	for (int32_t bone = 0; bone < iboneCount; ++bone) {
		ppnewBones[bone] = ppbones[bone];
	}
	int32_t icount = 0;
	if (apPartition) {
		icount = apPartition->m_uiPartitions;
	} else {
		uint32_t J = 2;
		for (uint32_t I = 0; I < 15; ++I) {
			if ((J & iBodyParts) != 0) {
				ibodyPartsA[icount++] = I;
			}
			J *= 2;
		}
	}
	BSDismemberSkinInstance::PartitionData* ppartitionExtraDataArray =
		static_cast<BSDismemberSkinInstance::PartitionData*>(ourMalloc(static_cast<uint32_t>(icount) <=
					SIZE_MAX / sizeof(BSDismemberSkinInstance::PartitionData)
				? sizeof(BSDismemberSkinInstance::PartitionData) * static_cast<uint32_t>(icount)
				: SIZE_MAX));
	for (int32_t i = 0; i < icount; ++i) {
		uint16_t v10;
		ppartitionExtraDataArray[i].bEnabled = 1;
		ppartitionExtraDataArray[i].bStateChange = 1;
		if (apBodyPartIndex) {
			v10 = apBodyPartIndex[i];
		} else {
			v10 = ibodyPartsA[i];
		}
		ppartitionExtraDataArray[i].usBodyPartNum = v10;
	}
	BSDismemberSkinInstance* v28 =
		static_cast<BSDismemberSkinInstance*>(NiMemObject::operator new(sizeof(BSDismemberSkinInstance)));
	if (v28) {
		NiAVObject* v11 = pskin->m_pkRootParent;
		pnewSkinInstance =
			::new (v28) BSDismemberSkinInstance(pnewSkinData, v11, ppnewBones, ppartitionExtraDataArray, icount);
	} else {
		pnewSkinInstance = 0;
	}
	if (apPartition) {
		pnewSkinInstance->SetSkinPartition(apPartition);
		pnewDecal->SetSkinInstance(pnewSkinInstance);
	} else {
		NiSkinPartition* ppartition;
		NiSkinPartition* v27 = static_cast<NiSkinPartition*>(NiMemObject::operator new(sizeof(NiSkinPartition)));
		if (v27) {
			ppartition = ::new (v27) NiSkinPartition;
		} else {
			ppartition = 0;
		}
		if (ppartition->MakePartitions(static_cast<NiTriBasedGeomData*>(pnewDecal->GetModelData()), pnewSkinData, 18, 4,
				1, 0, 0, 0)) {
			pnewSkinInstance->SetSkinPartition(ppartition);
			pnewDecal->SetSkinInstance(pnewSkinInstance);
		} else {
			if (ppartition) {
				delete ppartition;
			}
			if (pnewSkinInstance) {
				delete pnewSkinInstance;
			}
		}
	}
	const NiPoint3* a2 = &spTargetGeom.m_pObject->GetTranslate();
	pnewDecal->SetTranslate(*a2);
	NiTransform* a2a = &spTargetGeom.m_pObject->m_kLocal;
	pnewDecal->SetRotate(a2a->m_Rotate);
	float a2b = spTargetGeom.m_pObject->m_kLocal.m_fScale;
	pnewDecal->SetScale(a2b);
	const NiPoint3* a2c = &spTargetGeom.m_pObject->GetWorldTranslate();
	pnewDecal->SetWorldTranslate(*a2c);
	const NiTransform* a2d = &spTargetGeom.m_pObject->GetWorldTransform();
	pnewDecal->SetWorldRotate(a2d->m_Rotate);
	float a2e = spTargetGeom.m_pObject->m_kWorld.m_fScale;
	pnewDecal->SetWorldScale(a2e);
	pnewDecal->UpdateWorldBound();
	pnewDecal->UpdateProperties();
	BSShaderManager::PrepareObject(pnewDecal, 0, 0);
	spDecalGeometry.operator=(static_cast<NiTriBasedGeom*>(pnewDecal));
	pGeometry = spTargetGeom.m_pObject;
}

NiBlendWeights::NiBlendWeights() : fWeights{0.0f, 0.0f, 0.0f, 0.0f} {}

BSDismemberSkinInstance::PartitionData* BSDismemberSkinInstance::GetPartitionDataArray() const {
	return pPartitionDataArray;
}

uint16_t NiAdditionalGeometryData::GetVertexCount() const {
	return m_usVertexCount;
}

// The additional geometry data of the model data, null without model data.
NiAdditionalGeometryData* NiGeometry::GetAdditionalGeometryData() {
	if (!m_spModelData.operator->()) {
		return nullptr;
	}
	return m_spModelData.operator->()->GetAdditionalGeometryData();
}

NiSkinData::BoneData* NiSkinData::GetBoneData() const {
	return m_pkBoneData;
}

// Skins auiVertices model vertices (and normals, binormals and tangents when given) into the destination arrays: each
// bone's bone-to-skin transform, weighted per vertex. The binormals and tangents follow the normals in the source
// array. The destination arrays are raw vertex buffers auiStride bytes apart, walked as bytes (hence the char casts).
// The rounding is deliberately the game's: the scaled rotation, every transformed point and, with normals, every
// weighted point are stored as floats; without normals the weighted vertex is added unrounded.
void NiSkinInstance::Deform(const NiPoint3* apSrcVertex, const NiPoint3* apSrcNormal, uint32_t auiVertices,
	NiPoint3* apDstVertex, NiPoint3* apDstNormal, NiPoint3* apDstBinormal, NiPoint3* apDstTangent, uint32_t auiStride) {
	NiTransform kWorldToRootParent;
	m_pkRootParent->m_kWorld.Invert(kWorldToRootParent);
	NiTransform kWorldToSkin = m_spSkinData.m_pObject->m_kRootParentToSkin * kWorldToRootParent;
	NiSkinData* pkSkinData = m_spSkinData.m_pObject;
	NiSkinData::BoneData* pkBoneData = pkSkinData->m_pkBoneData;
	uint32_t uiBones = pkSkinData->m_uiBones;

	if (apSrcNormal && apDstNormal) {
		if (apDstBinormal && apDstTangent) {
			const NiPoint3* pkSrcBinormal = apSrcNormal + auiVertices;
			const NiPoint3* pkSrcTangent = pkSrcBinormal + auiVertices;
			if (auiStride == sizeof(NiPoint3)) {
				std::memset(apDstVertex, 0, sizeof(NiPoint3) * auiVertices);
				std::memset(apDstNormal, 0, sizeof(NiPoint3) * auiVertices);
				std::memset(apDstBinormal, 0, sizeof(NiPoint3) * auiVertices);
				std::memset(apDstTangent, 0, sizeof(NiPoint3) * auiVertices);
			} else {
				NiPoint3* pkV = apDstVertex;
				NiPoint3* pkN = apDstNormal;
				NiPoint3* pkB = apDstBinormal;
				NiPoint3* pkT = apDstTangent;
				for (uint32_t i = auiVertices; i; --i) {
					*pkV = NiPoint3_ZERO;
					*pkN = NiPoint3_ZERO;
					*pkB = NiPoint3_ZERO;
					*pkT = NiPoint3_ZERO;
					pkV = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(pkV) + auiStride);
					pkN = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(pkN) + auiStride);
					pkB = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(pkB) + auiStride);
					pkT = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(pkT) + auiStride);
				}
			}

			for (uint32_t i = 0; i < uiBones; ++i) {
				NiSkinData::BoneData* pkBone = &pkBoneData[i];
				uint32_t uiVerts = pkBone->m_usVerts;
				NiTransform kBoneToSkin = kWorldToSkin * m_ppkBones[i]->m_kWorld * pkBone->m_kSkinToBone;
				const NiMatrix3& kRot = kBoneToSkin.m_Rotate;
				float fScale = kBoneToSkin.m_fScale;
				float s00 = kRot.m_pEntry[0][0] * fScale;
				float s01 = kRot.m_pEntry[0][1] * fScale;
				float s02 = kRot.m_pEntry[0][2] * fScale;
				float s10 = kRot.m_pEntry[1][0] * fScale;
				float s11 = kRot.m_pEntry[1][1] * fScale;
				float s12 = kRot.m_pEntry[1][2] * fScale;
				float s20 = kRot.m_pEntry[2][0] * fScale;
				float s21 = kRot.m_pEntry[2][1] * fScale;
				float s22 = kRot.m_pEntry[2][2] * fScale;
				NiSkinData::BoneVertData* pkVertData = pkBone->m_pkBoneVertData;
				for (uint32_t j = 0; j < uiVerts; ++j) {
					uint32_t uiVert = pkVertData[j].m_usVert;
					float fWeight = pkVertData[j].m_fWeight;
					uint32_t uiOffset = uiVert * auiStride;
					const NiPoint3& kSrcV = apSrcVertex[uiVert];
					const NiPoint3& kSrcN = apSrcNormal[uiVert];
					const NiPoint3& kSrcB = pkSrcBinormal[uiVert];
					const NiPoint3& kSrcT = pkSrcTangent[uiVert];
					NiPoint3 kV;
					kV.x = s00 * kSrcV.x + s01 * kSrcV.y + s02 * kSrcV.z + kBoneToSkin.m_Translate.x;
					kV.y = s10 * kSrcV.x + s11 * kSrcV.y + s12 * kSrcV.z + kBoneToSkin.m_Translate.y;
					kV.z = s20 * kSrcV.x + s21 * kSrcV.y + s22 * kSrcV.z + kBoneToSkin.m_Translate.z;
					NiPoint3 kN;
					kN.x =
						kRot.m_pEntry[0][0] * kSrcN.x + kRot.m_pEntry[0][1] * kSrcN.y + kRot.m_pEntry[0][2] * kSrcN.z;
					kN.y =
						kRot.m_pEntry[1][0] * kSrcN.x + kRot.m_pEntry[1][1] * kSrcN.y + kRot.m_pEntry[1][2] * kSrcN.z;
					kN.z =
						kRot.m_pEntry[2][0] * kSrcN.x + kRot.m_pEntry[2][1] * kSrcN.y + kRot.m_pEntry[2][2] * kSrcN.z;
					NiPoint3 kB;
					kB.x =
						kRot.m_pEntry[0][0] * kSrcB.x + kRot.m_pEntry[0][1] * kSrcB.y + kRot.m_pEntry[0][2] * kSrcB.z;
					kB.y =
						kRot.m_pEntry[1][0] * kSrcB.x + kRot.m_pEntry[1][1] * kSrcB.y + kRot.m_pEntry[1][2] * kSrcB.z;
					kB.z =
						kRot.m_pEntry[2][0] * kSrcB.x + kRot.m_pEntry[2][1] * kSrcB.y + kRot.m_pEntry[2][2] * kSrcB.z;
					NiPoint3 kT;
					kT.x =
						kRot.m_pEntry[0][0] * kSrcT.x + kRot.m_pEntry[0][1] * kSrcT.y + kRot.m_pEntry[0][2] * kSrcT.z;
					kT.y =
						kRot.m_pEntry[1][0] * kSrcT.x + kRot.m_pEntry[1][1] * kSrcT.y + kRot.m_pEntry[1][2] * kSrcT.z;
					kT.z =
						kRot.m_pEntry[2][0] * kSrcT.x + kRot.m_pEntry[2][1] * kSrcT.y + kRot.m_pEntry[2][2] * kSrcT.z;

					NiPoint3* pkDst = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(apDstVertex) + uiOffset);
					NiPoint3 kW;
					kW.x = kV.x * fWeight;
					kW.y = kV.y * fWeight;
					kW.z = kV.z * fWeight;
					pkDst->x += kW.x;
					pkDst->y += kW.y;
					pkDst->z += kW.z;
					pkDst = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(apDstNormal) + uiOffset);
					kW.x = kN.x * fWeight;
					kW.y = kN.y * fWeight;
					kW.z = kN.z * fWeight;
					pkDst->x += kW.x;
					pkDst->y += kW.y;
					pkDst->z += kW.z;
					pkDst = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(apDstBinormal) + uiOffset);
					kW.x = kB.x * fWeight;
					kW.y = kB.y * fWeight;
					kW.z = kB.z * fWeight;
					pkDst->x += kW.x;
					pkDst->y += kW.y;
					pkDst->z += kW.z;
					pkDst = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(apDstTangent) + uiOffset);
					kW.x = kT.x * fWeight;
					kW.y = kT.y * fWeight;
					kW.z = kT.z * fWeight;
					pkDst->x += kW.x;
					pkDst->y += kW.y;
					pkDst->z += kW.z;
				}
			}
			NiPoint3::UnitizeVectors(apDstNormal, auiVertices, auiStride);
			NiPoint3::UnitizeVectors(apDstBinormal, auiVertices, auiStride);
			NiPoint3::UnitizeVectors(apDstTangent, auiVertices, auiStride);
		} else {
			if (auiStride == sizeof(NiPoint3)) {
				std::memset(apDstVertex, 0, sizeof(NiPoint3) * auiVertices);
				std::memset(apDstNormal, 0, sizeof(NiPoint3) * auiVertices);
			} else {
				NiPoint3* pkV = apDstVertex;
				NiPoint3* pkN = apDstNormal;
				for (uint32_t i = auiVertices; i; --i) {
					*pkV = NiPoint3_ZERO;
					*pkN = NiPoint3_ZERO;
					pkV = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(pkV) + auiStride);
					pkN = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(pkN) + auiStride);
				}
			}

			for (uint32_t i = 0; i < uiBones; ++i) {
				NiSkinData::BoneData* pkBone = &pkBoneData[i];
				uint32_t uiVerts = pkBone->m_usVerts;
				NiTransform kBoneToSkin = kWorldToSkin * m_ppkBones[i]->m_kWorld * pkBone->m_kSkinToBone;
				const NiMatrix3& kRot = kBoneToSkin.m_Rotate;
				float fScale = kBoneToSkin.m_fScale;
				float s00 = kRot.m_pEntry[0][0] * fScale;
				float s01 = kRot.m_pEntry[0][1] * fScale;
				float s02 = kRot.m_pEntry[0][2] * fScale;
				float s10 = kRot.m_pEntry[1][0] * fScale;
				float s11 = kRot.m_pEntry[1][1] * fScale;
				float s12 = kRot.m_pEntry[1][2] * fScale;
				float s20 = kRot.m_pEntry[2][0] * fScale;
				float s21 = kRot.m_pEntry[2][1] * fScale;
				float s22 = kRot.m_pEntry[2][2] * fScale;
				NiSkinData::BoneVertData* pkVertData = pkBone->m_pkBoneVertData;
				for (uint32_t j = 0; j < uiVerts; ++j) {
					uint32_t uiVert = pkVertData[j].m_usVert;
					float fWeight = pkVertData[j].m_fWeight;
					uint32_t uiOffset = uiVert * auiStride;
					const NiPoint3& kSrcV = apSrcVertex[uiVert];
					const NiPoint3& kSrcN = apSrcNormal[uiVert];
					NiPoint3 kV;
					kV.x = s00 * kSrcV.x + s01 * kSrcV.y + s02 * kSrcV.z + kBoneToSkin.m_Translate.x;
					kV.y = s10 * kSrcV.x + s11 * kSrcV.y + s12 * kSrcV.z + kBoneToSkin.m_Translate.y;
					kV.z = s20 * kSrcV.x + s21 * kSrcV.y + s22 * kSrcV.z + kBoneToSkin.m_Translate.z;
					NiPoint3 kN;
					kN.x =
						kRot.m_pEntry[0][0] * kSrcN.x + kRot.m_pEntry[0][1] * kSrcN.y + kRot.m_pEntry[0][2] * kSrcN.z;
					kN.y =
						kRot.m_pEntry[1][0] * kSrcN.x + kRot.m_pEntry[1][1] * kSrcN.y + kRot.m_pEntry[1][2] * kSrcN.z;
					kN.z =
						kRot.m_pEntry[2][0] * kSrcN.x + kRot.m_pEntry[2][1] * kSrcN.y + kRot.m_pEntry[2][2] * kSrcN.z;

					NiPoint3* pkDst = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(apDstVertex) + uiOffset);
					NiPoint3 kW;
					kW.x = kV.x * fWeight;
					kW.y = kV.y * fWeight;
					kW.z = kV.z * fWeight;
					pkDst->x += kW.x;
					pkDst->y += kW.y;
					pkDst->z += kW.z;
					pkDst = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(apDstNormal) + uiOffset);
					kW.x = kN.x * fWeight;
					kW.y = kN.y * fWeight;
					kW.z = kN.z * fWeight;
					pkDst->x += kW.x;
					pkDst->y += kW.y;
					pkDst->z += kW.z;
				}
			}
			NiPoint3::UnitizeVectors(apDstNormal, auiVertices, auiStride);
		}
	} else {
		if (auiStride == sizeof(NiPoint3)) {
			std::memset(apDstVertex, 0, sizeof(NiPoint3) * auiVertices);
		} else {
			NiPoint3* pkV = apDstVertex;
			for (uint32_t i = auiVertices; i; --i) {
				*pkV = NiPoint3_ZERO;
				pkV = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(pkV) + auiStride);
			}
		}

		for (uint32_t i = 0; i < uiBones; ++i) {
			NiSkinData::BoneData* pkBone = &pkBoneData[i];
			uint32_t uiVerts = pkBone->m_usVerts;
			NiTransform kBoneToSkin = kWorldToSkin * m_ppkBones[i]->m_kWorld * pkBone->m_kSkinToBone;
			const NiMatrix3& kRot = kBoneToSkin.m_Rotate;
			float fScale = kBoneToSkin.m_fScale;
			float s00 = kRot.m_pEntry[0][0] * fScale;
			float s01 = kRot.m_pEntry[0][1] * fScale;
			float s02 = kRot.m_pEntry[0][2] * fScale;
			float s10 = kRot.m_pEntry[1][0] * fScale;
			float s11 = kRot.m_pEntry[1][1] * fScale;
			float s12 = kRot.m_pEntry[1][2] * fScale;
			float s20 = kRot.m_pEntry[2][0] * fScale;
			float s21 = kRot.m_pEntry[2][1] * fScale;
			float s22 = kRot.m_pEntry[2][2] * fScale;
			NiSkinData::BoneVertData* pkVertData = pkBone->m_pkBoneVertData;
			for (uint32_t j = 0; j < uiVerts; ++j) {
				uint32_t uiVert = pkVertData[j].m_usVert;
				float fWeight = pkVertData[j].m_fWeight;
				NiPoint3* pkDst =
					reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(apDstVertex) + uiVert * auiStride);
				const NiPoint3& kSrcV = apSrcVertex[uiVert];
				NiPoint3 kV;
				kV.x = s00 * kSrcV.x + s01 * kSrcV.y + s02 * kSrcV.z + kBoneToSkin.m_Translate.x;
				kV.y = s10 * kSrcV.x + s11 * kSrcV.y + s12 * kSrcV.z + kBoneToSkin.m_Translate.y;
				kV.z = s20 * kSrcV.x + s21 * kSrcV.y + s22 * kSrcV.z + kBoneToSkin.m_Translate.z;
				// The weighted vertex is not rounded to float before it is added here.
				pkDst->x = kV.x * fWeight + pkDst->x;
				pkDst->y = kV.y * fWeight + pkDst->y;
				pkDst->z = kV.z * fWeight + pkDst->z;
			}
		}
	}
}

void NiSkinInstance::Deform([[maybe_unused]] NiGeometryDataIteratorNiPoint3* pkSrcVertex,
	[[maybe_unused]] NiGeometryDataIteratorNiBlendWeights* pkWeights,
	[[maybe_unused]] NiGeometryDataIteratorNiBlendIndices* pkIndices,
	[[maybe_unused]] NiGeometryDataIteratorNiPoint3* pkSrcNormal, [[maybe_unused]] uint32_t uiVertices,
	[[maybe_unused]] NiPoint3* pkDstVertex, [[maybe_unused]] NiPoint3* pkDstNormal,
	[[maybe_unused]] uint32_t uiStride) {}
