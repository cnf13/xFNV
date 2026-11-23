#include "Fallout/AI/CameraShake.h"

#include "BSCore/BSTimer.h"
#include "BSDevices/Controls.h"
#include "BSHavok/bhkPickData.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

#include <cstdint>

void SetCameraShakeData(float afMult, float afCurrentTime) {
    fCameraShakeMult = afMult;
    fCameraShakeCurrentTime = afCurrentTime;
}

void ShakeCamera(float afMult, const NiPoint3& arPos) {
    if (ShakeData_pModel && afMult > 0.0) {
        NiPoint3 cameraPos = Main::GetWorldSceneGraph()->GetCameraRoot()->GetTranslate();
        float fdelta = (cameraPos - arPos).Length();
        float fFadeStart = fCameraShakeDistFadeStart.Float() * afMult;
        float fFadeDelta = fCameraShakeDistFadeDelta.Float() * afMult;
        if (fCameraShakeDistMin.Float() > static_cast<double>(fFadeStart)) {
            fFadeStart = fCameraShakeDistMin.Float();
        }
        if (fdelta < fFadeStart + fFadeDelta) {
            if (fFadeStart < static_cast<double>(fdelta)) {
                afMult = (1.0 - (fdelta - fFadeStart) / fFadeDelta) * afMult;
            }
            if (afMult > 1.0) {
                afMult = 1.0f;
            }
            else if (fCameraShakeMultMin.Float() > static_cast<double>(afMult)) {
                afMult = fCameraShakeMultMin.Float();
            }
            fCameraShakeMult = afMult;
            fCameraShakeCurrentTime = fCameraShakeTime.Float();
            if (!Interface::IsInMenuMode()) {
                Controls* pcontrols = appMain->GetControls();
                double dShakeTimeMS = fCameraShakeCurrentTime * 1000.0;
                int32_t iTimeInMillis = static_cast<int32_t>(fRumbleShakeTimeMult.Float() * dShakeTimeMS);
                float fRightMotor = fCameraShakeMult * fRumbleShakeRadiusMult.Float();
                float fLeftMotor = fCameraShakeMult * fRumbleShakeRadiusMult.Float();
                pcontrols->Rumble(fLeftMotor, fRightMotor, iTimeInMillis, 0, CONSTANT, false, false);
            }
        }
    }
}

float GetCameraShake(float afTime, NiMatrix3& apReturnShake) {
    NiMatrix3 kInverse;
    NiMatrix3 startRot;
    if (!ShakeData_pModel || fCameraShakeMult == 0.0 || fCameraShakeCurrentTime == 0.0) {
        if (fCameraShakeCurrentTime != 0.0 || fCameraShakeMult != 0.0) {
            fCameraShakeCurrentTime = 0.0f;
            fCameraShakeMult = 0.0f;
        }
        return fCameraShakeMult;
    }
    NiTimeController* pcontroller = ShakeData_pModel->GetControllers();
    float fstartTime = pcontroller->m_fLoKeyTime;
    NiUpdateData UpdateData1(fstartTime, true, false);
    ShakeData_pModel->Update(UpdateData1);
    startRot = ShakeData_pModel->m_kLocal.m_Rotate;
    NiUpdateData UpdateData2(afTime, true, false);
    ShakeData_pModel->Update(UpdateData2);
    NiTransform* pLocalTransform = &ShakeData_pModel->m_kLocal;
    apReturnShake = *startRot.Inverse(&kInverse) * pLocalTransform->m_Rotate;
    fCameraShakeCurrentTime = fCameraShakeCurrentTime - appTimer.fDelta;
    if (fCameraShakeCurrentTime < 0.0) {
        fCameraShakeCurrentTime = 0.0f;
        fCameraShakeMult = 0.0f;
        return fCameraShakeMult;
    }
    if (!(fCameraShakeFadeTime.Float() > static_cast<double>(fCameraShakeCurrentTime))) {
        return fCameraShakeMult;
    }
    double dFadedMult = fCameraShakeMult * fCameraShakeCurrentTime;
    return dFadedMult / fCameraShakeFadeTime.Float();
}

float GetCameraShakeMult() {
    return fCameraShakeMult;
}

float GetCameraShakeCurrentTime() {
    return fCameraShakeCurrentTime;
}
