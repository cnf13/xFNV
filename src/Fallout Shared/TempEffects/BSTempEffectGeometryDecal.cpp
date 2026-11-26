#include "Fallout Shared/TempEffects/BSTempEffectGeometryDecal.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/GeometryDecalShaderProperty.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSDecalNode.h"
#include "Fallout Shared/BGSTextureSet.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout/AI/FixedStrings.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryDataIterators.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
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
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/System.h"

#include <cmath>

NiRTTI BSTempEffectGeometryDecal::ms_RTTI("BSTempEffectGeometryDecal", &BSTempEffect::ms_RTTI);

BSTempEffectGeometryDecal::BSTempEffectGeometryDecal(
    TESObjectCELL* apCell,
    float afLifetime,
    DECAL_CREATION_DATA* apDecalData,
    NiTriBasedGeom* apGeometry,
    NiPoint3 apOrigin,
    NiPoint3 apVector,
    float afScale,
    float afRotation,
    uint32_t aiBodyParts
)
  : BSTempEffect(apCell, afLifetime), spDecalGeometry(nullptr), pOldIndex(nullptr), bFinished(false),
    spTargetGeom(apGeometry), spParent(apGeometry->m_pkParent), spDecalParent(nullptr), pOrigin(apOrigin),
    pVector(apVector), fScale(afScale), fRotation(afRotation), pTextureSet(apDecalData->pTexSet), iNumVerts(0),
    iBodyParts(aiBodyParts) {}

BSTempEffectGeometryDecal::~BSTempEffectGeometryDecal() {
    if (spDecalGeometry.m_pObject) {
        if (spDecalGeometry->NiAVObject::GetParent()) {
            NiNode* pParent = spDecalGeometry->NiAVObject::GetParent();
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
    if (spTargetGeom->GetSkinInstance()) {
        if (!bInitialized) {
            if (spParent->GetRefCount() > 1) {
                if (spTargetGeom->GetRefCount() > 1) {
                    NiAdditionalGeometryData* pAdditionalGeometryData = spTargetGeom->GetAdditionalGeometryData();
                    if (pAdditionalGeometryData && pAdditionalGeometryData->GetVBReady()) {
                        InitializeUsingAGD();
                    }
                    else {
                        InitializeUsingData();
                    }
                }
            }
        }
        BSTempEffect::Initialize();
        spTargetGeom = static_cast<NiTriBasedGeom*>(nullptr);
        spParent = static_cast<NiNode*>(nullptr);
    }
}

bool BSTempEffectGeometryDecal::Update(float afTimeDelta) {
    if (bFinished) {
        return false;
    }
    bool ret = BSTempEffect::Update(afTimeDelta);
    NiAVObject* pobj = Get3D();
    // The shade property of the decal geometry is its GeometryDecalShaderProperty.
    static_cast<GeometryDecalShaderProperty*>(pobj->GetProperty(NiProperty::SHADE))->fTimer = fAge / fLifetime;
    if (fLifetime < static_cast<double>(fAge)) {
        BGSDecalNode* pdecalnode = nullptr;
        if (spDecalParent.m_pObject) {
            spDecalParent->DetachChild(spDecalGeometry.m_pObject);
            if (spDecalParent->m_pkParent) {
                pdecalnode = static_cast<BGSDecalNode*>(spDecalParent->m_pkParent);
                pdecalnode->RemoveFromDecalList(this);
            }
        }
        if (pdecalnode && !spDecalParent->GetChildCount()) {
            NiNode* pParent = spDecalParent->m_pkParent;
            pParent->DetachChild(spDecalParent.m_pObject);
            --BGSDecalManager::iSkinnedDecalCount;
        }

        bFinished = true;
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
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                isize,
                pheader->iFormID,
                pform->GetFormEditorID(),
                pheader->iFlags,
                1839,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                isize,
                1839,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp"
            );
        }
    }
    return isize;
}

void BSTempEffectGeometryDecal::SaveGame() {
    uint32_t iblockHeader;
    uint16_t iblockSize = 0;
    void* pblockSize = nullptr;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // 'BLOK'.
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        pblockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pBufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* pcEditorID = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                pBufferPointer - itempStartPosition,
                pheader->iFormID,
                pcEditorID,
                iFlags,
                1923,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                pBufferPointer - itempStartPosition,
                1923,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The save buffer is raw bytes: the 2-byte size placeholder is patched in place.
        uint8_t* pcurrent = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pblockStart = static_cast<uint8_t*>(pblockSize);
        if (pcurrent > pblockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp",
                1923
            );
        }
        *static_cast<uint16_t*>(pblockSize) = static_cast<uint16_t>(pcurrent - pblockStart);
    }
}

bool BSTempEffectGeometryDecal::LoadGame() {
    uint32_t iblockHeader;
    uint16_t usBlockSize[3];
    bool bvalid = true;
    usBlockSize[0] = 0;
    uint8_t* istartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout "
                    "Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp",
                    1932,
                    pheader->iFormID,
                    pform->GetFormEditorID(),
                    pheader->cVersion,
                    pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout "
                    "Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp",
                    1932,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(usBlockSize, sizeof(*usBlockSize));
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            if (icurrentBufferPosition <= &istartBufferPosition[usBlockSize[0]]) {
                if (icurrentBufferPosition < &istartBufferPosition[usBlockSize[0]]) {
                    uint32_t iFlags = pheader->iFlags;
                    int32_t cVersion = pheader->cVersion;
                    const char* pcEditorID = pform->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        &istartBufferPosition[usBlockSize[0]] - icurrentBufferPosition,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout "
                        "Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp",
                        2104,
                        pheader->iFormID,
                        pcEditorID,
                        cVersion,
                        iFlags
                    );
                }
            }
            else {
                uint32_t iFlags = pheader->iFlags;
                int32_t cVersion = pheader->cVersion;
                const char* pcEditorID = pform->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    icurrentBufferPosition - &istartBufferPosition[usBlockSize[0]],
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout "
                    "Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp",
                    2104,
                    pheader->iFormID,
                    pcEditorID,
                    cVersion,
                    iFlags
                );
            }
        }
        else if (icurrentBufferPosition <= &istartBufferPosition[usBlockSize[0]]) {
            if (icurrentBufferPosition < &istartBufferPosition[usBlockSize[0]]) {
                int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                    "%i",
                    &istartBufferPosition[usBlockSize[0]] - icurrentBufferPosition,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout "
                    "Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp",
                    2104,
                    iCurrentVersion
                );
            }
        }
        else {
            int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                icurrentBufferPosition - &istartBufferPosition[usBlockSize[0]],
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout "
                "Shared\\TempEffects\\BSTempEffectGeometryDecal.cpp",
                2104,
                iCurrentVersion
            );
        }
    }
    return bvalid;
}

// Which sides of the decal's texture space a vertex lies past.
enum DECAL_OUTSIDE_FLAGS : int32_t {
    DECAL_OUTSIDE_X_MIN = 0x1,
    DECAL_OUTSIDE_X_MAX = 0x2,
    DECAL_OUTSIDE_Y_MIN = 0x4,
    DECAL_OUTSIDE_Y_MAX = 0x8,
    DECAL_ABOVE_MIN_DEPTH = 0x10,
};

bool BSTempEffectGeometryDecal::ShouldAddTriangle(
    uint32_t aiIndex,
    int32_t aiBaseVertex,
    bool abStrip,
    uint16_t* apIB,
    NiPoint3* apTexCoords,
    uint16_t* apusVertexMap
) {
    int32_t iOutsideFlags = 0;
    for (int32_t i = 0; i < 3; ++i) {
        int32_t iInsideCount = 0;
        uint16_t usIndex = abStrip ? apIB[i + aiIndex] : apIB[3 * aiIndex + i];
        uint16_t usVertex = static_cast<uint16_t>(aiBaseVertex + usIndex);
        if (apusVertexMap) {
            usVertex = apusVertexMap[usVertex];
        }
        if (!(apTexCoords[usVertex].x < 0.0)) {
            if (!(apTexCoords[usVertex].x > 1.0)) {
                iInsideCount = 1;
            }
            else {
                iOutsideFlags |= DECAL_OUTSIDE_X_MAX;
            }
        }
        else {
            iOutsideFlags |= DECAL_OUTSIDE_X_MIN;
        }
        if (!(apTexCoords[usVertex].y < 0.0)) {
            if (!(apTexCoords[usVertex].y > 1.0)) {
                ++iInsideCount;
            }
            else {
                iOutsideFlags |= DECAL_OUTSIDE_Y_MAX;
            }
        }
        else {
            iOutsideFlags |= DECAL_OUTSIDE_Y_MIN;
        }
        if (apTexCoords[usVertex].z > 0.10000000149011612) {
            iOutsideFlags |= DECAL_ABOVE_MIN_DEPTH;
            ++iInsideCount;
        }
        if (iInsideCount == 3) {
            return true;
        }
    }
    if (((iOutsideFlags & (DECAL_OUTSIDE_X_MIN | DECAL_OUTSIDE_X_MAX)) == (DECAL_OUTSIDE_X_MIN | DECAL_OUTSIDE_X_MAX) ||
         (iOutsideFlags & (DECAL_OUTSIDE_Y_MIN | DECAL_OUTSIDE_Y_MAX)) ==
             (DECAL_OUTSIDE_Y_MIN | DECAL_OUTSIDE_Y_MAX)) &&
        (iOutsideFlags & DECAL_ABOVE_MIN_DEPTH) != 0) {
        return true;
    }
    return false;
}

// Projects the decal onto a dismemberable skinned target whose vertices come through the model data's packed streams:
// the vertices are skinned (Deform is empty in this build), moved into the decal's texture space, and every triangle of
// an enabled body part's partition that the decal touches is copied into the decal geometry, with a new skin partition
// per body part and new bone weights. When no partition is enabled it deliberately returns without unlocking the packed
// vertex data, and when no vertex is kept it leaks the new partitions. The new[] allocations of
// 16-bit counts keep a size overflow check only where a count can overflow it.
void BSTempEffectGeometryDecal::InitializeUsingAGD() {
    NiSkinInstance* pSkinInstance = spTargetGeom->GetSkinInstance();
    if (!NiObject::IsExactKindOf(&BSDismemberSkinInstance::ms_RTTI, pSkinInstance)) {
        return;
    }
    BSDismemberSkinInstance* pDismemberSkinInstance = static_cast<BSDismemberSkinInstance*>(pSkinInstance);
    int32_t iPartitionCount = static_cast<int32_t>(pSkinInstance->GetSkinPartition()->GetPartitionCount());

    // The projection: the decal's rotation about Z, then the rotation of Z onto the decal's direction.
    NiMatrix3 kProjection;
    NiTriBasedGeom* pTargetGeom = spTargetGeom;
    const NiTransform* pkWorldTransform;
    pkWorldTransform = &pTargetGeom->GetWorldTransform();
    NiMatrix3 kWorldRotate = pkWorldTransform->m_Rotate;
    NiMatrix3 kInverseRotate;
    kWorldRotate.Inverse(&kInverseRotate);
    NiPoint3 kLocalVector;
    kLocalVector = kInverseRotate * pVector;
    NiMatrix3 kZRotation;
    kZRotation.MakeZRotation(fRotation);
    kProjection = kZRotation * BSUtilities::GetRotationZToVector(pVector);
    pTargetGeom = spTargetGeom;
    pkWorldTransform = &pTargetGeom->GetWorldTransform();
    NiTransform kWorld = *pkWorldTransform;
    kWorld.m_Translate -= pOrigin;

    NiGeometryDataIteratorNiPoint3 kVertexIter;
    NiGeometryDataIteratorNiPoint3 kNormalIter;
    NiGeometryDataIteratorNiPoint2 kTextureIter;
    NiGeometryDataIteratorNiBlendWeights kWeightIter;
    NiGeometryDataIteratorNiBlendIndices kIndexIter;
    NiAdditionalGeometryData* pAdditionalData = spTargetGeom->GetModelData()->GetAdditionalGeometryData();
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
    bool bLocked = spTargetGeom->GetModelData()->LockPackedVertexData(false);
    spTargetGeom->GetModelData()->GetVerticesIterator(kVertexIter);
    spTargetGeom->GetModelData()->GetNormalsIterator(kNormalIter);
    spTargetGeom->GetModelData()->GetTextureSetIterator(0, kTextureIter);
    spTargetGeom->GetModelData()->GetBlendWeightsIterator(kWeightIter);
    spTargetGeom->GetModelData()->GetBlendIndicesIterator(kIndexIter);

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
        if (usBodyPart > BSDismemberSkinInstance::TORSO_SECTION) {
            usBodyPart = static_cast<uint16_t>(usBodyPart / BSDismemberSkinInstance::TORSO_SECTION);
        }
        else if (usBodyPart > BSDismemberSkinInstance::SECTION_CAP) {
            while (usBodyPart > BSDismemberSkinInstance::SECTION_CAP) {
                usBodyPart = static_cast<uint16_t>(usBodyPart - BSDismemberSkinInstance::SECTION_CAP);
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

    int32_t iIndexCount = usTriangleCount * 3;
    uint16_t* pNewIndex = new uint16_t[static_cast<uint32_t>(iIndexCount)];
    int32_t* piVertexMap = new int32_t[uiVertexCount];
    uint16_t usNewVertexCount = 0;
    uint16_t* pPartIndex = new uint16_t[static_cast<uint32_t>(iIndexCount)];
    uint16_t* pPartVertexMap = new uint16_t[uiVertexCount];
    int32_t* piPartVertexMap = new int32_t[uiVertexCount];
    uint32_t uiMaxPartitions = static_cast<uint32_t>(iPartitions);
    uint32_t uiCount = static_cast<uint32_t>(iPartitions);
    uint16_t* pBodyPartIndex = new uint16_t[uiCount];
    float* pfPartWeights = new float[uiVertexCount << 2];
    uint8_t* pucPartBones = new uint8_t[uiVertexCount << 2];
    ourMemset(piVertexMap, -1, uiVertexCount << 2);

    // A real new[]: clang gives the class a vector deleting destructor only where its new[] is seen, and
    // ~NiSkinPartition's delete[] of the partitions goes through it.
    NiSkinPartition::Partition* pNewPartitions = new NiSkinPartition::Partition[static_cast<uint32_t>(iPartitions)];
    NiPoint3* pTexCoords = new NiPoint3[uiVertexCount];
    NiBlendWeights* pWeights = new NiBlendWeights[uiVertexCount];
    NiPoint3* pSkinVertices = new NiPoint3[uiVertexCount];
    NiPoint3* pSkinNormals = new NiPoint3[uiVertexCount];
    pSkinInstance->Deform(
        &kVertexIter,
        &kWeightIter,
        &kIndexIter,
        &kNormalIter,
        uiVertexCount,
        pSkinVertices,
        pSkinNormals,
        sizeof(NiPoint3)
    );

    // A copy of the skin's bones, each with room for the vertex weights the kept vertices bring (the first bone for
    // four per vertex).
    pSkinData = pSkinInstance->GetSkinData();
    pSkinBoneData = pSkinData->GetBoneData();
    iBones = static_cast<int32_t>(pSkinData->GetBoneCount());
    uiCount = static_cast<uint32_t>(iBones);
    pBoneData = new NiSkinData::BoneData[uiCount];
    for (int32_t j = 0; j < iBones; ++j) {
        NiSkinData::BoneData* pBone = &pBoneData[j];
        pBone->m_kSkinToBone = pSkinBoneData[j].m_kSkinToBone;
        NiSkinData::BoneVertData* pBoneVertData;
        if (j == 0) {
            uiCount = uiVertexCount << 2;
            pBoneVertData = new NiSkinData::BoneVertData[uiCount];
        }
        else {
            pBoneVertData = new NiSkinData::BoneVertData[uiVertexCount];
        }
        pBone->m_pkBoneVertData = pBoneVertData;
    }

    // Each vertex's texture coordinates: x and y in the decal's plane scaled to [0, 1], z the weight of the decal
    // there, falling off with the angle between the normal and the decal's direction and with the depth.
    NiPoint3 kDirection = kLocalVector;
    NiPoint3 kSkinVertex;
    NiPoint3 kSkinNormal;
    for (int32_t k = 0; k < iVertexCount; ++k) {
        NiPoint3 kProjected = NiPoint3::ZERO;
        kSkinVertex = pSkinVertices[k];
        kSkinNormal = pSkinNormals[k];
        kWeightIter.Get(k, pWeights[k]);
        // Sums the three weights in double precision and stores the sum as a float.
        float fSum = static_cast<float>(
            static_cast<double>(pWeights[k].fWeights[0]) + pWeights[k].fWeights[1] + pWeights[k].fWeights[2]
        );
        pWeights[k].fWeights[3] = static_cast<float>(1.0 - fSum);
        if (pWeights[k].fWeights[3] < 0.0001) {
            pWeights[k].fWeights[3] = 0.0f;
        }
        kSkinVertex *= kWorld.m_fScale;
        NiPoint3 kWorldVertex = NiPoint3::ZERO;
        NiMatrix3::TransformVertices(kWorld.m_Rotate, kWorld.m_Translate, 1, &kSkinVertex, &kWorldVertex);
        NiMatrix3::TransformVertices(kProjection, NiPoint3::ZERO, 1, &kWorldVertex, &kProjected);
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
        if (usBodyPart > BSDismemberSkinInstance::TORSO_SECTION) {
            usBodyPart = static_cast<uint16_t>(usBodyPart / BSDismemberSkinInstance::TORSO_SECTION);
        }
        else if (usBodyPart > BSDismemberSkinInstance::SECTION_CAP) {
            while (usBodyPart > BSDismemberSkinInstance::SECTION_CAP) {
                usBodyPart = static_cast<uint16_t>(usBodyPart - BSDismemberSkinInstance::SECTION_CAP);
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
                }
                else {
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
                                    pBone->m_pkBoneVertData[pBone->m_usVerts].m_fWeight = pWeights[usVertex]
                                                                                              .fWeights[w];
                                    pBone->m_usVerts = static_cast<uint16_t>(pBone->m_usVerts + 1);
                                }
                            }
                            usNewVertexCount = static_cast<uint16_t>(usNewVertexCount + 1);
                        }
                        else {
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
                        }
                        else {
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
                rPartition.m_pfWeights = static_cast<float*>(
                    _NiMalloc((static_cast<uint32_t>(iWeightCount)) * sizeof(float))
                );
                rPartition.m_pucBonePalette = static_cast<uint8_t*>(_NiMalloc(static_cast<uint32_t>(iWeightCount)));
                ourMemcpy(
                    rPartition.m_pfWeights,
                    pfPartWeights,
                    (rPartition.m_usBonesPerVertex << 2) * rPartition.m_usVertices
                );
                ourMemcpy(
                    rPartition.m_pucBonePalette,
                    pucPartBones,
                    rPartition.m_usBonesPerVertex * rPartition.m_usVertices
                );
                if (static_cast<uint32_t>(iPartitions) >= uiMaxPartitions) {
                    platform::DebuggerBreak();
                }
                pBodyPartIndex[iPartitions] = usBodyPart;
                ++iPartitions;
            }
        }
        iBaseVertex += pPartitions[m].m_usVertices;
    }
    delete[] pWeights;

    NiSkinPartition* pNewSkinPartition = nullptr;
    if (iPartitions > 0) {
        pNewSkinPartition = new NiSkinPartition;
        pNewSkinPartition->ReplacePartitions(pNewPartitions, static_cast<uint32_t>(iPartitions));
    }
    if (usNewVertexCount > 0) {
        uint16_t* pGeomIndex = static_cast<uint16_t*>(ourMalloc(usNewIndexCount * sizeof(uint16_t)));
        ourMemcpy(pGeomIndex, pNewIndex, usNewIndexCount * sizeof(uint16_t));
        NiPoint3* pGeomVertex = new NiPoint3[usNewVertexCount];
        NiPoint3* pGeomTexCoord = new NiPoint3[usNewVertexCount];
        for (int32_t q = 0; q < iVertexCount; ++q) {
            NiPoint3 kVertex;
            NiPoint3 kNormal;
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
            pBoneData[b].m_pkBoneVertData = new NiSkinData::BoneVertData[uiVerts];
            ourMemcpy(pBoneData[b].m_pkBoneVertData, pOldVertData, pBone->m_usVerts * sizeof(NiSkinData::BoneVertData));
            delete[] pOldVertData;
        }
        if (pNewSkinPartition) {
            CreateGeometry(
                spTargetGeom,
                usNewVertexCount,
                usNewIndexCount,
                pGeomVertex,
                pGeomTexCoord,
                pGeomIndex,
                pBoneData,
                pNewSkinPartition,
                pBodyPartIndex
            );
        }
        else {
            CreateGeometry(
                spTargetGeom,
                usNewVertexCount,
                usNewIndexCount,
                pGeomVertex,
                pGeomTexCoord,
                pGeomIndex,
                pBoneData,
                nullptr,
                nullptr
            );
        }
        iNumVerts = usNewVertexCount;
    }
    else {
        for (int32_t b = 0; b < iBones; ++b) {
            delete[] pBoneData[b].m_pkBoneVertData;
            pBoneData[b].m_pkBoneVertData = nullptr;
        }
        delete[] pBoneData;
    }
    delete[] pSkinVertices;
    delete[] pSkinNormals;
    delete[] pNewIndex;
    delete[] piVertexMap;
    delete[] pTexCoords;
    delete[] pPartIndex;
    delete[] pPartVertexMap;
    delete[] piPartVertexMap;
    delete[] pfPartWeights;
    delete[] pucPartBones;
    delete[] pBodyPartIndex;
    if (bLocked) {
        spTargetGeom->GetModelData()->UnlockPackedVertexData();
    }
}

// Projects the decal onto a dismemberable skinned target from its model data's plain vertex arrays: the vertices are
// skinned (Deform), moved into the decal's texture space, and every triangle of an enabled body part's partition that
// the decal touches is copied into the decal geometry, with a new skin partition per body part and new bone weights.
// When no vertex is kept the new bones are freed but the new partitions are leaked, deliberately.
void BSTempEffectGeometryDecal::InitializeUsingData() {
    if (!iBodyParts) {
        return;
    }
    NiSkinInstance* pSkinInstance = spTargetGeom->GetSkinInstance();
    BSDismemberSkinInstance* pDismemberSkinInstance = nullptr;
    if (!NiObject::IsExactKindOf(&BSDismemberSkinInstance::ms_RTTI, pSkinInstance)) {
        return;
    }
    pDismemberSkinInstance = static_cast<BSDismemberSkinInstance*>(pSkinInstance);
    int32_t iPartitionCount = static_cast<int32_t>(pSkinInstance->GetSkinPartition()->GetPartitionCount());

    // The projection: the decal's rotation about Z, then the rotation of Z onto the decal's direction.
    NiMatrix3 kProjection;
    NiTriBasedGeom* pTargetGeom = spTargetGeom;
    const NiTransform* pkWorldTransform;
    pkWorldTransform = &pTargetGeom->GetWorldTransform();
    NiMatrix3 kWorldRotate = pkWorldTransform->m_Rotate;
    NiMatrix3 kInverseRotate;
    kWorldRotate.Inverse(&kInverseRotate);
    NiPoint3 kLocalVector;
    kLocalVector = kInverseRotate * pVector;
    NiMatrix3 kZRotation;
    kZRotation.MakeZRotation(fRotation);
    kProjection = kZRotation * BSUtilities::GetRotationZToVector(pVector);
    pTargetGeom = spTargetGeom;
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
    int32_t iVertexCount = spTargetGeom->GetModelData()->GetVertexCount();
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
        if (usBodyPart > BSDismemberSkinInstance::TORSO_SECTION) {
            usBodyPart = static_cast<uint16_t>(usBodyPart / BSDismemberSkinInstance::TORSO_SECTION);
        }
        else if (usBodyPart > BSDismemberSkinInstance::SECTION_CAP) {
            while (usBodyPart > BSDismemberSkinInstance::SECTION_CAP) {
                usBodyPart = static_cast<uint16_t>(usBodyPart - BSDismemberSkinInstance::SECTION_CAP);
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

    uint32_t uiIndexCount = static_cast<uint32_t>(usTriangles * 3);
    uint16_t* pusNewIndex = new uint16_t[uiIndexCount];
    int32_t* piVertexMap = new int32_t[uiVertexCount];
    uint16_t* pusPartIndex = new uint16_t[uiIndexCount];
    uint16_t* pusPartVertexMap = new uint16_t[uiVertexCount];
    int32_t* piPartVertexMap = new int32_t[uiVertexCount];
    uint32_t uiBodyPartCount = static_cast<uint32_t>(iPartitionCount);
    uint16_t* pusBodyParts = new uint16_t[uiBodyPartCount];
    uint32_t uiInteractions = uiVertexCount << 2;
    float* pfPartWeights = new float[uiInteractions];
    uint8_t* pucPartBonePalette = new uint8_t[uiVertexCount << 2];
    ourMemset(piVertexMap, -1, uiVertexCount << 2);

    // A real new[], as in the first decal's partitions above.
    NiSkinPartition::Partition* pNewPartitions = new NiSkinPartition::Partition[static_cast<uint32_t>(iPartitions)];

    NiPoint3* pTexCoords = new NiPoint3[uiVertexCount];
    NiPoint3* pVertices = new NiPoint3[uiVertexCount];
    NiPoint3* pNormals = new NiPoint3[uiVertexCount];
    pSkinInstance->Deform(
        spTargetGeom->GetModelData(),
        pVertices,
        pNormals,
        static_cast<NiPoint3*>(nullptr),
        static_cast<NiPoint3*>(nullptr),
        sizeof(NiPoint3)
    );

    // The new bones: the skin-to-bone transforms copied, room for four weights per vertex on bone 0 and one per
    // vertex on the others.
    pSkinData = pSkinInstance->GetSkinData();
    pSkinBoneData = pSkinData->GetBoneData();
    iBones = static_cast<int32_t>(pSkinData->GetBoneCount());
    pBoneData = new NiSkinData::BoneData[static_cast<uint32_t>(iBones)];
    for (int32_t i = 0; i < iBones; ++i) {
        NiSkinData::BoneData* pBone = &pBoneData[i];
        pBone->m_kSkinToBone = pSkinBoneData[i].m_kSkinToBone;
        uint32_t uiCount = i == 0 ? uiVertexCount << 2 : uiVertexCount;
        pBone->m_pkBoneVertData = new NiSkinData::BoneVertData[uiCount];
    }

    // Each skinned vertex in the decal's texture space: x and y scaled by fScale around 0.5, z the weight that falls
    // off with the normal's angle to the decal direction and with the depth.
    NiPoint3 kDirection = kLocalVector;
    NiPoint3 kVertex;
    NiPoint3 kNormal;
    for (int32_t i = 0; i < iVertexCount; ++i) {
        NiPoint3 kTexCoord = NiPoint3::ZERO;
        kVertex = pVertices[i];
        kNormal = pNormals[i];
        NiPoint3 kWorldVertex = NiPoint3::ZERO;
        kVertex *= kWorld.m_fScale;
        NiMatrix3::TransformVertices(kWorld.m_Rotate, kWorld.m_Translate, 1, &kVertex, &kWorldVertex);
        NiMatrix3::TransformVertices(kProjection, NiPoint3::ZERO, 1, &kWorldVertex, &kTexCoord);
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
        if (usBodyPart > BSDismemberSkinInstance::TORSO_SECTION) {
            usBodyPart = static_cast<uint16_t>(usBodyPart / BSDismemberSkinInstance::TORSO_SECTION);
        }
        else if (usBodyPart > BSDismemberSkinInstance::SECTION_CAP) {
            while (usBodyPart > BSDismemberSkinInstance::SECTION_CAP) {
                usBodyPart = static_cast<uint16_t>(usBodyPart - BSDismemberSkinInstance::SECTION_CAP);
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
                }
                else {
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
                    }
                    else {
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
                    }
                    else {
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
            }
            else {
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
            pNew[iPartitions].m_pusBones = static_cast<uint16_t*>(
                _NiMalloc(pPartitions[i].m_usBones * sizeof(uint16_t))
            );
            ourMemcpy(
                pNew[iPartitions].m_pusBones,
                pPartitions[i].m_pusBones,
                pPartitions[i].m_usBones * sizeof(uint16_t)
            );
            pNew[iPartitions].m_usBonesPerVertex = pPartitions[i].m_usBonesPerVertex;
            uint32_t uiPartInteractions = pNew[iPartitions].m_usBonesPerVertex * usPartVertices;
            pNew[iPartitions].m_pfWeights = static_cast<float*>(_NiMalloc(uiPartInteractions * sizeof(float)));
            pNew[iPartitions].m_pucBonePalette = static_cast<uint8_t*>(_NiMalloc(uiPartInteractions));
            ourMemcpy(
                pNew[iPartitions].m_pfWeights,
                pfPartWeights,
                (pNew[iPartitions].m_usBonesPerVertex * 4u) * pNew[iPartitions].m_usVertices
            );
            ourMemcpy(
                pNew[iPartitions].m_pucBonePalette,
                pucPartBonePalette,
                static_cast<uint32_t>(pNew[iPartitions].m_usBonesPerVertex * pNew[iPartitions].m_usVertices)
            );
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
        uint16_t* pusGeomIndex = static_cast<uint16_t*>(
            ourMalloc(uiCount <= SIZE_MAX / sizeof(uint16_t) ? sizeof(uint16_t) * uiCount : SIZE_MAX)
        );
        ourMemcpy(pusGeomIndex, pusNewIndex, usNewIndices * sizeof(uint16_t));
        NiPoint3* pModelVertices = spTargetGeom->GetModelData()->GetVertices();
        NiPoint3* pGeomVertex = new NiPoint3[usNewVertices];
        NiPoint3* pGeomTexCoord = new NiPoint3[usNewVertices];
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
                    pNewBone->m_pkBoneVertData[pNewBone->m_usVerts].m_usVert = static_cast<uint16_t>(
                        piVertexMap[usVert]
                    );
                    pNewBone->m_pkBoneVertData[pNewBone->m_usVerts].m_fWeight = pOldBone->m_pkBoneVertData[j].m_fWeight;
                    ++pNewBone->m_usVerts;
                }
            }
            NiSkinData::BoneVertData* pOldVertData = pNewBone->m_pkBoneVertData;
            uint32_t uiVerts = pNewBone->m_usVerts;
            pBoneData[i].m_pkBoneVertData = new NiSkinData::BoneVertData[uiVerts];
            ourMemcpy(
                pBoneData[i].m_pkBoneVertData,
                pOldVertData,
                pNewBone->m_usVerts * sizeof(NiSkinData::BoneVertData)
            );
            delete[] pOldVertData;
        }
        if (pSkinPartition) {
            CreateGeometry(
                spTargetGeom,
                usNewVertices,
                usNewIndices,
                pGeomVertex,
                pGeomTexCoord,
                pusGeomIndex,
                pBoneData,
                pSkinPartition,
                pusBodyParts
            );
        }
        else {
            CreateGeometry(
                spTargetGeom,
                usNewVertices,
                usNewIndices,
                pGeomVertex,
                pGeomTexCoord,
                pusGeomIndex,
                pBoneData,
                nullptr,
                nullptr
            );
        }
        iNumVerts = usNewVertices;
    }
    else {
        for (int32_t i = 0; i < iBones; ++i) {
            delete[] pBoneData[i].m_pkBoneVertData;
            pBoneData[i].m_pkBoneVertData = nullptr;
        }
        delete[] pBoneData;
    }
    delete[] pVertices;
    delete[] pNormals;
    delete[] pusNewIndex;
    delete[] pusPartIndex;
    delete[] pusPartVertexMap;
    delete[] piVertexMap;
    delete[] piPartVertexMap;
    delete[] pfPartWeights;
    delete[] pucPartBonePalette;
    delete[] pTexCoords;
    delete[] pusBodyParts;
}

// Deliberately keeps an uninitialised read: with a partition and no body part index array, the
// partition data's body parts are filled from ibodyPartsA, which only the no-partition branch writes, for up to the
// partition count's entries (past its 15). Both callers pass the two together.
void BSTempEffectGeometryDecal::CreateGeometry(
    NiTriBasedGeom* apGeometry,
    uint16_t aiNewVertex,
    uint16_t aiNewIndex,
    NiPoint3* apGeomVertex,
    NiPoint3* apGeomTexCoord,
    uint16_t* apGeomIndex,
    NiSkinData::BoneData* apGeomBoneData,
    NiSkinPartition* apPartition,
    uint16_t* apBodyPartIndex
) {
    BSDismemberSkinInstance* pnewSkinInstance;
    NiSkinData* pnewSkinData;
    GeometryDecalShaderProperty* pshaderprop;
    NiTriShape* pnewDecal;
    uint32_t ibodyPartsA[15];
    pnewDecal = new NiTriShape(
        aiNewVertex,
        apGeomVertex,
        apGeomTexCoord,
        nullptr,
        nullptr,
        0,
        NiGeometryData::NBT_METHOD_NONE,
        aiNewIndex / 3,
        apGeomIndex
    );
    pnewDecal->SetName(*FixedStrings::GetDecal());
    pshaderprop = new GeometryDecalShaderProperty;
    pnewDecal->AttachProperty(pshaderprop);
    pshaderprop->SetDepthBias(1.0f);
    NiPointer<NiTexture> sptexture(static_cast<NiTexture*>(nullptr));
    pTextureSet->GetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, sptexture);
    NiTexture* ptexture = sptexture.m_pObject;
    if (NiObject::IsKindOf_0(&NiSourceTexture::ms_RTTI, ptexture)) {
        pshaderprop->spDecalTexture = static_cast<NiSourceTexture*>(ptexture);
    }
    NiSkinInstance* pskin = apGeometry->GetSkinInstance();
    int32_t iboneCount = pskin->GetSkinData()->m_uiBones;
    pnewSkinData = new NiSkinData(iboneCount, apGeomBoneData, pskin->GetSkinData()->m_kRootParentToSkin, apGeomVertex);
    NiAVObject** ppbones = pskin->m_ppkBones;
    // Checked array allocation: the count is taken unsigned.
    NiAVObject** ppnewBones = static_cast<NiAVObject**>(ourMalloc(
        static_cast<uint32_t>(iboneCount) <= SIZE_MAX / sizeof(NiAVObject*)
            ? sizeof(NiAVObject*) * static_cast<uint32_t>(iboneCount)
            : SIZE_MAX
    ));
    for (int32_t bone = 0; bone < iboneCount; ++bone) {
        ppnewBones[bone] = ppbones[bone];
    }
    int32_t icount = 0;
    if (apPartition) {
        icount = apPartition->m_uiPartitions;
    }
    else {
        uint32_t J = 2;
        for (uint32_t I = 0; I < 15; ++I) {
            if ((J & iBodyParts) != 0) {
                ibodyPartsA[icount++] = I;
            }
            J *= 2;
        }
    }
    BSDismemberSkinInstance::PartitionData*
        ppartitionExtraDataArray = static_cast<BSDismemberSkinInstance::PartitionData*>(ourMalloc(
            static_cast<uint32_t>(icount) <= SIZE_MAX / sizeof(BSDismemberSkinInstance::PartitionData)
                ? sizeof(BSDismemberSkinInstance::PartitionData) * static_cast<uint32_t>(icount)
                : SIZE_MAX
        ));
    for (int32_t i = 0; i < icount; ++i) {
        ppartitionExtraDataArray[i].bEnabled = true;
        ppartitionExtraDataArray[i].bStateChange = true;
        uint16_t usBodyPartNum = apBodyPartIndex ? apBodyPartIndex[i] : ibodyPartsA[i];
        ppartitionExtraDataArray[i].usBodyPartNum = usBodyPartNum;
    }
    pnewSkinInstance = new BSDismemberSkinInstance(
        pnewSkinData,
        pskin->m_pkRootParent,
        ppnewBones,
        ppartitionExtraDataArray,
        icount
    );
    if (apPartition) {
        pnewSkinInstance->SetSkinPartition(apPartition);
        pnewDecal->SetSkinInstance(pnewSkinInstance);
    }
    else {
        NiSkinPartition* ppartition = new NiSkinPartition;
        if (ppartition->MakePartitions(
                static_cast<NiTriBasedGeomData*>(pnewDecal->GetModelData()),
                pnewSkinData,
                18,
                4,
                1,
                nullptr,
                nullptr,
                nullptr
            )) {
            pnewSkinInstance->SetSkinPartition(ppartition);
            pnewDecal->SetSkinInstance(pnewSkinInstance);
        }
        else {
            if (ppartition) {
                delete ppartition;
            }
            if (pnewSkinInstance) {
                delete pnewSkinInstance;
            }
        }
    }
    pnewDecal->SetTranslate(spTargetGeom->GetTranslate());
    pnewDecal->SetRotate(spTargetGeom->m_kLocal.m_Rotate);
    pnewDecal->SetScale(spTargetGeom->m_kLocal.m_fScale);
    pnewDecal->SetWorldTranslate(spTargetGeom->GetWorldTranslate());
    pnewDecal->SetWorldRotate(spTargetGeom->GetWorldTransform().m_Rotate);
    pnewDecal->SetWorldScale(spTargetGeom->m_kWorld.m_fScale);
    pnewDecal->UpdateWorldBound();
    pnewDecal->UpdateProperties();
    BSShaderManager::PrepareObject(pnewDecal, 0, 0);
    spDecalGeometry = static_cast<NiTriBasedGeom*>(pnewDecal);
    pGeometry = spTargetGeom.m_pObject;
}
