#include "Fallout Shared/GridCell.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkWorldM.h"
#include "BSMain/BSCullingProcess.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderNoLightingProperty.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/ShadowLightShader.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GridArray.h"
#include "Fallout Shared/Region System/TESRegionDataLandscape.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESLand.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStripsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"

// Flips every call, so the edge cells alternate between dynamic and large-only updates.
static uint8_t bSwitch;

// The game settings this file defines, with their IDs and default values, in initialization order.
SettingT<INISettingCollection> uGridsToLoad("uGridsToLoad:General", 5u);

GridCellArray::GridCellArray() : WorldCenter(0.0f, 0.0f, 0.0f), bLandAttached(0), spTexRendered(nullptr) {
    FixGridsToLoadINISetting();
    iDimension = uGridsToLoad.Int();
    uint32_t uiCells = iDimension * iDimension;
    // new[]'s size: a byte count past 32 bits asks for 0xFFFFFFFF bytes.
    pGridCellA = static_cast<GridCell*>(ourMalloc(
        uiCells > UINT32_MAX / sizeof(GridCell) ? UINT32_MAX : static_cast<uint32_t>(uiCells * sizeof(GridCell))
    ));
    ourMemset(pGridCellA, 0, static_cast<uint32_t>(sizeof(GridCell) * iDimension * iDimension));
    bRecreateCanopyShadowMaskTexture = 1;
}

GridCellArray::~GridCellArray() {
    KillAll();
    ClearCanopyShadowMaskTexture();
    ourFree(pGridCellA);
}

void GridCellArray::Init() {
    GridArray::Init();
    TESObjectCELL::InitStatics();
}

void GridCellArray::KillAll() {
    GridArray::KillAll();
    if (pDataHandler->GetClearingData()) {
        TESObjectCELL::KillStatics();
    }
}

// GridArray::SetCenter decides whether the center really moved; the world center only follows when it did. The
// two coordinates go through an int-to-float conversion of the grid coordinate shifted up by the cell size.
bool GridCellArray::SetCenter(int32_t aiX, int32_t aiY) {
    bool bMoved = false;
    if (!GridArray::SetCenter(aiX, aiY)) {
        return bMoved;
    }
    WorldCenter.x = static_cast<float>(aiX << 12);
    WorldCenter.y = static_cast<float>(aiY << 12);
    WorldCenter.z = 0.0f;
    TESObjectCELL::MoveExteriorWorld(WorldCenter);
    bMoved = true;
    bLandAttached = false;
    return bMoved;
}

void GridCellArray::Detach(uint32_t aiX, uint32_t aiY) {
    GridCell* pGridCell = Get(aiX, aiY);
    if (pGridCell->pCell != nullptr) {
        pGridCell->pCell->Detach(true);
        ClearItem(aiX, aiY);
    }
}

void GridCellArray::ClearItem(uint32_t aiX, uint32_t aiY) {
    Get(aiX, aiY)->pCell = nullptr;
    bLandAttached = (bool)0;
}

void GridCellArray::MoveItem(uint32_t aiX1, uint32_t aiY1, uint32_t aiX2, uint32_t aiY2) {
    GridCell* pCell = Get(aiX1, aiY1);
    Set(aiX2, aiY2, pCell);
}

void GridCellArray::SwapItem(uint32_t aiX1, uint32_t aiY1, uint32_t aiX2, uint32_t aiY2) {
    GridCell tempgridcell__pgc;
    GridCell* pgc = Get(aiX1, aiY1);
    tempgridcell__pgc.pCell = pgc->pCell;
    GridCell* v5 = Get(aiX2, aiY2);
    Set(aiX1, aiY1, v5);
    Set(aiX2, aiY2, &tempgridcell__pgc);
}

void GridCellArray::UpdateManagedNodeTransforms() {
    static bool bswitch;
    uint32_t CastingType = GetDimension();
    uint32_t v7 = CastingType - 1;
    bswitch = !bswitch;
    for (uint32_t x = 0; x < CastingType; ++x) {
        for (uint32_t y = 0; y < CastingType; ++y) {
            bool v3 = 1;
            if (!x || (!y || (x == v7 || y == v7))) {
                if (((static_cast<uint8_t>(y) ^ static_cast<uint8_t>(x)) & 1) != 0) {
                    v3 = 0;
                }
                if (bswitch) {
                    v3 = !v3;
                }
            }
            if (v3) {
                GridCell* v2 = Get(x, y);
                if (v2->pCell) {
                    v2->pCell->UpdateManagedNodeTransforms();
                }
            }
        }
    }
}

// Reinitialises the distant texture blending of the land of every loaded cell.
void GridCellArray::UpdateDistantLandBlending() {
    uint32_t uiDimension = GetDimension();
    for (uint32_t x = 0; x < uiDimension; ++x) {
        for (uint32_t y = 0; y < uiDimension; ++y) {
            GridCell* pCell = Get(x, y);
            if (pCell && pCell->pCell) {
                pCell->pCell->GetLand()->InitDistantTextureBlending();
            }
        }
    }
}

// Draws or hides the land border of every attached cell. Rereads the dimension on every test.
void GridCellArray::ShowLandBorders(bool abShow) {
    for (uint32_t x = 0; x < GetDimension(); ++x) {
        for (uint32_t y = 0; y < GetDimension(); ++y) {
            GridCell* pCell = Get(x, y);
            if (pCell->pCell && pCell->pCell->GetAttached()) {
                TESObjectLAND* pLand = pCell->pCell->GetLand();
                if (pLand) {
                    pLand->DrawLandBorder(abShow);
                }
            }
        }
    }
}

void GridCellArray::UpdateRefSounds() {
    uint32_t i;
    GridCell* v3;
    uint32_t y;
    uint32_t x = 0;
    for (i = GridArray::GetDimension(); x < i; i = GridArray::GetDimension()) {
        for (y = 0; y < GridArray::GetDimension(); ++y) {
            v3 = Get(x, y);
            if (v3) {
                if (v3->pCell) {
                    v3->pCell->UpdateRefSounds();
                }
            }
        }
        ++x;
    }
}

// Flips the exterior world's debug display and applies the new state to every attached cell. The dimension is reread on
// every loop test, and the debug state is read once.
void GridCellArray::ToggleCollisionGeometry() {
    bhkWorldM* pHavokWorld = TESObjectCELL::ExteriorWorld();
    if (pHavokWorld) {
        bool bShow = !pHavokWorld->QDebugDisplay();
        for (uint32_t x = 0; x < GetDimension(); ++x) {
            for (uint32_t y = 0; y < GetDimension(); ++y) {
                GridCell* pGridCell = Get(x, y);
                if (pGridCell->pCell) {
                    if (pGridCell->pCell->GetAttached()) {
                        pGridCell->pCell->SetCollisionGeometryDisplay(bShow);
                    }
                }
            }
        }
    }
}

GridCell* GridCellArray::Get(uint32_t aiX, uint32_t aiY) {
    uint32_t dimension = GetDimension();
    if (aiX >= dimension || aiY >= dimension) {
        return nullptr;
    }
    return &pGridCellA[aiX * dimension + aiY];
}

// The bounds test uses one dimension read, the index a second one. A null apCell goes through the vtable's
// ClearItem, which also drops the land-attached flag; Set itself never touches that flag.
void GridCellArray::Set(uint32_t aiX, uint32_t aiY, GridCell* apCell) {
    uint32_t uiDimension = GetDimension();
    if (aiX < uiDimension && aiY < uiDimension) {
        if (apCell != nullptr) {
            pGridCellA[aiY + aiX * GetDimension()].pCell = apCell->pCell;
        }
        else {
            GridArray* pArray = this;
            pArray->ClearItem(aiX, aiY);
        }
    }
}

void GridCellArray::ClearCanopyShadowMaskTexture() {
    if (spTexRendered.m_pObject != nullptr) {
        BSShaderManager::SetProjectedShadowMaskTexture(nullptr);
        BSShaderManager::TextureManagerPtr()->ReturnRenderedTexture(spTexRendered.m_pObject);
        spTexRendered = nullptr;
        bRecreateCanopyShadowMaskTexture = true;
    }
}

// The explicit constructor and destructor calls are kept where the game makes them. The mask texture and rect
// GetCanopyShadowMask returns are tested even when it writes neither (an interior cell); the grid holds only exterior
// cells, so that path is not taken.
void GridCellArray::RecreateCanopyShadowMaskTexture() {
    alignas(AutoMemContext) uint8_t aMemContext[sizeof(AutoMemContext)];
    AutoMemContext* pMemContext = ::new (aMemContext) AutoMemContext(
        MEM_CONTEXT_SHADOWS, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\GridCell.cpp", 544
    );
    BSShaderNoLightingProperty* pMaskProp = nullptr;
    alignas(NiPointer<NiNode>) uint8_t aRootStorage[sizeof(NiPointer<NiNode>)];
    NiPointer<NiNode>& spRoot = *::new (aRootStorage) NiPointer<NiNode>(nullptr);
    alignas(NiPointer<NiNode>) uint8_t aCameraRootStorage[sizeof(NiPointer<NiNode>)];
    NiPointer<NiNode>& spCameraRoot = *::new (aCameraRootStorage) NiPointer<NiNode>(nullptr);
    alignas(NiPointer<NiTriStrips>) uint8_t aStripsStorage[sizeof(NiPointer<NiTriStrips>)];
    NiPointer<NiTriStrips>& spStrips = *::new (aStripsStorage) NiPointer<NiTriStrips>(nullptr);
    alignas(NiPointer<NiTriStripsData>) uint8_t aStripsDataStorage[sizeof(NiPointer<NiTriStripsData>)];
    NiPointer<NiTriStripsData>& spStripsData = *::new (aStripsDataStorage) NiPointer<NiTriStripsData>(nullptr);
    alignas(NiPointer<NiCamera>) uint8_t aCameraStorage[sizeof(NiPointer<NiCamera>)];
    NiPointer<NiCamera>& spCamera = *::new (aCameraStorage) NiPointer<NiCamera>(nullptr);
    NiFrustum kFrustum;
    kFrustum.~NiFrustum();
    ::new (&kFrustum) NiFrustum(false);
    alignas(NiPointer<NiDirectionalLight>) uint8_t aLightStorage[sizeof(NiPointer<NiDirectionalLight>)];
    NiPointer<NiDirectionalLight>& spLight = *::new (aLightStorage) NiPointer<NiDirectionalLight>(nullptr);
    NiPoint3 kPos;
    kPos.~NiPoint3();
    ::new (&kPos) NiPoint3;
    alignas(NiColorA) uint8_t aBackgroundStorage[sizeof(NiColorA)];
    NiColorA& kBackgroundColor = *::new (aBackgroundStorage) NiColorA(0.0f, 0.0f, 0.0f, 0.0f);
    const uint32_t uiDimension = GetDimension();
    // Stays set only when no cell leaves its slot empty or gives a partial mask.
    bool bUseFullMask = true;
    // Cleared by any cell with a partial or full mask.
    bool bNoMasks = true;
    bRecreateCanopyShadowMaskTexture = false;
    if (Renderer::QDoCanopyShadowPass()) {
        // The shifted size is converted through a 64-bit integer load (unsigned).
        const float fWidth = static_cast<float>(GetDimension() << 12);
        const float fOffset = static_cast<float>(2048.0 - fWidth / 2.0);
        const float fLLY = WorldCenter.y + fOffset;
        const float fLLX = WorldCenter.x + fOffset;
        NiPoint2 kLL;
        ShadowLightShader::SetProjectedShadowMaskLL(*::new (&kLL) NiPoint2(fLLX, fLLY));
        ShadowLightShader::SetProjectedShadowMaskWidth(fWidth);
        // 0x20 is the canopy shadow pass's bit.
        BSShaderManager::SetEnabledPasses(BSShaderManager::GetEnabledPasses() | 0x20);
        if (uiDimension != 0 && Renderer::GetNIRenderer()) {
            // The size is converted through a 64-bit integer load (unsigned).
            const float fHalfSize = static_cast<float>(uiDimension / 2.0);
            kPos.y = static_cast<float>(fHalfSize * -1.0);
            kPos.x = kPos.y;
            kPos.z = 0.0f;
            void* pRootMemory = NiMemObject::operator new(sizeof(NiNode));
            spRoot = pRootMemory ? ::new (pRootMemory) NiNode(0) : nullptr;

            // The quad: four corners, all facing +Z, with texture coordinates over the whole mask.
            void* pVertexMemory = ourMalloc(4 * sizeof(NiPoint3));
            NiPoint3* pVertices = nullptr;
            if (pVertexMemory) {
                rt::eh_vector_constructor_iterator(pVertexMemory, sizeof(NiPoint3), 4, &emptyPointConstructor);
                pVertices = static_cast<NiPoint3*>(pVertexMemory);
            }
            NiPoint3 kVertex0;
            pVertices[0] = *::new (&kVertex0) NiPoint3(0.0f, 0.0f, 0.0f);
            NiPoint3 kVertex1;
            pVertices[1] = *::new (&kVertex1) NiPoint3(1.0f, 0.0f, 0.0f);
            NiPoint3 kVertex2;
            pVertices[2] = *::new (&kVertex2) NiPoint3(1.0f, 1.0f, 0.0f);
            NiPoint3 kVertex3;
            pVertices[3] = *::new (&kVertex3) NiPoint3(0.0f, 1.0f, 0.0f);
            void* pNormalMemory = ourMalloc(4 * sizeof(NiPoint3));
            NiPoint3* pNormals = nullptr;
            if (pNormalMemory) {
                rt::eh_vector_constructor_iterator(pNormalMemory, sizeof(NiPoint3), 4, &emptyPointConstructor);
                pNormals = static_cast<NiPoint3*>(pNormalMemory);
            }
            pNormals[3] = NiPoint3_UNIT_Z;
            pNormals[2] = pNormals[3];
            pNormals[1] = pNormals[2];
            pNormals[0] = pNormals[1];
            void* pTexCoordMemory = ourMalloc(4 * sizeof(NiPoint2));
            NiPoint2* pTexCoords = nullptr;
            if (pTexCoordMemory) {
                rt::eh_vector_constructor_iterator(pTexCoordMemory, sizeof(NiPoint2), 4, &emptyPointConstructor);
                pTexCoords = static_cast<NiPoint2*>(pTexCoordMemory);
            }
            NiPoint2 kTexCoord0;
            pTexCoords[0] = *::new (&kTexCoord0) NiPoint2(0.0f, 0.0f);
            NiPoint2 kTexCoord1;
            pTexCoords[1] = *::new (&kTexCoord1) NiPoint2(1.0f, 0.0f);
            NiPoint2 kTexCoord2;
            pTexCoords[2] = *::new (&kTexCoord2) NiPoint2(1.0f, 1.0f);
            NiPoint2 kTexCoord3;
            pTexCoords[3] = *::new (&kTexCoord3) NiPoint2(0.0f, 1.0f);
            uint16_t* pusStripLists = static_cast<uint16_t*>(_NiMalloc(4 * sizeof(uint16_t)));
            pusStripLists[0] = 1;
            pusStripLists[1] = 0;
            pusStripLists[2] = 2;
            pusStripLists[3] = 3;
            uint16_t* pusStripLengths = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t)));
            pusStripLengths[0] = 4;
            void* pStripsDataMemory = NiMemObject::operator new(sizeof(NiTriStripsData));
            spStripsData = pStripsDataMemory
                ? ::new (pStripsDataMemory) NiTriStripsData(
                      4, pVertices, pNormals, nullptr, pTexCoords, 1, 0, 2, 1, pusStripLengths, pusStripLists
                  )
                : nullptr;

            // One quad per cell with a partial mask, textured with the cell's mask.
            for (uint32_t i = 0; i < uiDimension; ++i) {
                for (uint32_t j = 0; j < uiDimension; ++j) {
                    pMaskProp = nullptr;
                    TESObjectCELL* pCell = Get(static_cast<int32_t>(i), static_cast<int32_t>(j))->pCell;
                    if (pCell) {
                        NiRenderedTexture* pMaskTex;
                        D3DLOCKED_RECT* pMaskRect;
                        const int32_t eState = pCell->GetCanopyShadowMask(pMaskTex, pMaskRect);
                        if (pMaskTex && pMaskRect && pMaskRect->pBits) {
                            // A mask still locked from its creation is unlocked: the renderer data is the DX9 texture
                            // data.
                            NiDX9TextureData* pMaskData = static_cast<NiDX9TextureData*>(pMaskTex->GetRendererData());
                            IDirect3DBaseTexture9* pD3DTexture = pMaskData->GetD3DTexture();
                            static_cast<IDirect3DTexture9*>(pD3DTexture)->UnlockRect(0);
                            pMaskRect->pBits = nullptr;
                        }
                        if (eState == ExtraCellCanopyShadowMask::CS_PARTIAL && pMaskTex) {
                            void* pPropMemory = NiMemObject::operator new(sizeof(BSShaderNoLightingProperty));
                            pMaskProp = pPropMemory ? ::new (pPropMemory) BSShaderNoLightingProperty : nullptr;
                            pMaskProp->SetBaseMap(pMaskTex);
                            pMaskProp->SetClampNI(NiTexturingProperty::CLAMP_S_CLAMP_T);
                            bNoMasks = false;
                            bUseFullMask = bNoMasks;
                        }
                        else if (eState == ExtraCellCanopyShadowMask::CS_FULL) {
                            bNoMasks = false;
                        }
                    }
                    if (!pMaskProp) {
                        kPos.y = static_cast<float>(kPos.y + 1.0);
                        bUseFullMask = false;
                        continue;
                    }
                    void* pStripsMemory = NiMemObject::operator new(sizeof(NiTriStrips));
                    spStrips = pStripsMemory ? ::new (pStripsMemory) NiTriStrips(spStripsData.operator->()) : nullptr;
                    spStrips->SetTranslate(kPos);
                    spStrips->AttachProperty(pMaskProp);
                    BSShaderManager::PrepareObject(spStrips.operator->(), 0, 0);
                    NiNode* pRoot = spRoot.operator->();
                    pRoot->AttachChild(spStrips.operator->(), true);
                    kPos.y = static_cast<float>(kPos.y + 1.0);
                }
                kPos.x = static_cast<float>(kPos.x + 1.0);
                kPos.y = static_cast<float>(fHalfSize * -1.0);
            }

            if (bUseFullMask) {
                NiTexturingProperty* pFullMaskProp = TESRegionDataLandscape::GetDefaultFullMaskTextureProp();
                BSShaderManager::SetProjectedShadowMaskTexture(pFullMaskProp->GetBaseTexture());
            }
            else if (bNoMasks) {
                BSShaderManager::SetEnabledPasses(BSShaderManager::GetEnabledPasses() & ~0x20u);
                BSShaderManager::SetProjectedShadowMaskTexture(nullptr);
            }
            else {
                // An orthographic camera one unit above the quads, looking down at them.
                void* pCameraRootMemory = NiMemObject::operator new(sizeof(NiNode));
                spCameraRoot = pCameraRootMemory ? ::new (pCameraRootMemory) NiNode(0) : nullptr;
                void* pCameraMemory = NiMemObject::operator new(sizeof(NiCamera));
                spCamera = pCameraMemory ? ::new (pCameraMemory) NiCamera : nullptr;
                NiNode* pCameraRoot = spCameraRoot.operator->();
                pCameraRoot->AttachChild(spCamera.operator->(), true);
                spCameraRoot->SetTranslate(0.0f, 0.0f, 1.0f);
                NiUpdateData kCameraRootUpdate;
                kCameraRootUpdate.~NiUpdateData();
                ::new (&kCameraRootUpdate) NiUpdateData(0.0f, false, false);
                spCameraRoot->Update(kCameraRootUpdate);
                spCamera->LookAtWorldPoint(NiPoint3_ZERO, NiPoint3_UNIT_Y);
                NiUpdateData kCameraUpdate;
                kCameraUpdate.~NiUpdateData();
                ::new (&kCameraUpdate) NiUpdateData(0.0f, false, false);
                spCamera->Update(kCameraUpdate);
                NiUpdateData kCameraRootUpdate2;
                kCameraRootUpdate2.~NiUpdateData();
                ::new (&kCameraRootUpdate2) NiUpdateData(0.0f, false, false);
                spCameraRoot->Update(kCameraRootUpdate2);
                kFrustum.m_bOrtho = true;
                kFrustum.m_fLeft = static_cast<float>(fHalfSize * -1.0);
                kFrustum.m_fBottom = kFrustum.m_fLeft;
                kFrustum.m_fRight = fHalfSize;
                kFrustum.m_fTop = kFrustum.m_fRight;
                kFrustum.m_fNear = 1.0f;
                kFrustum.m_fFar = 2.0f;
                spCamera->SetViewFrustum(kFrustum);

                // A white light and material, drawing both faces.
                void* pLightMemory = NiMemObject::operator new(sizeof(NiDirectionalLight));
                spLight = pLightMemory ? ::new (pLightMemory) NiDirectionalLight : nullptr;
                spLight->SetAmbientColor(NiColor_WHITE);
                spLight->SetDiffuseColor(NiColor_WHITE);
                spLight->AttachAffectedNode(spRoot.operator->());
                void* pMaterialMemory = NiMemObject::operator new(sizeof(NiMaterialProperty));
                NiMaterialProperty* pMaterial = pMaterialMemory ? ::new (pMaterialMemory) NiMaterialProperty : nullptr;
                pMaterial->SetAmbientColor(NiColor_WHITE);
                pMaterial->SetDiffuseColor(NiColor_WHITE);
                pMaterial->SetEmittance(NiColor_WHITE);
                spRoot->AttachProperty(pMaterial);
                void* pStencilMemory = NiMemObject::operator new(sizeof(NiStencilProperty));
                NiStencilProperty* pStencil = pStencilMemory ? ::new (pStencilMemory) NiStencilProperty : nullptr;
                pStencil->SetDrawMode(NiStencilProperty::DRAW_BOTH);
                spRoot->AttachProperty(pStencil);
                // SWEEP-INLINE 0x822510
                if (spTexRendered.operator==(nullptr)) {
                    NiRenderer* pRenderer = NiRenderer::GetRenderer();
                    BSTextureManager* pTextureManager = BSShaderManager::TextureManagerPtr();
                    spTexRendered = pTextureManager->BorrowRenderedTexture(
                        static_cast<NiDX9Renderer*>(pRenderer), BSTextureManager::TEX_TREES_CANOPYSHADOWMASK, 0,
                        nullptr, 0
                    );
                }
                // The clear colour is saved, cleared to black for the mask and restored afterwards.
                NiDX9Renderer* pNIRenderer = Renderer::GetNIRenderer();
                pNIRenderer->GetBackgroundColor(kBackgroundColor);
                NiDX9Renderer* pClearRenderer = Renderer::GetNIRenderer();
                pClearRenderer->SetBackgroundColorAlpha(NiColorA_BLACK);
                spRoot->UpdateProperties();
                NiUpdateData kRootUpdate;
                kRootUpdate.~NiUpdateData();
                ::new (&kRootUpdate) NiUpdateData(0.0f, false, false);
                spRoot->Update(kRootUpdate);
                NiUpdateData kCameraRootUpdate3;
                kCameraRootUpdate3.~NiUpdateData();
                ::new (&kCameraRootUpdate3) NiUpdateData(0.0f, false, false);
                spCameraRoot->Update(kCameraRootUpdate3);
                NiUpdateData kCameraUpdate2;
                kCameraUpdate2.~NiUpdateData();
                ::new (&kCameraUpdate2) NiUpdateData(0.0f, false, false);
                spCamera->Update(kCameraUpdate2);

                // Draw the quads into the mask texture with a private accumulator.
                alignas(BSCullingProcess) uint8_t aCullerStorage[sizeof(BSCullingProcess)];
                BSCullingProcess* pCuller = ::new (aCullerStorage) BSCullingProcess(nullptr);
                void* pAccumulatorMemory = NiMemObject::operator new(sizeof(BSShaderAccumulator));
                BSShaderAccumulator* pCustomAccumulator =
                    pAccumulatorMemory ? ::new (pAccumulatorMemory) BSShaderAccumulator(99, 1, 759) : nullptr;
                alignas(NiPointer<BSShaderAccumulator>)
                    uint8_t aAccumulatorStorage[sizeof(NiPointer<BSShaderAccumulator>)];
                NiPointer<BSShaderAccumulator>& spCustomAccumulator =
                    *::new (aAccumulatorStorage) NiPointer<BSShaderAccumulator>(pCustomAccumulator);
                pCuller->SetAccumulator(spCustomAccumulator.operator->());
                spCustomAccumulator->SetAccumulate(false);
                BSRenderedTexture::StartOffscreen(NiRenderer::CLEAR_ALL, spTexRendered->GetGroup());
                NiNode* pScene = spRoot.operator->();
                BSShaderUtil::AccumulateScene(spCamera.operator->(), pScene, *pCuller);
                BSShaderAccumulator* pDrawAccumulator = spCustomAccumulator.operator->();
                BSShaderUtil::RenderScene(spCamera.operator->(), pDrawAccumulator, false);
                BSRenderedTexture::StopOffscreen();
                pCuller->SetAccumulator(nullptr);
                spCustomAccumulator = nullptr;
                BSShaderManager::SetProjectedShadowMaskTexture(spTexRendered->GetTexture(0));
                BSShaderManager::GetAccumulator()->SetAccumulate(true);
                Renderer::GetNIRenderer()->SetBackgroundColorAlpha(kBackgroundColor);
                spCustomAccumulator.~NiPointer<BSShaderAccumulator>();
                pCuller->~BSCullingProcess();
            }
        }
        else {
            BSShaderManager::SetEnabledPasses(BSShaderManager::GetEnabledPasses() & ~0x20u);
            BSShaderManager::SetProjectedShadowMaskTexture(nullptr);
        }
    }
    spLight.~NiPointer<NiDirectionalLight>();
    spCamera.~NiPointer<NiCamera>();
    spStripsData.~NiPointer<NiTriStripsData>();
    spStrips.~NiPointer<NiTriStrips>();
    spCameraRoot.~NiPointer<NiNode>();
    spRoot.~NiPointer<NiNode>();
    pMemContext->~AutoMemContext();
}

void GridCellArray::AttachToWorld(NiNode* apRoot, uint32_t aiX, uint32_t aiY) {
    GridCell* v4 = Get(aiX, aiY);
    if (v4->pCell) {
        v4->pCell->AttachToWorld(apRoot);
    }
}

// Cells on the grid's edge whose coordinates differ in parity skip the dynamic update; bSwitch inverts that choice on
// alternate calls. The "not" turns any nonzero byte into 0 and zero into 1.
void GridCellArray::UpdateManagedNodes(float afTime) {
    TESObjectCELL::UpdateExteriorWorld(afTime);
    uint32_t uiDimension = GetDimension();
    uint32_t uiLastPos = uiDimension - 1;
    bSwitch = (bSwitch == 0);
    for (uint32_t x = 0; x < uiDimension; ++x) {
        for (uint32_t y = 0; y < uiDimension; ++y) {
            uint8_t bDynamicUpdate = 1;
            if (x == 0 || y == 0 || x == uiLastPos || y == uiLastPos) {
                if ((x ^ y) & 1) {
                    bDynamicUpdate = 0;
                }
                if (bSwitch) {
                    bDynamicUpdate = (bDynamicUpdate == 0);
                }
            }
            GridCell* pGridCell = Get(static_cast<int32_t>(x), static_cast<int32_t>(y));
            if (pGridCell->pCell) {
                pGridCell->pCell->UpdateManagedNodes(afTime, bDynamicUpdate == 0);
            }
        }
    }
}

void GridCellArray::RunAnimations() {
    uint32_t i;
    GridCell* v3;
    uint32_t y;
    uint32_t x = 0;
    for (i = GridArray::GetDimension(); x < i; i = GridArray::GetDimension()) {
        for (y = 0; y < GridArray::GetDimension(); ++y) {
            v3 = Get(x, y);
            if (v3->pCell) {
                v3->pCell->RunAnimations();
            }
        }
        ++x;
    }
}

bool GridCellArray::GetAttached(uint32_t aiX, uint32_t aiY) {
    GridCell* v4 = Get(aiX, aiY);
    return (bool)((uint8_t)(v4->pCell && v4->pCell->GetAttached()));
}

void GridCellArray::ShowPrimitives(bool abShow) {
    GridCell* cell;
    uint32_t y;
    uint32_t x;
    if (TESObjectCELL::ExteriorWorld()) {
        for (x = 0; x < GridArray::GetDimension(); ++x) {
            for (y = 0; y < GridArray::GetDimension(); ++y) {
                cell = Get(x, y);
                if (cell->pCell) {
                    if (cell->pCell->GetAttached()) {
                        cell->pCell->ShowPrimitives(abShow);
                    }
                }
            }
        }
    }
}

void GridCellArray::Set(uint32_t aiX, uint32_t aiY, TESObjectCELL* apCell) {
    if (apCell) {
        Get(aiX, aiY)->pCell = apCell;
    }
    else {
        ClearItem(aiX, aiY);
    }
}

void GridCellArray::AttachAllLand3D() {
    if (!bLandAttached) {
        AutoMemContext apMemory(
            MEM_CONTEXT_LAND, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\GridCell.cpp", 269
        );
        uint32_t CastingType = GetDimension();
        for (uint32_t x = 0; x < CastingType; ++x) {
            for (uint32_t y = 0; y < CastingType; ++y) {
                GridCell* v3 = Get(x, y);
                if (v3) {
                    if (v3->pCell) {
                        TESObjectLAND* Land = v3->pCell->GetLand();
                        Land->Attach3D();
                        v3->pCell->SetupMopp();
                    }
                }
            }
        }
        bLandAttached = 1;
    }
}

void GridCellArray::SetRecreateCanopyShadowMaskTexture(bool abRecreate) {
    bRecreateCanopyShadowMaskTexture = abRecreate;
}

// The engine first clamps values below 3 up to 3, but the local it tests is always 0, so that branch is dead. The
// setting is read again at every test and before the increment.
void GridCellArray::FixGridsToLoadINISetting() {
    if (uGridsToLoad.UInt() < 5) {
        uGridsToLoad = 5;
    }
    else if ((uGridsToLoad.UInt() & 1) == 0) {
        uGridsToLoad = static_cast<int32_t>(uGridsToLoad.UInt() + 1);
    }
}

void BSShaderAccumulator::SetAccumulate(bool abTF) {
    bAccumulate = abTF;
}

void ShadowLightShader::SetProjectedShadowMaskLL(NiPoint2 aLL) {
    ShadowLightShader::kShadowProjData.x = aLL.x;
    ShadowLightShader::kShadowProjData.y = aLL.y;
}

void NiDynamicEffect::IncRevisionID() {
    ++m_uiRevID;
}

void NiMaterialProperty::SetAmbientColor(const NiColor& c) {}

void NiMaterialProperty::SetDiffuseColor(const NiColor& c) {}

NiTexture* NiTexturingProperty::Map::GetTexture() const {
    return m_spTexture.m_pObject;
}

NiTexture* NiTexturingProperty::GetBaseTexture() {
    Map* pkMap = m_kMaps[BASE_INDEX];
    if (pkMap) {
        return pkMap->GetTexture();
    }
    return nullptr;
}
