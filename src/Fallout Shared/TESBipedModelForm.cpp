#include "Fallout Shared/TESBipedModelForm.h"

#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSBipedModelList.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelRDT.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObjectARMA.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectCLOT.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/AI/Actor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

void BIPED_MODEL::Endian() {
    BSEndian(iBipedObjectSlots, false);
}

TESBipedModelForm::TESBipedModelForm() {
    TESBipedModelForm::InitializeDataComponent();
}

// The destructor clears the members' data before the members' destructors run.
TESBipedModelForm::~TESBipedModelForm() {
    TESBipedModelForm::ClearDataComponent();
}

void TESBipedModelForm::InitializeDataComponent() {
    bipedModelData.iBipedObjectSlots = 0;
    bipedModelData.sFlags = 0;
    BaseFormComponent* pComponent = &bipedModel[0];
    pComponent->InitializeDataComponent();
    pComponent = &worldModel[0];
    pComponent->InitializeDataComponent();
    pComponent = &inventoryIcon[0];
    pComponent->InitializeDataComponent();
    pComponent = &bipedModel[1];
    pComponent->InitializeDataComponent();
    pComponent = &worldModel[1];
    pComponent->InitializeDataComponent();
    pComponent = &inventoryIcon[1];
    pComponent->InitializeDataComponent();
    pComponent = &ConstraintTemplate;
    pComponent->InitializeDataComponent();
}

void TESBipedModelForm::ClearDataComponent() {
    bipedModel->ClearDataComponent();
    worldModel->ClearDataComponent();
    inventoryIcon->ClearDataComponent();
    bipedModel[1].ClearDataComponent();
    worldModel[1].ClearDataComponent();
    inventoryIcon[1].ClearDataComponent();
    ConstraintTemplate.ClearDataComponent();
}

void TESBipedModelForm::CopyComponent(BaseFormComponent* apCopyFC) {
    TESBipedModelForm* pCopy = dynamic_cast<TESBipedModelForm*>(apCopyFC);
    if (pCopy) {
        ourMemcpy(&bipedModelData, &pCopy->bipedModelData, sizeof(bipedModelData));
        bipedModel->CopyComponent(&pCopy->bipedModel[0]);
        worldModel->CopyComponent(&pCopy->worldModel[0]);
        inventoryIcon[0].CopyComponent(&pCopy->inventoryIcon[0]);
        messageIcon->CopyComponent(&pCopy->messageIcon[0]);
        bipedModel[1].CopyComponent(&pCopy->bipedModel[1]);
        worldModel[1].CopyComponent(&pCopy->worldModel[1]);
        inventoryIcon[1].CopyComponent(&pCopy->inventoryIcon[1]);
        messageIcon[1].CopyComponent(&pCopy->messageIcon[1]);
        ConstraintTemplate.CopyComponent(&pCopy->ConstraintTemplate);
    }
}

bool TESBipedModelForm::CompareComponent(BaseFormComponent* apCompareFC) {
    TESBipedModelForm* pCompare = dynamic_cast<TESBipedModelForm*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    if (std::memcmp(&bipedModelData, &pCompare->bipedModelData, sizeof(bipedModelData))) {
        return true;
    }
    if (bipedModel->CompareComponent(&pCompare->bipedModel[0])) {
        return true;
    }
    if (worldModel->CompareComponent(&pCompare->worldModel[0])) {
        return true;
    }
    if (inventoryIcon[0].CompareComponent(&pCompare->inventoryIcon[0])) {
        return true;
    }
    if (messageIcon->CompareComponent(&pCompare->messageIcon[0])) {
        return true;
    }
    if (bipedModel[1].CompareComponent(&pCompare->bipedModel[1])) {
        return true;
    }
    if (worldModel[1].CompareComponent(&pCompare->worldModel[1])) {
        return true;
    }
    if (inventoryIcon[1].CompareComponent(&pCompare->inventoryIcon[1])) {
        return true;
    }
    if (messageIcon[1].CompareComponent(&pCompare->messageIcon[1])) {
        return true;
    }
    return ConstraintTemplate.CompareComponent(&pCompare->ConstraintTemplate);
}

// Adds the selected model to the biped and applies its constraint template when available.
void TESBipedModelForm::AddToBiped(BipedAnim* apBiped, TESActorBase::SEX aeSex, BIPED_OBJECT aeBipedObject) {
    if (aeSex == TESActorBase::SEX_FEMALE && bipedModel[1].GetModelLength() == 0) {
        aeSex = TESActorBase::SEX_MALE;
    }
    TESForm* pParent = dynamic_cast<TESForm*>(this);
    apBiped->SetBipedPart(pParent, &bipedModel[aeSex], aeBipedObject);
    if (ConstraintTemplate.GetModelLength()) {
        const char* pModelPath = GetBipedConstraint();
        alignas(NiPointer<Model>) uint8_t modelStorage[sizeof(NiPointer<Model>)];
        NiPointer<Model>* model = ::new (modelStorage) NiPointer<Model>(nullptr);
        pModelLoader->FindModel(pModelPath, *model);
        if (*model) {
            Model* pLoadedModel = *model;
            NiNode* pObject3D = pLoadedModel->GetModel();
            TESObjectREFR* pRequester = apBiped->GetRequester();
            static_cast<Actor*>(pRequester)->ApplyConstrTemplate(pObject3D);
        }
        NiPointer<Model>::Destroy(model);
    }
}

bool TESBipedModelForm::FillsBipedSlot(BIPED_OBJECT aeBipedSlot, bool abHairTest, BGSBipedModelList* apBipedModelList) {
    if (abHairTest && aeBipedSlot == BO_HAIR && (bipedModelData.iBipedObjectSlots & 1) != 0) {
        return false;
    }
    const uint32_t uiSlotMask = 1u << (static_cast<uint32_t>(aeBipedSlot) & 0x1F);
    bool bFillsSlot = (bipedModelData.iBipedObjectSlots & uiSlotMask) != 0;
    if (bFillsSlot || !apBipedModelList) {
        return bFillsSlot;
    }
    BGSListForm* pModelList = apBipedModelList->GetBipedModelList();
    if (pModelList) {
        BSSimpleList<TESForm*>* pFormList = pModelList->GetFormList();
        while (!bFillsSlot && pFormList && !pFormList->IsEmpty()) {
            TESForm* pForm = pFormList->GetItem();
            TESBipedModelForm* pBipedModelForm = GetFormAsBipedModel(pForm);
            if (pBipedModelForm) {
                bFillsSlot = pBipedModelForm->FillsBipedSlot(aeBipedSlot, abHairTest, nullptr);
            }
            pFormList = pFormList->GetNext();
        }
    }
    return bFillsSlot;
}

void TESBipedModelForm::InitItem(TESForm* apOwner) {
    bipedModel[0].InitItem(apOwner);
    worldModel[0].InitItem(apOwner);
    bipedModel[1].InitItem(apOwner);
    worldModel[1].InitItem(apOwner);
}

void TESBipedModelForm::Save() {
    if (TESForm::QEndianSwapOnSave()) {
        bipedModelData.Endian();
    }
    TESForm::__AddChunkData(BMDT_ID, &bipedModelData, sizeof(bipedModelData));
    if (TESForm::QEndianSwapOnSave()) {
        bipedModelData.Endian();
    }
    bipedModel[0].Save(MODL_ID, MODT_ID, MODS_ID);
    bipedModel[0].SaveFlags(MODD_ID);
    worldModel[0].Save(MOD2_ID, MO2T_ID, MO2S_ID);
    inventoryIcon[0].Save(ICON_ID);
    messageIcon[0].Save(MICO_ID);
    bipedModel[1].Save(MOD3_ID, MO3T_ID, MO3S_ID);
    bipedModel[1].SaveFlags(MOSD_ID);
    worldModel[1].Save(MOD4_ID, MO4T_ID, MO4S_ID);
    inventoryIcon[1].Save(ICO2_ID);
    messageIcon[1].Save(MIC2_ID);
    ConstraintTemplate.Save(BMCT_ID, NO_CHUNK);
}

// Returns whether the power-armor flag is set.
bool TESBipedModelForm::IsPowerArmor() {
    return (bipedModelData.sFlags & BIPED_MODEL::POWER_ARMOR) != 0;
}

// Returns whether the backpack flag is set.
bool TESBipedModelForm::HasBackPack() {
    return (bipedModelData.sFlags & BIPED_MODEL::HAS_BACKPACK) != 0;
}

// Returns the constraint-template model path through the TESModel virtual slot.
const char* TESBipedModelForm::GetBipedConstraint() {
    TESModel* pModel = &ConstraintTemplate;
    return const_cast<char*>(pModel->GetModel());
}

// Sets the addon-model flag.
void TESBipedModelForm::SetAddOnModel(bool abVal) {
    if (abVal) {
        bipedModelData.sFlags |= BIPED_MODEL::ADDON_MODEL;
    }
    else {
        bipedModelData.sFlags &= ~BIPED_MODEL::ADDON_MODEL;
    }
}

bool TESBipedModelForm::IsMedium() {
    return (bipedModelData.sFlags & BIPED_MODEL::MEDIUM) != 0;
}

TESModelTextureSwap* TESBipedModelForm::GetWorldTESModel(TESActorBase::SEX aeSex) {
    if (!worldModel[aeSex].TESModel::GetModelLength() && aeSex == TESActorBase::SEX_FEMALE) {
        aeSex = TESActorBase::SEX_MALE;
    }
    return &worldModel[aeSex];
}

// The female icon falls back to the male one when it has no file.
const char* TESBipedModelForm::GetIcon(TESActorBase::SEX aeSex) {
    if (!inventoryIcon[aeSex].GetTextureNameLength() && aeSex == TESActorBase::SEX_FEMALE) {
        aeSex = TESActorBase::SEX_MALE;
    }
    return inventoryIcon[aeSex].GetTextureName();
}

TESModelTextureSwap* TESBipedModelForm::GetBipedTESModel(TESActorBase::SEX aeSex) {
    if (!bipedModel[aeSex].TESModel::GetModelLength() && aeSex == TESActorBase::SEX_FEMALE) {
        aeSex = TESActorBase::SEX_MALE;
    }
    return &bipedModel[aeSex];
}

// The biped model path of sex aeSex, the male one when the female one has no file.
const char* TESBipedModelForm::GetBipedModel(TESActorBase::SEX aeSex) {
    if (!bipedModel[aeSex].TESModel::GetModelLength() && aeSex == TESActorBase::SEX_FEMALE) {
        aeSex = TESActorBase::SEX_MALE;
    }
    TESModel* pModel = &bipedModel[aeSex];
    return pModel->GetModel();
}

const char* TESBipedModelForm::GetMessageIcon(TESActorBase::SEX aeSex) {
    if (!messageIcon[aeSex].GetMessageIconTextureNameLength() && aeSex == TESActorBase::SEX_FEMALE) {
        aeSex = TESActorBase::SEX_MALE;
    }
    return messageIcon[aeSex].GetMessageIconTextureName();
}

bool TESBipedModelForm::GetPlayable() {
    return (bipedModelData.sFlags & BIPED_MODEL::NON_PLAYABLE) == 0;
}

bool TESBipedModelForm::IsJewelry() {
    return false;
}

bool TESBipedModelForm::FillsMatchingBipedSlots(TESBipedModelForm* apOtherBiped) {
    return (apOtherBiped->bipedModelData.iBipedObjectSlots & bipedModelData.iBipedObjectSlots) != 0;
}

const char* TESBipedModelForm::GetWorldModel_0(TESActorBase::SEX aeSex) {
    if (!worldModel[aeSex].GetModelLength() && aeSex == TESActorBase::SEX_FEMALE) {
        aeSex = TESActorBase::SEX_MALE;
    }
    return worldModel[aeSex].GetModel();
}

// The biped model component of an armor, clothing or armor addon form; null for any other form. Clothing has no
// TESHealthForm, so its component sits at +0x68 instead of +0x70.
TESBipedModelForm* TESBipedModelForm::GetFormAsBipedModel(TESForm* apForm) {
    TESBipedModelForm* pBipedModel = nullptr;
    if (apForm) {
        switch (apForm->GetFormType()) {
            case ARMO_ID: pBipedModel = static_cast<TESObjectARMO*>(apForm); break;
            case ARMA_ID: pBipedModel = static_cast<TESObjectARMA*>(apForm); break;
            case CLOT_ID: pBipedModel = static_cast<TESObjectCLOT*>(apForm); break;
        }
    }
    return pBipedModel;
}

TESModelTextureSwap* TESBipedModelForm::GetWorldTESModel(TESForm* apObj, TESObjectREFR* apRef) {
    TESBipedModelForm* pBipedModelForm = dynamic_cast<TESBipedModelForm*>(apObj);
    if (!pBipedModelForm) {
        return nullptr;
    }
    if (!apRef) {
        return &pBipedModelForm->worldModel[0];
    }
    TESForm* pOwner = apRef->GetOwner();
    TESActorBase::SEX eSex = TESActorBase::SEX_MALE;
    if (pOwner && pOwner->GetFormType() == NPC__ID) {
        TESActorBase* pActorBase = static_cast<TESActorBase*>(pOwner);
        eSex = pActorBase->GetSex();
    }
    return pBipedModelForm->GetWorldTESModel(eSex);
}

TESModelTextureSwap* TESBipedModelForm::GetBipedTESModel(TESForm* apObj, TESObjectREFR* apRef) {
    TESBipedModelForm* pBipedModelForm = dynamic_cast<TESBipedModelForm*>(apObj);
    if (!pBipedModelForm) {
        return nullptr;
    }
    if (!apRef) {
        return &pBipedModelForm->bipedModel[0];
    }
    TESForm* pOwner = apRef->GetOwner();
    TESActorBase::SEX eSex = TESActorBase::SEX_MALE;
    if (pOwner && pOwner->GetFormType() == NPC__ID) {
        TESActorBase* pActorBase = static_cast<TESActorBase*>(pOwner);
        eSex = pActorBase->GetSex();
    }
    return pBipedModelForm->GetBipedTESModel(eSex);
}

const char* TESBipedModelForm::GetMessageIcon(TESForm* apObj, TESObjectREFR* apRef) {
    TESBipedModelForm* pBipedModelForm = dynamic_cast<TESBipedModelForm*>(apObj);
    if (!pBipedModelForm) {
        return nullptr;
    }
    if (!apRef) {
        return pBipedModelForm->messageIcon->GetMessageIconTextureName();
    }
    TESActorBase* pOwner = static_cast<TESActorBase*>(apRef->GetOwner());
    TESActorBase::SEX eSex = TESActorBase::SEX_MALE;
    if (pOwner && pOwner->GetFormType() == NPC__ID) {
        eSex = pOwner->GetSex();
    }

    return pBipedModelForm->GetMessageIcon(eSex);
}

const char* TESBipedModelForm::GetWorldModel(TESForm* apObj, TESObjectREFR* apRef) {
    TESBipedModelForm* pBipedModel = dynamic_cast<TESBipedModelForm*>(apObj);
    if (!pBipedModel) {
        return nullptr;
    }
    if (!apRef) {
        return pBipedModel->worldModel[0].GetModel();
    }
    TESForm* pOwner = apRef->GetOwner();
    TESActorBase::SEX eSex = TESActorBase::SEX_MALE;
    if (pOwner && pOwner->GetFormType() == NPC__ID) {
        eSex = static_cast<TESActorBase*>(pOwner)->TESActorBase::GetSex();
    }

    return pBipedModel->GetWorldModel_0(eSex);
}

const char* TESBipedModelForm::GetIcon_0(TESForm* apObj, TESObjectREFR* apRef) {
    TESBipedModelForm* pBipedModel = dynamic_cast<TESBipedModelForm*>(apObj);
    if (!pBipedModel) {
        return nullptr;
    }
    if (!apRef) {
        return pBipedModel->inventoryIcon[0].GetTextureName();
    }
    TESForm* pOwner = apRef->GetOwner();
    TESActorBase::SEX eSex = TESActorBase::SEX_MALE;
    if (pOwner && pOwner->GetFormType() == NPC__ID) {
        eSex = static_cast<TESActorBase*>(pOwner)->TESActorBase::GetSex();
    }

    return pBipedModel->GetIcon(eSex);
}

uint32_t BGSMessageIcon::GetMessageIconTextureNameLength() const {
    return Icon.GetTextureNameLength();
}

TESModelRDT::TESModelRDT() : TESModel() {}

TESModelRDT::~TESModelRDT() {}
