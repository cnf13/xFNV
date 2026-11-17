#include "Fallout Shared/TESRecipe.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
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

void RECIPE_DATA::Endian() {
	ENDIAN(ReqSkillId, false);
	ENDIAN(ReqSkillLevel, false);
	ENDIAN(CategoryID, false);
	ENDIAN(SubCategoryID, false);
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
	uint32_t Quantity = this->Quantity;
	if (Quantity != apCompare->Quantity) {
		return 1;
	}
	TESForm* ComponentPointer = pInternalItem;
	return ComponentPointer != apCompare->pInternalItem;
}

TESRecipe::TESRecipe() {
	SetFormType(RCPE_ID);
}

TESRecipe::~TESRecipe() {
	TESRecipe::ClearData();
}

void TESRecipe::ClearData() {
	(&RecipeConditions)->ClearAllConditionItems();
	(&ingredientlist)->RemoveAll();
	(&outputlist)->RemoveAll();
	(&garbageCollection)->RemoveAll();
	data.ReqSkillId = -1;
	data.ReqSkillLevel = 0;
	pRecipeCat = 0;
	pRecipeSubCat = 0;
}

// When the EDID chunk is empty (malformed data), its buffer is initialised to "" (one zeroed byte, allocated only for
// an empty chunk). RCPE. An RCIL or RCOD chunk starts a new component holding the item's form ID; RCQY sets the current
// component's quantity. After every chunk, a current component with a quantity and an item ID gets the ID and is
// appended to the ingredient (RCIL) or output (RCOD) list, as the game does.
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
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
			// An empty chunk leaves the buffer unwritten, so it is cleared first.
			pBuffer[0] = '\0';
			apFile->GetChunkData(pBuffer, 0x200);
			SetFormEditorID(pBuffer);
			break;
		}
		case OBND_ID:
			LoadObjectBound(apFile);
			break;
		case FULL_ID:
			TESFullName::LoadFullNameChunk(this, apFile);
			break;
		case DATA_ID:
			LoadData(apFile, static_cast<char*>(static_cast<void*>(&data)), 0x10);
			if (apFile->QEndian())
				data.Endian();
			break;
		case CIDA_ID:
			GetConditions()->Load(apFile);
			break;
		case RCIL_ID: {
			bOutput = false;
			void* pMemory = ourMalloc(sizeof(TESRecipeComponent));
			pComponent = pMemory ? ::new (pMemory) TESRecipeComponent : nullptr;
			uint32_t uiID = 0;
			apFile->GetChunkData(uiID);
			TESForm::AddCompileIndex(uiID, GetFile(-1));
			uiItemID = uiID;
			break;
		}
		case RCOD_ID: {
			bOutput = true;
			void* pMemory = ourMalloc(sizeof(TESRecipeComponent));
			pComponent = pMemory ? ::new (pMemory) TESRecipeComponent : nullptr;
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
		default:
			break;
		}
		if (pComponent && pComponent->GetQuantity() && uiItemID) {
			// The form ID stands in for the item until the forms are resolved.
			pComponent->SetComponentPointer(reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiItemID)));
			if (bOutput) {
				GetOutputList()->AddTail(pComponent);
			} else {
				GetIngredientsList()->AddTail(pComponent);
			}
		}
	} while (apFile->NextChunk());
	return true;
}

void TESRecipe::Save() {
	uint32_t v1;
	uint32_t CastingType;
	StartForm();
	TESFullName::Save();
	(&RecipeConditions)->Save();
	if (pRecipeCat) {
		CastingType = pRecipeCat->iFormID;
	} else {
		CastingType = 0;
	}
	(&data)->CategoryID = CastingType;
	if (GetSubCategory()) {
		v1 = GetSubCategory()->iFormID;
	} else {
		v1 = 0;
	}
	(&data)->SubCategoryID = v1;
	if (TESForm::QEndianSwapOnSave()) {
		data.Endian();
	}
	__SaveData(&data, 16u);
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
	BSSimpleList<TESRecipeComponent*>* pCompList_2 = &outputlist;
	while (pCompList_2 && pCompList_2->m_item) {
		TESRecipeComponent* pComponent_2 = pCompList_2->m_item;
		pCompList_2 = pCompList_2->m_pkNext;
		TESForm::AddChunk(RCOD_ID, pComponent_2->pInternalItem->iFormID);
		TESForm::AddChunk(RCQY_ID, pComponent_2->Quantity);
	}
	CloseForm();
}

void TESRecipe::InitItem() {
	if (!GetInitialized()) {
		RecipeConditions.InitItem(this);
		uint32_t ReqSkillId = (&data)->ReqSkillId;
		uint32_t tempint = (&data)->CategoryID;
		if (tempint) {
			TESFile* File = GetFile(-1);
			TESForm::AddCompileIndex(tempint, File);
			TESForm* FormByNumericID = TESForm::GetFormByNumericID(tempint);
			TESRecipeCategory* pCat = dynamic_cast<TESRecipeCategory*>(FormByNumericID);
			if (pCat) {
				pRecipeCat = pCat;
			} else {
				const char* v7 = GetFormEditorID();
				int32_t CastingType = iFormID;
				Warning("MASTERFILE: Recipe Category (%08X) not found in InitItem for Recipe (%08X) '%s'.", tempint,
					CastingType, v7);
			}
		}
		tempint = (&data)->SubCategoryID;
		if (tempint) {
			TESFile* v3 = GetFile(-1);
			TESForm::AddCompileIndex(tempint, v3);
			TESForm* v4 = TESForm::GetFormByNumericID(tempint);
			TESRecipeCategory* pCat_2 = dynamic_cast<TESRecipeCategory*>(v4);
			if (pCat_2) {
				pRecipeSubCat = pCat_2;
			} else {
				const char* v8 = GetFormEditorID();
				int32_t v6 = iFormID;
				Warning("MASTERFILE: Recipe (sub)Category (%08X) not found in InitItem for Recipe (%08X) '%s'.",
					tempint, v6, v8);
			}
		}
		BSSimpleList<TESRecipeComponent*>* pCompList = &ingredientlist;
		while (pCompList && pCompList->m_item) {
			TESRecipeComponent* pComponent = pCompList->m_item;
			pCompList = pCompList->m_pkNext;
			// Load keeps the item's form ID in pInternalItem until here, so the TESForm* is read back as a uint32_t.
			pComponent->pInternalItem =
				TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pComponent
						->GetComponentPointer())));
		}
		BSSimpleList<TESRecipeComponent*>* pCompList_2 = &outputlist;
		while (pCompList_2 && pCompList_2->m_item) {
			TESRecipeComponent* pComponent_2 = pCompList_2->m_item;
			pCompList_2 = pCompList_2->m_pkNext;
			// Load keeps the item's form ID in pInternalItem until here, so the TESForm* is read back as a uint32_t.
			pComponent_2->pInternalItem =
				TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pComponent_2
						->GetComponentPointer())));
		}
		SetInitialized(1);
	}
}

void TESRecipe::Copy(TESForm* apCopy) {
	TESRecipe* pmisc = dynamic_cast<TESRecipe*>(apCopy);
	if (pmisc) {
		(&RecipeConditions)->Copy(&pmisc->RecipeConditions, 0);
		ourMemcpy(&data, &pmisc->data, sizeof(data));
		CopyAllComponents(apCopy);
		while (!(&ingredientlist)->IsEmpty()) {
			(&ingredientlist)->RemoveHead();
		}
		for (BSSimpleList<TESRecipeComponent*>* pList = &pmisc->ingredientlist; pList && !pList->IsEmpty();
			pList = pList->m_pkNext) {
			TESRecipeComponent* pComp = pList->m_item;
			TESRecipeComponent* pNew;
			TESRecipeComponent* pMemory = static_cast<TESRecipeComponent*>(ourMalloc(sizeof(TESRecipeComponent)));
			if (pMemory) {
				pNew = ::new (pMemory) TESRecipeComponent;
			} else {
				pNew = 0;
			}
			pNew->Quantity = pComp->Quantity;
			pNew->pInternalItem = pComp->pInternalItem;
			(&ingredientlist)->AddTail(pNew);
		}
		while (!(&outputlist)->IsEmpty()) {
			(&outputlist)->RemoveHead();
		}
		for (BSSimpleList<TESRecipeComponent*>* pList_2 = &pmisc->outputlist; pList_2 && !pList_2->IsEmpty();
			pList_2 = pList_2->m_pkNext) {
			TESRecipeComponent* pComp_2 = pList_2->m_item;
			TESRecipeComponent* pNew_2;
			TESRecipeComponent* pMemory_2 = static_cast<TESRecipeComponent*>(ourMalloc(sizeof(TESRecipeComponent)));
			if (pMemory_2) {
				pNew_2 = ::new (pMemory_2) TESRecipeComponent;
			} else {
				pNew_2 = 0;
			}
			pNew_2->Quantity = pComp_2->Quantity;
			pNew_2->pInternalItem = pComp_2->pInternalItem;
			(&outputlist)->AddTail(pNew_2);
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
		return 1;
	}
	TESRecipeCategory* Category = pRecipeCat;
	if (Category != pcomp->pRecipeCat) {
		return 1;
	}
	TESRecipeCategory* SubCategory = GetSubCategory();
	if (SubCategory != pcomp->GetSubCategory()) {
		return 1;
	}
	ActorValue::Index RequiredSkill = GetRequiredSkill();
	if (RequiredSkill != pcomp->GetRequiredSkill()) {
		return 1;
	}
	int32_t ProcessLevel = data.ReqSkillLevel;
	if (ProcessLevel != pcomp->data.ReqSkillLevel) {
		return 1;
	}
	if (CompareAllComponents(pcomp)) {
		return 1;
	}
	if ((&RecipeConditions)->Compare(&pcomp->RecipeConditions)) {
		return 1;
	}
	int32_t v7 = (&ingredientlist)->ItemsInList();
	if (v7 != (&pcomp->ingredientlist)->ItemsInList()) {
		return 1;
	}
	int32_t v8 = (&outputlist)->ItemsInList();
	if (v8 != (&pcomp->outputlist)->ItemsInList()) {
		return 1;
	}
	BSSimpleList<TESRecipeComponent*>* ingredients = &ingredientlist;
	BSSimpleList<TESRecipeComponent*>* copyingredients = &pcomp->ingredientlist;
	while (copyingredients && !copyingredients->IsEmpty()) {
		if (!ingredients || ingredients->IsEmpty()) {
			return 1;
		}
		TESRecipeComponent* pRecipeComponent = ingredients->m_item;
		TESRecipeComponent* pRecipeComponentCopy = copyingredients->m_item;
		if (pRecipeComponent->Compare(pRecipeComponentCopy)) {
			return 1;
		}
		copyingredients = copyingredients->m_pkNext;
		ingredients = ingredients->m_pkNext;
	}
	BSSimpleList<TESRecipeComponent*>* output = &outputlist;
	BSSimpleList<TESRecipeComponent*>* copyoutput = &pcomp->outputlist;
	while (copyoutput && !copyoutput->IsEmpty()) {
		if (!output || output->IsEmpty()) {
			return 1;
		}
		TESRecipeComponent* pRecipeComponent_2 = output->m_item;
		TESRecipeComponent* pRecipeComponentCopy_2 = copyoutput->m_item;
		if (pRecipeComponent_2->Compare(pRecipeComponentCopy_2)) {
			return 1;
		}
		copyoutput = copyoutput->m_pkNext;
		output = output->m_pkNext;
	}
	return 0;
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

int32_t TESRecipe::CompareForSorting(ListBoxItem<TESRecipe*>* a1, ListBoxItem<TESRecipe*>* a2) {
	const char* _s1 = a1->data->GetFullName();
	const char* _s2 = a2->data->GetFullName();
	int32_t Recipe = RecipeMenu::CanMakeRecipe(a1->data);
	int32_t v6 = RecipeMenu::CanMakeRecipe(a2->data);
	if ((Recipe > 0 && v6 > 0) || (Recipe <= 0 && v6 <= 0)) {
		return _tcscmp(_s1, _s2);
	}
	if (Recipe > 0) {
		return -1;
	}
	if (v6 <= 0) {
		return _tcscmp(_s1, _s2);
	}
	return 1;
}

bool TESRecipe::ShouldHideItem(TESRecipe* a1) {
	if (g_currentRecipeMenuCategory == -1) {
		return 0;
	}
	TESRecipeCategory* SubCategory = a1->GetSubCategory();
	return SubCategory != get_g_RecipeMenuCategories()->operator[](static_cast<uint32_t>(g_currentRecipeMenuCategory));
}

void RECIPE_CATEGORY_DATA::Endian() {}

TESRecipeCategory::TESRecipeCategory() {
	SetFormType(RCCT_ID);
	ResetFlag(1u);
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
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0x200);
			SetFormEditorID(pBuffer);
			break;
		}
		case OBND_ID:
			LoadObjectBound(apFile);
			break;
		case DATA_ID:
			// The DATA chunk is a raw 1-byte buffer.
			LoadData(apFile, static_cast<char*>(static_cast<void*>(&data)), 1);
			if (apFile->QEndian())
				data.Endian();
			break;
		case FULL_ID:
			TESFullName::LoadFullNameChunk(this, apFile);
			break;
		default:
			break;
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
	__SaveData(&data, 1);
	if (QEndianSwapOnSave()) {
		data.Endian();
	}
	CloseForm();
}

void TESRecipeCategory::Copy(TESForm* apCopy) {
	TESRecipeCategory* v3 = dynamic_cast<TESRecipeCategory*>(apCopy);
	if (v3) {
		ourMemcpy(&data.flags, &v3->data.flags, 1u);
		TESForm::CopyAllComponents(apCopy);
	}
}

bool TESRecipeCategory::Compare(TESForm* apCompare) {
	TESRecipeCategory* v5 = dynamic_cast<TESRecipeCategory*>(apCompare);
	if (!v5) {
		return (bool)1;
	}
	return (std::memcmp(&data.flags, &v5->data, 1u) || (((int8_t)TESForm::CompareAllComponents(v5)) != 0));
}

void TESRecipeCategory::ResetFlag(uint8_t flag) {
	data.flags &= (~((int8_t)flag));
}
