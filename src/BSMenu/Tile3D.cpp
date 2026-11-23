#include "BSMenu/Tile3D.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/TileShaderProperty.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <limits>

BSTimer Tile3D::xTimer;

// Starts the frame timer of the animated tiles along with the tile.
void Tile3D::Init(Tile* apParent, const char* apName, Tile* apReplacedTile) {
    Tile::Init(apParent, apName, apReplacedTile);
    Set(kTileValue_Brightness, 255.0f, true);
    Set(kTileValue_Red, 255.0f, true);
    Set(kTileValue_Green, 255.0f, true);
    Set(kTileValue_Blue, 255.0f, true);
    Set(kTileValue_Alpha, 255.0f, true);
    Set(kTileValue_Visible, 1.0f, true);
    SetNeedsUpdate(CHANGE_POSITION | CHANGE_CREATE);
    xTimer.Enable();
    xTimer.Update(GetTickCount());
}

// Drops the old model (cutting its parent link by hand), loads the NIF into a new node and hangs that under the
// parent's model (or the interface root) with the tile's extra data. The parent tile's GetParentModel is asked.
NiNode* Tile3D::MakeNode() {
    if (spModel.m_pObject) {
        spModel->m_pkParent = nullptr;
        spModel = nullptr;
    }
    NiNode* pNode = new NiNode(0);
    spModel = pNode;
    UpdateNIF();
    if (spModel.m_pObject) {
        NiNode* pParentModel = pParent->GetParentModel();
        if (!pParentModel) {
            pParentModel = Interface::GetInterfaceRoot();
        }
        pParentModel->AttachChild(spModel.m_pObject, true);
        Extra* pExtra = new Extra(this, spModel.m_pObject);
        spModel->AddExtraData(pExtra);
    }
    return spModel.m_pObject;
}

int32_t Tile3D::GetType() {
    return kTileValue_Nif;
}

Tile* Tile3D::PostParse(int32_t aiTrait, float afValue, const char* axString) {
    if (aiTrait == kTileValue_String) {
        this->strCurrentName.Set("", 0);
    }
    return nullptr;
}

// Every geometry under the node is taken to carry a tile shader property.
void Tile3D::SetAlphaAndColor(NiNode* apNode, float afAlpha, const NiColorA& aColor) {
    if (!apNode) {
        return;
    }
    for (NiAVObject* pChild : apNode->m_kChildren) {
        if (!pChild) {
            continue;
        }
        SetAlphaAndColor(pChild->IsNode(), afAlpha, aColor);
        NiGeometry* pGeometry = pChild->IsGeometry();
        if (pGeometry) {
            TileShaderProperty* pTileShader = static_cast<TileShaderProperty*>(
                pGeometry->GetProperty(NiProperty::SHADE)
            );
            pTileShader->fAlpha = afAlpha;
            pTileShader->TintColor = aColor;
        }
    }
}

bool Tile3D::IsPlaying() {
    return this->pCurrentSequence && this->pCurrentSequence->GetState() == NiControllerSequence::kAnimating;
}

bool Tile3D::FindManagerNode(NiNode* apNode) {
    NiTimeController* manager = apNode->GetController(&NiControllerManager::ms_RTTI);
    if (!manager) {
        return false;
    }
    this->pAniManager = static_cast<NiControllerManager*>(manager);
    return true;
}

void Tile3D::UpdateNIF() {
    const char* pFilename = GetString(kTileValue_Filename);
    if (!pFilename) {
        return;
    }
    if (std::strstr(pFilename, "\\Data")) {
        strFilename.SPrintF("%s", pFilename);
    }
    else {
        strFilename.SPrintF("%s\\%s", Interface::GetMeshesFolder(), pFilename);
    }
    bool bFromModelLoader = false;
    NiNode* pModel = Interface::LoadModelFile(strFilename.pString, bFromModelLoader);
    if (!pModel) {
        spModel = nullptr;
        return;
    }
    if (bFromModelLoader) {
        uiFlags |= MODEL_FROM_MODEL_LOADER;
    }
    else {
        uiFlags &= ~MODEL_FROM_MODEL_LOADER;
    }
    if (pModel->m_pkParent) {
        pModel = Interface::CopyOrDeepCopyNode(pModel);
    }
    FindManagerNode(pModel);
    spModel->m_kChildren.RemoveAll();
    RemoveProperties(pModel);
    SetupShaderPropertiesRecurse(pModel);
    spModel->AttachChild(pModel, true);
    Set(kTileValue_Locus, 1.0f, true);
    // A hidden tile's node is culled.
    NiNode* pNode = spModel.m_pObject;
    if (!IsTrue(kTileValue_Visible)) {
        pNode->m_uFlags = NiAVObject::NiFlags(pNode->m_uFlags | NiAVObject::APP_CULLED);
    }
    else {
        pNode->m_uFlags = NiAVObject::NiFlags(pNode->m_uFlags & ~NiAVObject::APP_CULLED);
    }
    spModel->UpdateProperties();
    if (uiFlags & CHANGE_NIF_FILE) {
        uiFlags ^= CHANGE_NIF_FILE;
    }
    AddNeedsUpdate(CHANGE_POSITION | CHANGE_COLOR);
    Interface::SetNeedToUpdate(true);
    Interface::ClearTempModel();
}

void Tile3D::RemoveProperties(NiNode* pTarget) {
    pTarget->RemoveProperty(NiProperty::MATERIAL);
    pTarget->CompactChildArray();
    for (uint32_t i = 0; i < pTarget->m_kChildren.m_usESize; ++i) {
        NiAVObject* pChild = pTarget->m_kChildren.m_pBase[i].m_pObject;
        if (pChild) {
            pChild->RemoveProperty(NiProperty::MATERIAL);
            NiNode* pChildNode = pChild->IsNode();
            if (pChildNode) {
                RemoveProperties(pChildNode);
            }
        }
    }
}

// The sequence that was playing is finished first and stopped, and the new one is activated from the start.
bool Tile3D::PlaySequence(const char* apSequenceName) {
    if (!spModel.m_pObject || !pAniManager || !apSequenceName) {
        return false;
    }
    NiGlobalStringTable::GlobalStringHandle kName = apSequenceName ? NiGlobalStringTable::AddString(apSequenceName)
                                                                   : NiGlobalStringTable::NULL_STRING;
    NiControllerSequence* pFound;
    NiControllerSequence* pSequence = pAniManager->m_kStringMap.GetAt(kName, pFound) ? pFound : nullptr;
    if (kName != NiGlobalStringTable::NULL_STRING) {
        InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kName) - 8));
    }
    if (!pSequence) {
        return false;
    }
    if (pCurrentSequence) {
        if (pCurrentSequence->m_eState == NiControllerSequence::kAnimating) {
            UpdateSequence(1.0f, true);
        }
        pCurrentSequence->Deactivate(0.0f, false);
    }
    if (pSequence->m_eState != NiControllerSequence::kInactive) {
        pSequence->Deactivate(0.0f, false);
    }
    if (!pSequence->Activate(0, false, 1.0f, 0.0f, nullptr, false)) {
        return false;
    }
    // Equivalent to NiTimeController::SetActive(true).
    pAniManager->m_usFlags = static_cast<uint16_t>(pAniManager->m_usFlags | NiTimeController::ACTIVE_MASK);
    pCurrentSequence = pSequence;
    return true;
}

// The "-FLT_MAX" last time marks a sequence that has not run yet: the model is updated at time 0. Playing on (-1)
// wraps the advanced time into the key range and, once the sequence's sounds report the end, stops it; a
// percentage (or a value above 1) is clamped into the key range.
bool Tile3D::UpdateSequence(float afValue, bool abIsPercentage) {
    const char* pAnimation = GetString(kTileValue_Animation);
    if ((strCurrentName.pString || pAnimation) && strCurrentName.StrCmp(pAnimation, true) != 0) {
        strCurrentName.Set(pAnimation, 0);
        if (strCurrentName.Length() && !PlaySequence(strCurrentName.pString)) {
            strCurrentName.Set(nullptr, 0);
        }
    }
    if (!spModel.m_pObject || !pAniManager || !IsTrue(kTileValue_Visible)) {
        return false;
    }
    if (!IsTrue(kTileValue_Visible)) {
        NiNode* pModel = spModel.m_pObject;
        pModel->m_uFlags = NiAVObject::NiFlags(pModel->m_uFlags | NiAVObject::APP_CULLED);
    }
    if (!pCurrentSequence) {
        return false;
    }
    if (pCurrentSequence->m_fLastTime == -std::numeric_limits<float>::max()) {
        NiUpdateData kData = {0.0f, true, false, false, false, false};
        spModel->Update(kData);
    }
    else if (afValue == -1.0) {
        float fTime = pCurrentSequence->m_fLastTime + xTimer.fDelta;
        const float fEndKeyTime = pCurrentSequence->m_fEndKeyTime;
        while (fTime > fEndKeyTime) {
            fTime = fTime - fEndKeyTime;
        }
        xTimer.Update(GetTickCount());
        NiUpdateData kData = {fTime, true, false, false, false, false};
        spModel->Update(kData);
        if (Interface::PlayNifSounds(pCurrentSequence)) {
            pCurrentSequence->Deactivate(0.0f, false);
            pCurrentSequence = nullptr;
            // NiTimeController::SetActive(false).
            pAniManager->m_usFlags = static_cast<uint16_t>(
                pAniManager->m_usFlags & ~static_cast<uint32_t>(NiTimeController::ACTIVE_MASK)
            );
        }
    }
    else if (abIsPercentage || afValue > 1.0) {
        const float fEndKeyTime = pCurrentSequence->m_fEndKeyTime;
        const float fTime = afValue * fEndKeyTime;
        const float fLow = fTime < 0.0f ? 0.0f : fTime;
        const float fClamped = fLow > fEndKeyTime ? fEndKeyTime : fLow;
        NiUpdateData kData = {fClamped, true, false, false, false, false};
        spModel->Update(kData);
    }
    else {
        NiUpdateData kData = {afValue, true, false, false, false, false};
        spModel->Update(kData);
    }
    return true;
}

// A geometry that already has a tile shader property (shader type 0x13) keeps it.
void Tile3D::SetupShaderPropertiesRecurse(NiNode* apNode) {
    if (!apNode) {
        return;
    }
    for (uint32_t i = 0; i < apNode->m_kChildren.m_usESize; ++i) {
        if (!apNode->m_kChildren.m_pBase[i].m_pObject) {
            continue;
        }
        NiGeometry* pGeometry = apNode->m_kChildren.m_pBase[i]->IsGeometry();
        if (pGeometry) {
            NiShadeProperty* pShade = static_cast<NiShadeProperty*>(pGeometry->GetProperty(NiProperty::SHADE));
            if (pShade && pShade->iShaderPropertyType != 0x13) {
                TileShaderProperty* pTileShader = new TileShaderProperty(false);
                pGeometry->RemoveProperty(NiProperty::SHADE);
                pGeometry->AttachProperty(pTileShader);
                NiTexturingProperty* pTexturing = static_cast<NiTexturingProperty*>(
                    pGeometry->GetProperty(NiProperty::TEXTURING)
                );
                if (pTexturing) {
                    // Equivalent to NiTexturingProperty::GetBaseTexture.
                    NiTexturingProperty::Map* pBaseMap = pTexturing->m_kMaps.m_pBase[NiTexturingProperty::BASE_INDEX];
                    pTileShader->SetTileTexture(pBaseMap ? pBaseMap->m_spTexture.m_pObject : nullptr);
                }
                // The interface's tile shader.
                pGeometry->m_spShader = BSShaderManager::GetShader(0x20);
            }
        }
        NiNode* pChildNode = apNode->m_kChildren.m_pBase[i]->IsNode();
        if (pChildNode) {
            SetupShaderPropertiesRecurse(pChildNode);
        }
    }
}

void Tile3D::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
