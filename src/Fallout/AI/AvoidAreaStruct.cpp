#include "Fallout/AI/AvoidAreaStruct.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectSTAT.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "SDK/MSVC/rtti.h"

bool AvoidAreaStruct::LoadGameBGS(BGSLoadFormBuffer* arg0) {
    pAvoidNode.LoadGame(arg0);
    arg0->LoadData(&fTimeExpire, 4u);
    arg0->LoadData(&afRadiationLevel, 4u);
    uint32_t FormID_0 = arg0->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    pRadiationMarker = dynamic_cast<TESObjectSTAT*>(FormByNumericID);
    return arg0->LoadFormID(pRefObj);
}
