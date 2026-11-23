#include "Fallout Shared/BGSNote.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

BGSNote::BGSNote() {
    SetFormType(49u);
    BGSNote::InitializeData();
}

// Clears the data before the members and bases are destroyed.
BGSNote::~BGSNote() {
    BGSNote::ClearData();
}

void BGSNote::InitializeData() {
    cType = 0;
    pNoteSound = nullptr;
    pSpeaker = nullptr;
    bHasBeenRead = false;
}

void BGSNote::ClearData() {
    OwnerQuests.RemoveAll();
    uint8_t cNoteType = this->cType;
    if (cNoteType == 1) {
        ourFree(this->pNoteSound);
        this->pNoteSound = nullptr;
    }
    else if (cNoteType == 2) {
        TESTexture* pImage = this->pNoteImage;
        if (pImage) {
            delete pImage;
        }
        this->pNoteSound = nullptr;
    }
}

// Reads the note's chunks. The chunks handled directly are the model, name, icon and sounds; every other one belongs to
// the note's kind, which DATA sets: SNAM is the sound or the speaker, TNAM the text or the topic and any texture chunk
// the image. When the chunk is empty (malformed data), this slot is initialised to "" (one zeroed byte, allocated only
// for an empty chunk).
bool BGSNote::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != NOTE_ID) {
        return false;
    }
    this->LoadForm(apFile);
    do {
        CHUNK_ID eChunk = apFile->GetTESChunk();
        if (eChunk == NO_CHUNK) {
            break;
        }
        switch (eChunk) {
            case ONAM_ID: {
                // The chunk holds a form ID, which is stored as the raw pointer value.
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                TESQuest* pQuest = reinterpret_cast<TESQuest*>(static_cast<uintptr_t>(uiFormID));
                OwnerQuests.AddHead(pQuest);
                break;
            }
            case OBND_ID: static_cast<TESForm*>(this)->LoadObjectBound(apFile); break;
            case DATA_ID: {
                uint8_t ucType = 0;
                apFile->GetChunkData(&ucType, 1u);
                this->SetNoteType(static_cast<NOTE_TYPE>(ucType));
                break;
            }
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // An empty chunk leaves the buffer unwritten (see above).
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200u);
                static_cast<TESForm*>(this)->SetFormEditorID(pBuffer);
                break;
            }
            case MODL_ID:
            case MODT_ID: TESModel::LoadModelChunk(this, apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case ZNAM_ID: {
                // The chunk holds a form ID, which is passed on as the raw pointer value.
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                static_cast<BGSPickupPutdownSounds*>(this)->SetPutdownSound(
                    reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiFormID))
                );
                break;
            }
            case XNAM_ID: {
                // Skips an empty chunk.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                if (uiChunkSize) {
                    // A 16-byte aligned stack buffer of the chunk's size.
                    char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize));
                    apFile->GetChunkData(pBuffer, uiChunkSize);
                    // The image is fetched twice, once to test it and once to write to.
                    if (this->GetNoteImage()) {
                        this->GetNoteImage()->SetTextureName(pBuffer);
                    }
                }
                break;
            }
            case YNAM_ID: {
                // The chunk holds a form ID, which is passed on as the raw pointer value.
                uint32_t uiFormID = 0;
                apFile->GetChunkData<uint32_t>(uiFormID);
                static_cast<BGSPickupPutdownSounds*>(this)->SetPickupSound(
                    reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiFormID))
                );
                break;
            }
            case ICON_ID: TESTexture::LoadTextureChunk(this, apFile); break;
            default:
                // Reads the note's kind as a signed byte and skips every value outside 0 to 3.
                switch (static_cast<int8_t>(cType)) {
                    case 0:
                        if (eChunk == SNAM_ID) {
                            // The chunk holds a form ID, which is passed on as the raw pointer value.
                            uint32_t uiFormID = 0;
                            apFile->GetChunkData<uint32_t>(uiFormID);
                            this->SetNoteSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiFormID)));
                        }
                        break;
                    case 1:
                        if (eChunk == TNAM_ID) {
                            TESDescription* pText = this->GetNoteText();
                            if (pText) {
                                TESDescription::LoadDescriptionData(pText, apFile);
                            }
                        }
                        break;
                    case 2:
                        if (TESTexture::IsTextureChunk(eChunk)) {
                            TESTexture* pImage = this->GetNoteImage();
                            if (pImage) {
                                TESTexture::LoadTextureChunk(pImage, apFile);
                            }
                        }
                        break;
                    case 3:
                        if (eChunk == SNAM_ID) {
                            // The chunk holds a form ID, which is passed on as the raw pointer value.
                            uint32_t uiFormID = 0;
                            apFile->GetChunkData<uint32_t>(uiFormID);
                            this->SetNoteSpeaker(reinterpret_cast<TESActorBase*>(static_cast<uintptr_t>(uiFormID)));
                        }
                        else if (eChunk == TNAM_ID) {
                            // The chunk holds a form ID, which is passed on as the raw pointer value.
                            uint32_t uiFormID = 0;
                            apFile->GetChunkData<uint32_t>(uiFormID);
                            this->SetNoteTopic(reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(uiFormID)));
                        }
                        break;
                }
                break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void BGSNote::Save() {
    StartForm();
    TESFullName::Save();
    TESModel::Save(MODL_ID, MODT_ID);
    TESTexture::Save(ICON_ID);
    BGSPickupPutdownSounds::Save();
    TESForm::AddChunk(DATA_ID, cType);
    for (BSSimpleList<TESQuest*>* plist = &OwnerQuests; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        TESForm::AddChunk(ONAM_ID, plist->m_item->iFormID);
    }
    switch (cType) {
        case 0: {
            TESSound* psound = GetNoteSound();
            if (psound) {
                TESForm::AddChunk(SNAM_ID, psound->iFormID);
            }
            break;
        }
        case 1: {
            TESDescription* pdesc = GetNoteText();
            if (pdesc) {
                TESDescription::SaveDescriptionData(pdesc, TNAM_ID);
            }
            break;
        }
        case 2: {
            TESTexture* pimage = GetNoteImage();
            if (pimage) {
                TESForm::AddChunkArray(XNAM_ID, pimage->GetTextureName(), pimage->GetTextureNameLength() + 1);
            }
            break;
        }
        case 3: {
            TESTopic* ptopic = GetNoteTopic();
            if (ptopic) {
                TESForm::AddChunk(TNAM_ID, ptopic->iFormID);
            }
            TESActorBase* pspeaker = GetNoteSpeaker();
            if (pspeaker) {
                TESForm::AddChunk(SNAM_ID, pspeaker->iFormID);
            }
            break;
        }
        default: {
            break;
        }
    }
    CloseForm();
}

void BGSNote::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_NOTE_READ)) {
        bHasBeenRead = true;
    }
}

void BGSNote::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_NOTE_READ)) {
        bHasBeenRead = false;
    }
}

void BGSNote::InitItem() {
    if (!GetInitialized()) {
        BSSimpleList<TESQuest*>* pList = &OwnerQuests;
        BSSimpleList<TESQuest*>* pLastList = nullptr;
        while (pList && !pList->IsEmpty()) {
            uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pList->m_item));
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempint, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            TESQuest* pQuest = dynamic_cast<TESQuest*>(pForm);
            if (pQuest) {
                pList->SetItem(pQuest);
                pLastList = pList;
                pList = pList->m_pkNext;
            }
            else {
                int32_t iNoteFormID = iFormID;
                const char* pcEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Unable to find owner quest (%08X) for Note '%s' (%08X).",
                    itempint,
                    pcEditorID,
                    iNoteFormID
                );
                if (pLastList) {
                    pLastList->Remove(pList->m_item);
                    pList = pLastList->m_pkNext;
                }
                else {
                    pList->RemoveHead();
                }
            }
        }
        if (GetNoteSound()) {
            uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetNoteSound()));
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempint, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            TESSound* pSound = dynamic_cast<TESSound*>(pForm);
            SetNoteSound(pSound);
            if (!pSound) {
                int32_t iNoteFormID = iFormID;
                const char* pcEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Unable to find sound (%08X) for Note '%s' (%08X).",
                    itempint,
                    pcEditorID,
                    iNoteFormID
                );
            }
        }
        if (GetNoteTopic()) {
            uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetNoteTopic()));
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempint, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            TESTopic* pTopic = dynamic_cast<TESTopic*>(pForm);
            SetNoteTopic(pTopic);
            if (!pTopic) {
                int32_t iNoteFormID = iFormID;
                const char* pcEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Unable to find topic (%08X) for Note '%s' (%08X).",
                    itempint,
                    pcEditorID,
                    iNoteFormID
                );
            }
        }
        if (GetNoteSpeaker()) {
            uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetNoteSpeaker()));
            TESFile* pFile = GetFile(-1);
            TESForm::AddCompileIndex(itempint, pFile);
            TESForm* pForm = TESForm::GetFormByNumericID(itempint);
            TESActorBase* pActorBase = dynamic_cast<TESActorBase*>(pForm);
            SetNoteSpeaker(pActorBase);
            if (!pActorBase) {
                int32_t iNoteFormID = iFormID;
                const char* pcEditorID = GetFormEditorID();
                Warning(
                    "MASTERFILE: Unable to find actor base (%08X) for Note '%s' (%08X).",
                    itempint,
                    pcEditorID,
                    iNoteFormID
                );
            }
        }
        BGSPickupPutdownSounds::InitItem(this);
        SetInitialized(true);
    }
}

void BGSNote::Copy(TESForm* apCopy) {
    BGSNote* pCopy = dynamic_cast<BGSNote*>(apCopy);
    if (pCopy) {
        CopyAllComponents(apCopy);
        SetNoteType(pCopy->GetNoteType());
        SetNoteSpeaker(pCopy->GetNoteSpeaker());
        switch (cType) {
            case 0: {
                SetNoteSound(pCopy->GetNoteSound());
                break;
            }
            case 1: {
                SetNoteText(pCopy->GetNoteText());
                break;
            }
            case 2: {
                SetNoteImage(pCopy->GetNoteImage());
                break;
            }
            case 3: {
                SetNoteTopic(pCopy->GetNoteTopic());
                break;
            }
            default: {
                break;
            }
        }
        OwnerQuests.RemoveAll();
        for (BSSimpleList<TESQuest*>* pList = &pCopy->OwnerQuests; pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            OwnerQuests.AddHead(pList->m_item);
        }
        bHasBeenRead = pCopy->bHasBeenRead;
    }
}

bool BGSNote::Compare(TESForm* apCompare) {
    BGSNote* pComp = dynamic_cast<BGSNote*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    if (pComp->cType != this->cType) {
        return true;
    }
    switch (this->cType) {
        case 0: {
            TESSound* pSound = GetNoteSound();
            if (pSound != pComp->GetNoteSound()) {
                return true;
            }
            break;
        }
        case 1: {
            TESDescription* pDesc = GetNoteText();
            TESDescription* pCompDesc = pComp->GetNoteText();
            if (pDesc && pCompDesc) {
                if (pDesc->CompareComponent(pCompDesc)) {
                    return true;
                }
            }
            else {
                if (pDesc || pCompDesc) {
                    return true;
                }
            }
            break;
        }
        case 2: {
            TESTexture* pTexture = GetNoteImage();
            TESTexture* pCompTexture = pComp->GetNoteImage();
            if (pTexture && pCompTexture) {
                if (pTexture->CompareComponent(pCompTexture)) {
                    return true;
                }
            }
            else {
                if (pTexture || pCompTexture) {
                    return true;
                }
            }
            break;
        }
        case 3: {
            TESTopic* pTopic = GetNoteTopic();
            if (pTopic != pComp->GetNoteTopic()) {
                return true;
            }
            TESActorBase* pActorBase = GetNoteSpeaker();
            if (pActorBase != pComp->GetNoteSpeaker()) {
                return true;
            }
            break;
        }
        default: {
            break;
        }
    }
    int32_t iCount = OwnerQuests.ItemsInList();
    if (iCount == pComp->OwnerQuests.ItemsInList()) {
        for (BSSimpleList<TESQuest*>* pList = &OwnerQuests; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
            TESQuest* pQuest = pList->m_item;
            if (!pComp->OwnerQuests.IsInList(pQuest)) {
                return true;
            }
        }
        return false;
    }
    return true;
}

bool BGSNote::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjectToGet,
    int32_t aiCount
) {
    bool bKarmaPenalty = false;
    if (apActionRef != pPlayer) {
        return false;
    }
    TESForm* pOwner = apItemActivated->GetOwner();
    if (pOwner && !apItemActivated->IsAnOwner(pPlayer, true)) {
        if (pOwner->GetFormType() == FACT_ID) {
            bKarmaPenalty = !static_cast<TESFaction*>(pOwner)->TESFaction::GetIsEvil();
        }
        else {
            // The owner is an actor base: its karma is read through its ActorValueOwner.
            TESActorBase* pBase = static_cast<TESActorBase*>(pOwner);
            if (pBase) {
                ActorValueOwner* pAVOwner = pBase;
                float fKarma = pAVOwner->GetActorFloatValue(ActorValue::KARMA);
                TESActorBaseData::ALIGNMENT eAlign = TESActorBaseData::GetAlignmentForKarma(fKarma);
                if (eAlign != TESActorBaseData::ALIGNMENT_EVIL) {
                    bKarmaPenalty = eAlign != TESActorBaseData::ALIGNMENT_VERY_EVIL;
                }
            }
        }
    }

    if (bKarmaPenalty) {
        double dKarmaMod = fKarmaModStealing.Float();
        pPlayer->RewardKarma(static_cast<int32_t>(dKarmaMod));
    }
    pPlayer->AddNote(this, true);
    apItemActivated->Disable();
    return true;
}

NOTE_TYPE BGSNote::GetNoteType() {
    return static_cast<NOTE_TYPE>(static_cast<int8_t>(cType));
}

void BGSNote::SetHasBeenRead(bool abRead) {
    if (bHasBeenRead != abRead) {
        bHasBeenRead = abRead;
        if (bHasBeenRead) {
            AddChange(CHANGE_NOTE_READ);
        }
        else {
            RemoveChange(CHANGE_NOTE_READ);
        }
    }
}

BSSimpleList<TESQuest*>* BGSNote::GetQuestList() {
    return &OwnerQuests;
}

TESSound* BGSNote::GetNoteSound() {
    if (GetNoteType() != NT_SOUND) {
        return nullptr;
    }

    return pNoteSound;
}

void BGSNote::SetNoteSound(TESSound* apSound) {
    if (GetNoteType() == NT_SOUND) {
        pNoteSound = apSound;
    }
}

TESActorBase* BGSNote::GetNoteSpeaker() {
    if (GetNoteType() == NT_VOICE) {
        return pSpeaker;
    }

    return nullptr;
}

void BGSNote::SetNoteSpeaker(TESActorBase* apSpeaker) {
    if (GetNoteType() == NT_VOICE) {
        pSpeaker = apSpeaker;
    }
}

TESTexture* BGSNote::GetNoteImage() {
    if (GetNoteType() == NT_IMAGE) {
        return this->pNoteImage;
    }

    return nullptr;
}

TESDescription* BGSNote::GetNoteText() {
    if (GetNoteType() == NT_TEXT) {
        return this->pNoteText;
    }

    return nullptr;
}

void BGSNote::SetNoteType(NOTE_TYPE aeVal) {
    // cType is sign-extended for the compare.
    if (aeVal != static_cast<int8_t>(this->cType)) {
        uint8_t cNoteType = this->cType;
        if (cNoteType == 1) {
            ourFree(this->pNoteSound);
            this->pNoteSound = nullptr;
        }
        else if (cNoteType == 2) {
            TESTexture* pImage = this->pNoteImage;
            if (pImage) {
                delete pImage;
            }
            this->pNoteSound = nullptr;
        }
        this->cType = aeVal;
        uint8_t cNewType = this->cType;
        if (cNewType == 1) {
            this->pNoteText = new TESDescription;
        }
        else if (cNewType == 2) {
            this->pNoteImage = new TESTexture;
        }
    }
}

void BGSNote::SetNoteTopic(TESTopic* apTopic) {
    if (GetNoteType() == NT_VOICE) {
        pNoteTopic = apTopic;
    }
}

void BGSNote::SetNoteImage(TESTexture* apTex) {
    if (GetNoteType() == NT_IMAGE) {
        static_cast<TESForm*>(pNoteSound)->CopyComponent(apTex);
    }
}

void BGSNote::SetNoteText(TESDescription* apDesc) {
    if (GetNoteType() == NT_TEXT) {
        static_cast<TESForm*>(pNoteSound)->CopyComponent(apDesc);
    }
}

TESTopic* BGSNote::GetNoteTopic() {
    if (GetNoteType() == NT_VOICE) {
        return this->pNoteTopic;
    }

    return nullptr;
}
