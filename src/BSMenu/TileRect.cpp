#include "BSMenu/TileRect.h"

#include "BSMenu/Tile.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"

TileRect::TileRect() {}

TileRect::~TileRect() {
    if (!IsReleased()) {
        Release();
    }
}

void TileRect::Init(Tile* apParent, const char* apName, Tile* apReplacedTile) {
    Tile::Init(apParent, apName, apReplacedTile);
    this->Set(kTileValue_Visible, 1.0f, true);
    this->SetNeedsUpdate(CHANGE_POSITION | CHANGE_CREATE);
}

// Builds the rectangle's empty node under the parent's model (or the interface root), named after the tile.
NiNode* TileRect::MakeNode() {
    NiNode* parentModel = this->GetParentModel();
    NiNode* node = new NiNode(0);
    this->spModel = node;
    if (!parentModel) {
        parentModel = Interface::GetInterfaceRoot();
    }
    parentModel->AttachChild(this->spModel.m_pObject, true);
    // A temporary fixed string of the name, built and released inline.
    NiGlobalStringTable::GlobalStringHandle kName = this->xName.pString
        ? NiGlobalStringTable::AddString(this->xName.pString)
        : NiGlobalStringTable::NULL_STRING;
    this->spModel->SetName(reinterpret_cast<const NiFixedString&>(kName));
    if (kName != NiGlobalStringTable::NULL_STRING) {
        InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kName) - 8));
    }
    // A hidden tile's node is culled.
    NiNode* model = this->spModel.m_pObject;
    const bool culled = !this->IsTrue(kTileValue_Visible);
    if (culled) {
        model->m_uFlags = NiAVObject::NiFlags(model->m_uFlags | NiAVObject::APP_CULLED);
    }
    else {
        model->m_uFlags = NiAVObject::NiFlags(model->m_uFlags & ~NiAVObject::APP_CULLED);
    }
    this->spModel->UpdateProperties();
    Interface::SetNeedToUpdate(true);
    Tile::Extra* extra = new Tile::Extra(this, this->spModel.m_pObject);
    this->spModel->AddExtraData(extra);
    this->FinalPostParse(kTileValue_X, this->GetFloat(kTileValue_X), nullptr);
    this->FinalPostParse(kTileValue_Y, this->GetFloat(kTileValue_Y), nullptr);
    return this->spModel.m_pObject;
}

int32_t TileRect::GetType() {
    return kTileValue_Rect;
}

Tile* TileRect::PostParse(int32_t aiTrait, float afValue, const char* axString) {
    return nullptr;
}

Tile::Extra::Extra(Tile* apTile, NiNode* apNode) : pTile(apTile), pNode(apNode) {
    NiFixedString kName;
    kName = "Tileptr";
    SetName(kName);
}
