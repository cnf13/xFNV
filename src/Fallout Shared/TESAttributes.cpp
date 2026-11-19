#include "Fallout Shared/TESAttributes.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"

TESAttributes::TESAttributes() {
    TESAttributes::InitializeDataComponent();
}

TESAttributes::~TESAttributes() {}

void TESAttributes::InitializeDataComponent() {
    for (uint8_t& cValue : cAttribute) {
        cValue = (uint8_t)5;
    }
}

void TESAttributes::CopyComponent(BaseFormComponent* apCopyFC) {
    int32_t v2;
    int32_t i;
    TESAttributes* v5 = dynamic_cast<TESAttributes*>(apCopyFC);
    if (v5) {
        for (i = 0; i < 7; ++i) {
            v2 = ActorValue::ToActorValue(ActorValue::ATTRIBUTE, i);
            cAttribute[i] = v5->GetAttributeValue(static_cast<ActorValue::Index>(v2));
        }
    }
}

bool TESAttributes::CompareComponent(BaseFormComponent* apCompareFC) {
    TESAttributes* v7 = dynamic_cast<TESAttributes*>(apCompareFC);
    if (!v7) {
        return true;
    }
    for (int32_t i = 0; i < 7; ++i) {
        int32_t v3 = cAttribute[i];
        ActorValue::Index v4 = ActorValue::ToActorValue(ActorValue::ATTRIBUTE, i);
        if (v3 != v7->GetAttributeValue(v4)) {
            return true;
        }
    }
    return false;
}

void TESAttributes::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_BASE_ATTRIBUTES)) {
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(cAttribute), 7, 0);
    }
}

void TESAttributes::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_BASE_ATTRIBUTES)) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(cAttribute), 7);
    }
}

void TESAttributes::SaveGameTES(uint32_t aiFlags) {
    ;
}

void TESAttributes::LoadGameTES(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    ;
}

// The bytes the attributes add to a save: none. The 7 bytes are added under `aiFlags & 0`, a change flag compiled out
// to zero, so the test never passes.
uint16_t TESAttributes::GetSaveSize(uint32_t aiFlags) {
    uint16_t sSize = 0;
    if (aiFlags & 0) {
        sSize += 7;
    }
    return sSize;
}

// The attributes are stored from ActorValue::STRENGTH on.
uint8_t TESAttributes::GetAttributeValue(ActorValue::Index aeAttribute) const {
    return cAttribute[aeAttribute - ActorValue::STRENGTH];
}

void TESAttributes::SetAttributeValue(ActorValue::Index aeAttribute, uint8_t acValue, bool abMarkChange) {
    cAttribute[aeAttribute - ActorValue::STRENGTH] = acValue;
    if (abMarkChange) {
        TESForm* pform = dynamic_cast<TESForm*>(this);
        if (pform) {
            pform->AddChange(4);
        }
    }
}

uint32_t TESAttributes::GetAttributeDataSize() {
    return sizeof(cAttribute);
}

// Copies the seven attributes out to apData; returns their size.
uint32_t TESAttributes::Save(void* apData) {
    ourMemcpy(apData, cAttribute, 7);
    return GetAttributeDataSize();
}

// Copies the seven attributes in from apData; returns their size.
uint32_t TESAttributes::Load(void* apData) {
    ourMemcpy(cAttribute, apData, 7);
    return GetAttributeDataSize();
}
