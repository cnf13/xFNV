#include "Fallout Shared/TESRecipe.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/RecipeMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/tchar.h"
#include "xfnv/Compiler.h"

void RECIPE_DATA::Endian() {
    BSEndian(ReqSkillId, false);
    BSEndian(ReqSkillLevel, false);
    BSEndian(CategoryID, false);
    BSEndian(SubCategoryID, false);
}

TESRecipeComponent::TESRecipeComponent() : Quantity(0) {}

uint32_t TESRecipeComponent::GetQuantity() {
    return Quantity;
}

void TESRecipeComponent::SetQuantity(uint32_t aquant) {
    Quantity = aquant;
}

void TESRecipeComponent::SetComponentPointer(TESForm* apPointer) {
    pInternalItem = apPointer;
}

TESForm* TESRecipeComponent::GetComponentPointer() {
    return pInternalItem;
}

bool TESRecipeComponent::Compare(TESRecipeComponent* apCompare) {
    if (Quantity != apCompare->Quantity) {
        return true;
    }
    return pInternalItem != apCompare->pInternalItem;
}

TESRecipe::TESRecipe() {
    SetFormType(RCPE_ID);
}

TESRecipe::~TESRecipe() {
    TESRecipe::ClearData();
}

void TESRecipe::ClearData() {
    RecipeConditions.ClearAllConditionItems();
    ingredientlist.RemoveAll();
    outputlist.RemoveAll();
    garbageCollection.RemoveAll();
    data.ReqSkillId = -1;
    data.ReqSkillLevel = 0;
    pRecipeCat = nullptr;
    pRecipeSubCat = nullptr;
}

// When the EDID chunk is empty (malformed data), its buffer is initialised to "" (one zeroed byte, allocated only for
// an empty chunk). RCPE. An RCIL or RCOD chunk starts a new component holding the item's form ID; RCQY sets the current
// component's quantity. After every chunk, a current component with a quantity and an item ID gets the ID and is
// appended to the ingredient (RCIL) or output (RCOD) list.
bool TESRecipe::Load(TESFile* apFile) {
    uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
    ClearData();
    if (cFormType != RCPE_ID) {
        return false;
    }
    LoadForm(apFile);
    SetInitialized(false);
    TESRecipeComponent* pComponent = nullptr;
    uint32_t uiItemID = 0;
    bool bOutput = false;
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // An empty chunk leaves the buffer unwritten, so it is cleared first.
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case DATA_ID:
                LoadData(apFile, static_cast<char*>(static_cast<void*>(&data)), sizeof(data));
                if (apFile->QEndian()) {
                    data.Endian();
                }
                break;
            case CIDA_ID: GetConditions()->Load(apFile); break;
            case RCIL_ID: {
                bOutput = false;
                pComponent = new TESRecipeComponent;
                uint32_t uiID = 0;
                apFile->GetChunkData(uiID);
                TESForm::AddCompileIndex(uiID, GetFile(-1));
                uiItemID = uiID;
                break;
            }
            case RCOD_ID: {
                bOutput = true;
                pComponent = new TESRecipeComponent;
                uint32_t uiID = 0;
                apFile->GetChunkData(uiID);
                TESForm::AddCompileIndex(uiID, GetFile(-1));
                uiItemID = uiID;
                break;
            }
            case RCQY_ID: {
                uint32_t uiQuantity = 0;
                apFile->GetChunkData(uiQuantity);
                pComponent->SetQuantity(uiQuantity);
                break;
            }
            default: break;
        }
        if (pComponent && pComponent->GetQuantity() && uiItemID) {
            // The form ID stands in for the item until the forms are resolved.
            pComponent->SetComponentPointer(reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiItemID)));
            if (bOutput) {
                GetOutputList()->AddTail(pComponent);
            }
            else {
                GetIngredientsList()->AddTail(pComponent);
            }
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESRecipe::Save() {
    StartForm();
    TESFullName::Save();
    RecipeConditions.Save();
    data.CategoryID = pRecipeCat ? pRecipeCat->iFormID : 0;
    data.SubCategoryID = GetSubCategory() ? GetSubCategory()->iFormID : 0;
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    __SaveData(&data, sizeof(data));
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    BSSimpleList<TESRecipeComponent*>* pCompList = &ingredientlist;
    while (pCompList && pCompList->m_item) {
        TESRecipeComponent* pComponent = pCompList->m_item;
        pCompList = pCompList->m_pkNext;
        TESForm::AddChunk(RCIL_ID, pComponent->pInternalItem->iFormID);
        TESForm::AddChunk(RCQY_ID, pComponent->Quantity);
    }
    pCompList = &outputlist;
    while (pCompList && pCompList->m_item) {
        TESRecipeComponent* pComponent = pCompList->m_item;
        pCompList = pCompList->m_pkNext;
        TESForm::AddChunk(RCOD_ID, pComponent->pInternalItem->iFormID);
        TESForm::AddChunk(RCQY_ID, pComponent->Quantity);
    }
    CloseForm();
}

void TESRecipe::InitItem() {
    if (!GetInitialized()) {
        RecipeConditions.InitItem(this);
        uint32_t tempint = data.CategoryID;
        if (tempint) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(tempint, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(tempint);
            TESRecipeCategory* pCat = dynamic_cast<TESRecipeCategory*>(pForm);
            if (pCat) {
                pRecipeCat = pCat;
            }
            else {
                const char* pEditorID = GetFormEditorID();
                int32_t iID = iFormID;
                Warning(
                    "MASTERFILE: Recipe Category (%08X) not found in InitItem for Recipe (%08X) '%s'.",
                    tempint,
                    iID,
                    pEditorID
                );
            }
        }
        tempint = data.SubCategoryID;
        if (tempint) {
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(tempint, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(tempint);
            TESRecipeCategory* pCat = dynamic_cast<TESRecipeCategory*>(pForm);
            if (pCat) {
                pRecipeSubCat = pCat;
            }
            else {
                const char* pEditorID = GetFormEditorID();
                int32_t iID = iFormID;
                Warning(
                    "MASTERFILE: Recipe (sub)Category (%08X) not found in InitItem for Recipe (%08X) '%s'.",
                    tempint,
                    iID,
                    pEditorID
                );
            }
        }
        BSSimpleList<TESRecipeComponent*>* pCompList = &ingredientlist;
        while (pCompList && pCompList->m_item) {
            TESRecipeComponent* pComponent = pCompList->m_item;
            pCompList = pCompList->m_pkNext;
            // Load keeps the item's form ID in pInternalItem until here, so the TESForm* is read back as a uint32_t.
            pComponent->pInternalItem = TESForm::GetFormByNumericID(
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pComponent->GetComponentPointer()))
            );
        }
        pCompList = &outputlist;
        while (pCompList && pCompList->m_item) {
            TESRecipeComponent* pComponent = pCompList->m_item;
            pCompList = pCompList->m_pkNext;
            // Load keeps the item's form ID in pInternalItem until here, so the TESForm* is read back as a uint32_t.
            pComponent->pInternalItem = TESForm::GetFormByNumericID(
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pComponent->GetComponentPointer()))
            );
        }
        SetInitialized(true);
    }
}

void TESRecipe::Copy(TESForm* apCopy) {
    TESRecipe* pmisc = dynamic_cast<TESRecipe*>(apCopy);
    if (pmisc) {
        RecipeConditions.Copy(&pmisc->RecipeConditions, nullptr);
        ourMemcpy(&data, &pmisc->data, sizeof(data));
        CopyAllComponents(apCopy);
        while (!ingredientlist.IsEmpty()) {
            ingredientlist.RemoveHead();
        }
        for (BSSimpleList<TESRecipeComponent*>* pList = &pmisc->ingredientlist; pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            TESRecipeComponent* pComp = pList->m_item;
            TESRecipeComponent* pNew = new TESRecipeComponent;
            pNew->Quantity = pComp->Quantity;
            pNew->pInternalItem = pComp->pInternalItem;
            ingredientlist.AddTail(pNew);
        }
        while (!outputlist.IsEmpty()) {
            outputlist.RemoveHead();
        }
        for (BSSimpleList<TESRecipeComponent*>* pList = &pmisc->outputlist; pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            TESRecipeComponent* pComp = pList->m_item;
            TESRecipeComponent* pNew = new TESRecipeComponent;
            pNew->Quantity = pComp->Quantity;
            pNew->pInternalItem = pComp->pInternalItem;
            outputlist.AddTail(pNew);
        }
        data.ReqSkillId = pmisc->data.ReqSkillId;
        data.ReqSkillLevel = pmisc->data.ReqSkillLevel;
        pRecipeCat = pmisc->pRecipeCat;
        pRecipeSubCat = pmisc->pRecipeSubCat;
    }
}

bool TESRecipe::Compare(TESForm* apCompare) {
    TESRecipe* pcomp = dynamic_cast<TESRecipe*>(apCompare);
    if (!pcomp) {
        return true;
    }
    if (pRecipeCat != pcomp->pRecipeCat) {
        return true;
    }
    TESRecipeCategory* pSubCategory = GetSubCategory();
    if (pSubCategory != pcomp->GetSubCategory()) {
        return true;
    }
    ActorValue::Index eRequiredSkill = GetRequiredSkill();
    if (eRequiredSkill != pcomp->GetRequiredSkill()) {
        return true;
    }
    if (data.ReqSkillLevel != pcomp->data.ReqSkillLevel) {
        return true;
    }
    if (CompareAllComponents(pcomp)) {
        return true;
    }
    if (RecipeConditions.Compare(&pcomp->RecipeConditions)) {
        return true;
    }
    int32_t iIngredientCount = ingredientlist.ItemsInList();
    if (iIngredientCount != pcomp->ingredientlist.ItemsInList()) {
        return true;
    }
    int32_t iOutputCount = outputlist.ItemsInList();
    if (iOutputCount != pcomp->outputlist.ItemsInList()) {
        return true;
    }
    BSSimpleList<TESRecipeComponent*>* ingredients = &ingredientlist;
    BSSimpleList<TESRecipeComponent*>* copyingredients = &pcomp->ingredientlist;
    while (copyingredients && !copyingredients->IsEmpty()) {
        if (!ingredients || ingredients->IsEmpty()) {
            return true;
        }
        TESRecipeComponent* pRecipeComponent = ingredients->m_item;
        TESRecipeComponent* pRecipeComponentCopy = copyingredients->m_item;
        if (pRecipeComponent->Compare(pRecipeComponentCopy)) {
            return true;
        }
        copyingredients = copyingredients->m_pkNext;
        ingredients = ingredients->m_pkNext;
    }
    BSSimpleList<TESRecipeComponent*>* output = &outputlist;
    BSSimpleList<TESRecipeComponent*>* copyoutput = &pcomp->outputlist;
    while (copyoutput && !copyoutput->IsEmpty()) {
        if (!output || output->IsEmpty()) {
            return true;
        }
        TESRecipeComponent* pRecipeComponent = output->m_item;
        TESRecipeComponent* pRecipeComponentCopy = copyoutput->m_item;
        if (pRecipeComponent->Compare(pRecipeComponentCopy)) {
            return true;
        }
        copyoutput = copyoutput->m_pkNext;
        output = output->m_pkNext;
    }
    return false;
}

TESCondition* TESRecipe::GetConditions() {
    return &RecipeConditions;
}

BSSimpleList<TESRecipeComponent*>* TESRecipe::GetIngredientsList() {
    return &ingredientlist;
}

BSSimpleList<TESRecipeComponent*>* TESRecipe::GetOutputList() {
    return &outputlist;
}

ActorValue::Index TESRecipe::GetRequiredSkill() {
    return static_cast<ActorValue::Index>(data.ReqSkillId);
}

int32_t TESRecipe::GetRequiredSkillLevel() {
    return data.ReqSkillLevel;
}

TESRecipeCategory* TESRecipe::GetCategory() {
    return pRecipeCat;
}

TESRecipeCategory* TESRecipe::GetSubCategory() {
    return pRecipeSubCat;
}

int32_t TESRecipe::CompareForSorting(ListBoxItem<TESRecipe*>* apItem1, ListBoxItem<TESRecipe*>* apItem2) {
    const char* pName1 = apItem1->data->GetFullName();
    const char* pName2 = apItem2->data->GetFullName();
    int32_t iCanMake1 = RecipeMenu::CanMakeRecipe(apItem1->data);
    int32_t iCanMake2 = RecipeMenu::CanMakeRecipe(apItem2->data);
    if ((iCanMake1 > 0 && iCanMake2 > 0) || (iCanMake1 <= 0 && iCanMake2 <= 0)) {
        return _tcscmp(pName1, pName2);
    }
    if (iCanMake1 > 0) {
        return -1;
    }
    if (iCanMake2 <= 0) {
        return _tcscmp(pName1, pName2);
    }
    return 1;
}

bool TESRecipe::ShouldHideItem(TESRecipe* apRecipe) {
    if (g_currentRecipeMenuCategory == -1) {
        return false;
    }
    TESRecipeCategory* pSubCategory = apRecipe->GetSubCategory();
    return pSubCategory != (*get_g_RecipeMenuCategories())[static_cast<uint32_t>(g_currentRecipeMenuCategory)];
}

void RECIPE_CATEGORY_DATA::Endian() {}

TESRecipeCategory::TESRecipeCategory() {
    SetFormType(RCCT_ID);
    ResetFlag(FLAG_SUBCATEGORY);
}

TESRecipeCategory::~TESRecipeCategory() {}

// RCCT. The DATA chunk is one flag byte; the engine's empty Endian is called on it after reading.
bool TESRecipeCategory::Load(TESFile* apFile) {
    uint8_t cFormType = static_cast<uint8_t>(apFile->GetTESForm());
    if (cFormType != RCCT_ID) {
        return false;
    }
    LoadForm(apFile);
    SetInitialized(false);
    for (;;) {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case DATA_ID:
                // The DATA chunk is a raw 1-byte buffer.
                LoadData(apFile, static_cast<char*>(static_cast<void*>(&data)), sizeof(data));
                if (apFile->QEndian()) {
                    data.Endian();
                }
                break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            default: break;
        }
        if (!apFile->NextChunk()) {
            break;
        }
    }
    return true;
}

// Writes the FULL name and the DATA chunk, swapped around the write when saving byte-swapped.
void TESRecipeCategory::Save() {
    StartForm();
    TESFullName::Save();
    if (QEndianSwapOnSave()) {
        data.Endian();
    }
    __SaveData(&data, sizeof(data));
    if (QEndianSwapOnSave()) {
        data.Endian();
    }
    CloseForm();
}

void TESRecipeCategory::Copy(TESForm* apCopy) {
    TESRecipeCategory* pCategory = dynamic_cast<TESRecipeCategory*>(apCopy);
    if (pCategory) {
        ourMemcpy(&data, &pCategory->data, sizeof(data));
        TESForm::CopyAllComponents(apCopy);
    }
}

bool TESRecipeCategory::Compare(TESForm* apCompare) {
    TESRecipeCategory* pCategory = dynamic_cast<TESRecipeCategory*>(apCompare);
    if (!pCategory) {
        return true;
    }
    return std::memcmp(&data, &pCategory->data, sizeof(data)) || TESForm::CompareAllComponents(pCategory);
}

void TESRecipeCategory::ResetFlag(uint8_t flag) {
    data.flags &= ~flag;
}
