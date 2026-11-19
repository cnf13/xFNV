#include "BSMenu/TileImage.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSScissorTriShape.h"
#include "BSMain/Setting.h"
#include "BSMenu/Tile.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/TileShaderProperty.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

TileImage::TileImage() : fScale(1.0f), bIsScissor(0) {}

TileImage::~TileImage() {
    spTSP = nullptr;
    if (!IsReleased()) {
        Release();
    }
}

void TileImage::Init(Tile* apParent, const char* apName, Tile* apReplacedTile) {
    Tile::Init(apParent, apName, apReplacedTile);
    Set(kTileValue_Brightness, 255.0f, true);
    Set(kTileValue_Red, 255.0f, true);
    Set(kTileValue_Green, 255.0f, true);
    Set(kTileValue_Blue, 255.0f, true);
    Set(kTileValue_Alpha, 255.0f, true);
    Set(kTileValue_Depth, 1.0f, true);
    Set(kTileValue_Visible, 1.0f, true);
    SetNeedsUpdate(CHANGE_POSITION | CHANGE_CREATE);
    fScale = 1.0f;
    spTexture = nullptr;
    bIsScissor = 0;
}

// A unit quad (two triangles) in a scissor shape when the tile clips, else in a plain shape; the arrays are handed
// to the shape's data. The tile's model is a new node holding the quad, attached to the parent's model.
NiNode* TileImage::MakeNode() {
    NiNode* pParentModel = GetParentModel();
    NiPointer<NiTriShape> spShape;
    NiPoint3* pkVertex = static_cast<NiPoint3*>(MemoryManager_Singleton.Allocate(4 * sizeof(NiPoint3)));
    NiPoint2* pkTexture = static_cast<NiPoint2*>(MemoryManager_Singleton.Allocate(4 * sizeof(NiPoint2)));
    uint16_t* pusTriList = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t)));
    NiPoint3* pkNormal = nullptr;
    pkVertex[0].x = 0.0f;
    pkVertex[0].y = 0.0f;
    pkVertex[0].z = 0.0f;
    pkVertex[1].x = 0.0f;
    pkVertex[1].y = 0.0f;
    pkVertex[1].z = -1.0f;
    pkVertex[2].x = 1.0f;
    pkVertex[2].y = 0.0f;
    pkVertex[2].z = 0.0f;
    pkVertex[3].x = 1.0f;
    pkVertex[3].y = 0.0f;
    pkVertex[3].z = -1.0f;
    pusTriList[0] = 0;
    pusTriList[1] = 1;
    pusTriList[2] = 2;
    pusTriList[3] = 2;
    pusTriList[4] = 1;
    pusTriList[5] = 3;
    pkTexture[0].x = 0.0f;
    pkTexture[0].y = 0.0f;
    pkTexture[1].x = 0.0f;
    pkTexture[1].y = 1.0f;
    pkTexture[2].x = 1.0f;
    pkTexture[2].y = 0.0f;
    pkTexture[3].x = 1.0f;
    pkTexture[3].y = 1.0f;
    if (IsTrue(kTileValue_Clips)) {
        void* pMemory = NiMemObject::operator new(sizeof(BSScissorTriShape));
        BSScissorTriShape* pScissor = nullptr;
        if (pMemory) {
            const int32_t iBottom = static_cast<int32_t>(Interface::GetScreenHeight());
            const int32_t iRight = static_cast<int32_t>(Interface::GetScreenWidth());
            pScissor = ::new (pMemory) BSScissorTriShape(
                4, pkVertex, pkNormal, nullptr, pkTexture, 1, NiGeometryData::NBT_METHOD_NONE, 2, pusTriList, 0, 0,
                iRight, iBottom
            );
        }
        spShape = pScissor;
        bIsScissor = 1;
    }
    else {
        NiTriShape* pShape = new NiTriShape(
            4, pkVertex, pkNormal, nullptr, pkTexture, 1, NiGeometryData::NBT_METHOD_NONE, 2, pusTriList
        );
        spShape = pShape;
        bIsScissor = 0;
    }

    NiGlobalStringTable::GlobalStringHandle kName = NiGlobalStringTable::AddString("Interface: TileImage");
    spShape->SetName(reinterpret_cast<const NiFixedString&>(kName));
    if (kName != NiGlobalStringTable::NULL_STRING) {
        InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kName) - 8));
    }
    spShape->m_spModelData->SetConsistency(NiGeometryData::MUTABLE);

    spTSP = new TileShaderProperty(false);
    spShape->AttachProperty(spTSP);
    // The interface's tile shader.
    BSShader* pShader = BSShaderManager::GetShader(0x20);
    spShape->m_spShader = pShader;

    spModel = new NiNode(0);
    spModel->AttachChild(spShape, true);
    if (!pParentModel) {
        pParentModel = Interface::GetInterfaceRoot();
    }
    pParentModel->AttachChild(spModel, true);
    kName = xName.pString ? NiGlobalStringTable::AddString(xName.pString) : NiGlobalStringTable::NULL_STRING;
    spModel->SetName(reinterpret_cast<const NiFixedString&>(kName));
    if (kName != NiGlobalStringTable::NULL_STRING) {
        InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kName) - 8));
    }
    Interface::AddAlphaProperty(spShape);
    spModel->UpdateProperties();
    Interface::SetNeedToUpdate(true);

    Extra* pExtra = new Extra(this, spModel);
    spModel->AddExtraData(pExtra);
    FinalPostParse(kTileValue_Depth, GetFloat(kTileValue_Depth), nullptr);
    if (IsTrue(kTileValue_Clips)) {
        AddNeedsUpdate(CHANGE_SCISSOR);
    }
    AddNeedsUpdate(CHANGE_POSITION | CHANGE_VISIBILITY | CHANGE_COLOR | CHANGE_GEOMETRY | CHANGE_TEXTURE);
    NiNode* pModel = spModel;
    return pModel;
}

int32_t TileImage::GetType() {
    return kTileValue_Image;
}

Tile* TileImage::PostParse(int32_t aiTrait, float afValue, const char* axString) {
    if ((aiTrait == kTileValue_X || aiTrait == kTileValue_Y) && (this->uiFlags & CHANGE_POSITION) == 0) {
        AddNeedsUpdate(CHANGE_POSITION);
    }
    else if (
        ((aiTrait == kTileValue_Width || aiTrait == kTileValue_Height) && (this->uiFlags & CHANGE_GEOMETRY) == 0) ||
        (aiTrait == kTileValue_Filename && this->GetValueQ(kTileValue_Texatlas))
    ) {
        AddNeedsUpdate(CHANGE_GEOMETRY);
    }
    return nullptr;
}

void TileImage::SetSourceTexture(NiTexture* apTexture) {
    this->Set(kTileValue_Filename, "", true);
    this->spTexture = static_cast<NiSourceTexture*>(apTexture);
    AddNeedsUpdate(0x30);
}

void* TileImage::GetShaderProp() {
    return (&spTSP)->operator->();
}

// The path's interned handle is created and released inline, as a NiFixedString temporary would be; the texture palette
// fills a local smart pointer that is released at the end. GetAspectRatioRatio's result is unused.
void TileImage::ConvertToActualFilename(const char* apFilename, float& afScale, float afZoom, char* axOut) {
    if (!apFilename || !*apFilename || *apFilename == ' ') {
        return;
    }
    Interface::GetAspectRatioRatio();
    afScale = 1.0f;
    strcpy_s(axOut, 0x104, "Data\\Textures\\");
    strcat_s(axOut, 0x104, apFilename);
    NiPointer<NiTexture> spTexture;
    NiGlobalStringTable::GlobalStringHandle kPath =
        axOut ? NiGlobalStringTable::AddString(axOut) : NiGlobalStringTable::NULL_STRING;
    BSTexturePalette::GetTexture(kPath, spTexture);
    const FileFinder::LOOKIN_FLAGS eLookIn =
        static_cast<FileFinder::LOOKIN_FLAGS>(FileFinder::SKIP_CWD | FileFinder::SKIP_PATHS);
    if (spTexture.m_pObject == nullptr &&
        !FileFinder::pFinder->Exist(axOut, axOut, eLookIn, FileFinder::ARCHIVE_TYPE_ANY)) {
        if (afScale == 1.0) {
            strcpy_s(axOut, 0x104, sMissingImage.uValue.str);
        }
        else {
            strcpy_s(axOut, 0x104, "Data\\Textures\\");
            strcat_s(axOut, 0x104, apFilename);
            if (FileFinder::pFinder->Exist(axOut, nullptr, eLookIn, FileFinder::ARCHIVE_TYPE_ANY)) {
                afScale = 1.0f;
            }
            else {
                strcpy_s(axOut, 0x104, sMissingImage.uValue.str);
            }
        }
    }
    if (kPath != NiGlobalStringTable::NULL_STRING) {
        InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kPath) - 8));
    }
}

// Assigning the same path again re-interns it (the old handle is released after), and the texture is created in the
// interface's memory context.
void TileImage::AddToTesTextures(
    const char* apFilename,
    float& afScale,
    NiPointer<NiTexture>& aspTexture,
    float afZoom,
    NiPointer<NiTexture> apDataImage
) {
    if (apDataImage.m_pObject == nullptr) {
        char acPath[0x400];
        acPath[0] = '\0';
        std::memset(acPath + 1, 0, 0x3FF);
        ConvertToActualFilename(apFilename, afScale, afZoom, acPath);
        if (acPath[0]) {
            NiGlobalStringTable::GlobalStringHandle kPath = NiGlobalStringTable::AddString(acPath);
            BSTexturePalette::GetTexture(kPath, aspTexture);
            if (aspTexture.m_pObject == nullptr &&
                FileFinder::pFinder->Exist(acPath, nullptr, FileFinder::SKIP_NONE, FileFinder::ARCHIVE_TYPE_ANY)) {
                if (kPath != acPath) {
                    NiGlobalStringTable::GlobalStringHandle kOld = kPath;
                    kPath = NiGlobalStringTable::AddString(acPath);
                    if (kOld != NiGlobalStringTable::NULL_STRING) {
                        InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kOld) - 8));
                    }
                }
                AutoMemContext kMemContext(MEM_CONTEXT_INTERFACE);
                aspTexture = NiSourceTexture::Create(
                    reinterpret_cast<const NiFixedString&>(kPath), NiSourceTexture::ms_kDefaultPrefs, true, false
                );
                if (aspTexture.m_pObject) {
                    if (!std::strstr(acPath, "oading\\load_")) {
                        BSTexturePalette::SetTexture(aspTexture.m_pObject, nullptr);
                    }
                }
                else {
                    InitializationFlow("*** ERROR: Could not create ptexture '%s'!", acPath);
                }
            }
            if (kPath != NiGlobalStringTable::NULL_STRING) {
                InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kPath) - 8));
            }
        }
    }
    else {
        aspTexture = apDataImage;
    }
}
