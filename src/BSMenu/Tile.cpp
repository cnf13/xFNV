#include "BSMenu/Tile.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/NiObjectGarbage.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/RadialTile.h"
#include "BSMenu/Tile3D.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSMenu/TileRect.h"
#include "BSMenu/TileText.h"
#include "BSShader/TileShaderProperty.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TES.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cstdint>
#include <cstring>
#include <string>

// Set by ParseFile on a syntax error; ReadFile clears it and stops at it.
static bool bReadFileError;
BSPerformanceTimer TileUpdateTimer("Interface", "TileUpdate", false, true);
Tile::MenuStringMap Tile::xTextTable;
Tile::TraitExtraDataMap Tile::xTraitExtraData;
NiTPointerListBase<Tile*> Tile::xDirtyTiles;
NiTPointerListBase<Tile*> Tile::xHibernatingTiles;
BSCriticalSection Tile::TileUpdateAllCritSect;
BSSimpleList<Tile::FadeControl*> Tile::xFadeControls;
BSSimpleList<Tile::TextureAtlasEntry*> Tile::TextureEntryList;
Tile::ValueReactionMap Tile::ValueReactionList;
SettingT<INISettingCollection> sDefaultTexAtlas("sDefaultTexAtlas", "Interface\\InterfaceShared.tai");

using FloatArray = BSSimpleArray<float, 1024>;

void XMLStorage::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

Tile::Value::~Value() {
    Tile::Reaction* preactionListA = nullptr;
    ClearActions();
    if (Tile::ValueReactionList.GetAt(this, preactionListA)) {
        while (preactionListA) {
            for (Tile::Action* pcurrAction = preactionListA->preactionValue->pActionListA; pcurrAction;
                 pcurrAction = pcurrAction->pnext) {
                if (pcurrAction->QRefValue() == this) {
                    // Only a RefValueAction has a value to return, so the action is one.
                    static_cast<Tile::RefValueAction*>(pcurrAction)->pRefValue = nullptr;
                }
            }
            Tile::Reaction* premove = preactionListA;
            preactionListA = preactionListA->pnext;
            MemoryManager_Singleton.Deallocate(premove);
        }
        Tile::ValueReactionList.RemoveAt(this);
    }
    if (strValue) {
        MemoryManager_Singleton.Deallocate(strValue);
    }
    fValue = 0.0f;
    pParent = nullptr;
}

void Tile::Value::SetString(const char* apString, bool abClearActions) {
    bool v3 = (!strValue && apString) ||
        ((strValue && !apString) || (strValue && (apString && std::strcmp(strValue, apString))));
    if (strValue) {
        MemoryManager_Singleton.Deallocate(strValue);
        strValue = nullptr;
    }
    if (apString) {
        size_t v5 = std::strlen(apString);
        strValue = static_cast<char*>(MemoryManager_Singleton.Allocate(v5 + 1));
        strcpy_s(strValue, v5 + 1, apString);
    }
    fValue = 0.0;
    if (abClearActions) {
        ClearActions();
    }
    CalculateValue(v3);
}

// Runs the value's action list from its current value: each action applies its operator (a kTileValue_* id) with its
// operand, a constant or another trait's value; ActionBegin/ActionEnd push and pop the running value on a local float
// stack. A string operand is copied by Copy. A changed (or forced) value recalculates its dependants and is reported to
// the tile. The String trait's value is also written out as its decimal text. While the tile connects its traits only
// the class trait is calculated; nothing is calculated for a tile without a parent or one whose menu is being deleted.
void Tile::Value::CalculateValue(bool abForceUpdate) {
    // The parent's flags are read before the parent is tested for null, as the game does.
    if ((this->pParent->uiFlags & CONNECTING_TRAITS) != 0 && this->eIndex != kTileValue_Class) {
        return;
    }
    if (!this->pParent || (this->pParent->uiFlags & MENU_DELETING) != 0) {
        return;
    }

    // EXE-INLINED none: AutoMemContext
    int32_t iOldMemContext = GetThreadMemContext();
    GetThreadMemContext() = MEM_CONTEXT_INTERFACE;
    Action* pAction = static_cast<Action*>(this->pActionListA);
    float fOldValue = this->fValue;
    FloatArray kStack;
    uint8_t bChanged = 0;
    while (pAction) {
        float fOperand = pAction->QFloat();
        const char* pcString = nullptr;
        if (pAction->QRefValue()) {
            Value* pRefValue = pAction->QRefValue();
            int32_t iExtraData = 0;
            pcString = pRefValue->strValue;
            if (pRefValue->eIndex >= 10000) {
                Tile::xTraitExtraData.GetAt(pRefValue->eIndex, iExtraData);
            }
            // A user trait with an underscore id reads the trait named after it and this value's integer.
            if (iExtraData == -1) {
                Value* pUnderscoreValue =
                    pRefValue->pParent->GetUnderscoreValue(pRefValue->eIndex, NiFastFloatToInt(this->fValue));
                if (pUnderscoreValue) {
                    fOperand = pUnderscoreValue->fValue;
                    pcString = pUnderscoreValue->strValue;
                }
            }
        }
        int32_t eType = static_cast<int32_t>(pAction->eActionType);
        if (eType == kTileValue_ActionBegin) {
            kStack.Add(this->fValue);
            this->fValue = 0.0f;
        }
        else if (eType == kTileValue_ActionEnd) {
            // The group's result becomes the operand of the group's operator (the action's float), applied to the
            // value saved by the matching begin.
            float fSaved = kStack.pBuffer[kStack.iSize - 1];
            eType = NiFastFloatToInt(pAction->QFloat());
            fOperand = this->fValue;
            this->fValue = fSaved;
            if (kStack.iSize != 0) {
                // EXE-INLINED none: BSSimpleArray<float,1024> pop (an empty one-item destruct loop, then the count)
                --kStack.iSize;
            }
        }
        switch (eType) {
            case kTileValue_Copy:
                if (pcString) {
                    size_t uiLength = std::strlen(pcString);
                    bChanged = !this->strValue || std::strcmp(this->strValue, pcString) != 0;
                    if (this->strValue) {
                        MemoryManager_Singleton.Deallocate(this->strValue);
                        this->strValue = nullptr;
                    }
                    this->strValue = static_cast<char*>(MemoryManager_Singleton.Allocate(uiLength + 1));
                    strcpy_s(this->strValue, uiLength + 1, pcString);
                }
                else {
                    this->fValue = fOperand;
                }
                break;
            case kTileValue_Add: this->fValue = this->fValue + fOperand; break;
            case kTileValue_Sub: this->fValue = this->fValue - fOperand; break;
            case kTileValue_Mul: this->fValue = this->fValue * fOperand; break;
            case kTileValue_Div:
                if (fOperand != 0.0) {
                    this->fValue = this->fValue / fOperand;
                }
                break;
            case kTileValue_Min: {
                // An unordered compare takes the operand.
                float fCurrent = this->fValue;
                this->fValue = fOperand > fCurrent ? fCurrent : fOperand;
                break;
            }
            case kTileValue_Max: {
                // An unordered compare takes the operand.
                float fCurrent = this->fValue;
                this->fValue = fOperand < fCurrent ? fCurrent : fOperand;
                break;
            }
            case kTileValue_Mod:
                if (fOperand != 0.0) {
                    int32_t iValue = NiFastFloatToInt(this->fValue);
                    int32_t iOperand = NiFastFloatToInt(fOperand);
                    this->fValue = static_cast<float>(iValue % iOperand);
                }
                break;
            case kTileValue_Floor: {
                float fSum = fOperand + this->fValue;
                this->fValue = std::floor(fSum);
                break;
            }
            case kTileValue_Ceil: {
                float fSum = fOperand + this->fValue;
                this->fValue = std::ceil(fSum);
                break;
            }
            case kTileValue_Abs: {
                float fSum = fOperand + this->fValue;
                this->fValue = std::fabs(fSum);
                break;
            }
            case kTileValue_Round: {
                // A multiple of the operand: the quotient truncated, plus one when its fraction is not below 0.5 (an
                // unordered compare rounds up). The integer is subtracted and multiplied exactly.
                float fQuotient = this->fValue / fOperand;
                int32_t iRoundUp = !((static_cast<double>(fQuotient) - static_cast<int32_t>(fQuotient)) < 0.5);
                this->fValue =
                    static_cast<float>(static_cast<double>(fOperand) * (static_cast<int32_t>(fQuotient) + iRoundUp));
                break;
            }
            case kTileValue_Gt: this->fValue = static_cast<float>(static_cast<int32_t>(fOperand < this->fValue)); break;
            case kTileValue_Gte:
                this->fValue = static_cast<float>(static_cast<int32_t>(fOperand <= this->fValue));
                break;
            case kTileValue_Eq:
                this->fValue = static_cast<float>(static_cast<int32_t>(fOperand == this->fValue));
                break;
            case kTileValue_Neq:
                this->fValue = static_cast<float>(static_cast<int32_t>(fOperand != this->fValue));
                break;
            case kTileValue_Lt: this->fValue = static_cast<float>(static_cast<int32_t>(fOperand > this->fValue)); break;
            case kTileValue_Lte:
                this->fValue = static_cast<float>(static_cast<int32_t>(fOperand >= this->fValue));
                break;
            case kTileValue_And:
                if (pcString) {
                    this->fValue = static_cast<float>(static_cast<int32_t>(this->fValue != 0.0 && pcString));
                }
                else {
                    this->fValue = static_cast<float>(static_cast<int32_t>(this->fValue != 0.0 && fOperand != 0.0));
                }
                break;
            case kTileValue_Or:
                if (pcString) {
                    this->fValue = static_cast<float>(static_cast<int32_t>(this->fValue != 0.0 || pcString));
                }
                else {
                    this->fValue = static_cast<float>(static_cast<int32_t>(this->fValue != 0.0 || fOperand != 0.0));
                }
                break;
            case kTileValue_Not:
                if (pcString) {
                    this->fValue = static_cast<float>(static_cast<int32_t>(pcString == nullptr));
                }
                else {
                    this->fValue = static_cast<float>(static_cast<int32_t>(fOperand == 0.0));
                }
                break;
            case kTileValue_OnlyIf:
                if (fOperand == 0.0 && !pcString) {
                    this->fValue = 0.0f;
                }
                break;
            case kTileValue_OnlyIfNot:
                if (fOperand != 0.0 || pcString) {
                    this->fValue = 0.0f;
                }
                break;
            default: break;
        }
        pAction = pAction->pnext;
    }
    if (!this->strValue) {
        bChanged = this->fValue != fOldValue;
    }
    if (this->fValue != fOldValue && this->eIndex == kTileValue_String) {
        // The string trait also holds its value as decimal text: count the digits (and the sign), resize the
        // string when its length differs, then print it.
        int32_t iValue = NiFastFloatToInt(this->fValue);
        int32_t iLength = iValue == 0;
        for (int32_t iRest = iValue; iRest; iRest /= 10) {
            ++iLength;
        }
        if (iValue < 0) {
            ++iLength;
        }
        if (!this->strValue || (this->strValue && std::strlen(this->strValue) != static_cast<size_t>(iLength))) {
            MemoryManager_Singleton.Deallocate(this->strValue);
            this->strValue = static_cast<char*>(MemoryManager_Singleton.Allocate(iLength + 1));
        }
        BSsprintf(this->strValue, iLength + 1, "%d", iValue);
        bChanged = 1;
    }
    if (bChanged || abForceUpdate) {
        Tile::ValueChangeEvent(this);
        if (this->pParent && (this->pParent->uiFlags & RELEASED) == 0 &&
            !this->pParent->PostParse(this->eIndex, this->fValue, this->strValue)) {
            this->pParent->FinalPostParse(this->eIndex, this->fValue, this->strValue);
        }
    }
    // EXE-INLINED none: AutoMemContext
    GetThreadMemContext() = iOldMemContext;
}

void Tile::Value::AddAction(int32_t aiActionType, float afActionVal) {
    Tile::FloatAction* pnewAction_2;
    Tile::Action* ptail = static_cast<Tile::Action*>(pActionListA);
    Tile::FloatAction* pnewAction =
        static_cast<Tile::FloatAction*>(MemoryManager_Singleton.Allocate(sizeof(Tile::FloatAction)));
    if (pnewAction) {
        pnewAction_2 = ::new (pnewAction) Tile::FloatAction;
    }
    else {
        pnewAction_2 = nullptr;
    }
    pnewAction_2->eActionType = static_cast<eTileValue>(aiActionType);
    pnewAction_2->fValue = afActionVal;
    if (ptail) {
        while (ptail->pnext) {
            ptail = ptail->pnext;
        }
        ptail->pnext = pnewAction_2;
    }
    else {
        pActionListA = pnewAction_2;
    }
}

void Tile::Value::AddAction(int32_t aiActionType, Tile* apSourceTile, int32_t aeSourceTrait) {
    Tile::RefValueAction* pnewAction_2;
    Tile::Action* ptail = static_cast<Tile::Action*>(pActionListA);
    Tile::RefValueAction* pnewAction =
        static_cast<Tile::RefValueAction*>(MemoryManager_Singleton.Allocate(sizeof(Tile::RefValueAction)));
    if (pnewAction) {
        pnewAction_2 = ::new (pnewAction) Tile::RefValueAction;
    }
    else {
        pnewAction_2 = nullptr;
    }
    pnewAction_2->eActionType = static_cast<eTileValue>(aiActionType);
    pnewAction_2->pRefValue = apSourceTile->GetOrCreateValue(aeSourceTrait);
    if (ptail) {
        while (ptail->pnext) {
            ptail = ptail->pnext;
        }
        ptail->pnext = pnewAction_2;
    }
    else {
        pActionListA = pnewAction_2;
    }
    Tile::AddReaction(pnewAction_2->pRefValue, this);
}

void Tile::Value::ClearActions() {
    Tile::Reaction* preactionListA;
    Tile::Action** ppcurrAction = &pActionListA;
    while (true) {
        Tile::Action* pcurrAction = *ppcurrAction;
        if (!pcurrAction) {
            break;
        }
        if (pcurrAction->QRefValue()) {
            preactionListA = nullptr;
            if (Tile::ValueReactionList.GetAt(pcurrAction->QRefValue(), preactionListA)) {
                Tile::Reaction** ppcurrReaction = &preactionListA;
                Tile::Reaction* pprevhead = preactionListA;
                while (true) {
                    Tile::Reaction* pcurrReaction = *ppcurrReaction;
                    if (!*ppcurrReaction) {
                        break;
                    }
                    if (pcurrReaction->preactionValue == this) {
                        *ppcurrReaction = pcurrReaction->pnext;
                        MemoryManager_Singleton.Deallocate(pcurrReaction);
                    }
                    else {
                        ppcurrReaction = &pcurrReaction->pnext;
                    }
                }
                if (preactionListA) {
                    if (preactionListA != pprevhead) {
                        Tile::Reaction* pnewhead = preactionListA;
                        Tile::ValueReactionList.SetAt(pcurrAction->QRefValue(), pnewhead);
                    }
                }
                else {
                    Tile::ValueReactionList.RemoveAt(pcurrAction->QRefValue());
                }
            }
        }
        *ppcurrAction = pcurrAction->pnext;
        MemoryManager_Singleton.Deallocate(pcurrAction);
    }
}

void Tile::Value::SetFloat(float afValue, bool abClearActions) {
    uint8_t v3 = (uint8_t)((afValue != fValue) || strValue);
    fValue = afValue;
    if (strValue) {
        (&MemoryManager_Singleton)->Deallocate(strValue);
        strValue = nullptr;
    }
    if (abClearActions) {
        ClearActions();
    }
    CalculateValue(v3);
}

Tile::TileTemplateItem::TileTemplateItem(int32_t aiCmd, float afVal, const char* axStr, int32_t aiTA, int32_t aiLine)
  : iCmd(aiCmd), fVal(afVal), iLine(aiLine) {
    xStr.Set(axStr, 31u);
    u.iTrait = aiTA;
}

Tile::TileTemplate::TileTemplate(const char* apString, BuildStorage* apParent) : pParent(apParent) {
    xName = apString;
}

// xList empties itself in its own destructor.
Tile::TileTemplate::~TileTemplate() {
    Clear();
}

// Appends the parsed XML command aiCmd with its string apStr: numeric strings become the value, and the last commands
// are merged with this one where they form a template definition, a tile open, a trait value or an action operand.
// pParent is used as the BuildStorage that owns the template (BuildStorage::GetSubTemplateByName is called on it and
// its pCurrentTemplate is read), though it is typed TileTemplate*: hence the casts.
void Tile::TileTemplate::AddPair(int32_t aiCmd, const char* apStr, int32_t aiLine, bool aiPreserveStrings) {
    float fVal = -2147483648.0f;
    if (!aiPreserveStrings) {
        fVal = static_cast<float>(Tile::TextToTrait(apStr));
    }
    if (fVal == -2147483648.0 && (apStr[0] == '_' || (apStr[0] == '&' && apStr[1] == '_'))) {
        fVal = static_cast<float>(Tile::AddUserTrait(apStr, -1));
    }
    const int32_t iTA = static_cast<int32_t>(fVal);
    TileTemplateItem* pItem = GetItem(aiCmd, fVal, apStr, iTA, aiLine);
    bool bNumber = true;
    for (uint32_t i = 0; i < std::strlen(apStr); ++i) {
        if ((apStr[i] < '0' || apStr[i] > '9') && apStr[i] != '-' && apStr[i] != '.') {
            bNumber = false;
            break;
        }
    }
    if (bNumber) {
        if (fVal == -2147483648.0) {
            std::sscanf(apStr, "%f", &fVal);
        }
        if (fVal == 0.0) {
            std::sscanf(apStr, "%f", &fVal);
        }
        pItem->xStr.Set("", 0);
        pItem->fVal = fVal;
        pItem->u.iTrait = static_cast<int32_t>(fVal);
    }
    NiTListItem<TileTemplateItem*>* pPos = xList.m_pkTail;
    TileTemplateItem* pLast = nullptr;
    TileTemplateItem* pPrev = nullptr;
    TileTemplateItem* pPrev2 = nullptr;
    if (pPos) {
        pLast = xList.GetPrev(pPos);
    }
    if (pPos) {
        pPrev = xList.GetPrev(pPos);
    }
    if (pPos) {
        pPrev2 = xList.GetPrev(pPos);
    }
    if (aiCmd == 3002 && pLast && pLast->iCmd == 0 && pLast->fVal == 999.0) {
        if (pParent->pCurrentTemplate) {
            Warning("MENUS: Can't have nested template definitions in an XML file.");
        }
        else {
            TileTemplate* pSub = pParent->GetSubTemplateByName(apStr);
            pParent->pCurrentTemplate = pSub;
            if (!pParent->pCurrentTemplate) {
                pSub = pParent->NewSubTemplate(apStr);
                pParent->pCurrentTemplate = pSub;
            }
        }
        ThrowAway(xList.RemoveTail());
        ThrowAway(pItem);
    }
    else if (aiCmd == 1 && fVal == 999.0 && pParent->pCurrentTemplate) {
        pParent->pCurrentTemplate = nullptr;
        ThrowAway(pItem);
    }
    else if (aiCmd == 3002 && pLast && pLast->iCmd == 0 && pLast->fVal >= 901.0 && pLast->fVal <= 908.0) {
        pLast->iCmd = 6;
        pLast->xStr.Set(apStr, 0);
        ThrowAway(pItem);
    }
    else if (aiCmd == 3002) {
        pItem->iCmd = 8;
        pItem->u.iTrait = 3002;
        // SWEEP-INLINE 0x43A010
        NiTListItem<TileTemplateItem*>* pNode = xList.NewItem();
        pNode->m_element = pItem;
        xList.AddNodeTail(pNode);
    }
    else if (
        aiCmd == 1 && pLast && pLast->iCmd == 3001 && pPrev && (pPrev->iCmd == 4 || pPrev->iCmd == 2) &&
        fVal == pPrev->fVal
    ) {
        if ((pItem->fVal >= 4001.0 && pItem->fVal <= 4125.0) || pItem->u.iTrait >= 10000) {
            pPrev->iCmd = 8;
        }
        else if (pItem->fVal >= 2000.0 && pItem->fVal <= 2025.0) {
            pPrev->iCmd = 9;
        }
        else {
            pPrev->iCmd = -1;
            Warning("MENUS: Bad trait/action type in XML");
        }
        pPrev->u.iTrait = static_cast<int32_t>(pPrev->fVal);
        pPrev->fVal = pLast->fVal;
        pPrev->xStr.Set(pLast->xStr);
        ThrowAway(xList.RemoveTail());
        ThrowAway(pItem);
    }
    else if (
        aiCmd == 1 && pLast && pLast->iCmd == 3004 && pPrev && pPrev->iCmd == 3003 && pPrev2 && pPrev2->iCmd == 2 &&
        fVal == pPrev2->fVal
    ) {
        pPrev2->iCmd = 10;
        pPrev2->u.iTrait = static_cast<int32_t>(pPrev2->fVal);
        pPrev2->fVal = pLast->fVal;
        pPrev2->xStr.Set(pPrev->xStr);
        ThrowAway(xList.RemoveTail());
        ThrowAway(xList.RemoveTail());
        ThrowAway(pItem);
    }
    else {
        if ((pItem->u.iTrait >= 4001 && pItem->u.iTrait <= 4125) || pItem->u.iTrait >= 10000) {
            if (pItem->iCmd == 0) {
                pItem->iCmd = 4;
            }
            else if (pItem->iCmd == 1) {
                pItem->iCmd = 5;
            }
        }
        else if (pItem->u.iTrait >= 2000 && pItem->u.iTrait <= 2025) {
            if (pItem->iCmd == 0) {
                pItem->iCmd = 2;
            }
            else if (pItem->iCmd == 1 && pLast) {
                pItem->iCmd = 3;
            }
        }
        else if (pItem->iCmd == 1 && pItem->fVal >= 901.0 && pItem->fVal <= 908.0) {
            pItem->iCmd = 7;
        }
        // SWEEP-INLINE 0x43A010
        NiTListItem<TileTemplateItem*>* pNode = xList.NewItem();
        pNode->m_element = pItem;
        xList.AddNodeTail(pNode);
    }
}

void Tile::TileTemplate::Clear() {
    while (xList.m_uiCount) {
        Tile::TileTemplateItem* pItem = xList.RemoveHead();
        if (pItem) {
            pItem->xStr.Set(nullptr, 0);
            MemoryManager_Singleton.Deallocate(pItem);
        }
    }
}

Tile::TileTemplateItem*
Tile::TileTemplate::GetItem(int32_t aiCmd, float afVal, const char* apStr, int32_t aiTA, int32_t aiLine) {
    Tile::TileTemplateItem* pitem;
    if (Tile::TileTemplate::iNumItems) {
        pitem = Tile::TileTemplate::pItemsA[--Tile::TileTemplate::iNumItems];
        pitem->iCmd = aiCmd;
        pitem->fVal = afVal;
        pitem->xStr.Set(apStr, 0);
        pitem->u.iTrait = aiTA;
        pitem->iLine = aiLine;
    }
    else {
        Tile::TileTemplateItem* apMemory =
            static_cast<Tile::TileTemplateItem*>(MemoryManager_Singleton.Allocate(sizeof(Tile::TileTemplateItem)));
        if (apMemory) {
            return ::new (apMemory) Tile::TileTemplateItem(aiCmd, afVal, apStr, aiTA, aiLine);
        }

        return nullptr;
    }
    return pitem;
}

void Tile::TileTemplate::ThrowAway(Tile::TileTemplateItem* apItem) {
    // The compare is unsigned: the array holds 10 items before one is freed instead of stored.
    if (static_cast<uint32_t>(Tile::TileTemplate::iNumItems) >= 10) {
        if (apItem) {
            apItem->xStr.Set(nullptr, 0);
            MemoryManager_Singleton.Deallocate(apItem);
        }
    }
    else {
        Tile::TileTemplate::pItemsA[Tile::TileTemplate::iNumItems++] = apItem;
    }
}

void Tile::TileTemplate::ClearAll() {
    while (Tile::TileTemplate::iNumItems--) {
        Tile::TileTemplateItem* pItem = Tile::TileTemplate::pItemsA[Tile::TileTemplate::iNumItems];
        if (pItem) {
            pItem->xStr.Set(nullptr, 0);
            MemoryManager_Singleton.Deallocate(pItem);
        }
    }
    Tile::TileTemplate::iNumItems = 0;
}

Tile::BuildStorage::BuildStorage()
  : pTemplate(new Tile::TileTemplate("main", this)), pCurrentTemplate(nullptr), bDeleteTemplates(true) {}

Tile::BuildStorage::~BuildStorage() {
    delete pTemplate;
    pCurrentTemplate = nullptr;
    if (bDeleteTemplates) {
        for (Tile::TileTemplate* m_item : xSubTemplates) {
            delete m_item;
        }
    }
}

// Sub-templates are matched by name: first on the interned handle, then on the contents when both are set.
Tile::TileTemplate* Tile::BuildStorage::GetSubTemplateByName(const char* pName) {
    NiGlobalStringTable::GlobalStringHandle handle;
    if (!pName) {
        handle = NiGlobalStringTable::NULL_STRING;
    }
    else {
        handle = NiGlobalStringTable::AddString(pName);
    }
    for (BSSimpleList<TileTemplate*>* node = &this->xSubTemplates; node && node->m_item; node = node->m_pkNext) {
        NiGlobalStringTable::GlobalStringHandle name = node->m_item->xName.GetHandle();
        if (handle == name || (name && handle && std::strcmp(handle, name) == 0)) {
            TileTemplate* found = node->m_item;
            // Drops the handle taken here, but never the table's empty one: a NULL_STRING compare, then
            // InterlockedDecrement on the reference count at handle - 8.
            if (handle != NiGlobalStringTable::NULL_STRING) {
                InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(handle) - 8));
            }
            return found;
        }
    }
    if (handle != NiGlobalStringTable::NULL_STRING) {
        InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(handle) - 8));
    }
    return nullptr;
}

Tile::TileTemplate* Tile::BuildStorage::NewSubTemplate(const char* pName) {
    Tile::TileTemplate* ptemplate;
    Tile::TileTemplate* apMemory =
        static_cast<Tile::TileTemplate*>(MemoryManager_Singleton.Allocate(sizeof(Tile::TileTemplate)));
    if (apMemory) {
        ptemplate = ::new (apMemory) Tile::TileTemplate(pName, this);
    }
    else {
        ptemplate = nullptr;
    }
    xSubTemplates.AddTail(ptemplate);
    return ptemplate;
}

Tile::TextureAtlasEntry::TextureAtlasEntry() : SubRect{0.0f, 0.0f, 0.0f, 0.0f} {}

Tile::TextureAtlasEntry::~TextureAtlasEntry() {}

Tile::Tile() : xTraits(16u, 0), pParent(nullptr), spModel(nullptr), uiFlags(0), bNeedsNiUpdate(false) {}

Tile::~Tile() {
    EnterCriticalSection(&Tile::TileUpdateAllCritSect.CriticalSection);
    Interface::TileIsBeingDeleted(this);
    if ((uiFlags & RELEASED) == 0) {
        Warning("MENUS: Base tile should have been released before deleted.");
        Release();
    }
    if ((uiFlags & DIRTY) != 0) {
        Tile* pTile = this;
        Tile::xDirtyTiles.Remove(pTile);
    }
    uiFlags &= ~DIRTY;
    Tile* pHibernating = this;
    if ((Tile::xHibernatingTiles.Remove(pHibernating)) && ((uiFlags & HIBERNATED) != 0)) {
        --Tile::iHibernatingTileCount;
    }

    uiFlags &= ~HIBERNATED;
    uiFlags &= ~PROMOTED;
    NiObjectGarbage::Add(spModel.m_pObject);
    LeaveCriticalSection(&Tile::TileUpdateAllCritSect.CriticalSection);
    // spModel, xName, xTraits and xChildren are released by their own destructors.
}

void Tile::Init(Tile* apParent, const char* apName, Tile* apReplacedTile) {
    this->pParent = nullptr;
    this->spModel = nullptr;
    this->xName.Set("", 0);
    this->bSpeechChallengeFailure = false;
    this->uiFlags = 0;
    if (apParent) {
        this->SetParent(apParent, apReplacedTile);
    }
    if (apName) {
        this->xName.Set(apName, 0);
    }
}

void Tile::ForceTextureRelease() {
    NiNode* model = this->spModel.m_pObject;
    if (model) {
        NiAVObject* first = model->m_kChildren.m_usSize > 0 ? model->m_kChildren.m_pBase[0].m_pObject : nullptr;
        if (this->GetType() == kTileValue_Image && first) {
            TileShaderProperty* property = static_cast<TileShaderProperty*>(first->GetProperty(NiProperty::SHADE));
            if (property) {
                property->SetTileTexture(nullptr);
                this->AddNeedsUpdate(CHANGE_TEXTURE);
            }
        }
    }
    // Children that are menus of their own release their textures themselves.
    NiTListItem<Tile*>* position = this->xChildren.m_pkHead;
    while (position) {
        Tile* child = this->xChildren.GetNext(position);
        if (!child->GetMenu()) {
            child->ForceTextureRelease();
        }
    }
}

TileShaderProperty* Tile::GetShaderProperty() {
    TileShaderProperty* property = nullptr;
    NiNode* model = this->spModel.m_pObject;
    NiAVObject* first = nullptr;
    if (model) {
        first = model->m_kChildren.m_usSize > 0 ? model->m_kChildren.m_pBase[0].m_pObject : nullptr;
    }
    if (first) {
        property = static_cast<TileShaderProperty*>(first->GetProperty(NiProperty::SHADE));
    }
    return property;
}

void Tile::SetAlphaAndColor(NiNode* apNode, float afAlpha, const NiColorA& aColor) {
    if (!apNode) {
        return;
    }
    for (uint32_t i = 0; i < apNode->m_kChildren.m_usSize; ++i) {
        NiAVObject* child = apNode->m_kChildren.m_pBase[i].m_pObject;
        NiGeometry* geometry = child ? child->IsGeometry() : nullptr;
        if (geometry) {
            // Every geometry of a tile carries a tile shader property.
            TileShaderProperty* property = static_cast<TileShaderProperty*>(geometry->GetProperty(NiProperty::SHADE));
            property->fAlpha = afAlpha;
            property->TintColor = aColor;
        }
    }
}

// The string is copied into the tile's own buffer.
void Tile::SetName(const char* apName) {
    this->xName = apName;
}

void Tile::SetName(const BSStringT<char>& arName) {
    xName.Copy(arName);
}

const char* Tile::GetName() {
    return xName.pString;
}

NiNode* Tile::GetModel() {
    return this->spModel;
}

bool Tile::IsReleased() {
    return (this->uiFlags & RELEASED) != 0;
}

void Tile::ClearNeedsNiUpdate() {
    this->bNeedsNiUpdate = false;
}

void Tile::AddNeedsUpdate(int32_t aiFlag) {
    if (aiFlag <= CHANGE_MASK) {
        const uint32_t oldFlags = this->uiFlags;
        this->uiFlags = oldFlags | aiFlag;
        if (this->uiFlags != oldFlags) {
            Tile::AddDirtyTile(this);
        }
    }
}

void Tile::RecursiveColorUpdate(int32_t aiColorID, float afInheritedVal) {
    Tile::Lock();
    float fColor = this->GetFloat(kTileValue_SystemColor);
    if (fColor == 0.0) {
        fColor = afInheritedVal;
    }
    // The id and the float are compared as doubles, so neither side is rounded.
    if (static_cast<double>(fColor) == static_cast<double>(aiColorID)) {
        this->AddNeedsUpdate(CHANGE_COLOR);
    }
    // EXE-INLINED none: NiTPointerListBase<Tile*>::GetHeadPos / GetNext
    NiTListItem<Tile*>* pPos = this->xChildren.m_pkHead;
    while (pPos) {
        Tile** ppChild = &pPos->m_element;
        pPos = pPos->m_pkNext;
        (*ppChild)->RecursiveColorUpdate(aiColorID, fColor);
    }
    Tile::Unlock();
}

void Tile::RecursiveColorUpdate(BSSimpleList<int32_t>* pColorIDList, float afInheritedVal) {
    Tile::Lock();
    float fColor = this->GetFloat(kTileValue_SystemColor);
    if (fColor == 0.0) {
        fColor = afInheritedVal;
    }
    const int32_t iColor = static_cast<int32_t>(fColor);
    // EXE-INLINED none: BSSimpleList<int>::IsInList
    BSSimpleList<int32_t>* pIter = pColorIDList;
    while (pIter && pIter->m_item != iColor) {
        pIter = pIter->m_pkNext;
    }
    if (pIter) {
        this->AddNeedsUpdate(CHANGE_COLOR);
    }
    // EXE-INLINED none: NiTPointerListBase<Tile*>::GetHeadPos / GetNext
    NiTListItem<Tile*>* pPos = this->xChildren.m_pkHead;
    while (pPos) {
        Tile** ppChild = &pPos->m_element;
        pPos = pPos->m_pkNext;
        (*ppChild)->RecursiveColorUpdate(pColorIDList, fColor);
    }
    Tile::Unlock();
}

void Tile::UpdateChildren(bool) {
    // Rebuilds every tile's geometry from the ground up.
    Tile::UpdateAll(false);
}

bool Tile::GetRenderedMenu() {
    return this->GetMenu()->bRenderedMenu;
}

NiNode* Tile::GetParentModel() {
    Tile* current = this;
    while (current && !static_cast<NiNode*>(current->spModel)) {
        current = current->pParent;
    }
    return current ? static_cast<NiNode*>(current->spModel) : nullptr;
}

bool Tile::IsVisible() {
    // Walks the models toward the root; any culled model hides the tile.
    for (NiAVObject* model = static_cast<NiNode*>(this->spModel); model; model = model->m_pkParent) {
        if ((model->m_uFlags & NiAVObject::APP_CULLED) != 0) {
            return false;
        }
    }
    return true;
}

// The tile lock is held across the lookup and released on both returns.
Menu* Tile::GetMenu() {
    Tile::Lock();
    Tile* menuTile = this->GetMenuTile();
    if (menuTile && menuTile->GetType() == kTileValue_Menu) {
        Menu* menu = static_cast<TileMenu*>(menuTile)->pMenu;
        Tile::Unlock();
        return menu;
    }
    Tile::Unlock();
    return nullptr;
}

Tile* Tile::GetParent() {
    return this->pParent;
}

Tile* Tile::GetMenuTile() {
    Tile::Lock();
    Tile* current = this;
    while (current && current->pParent && current->pParent->pParent) {
        current = current->pParent;
    }
    Tile::Unlock();
    return current;
}

// The child list is walked under the tile lock, which is released on both returns.
Tile* Tile::GetChildByName(const char* apName) {
    Tile::Lock();
    NiTListItem<Tile*>* position = this->xChildren.m_pkHead;
    while (position && apName) {
        Tile* child = this->xChildren.GetNext(position);
        if (child->xName.pString && !std::strcmp(child->xName.pString, apName)) {
            Tile::Unlock();
            return child;
        }
        Tile* found = child->GetChildByName(apName);
        if (found) {
            Tile::Unlock();
            return found;
        }
    }
    Tile::Unlock();
    return nullptr;
}

// The child list is walked under the tile lock, which is released on both returns.
Tile* Tile::GetChildByID(int32_t aiID) {
    Tile::Lock();
    NiTListItem<Tile*>* position = this->xChildren.m_pkHead;
    while (position) {
        // The child is read before the node moves on, as the game does.
        Tile** child = &position->m_element;
        position = position->m_pkNext;
        Tile* tile = *child;
        // The id is converted to float and compared with the returned float for equality only; a NaN never matches.
        if (aiID == tile->GetFloat(kTileValue_ID)) {
            Tile::Unlock();
            return tile;
        }
    }
    Tile::Unlock();
    return nullptr;
}

// The trait's actions are searched for a reference to another tile's trait here, else in the parent chain, under the
// tile lock.
Tile* Tile::GetFirstRefCopy(int32_t aiTrait, int32_t& aiType) {
    Tile::Lock();
    Value* value = this->GetValueQ(aiTrait);
    Tile* result = nullptr;
    if (value) {
        Action* action = static_cast<Action*>(value->pActionListA);
        bool found = false;
        while (action && !found) {
            // The referenced value is asked for three times, as the game does: for the test, its tile, its trait.
            if (action->eActionType == kTileValue_Ref && action->QRefValue()) {
                result = action->QRefValue()->pParent;
                aiType = action->QRefValue()->eIndex;
                found = true;
            }
            action = action->pnext;
        }
    }
    else if (this->pParent) {
        Tile* parent = this->pParent->GetFirstRefCopy(aiTrait, aiType);
        Tile::Unlock();
        return parent;
    }
    Tile::Unlock();
    return result;
}

void Tile::DeleteChildren() {
    Tile::Lock();
    NiTListItem<Tile*>* position = this->xChildren.m_pkHead;
    while (position) {
        Tile* child = this->xChildren.GetNext(position);
        if (child) {
            delete child;
        }
    }
    this->xChildren.RemoveAll();
    Tile::Unlock();
}

// Applies the tile's pending trait changes to its Gamebryo model, one CHANGE_* bit at a time, under the tile tree's
// lock and the interface heap context; abClearUpdateFlags clears each bit it handled. Returns true when anything was
// updated. Leaves the tile on the hibernating list when bits remain to clear, otherwise counts it as updated.
bool Tile::UpdateTile(bool abClearUpdateFlags) {
    EnterCriticalSection(reinterpret_cast<_RTL_CRITICAL_SECTION*>(&Tile::TileUpdateAllCritSect));
    bool bUpdated = false;
    if ((this->uiFlags & CHANGE_MASK) != 0) {
        AutoMemContext kMemContext(MEM_CONTEXT_INTERFACE);

        if ((this->uiFlags & CHANGE_CREATE) != 0) {
            this->DeleteModel();
            if (this->MakeNode()) {
                if (this->GetType() == kTileValue_Text) {
                    Interface::SetTextWasCreated(true);
                }
                bUpdated = true;
                if (abClearUpdateFlags && (this->uiFlags & CHANGE_CREATE) != 0) {
                    this->uiFlags ^= CHANGE_CREATE;
                }
            }
        }

        if ((this->uiFlags & CHANGE_VISIBILITY) != 0 && this->spModel) {
            const bool bWasCulled = (this->spModel->m_uFlags & NiAVObject::APP_CULLED) != 0;
            const bool bHide = this->GetFloat(kTileValue_Visible) == 0.0;
            NiNode* pModel = this->spModel;
            if (bHide) {
                pModel->m_uFlags = static_cast<NiAVObject::NiFlags>(pModel->m_uFlags | NiAVObject::APP_CULLED);
            }
            else {
                pModel->m_uFlags = static_cast<NiAVObject::NiFlags>(pModel->m_uFlags & ~NiAVObject::APP_CULLED);
            }
            bUpdated = true;
            // A tile that was hidden and is shown again may wake hibernating tiles.
            if (bWasCulled && bWasCulled != ((this->spModel->m_uFlags & NiAVObject::APP_CULLED) != 0)) {
                Tile::bNeedsCheckHibernate = true;
            }
            if (abClearUpdateFlags && (this->uiFlags & CHANGE_VISIBILITY) != 0) {
                this->uiFlags ^= CHANGE_VISIBILITY;
            }
        }

        // On a first-chance texture release, an image drops its texture and asks for it again.
        if (Interface::GetFirstChanceTextureRelease() && this->spModel) {
            NiNode* pModel = this->spModel;
            NiAVObject* pChild = pModel->m_kChildren.m_usSize > 0 ? pModel->m_kChildren.m_pBase[0].m_pObject : nullptr;
            if (this->GetType() == kTileValue_Image && pChild) {
                auto* pShader = static_cast<TileShaderProperty*>(pChild->GetProperty(NiProperty::SHADE));
                if (pShader) {
                    pShader->SetTileTexture(nullptr);
                    this->AddNeedsUpdate(CHANGE_TEXTURE);
                }
            }
        }

        const bool bIsImage = this->GetType() == kTileValue_Image || this->GetType() == kTileValue_Hotrect;
        // The atlas rectangle of the texture loaded below; -1 while no atlas was read.
        NiRect<float> kAtlasRect;
        kAtlasRect.m_bottom = -1.0f;
        kAtlasRect.m_left = -1.0f;
        kAtlasRect.m_right = -1.0f;
        kAtlasRect.m_top = -1.0f;

        if (bIsImage && (this->uiFlags & CHANGE_TEXTURE) != 0 && this->spModel) {
            NiNode* pModel = this->spModel;
            NiAVObject* pChild = pModel->m_kChildren.m_usSize > 0 ? pModel->m_kChildren.m_pBase[0].m_pObject : nullptr;
            if (bIsImage && pChild && ((this->uiFlags & PROMOTED) != 0 || this->IsVisible())) {
                auto* pShader = static_cast<TileShaderProperty*>(pChild->GetProperty(NiProperty::SHADE));
                if (pShader) {
                    TileImage* pImage = static_cast<TileImage*>(this);
                    NiPointer<NiTexture> spTexture;
                    Value* pFilenameValue = this->GetValueQ(kTileValue_Filename);
                    void* pActions = nullptr;
                    const char* pFilename = nullptr;
                    bool bEmpty = false;
                    if (pFilenameValue) {
                        pActions = pFilenameValue->pActionListA;
                        pFilename = pFilenameValue->strValue;
                    }
                    if ((!pFilename || !*pFilename) && !pActions) {
                        this->Set(kTileValue_Filename, "Interface\\Shared\\empty.dds", true);
                        pFilename = this->GetString(kTileValue_Filename);
                        bEmpty = true;
                    }
                    if (pFilename && *pFilename) {
                        if (bEmpty || this->GetValueQ(kTileValue_Texatlas)) {
                            // The atlas's own texture file, filled by GetTextureAtlasInfo.
                            alignas(BSStringT<char>) uint8_t aAtlasPathStorage[sizeof(BSStringT<char>)];
                            BSStringT<char>* pAtlasPath = ::new (aAtlasPathStorage) BSStringT<char>("");
                            const char* pAtlas =
                                bEmpty ? sDefaultTexAtlas.uValue.str : this->GetString(kTileValue_Texatlas);
                            Tile::GetTextureAtlasInfo(
                                pAtlas, this->GetString(kTileValue_Filename), pAtlasPath, &kAtlasRect
                            );
                            // The by-value texture is copied (a reference taken) before the zoom is read.
                            TileImage::AddToTesTextures(
                                pAtlasPath->pString, pImage->fScale, spTexture, pImage->GetFloat(kTileValue_Zoom),
                                pImage->spTexture.m_pObject
                            );
                            // The string's destructor.
                            pAtlasPath->Set(nullptr, 0);
                        }
                        if (!spTexture) {
                            TileImage::AddToTesTextures(
                                pFilename, pImage->fScale, spTexture, pImage->GetFloat(kTileValue_Zoom),
                                pImage->spTexture.m_pObject
                            );
                        }
                    }
                    if (spTexture) {
                        pShader->SetTileTexture(spTexture);
                        if (this->IsTrue(kTileValue_Tile)) {
                            pShader->TexClampMode = NiTexturingProperty::WRAP_S_CLAMP_T;
                        }
                        else {
                            pShader->TexClampMode = NiTexturingProperty::CLAMP_S_CLAMP_T;
                        }
                        float fZoom = this->GetFloat(kTileValue_Zoom);
                        const char* pName = this->GetString(kTileValue_Filename);
                        const float fFileWidth = static_cast<float>(spTexture->GetWidth());
                        const float fFileHeight = static_cast<float>(spTexture->GetHeight());
                        // A named texture is sized in tile units; a blank name (" ") keeps its pixel size.
                        const bool bNamed = pName && *pName && (*pName != ' ' || pName[1]);
                        if (this->GetFloat(kTileValue_Tile) == -1.0) {
                            if (bNamed) {
                                this->Set(kTileValue_FileWidth, fFileWidth / pImage->fScale, true);
                                this->Set(kTileValue_FileHeight, fFileHeight / pImage->fScale, true);
                            }
                            else {
                                this->Set(kTileValue_FileWidth, fFileWidth, true);
                                this->Set(kTileValue_FileHeight, fFileHeight, true);
                            }
                            this->AddNeedsUpdate(CHANGE_GEOMETRY);
                            fZoom = -1.0f;
                        }
                        if (fZoom >= 0.0) {
                            if (fZoom == 0.0) {
                                fZoom = 100.0f;
                            }
                            if (bNamed) {
                                this->Set(kTileValue_FileWidth, fFileWidth * (fZoom / 100.0) / pImage->fScale, true);
                                this->Set(kTileValue_FileHeight, fFileHeight * (fZoom / 100.0) / pImage->fScale, true);
                            }
                            else {
                                this->Set(kTileValue_FileWidth, fFileWidth * (fZoom / 100.0), true);
                                this->Set(kTileValue_FileHeight, fFileHeight * (fZoom / 100.0), true);
                            }
                            this->AddNeedsUpdate(CHANGE_GEOMETRY);
                        }
                        bUpdated = true;
                        if (abClearUpdateFlags && (this->uiFlags & CHANGE_TEXTURE) != 0) {
                            this->uiFlags ^= CHANGE_TEXTURE;
                        }
                    }
                    else if (
                        abClearUpdateFlags && !this->IsTrue(kTileValue_Visible) && (this->uiFlags & CHANGE_TEXTURE) != 0
                    ) {
                        this->uiFlags ^= CHANGE_TEXTURE;
                    }
                }
            }
        }

        // Geometry data the renderer holds as static is not rewritten.
        bool bStaticData = false;
        if ((this->uiFlags & CHANGE_GEOMETRY) != 0 && this->spModel) {
            NiNode* pModel = this->spModel;
            NiAVObject* pChild = pModel->m_kChildren.m_usSize > 0 ? pModel->m_kChildren.m_pBase[0].m_pObject : nullptr;
            NiTriBasedGeom* pTriGeom = pChild ? pChild->IsTriBasedGeom() : nullptr;
            NiGeometryData* pData = pTriGeom ? pTriGeom->m_spModelData.m_pObject : nullptr;
            if (pData && (pData->m_usDirtyFlags & NiGeometryData::CONSISTENCY_MASK) == NiGeometryData::STATIC) {
                bStaticData = true;
            }
        }

        if ((this->uiFlags & CHANGE_GEOMETRY) != 0 && this->spModel) {
            NiNode* pModel = this->spModel;
            NiAVObject* pChild = pModel->m_kChildren.m_usSize > 0 ? pModel->m_kChildren.m_pBase[0].m_pObject : nullptr;
            const float fWidth = this->GetFloat(kTileValue_Width);
            const float fHeight = this->GetFloat(kTileValue_Height);
            const float fCropX = this->GetFloat(kTileValue_CropOffsetX);
            const float fCropY = this->GetFloat(kTileValue_CropOffsetY);
            // The first child of an image's model is its quad.
            NiGeometry* pGeom = static_cast<NiGeometry*>(pChild);
            if (pGeom && this->GetType() == kTileValue_Image) {
                if (!bStaticData && pGeom->m_spModelData->m_pkVertex) {
                    NiPoint3* pVertex = pGeom->m_spModelData->m_pkVertex;
                    pVertex[0].x = 0.0f;
                    pVertex[0].y = 0.0f;
                    pVertex[0].z = 0.0f;
                    pVertex[1].x = 0.0f;
                    pVertex[1].y = 0.0f;
                    pVertex[1].z = -fHeight;
                    pVertex[2].x = fWidth;
                    pVertex[2].y = 0.0f;
                    pVertex[2].z = 0.0f;
                    pVertex[3].x = fWidth;
                    pVertex[3].y = 0.0f;
                    pVertex[3].z = -fHeight;
                }
                // The size one whole texture covers, in tile units.
                float fTextureWidth = fWidth;
                float fTextureHeight = fHeight;
                auto* pShader = static_cast<TileShaderProperty*>(pChild->GetProperty(NiProperty::SHADE));
                if (pShader && bIsImage && pShader->spTileTexture) {
                    if (this->GetFloat(kTileValue_Tile) == 0.0) {
                        pShader->TexClampMode = NiTexturingProperty::CLAMP_S_CLAMP_T;
                    }
                    else {
                        pShader->TexClampMode = NiTexturingProperty::WRAP_S_CLAMP_T;
                    }
                    float fZoom = this->GetFloat(kTileValue_Zoom);
                    if (this->GetFloat(kTileValue_Tile) == -1.0) {
                        // Tiled to the height: the width keeps the texture's aspect ratio.
                        NiTexture* pTexture = pShader->spTileTexture;
                        const double dRatio = fHeight / static_cast<double>(pTexture->GetHeight());
                        fTextureWidth = static_cast<float>(pTexture->GetWidth() * dRatio);
                    }
                    else if (fZoom >= 0.0) {
                        if (fZoom == 0.0) {
                            fZoom = 100.0f;
                        }
                        const char* pName = this->GetString(kTileValue_Filename);
                        if (pName && *pName && (*pName != ' ' || pName[1])) {
                            TileImage* pImage = static_cast<TileImage*>(this);
                            NiTexture* pTexture = pShader->spTileTexture;
                            float fScale = pImage->fScale;
                            fTextureWidth = static_cast<float>(pTexture->GetWidth() * (fZoom / 100.0) / fScale);
                            pTexture = pShader->spTileTexture;
                            fScale = pImage->fScale;
                            fTextureHeight = static_cast<float>(pTexture->GetHeight() * (fZoom / 100.0) / fScale);
                        }
                        else {
                            fTextureWidth = static_cast<float>(pShader->spTileTexture->GetWidth() * (fZoom / 100.0));
                            fTextureHeight = static_cast<float>(pShader->spTileTexture->GetHeight() * (fZoom / 100.0));
                        }
                    }
                    else {
                        fTextureWidth = fWidth;
                        fTextureHeight = fHeight;
                    }
                }

                NiPoint2* pTexCoord = pGeom->m_spModelData->m_pkTexture;
                if (pTexCoord) {
                    float fU = fCropX / fTextureWidth;
                    float fV = fCropY / fTextureHeight;
                    float fUSize = fWidth / fTextureWidth;
                    float fVSize = fHeight / fTextureHeight;
                    if (this->GetValueQ(kTileValue_Texatlas)) {
                        // Map the crop into the texture's rectangle inside the atlas.
                        NiRect<float> kSubRect;
                        kSubRect.m_left = 0.0f;
                        kSubRect.m_right = 0.0f;
                        kSubRect.m_top = 0.0f;
                        kSubRect.m_bottom = 0.0f;
                        if (kAtlasRect.m_right == -1.0 || kAtlasRect.m_bottom == -1.0) {
                            const char* pFilename = this->GetString(kTileValue_Filename);
                            const char* pAtlas = this->GetString(kTileValue_Texatlas);
                            Tile::GetTextureAtlasInfo(pAtlas, pFilename, nullptr, &kSubRect);
                        }
                        else {
                            kSubRect = kAtlasRect;
                        }
                        const float fSubWidth = kSubRect.m_right - kSubRect.m_left;
                        const float fSubHeight = kSubRect.m_bottom - kSubRect.m_top;
                        if (fSubWidth > 0.0 && fSubHeight > 0.0) {
                            fU = fU * fSubWidth + kSubRect.m_left;
                            fV = fV * fSubHeight + kSubRect.m_top;
                            fUSize = fSubWidth;
                            fVSize = fSubHeight;
                        }
                        float fAtlasZoom = this->GetFloat(kTileValue_Zoom);
                        if (fAtlasZoom > 0.0) {
                            fAtlasZoom = fAtlasZoom / 100.0;
                            const double dWidth = this->GetFloat(kTileValue_Width);
                            fUSize = static_cast<float>(dWidth / this->GetFloat(kTileValue_FileWidth) * fAtlasZoom);
                            const double dHeight = this->GetFloat(kTileValue_Height);
                            fVSize = static_cast<float>(dHeight / this->GetFloat(kTileValue_FileHeight) * fAtlasZoom);
                        }
                    }
                    pTexCoord[0].x = fU;
                    pTexCoord[0].y = fV;
                    pTexCoord[1].x = fU;
                    pTexCoord[1].y = fVSize + fV;
                    pTexCoord[2].x = fUSize + fU;
                    pTexCoord[2].y = fV;
                    pTexCoord[3].x = fUSize + fU;
                    pTexCoord[3].y = fVSize + fV;
                }

                if (!bStaticData) {
                    if (pGeom->m_spModelData) {
                        pGeom->m_spModelData->MarkAsChanged(NiGeometryData::VERTEX_MASK | NiGeometryData::TEXTURE_MASK);
                    }
                    NiPoint3* pVertex = pGeom->m_spModelData->m_pkVertex;
                    const uint16_t usVertices = pGeom->m_spModelData->m_usVertices;
                    pGeom->m_spModelData->m_kBound.ComputeFromData(usVertices, pVertex);
                }
                bUpdated = true;
                NiGeometryData* pData = pGeom->m_spModelData;
                if (pData) {
                    pData->SetConsistency(NiGeometryData::MUTABLE);
                }
            }
            if (abClearUpdateFlags && (this->uiFlags & CHANGE_GEOMETRY) != 0) {
                this->uiFlags ^= CHANGE_GEOMETRY;
            }
        }

        if ((this->uiFlags & CHANGE_NIF_FILE) != 0 && this->spModel) {
            if (this->GetType() == kTileValue_Nif) {
                static_cast<Tile3D*>(this)->UpdateNIF();
            }
            bUpdated = true;
            if (abClearUpdateFlags && (this->uiFlags & CHANGE_NIF_FILE) != 0) {
                this->uiFlags ^= CHANGE_NIF_FILE;
            }
        }

        if ((this->uiFlags & CHANGE_POSITION) != 0 && this->spModel) {
            if (this->IsTrue(kTileValue_Locus) || this->pParent == Interface::GetMenusRoot()) {
                // A locus (or a menu) places its model; depth moves it toward the camera.
                const float fDepth = this->GetFloat(kTileValue_Depth);
                NiPoint3 kPosition = NiPoint3_ZERO;
                kPosition.x = this->GetFloat(kTileValue_X);
                kPosition.z = -this->GetFloat(kTileValue_Y);
                kPosition.y = fDepth * -0.00800000037997961;
                if (this->pParent == Interface::GetMenusRoot()) {
                    // Menus are placed relative to the screen centre.
                    kPosition.x = kPosition.x - Interface::GetScreenWidth() / 2.0;
                    const double dBottom = Interface::GetScreenHeight() + kPosition.z;
                    kPosition.z = dBottom - Interface::GetScreenHeight() / 2.0;
                }
                this->spModel->m_kLocal.m_Translate = kPosition;
                if ((this->uiFlags & CHANGE_LOCUS) != 0) {
                    NiNode* pModel = this->spModel;
                    for (uint32_t i = 0; pModel && i < pModel->m_kChildren.m_usESize; ++i) {
                        NiAVObject* pChild =
                            pModel->m_kChildren.m_usSize > i ? pModel->m_kChildren.m_pBase[i].m_pObject : nullptr;
                        NiTriShape* pShape = pChild ? pChild->IsTriShape() : nullptr;
                        if (pShape && (this->uiFlags & TRI_UPDATE_MANUAL) == 0) {
                            pShape->m_kLocal.m_Translate = NiPoint3_ZERO;
                        }
                    }
                }
                this->UpdateClipwindows();
            }
            else {
                const float fDepth = this->GetFloat(kTileValue_Depth);
                NiPoint3 kPosition = NiPoint3_ZERO;
                this->spModel->m_kLocal.m_Translate = kPosition;
                kPosition.x = this->GetFloat(kTileValue_X) + kPosition.x;
                kPosition.z = -this->GetFloat(kTileValue_Y) + kPosition.z;
                kPosition.y = fDepth * -0.00800000037997961 + kPosition.y;
                // Every tri shape child is rotated about the rotate axis and placed at the tile's position.
                NiNode* pModel = this->spModel;
                for (uint32_t i = 0; pModel && i < pModel->m_kChildren.m_usESize; ++i) {
                    NiAVObject* pChild = pModel->m_kChildren.m_pBase[i].m_pObject;
                    NiTriShape* pShape = nullptr;
                    if (pChild && pChild->IsKindOf(&NiTriShape::ms_RTTI)) {
                        pShape = static_cast<NiTriShape*>(pChild);
                    }
                    const float fAxisX = this->GetFloat(kTileValue_RotateAxisX);
                    const float fAxisZ = -this->GetFloat(kTileValue_RotateAxisY);
                    NiPoint3 kAxis;
                    kAxis.x = fAxisX;
                    kAxis.y = 0.0f;
                    kAxis.z = fAxisZ;
                    const float fAngle = this->GetFloat(kTileValue_RotateAngle);
                    const float fSin = std::sin(fAngle);
                    const float fCos = std::cos(fAngle);
                    NiMatrix3 kRotation;
                    kRotation.m_pEntry[0][0] = fCos;
                    kRotation.m_pEntry[0][1] = 0.0f;
                    kRotation.m_pEntry[0][2] = -fSin;
                    kRotation.m_pEntry[1][0] = 0.0f;
                    kRotation.m_pEntry[1][1] = 1.0f;
                    kRotation.m_pEntry[1][2] = 0.0f;
                    kRotation.m_pEntry[2][0] = fSin;
                    kRotation.m_pEntry[2][1] = 0.0f;
                    kRotation.m_pEntry[2][2] = fCos;
                    if (pShape) {
                        pShape->m_kLocal.m_Rotate = kRotation;
                        const NiPoint3 kRotatedAxis = kRotation * kAxis;
                        const NiPoint3 kShifted = kPosition + kAxis;
                        pShape->m_kLocal.m_Translate = kShifted - kRotatedAxis;
                    }
                }
            }
            bUpdated = true;
            if (abClearUpdateFlags && (this->uiFlags & CHANGE_POSITION) != 0) {
                this->uiFlags ^= CHANGE_POSITION;
            }
        }
        if (abClearUpdateFlags && (this->uiFlags & CHANGE_LOCUS) != 0) {
            this->uiFlags ^= CHANGE_LOCUS;
        }

        if (this->spModel && (this->uiFlags & CHANGE_SCISSOR_WINDOW) != 0) {
            // The tile's own rectangle, in real screen pixels, clips its descendants.
            float fScreenWidth = static_cast<float>(Interface::GetRealScreenWidth());
            float fScreenHeight = Interface::GetRealScreenHeight();
            if (this->GetRenderedMenu()) {
                fScreenWidth = Interface::GetRenderedMenuScreenWidth();
                fScreenHeight = Interface::GetRenderedMenuScreenHeight();
            }
            float fScaleX = (fScreenWidth / Interface::GetScreenWidth());
            const float fScaleY = (fScreenHeight / Interface::GetScreenHeight());
            if (Menu::ShouldScaleByResolutionRatio(this->GetMenu())) {
                fScaleX = Interface::GetAspectRatioRatio() * fScaleX;
            }
            float fLeft = this->GetRealX();
            float fTop = this->GetRealY();
            const float fRight = (this->GetFloat(kTileValue_Width) + fLeft) * fScaleX;
            const float fBottom = (this->GetFloat(kTileValue_Height) + fTop) * fScaleY;
            fLeft = fLeft * fScaleX;
            fTop = fTop * fScaleY;
            this->ReClipChildren(fLeft, fTop, fRight, fBottom);
            bUpdated = true;
            if (abClearUpdateFlags && (this->uiFlags & CHANGE_SCISSOR_WINDOW) != 0) {
                this->uiFlags ^= CHANGE_SCISSOR_WINDOW;
            }
        }

        if (this->spModel && (this->uiFlags & CHANGE_SCISSOR) != 0) {
            // The nearest clip window above the tile scissors every child's shader.
            Tile* pClipWindow = this->pParent;
            while (pClipWindow && !pClipWindow->IsTrue(kTileValue_ClipWindow)) {
                pClipWindow = pClipWindow->pParent;
            }
            if (pClipWindow) {
                float fScreenWidth = static_cast<float>(Interface::GetRealScreenWidth());
                float fScreenHeight = Interface::GetRealScreenHeight();
                if (this->GetRenderedMenu()) {
                    fScreenWidth = Interface::GetRenderedMenuScreenWidth();
                    fScreenHeight = Interface::GetRenderedMenuScreenHeight();
                }
                float fScaleX = (fScreenWidth / Interface::GetScreenWidth());
                const float fScaleY = (fScreenHeight / Interface::GetScreenHeight());
                if (Menu::ShouldScaleByResolutionRatio(this->GetMenu())) {
                    fScaleX = Interface::GetAspectRatioRatio() * fScaleX;
                }
                // Rounded through float slots as above.
                float fLeft = pClipWindow->GetRealX();
                float fTop = pClipWindow->GetRealY();
                const float fRight = (pClipWindow->GetFloat(kTileValue_Width) + fLeft) * fScaleX;
                const float fBottom = (pClipWindow->GetFloat(kTileValue_Height) + fTop) * fScaleY;
                fLeft = fLeft * fScaleX;
                fTop = fTop * fScaleY;
                // EXE-INLINED 0x4ADD70
                NiNode* pModel = this->spModel.m_pObject;
                NiAVObjectArray_Compact(&pModel->m_kChildren);
                NiAVObjectArray_UpdateSize(&pModel->m_kChildren);
                for (uint32_t i = 0; i < this->spModel->m_kChildren.m_usESize; ++i) {
                    NiAVObject* pChild = this->spModel->m_kChildren.m_pBase[i].m_pObject;
                    auto* pShader = static_cast<TileShaderProperty*>(pChild->GetProperty(NiProperty::SHADE));
                    if (pShader) {
                        // Clamped to the screen; a NaN edge clamps too.
                        const float fClampedLeft = fLeft > 0.0f ? fLeft : 0.0f;
                        const int32_t iLeft = static_cast<int32_t>(fClampedLeft);
                        const float fClampedTop = fTop > 0.0f ? fTop : 0.0f;
                        const int32_t iTop = static_cast<int32_t>(fClampedTop);
                        const float fClampedRight = fScreenWidth > fRight ? fRight : fScreenWidth;
                        const int32_t iRight = static_cast<int32_t>(fClampedRight);
                        const float fClampedBottom = fScreenHeight > fBottom ? fBottom : fScreenHeight;
                        const int32_t iBottom = static_cast<int32_t>(fClampedBottom);
                        pShader->ScissorRect[0] = iLeft;
                        pShader->ScissorRect[1] = iTop;
                        pShader->ScissorRect[2] = iRight;
                        pShader->ScissorRect[3] = iBottom;
                        pShader->SetUseScissorTest(true);
                    }
                }
            }
            bUpdated = true;
            if (abClearUpdateFlags && (this->uiFlags & CHANGE_SCISSOR) != 0) {
                this->uiFlags ^= CHANGE_SCISSOR;
            }
        }

        if ((this->uiFlags & CHANGE_COLOR) != 0) {
            NiColorA kColor(1.0f, 1.0f, 1.0f, 1.0f);
            float fAlpha = static_cast<float>(this->GetFloat(kTileValue_Alpha) / 255.0);
            float fBrightness = this->GetFloat(kTileValue_Brightness);
            if (fBrightness >= 0.0) {
                fBrightness = static_cast<float>(fBrightness / 255.0);
            }
            // A non-negative brightness scales a system colour: the nearest one set on the tile or an ancestor,
            // stopping at the tile with id 111; a top-level tile with id 110 takes its own or its parent's colour,
            // or the HUD's (1).
            if (fBrightness >= 0.0) {
                int32_t iColorID = 0;
                Tile* pTile = this;
                while (pTile && !pTile->GetValueQ(kTileValue_SystemColor) && pTile->GetFloat(kTileValue_ID) != 111.0) {
                    if (pTile->pParent && pTile->pParent->pParent && !pTile->pParent->pParent->pParent &&
                        pTile->GetFloat(kTileValue_ID) == 110.0) {
                        iColorID = static_cast<int32_t>(pTile->GetFloat(kTileValue_SystemColor));
                        if (!iColorID) {
                            iColorID = static_cast<int32_t>(pTile->pParent->GetFloat(kTileValue_SystemColor));
                        }
                        if (!iColorID) {
                            iColorID = 1;
                        }
                        pTile = nullptr;
                        break;
                    }
                    pTile = pTile->pParent;
                }
                if (pTile) {
                    iColorID = static_cast<int32_t>(pTile->GetFloat(kTileValue_SystemColor));
                }
                if (SystemColorManager::GetInstance()->GetColor(iColorID, kColor)) {
                    kColor.Scale(fBrightness);
                }
                else {
                    fBrightness = -1.0f;
                }
            }
            if (fBrightness == -1.0) {
                kColor.r = static_cast<float>(this->GetFloat(kTileValue_Red) / 255.0);
                kColor.g = static_cast<float>(this->GetFloat(kTileValue_Green) / 255.0);
                kColor.b = static_cast<float>(this->GetFloat(kTileValue_Blue) / 255.0);
            }
            // A failed speech challenge paints the list box highlight red.
            if (this->bSpeechChallengeFailure) {
                if (!std::strcmp(this->xName.pString, "lb_highlight_box")) {
                    kColor.r = 0.545f;
                    kColor.g = 0.0f;
                    kColor.b = 0.0f;
                    fAlpha = 0.2f;
                }
                else if (this->pParent && !std::strcmp(this->pParent->xName.pString, "lb_highlight_box")) {
                    kColor.r = 0.545f;
                    kColor.g = 0.0f;
                    kColor.b = 0.0f;
                    fAlpha = 1.0f;
                }
            }
            this->SetAlphaAndColor(this->spModel, fAlpha, kColor);
            if (abClearUpdateFlags && (this->uiFlags & CHANGE_COLOR) != 0) {
                this->uiFlags ^= CHANGE_COLOR;
            }
        }
    }
    if (abClearUpdateFlags && (this->uiFlags & CHANGE_MASK) != 0) {
        Tile::AddHibernatingTile(this);
    }
    else {
        ++Tile::iTilesUpdatedThisFrame;
    }
    LeaveCriticalSection(reinterpret_cast<_RTL_CRITICAL_SECTION*>(&Tile::TileUpdateAllCritSect));
    return bUpdated;
}

Tile* Tile::BuildAndNameTree(TileTemplate* apTemplate) {
    Tile* parent = this;
    Tile* firstTile = nullptr;
    NiTListItem<TileTemplateItem*>* position = apTemplate->xList.m_pkHead;
    while (position) {
        TileTemplateItem* item = apTemplate->xList.GetNext(position);
        if (item->iCmd == 6) {
            Tile* tile = Tile::BuildTile(static_cast<int32_t>(item->fVal));
            if (!tile) {
                Warning("MENUS: Unable to create tile. Aborting menu creation.");
                return nullptr;
            }
            tile->Init(parent, item->xStr.pString, nullptr);
            // A hotrect (tile type 906) starts as an invisible, solid, untinted target.
            if (item->fVal == 906.0) {
                tile->Set(kTileValue_ID, -1.0f, true);
                tile->Set(kTileValue_Target, 1.0f, true);
                tile->Set(kTileValue_Brightness, -1.0f, true);
                tile->Set(kTileValue_Red, 255.0f, true);
                tile->Set(kTileValue_Green, 255.0f, true);
                tile->Set(kTileValue_Blue, 255.0f, true);
                tile->Set(kTileValue_Alpha, 0.0f, true);
                tile->Set(kTileValue_Filename, "solid.dds", true);
                tile->Set(kTileValue_Texatlas, sDefaultTexAtlas.uValue.str, true);
                tile->AddNeedsUpdate(CHANGE_TEXTURE);
            }
            if (!firstTile) {
                firstTile = tile;
            }
            item->u.pTile = tile;
            parent = tile;
        }
        else if (item->iCmd == 7) {
            parent = parent->pParent;
        }
    }
    return firstTile;
}

void Tile::ConnectTraitsToTree(TileTemplate* apTemplate) {
    Tile* tile = nullptr;
    int32_t action = 0;
    NiTListItem<TileTemplateItem*>* position = apTemplate->xList.m_pkHead;
    while (position) {
        TileTemplateItem* item = apTemplate->xList.GetNext(position);
        if (item->iCmd == 6) {
            tile = item->u.pTile;
            // CONNECTING_TRAITS is a flag above CHANGE_MASK, so AddNeedsUpdate refuses it, but the call is made anyway,
            // as the game does.
            tile->AddNeedsUpdate(CONNECTING_TRAITS);
        }
        else if (item->iCmd == 7) {
            if ((tile->uiFlags & CONNECTING_TRAITS) != 0) {
                tile->uiFlags ^= CONNECTING_TRAITS;
            }
            EnterCriticalSection(reinterpret_cast<_RTL_CRITICAL_SECTION*>(&Tile::TileUpdateAllCritSect));
            for (uint32_t i = 0; i < tile->xTraits.iSize; ++i) {
                if (tile->xTraits.pBuffer[i]->eIndex != kTileValue_Class) {
                    tile->xTraits.pBuffer[i]->CalculateValue(true);
                }
            }
            LeaveCriticalSection(reinterpret_cast<_RTL_CRITICAL_SECTION*>(&Tile::TileUpdateAllCritSect));
            tile = tile->pParent;
        }
        else if (item->iCmd == 8) {
            if (!tile) {
                Warning("MENUS: Trait defined outside of any tile.");
            }
            else if (
                (item->fVal != -2147483648.0 || item->xStr.Length() == 0) && item->u.iTrait != kTileValue_String &&
                item->u.iTrait != kTileValue_Filename
            ) {
                tile->Set(item->u.iTrait, item->fVal, true);
            }
            else if (item->u.iTrait == kTileValue_Name) {
                tile->xName.Set(item->xStr);
            }
            else if (item->u.iTrait == kTileValue_Filename && tile->GetType() == kTileValue_Image) {
                tile->Set(item->u.iTrait, item->xStr.pString, true);
                tile->AddNeedsUpdate(CHANGE_TEXTURE);
            }
            else {
                tile->Set(item->u.iTrait, item->xStr.pString, true);
            }
        }
        else if (item->iCmd == 4) {
            action = static_cast<int32_t>(item->fVal);
        }
        else if (item->iCmd == 5) {
            action = 0;
        }
        else if (item->iCmd == 9) {
            if (action) {
                tile->GetOrCreateValue(action)->AddAction(item->u.iTrait, item->fVal);
            }
            else {
                Warning("MENUS: Action defined outside of any trait.");
            }
        }
        else if (item->iCmd == 10) {
            if (action) {
                Tile* sourceTile = Tile::GetTileByName(tile, item->xStr.pString);
                if (sourceTile) {
                    const int32_t sourceTrait = NiFastFloatToInt(item->fVal);
                    tile->GetOrCreateValue(action)->AddAction(item->u.iTrait, sourceTile, sourceTrait);
                }
            }
            else {
                Warning("MENUS: Action link defined outside of any trait.");
            }
        }
        else if (item->iCmd == 2) {
            if (action) {
                const float operand = static_cast<float>(NiFastFloatToInt(item->fVal));
                tile->GetOrCreateValue(action)->AddAction(kTileValue_ActionBegin, operand);
            }
            else {
                Warning("MENUS: Action begun outside of any trait.");
            }
        }
        else if (item->iCmd == 3) {
            if (action) {
                const float operand = static_cast<float>(NiFastFloatToInt(item->fVal));
                tile->GetOrCreateValue(action)->AddAction(kTileValue_ActionEnd, operand);
            }
            else {
                Warning("MENUS: Action ended outside of any trait.");
            }
        }
    }
}

Tile* Tile::FinalPostParse(int32_t aiTrait, float afValue, const char*) {
    if (aiTrait == kTileValue_X || aiTrait == kTileValue_Y || aiTrait == kTileValue_Depth) {
        if (aiTrait == kTileValue_Depth) {
            // The tile's absolute depth: its own plus the depth of every locus between it and the menu root.
            float depth = afValue;
            Tile* current = this;
            while (current->pParent && current->pParent->pParent) {
                if (current->GetFloat(kTileValue_Locus) != 0.0) {
                    depth = current->GetFloat(kTileValue_Depth) + depth;
                }
                current = current->pParent;
            }
            Menu* menu = this->GetMenu();
            Tile* menuTile = this->GetMenuTile();
            if (menu && menuTile && menuTile != this && menu &&
                static_cast<double>(depth) > static_cast<double>(menu->iMenuThickness)) {
                menu->iMenuThickness = static_cast<int32_t>(depth);
            }
        }
        if (this->GetFloat(kTileValue_ClipWindow) != 0.0) {
            this->AddNeedsUpdate(CHANGE_SCISSOR_WINDOW);
        }
        this->AddNeedsUpdate(CHANGE_POSITION);
        return this;
    }
    if (this->GetType() == kTileValue_Text &&
        (aiTrait == kTileValue_WrapWidth || aiTrait == kTileValue_WrapLimit || aiTrait == kTileValue_WrapLines ||
         aiTrait == kTileValue_LineGap || aiTrait == kTileValue_PageNum || aiTrait == kTileValue_IsHTML ||
         aiTrait == kTileValue_Font || aiTrait == kTileValue_Justify)) {
        this->AddNeedsUpdate(CHANGE_CREATE);
        return this;
    }
    if ((aiTrait == kTileValue_Width || aiTrait == kTileValue_Height || aiTrait == kTileValue_CropOffsetX ||
         aiTrait == kTileValue_CropOffsetY) &&
        this->GetType() == kTileValue_Image) {
        if (this->GetFloat(kTileValue_ClipWindow) != 0.0) {
            this->AddNeedsUpdate(CHANGE_SCISSOR_WINDOW);
        }
        this->AddNeedsUpdate(CHANGE_GEOMETRY);
        return this;
    }
    if (aiTrait == kTileValue_ClipWindow &&
        (this->GetType() == kTileValue_Image || this->GetType() == kTileValue_Rect)) {
        this->AddNeedsUpdate(CHANGE_SCISSOR_WINDOW);
        return this;
    }
    if (aiTrait == kTileValue_Clips && (this->GetType() == kTileValue_Image || this->GetType() == kTileValue_Text)) {
        this->AddNeedsUpdate(CHANGE_SCISSOR);
        NiTListItem<Tile*>* position = this->xChildren.m_pkHead;
        while (position) {
            Tile*& child = this->xChildren.GetNext(position);
            const float clips = this->GetFloat(kTileValue_Clips);
            child->Set(kTileValue_Clips, clips, true);
        }
        return this;
    }
    if (aiTrait == kTileValue_Visible) {
        this->AddNeedsUpdate(CHANGE_VISIBILITY);
        return this;
    }
    if (aiTrait == kTileValue_Alpha || aiTrait == kTileValue_Red || aiTrait == kTileValue_Green ||
        aiTrait == kTileValue_Blue || aiTrait == kTileValue_SystemColor || aiTrait == kTileValue_Brightness) {
        if (this->GetType() == kTileValue_Image || this->GetType() == kTileValue_Text) {
            this->AddNeedsUpdate(CHANGE_COLOR);
        }
        return this;
    }
    if (aiTrait == kTileValue_Filename && this->GetType() == kTileValue_Image && this->GetString(kTileValue_Filename)) {
        this->AddNeedsUpdate(CHANGE_TEXTURE);
        return this;
    }
    if (aiTrait == kTileValue_Filename && this->GetType() == kTileValue_Nif && this->GetString(kTileValue_Filename)) {
        this->AddNeedsUpdate(CHANGE_NIF_FILE);
        return this;
    }
    if (aiTrait == kTileValue_Locus) {
        this->AddNeedsUpdate(CHANGE_LOCUS);
        return this;
    }
    if (aiTrait == kTileValue_ID) {
        Menu* menu = this->GetMenu();
        if (menu) {
            menu->AttachTileByID(static_cast<int32_t>(afValue), this);
        }
    }
    if (aiTrait == kTileValue_RotateAngle || aiTrait == kTileValue_RotateAxisX || aiTrait == kTileValue_RotateAxisY) {
        this->AddNeedsUpdate(CHANGE_POSITION);
        return this;
    }
    return nullptr;
}

bool Tile::HasFadeControl(int32_t aiTrait) {
    // The scan ends at the first node without an item: the loop condition tests both.
    for (const BSSimpleList<FadeControl*>* node = &Tile::xFadeControls; node && node->GetItem();
         node = node->GetNext()) {
        const FadeControl* control = node->GetItem();
        if (control->iTrait == aiTrait && control->pParent == this) {
            return true;
        }
    }
    return false;
}

Tile::Value* Tile::GetValue(int32_t aiTrait) {
    // The values are sorted by index, so the scan can stop at the first one past it. The tree's critical section
    // is held across the scan.
    Value* value = nullptr;
    uint32_t i = 0;
    EnterCriticalSection(reinterpret_cast<_RTL_CRITICAL_SECTION*>(&Tile::TileUpdateAllCritSect));
    while (i < this->xTraits.iSize && this->xTraits.pBuffer[i]->eIndex < aiTrait) {
        ++i;
    }
    if (i < this->xTraits.iSize && this->xTraits.pBuffer[i]->eIndex == aiTrait) {
        value = this->xTraits.pBuffer[i];
    }
    LeaveCriticalSection(reinterpret_cast<_RTL_CRITICAL_SECTION*>(&Tile::TileUpdateAllCritSect));
    return value;
}

// Like GetValueQ it builds a search key, compareVal, on first use.
Tile::Value* Tile::GetOrCreateValue(int32_t aiTrait) {
    Tile::Value* preturnVal_2 = nullptr;
    EnterCriticalSection(&Tile::TileUpdateAllCritSect.CriticalSection);
    static Tile::Value compareVal(Tile::kTileValue_Invalid, nullptr, 0.0f, nullptr, nullptr);
    uint32_t v8 = -1;
    compareVal.eIndex = static_cast<Tile::eTileValue>(aiTrait);
    Tile::Value* Key = &compareVal;
    uint32_t uifoundIndex = xTraits.SortedFind(Key, &Tile::ValueSortFunc);
    if (uifoundIndex == -1) {
        Tile::Value* preturnVal_3;
        Tile::Value* preturnVal = static_cast<Tile::Value*>(MemoryManager_Singleton.Allocate(sizeof(Tile::Value)));
        if (preturnVal) {
            preturnVal->eIndex = Tile::kTileValue_Invalid;
            preturnVal->pParent = nullptr;
            preturnVal->fValue = 0.0f;
            preturnVal->strValue = nullptr;
            preturnVal->pActionListA = nullptr;
            preturnVal_3 = preturnVal;
        }
        else {
            preturnVal_3 = nullptr;
        }
        preturnVal_2 = preturnVal_3;
        preturnVal_3->eIndex = static_cast<Tile::eTileValue>(aiTrait);
        preturnVal_2->pParent = this;
        xTraits.SortedInsert(preturnVal_2, &Tile::ValueSortFunc);
    }
    else {
        preturnVal_2 = xTraits.pBuffer[uifoundIndex];
    }
    LeaveCriticalSection(&Tile::TileUpdateAllCritSect.CriticalSection);
    return preturnVal_2;
}

// The search key compareVal is built on first use and destroyed at exit. The sorted find reads it.
Tile::Value* Tile::GetValueQ(int32_t aiTrait) {
    Tile::Value* v5 = nullptr;
    EnterCriticalSection(&Tile::TileUpdateAllCritSect.CriticalSection);
    static Tile::Value compareVal(Tile::kTileValue_Invalid, nullptr, 0.0f, nullptr, nullptr);
    uint32_t v6 = -1;
    compareVal.eIndex = static_cast<Tile::eTileValue>(aiTrait);
    Tile::Value* Key = &compareVal;
    v6 = xTraits.SortedFind(Key, &Tile::ValueSortFunc);
    if (v6 != -1) {
        v5 = xTraits.pBuffer[v6];
    }
    LeaveCriticalSection(&Tile::TileUpdateAllCritSect.CriticalSection);
    return v5;
}

Tile::Value* Tile::GetUnderscoreValue(int32_t aiBaseTrait, int32_t aiID) {
    char acName[260];
    const char* pcBaseName = Tile::TraitToText(aiBaseTrait);
    BSsprintf(acName, sizeof(acName), "%s%d", pcBaseName, aiID);
    return this->GetValueQ(Tile::TextToTrait(acName));
}

float Tile::GetFloat(int32_t aiTrait) {
    const Value* value = this->GetValueQ(aiTrait);
    return value ? value->fValue : 0.0f;
}

const char* Tile::GetString(int32_t aiTrait) {
    const Value* value = this->GetValueQ(aiTrait);
    return value ? value->strValue : nullptr;
}

bool Tile::IsTrue(int32_t aeTrait) {
    const Value* value = this->GetValueQ(aeTrait);
    // True for any value but zero, NaN included.
    return value ? value->fValue != 0.0 : false;
}

void Tile::Poke(int32_t aiTrait, float afValue) {
    // First sets the value, then reports it back as zero so the XML sees both events.
    this->Set(aiTrait, afValue, true);
    this->Set(aiTrait, 0.0f, true);
}

void Tile::Set(int32_t aiTrait, int32_t aiValue) {
    this->Set(aiTrait, static_cast<float>(aiValue), true);
}

void Tile::Set(int32_t aiTrait, uint32_t auiValue) {
    // The value converts as unsigned.
    const float value = static_cast<float>(static_cast<double>(auiValue));
    this->Set(aiTrait, value, true);
}

void Tile::Set(int32_t aiTrait, float afValue, bool abClearActions) {
    Value* value = this->GetOrCreateValue(aiTrait);
    if (value) {
        Tile::Lock();
        value->SetFloat(afValue, abClearActions);
        Tile::Unlock();
    }
}

void Tile::Set(int32_t aiTrait, const char* apString, bool abClearActions) {
    Value* value = this->GetOrCreateValue(aiTrait);
    if (value) {
        Tile::Lock();
        value->SetString(const_cast<char*>(apString), abClearActions);
        Tile::Unlock();
    }
}

void Tile::SetNeedsUpdate(int32_t aiFlag) {
    if (aiFlag <= CHANGE_MASK) {
        const uint32_t oldFlags = this->uiFlags;
        this->uiFlags = oldFlags & ~static_cast<uint32_t>(CHANGE_MASK);
        this->uiFlags |= aiFlag;
        if (this->uiFlags != oldFlags) {
            Tile::AddDirtyTile(this);
        }
    }
}

float Tile::GetRealX() {
    float x = this->GetFloat(0xFA1 /* kTileValue_X */);
    for (Tile* ancestor = this->pParent; ancestor; ancestor = ancestor->pParent) {
        if (ancestor->GetFloat(0xFA8 /* kTileValue_Locus */) != 0.0) {
            x = ancestor->GetFloat(0xFA1 /* kTileValue_X */) + x;
        }
    }
    return x;
}

float Tile::GetRealY() {
    float y = this->GetFloat(0xFA2 /* kTileValue_Y */);
    for (Tile* ancestor = this->pParent; ancestor; ancestor = ancestor->pParent) {
        if (ancestor->GetFloat(0xFA8 /* kTileValue_Locus */) != 0.0) {
            y = ancestor->GetFloat(0xFA2 /* kTileValue_Y */) + y;
        }
    }
    return y;
}

void Tile::UpdateClipwindows() {
    NiTListItem<Tile*>* position = this->xChildren.m_pkHead;
    while (position) {
        Tile* child = this->xChildren.GetNext(position);
        if (child->IsTrue(0xFA6 /* kTileValue_ClipWindow */)) {
            child->AddNeedsUpdate(0x80);
        }
        else {
            child->UpdateClipwindows();
        }
    }
}

void Tile::SetManualTriUpdate(bool abOn) {
    if (abOn) {
        this->uiFlags |= TRI_UPDATE_MANUAL;
    }
    else {
        this->uiFlags &= ~TRI_UPDATE_MANUAL;
    }
}

void Tile::SetSpeechChallengeFailure(bool bValue) {
    this->bSpeechChallengeFailure = bValue;
}

bool Tile::GetSpeechChallengeFailure() {
    return this->bSpeechChallengeFailure;
}

void Tile::ReClipChildren(float afx, float afy, float afx2, float afy2) {
    NiTListItem<Tile*>* position = this->xChildren.m_pkHead;
    while (position) {
        Tile* child = this->xChildren.GetNext(position);
        if (child->IsTrue(kTileValue_Clips) && child->spModel.m_pObject) {
            // EXE-INLINED 0x4ADD70
            NiNode* pModel = child->spModel.m_pObject;
            NiAVObjectArray_Compact(&pModel->m_kChildren);
            NiAVObjectArray_UpdateSize(&pModel->m_kChildren);
            for (uint32_t i = 0; i < child->spModel.m_pObject->m_kChildren.m_usESize; ++i) {
                NiAVObject* geometry = child->spModel.m_pObject->m_kChildren.m_pBase[i].m_pObject;
                TileShaderProperty* property =
                    static_cast<TileShaderProperty*>(geometry->GetProperty(NiProperty::SHADE));
                if (!property) {
                    continue;
                }
                float width = static_cast<float>(Interface::GetRealScreenWidth());
                float height = Interface::GetRealScreenHeight();
                // A menu rendered into the pip-boy clips to the rendered menu's screen.
                if (this->GetRenderedMenu()) {
                    width = Interface::GetRenderedMenuScreenWidth();
                    height = Interface::GetRenderedMenuScreenHeight();
                }
                // The rectangle is clamped to the screen; NaN edges become 0 (left, top) or the screen size.
                const float left = afx > 0.0f ? afx : 0.0f;
                const int32_t iLeft = static_cast<int32_t>(left);
                const float top = afy > 0.0f ? afy : 0.0f;
                const int32_t iTop = static_cast<int32_t>(top);
                const float right = width > afx2 ? afx2 : width;
                const int32_t iRight = static_cast<int32_t>(right);
                const float bottom = height > afy2 ? afy2 : height;
                const int32_t iBottom = static_cast<int32_t>(bottom);
                property->ScissorRect[0] = iLeft;
                property->ScissorRect[1] = iTop;
                property->ScissorRect[2] = iRight;
                property->ScissorRect[3] = iBottom;
                property->SetUseScissorTest(true);
            }
        }
        child->ReClipChildren(afx, afy, afx2, afy2);
    }
}

void Tile::AddFadeControl(
    int32_t aiTrait,
    float afStart,
    float afEnd,
    float afLength,
    Tile::TILE_FADE_CONTROL_TYPE aeFadeType
) {
    // Nothing to fade, or no time to fade in (NaN lengths too).
    if (afStart == afEnd || !(afLength > 0.0)) {
        return;
    }
    this->RemoveFadeControl(aiTrait);
    FadeControl* control = static_cast<FadeControl*>(MemoryManager_Singleton.Allocate(sizeof(FadeControl)));
    control->pParent = this;
    control->iTrait = static_cast<eTileValue>(aiTrait);
    control->fStartValue = afStart;
    control->fEndValue = afEnd;
    control->fDurationMillis = static_cast<float>(afLength * 1000.0);
    control->uiStartTime = GetTickCount();
    control->eFadeType = aeFadeType;
    if (aeFadeType == 3) {
        // A counted flash: one flash per half second, each lasting its share of the length.
        const float flashes = static_cast<float>(afLength / 0.5);
        this->Set(Tile::AddUserTrait("_FlashCount", -1), 0.0f, true);
        const float totalFlashes = static_cast<float>(flashes - 2.0);
        this->Set(Tile::AddUserTrait("_TotalFlashCount", -1), totalFlashes, true);
        control->fDurationMillis = static_cast<float>(afLength / flashes * 1000.0);
    }
    if (aeFadeType == 2) {
        this->Set(Tile::AddUserTrait("_FlashCount", -1), 0.0f, true);
    }
    Tile::xFadeControls.AddHead(control);
}

void Tile::RemoveFadeControl(int32_t aiTrait) {
    // The fades to drop are collected first, then removed from the global list and freed.
    BSSimpleList<FadeControl*> removed;
    for (BSSimpleList<FadeControl*>* node = &Tile::xFadeControls; node && node->m_item; node = node->m_pkNext) {
        const bool traitMatches = node->m_item->iTrait == aiTrait || aiTrait == static_cast<int32_t>(0x80000000);
        if (traitMatches && node->m_item->pParent == this) {
            removed.AddHead(node->m_item);
        }
    }
    for (BSSimpleList<FadeControl*>* node = &removed; node && node->m_item; node = node->m_pkNext) {
        FadeControl* control = node->m_item;
        Tile::xFadeControls.Remove(control);
        MemoryManager_Singleton.Deallocate(control);
    }
    // Emptied once explicitly and once more by the list's destructor.
    removed.RemoveAll();
    removed.RemoveAll();
}

bool Tile::GetFadeFinished(int32_t aiTrait) {
    for (BSSimpleList<FadeControl*>* node = &Tile::xFadeControls; node && node->m_item; node = node->m_pkNext) {
        if (node->m_item->iTrait == aiTrait && node->m_item->pParent == this) {
            if (node->m_item->eFadeType == 1) {
                return false;
            }
            const uint32_t now = GetTickCount();
            // False for NaN, as the game does.
            return static_cast<double>(now - node->m_item->uiStartTime) / node->m_item->fDurationMillis >= 1.0;
        }
    }
    return true;
}

void Tile::SetParent(Tile* apParent, Tile* apReplacedTile) {
    // A released parent is being torn down and keeps its child list and count.
    if (this->pParent && (this->pParent->uiFlags & RELEASED) == 0) {
        const float childCount = static_cast<float>(this->pParent->GetFloat(kTileValue_ChildCount) - 1.0);
        this->pParent->Set(kTileValue_ChildCount, childCount, true);
        this->pParent->xChildren.Remove(this);
    }
    this->pParent = apParent;
    if (!this->pParent) {
        return;
    }
    const float childCount = static_cast<float>(this->pParent->GetFloat(kTileValue_ChildCount) + 1.0);
    this->pParent->Set(kTileValue_ChildCount, childCount, true);
    if (!apReplacedTile) {
        this->pParent->xChildren.AddHead(this);
        return;
    }
    bool inserted = false;
    NiTListItem<Tile*>* current = nullptr;
    NiTListItem<Tile*>* position = this->pParent->xChildren.m_pkHead;
    while (position) {
        current = position;
        Tile* child = this->pParent->xChildren.GetNext(position);
        if (child == apReplacedTile) {
            this->pParent->xChildren.Remove(this);
            this->pParent->xChildren.InsertAfter(current, this);
            inserted = true;
            break;
        }
    }
    if (!inserted) {
        this->pParent->xChildren.AddHead(this);
    }
}

void Tile::DeleteModel() {
    NiNode* model = this->spModel.m_pObject;
    if (!model) {
        return;
    }
    this->SeverExtraData();
    if (model->m_pkParent) {
        model->m_pkParent->DetachChild(model);
    }
    // The model stays alive until both references are dropped.
    NiPointer<NiNode> spOldModel(model);
    this->spModel = nullptr;
    spOldModel = nullptr;
}

void Tile::PlayTileSound(int32_t aiSoundType) {
    BSSoundHandle sound;
    const char* name = this->GetString(aiSoundType);
    Menu* menu = this->GetMenu();
    // Only while the menu's fade state is 1.
    if (menu && menu->xFadeState == 1 && name && *name) {
        sound = BSAudio::pAudioInstance->GetSoundHandleByName(
            name,
            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
        );
        sound.Play(false);
    }
}

void Tile::FadeIn3D(NiAVObject* apObject, float afPct) {
    TileShaderProperty* property;
    uint32_t i;
    NiNode* v6;
    if (apObject) {
        v6 = apObject->IsNode();
        for (i = 0; v6 && (i < ((uint32_t)((int32_t)v6->m_kChildren.m_usSize))); ++i) {
            FadeIn3D(static_cast<NiNode*>(v6->m_kChildren.m_pBase[i].m_pObject), afPct);
        }
        if (apObject->IsGeometry()) {
            property = static_cast<TileShaderProperty*>(apObject->NiAVObject::GetProperty(3));
            if (property) {
                property->fAlpha = afPct;
            }
        }
    }
}

void Tile::Release() {
    AutoMemContext memContext(MEM_CONTEXT_INTERFACE);
    Tile::Lock();
    this->SetMenuDeleting();
    Interface::TileIsBeingDeleted(this);
    this->uiFlags |= RELEASED;
    this->SeverExtraData();
    this->SetParent(nullptr, nullptr);
    for (uint32_t i = 0; i < this->xTraits.iSize; ++i) {
        Value* value = this->xTraits.pBuffer[i];
        if (value) {
            value->~Value();
            MemoryManager_Singleton.Deallocate(value);
        }
    }
    this->xTraits.Clear(true);
    // 0x80000000 removes the tile's fades on every trait.
    this->RemoveFadeControl(static_cast<int32_t>(0x80000000));
    if (this->spModel && this->spModel->m_pkParent) {
        NiNode* parent = this->spModel->m_pkParent;
        parent->DetachChild(this->spModel);
    }
    while (this->xChildren.m_uiCount) {
        Tile* child = this->xChildren.RemoveHead();
        if (child) {
            delete child;
        }
    }
    Tile::Unlock();
}

void Tile::SetMenuDeleting() {
    Tile::Lock();
    if (!(this->uiFlags & RELEASED) && !(this->uiFlags & MENU_DELETING)) {
        this->uiFlags |= MENU_DELETING;
        // The next node is read before the child's call.
        NiTListItem<Tile*>* position = this->xChildren.m_pkHead;
        while (position) {
            Tile** child = &position->m_element;
            position = position->m_pkNext;
            (*child)->SetMenuDeleting();
        }
    }
    Tile::Unlock();
}

void Tile::SeverExtraData() {
    Tile::Lock();
    if (!this->spModel || !this->spModel->m_usExtraDataSize) {
        Tile::Unlock();
        return;
    }
    {
        NiPointer<Extra> spExtra = static_cast<Extra*>(this->spModel->GetExtraData(*Menu::pTileptrStr));
        if (spExtra) {
            spExtra->pTile = nullptr;
        }
    }
    Tile::Unlock();
}

Tile* Tile::ReadFile(const char* apName) {
    AutoMemContext memContext(MEM_CONTEXT_INTERFACE);
    bReadFileError = false;
    Tile* tile = nullptr;
    BuildStorage* build = nullptr;
    // Never set: the menu always takes the sub-templates over and the storage is always freed.
    bool keepStorage = false;
    build = Tile::ParseFile(apName, nullptr, 0);
    if (bReadFileError) {
        return nullptr;
    }
    tile = this->BuildAndNameTree(build->GetCurrentTemplate());
    if (bReadFileError) {
        return nullptr;
    }
    tile->ConnectTraitsToTree(build->GetCurrentTemplate());
    if (bReadFileError) {
        return nullptr;
    }
    Menu* menu = tile->GetMenu();
    if (menu) {
        for (BSSimpleList<TileTemplate*>* node = &build->xSubTemplates; node && node->m_item; node = node->m_pkNext) {
            menu->AddTemplate(node->m_item);
            menu->bDeleteTemplates = !keepStorage;
            build->bDeleteTemplates = keepStorage;
            if (bReadFileError) {
                return nullptr;
            }
        }
    }
    if (!keepStorage) {
        if (build) {
            build->~BuildStorage();
            MemoryManager_Singleton.Deallocate(build);
        }
        build = nullptr;
    }
    NiRenderer::ms_pkRenderer->m_kRendererLock.Lock();
    if (tile) {
        tile->UpdateChildren(false);
    }
    NiRenderer::ms_pkRenderer->m_kRendererLock.Unlock();
    bReadFileError = false;
    return tile;
}

bool Tile::IsInGlowBranch() {
    Tile* th_s = pParent;
    while (th_s) {
        if (th_s->GetFloat(4010) == 110.0) {
            th_s = th_s->pParent;
        }
    }
    return false;
}

float Tile::GetRealDepth() {
    float Float = GetFloat(4013);
    for (Tile* i = this->pParent; i; i = i->pParent) {
        if (i->GetFloat(4008) == 0.0) {
            Tile* pParent = i->pParent;
            if (pParent != Interface::GetMenusRoot()) {
                continue;
            }
        }
        Float = i->GetFloat(4013) + Float;
    }
    return Float;
}

float Tile::GetFadeEndFor(int32_t aiTrait) {
    int32_t fEndValue = -1;
    for (Tile::FadeControl*& rpItem : Tile::xFadeControls.ItemsUntilNull()) {
        if (rpItem->iTrait == aiTrait && rpItem->pParent == this) {
            fEndValue = static_cast<int32_t>(rpItem->fEndValue);
        }
    }
    if (static_cast<double>(fEndValue) == -1.0) {
        return GetFloat(aiTrait);
    }

    return static_cast<float>(fEndValue);
}

Tile::TILE_FADE_CONTROL_TYPE Tile::GetFadeType(int32_t aiTrait) {
    for (Tile::FadeControl*& rpItem : Tile::xFadeControls.ItemsUntilNull()) {
        if (rpItem->iTrait == aiTrait && rpItem->pParent == this) {
            return rpItem->eFadeType;
        }
    }
    return Tile::TFCT_STANDARD;
}

void Tile::RecursiveRotationUpdate() {
    NiPoint3 result;
    NiMatrix3 mrot;
    NiTListItem<Tile*>* m_pkHead = xChildren.m_pkHead;
    NiTListItem<Tile*>* Itor = m_pkHead;
    while (Itor) {
        Tile** p_m_element = &Itor->m_element;
        Itor = Itor->m_pkNext;
        Tile* pChildTile = *p_m_element;
        if (pChildTile) {
            float fParentX = GetFloat(4001);
            float fParentY = GetFloat(4002);
            float fParentRotateAxisX = GetFloat(4091);
            float fParentRotateAxisY = GetFloat(4092);
            float fChildX = pChildTile->GetFloat(4001);
            float fChildY = pChildTile->GetFloat(4002);
            if (IsTrue(4008)) {
                fChildX = fChildX + fParentX;
                fChildY = fChildY + fParentY;
            }
            float v10 = fParentX + fParentRotateAxisX - fChildX;
            pChildTile->Set(4091, v10, true);
            float v9 = fParentY + fParentRotateAxisY - fChildY;
            pChildTile->Set(4092, v9, true);
            float afValue = GetFloat(4090);
            pChildTile->Set(4090, afValue, true);
            if (pChildTile->IsTrue(4008) && pChildTile->GetType() == 902) {
                float fdepth = pChildTile->GetFloat(4013);
                NiPoint3 xpos = NiPoint3_ZERO;
                NiNode* m_pObject = pChildTile->spModel.m_pObject;
                NiPoint3* p_m_Translate = &m_pObject->m_kLocal.m_Translate;
                m_pObject->m_kLocal.m_Translate.x = NiPoint3_ZERO.x;
                std::memcpy(&p_m_Translate->y, &xpos.y, sizeof(NiPoint3) - offsetof(NiPoint3, y));
                double Float = pChildTile->GetFloat(4001);
                xpos.x = Float + xpos.x;
                double v3 = pChildTile->GetFloat(4002);
                xpos.z = xpos.z - v3;
                xpos.y = fdepth * -0.00800000037997961 + xpos.y;
                NiNode* v21 = pChildTile->spModel.m_pObject;
                NiNode* pnode = v21;
                for (uint32_t a = 0; pnode && a < pnode->m_kChildren.m_usESize; ++a) {
                    NiPoint3 axisPoint;
                    NiAVObject* ptri;
                    NiAVObject** p_m_pObject = &pnode->m_kChildren.m_pBase[a].m_pObject;
                    NiAVObject* v19 = *p_m_pObject;
                    if (v19) {
                        char v17;
                        for (const NiRTTI* i = v19->GetRTTI(); i; i = i->GetBaseRTTI()) {
                            if (i == &NiTriShape::ms_RTTI) {
                                v17 = 1;
                                goto LABEL_19;
                            }
                        }
                        v17 = 0;
                    LABEL_19:;
                        ptri = v17 != 0 ? v19 : nullptr;
                    }
                    else {
                        ptri = nullptr;
                    }
                    float v15 = pChildTile->GetFloat(4091);
                    float v16 = -pChildTile->GetFloat(4092);
                    axisPoint.x = v15;
                    axisPoint.y = 0.0f;
                    axisPoint.z = v16;
                    float afAngle = pChildTile->GetFloat(4090);
                    mrot.MakeYRotation(afAngle);
                    if (ptri) {
                        ptri->m_kLocal.m_Rotate = mrot;
                        NiPoint3* afAnglea = &(result = mrot * axisPoint);
                        ptri->m_kLocal.m_Translate = xpos + axisPoint - *afAnglea;
                    }
                }
            }
            pChildTile->RecursiveRotationUpdate();
        }
    }
}

int32_t Tile::ValueSortFunc(const void* apVal1, const void* apVal2) {
    const Value* pValue1 = *static_cast<const Value* const*>(apVal1);
    const Value* pValue2 = *static_cast<const Value* const*>(apVal2);
    if (pValue1->eIndex < pValue2->eIndex) {
        return -1;
    }
    if (pValue1->eIndex > pValue2->eIndex) {
        return 1;
    }
    return 0;
}

bool Tile::IsValidUserTraitName(const char* apTraitName) {
    return apTraitName && *apTraitName && (*apTraitName == '_' || !std::memcmp(apTraitName, "&_", 2));
}

int32_t Tile::GetExtraDataFromTraitName(const char* apTraitName) {
    // Extra data traits end in "_<id>"; the id is everything after the last underscore.
    const char* underscore = std::strrchr(apTraitName, '_');
    if (!underscore) {
        return 0;
    }
    return underscore[1] != '\0' ? std::atoi(underscore + 1) : -1;
}

float Tile::GetMaximumDepth() {
    return Menu::GetMaxDepth();
}

double Tile::GetTileThickness(Tile* apTile) {
    float Float = apTile->GetFloat(4013);
    NiTListItem<Tile*>* m_pkHead = apTile->xChildren.m_pkHead;
    while (m_pkHead) {
        Tile** p_m_element = &m_pkHead->m_element;
        m_pkHead = m_pkHead->m_pkNext;
        Tile* v10 = *p_m_element;
        if (apTile->GetFloat(4008) == 0.0) {
            float v2;
            float v5 = GetTileThickness(v10);
            if (!(v5 < static_cast<double>(Float))) {
                v2 = v5;
            }
            else {
                v2 = Float;
            }
            Float = v2;
        }
        else {
            float v3;
            double v4 = apTile->GetFloat(4013);
            float v6 = GetTileThickness(v10) + v4;
            if (!(v6 < static_cast<double>(Float))) {
                v3 = v6;
            }
            else {
                v3 = Float;
            }
            Float = v3;
        }
    }
    return Float;
}

void Tile::ResetUpdateCount() {
    Tile::iTilesUpdatedThisFrame = 0;
}

BSCriticalSection* Tile::GetTileUpdateAllCriticalSectionLock() {
    return &Tile::TileUpdateAllCritSect;
}

// The TileMenu constructor ignores its arguments.
Tile* Tile::BuildTile(int32_t aiID) {
    Tile* result;
    switch (aiID) {
        case 901: {
            Tile* v7;
            TileRect* v13 = static_cast<TileRect*>(MemoryManager_Singleton.Allocate(sizeof(TileRect)));
            if (v13) {
                v7 = ::new (v13) TileRect;
            }
            else {
                v7 = nullptr;
            }
            result = v7;
            break;
        }
        case 902:
        case 906: {
            TileImage* v6;
            TileImage* apMemory = static_cast<TileImage*>(MemoryManager_Singleton.Allocate(sizeof(TileImage)));
            if (apMemory) {
                v6 = ::new (apMemory) TileImage;
            }
            else {
                v6 = nullptr;
            }
            result = v6;
            break;
        }
        case 903: {
            TileText* v4;
            TileText* v10 = static_cast<TileText*>(MemoryManager_Singleton.Allocate(sizeof(TileText)));
            if (v10) {
                v4 = ::new (v10) TileText;
            }
            else {
                v4 = nullptr;
            }
            result = v4;
            break;
        }
        case 904: {
            Tile3D* v5;
            Tile3D* v11 = static_cast<Tile3D*>(MemoryManager_Singleton.Allocate(sizeof(Tile3D)));
            if (v11) {
                v5 = ::new (v11) Tile3D;
            }
            else {
                v5 = nullptr;
            }
            result = v5;
            break;
        }
        case 905: {
            Tile* v3;
            TileMenu* v9 = static_cast<TileMenu*>(MemoryManager_Singleton.Allocate(sizeof(TileMenu)));
            if (v9) {
                v3 = ::new (v9) TileMenu(nullptr, nullptr);
            }
            else {
                v3 = nullptr;
            }
            result = v3;
            break;
        }
        case 908: {
            RadialTile* v2;
            RadialTile* v8 = static_cast<RadialTile*>(MemoryManager_Singleton.Allocate(sizeof(RadialTile)));
            if (v8) {
                v2 = ::new (v8) RadialTile;
            }
            else {
                v2 = nullptr;
            }
            result = v2;
            break;
        }
        default: {
            result = nullptr;
            break;
        }
    }
    return result;
}

Tile* Tile::GetTileByName(Tile* apTarget, const char* apName) {
    Tile* found = nullptr;
    char acParam[0x800];
    acParam[0] = '\0';
    switch (Tile::TextToTraitAndParam(apName, acParam)) {
        case kTileValue_Parent: return apTarget->pParent;
        case kTileValue_Me: return apTarget;
        case kTileValue_Sibling: {
            if (!acParam[0] && apTarget->pParent->xChildren.m_uiCount != 0) {
                // The next sibling, wrapping around to the first child.
                NiTListItem<Tile*>* position = apTarget->pParent->xChildren.m_pkHead;
                while (position) {
                    if (apTarget->pParent->xChildren.GetNext(position) == apTarget) {
                        break;
                    }
                }
                if (position) {
                    return apTarget->pParent->xChildren.GetNext(position);
                }
                return apTarget->pParent->xChildren.m_pkHead->m_element;
            }
            if (acParam[0] && apTarget->pParent->xChildren.m_uiCount != 0) {
                // The sibling with the given name.
                NiTListItem<Tile*>* position = apTarget->pParent->xChildren.m_pkHead;
                while (position) {
                    Tile* sibling = apTarget->pParent->xChildren.GetNext(position);
                    if (sibling->xName.pString && !_stricmp(sibling->xName.pString, acParam)) {
                        found = sibling;
                        break;
                    }
                }
                return found;
            }
            return nullptr;
        }
        case kTileValue_Child:
            if (!acParam[0] && apTarget->xChildren.m_uiCount != 0) {
                return apTarget->xChildren.m_pkHead->m_element;
            }
            if (apTarget && acParam[0] && apTarget->xChildren.m_uiCount != 0) {
                return apTarget->GetChildByName(acParam);
            }
            return nullptr;
        case kTileValue_Screen: return Interface::GetMenusRoot();
        case kTileValue_Globals: return InterfaceManager::GetGlobalsTile();
        case kTileValue_IO: return apTarget->GetMenu()->pMenu;
        case kTileValue_Grandparent: return apTarget->pParent ? apTarget->pParent->pParent : nullptr;
        default:
            if (apTarget) {
                return Tile::SearchForTileByName(apTarget->GetMenuTile(), apName);
            }
            return Tile::SearchForTileByName(Interface::GetMenusRoot(), apName);
    }
}

void Tile::CheckHibernatingTiles() {
    NiTListItem<Tile*>* position = Tile::xHibernatingTiles.m_pkHead;
    while (position) {
        Tile* tile = position->m_element;
        if (tile->IsVisible()) {
            // Equivalent to SetHibernated(false) and SetPromoted(false).
            tile->uiFlags &= ~HIBERNATED;
            tile->uiFlags &= ~PROMOTED;
            Tile::AddDirtyTile(tile);
            // Moves the position on to the next tile.
            Tile::xHibernatingTiles.RemovePos(position);
        }
        else {
            position = Tile::xHibernatingTiles.GetNextPos(position);
        }
    }
}

void Tile::AddHibernatingTile(Tile* apTile) {
    if (!apTile) {
        return;
    }
    if ((apTile->uiFlags & HIBERNATED) != 0) {
        return;
    }
    // A tile that was just added last is not added twice.
    if (Tile::xHibernatingTiles.m_uiCount != 0 && apTile == Tile::xHibernatingTiles.m_pkTail->m_element) {
        return;
    }
    ++Tile::iHibernatingTileCount;
    // The node comes from the node pool, called with the list's count field as its `this`.
    NiTListItem<Tile*>* pkNode = static_cast<NiTListItem<Tile*>*>(
        reinterpret_cast<NiTPointerAllocator<uint32_t>&>(Tile::xHibernatingTiles.m_uiCount).Allocate()
    );
    pkNode->m_element = apTile;
    Tile::xHibernatingTiles.AddNodeTail(pkNode);
    // Equivalent to SetHibernated(true) and SetPromoted(false).
    apTile->uiFlags |= HIBERNATED;
    apTile->uiFlags &= ~PROMOTED;
}

void Tile::UpdateFadeControls() {
    if (!Tile::xFadeControls.m_pkNext && !Tile::xFadeControls.m_item) {
        return;
    }
    BSSimpleList<FadeControl*> finished;
    for (BSSimpleList<FadeControl*>* node = &Tile::xFadeControls; node && node->m_item; node = node->m_pkNext) {
        const float end = node->m_item->fEndValue;
        const float start = node->m_item->fStartValue;
        const int32_t trait = node->m_item->iTrait;
        switch (node->m_item->eFadeType) {
            case 0: {
                // A linear fade from start to end.
                const uint32_t now = GetTickCount();
                float t = static_cast<float>(
                    static_cast<double>(now - node->m_item->uiStartTime) / node->m_item->fDurationMillis
                );
                t = 1.0f > t ? t : 1.0f;
                if (node->m_item->pParent->GetType() == kTileValue_Nif && trait == kTileValue_Alpha) {
                    const float pct = static_cast<float>(((end - start) * t + start) / 255.0);
                    node->m_item->pParent->FadeIn3D(node->m_item->pParent->spModel.m_pObject, pct);
                }
                const float value = ((end - start) * t + start);
                node->m_item->pParent->Set(trait, value, true);
                if (t >= 1.0) {
                    finished.AddHead(node->m_item);
                }
                break;
            }
            case 1:
            case 2:
            case 3: {
                // Flashes between start and end: 1 forever, 2 three times then holding end, 3 a counted number.
                const int32_t type = node->m_item->eFadeType;
                const float flashCount = node->m_item->pParent->GetFloat(Tile::TextToTrait("_FlashCount"));
                const float totalFlashCount = node->m_item->pParent->GetFloat(Tile::TextToTrait("_TotalFlashCount"));
                const uint32_t now = GetTickCount();
                float t = static_cast<float>(
                    static_cast<double>(now - node->m_item->uiStartTime) / node->m_item->fDurationMillis
                );
                if (t >= 1.0) {
                    if ((type == 2 && flashCount < 3.0) || (type == 3 && totalFlashCount > flashCount) || type == 1) {
                        node->m_item->uiStartTime = GetTickCount();
                        t = 0.0f;
                    }
                    if (type == 2 || type == 3) {
                        const float nextCount = static_cast<float>(flashCount + 1.0);
                        node->m_item->pParent->Set(Tile::TextToTrait("_FlashCount"), nextCount, true);
                        if (type == 2 && flashCount > 3.0) {
                            node->m_item->pParent->Set(trait, end, true);
                        }
                        else if (type == 3 && totalFlashCount < flashCount) {
                            node->m_item->pParent->Set(trait, start, true);
                            finished.AddHead(node->m_item);
                        }
                    }
                }
                if (type == 2 && flashCount >= 3.0) {
                    const float value = ((end - start) * t + start);
                    node->m_item->pParent->Set(trait, end < value ? end : value, true);
                    if (end <= value) {
                        finished.AddHead(node->m_item);
                    }
                }
                else {
                    // A triangle wave: 0 at both ends of the period, 1 in its middle.
                    const float wave = static_cast<float>(t * 2.0 - 1.0);
                    const float magnitude = static_cast<float>(std::fabs(static_cast<double>(wave)));
                    const float level = static_cast<float>(1.0 - magnitude);
                    const float value = ((end - start) * level + start);
                    node->m_item->pParent->Set(trait, value, true);
                }
                break;
            }
            case 4: {
                // A pulse: up to end over the first sixth, hold, back to start over the last sixth.
                const uint32_t now = GetTickCount();
                const float t = static_cast<float>(
                    static_cast<double>(now - node->m_item->uiStartTime) / node->m_item->fDurationMillis
                );
                if (t < 0.16665999591350555) {
                    const float value = (start + (end - start) * ((t - 0.0f) / (0.16666f - 0.0f)));
                    node->m_item->pParent->Set(trait, value, true);
                }
                else if (t < 0.8333399891853333) {
                    node->m_item->pParent->Set(trait, end, true);
                }
                else if (t >= 1.0) {
                    node->m_item->pParent->Set(trait, start, true);
                    finished.AddHead(node->m_item);
                }
                else {
                    const float value = (end + (start - end) * ((t - 0.83334f) / (1.0f - 0.83334f)));
                    node->m_item->pParent->Set(trait, value, true);
                }
                break;
            }
            default: break;
        }
    }
    for (BSSimpleList<FadeControl*>* node = &finished; node && node->m_item; node = node->m_pkNext) {
        FadeControl* control = node->m_item;
        Tile::xFadeControls.Remove(control);
        MemoryManager_Singleton.Deallocate(control);
    }
    // Emptied once explicitly and once more by the list's destructor.
    finished.RemoveAll();
    finished.RemoveAll();
}

Tile* Tile::SearchForTileByName(Tile* apTarget, const char* apName) {
    if (!apTarget) {
        return nullptr;
    }
    if (apTarget->xName.pString && !_stricmp(apTarget->xName.pString, apName)) {
        return apTarget;
    }
    NiTListItem<Tile*>* m_pkHead = apTarget->xChildren.m_pkHead;
    while (m_pkHead) {
        Tile** p_m_element = &m_pkHead->m_element;
        m_pkHead = m_pkHead->m_pkNext;
        Tile* pitem = Tile::SearchForTileByName(*p_m_element, apName);
        if (pitem) {
            return pitem;
        }
    }
    return nullptr;
}

void Tile::PromoteHibernators() {
    // The frame wakes at most 11 tiles in all, counting the ones already dirty or updated.
    int32_t budget = static_cast<int32_t>(Tile::xDirtyTiles.m_uiCount) + Tile::iTilesUpdatedThisFrame;
    if (budget > 10) {
        return;
    }
    budget = 11 - budget;
    NiTListItem<Tile*>* position = Tile::xHibernatingTiles.m_pkHead;
    while (position && budget) {
        Tile* tile = position->m_element;
        if (tile) {
            tile->uiFlags &= ~HIBERNATED;
            tile->uiFlags |= PROMOTED;
            Tile::AddDirtyTile(tile);
        }
        Tile::xHibernatingTiles.RemovePos(position);
        --budget;
    }
}

Tile* Tile::GetNextDirtyTile(bool abRemove) {
    Tile::Lock();
    if (Tile::xDirtyTiles.m_uiCount == 0) {
        Tile::Unlock();
        return nullptr;
    }
    if (abRemove) {
        Tile* tile = Tile::xDirtyTiles.RemoveHead();
        if (tile) {
            tile->uiFlags &= ~DIRTY;
        }
        Tile::Unlock();
        return tile;
    }
    Tile* tile = Tile::xDirtyTiles.m_pkHead->m_element;
    Tile::Unlock();
    return tile;
}

void Tile::AddDirtyTile(Tile* apTile) {
    if (!apTile) {
        return;
    }
    Tile::Lock();
    if ((apTile->uiFlags & DIRTY) == 0) {
        Tile::xDirtyTiles.AddTail(apTile);
        apTile->uiFlags |= DIRTY;
    }
    if ((apTile->uiFlags & HIBERNATED) != 0) {
        Tile::xHibernatingTiles.Remove(apTile);
        apTile->uiFlags &= ~HIBERNATED;
    }
    Tile::Unlock();
}

void Tile::UpdateAll(bool abDoNiUpdate) {
    AutoMemContext memContext(MEM_CONTEXT_INTERFACE);
    EnterCriticalSection(reinterpret_cast<_RTL_CRITICAL_SECTION*>(&Tile::TileUpdateAllCritSect));
    bool updated = false;
    Tile::bNeedsCheckHibernate = false;
    while (Tile::xDirtyTiles.m_uiCount) {
        Tile* tile = Tile::GetNextDirtyTile(true);
        if (!tile || (tile->uiFlags & (HIBERNATED | RELEASED | MENU_DELETING)) != 0 ||
            (tile->uiFlags & CHANGE_MASK) == 0) {
            continue;
        }
        updated = tile->UpdateTile(true);
        tile->bNeedsNiUpdate = tile->bNeedsNiUpdate | updated;
        if (updated) {
            InterfaceManager::pInstance->AddTileToUpdateList(tile);
        }
        // A top-level menu still at a placeholder stacking type gets the real one.
        if (tile->pParent == Interface::GetMenusRoot()) {
            if (tile->GetFloat(kTileValue_StackingType) == 102.0) {
                tile->Set(kTileValue_StackingType, 6006.0f, true);
            }
            else if (tile->GetFloat(kTileValue_StackingType) == 103.0) {
                tile->Set(kTileValue_StackingType, 6007.0f, true);
            }
        }
    }
    if (Tile::bNeedsCheckHibernate) {
        Tile::CheckHibernatingTiles();
        Tile::UpdateAll(abDoNiUpdate);
        LeaveCriticalSection(reinterpret_cast<_RTL_CRITICAL_SECTION*>(&Tile::TileUpdateAllCritSect));
        return;
    }
    if (abDoNiUpdate) {
        AutoMemContext modelContext(MEM_CONTEXT_INTERFACE);
        InterfaceManager::pInstance->UpdateTileModels();
    }
    if (Interface::GetFirstChanceTextureRelease()) {
        AutoMemContext purgeContext(MEM_CONTEXT_INTERFACE);
        Interface::SetFirstChanceTextureRelease(false);
        BSTexturePalette::PurgeUnusedTextures();
    }
    LeaveCriticalSection(reinterpret_cast<_RTL_CRITICAL_SECTION*>(&Tile::TileUpdateAllCritSect));
}

void Tile::Lock() {
    EnterCriticalSection(reinterpret_cast<_RTL_CRITICAL_SECTION*>(&Tile::TileUpdateAllCritSect));
}

void Tile::Unlock() {
    LeaveCriticalSection(reinterpret_cast<_RTL_CRITICAL_SECTION*>(&Tile::TileUpdateAllCritSect));
}

void Tile::AddToTable(const char* apText, int32_t aiIndex) {
    if (!apText || !*apText) {
        return;
    }
    Tile::Lock();
    Tile::xTextTable.SetAt(apText, aiIndex);
    // User trait ids start at 10000.
    if (Tile::IsValidUserTraitName(apText) && aiIndex >= 10000) {
        if (Tile::iTextTableUserNextID <= aiIndex) {
            Tile::iTextTableUserNextID = aiIndex + 1;
        }
        const int32_t extraData = Tile::GetExtraDataFromTraitName(apText);
        if (extraData) {
            Tile::xTraitExtraData.SetAt(aiIndex, extraData);
        }
    }
    Tile::Unlock();
}

// The table copies its keys, so the literals need not outlive the call. The controller and widescreen constants
// are read once, when the table is built.
void Tile::LoadTextTable() {
    const uint8_t ucHas360Controller = static_cast<uint8_t>(InterfaceManager::bHas360Controller);
    const uint8_t ucDisable360Controller = static_cast<uint8_t>(gs_bDisable360Controller.uValue.b);
    Tile::AddToTable("&generic;", -1);
    Tile::AddToTable("&false;", 0);
    Tile::AddToTable("&true;", 1);
    Tile::AddToTable("&left;", 1);
    Tile::AddToTable("&center;", 2);
    Tile::AddToTable("&right;", 4);
    Tile::AddToTable("&up;", 5);
    Tile::AddToTable("&down;", 6);
    Tile::AddToTable("&scale;", -1);
    Tile::AddToTable("&click_past;", 0x65);
    Tile::AddToTable("&no_click_past;", 0x66);
    Tile::AddToTable("&does_not_stack;", 0x1778);
    Tile::AddToTable("&mixed_menu;", 0x67);
    Tile::AddToTable("&xenon;", (ucHas360Controller != 0 && ucDisable360Controller == 0) ? 1 : 0);
    Tile::AddToTable("&console;", 0);
    Tile::AddToTable("&xbox;", (ucHas360Controller != 0 && ucDisable360Controller == 0) ? 1 : 0);
    Tile::AddToTable("&highdef;", 1);
    Tile::AddToTable("&widescreen;", Interface::IsWideScreen());
    Tile::AddToTable("&glow_branch;", 0x6E);
    Tile::AddToTable("&noglow_branch;", 0x6F);
    Tile::AddToTable("&pipboymenu;", 0x70);
    Tile::AddToTable("&uselocalcolor;", -1);
    Tile::AddToTable("&nosystemcolor;", 0);
    Tile::AddToTable("&nosound;", -1);
    Tile::AddToTable("&default_accept;", 9);
    Tile::AddToTable("&xbuttona;", 9);
    Tile::AddToTable("&xbuttonb;", 0xA);
    Tile::AddToTable("&xbuttonx;", 0xB);
    Tile::AddToTable("&xbuttony;", 0xC);
    Tile::AddToTable("&xbuttonlt;", 0xD);
    Tile::AddToTable("&xbuttonrt;", 0xE);
    Tile::AddToTable("&xbuttonlb;", 0xF);
    Tile::AddToTable("&xbuttonrb;", 0x10);
    Tile::AddToTable("&xbuttonls;", 0x11);
    Tile::AddToTable("&xbuttonrs;", 0x12);
    Tile::AddToTable("&xbuttonl;", 0x11);
    Tile::AddToTable("&xbuttonr;", 0x12);
    Tile::AddToTable("image", kTileValue_Image);
    Tile::AddToTable("menu", kTileValue_Menu);
    Tile::AddToTable("3d", kTileValue_Nif);
    Tile::AddToTable("nif", kTileValue_Nif);
    Tile::AddToTable("rect", kTileValue_Rect);
    Tile::AddToTable("hotrect", kTileValue_Hotrect);
    Tile::AddToTable("template", kTileValue_Template);
    Tile::AddToTable("text", kTileValue_Text);
    Tile::AddToTable("window", kTileValue_Window);
    Tile::AddToTable("radial", kTileValue_Radial);
    Tile::AddToTable("&AudioMenu;", 0x3F9);
    Tile::AddToTable("&BookMenu;", 0x402);
    Tile::AddToTable("&CharGenMenu;", 0x418);
    Tile::AddToTable("&ContainerMenu;", 0x3F0);
    Tile::AddToTable("&DialogMenu;", 0x3F1);
    Tile::AddToTable("&GameplayMenu;", 0x3FC);
    Tile::AddToTable("&HUDMainMenu;", 0x3EC);
    Tile::AddToTable("&InventoryMenu;", 0x3EA);
    Tile::AddToTable("&LoadingMenu;", 0x3EF);
    Tile::AddToTable("&LockPickMenu;", 0x3F6);
    Tile::AddToTable("&MapMenu;", 0x3FF);
    Tile::AddToTable("&MessageMenu;", 0x3E9);
    Tile::AddToTable("&TutorialMenu;", 0x423);
    Tile::AddToTable("&TextEditMenu;", 0x41B);
    Tile::AddToTable("&StartMenu;", 0x3F5);
    Tile::AddToTable("&QuantityMenu;", 0x3F8);
    Tile::AddToTable("&RaceSexMenu;", 0x40C);
    Tile::AddToTable("&SleepWaitMenu;", 0x3F4);
    Tile::AddToTable("&StatsMenu;", 0x3EB);
    Tile::AddToTable("&VideoMenu;", 0x3FA);
    Tile::AddToTable("&LevelUpMenu;", 0x403);
    Tile::AddToTable("&RepairMenu;", 0x40B);
    Tile::AddToTable("&ItemModMenu;", 0x425);
    Tile::AddToTable("&RepairServicesMenu;", 0x422);
    Tile::AddToTable("&CreditsMenu;", 0x417);
    Tile::AddToTable("&BarterMenu;", 0x41D);
    Tile::AddToTable("&SurgeryMenu;", 0x41E);
    Tile::AddToTable("&HackingMenu;", 0x41F);
    Tile::AddToTable("&ComputersMenu;", 0x421);
    Tile::AddToTable("&VATSMenu;", 0x420);
    Tile::AddToTable("&SPECIALBookMenu;", 0x424);
    Tile::AddToTable("&CompanionWheelMenu;", 0x433);
    Tile::AddToTable("&LoveTesterMenu;", 0x432);
    Tile::AddToTable("&TraitSelectMenu;", 0x434);
    Tile::AddToTable("&SlotMachineMenu;", 0x438);
    Tile::AddToTable("&BlackJackMenu;", 0x439);
    Tile::AddToTable("&RouletteMenu;", 0x43A);
    Tile::AddToTable("&RecipeMenu;", 0x435);
    Tile::AddToTable("&CaravanMenu;", 0x43B);
    Tile::AddToTable("&TraitMenu;", 0x43C);
    Tile::AddToTable("abs", kTileValue_Abs);
    Tile::AddToTable("add", kTileValue_Add);
    Tile::AddToTable("and", kTileValue_And);
    Tile::AddToTable("ceil", kTileValue_Ceil);
    Tile::AddToTable("copy", kTileValue_Copy);
    Tile::AddToTable("div", kTileValue_Div);
    Tile::AddToTable("eq", kTileValue_Eq);
    Tile::AddToTable("floor", kTileValue_Floor);
    Tile::AddToTable("gt", kTileValue_Gt);
    Tile::AddToTable("gte", kTileValue_Gte);
    Tile::AddToTable("lt", kTileValue_Lt);
    Tile::AddToTable("lte", kTileValue_Lte);
    Tile::AddToTable("max", kTileValue_Max);
    Tile::AddToTable("min", kTileValue_Min);
    Tile::AddToTable("mod", kTileValue_Mod);
    Tile::AddToTable("mul", kTileValue_Mul);
    Tile::AddToTable("mult", kTileValue_Mul);
    Tile::AddToTable("neq", kTileValue_Neq);
    Tile::AddToTable("not", kTileValue_Not);
    Tile::AddToTable("onlyif", kTileValue_OnlyIf);
    Tile::AddToTable("onlyifnot", kTileValue_OnlyIfNot);
    Tile::AddToTable("or", kTileValue_Or);
    Tile::AddToTable("ref", kTileValue_Ref);
    Tile::AddToTable("round", kTileValue_Round);
    Tile::AddToTable("sub", kTileValue_Sub);
    Tile::AddToTable("name", kTileValue_Name);
    Tile::AddToTable("src", kTileValue_Src);
    Tile::AddToTable("trait", kTileValue_Trait);
    Tile::AddToTable("value", kTileValue_Value);
    Tile::AddToTable("alpha", kTileValue_Alpha);
    Tile::AddToTable("animation", kTileValue_Animation);
    Tile::AddToTable("blue", kTileValue_Blue);
    Tile::AddToTable("brightness", kTileValue_Brightness);
    Tile::AddToTable("child_count", kTileValue_Child_Count);
    Tile::AddToTable("childcount", kTileValue_Child_Count);
    Tile::AddToTable("class", kTileValue_Class);
    Tile::AddToTable("clicked", kTileValue_Clicked);
    Tile::AddToTable("clicksound", kTileValue_ClickSound);
    Tile::AddToTable("mouseoversound", kTileValue_MouseoverSound);
    Tile::AddToTable("disablefade", kTileValue_DisableFade);
    Tile::AddToTable("cropoffsetx", kTileValue_CropOffsetX);
    Tile::AddToTable("cropoffsety", kTileValue_CropOffsetY);
    Tile::AddToTable("cropx", kTileValue_CropX);
    Tile::AddToTable("cropy", kTileValue_CropY);
    Tile::AddToTable("clips", kTileValue_Clips);
    Tile::AddToTable("clipwindow", kTileValue_ClipWindow);
    Tile::AddToTable("depth", kTileValue_Depth);
    Tile::AddToTable("draggable", kTileValue_Draggable);
    Tile::AddToTable("dragdeltax", kTileValue_DragDeltax);
    Tile::AddToTable("dragdeltay", kTileValue_DragDeltay);
    Tile::AddToTable("dragstartx", kTileValue_DragStartX);
    Tile::AddToTable("dragstarty", kTileValue_DragStartY);
    Tile::AddToTable("dragoffsetx", kTileValue_DragOffsetX);
    Tile::AddToTable("dragoffsety", kTileValue_DragOffsetY);
    Tile::AddToTable("dragx", kTileValue_DragX);
    Tile::AddToTable("dragy", kTileValue_DragY);
    Tile::AddToTable("explorefade", kTileValue_ExploreFade);
    Tile::AddToTable("fileheight", kTileValue_FileHeight);
    Tile::AddToTable("filename", kTileValue_Filename);
    Tile::AddToTable("filewidth", kTileValue_FileWidth);
    Tile::AddToTable("font", kTileValue_Font);
    Tile::AddToTable("green", kTileValue_Green);
    Tile::AddToTable("height", kTileValue_Height);
    Tile::AddToTable("id", kTileValue_ID);
    Tile::AddToTable("ishtml", kTileValue_IsHTML);
    Tile::AddToTable("justify", kTileValue_Justify);
    Tile::AddToTable("linecount", kTileValue_LineCount);
    Tile::AddToTable("linegap", kTileValue_LineGap);
    Tile::AddToTable("listindex", kTileValue_ListIndex);
    Tile::AddToTable("locus", kTileValue_Locus);
    Tile::AddToTable("menufade", kTileValue_MenuFade);
    Tile::AddToTable("mouseover", kTileValue_Mouseover);
    Tile::AddToTable("pagecount", kTileValue_PageCount);
    Tile::AddToTable("pagenum", kTileValue_PageNum);
    Tile::AddToTable("red", kTileValue_Red);
    Tile::AddToTable("repeatvertical", kTileValue_RepeatVertical);
    Tile::AddToTable("repeathorizontal", kTileValue_RepeatHorizontal);
    Tile::AddToTable("rotateangle", kTileValue_RotateAngle);
    Tile::AddToTable("rotateaxisx", kTileValue_RotateAxisX);
    Tile::AddToTable("rotateaxisy", kTileValue_RotateAxisY);
    Tile::AddToTable("shiftclicked", kTileValue_ShiftClicked);
    Tile::AddToTable("stackingtype", kTileValue_StackingType);
    Tile::AddToTable("string", kTileValue_String);
    Tile::AddToTable("systemcolor", kTileValue_SystemColor);
    Tile::AddToTable("target", kTileValue_Target);
    Tile::AddToTable("texatlas", kTileValue_Texatlas);
    Tile::AddToTable("tile", kTileValue_Tile);
    Tile::AddToTable("user0", kTileValue_User0);
    Tile::AddToTable("user1", kTileValue_User1);
    Tile::AddToTable("user2", kTileValue_User2);
    Tile::AddToTable("user3", kTileValue_User3);
    Tile::AddToTable("user4", kTileValue_User4);
    Tile::AddToTable("user5", kTileValue_User5);
    Tile::AddToTable("user6", kTileValue_User6);
    Tile::AddToTable("user7", kTileValue_User7);
    Tile::AddToTable("user8", kTileValue_User8);
    Tile::AddToTable("user9", kTileValue_User9);
    Tile::AddToTable("user10", kTileValue_User10);
    Tile::AddToTable("user11", kTileValue_User11);
    Tile::AddToTable("user12", kTileValue_User12);
    Tile::AddToTable("user13", kTileValue_User13);
    Tile::AddToTable("user14", kTileValue_User14);
    Tile::AddToTable("user15", kTileValue_User15);
    Tile::AddToTable("user16", kTileValue_User16);
    Tile::AddToTable("visible", kTileValue_Visible);
    Tile::AddToTable("wheelable", kTileValue_Wheelable);
    Tile::AddToTable("wheelmoved", kTileValue_Wheelmoved);
    Tile::AddToTable("width", kTileValue_Width);
    Tile::AddToTable("wraplimit", kTileValue_WrapLimit);
    Tile::AddToTable("wraplines", kTileValue_WrapLines);
    Tile::AddToTable("wrapwidth", kTileValue_WrapWidth);
    Tile::AddToTable("xbuttona", kTileValue_xButtonA);
    Tile::AddToTable("xbuttonb", kTileValue_xButtonB);
    Tile::AddToTable("xbuttonx", kTileValue_xButtonX);
    Tile::AddToTable("xbuttony", kTileValue_xButtonY);
    Tile::AddToTable("xbuttonlt", kTileValue_xButtonLT);
    Tile::AddToTable("xbuttonrt", kTileValue_xButtonRT);
    Tile::AddToTable("xbuttonlb", kTileValue_xButtonLB);
    Tile::AddToTable("xbuttonrb", kTileValue_xButtonRB);
    Tile::AddToTable("xbuttonstart", kTileValue_xButtonStart);
    Tile::AddToTable("xdefault", kTileValue_xdefault);
    Tile::AddToTable("xup", kTileValue_xUp);
    Tile::AddToTable("xdown", kTileValue_xDown);
    Tile::AddToTable("xleft", kTileValue_xLeft);
    Tile::AddToTable("xright", kTileValue_xRight);
    Tile::AddToTable("x", kTileValue_X);
    Tile::AddToTable("y", kTileValue_Y);
    Tile::AddToTable("zoom", kTileValue_Zoom);
    Tile::AddToTable("resolutionconverter", kTileValue_ResolutionConverter);
    Tile::AddToTable("child", kTileValue_Child);
    Tile::AddToTable("me", kTileValue_Me);
    Tile::AddToTable("io", kTileValue_IO);
    Tile::AddToTable("parent", kTileValue_Parent);
    Tile::AddToTable("screen", kTileValue_Screen);
    Tile::AddToTable("sibling", kTileValue_Sibling);
    Tile::AddToTable("grandparent", kTileValue_Grandparent);
    Tile::AddToTable("globals", kTileValue_Globals);
}

int32_t Tile::AddUserTrait(const char* apString, int32_t aiIndex) {
    Tile::Lock();
    int32_t existing = 0;
    if (Tile::xTextTable.GetAt(apString, existing)) {
        Tile::Unlock();
        return existing;
    }
    if (aiIndex == -1) {
        aiIndex = Tile::iTextTableUserNextID;
        ++Tile::iTextTableUserNextID;
    }
    if (aiIndex >= 10000 && Tile::IsValidUserTraitName(apString)) {
        Tile::AddToTable(apString, aiIndex);
        Tile::Unlock();
        return aiIndex;
    }
    Tile::Unlock();
    return static_cast<int32_t>(0x80000000);
}

int32_t Tile::TextToTrait(const char* apText) {
    Tile::Lock();
    int32_t trait = static_cast<int32_t>(0x80000000);
    Tile::xTextTable.GetAt(apText, trait);
    Tile::Unlock();
    return trait;
}

int32_t Tile::TextToTraitAndParam(const char* apText, char* apParam) {
    if (!apText || !*apText || !apParam) {
        return static_cast<int32_t>(0x80000000);
    }
    int32_t paramPos = -1;
    *apParam = '\0';
    char name[256];
    name[0] = '\0';
    for (int32_t i = 0; i < 0xFF && apText[i]; ++i) {
        apParam[i + 1] = '\0';
        name[i + 1] = '\0';
        if (apText[i] == '(') {
            paramPos = 0;
        }
        else if (apText[i] == ')') {
            i = 0x100;
        }
        else if (paramPos >= 0) {
            apParam[paramPos++] = apText[i];
        }
        else {
            name[i] = apText[i];
        }
    }
    return Tile::TextToTrait(name);
}

const char* Tile::TraitToText(int32_t aiTrait) {
    NiTMapItem<const char*, int32_t>* position = Tile::xTextTable.GetFirstPos();
    while (position) {
        const char* text;
        int32_t trait;
        Tile::xTextTable.GetNext(position, text, trait);
        if (trait == aiTrait) {
            return text;
        }
    }
    return nullptr;
}

void Tile::ValueChangeEvent(Value* apChangedVal) {
    Reaction* pReaction = nullptr;
    if (Tile::ValueReactionList.GetAt(apChangedVal, pReaction)) {
        while (pReaction) {
            pReaction->preactionValue->CalculateValue(false);
            pReaction = pReaction->pnext;
        }
    }
}

Tile* Tile::GetTileFromNode(NiNode* apNode) {
    Tile::Lock();
    Tile* tile = nullptr;
    if (apNode) {
        NiPointer<Extra> spExtra = static_cast<Extra*>(apNode->GetExtraData(*Menu::pTileptrStr));
        if (!spExtra && apNode->m_pkParent) {
            spExtra = static_cast<Extra*>(apNode->m_pkParent->GetExtraData(*Menu::pTileptrStr));
        }
        if (spExtra) {
            tile = spExtra->pTile;
        }
    }
    Tile::Unlock();
    return tile;
}

// A hand-written XML scanner. States: 0 before the first tag, 1 tag name, 2 after a '/' that closes a tag, 3 between
// tags, 4 comment, 5 attribute name, 6 attribute value. Every tag end bumps the line count the warnings report. On an
// error the storage and the XML stay allocated, deliberately, as the game does.
Tile::BuildStorage* Tile::ParseFile(const char* apName, char* apData, uint32_t apSize) {
    BuildStorage* build = nullptr;
    void* memory = MemoryManager_Singleton.Allocate(sizeof(BuildStorage));
    build = memory ? ::new (memory) BuildStorage : nullptr;
    XMLStorage* xml = nullptr;
    const char* text = nullptr;
    uint32_t size = 0;
    // A word longer than this overruns the word buffer; this only warns, as the game does.
    const int32_t maxLength = 0x1000;
    int32_t length = 0;
    if (apData) {
        text = apData;
        size = apSize;
    }
    else {
        xml = Menu::GetXML(apName, false);
        if (xml) {
            Tile::InsertIncludedFiles(xml);
            text = xml->pXMLData;
            size = xml->iFileSize;
        }
    }
    char word[0x1000];
    word[0] = '\0';
    int32_t state = 0;
    int32_t trait = 0;
    bool inWord = false;
    bool inQuote = false;
    bool closeTag = false;
    int32_t line = 1;
    char tagName[0x80];
    char expanded[0x1000];
    for (uint32_t i = 0; i < size; ++i) {
        if (state == 0) {
            if (text[i] == '<') {
                if (i + 3 < size && text[i + 1] == '!' && text[i + 2] == '-' && text[i + 3] == '-') {
                    inWord = false;
                    state = 4;
                }
                else {
                    state = 1;
                    tagName[0] = '\0';
                    closeTag = false;
                    length = 0;
                }
            }
        }
        else if (state == 1) {
            if (!tagName[0] && text[i] == '/') {
                closeTag = true;
            }
            else if (text[i] == '>' || (i + 1 < size && text[i] == '/' && text[i + 1] == '>')) {
                if (tagName[0] && closeTag) {
                    build->GetCurrentTemplate()->AddPair(1, tagName, line, false);
                }
                else if (tagName[0] && !closeTag) {
                    build->GetCurrentTemplate()->AddPair(0, tagName, line, false);
                }
                else if (!inWord) {
                    Warning("MENUS: (XML ERROR) %s -- in file '%s' on line %i.", "Empty tag name", apName, line);
                    bReadFileError = true;
                    return nullptr;
                }
                length = 0;
                state = text[i] == '/' ? 2 : 3;
                inWord = false;
                ++line;
            }
            else if (static_cast<uint8_t>(text[i]) <= 0x20) {
                if (tagName[0]) {
                    if (closeTag) {
                        build->GetCurrentTemplate()->AddPair(1, tagName, line, false);
                    }
                    else {
                        build->GetCurrentTemplate()->AddPair(0, tagName, line, false);
                    }
                    length = 0;
                    inWord = true;
                    ++line;
                }
            }
            else if (inWord) {
                --i;
                state = 5;
                inWord = false;
            }
            else {
                tagName[length] = text[i];
                ++length;
                tagName[length] = '\0';
            }
        }
        else if (state == 4) {
            if (i + 2 < size && text[i] == '-' && text[i + 1] == '-' && text[i + 2] == '>') {
                i += 2;
                state = 3;
                inWord = false;
            }
        }
        else if (state == 5) {
            if (text[i] == '>' || (i + 1 < size && text[i] == '/' && text[i + 1] == '>')) {
                Warning("MENUS: (XML ERROR) %s -- in file '%s' on line %i.", "Attribute with no value", apName, line);
                bReadFileError = true;
                return nullptr;
            }
            if (text[i] == '=') {
                if (!word[0]) {
                    Warning(
                        "MENUS: (XML ERROR) %s -- in file '%s' on line %i.", "Missing attribute name", apName, line
                    );
                    bReadFileError = true;
                    return nullptr;
                }
                trait = Tile::TextToTrait(word);
                word[0] = '\0';
                length = 0;
                state = 6;
                inWord = false;
            }
            else if (static_cast<uint8_t>(text[i]) <= 0x20) {
                if (word[0]) {
                    trait = Tile::TextToTrait(word);
                    word[0] = '\0';
                    length = 0;
                    inWord = true;
                }
            }
            else if (inWord) {
                Warning(
                    "MENUS: (XML ERROR) %s -- in file '%s' on line %i.", "Unexpected word after attribute name", apName,
                    line
                );
                bReadFileError = true;
                return nullptr;
            }
            else {
                word[length] = text[i];
                ++length;
                word[length] = '\0';
                if (length > maxLength) {
                    Warning("MENUS: XML Read buffer too small!");
                }
            }
        }
        else if (state == 6) {
            if (text[i] == '"') {
                inQuote = !inQuote;
            }
            else if (inQuote) {
                word[length] = text[i];
                ++length;
                word[length] = '\0';
                if (length > maxLength) {
                    Warning("MENUS: XML Read buffer too small!");
                }
            }
            else if (text[i] == '>' || (i + 1 < size && text[i] == '/' && text[i + 1] == '>')) {
                if (word[0]) {
                    build->GetCurrentTemplate()->AddPair(trait, word, line, false);
                }
                else if (!inWord) {
                    Warning(
                        "MENUS: (XML ERROR) %s -- in file '%s' on line %i.", "Missing attribute's value", apName, line
                    );
                    bReadFileError = true;
                    return nullptr;
                }
                word[0] = '\0';
                length = 0;
                state = text[i] == '/' ? 2 : 3;
                inWord = false;
            }
            else if (static_cast<uint8_t>(text[i]) <= 0x20) {
                if (word[0]) {
                    build->GetCurrentTemplate()->AddPair(trait, word, line, false);
                    word[0] = '\0';
                    length = 0;
                    inWord = true;
                }
            }
            else if (inWord) {
                --i;
                state = 5;
                inWord = false;
            }
            else {
                word[length] = text[i];
                ++length;
                word[length] = '\0';
                if (length > maxLength) {
                    Warning("MENUS: XML Read buffer too small!");
                }
            }
        }
        else if (state == 2) {
            if (text[i] == '>') {
                build->GetCurrentTemplate()->AddPair(1, tagName, line, false);
                state = 3;
                tagName[0] = '\0';
            }
            else {
                Warning(
                    "MENUS: (XML ERROR) %s -- in file '%s' on line %i.",
                    "Close-tag marker '/' not followed by end-of-tag marker '>'", apName, line
                );
                bReadFileError = true;
                return nullptr;
            }
        }
        else if (state == 3) {
            if (text[i] == '<') {
                if (i + 3 < size && text[i + 1] == '!' && text[i + 2] == '-' && text[i + 3] == '-') {
                    inWord = false;
                    state = 4;
                }
                else {
                    if (word[0]) {
                        // Trims trailing whitespace. There is no lower bound on length; word[0] is never whitespace, so
                        // the scan stops there.
                        length = static_cast<int32_t>(std::strlen(word)) - 1;
                        while (static_cast<uint8_t>(word[length]) <= 0x20) {
                            --length;
                        }
                        word[length + 1] = '\0';
                        if (word[0] == '&') {
                            if (word[1] == '-') {
                                if (Interface::TestConstantForGameSettings(word, expanded)) {
                                    build->GetCurrentTemplate()->AddPair(kTileValue_Value, expanded, line, false);
                                }
                            }
                            else {
                                build->GetCurrentTemplate()->AddPair(kTileValue_Value, word, line, false);
                            }
                        }
                        else if (word[0]) {
                            build->GetCurrentTemplate()->AddPair(kTileValue_Value, word, line, true);
                        }
                    }
                    word[0] = '\0';
                    length = 0;
                    inWord = false;
                    closeTag = false;
                    state = 1;
                    tagName[0] = '\0';
                }
            }
            else if (i + 1 < size && text[i] == '/' && text[i + 1] == '>') {
                Warning(
                    "MENUS: (XML ERROR) %s -- in file '%s' on line %i.", "Unballanced close-tag marker pair '/>' found",
                    apName, line
                );
                bReadFileError = true;
                return nullptr;
            }
            else if (text[i] == '>') {
                Warning(
                    "MENUS: (XML ERROR) %s -- in file '%s' on line %i.", "Unballanced end-of-tag marker '>' found",
                    apName, line
                );
                bReadFileError = true;
                return nullptr;
            }
            else if (static_cast<uint8_t>(text[i]) > 0x20 || inWord) {
                word[length] = text[i];
                ++length;
                word[length] = '\0';
                if (length > maxLength) {
                    Warning("MENUS: XML Read buffer too small!");
                }
                inWord = true;
            }
        }
    }
    if (xml) {
        delete xml;
    }
    return build;
}

bool Tile::InsertIncludedFiles(XMLStorage* apFile) {
    char* ptokenContext;
    char* pstartIndex = apFile->pXMLData;
    const char* pincludeIndex = std::strstr(pstartIndex, "<include");
    if (!pincludeIndex) {
        return false;
    }
    BSStringT<char> strfinalVersion;
    uint32_t strtempSize = apFile->iFileSize + 1;
    char* strtemp = static_cast<char*>(MemoryManager_Singleton.Allocate(strtempSize));
    std::memset(strtemp, 0, strtempSize);
    strncpy_s(
        strtemp, static_cast<size_t>(pincludeIndex - pstartIndex + 1), pstartIndex,
        static_cast<size_t>(pincludeIndex - pstartIndex)
    );
    strfinalVersion.Set(strtemp, 0x4000);
    bool bdone = false;
    while (!bdone) {
        BSStringT<char> strincludeFilename;
        int32_t iincludeTagLength = std::strchr(pincludeIndex, '>') - pincludeIndex + 1;
        std::memset(strtemp, 0, strtempSize);
        // pincludeIndex is const char*, but strtok_s cuts the XML data it points into in place, hence the const_cast.
        strtok_s(const_cast<char*>(pincludeIndex), "\"", &ptokenContext);
        strincludeFilename.SPrintF("Data\\Menus\\Prefabs\\%s", strtok_s(nullptr, "\"", &ptokenContext));
        XMLStorage* ptmpXML = Menu::GetXML(strincludeFilename.pString, false);
        strfinalVersion += static_cast<const char*>(ptmpXML->pXMLData);
        if (ptmpXML) {
            ptmpXML->~XMLStorage();
            MemoryManager_Singleton.Deallocate(ptmpXML);
        }
        const char* pincludeIndex2 = std::strstr(&pincludeIndex[iincludeTagLength], "<include");
        std::memset(strtemp, 0, strtempSize);
        if (pincludeIndex2) {
            strncpy_s(
                strtemp, static_cast<size_t>(pincludeIndex2 - pincludeIndex - iincludeTagLength + 1),
                &pincludeIndex[iincludeTagLength],
                static_cast<size_t>(pincludeIndex2 - pincludeIndex - iincludeTagLength)
            );
            pincludeIndex = pincludeIndex2;
        }
        else {
            strcpy_s(strtemp, strtempSize, &pincludeIndex[iincludeTagLength]);
            bdone = true;
        }
        strfinalVersion += static_cast<const char*>(strtemp);
    }
    MemoryManager_Singleton.Deallocate(apFile->pXMLData);
    apFile->pXMLData = static_cast<char*>(MemoryManager_Singleton.Allocate(strfinalVersion.Length() + 1));
    strcpy_s(apFile->pXMLData, strfinalVersion.Length() + 1, strfinalVersion.pString);
    apFile->iFileSize = strfinalVersion.Length();
    MemoryManager_Singleton.Deallocate(strtemp);
    Tile::InsertIncludedFiles(apFile);
    return true;
}

// Looks the subtexture strSubtextureFilename (its name after the last backslash) up among the loaded atlas entries, and
// loads the atlas file Data\Textures\<strAtlasFilename> when it has not been loaded: each line that does not start with
// '#' or white space becomes an entry (subtexture name, atlas texture path, x, y, width, height). The match's texture
// path and rectangle go to strAtlasTextureFilename and ResultRect.
void Tile::GetTextureAtlasInfo(
    const char* strAtlasFilename,
    const char* strSubtextureFilename,
    BSStringT<char>* strAtlasTextureFilename,
    NiRect<float>* ResultRect
) {
    char acPath[260];
    strcpy_s(acPath, 260, "Data\\Textures\\");
    BSFile* pFile = nullptr;
    if (!strAtlasFilename || !strSubtextureFilename || (!strAtlasTextureFilename && !ResultRect)) {
        return;
    }
    const char* pSlash = std::strrchr(strSubtextureFilename, '\\');
    bool bFound = false;
    bool bAtlasLoaded = false;
    strcat_s(acPath, 260, strAtlasFilename);
    strSubtextureFilename = pSlash ? pSlash + 1 : strSubtextureFilename;
    for (BSSimpleList<TextureAtlasEntry*>* pNode = &Tile::TextureEntryList; pNode && pNode->m_item;
         pNode = pNode->m_pkNext) {
        if (!std::strcmp(pNode->m_item->strAtlasFilename.pString, strAtlasFilename)) {
            bAtlasLoaded = true;
        }
        if (!std::strcmp(pNode->m_item->strSubtexture.pString, strSubtextureFilename)) {
            if (strAtlasTextureFilename) {
                strAtlasTextureFilename->Set(pNode->m_item->strAtlasTexture.pString, 0);
            }
            if (ResultRect) {
                ResultRect->m_left = pNode->m_item->SubRect.m_left;
                ResultRect->m_bottom = pNode->m_item->SubRect.m_bottom;
                ResultRect->m_top = pNode->m_item->SubRect.m_top;
                ResultRect->m_right = pNode->m_item->SubRect.m_right;
            }
            bFound = true;
        }
    }
    // Tests bFound twice: a found entry, or an atlas already loaded, ends the lookup.
    if (bFound || (!bFound && bAtlasLoaded)) {
        return;
    }
    pFile = static_cast<BSFile*>(FileFinder::GetFile(acPath, NiFile::READ_ONLY, 0x4000));
    if (pFile && pFile->Open(false, false)) {
        char acLine[0x400];
        while (pFile->GetLine(acLine, 0x400, '\n')) {
            std::string kName;
            TextureAtlasEntry* pEntry = nullptr;
            std::strtok(acLine, " ,\t");
            if (acLine[0] == '#' || acLine[0] == '\n' || acLine[0] == '\t' || acLine[0] == '\r' || acLine[0] == ' ') {
                continue;
            }
            void* pMemory = MemoryManager_Singleton.Allocate(sizeof(TextureAtlasEntry));
            pEntry = pMemory ? ::new (pMemory) TextureAtlasEntry : nullptr;
            pEntry->strAtlasFilename.Set(strAtlasFilename, 0);
            pEntry->strSubtexture.Set(acLine, 0);
            kName.assign(strAtlasFilename, std::strlen(strAtlasFilename));
            char cSlash = '\\';
            size_t uiSlash = kName.rfind(&cSlash, std::string::npos, 1);
            std::string kDir;
            kDir = kName.substr(0, uiSlash + 1);
            kName.assign(kDir, 0, std::string::npos);
            char* pTexture = std::strtok(nullptr, " ,\t");
            kName.append(pTexture, std::strlen(pTexture));
            pEntry->strAtlasTexture.Set(kName.c_str(), 0);
            for (int32_t i = 0; i < 2; ++i) {
                std::strtok(nullptr, " ,\t");
            }
            pEntry->SubRect.m_left = static_cast<float>(std::atof(std::strtok(nullptr, " ,\t")));
            pEntry->SubRect.m_top = static_cast<float>(std::atof(std::strtok(nullptr, " ,\t")));
            std::strtok(nullptr, " ,");
            pEntry->SubRect.m_right = static_cast<float>(std::atof(std::strtok(nullptr, " ,\t")));
            pEntry->SubRect.m_bottom = static_cast<float>(std::atof(std::strtok(nullptr, " ,\t")));
            pEntry->SubRect.m_right = pEntry->SubRect.m_right + pEntry->SubRect.m_left;
            pEntry->SubRect.m_bottom = pEntry->SubRect.m_bottom + pEntry->SubRect.m_top;
            if (!std::strcmp(pEntry->strSubtexture.pString, strSubtextureFilename)) {
                if (strAtlasTextureFilename) {
                    strAtlasTextureFilename->Set(pEntry->strAtlasTexture.pString, 0);
                }
                if (ResultRect) {
                    ResultRect->m_left = pEntry->SubRect.m_left;
                    ResultRect->m_bottom = pEntry->SubRect.m_bottom;
                    ResultRect->m_top = pEntry->SubRect.m_top;
                    ResultRect->m_right = pEntry->SubRect.m_right;
                }
            }
            Tile::TextureEntryList.AddHead(pEntry);
        }
    }
    if (pFile) {
        delete pFile;
    }
}

void Tile::FreeMaybePostUpdateList() {
    (&Tile::xHibernatingTiles)->RemoveAll();
}

void Tile::FreeTraitList() {
    Tile::Lock();
    (&Tile::xTextTable)->RemoveAll_0();
    (&Tile::xTraitExtraData)->RemoveAll_0();
    // The atlas entries go one by one: the destructor, then MemoryManager::Deallocate.
    for (BSSimpleList<TextureAtlasEntry*>* node = &Tile::TextureEntryList; node && node->m_item;
         node = node->m_pkNext) {
        TextureAtlasEntry* entry = node->m_item;
        entry->~TextureAtlasEntry();
        (&MemoryManager_Singleton)->Deallocate(entry);
    }
    (&Tile::TextureEntryList)->RemoveAll();
    for (BSSimpleList<FadeControl*>* node = &Tile::xFadeControls; node && node->m_item; node = node->m_pkNext) {
        (&MemoryManager_Singleton)->Deallocate(node->m_item);
    }
    (&Tile::xFadeControls)->RemoveAll();
    Tile::Unlock();
}

void Tile::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void Tile::AddReaction(Tile::Value* apRefVal, Tile::Value* apDependantVal) {
    Tile::Reaction* pnewReaction_2;
    Tile::Reaction* pnewReaction =
        static_cast<Tile::Reaction*>(MemoryManager_Singleton.Allocate(sizeof(Tile::Reaction)));
    if (pnewReaction) {
        pnewReaction->preactionValue = nullptr;
        pnewReaction->pnext = nullptr;
        pnewReaction_2 = pnewReaction;
    }
    else {
        pnewReaction_2 = nullptr;
    }
    Tile::Reaction* pfoundReactionListA = nullptr;
    pnewReaction_2->preactionValue = apDependantVal;
    if (Tile::ValueReactionList.GetAt(apRefVal, pfoundReactionListA)) {
        bool bfound = pfoundReactionListA->preactionValue == apDependantVal;
        while (pfoundReactionListA->pnext) {
            pfoundReactionListA = pfoundReactionListA->pnext;
            bfound = pfoundReactionListA->preactionValue == apDependantVal;
        }
        if (bfound) {
            MemoryManager_Singleton.Deallocate(pnewReaction_2);
        }
        else {
            pfoundReactionListA->pnext = pnewReaction_2;
        }
    }
    else {
        Tile::ValueReactionList.SetAt(apRefVal, pnewReaction_2);
    }
}

void au_re_LeaveCriticalSection_1_0() {
    Tile::Unlock();
}

Tile* FindTileByID(Tile* th_s, int32_t a2) {
    if (th_s && static_cast<double>(a2) == th_s->GetFloat(4010)) {
        return th_s;
    }
    NiTListItem<Tile*>* m_pkHead = th_s->xChildren.m_pkHead;
    while (m_pkHead) {
        Tile** p_m_element = &m_pkHead->m_element;
        m_pkHead = m_pkHead->m_pkNext;
        Tile* v4 = FindTileByID(*p_m_element, a2);
        if (v4) {
            return v4;
        }
    }
    return nullptr;
}

void RemoveAllAudioDebugStrings(void* apThis, [[maybe_unused]] void* apEdx) {
    static_cast<BSSimpleList<AudioDebugString*>*>(apThis)->RemoveAll();
}

Tile::Action::Action() : eActionType(kTileValue_Copy), pnext(nullptr) {}

Tile::FloatAction::FloatAction() {}

float Tile::FloatAction::QFloat() {
    return fValue;
}

// A constant operand refers to no value.
Tile::Value* Tile::FloatAction::QRefValue() {
    return nullptr;
}

Tile::RefValueAction::RefValueAction() {}

float Tile::RefValueAction::QFloat() {
    if (pRefValue) {
        return pRefValue->fValue;
    }

    return 0.0;
}

Tile::Value* Tile::RefValueAction::QRefValue() {
    return pRefValue;
}

Tile::Extra::~Extra() {
    if (pTile) {
        pTile->pParent = Tile::GetTileFromNode(pNode->m_pkParent);
        pTile->spModel = nullptr;
    }
    pTile = nullptr;
}

Tile::MenuStringMap::~MenuStringMap() {}

uint32_t Tile::MenuStringMap::KeyToHashIndex(const char* apKey) const {
    uint32_t iretVal = 0;
    while (*apKey) {
        iretVal = static_cast<char>(std::tolower(*apKey++)) + 33 * iretVal;
    }
    return iretVal % m_uiHashSize;
}

bool Tile::MenuStringMap::IsKeysEqual(const char* apKey1, const char* apKey2) const {
    return _stricmp(apKey1, apKey2) == 0;
}

void Tile::MenuStringMap::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}

Tile3D::Tile3D() : pAniManager(nullptr), pCurrentSequence(nullptr) {}

Tile3D::~Tile3D() {
    if ((uiFlags & MODEL_FROM_MODEL_LOADER) != 0) {
        Interface::ReleaseModelFile(strFilename.pString);
    }
    if ((uiFlags & RELEASED) == 0) {
        Release();
    }
}

const char* Tile3D::GetTypeName() {
    return "3DAN";
}

TileText::TileText() : bIsHyper(false) {}

TileText::~TileText() {
    if ((uiFlags & RELEASED) == 0) {
        Release();
    }
}

const char* TileText::GetTypeName() {
    return "TEXT";
}

void TileText::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}
