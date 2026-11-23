#include "Fallout Shared/BGSBodyPart.h"

#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkLimitedHingeConstraint.h"
#include "BSHavok/bhkMalleableConstraint.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/ParticleShaderProperty.h"
#include "Fallout Shared/BGSDebris.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelPSA.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoveTesterMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "xfnv/Compiler.h"

#include <cstdint>

// Reads the part's names, models and data. The chunks come in a fixed order and a missing one fails the load; NAM2 and
// NAM3 are read and dropped.
bool BGSBodyPart::Load(TESFile* apFile) {
    if (!apFile) {
        return false;
    }
    if (apFile->GetTESChunk() == BPTN_ID) {
        // A 16-byte aligned stack buffer of the chunk's size.
        char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
        apFile->GetChunkData(pBuffer, 0);
        PartName.Set(pBuffer, 0);
        if (!apFile->NextChunk()) {
            return false;
        }
    }
    if (apFile->GetTESChunk() == PNAM_ID) {
        TESModel::LoadModelChunk(&PSAModel, apFile);
        if (!apFile->NextChunk()) {
            return false;
        }
    }
    if (apFile->GetTESChunk() != BPNN_ID) {
        return false;
    }
    {
        char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
        apFile->GetChunkData(pBuffer, 0);
        NodeName.Set(pBuffer, 0);
    }
    if (!apFile->NextChunk()) {
        return false;
    }
    if (apFile->GetTESChunk() != BPNT_ID) {
        return false;
    }
    {
        char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
        apFile->GetChunkData(pBuffer, 0);
        TargetName.Set(pBuffer, 0);
    }
    if (!apFile->NextChunk()) {
        return false;
    }
    if (apFile->GetTESChunk() == BPNI_ID) {
        char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
        apFile->GetChunkData(pBuffer, 0);
        IKStartNodeName.Set(pBuffer, 0);
        if (!apFile->NextChunk()) {
            return false;
        }
    }
    if (apFile->GetTESChunk() != BPND_ID) {
        return false;
    }
    // The data is byte-swapped before it is read.
    if (apFile->QEndian()) {
        Data.Endian();
    }
    apFile->GetChunkData(&Data, 0x54u);
    // The file's record is 0x54 bytes, with a 4-byte form ID in each pointer field.
    TESFile::WidenFormIDRecords(
        &Data,
        1,
        0x54,
        sizeof(Data),
        {{0x0C, offsetof(PART_DATA, pExplosionGenericDebris)},
         {0x10, offsetof(PART_DATA, pExplosion)},
         {0x20, offsetof(PART_DATA, pDismemberGenericDebris)},
         {0x24, offsetof(PART_DATA, pDismemberExplosion)},
         {0x44, offsetof(PART_DATA, pDismemberImpactDataSet)},
         {0x48, offsetof(PART_DATA, pExplosionImpactDataSet)}}
    );
    if (apFile->QEndian()) {
        Data.Endian();
    }
    uint8_t ucZeroScale = 0;
    if (Data.fExplosionSpecialDebrisScale == 0.0) {
        Data.fExplosionSpecialDebrisScale = 0.1f;
        ucZeroScale = 1;
    }
    if (Data.fDismemberGenericDebrisScale == 0.0) {
        Data.fDismemberGenericDebrisScale = 0.1f;
        ucZeroScale = 1;
    }
    if (Data.fExplosionGenericDebrisScale == 0.0) {
        Data.fExplosionGenericDebrisScale = 0.1f;
        ucZeroScale = 1;
    }
    if (ucZeroScale) {
        Warning("MODELS: Debris scale value of zero loaded.  Please set to non-zero value in editor.");
    }
    if (!apFile->NextChunk()) {
        return false;
    }
    if (apFile->GetTESChunk() != NAM1_ID) {
        return false;
    }
    if (apFile->GetChunkSize()) {
        char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
        apFile->GetChunkData(pBuffer, 0);
        TESModel* pDebrisModel = &ExplosionSpecialDebris;
        pDebrisModel->SetModel(pBuffer);
    }
    if (apFile->GetFormVersion() < 3) {
        if (!apFile->NextChunk() || apFile->GetTESChunk() != NAM2_ID) {
            return apFile->GetFormVersion() < 2;
        }
        if (apFile->GetChunkSize()) {
            // Reads the chunk into a buffer that is never looked at.
            char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
            apFile->GetChunkData(pBuffer, 0);
        }
        if (!apFile->NextChunk() || apFile->GetTESChunk() != NAM3_ID) {
            return apFile->GetFormVersion() < 2;
        }
        if (apFile->GetChunkSize()) {
            char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
            apFile->GetChunkData(pBuffer, 0);
        }
    }
    if (!apFile->NextChunk() || apFile->GetTESChunk() != NAM4_ID) {
        return apFile->GetFormVersion() < 2;
    }
    if (apFile->GetChunkSize()) {
        char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
        apFile->GetChunkData(pBuffer, 0);
        GoreObjectName.Set(pBuffer, 0);
    }
    if (!apFile->NextChunk() || apFile->GetTESChunk() != NAM5_ID) {
        return true;
    }
    if (apFile->GetChunkSize()) {
        TESModel::LoadModelTextureChunk(&ExplosionSpecialDebris, apFile);
    }
    return true;
}

PART_DATA::PART_DATA() {}

void PART_DATA::Endian() {
    ENDIAN(fDamageMult, false);
    ENDIAN(fTrackingMaxAngle, false);
    ENDIAN(fExplosionGenericDebrisScale, false);
    ENDIAN(fDismemberGenericDebrisScale, false);
    ENDIAN(kGoreTranslate.x, false);
    ENDIAN(kGoreTranslate.y, false);
    ENDIAN(kGoreTranslate.z, false);
    ENDIAN(kGoreRotate.x, false);
    ENDIAN(kGoreRotate.y, false);
    ENDIAN(kGoreRotate.z, false);
    ENDIAN(fExplosionSpecialDebrisScale, false);
}

void PART_DATA::Initialize() {
    cHealthPercent = 5;
    fDamageMult = 1.0f;
    cActorValue = -1;
    cToHitChance = 100;
    cExplosionChance = 0;
    cExplosionGenericDebrisCount = 0;
    pExplosionGenericDebris = nullptr;
    pExplosion = nullptr;
    fTrackingMaxAngle = 0.0f;
    fExplosionGenericDebrisScale = 1.0f;
    fExplosionSpecialDebrisScale = 1.0f;
    cDismemberGenericDebrisCount = 0;
    pDismemberGenericDebris = nullptr;
    pDismemberExplosion = nullptr;
    fDismemberGenericDebrisScale = 1.0f;
    kGoreTranslate = NiPoint3::ZERO;
    kGoreRotate = NiPoint3::ZERO;
    pDismemberImpactDataSet = nullptr;
    pExplosionImpactDataSet = nullptr;
    cDismemberDecalCount = 0;
    cExplosionDecalCount = 0;
}

BGSBodyPart::BGSBodyPart() {
    ourMemset(&Data, 0, sizeof(PART_DATA));
    Data.Initialize();
    InitializeData();
}

BGSBodyPart::~BGSBodyPart() {
    ClearData();
}

void BGSBodyPart::QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) {
    if (ExplosionSpecialDebris.GetModelLength()) {
        pModelLoader->QueueModel(&ExplosionSpecialDebris, aePriority, apParent, LOD_MULT_OBJECTS, true, false, false);
    }
    if (Data.pExplosionGenericDebris) {
        Data.pExplosionGenericDebris->QueueFiles(aePriority, apParent);
    }
    if (Data.pExplosion) {
        Data.pExplosion->QueueFiles(aePriority, apParent);
    }
    if (Data.pDismemberGenericDebris) {
        Data.pDismemberGenericDebris->QueueFiles(aePriority, apParent);
    }
    if (Data.pDismemberExplosion) {
        Data.pDismemberExplosion->QueueFiles(aePriority, apParent);
    }
    if (Data.pDismemberImpactDataSet) {
        Data.pDismemberImpactDataSet->QueueFiles(aePriority, apParent);
    }
    if (Data.pExplosionImpactDataSet) {
        Data.pExplosionImpactDataSet->QueueFiles(aePriority, apParent);
    }
}

// Resolves the form IDs the loader stored in the pointer fields to their forms.
void BGSBodyPart::InitItem(TESForm* apParentForm) {
    if (apParentForm && !apParentForm->GetInitialized()) {
        uint32_t itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pExplosion));
        if (itempInt) {
            TESForm::AddCompileIndex(itempInt, apParentForm->GetFile(-1));
            Data.pExplosion = dynamic_cast<BGSExplosion*>(TESForm::GetFormByNumericID(itempInt));
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pExplosionGenericDebris));
        if (itempInt) {
            TESForm::AddCompileIndex(itempInt, apParentForm->GetFile(-1));
            Data.pExplosionGenericDebris = dynamic_cast<BGSDebris*>(TESForm::GetFormByNumericID(itempInt));
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pDismemberExplosion));
        if (itempInt) {
            TESForm::AddCompileIndex(itempInt, apParentForm->GetFile(-1));
            Data.pDismemberExplosion = dynamic_cast<BGSExplosion*>(TESForm::GetFormByNumericID(itempInt));
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pDismemberGenericDebris));
        if (itempInt) {
            TESForm::AddCompileIndex(itempInt, apParentForm->GetFile(-1));
            Data.pDismemberGenericDebris = dynamic_cast<BGSDebris*>(TESForm::GetFormByNumericID(itempInt));
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetDismemberImpactDataSet()));
        if (itempInt) {
            TESForm::AddCompileIndex(itempInt, apParentForm->GetFile(-1));
            Data.pDismemberImpactDataSet = dynamic_cast<BGSImpactDataSet*>(TESForm::GetFormByNumericID(itempInt));
        }
        itempInt = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pExplosionImpactDataSet));
        if (itempInt) {
            TESForm::AddCompileIndex(itempInt, apParentForm->GetFile(-1));
            Data.pExplosionImpactDataSet = dynamic_cast<BGSImpactDataSet*>(TESForm::GetFormByNumericID(itempInt));
        }
    }
}

BGSBodyPart::LIMB_ENUM BGSBodyPart::GetPartType() {
    return static_cast<LIMB_ENUM>(Data.cType);
}

bool BGSBodyPart::IsIKBiped() {
    return (Data.cFlags & PART_DATA::FLAG_IK_BIPED_DATA) != 0;
}

bool BGSBodyPart::DoesHeadtracking() {
    return (Data.cFlags & PART_DATA::FLAG_IK_HEADTRACKING) != 0;
}

char BGSBodyPart::GetActorValue() {
    return Data.cActorValue;
}

uint8_t BGSBodyPart::GetHealthPercent() {
    return Data.cHealthPercent;
}

bool BGSBodyPart::IsExplodeChanceAbsolute() {
    return (Data.cFlags & PART_DATA::FLAG_TO_HIT_CHANCE_ABSOLUTE) != 0;
}

uint8_t BGSBodyPart::GetExplosionChance() {
    return Data.cExplosionChance;
}

void BGSBodyPart::InitializeData() {
    ExplosionSpecialDebris.InitializeDataComponent();
    PSAModel.InitializeDataComponent();
}

void BGSBodyPart::ClearData() {
    ExplosionSpecialDebris.ClearDataComponent();
    PSAModel.ClearDataComponent();
}

bool BGSBodyPart::IsIKData() {
    return (Data.cFlags & PART_DATA::FLAG_IK_DATA) != 0;
}

uint8_t BGSBodyPart::GetDebrisCount() {
    return Data.cExplosionGenericDebrisCount;
}

uint8_t BGSBodyPart::GetSeveredDecalCount() {
    return Data.cDismemberDecalCount;
}

bool BGSBodyPart::IsExplodablePart() {
    return (Data.cFlags & PART_DATA::FLAG_EXPLODABLE) != 0;
}

bool BGSBodyPart::IsSeverablePart() {
    return (Data.cFlags & PART_DATA::FLAG_SEVERABLE) != 0;
}

const char* BGSBodyPart::GetPartName() {
    return PartName.String();
}

BGSImpactDataSet* BGSBodyPart::GetDismemberImpactDataSet() {
    return Data.pDismemberImpactDataSet;
}

NiPoint3 BGSBodyPart::GetGoreTranslate() {
    return Data.kGoreTranslate;
}

const char* BGSBodyPart::GetGoreObjectName() {
    return GoreObjectName.String();
}

NiPoint3 BGSBodyPart::GetGoreRotate() {
    return Data.kGoreRotate;
}

void BGSBodyPart::UnhideLimb(BGSBodyPart::LIMB_ENUM aeLimb, NiAVObject* apObj) {
    if (apObj) {
        if (apObj->IsNode()) {
            NiNode* pNode = apObj->IsNode();
            for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                BGSBodyPart::UnhideLimb(aeLimb, pNode->GetAt(i));
            }
        }
        else if (apObj->IsGeometry()) {
            NiObject* pkObject = apObj->IsGeometry()->GetSkinInstance();
            BSDismemberSkinInstance* pSkinInstance = static_cast<BSDismemberSkinInstance*>(
                NiObject::DynamicCastStatic(&BSDismemberSkinInstance::ms_RTTI, pkObject)
            );
            if (pSkinInstance) {
                pSkinInstance->SetBodyPartEnabled(aeLimb, true);
                pSkinInstance->SetBodyPartEnabled(aeLimb + BSDismemberSkinInstance::TORSO_CAP, false);
            }
        }
    }
}

void BGSBodyPart::HideLimb(BGSBodyPart::LIMB_ENUM aeLimb, NiAVObject* apObj) {
    if (apObj) {
        if (apObj->IsNode()) {
            NiNode* pNode = apObj->IsNode();
            for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                BGSBodyPart::HideLimb(aeLimb, pNode->GetAt(i));
            }
        }
        else if (apObj->IsGeometry()) {
            NiObject* pkObject = apObj->IsGeometry()->GetSkinInstance();
            BSDismemberSkinInstance* pSkinInstance = static_cast<BSDismemberSkinInstance*>(
                NiObject::DynamicCastStatic(&BSDismemberSkinInstance::ms_RTTI, pkObject)
            );
            if (pSkinInstance) {
                pSkinInstance->SetBodyPartEnabled(aeLimb, false);
                pSkinInstance->SetBodyPartEnabled(aeLimb + BSDismemberSkinInstance::TORSO_CAP, true);
            }
        }
    }
}

void BGSBodyPart::ShowLimb(BGSBodyPart::LIMB_ENUM aeBodyPart, NiObject* apObject) {
    NiNode* pnode = apObject->IsNode();
    uint32_t n = 0;
    while (pnode && n < pnode->GetArrayCount()) {
        NiGeometry* ptri;
        if (pnode->GetAt(n)) {
            ptri = pnode->GetAt(n)->IsGeometry();
        }
        else {
            ptri = apObject->IsGeometry();
        }
        ++n;
        NiSkinInstance* pskin = ptri ? ptri->GetSkinInstance() : nullptr;
        BSDismemberSkinInstance* pdSkinInstance = static_cast<BSDismemberSkinInstance*>(
            NiObject::DynamicCastStatic(&BSDismemberSkinInstance::ms_RTTI, pskin)
        );
        if (pdSkinInstance) {
            for (BGSBodyPart::LIMB_ENUM eindex = BGSBodyPart::LIMB_TORSO; eindex < BGSBodyPart::LIMB_COUNT;
                 eindex = static_cast<BGSBodyPart::LIMB_ENUM>(eindex + 1)) {
                if (eindex == aeBodyPart) {
                    pdSkinInstance->SetBodyPartEnabled(aeBodyPart + BSDismemberSkinInstance::SECTION_CAP, true);
                }
                else {
                    pdSkinInstance->SetBodyPartEnabled(eindex, false);
                    pdSkinInstance->SetBodyPartEnabled(eindex + BSDismemberSkinInstance::TORSO_CAP, false);
                }
                pdSkinInstance->SetBodyPartEnabled(BSDismemberSkinInstance::TORSO_SECTION * eindex, false);
            }
        }
    }
}

uint8_t GetGenericDebrisCount(BGSBodyPart* apThis, [[maybe_unused]] void* apEdx) {
    return apThis->Data.cDismemberGenericDebrisCount;
}

// The constraint's creation info, from the hkSerializableCinfo CreateCinfo returns, null for none. The downcast goes
// through a pointer: the result is tested for null before subtracting the base's offset.
hkLimitedHingeConstraintCinfo& bhkLimitedHingeConstraint::GetCinfo() {
    bool bc;
    hkSerializableCinfo* pCinfo = &CreateCinfo(bc);
    return *static_cast<hkLimitedHingeConstraintCinfo*>(pCinfo);
}

hkMalleableConstraintCinfo& bhkMalleableConstraint::GetCinfo() {
    bool bc;
    hkSerializableCinfo* pCinfo = &CreateCinfo(bc);
    return *static_cast<hkMalleableConstraintCinfo*>(pCinfo);
}

TESModelPSA::TESModelPSA() : TESModel() {}

TESModelPSA::~TESModelPSA() {}
