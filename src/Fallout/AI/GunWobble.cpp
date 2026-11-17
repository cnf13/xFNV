#include "Fallout/AI/GunWobble.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkBlendCollisionObjectAddRotation.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/BSUtilities.h"
#include "BSSystem/Archive.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

char* GetGunWobble(ANIM_GROUP_WEAPON_ENUM aeWeaponType, float afTime, NiMatrix3& apReturnWobble) {
    NiMatrix3 result;
    NiMatrix3 apResult;
    NiMatrix3 startRot;
    if (pWobbleNode[aeWeaponType] && pWobbleBoneName[aeWeaponType]) {
        NiTimeController* pcontroller = pWobbleNode[aeWeaponType]->GetControllers();
        if (aeWeaponType) {
            float fstartTime = pcontroller->m_fLoKeyTime;
            NiUpdateData UpdateData1(fstartTime, true, false);
            pWobbleNode[aeWeaponType]->Update(UpdateData1);
        }
        startRot = (*&pWobbleNode[aeWeaponType]->m_kLocal).m_Rotate;
        NiUpdateData UpdateData2(afTime, true, false);
        pWobbleNode[aeWeaponType]->Update(UpdateData2);
        NiTransform* LocalTransform = &pWobbleNode[aeWeaponType]->m_kLocal;
        NiMatrix3* v4 = startRot.Inverse(&apResult);
        apReturnWobble = (result = *v4 * LocalTransform->m_Rotate);
        return pWobbleBoneName[aeWeaponType];
    }
    else {
        apReturnWobble = NiMatrix3_IDENTITY;
        return nullptr;
    }
}

void ClearGunWobble(ANIM_GROUP_WEAPON_ENUM aeWeaponType, NiNode* apRoot) {
    if (pWobbleBoneName[aeWeaponType]) {
        NiAVObject* ObjectByName = BSUtilities::GetObjectByName(apRoot, pWobbleBoneName[aeWeaponType]);
        if (ObjectByName) {
            bhkBlendCollisionObjectAddRotation* v2 =
                static_cast<bhkBlendCollisionObjectAddRotation*>(NiObject::DynamicCastStatic(
                    &bhkBlendCollisionObjectAddRotation::ms_RTTI, ObjectByName->GetCollisionObject()
                ));
            if (v2) {
                v2->SetAdditionalRotation(NiMatrix3_IDENTITY);
            }
        }
    }
}

// The two clears cover only the first three entries each, as the game does, not the whole 12-entry tables.
void InitGunWobble() {
    char modelPath[260];
    ourMemset(pWobbleNode, 0, sizeof(NiNode*) * 3);
    ourMemset(pWobbleBoneName, 0, sizeof(char*) * 3);
    bRenameModel = false;
    for (int32_t i = 0; i < 12; ++i) {
        if (i == 0) {
            BSsprintf(modelPath, 0x104, "%s\\Characters\\WeaponWobbles\\ScopeWobble.nif", "Meshes");
        }
        else {
            BSsprintf(modelPath, 0x104, "%s\\Characters\\WeaponWobbles\\%s.nif", "Meshes", animWeaponInfo[i]);
        }
        if (FileFinder::Locate(
                modelPath, nullptr,
                static_cast<FileFinder::LOOKIN_FLAGS>(FileFinder::SKIP_CWD | FileFinder::SKIP_PATHS),
                ARCHIVE_TYPE_MESHES
            )) {
            pWobbleNode[i] = pModelLoader->LoadFile(modelPath, LOD_MULT_NONE, true, false, false, false);
            uint32_t uiSize = BSstrlen(pWobbleNode[i]->GetName().GetHandleString()) + 1;
            pWobbleBoneName[i] = static_cast<char*>(ourMalloc(uiSize));
            BSstrcpy(pWobbleBoneName[i], uiSize, pWobbleNode[i]->GetName().GetHandleString());
            NiFixedString kName;
            NiFixedString* pName = ::new (static_cast<void*>(&kName)) NiFixedString(modelPath);
            pWobbleNode[i]->SetName(*pName);
        }
    }
    bRenameModel = true;
}

void FreeGunWobble() {
    for (int32_t i = 0; i < 12; ++i) {
        if (pWobbleNode[i]) {
            NiFixedString* Name = &pWobbleNode[i]->m_kName;
            const char* HandleString = Name->GetHandleString();
            pModelLoader->ReleaseFile(HandleString);
            pWobbleNode[i] = nullptr;
        }
        if (pWobbleBoneName[i]) {
            ourFree(pWobbleBoneName[i]);
            pWobbleBoneName[i] = nullptr;
        }
    }
}
