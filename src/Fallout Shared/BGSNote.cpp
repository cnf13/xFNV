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

BGSNote::BGSNote() {
    SetFormType(49u);
    BGSNote::InitializeData();
}

// Clears the data before the members and bases are destroyed.
BGSNote::~BGSNote() {
    BGSNote::ClearData();
}

void BGSNote::InitializeData() {
    cType = (uint8_t)0;
    pNoteSound = nullptr;
    pSpeaker = nullptr;
    bHasBeenRead = false;
}

void BGSNote::ClearData() {
    OwnerQuests.RemoveAll();
    uint8_t cType = this->cType;
    if (cType == 1) {
        ourFree(this->pNoteSound);
        this->pNoteSound = nullptr;
    }
    else {
        if (cType == 2) {
            TESTexture* apThis = this->pNoteImage;
            if (apThis) {
                delete apThis;
            }
            this->pNoteSound = nullptr;
        }
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
                char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
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
                    char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize, 128));
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

// A note the save marks as read (change flag 0x80000000) is read.
void BGSNote::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(0x80000000)) {
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
        BSSimpleList<TESQuest*>* plist = &OwnerQuests;
        BSSimpleList<TESQuest*>* plastlist = nullptr;
        while (plist && !plist->IsEmpty()) {
            uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(plist->m_item));
            TESFile* File = GetFile(-1);
            TESForm::AddCompileIndex(itempint, File);
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
            TESQuest* pquest = dynamic_cast<TESQuest*>(FormByNumericID);
            if (pquest) {
                plist->SetItem(pquest);
                plastlist = plist;
                plist = plist->m_pkNext;
            }
            else {
                int32_t CastingType = iFormID;
                const char* v9 = GetFormEditorID();
                Warning(
                    "MASTERFILE: Unable to find owner quest (%08X) for Note '%s' (%08X).", itempint, v9, CastingType
                );
                if (plastlist) {
                    BSSimpleList<TESQuest*>* v14 = plist;
                    plastlist->Remove(v14->m_item);
                    plist = plastlist->m_pkNext;
                }
                else {
                    plist->RemoveHead();
                }
            }
        }
        if (GetNoteSound()) {
            uint32_t itempint_2 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetNoteSound()));
            TESFile* v3 = GetFile(-1);
            TESForm::AddCompileIndex(itempint_2, v3);
            TESForm* v4 = TESForm::GetFormByNumericID(itempint_2);
            TESSound* psound = dynamic_cast<TESSound*>(v4);
            SetNoteSound(psound);
            if (!psound) {
                int32_t v15 = iFormID;
                const char* v10 = GetFormEditorID();
                Warning("MASTERFILE: Unable to find sound (%08X) for Note '%s' (%08X).", itempint_2, v10, v15);
            }
        }
        if (GetNoteTopic()) {
            uint32_t itempint_3 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetNoteTopic()));
            TESFile* v5 = GetFile(-1);
            TESForm::AddCompileIndex(itempint_3, v5);
            TESForm* v6 = TESForm::GetFormByNumericID(itempint_3);
            TESTopic* ptopic = dynamic_cast<TESTopic*>(v6);
            SetNoteTopic(ptopic);
            if (!ptopic) {
                int32_t v16 = iFormID;
                const char* v11 = GetFormEditorID();
                Warning("MASTERFILE: Unable to find topic (%08X) for Note '%s' (%08X).", itempint_3, v11, v16);
            }
        }
        if (GetNoteSpeaker()) {
            uint32_t itempint_4 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetNoteSpeaker()));
            TESFile* v7 = GetFile(-1);
            TESForm::AddCompileIndex(itempint_4, v7);
            TESForm* v8 = TESForm::GetFormByNumericID(itempint_4);
            TESActorBase* pspeaker = dynamic_cast<TESActorBase*>(v8);
            SetNoteSpeaker(pspeaker);
            if (!pspeaker) {
                int32_t v17 = iFormID;
                const char* v12 = GetFormEditorID();
                Warning("MASTERFILE: Unable to find actor base (%08X) for Note '%s' (%08X).", itempint_4, v12, v17);
            }
        }
        BGSPickupPutdownSounds::InitItem(this);
        SetInitialized(true);
    }
}

void BGSNote::Copy(TESForm* apCopy) {
    BGSNote* pcopy = dynamic_cast<BGSNote*>(apCopy);
    if (pcopy) {
        CopyAllComponents(apCopy);
        NOTE_TYPE NoteType = pcopy->GetNoteType();
        SetNoteType(NoteType);
        TESActorBase* NoteSpeaker = pcopy->GetNoteSpeaker();
        SetNoteSpeaker(NoteSpeaker);
        switch (cType) {
            case 0: {
                TESSound* NoteSound = pcopy->GetNoteSound();
                SetNoteSound(NoteSound);
                break;
            }
            case 1: {
                TESDescription* NoteText = pcopy->GetNoteText();
                SetNoteText(NoteText);
                break;
            }
            case 2: {
                TESTexture* NoteImage = pcopy->GetNoteImage();
                SetNoteImage(NoteImage);
                break;
            }
            case 3: {
                TESTopic* NoteTopic = pcopy->GetNoteTopic();
                SetNoteTopic(NoteTopic);
                break;
            }
            default: {
                break;
            }
        }
        OwnerQuests.RemoveAll();
        for (BSSimpleList<TESQuest*>* plist = &pcopy->OwnerQuests; plist && !plist->IsEmpty();
             plist = plist->m_pkNext) {
            OwnerQuests.AddHead(plist->m_item);
        }
        bHasBeenRead = pcopy->bHasBeenRead;
    }
}

bool BGSNote::Compare(TESForm* apCompare) {
    bool result;
    BGSNote* pcomp = dynamic_cast<BGSNote*>(apCompare);
    if (!pcomp) {
        return true;
    }
    if (CompareAllComponents(pcomp)) {
        return true;
    }
    if (pcomp->cType != this->cType) {
        return true;
    }
    switch (this->cType) {
        case 0: {
            TESSound* NoteSound = GetNoteSound();
            if (NoteSound == pcomp->GetNoteSound()) {
                goto LABEL_33;
            }
            result = true;
            break;
        }
        case 1: {
            TESDescription* pdesc = GetNoteText();
            TESDescription* pcompdesc = pcomp->GetNoteText();
            if (pdesc && pcompdesc) {
                if (!pdesc->CompareComponent(pcompdesc)) {
                    goto LABEL_33;
                }
                result = true;
            }
            else {
                if (!pdesc && !pcompdesc) {
                    goto LABEL_33;
                }
                result = true;
            }
            break;
        }
        case 2: {
            TESTexture* ptexture = GetNoteImage();
            TESTexture* pcomptexture = pcomp->GetNoteImage();
            if (ptexture && pcomptexture) {
                if (!ptexture->CompareComponent(pcomptexture)) {
                    goto LABEL_33;
                }
                result = true;
            }
            else {
                if (!ptexture && !pcomptexture) {
                    goto LABEL_33;
                }
                result = true;
            }
            break;
        }
        case 3: {
            TESTopic* NoteTopic = GetNoteTopic();
            if (NoteTopic == pcomp->GetNoteTopic()) {
                TESActorBase* NoteSpeaker = GetNoteSpeaker();
                if (NoteSpeaker == pcomp->GetNoteSpeaker()) {
                    goto LABEL_33;
                }
                result = true;
            }
            else {
                result = true;
            }
            break;
        }
        default: {
        LABEL_33:;
            int32_t v7 = (&OwnerQuests)->ItemsInList();
            if (v7 == (&pcomp->OwnerQuests)->ItemsInList()) {
                for (BSSimpleList<TESQuest*>* plist_2 = &OwnerQuests; plist_2 && !plist_2->IsEmpty();
                     plist_2 = plist_2->m_pkNext) {
                    TESQuest* pquest = plist_2->m_item;
                    if (!(&pcomp->OwnerQuests)->IsInList(pquest)) {
                        return true;
                    }
                }
                result = false;
            }
            else {
                result = true;
            }
            break;
        }
    }
    return result;
}

bool BGSNote::Activate(
    TESObjectREFR* apItemActivated,
    TESObjectREFR* apActionRef,
    bool abIdFlag,
    TESBoundObject* apObjectToGet,
    int32_t aiCount
) {
    XFNV_DYNAMIC_CAST_VOID(Actor)(apActionRef, RTTI_TESObjectREFR, RTTI_Actor);
    bool v12 = false;
    if (apActionRef != pPlayer) {
        return false;
    }
    TESForm* powner = apItemActivated->GetOwner();
    if ((powner) && (!apItemActivated->IsAnOwner(pPlayer, true))) {
        if (powner->GetFormType() == FACT_ID) {
            v12 = !static_cast<TESFaction*>(powner)->TESFaction::GetIsEvil();
        }
        else {
            // The owner is an actor base: its karma is read through its ActorValueOwner.
            TESActorBase* pbase = static_cast<TESActorBase*>(powner);
            if (pbase) {
                ActorValueOwner* pAVOwner = pbase;
                float afKarma = pAVOwner->GetActorFloatValue(ActorValue::KARMA);
                TESActorBaseData::ALIGNMENT ealign = TESActorBaseData::GetAlignmentForKarma(afKarma);
                if (ealign != TESActorBaseData::ALIGNMENT_EVIL) {
                    v12 = ealign != TESActorBaseData::ALIGNMENT_VERY_EVIL;
                }
            }
        }
    }

    if (v12) {
        double v6 = fKarmaModStealing.Float();
        pPlayer->RewardKarma(static_cast<int32_t>(v6));
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
        uint8_t cType = this->cType;
        if (cType == 1) {
            ourFree(this->pNoteSound);
            this->pNoteSound = nullptr;
        }
        else {
            if (cType == 2) {
                TESTexture* apThis = this->pNoteImage;
                if (apThis) {
                    delete apThis;
                }
                this->pNoteSound = nullptr;
            }
        }
        this->cType = aeVal;
        uint8_t v4 = this->cType;
        if (v4 == 1) {
            TESDescription* v3;
            TESDescription* apMemory = static_cast<TESDescription*>(ourMalloc(sizeof(TESDescription)));
            if (apMemory) {
                v3 = ::new (apMemory) TESDescription;
            }
            else {
                v3 = nullptr;
            }
            this->pNoteText = v3;
        }
        else {
            if (v4 == 2) {
                TESTexture* v2;
                TESTexture* v7 = static_cast<TESTexture*>(ourMalloc(sizeof(TESTexture)));
                if (v7) {
                    v2 = ::new (v7) TESTexture;
                }
                else {
                    v2 = nullptr;
                }
                this->pNoteImage = v2;
            }
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
