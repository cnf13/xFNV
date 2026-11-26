#include "Fallout/AI/AvoidAreaStruct.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectSTAT.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "SDK/MSVC/rtti.h"

bool AvoidAreaStruct::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    pAvoidNode.LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&fTimeExpire, sizeof(fTimeExpire));
    apLoadGameBuffer->LoadData(&afRadiationLevel, sizeof(afRadiationLevel));
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
    pRadiationMarker = dynamic_cast<TESObjectSTAT*>(pForm);
    return apLoadGameBuffer->LoadFormID(pRefObj);
}
