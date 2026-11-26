#include "Fallout Shared/TESObjectTREE.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
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
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

// Byte swaps every field, in field order.
void OBJ_TREE::Endian() {
    BSEndian(fCurveScalar, false);
    BSEndian(fMinimumLeafAngle, false);
    BSEndian(fMaximumLeafAngle, false);
    BSEndian(fBranchDimming, false);
    BSEndian(fLeafDimming, false);
    BSEndian(iCanopyShadowRadius, false);
    BSEndian(fRockSpeed, false);
    BSEndian(fRustleSpeed, false);
}

TREE_BILLBOARD_DATA::TREE_BILLBOARD_DATA(
    TESObjectTREE* apTree,
    uint32_t aiCellChunk,
    uint32_t aiCellKey,
    NiNode* apInstancedNode,
    uint32_t aiArraySize,
    NiPoint3* apLocArray,
    float* apColorArray
)
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
    if (static_cast<uint8_t>(apFile->GetTESForm()) != TREE_ID) {
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
            case MODT_ID: TESModel::LoadModelChunk(static_cast<TESModelTree*>(this), apFile); break;
            case ICON_ID: TESTexture::LoadTextureChunk(static_cast<TESIconTree*>(this), apFile); break;
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
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize));
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
                if (uiChunkSize % 4 != 0) {
                    break;
                }
                uint32_t uiCount = uiChunkSize >> 2;
                uint32_t* pSeeds = static_cast<uint32_t*>(ourMalloc(sizeof(uint32_t) * uiCount));
                ourMemset(pSeeds, 0, uiChunkSize);
                apFile->GetChunkData(pSeeds, uiChunkSize);
                SeedArray.RemoveAll();
                for (uint32_t i = 0; i < uiCount; ++i) {
                    if (apFile->QEndian()) {
                        BSEndian(pSeeds[i], false);
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
                        BSEndian(BillboardSize.x, false);
                        BSEndian(BillboardSize.y, false);
                    }
                }
                break;
            }
            default: break;
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
        uint32_t* pdata = static_cast<uint32_t*>(
            ourMalloc(inumSeeds <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * inumSeeds : SIZE_MAX)
        );
        ourMemset(pdata, 0, sizeof(uint32_t) * inumSeeds);
        for (uint32_t i = 0; i < inumSeeds; ++i) {
            uint32_t ival = SeedArray[i];
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
    // The billboard's width and height.
    TESForm::AddChunkArray(BNAM_ID, &BillboardSize.x, 2u);
    CloseForm();
}

void TESObjectTREE::InitItem() {
    if (!TESForm::GetInitialized()) {
        static_cast<BGSDestructibleObjectForm*>(this)->InitItem(this);
        TESForm::SetInitialized(true);
    }
}

void TESObjectTREE::Copy(TESForm* apCopy) {
    TESObjectTREE* ptree = dynamic_cast<TESObjectTREE*>(apCopy);
    if (ptree) {
        CopyAllComponents(apCopy);
        uint32_t isize = ptree->SeedArray.m_usSize;
        SeedArray.RemoveAll();
        for (uint32_t i = 0; i < isize; ++i) {
            SeedArray.AddFirstEmpty(ptree->SeedArray[i]);
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
        return true;
    }
    OBJ_TREE* pData = pcomp->GetData();
    if (std::memcmp(&Data, pData, sizeof(Data))) {
        return true;
    }
    if (BillboardSize != pcomp->BillboardSize) {
        return true;
    }
    uint32_t isize = SeedArray.m_usSize;
    if (isize != pcomp->SeedArray.m_usSize) {
        return true;
    }
    for (uint32_t i = 0; i < isize; ++i) {
        if (SeedArray[i] != pcomp->SeedArray[i]) {
            return true;
        }
    }
    return CompareAllComponents(pcomp);
}

uint8_t TESObjectTREE::GetIndexForSeed(uint32_t aiSeed) {
    uint32_t i;
    for (i = 0; i < SeedArray.NiTArray<uint32_t, NiTMallocInterface<uint32_t>>::GetSize(); ++i) {
        if (SeedArray[i] == aiSeed) {
            return static_cast<uint8_t>(i);
        }
    }
    return 255;
}

uint32_t TESObjectTREE::GetSeedAtIndex(uint8_t aiIndex) {
    if (aiIndex == 255 || !SeedArray.NiTArray<uint32_t, NiTMallocInterface<uint32_t>>::GetSize()) {
        return GetRandomSeed();
    }
    if (aiIndex >= SeedArray.NiTArray<uint32_t, NiTMallocInterface<uint32_t>>::GetSize()) {
        aiIndex %= SeedArray.NiTArray<uint32_t, NiTMallocInterface<uint32_t>>::GetSize();
    }
    return SeedArray[aiIndex];
}

uint32_t TESObjectTREE::GetRandomSeed() {
    uint32_t size = SeedArray.NiTArray<uint32_t, NiTMallocInterface<uint32_t>>::GetSize();
    if (!size) {
        return 0;
    }
    uint32_t uiIndex = Random() % size;
    return SeedArray[uiIndex];
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
    Data.iCanopyShadowRadius = static_cast<int32_t>(aiRadius);
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

void TESObjectTREE::CreateDistant3D(
    NiPoint3* apLocArray,
    NiPoint3* apRotArray,
    float* apColorArray,
    uint32_t aiArraySize,
    uint32_t aiCellChunk,
    uint32_t aiCellKey,
    NiNode* apInstancedNode,
    NiNode* apFullNode
) {
    AutoMemContext apMemory(
        MEM_CONTEXT_LODLANDSCAPE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectTREE.cpp",
        552
    );
    if (!apInstancedNode ||
        !apLocArray ||
        !apColorArray ||
        !aiArraySize ||
        !BSTreeManager::CanTreesLoad() ||
        !IsLargeEnoughForDistantLOD()) {
        return;
    }
    NiBillboardNode* pBillboardNode = nullptr;
    uint32_t uiFormID = iFormID;
    if (!DistantLODShaderProperty::HasCachedGeometry(uiFormID)) {
        char cFormattedPath[268];
        char cTexturePath[264];
        GetBillboardTextureFilename(cTexturePath);
        StandardizeTexturePath(cTexturePath, cFormattedPath, 260u);
        NiPointer<NiTexture> pkSelf(nullptr);
        BSTexturePalette::GetTexture(cFormattedPath, pkSelf);
        if (!pkSelf.m_pObject) {
            TREE_BILLBOARD_DATA* pBillboardData;
            pBillboardData = new TREE_BILLBOARD_DATA(
                this,
                aiCellChunk,
                aiCellKey,
                apInstancedNode,
                aiArraySize,
                apLocArray,
                apColorArray
            );
            pModelLoader->QueueTreeBillboard(cFormattedPath, pBillboardData);
            return;
        }
        pBillboardNode = static_cast<NiBillboardNode*>(BuildDistant3D(true));
        if (!pBillboardNode) {
            return;
        }
    }
    DistantLODShaderProperty::DistantLODParam apParam;
    apParam.iBaseObjectID = iFormID;
    apParam.p3DData = pBillboardNode;
    apParam.pcGeomFilename = nullptr;
    DistantLODShaderProperty::AddDistantLOD(
        aiCellChunk,
        aiCellKey,
        apInstancedNode,
        &apParam,
        apLocArray,
        apColorArray,
        aiArraySize
    );
    if (apParam.p3DData) {
        delete apParam.p3DData;
    }
}

bool TESObjectTREE::IsLargeEnoughForDistantLOD() {
    return (!(BillboardSize.x > 0.0) || !(BillboardSize.x <= 200.0)) &&
        (!(BillboardSize.y > 0.0) || !(BillboardSize.y <= 350.0));
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

void TESObjectTREE::CreateBillboardGeomData(bool abZUp, NiPointer<NiTriShapeData>& aspTriShapeData) {
    NiPoint3* pVertex;
    NiColorA* pcol;
    NiPoint2* ptext;
    NiPoint3* pverts;
    NiPoint2 kTexCoord3;
    NiPoint2 kTexCoord2;
    NiPoint2 kTexCoord1;
    NiPoint2 kTexCoord0;
    NiPoint3 kTopRightYUp;
    NiPoint3 kTopLeftYUp;
    NiPoint3 kTopRightZUp;
    NiPoint3 kTopLeftZUp;
    NiPoint3 kBottomRight;
    NiPoint3 kBottomLeft;
    float STBBNBT[36];
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODLANDSCAPE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectTREE.cpp",
        409
    );
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
    pverts = new NiPoint3[4];
    float fSizeY = BillboardSize.y;
    float fHeight = fSizeY;
    if (fSizeY <= 0.0) {
        fHeight = 1400.0f;
    }
    float fHalfWidth = fHeight / 2.0;
    float fLeft = -fHalfWidth;
    pVertex = ::new (&kBottomLeft) NiPoint3(-fHalfWidth, 0.0f, 0.0f);
    pverts[0] = *pVertex;
    pVertex = ::new (&kBottomRight) NiPoint3(fHalfWidth, 0.0f, 0.0f);
    pverts[3] = *pVertex;
    if (abZUp) {
        pVertex = ::new (&kTopLeftZUp) NiPoint3(fLeft, 0.0f, fHeight);
        pverts[1] = *pVertex;
        pVertex = ::new (&kTopRightZUp) NiPoint3(fHalfWidth, 0.0f, fHeight);
    }
    else {
        pVertex = ::new (&kTopLeftYUp) NiPoint3(fLeft, fHeight, 0.0f);
        pverts[1] = *pVertex;
        pVertex = ::new (&kTopRightYUp) NiPoint3(fHalfWidth, fHeight, 0.0f);
    }
    pverts[2] = *pVertex;
    NiPoint3* pnorm = new NiPoint3[12];
    ourMemcpy(pnorm, static_cast<void*>(STBBNBT), sizeof(NiPoint3) * 12);
    ptext = new NiPoint2[4];
    ::new (&kTexCoord0) NiPoint2(1.0f, 1.0f);
    ::new (&kTexCoord1) NiPoint2(1.0f, 0.0f);
    ::new (&kTexCoord2) NiPoint2(0.0f, 0.0f);
    ::new (&kTexCoord3) NiPoint2(0.0f, 1.0f);
    ptext[0].x = kTexCoord0.x;
    ptext[0].y = kTexCoord0.y;
    ptext[1].x = kTexCoord1.x;
    ptext[1].y = kTexCoord1.y;
    ptext[2].x = kTexCoord2.x;
    ptext[2].y = kTexCoord2.y;
    ptext[3].x = kTexCoord3.x;
    ptext[3].y = kTexCoord3.y;
    pcol = new NiColorA[4];
    NiColorA kColor0(1.0f, 1.0f, 1.0f, 0.0f);
    NiColorA kColor1(1.0f, 1.0f, 1.0f, 0.0f);
    NiColorA kColor2(1.0f, 1.0f, 1.0f, 0.0f);
    NiColorA kColor3(1.0f, 1.0f, 1.0f, 0.0f);
    pcol[0] = kColor0;
    pcol[1] = kColor1;
    pcol[2] = kColor2;
    pcol[3] = kColor3;
    uint16_t* ptrilist = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
    ptrilist[0] = 0;
    ptrilist[1] = 2;
    ptrilist[2] = 1;
    ptrilist[3] = 0;
    ptrilist[4] = 3;
    ptrilist[5] = 2;
    void*
        pShapeData = new NiTriShapeData(4, pverts, pnorm, pcol, ptext, 1, NiGeometryData::NBT_METHOD_NDL, 2, ptrilist);
    NiPointer<NiTriShapeData> spgeomData(static_cast<NiTriShapeData*>(pShapeData));
    aspTriShapeData = spgeomData;
}

NiNode* TESObjectTREE::BuildDistant3D(bool abZUp) {
    BSShaderTextureSet* ptextureSet;
    SpeedTreeBillboardShaderProperty* pbillboardProp;
    NiTriShape* p3D;
    char cTexturePath[268];
    if (!IsLargeEnoughForDistantLOD()) {
        return nullptr;
    }
    AutoMemContext tempMemContext(
        MEM_CONTEXT_LODLANDSCAPE,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESObjectTREE.cpp",
        470
    );
    NiPointer<NiTriShapeData> spgeomData(nullptr);
    CreateBillboardGeomData(abZUp, spgeomData);
    p3D = new NiTriShape(spgeomData.m_pObject);
    NiAlphaProperty* paprop = BSTreeManager::GetDistantBillboardAlphaProperty();
    if (paprop) {
        p3D->AttachProperty(paprop);
    }
    pbillboardProp = new SpeedTreeBillboardShaderProperty;
    p3D->AttachProperty(pbillboardProp);
    GetBillboardTextureFilename(cTexturePath);
    ptextureSet = new BSShaderTextureSet;
    ptextureSet->SetTextureFilename(BSShaderProperty::TEXTYPE_DIFFUSE, cTexturePath);
    pbillboardProp->LoadTextureSet("STBB", ptextureSet);
    pbillboardProp->SetClampNI(NiTexturingProperty::CLAMP_S_CLAMP_T);
    NiPointer<NiTexture> spTempTexture(nullptr);
    ptextureSet->GetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, spTempTexture);
    if (spTempTexture == nullptr) {
        return nullptr;
    }

    NiBillboardNode* pnode = new NiBillboardNode;
    pnode->SetMode(NiBillboardNode::BSROTATE_ABOUT_UP);
    pnode->AttachChild(p3D, 1);
    return pnode;
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
