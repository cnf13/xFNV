#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "Fallout Shared/BGSTextureModel.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "SDK/FaceGen/MatrixVT.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESAttributes.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESReactionForm.h"
#include "Fallout Shared/TESSpellList.h"
#include "Fallout Shared/TESTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "SDK/MSVC/ehvec.h"

class BGSVoiceType;
class BSFaceGenNiNode;
class FaceGenNpcData;
struct FaceGenUndo;
class TESEyes;
class TESHair;
class TESNPC;

enum BIPED_OBJECT : int32_t;

// The body parts a race has textures and models for.
enum RACE_BODY_PART : int32_t {
    RBP_UPPER_BODY = 0,
    RBP_LEFT_HAND = 1,
    RBP_RIGHT_HAND = 2,
    RACE_BODY_PART_COUNT = 3,
};

// A skill a race raises, and by how much.
struct SKILL_BONUS {
    char eSkill;
    char cBonus;
};

// A race's DATA chunk.
struct RACE_DATA {
    // The race flag bits of iFlags.
    enum : int32_t {
        RACE_FLAG_PLAYABLE = 1,
        RACE_FLAG_CHILD = 4,
    };

    // Byte-swaps the record read from a big-endian file: the heights, weights and flags (the skill bonuses are bytes).
    void Endian();

    SKILL_BONUS eSkillBonus[7];
    float fHeight[2];
    float fWeight[2];
    int32_t iFlags;
};

// A race form (RACE).
class TESRace : public TESForm, public TESFullName, public TESDescription, public TESSpellList, public TESReactionForm {
public:
    using HeadPartNames = char* [8];
    using HeadPartFileDefaults = char* [2][RACE_HEAD_PART_COUNT];
    using BodyPartFileDefaults = char* [2][RACE_BODY_PART_COUNT];
    using BodyTextureModelDefaults = char* [2];

    TESRace();
    ~TESRace() override;

    void InitializeData() override;
    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    const char* GetFormEditorID() override;
    bool SetFormEditorID(const char* apEditorID) override;
    using TESForm::Save;

    // Empties the list of eye colors.
    void ClearEyeColorList();
    void GetBodyModTextureName(TESNPC* apNPC, char* apName);
    bool GetBodyModTextureFileName(TESNPC* apNPC, const char* apName, char* apFileName);
    TESHair* GetDefaultHair(TESActorBase::SEX aeSex);
    BGSVoiceType* GetDefaultVoiceType(TESActorBase::SEX aeSex);

    float GetHeight(TESActorBase::SEX aeSex) const;
    float GetWeight(TESActorBase::SEX aeSex) const;
    bool GetChildRace() const;
    bool GetPlayable();
    void SetDefaultVoiceType(TESActorBase::SEX aeSex, BGSVoiceType* apVoiceType);
    BSSimpleList<TESHair*>* GetHairList();
    TESHair* GetRaceHair(uint32_t aiFormID);
    void AddHair(TESHair* apHair);
    void SetDefaultHair(TESActorBase::SEX aeSex, TESHair* apHair);
    void ClearHairList();
    // The male and the female face shape clamps, 5 and 3 when the race sets none above zero.
    float GetClampFaceGeoValue();
    float GetClampFaceGeoValue2();
    void SetDefaultHairColor(TESActorBase::SEX aeSex, char acDefaultHairColor);
    BSSimpleList<TESEyes*>* GetEyeColorList();
    TESEyes* GetRaceEyeColor(uint32_t aiFormID);
    void AddEyeColor(TESEyes* apEyes);
    TESModel* GetHeadPartModel(TESActorBase::SEX aeSex, RACE_HEAD_PART aePart);
    TESTexture* GetHeadPartTexture(TESActorBase::SEX aeSex, RACE_HEAD_PART aePart);
    TESModel* GetBodyPartModel(TESActorBase::SEX aeSex, RACE_BODY_PART aePart);
    TESModel* GetBodyTextureModel(TESActorBase::SEX aeSex);
    FR2MatrixVTC<float> (*GetMeanFaceCoord(TESActorBase::SEX aeSex))[2][2];
    uint16_t GetFaceCoordNum() const;
    bool GetBodyTextureFileName(TESNPC* apNPC, BIPED_OBJECT aePart, char* apFileName);
    bool GetBodyTexture(
        NiPointer<NiTexture>& aspOutBaseTexture,
        NiPointer<NiTexture>& aspOutModTexture,
        TESNPC* apNPC,
        BIPED_OBJECT aePart,
        bool abForceCreate
    );
    void KillEGTData();
    TESModel* GetPartModel(TESActorBase::SEX aeSex, BIPED_OBJECT aePart);
    TESRace* GetYoungerRace() const {
        return pYoungRace;
    }
    TESRace* GetOlderRace() const {
        return pOldRace;
    }
    char GetDefaultHairColor(TESActorBase::SEX aeSex);
    // Fills arNpcData with apNPC's face data for sex aeSex.
    void GetFaceGenData(TESNPC* apNPC, FaceGenNpcData& arNpcData, TESActorBase::SEX aeSex, bool abNoModTextures);
    TESEyes* GetDefaultEyeColor();
    // &Data.eSkillBonus[aiIndex].
    SKILL_BONUS* GetSkillBonus(int32_t aiIndex);
    // &ppBodyTextureFiles[aeSex][aePart], null outside them.
    TESTexture* GetBodyPartTexture(TESActorBase::SEX aeSex, RACE_BODY_PART aePart);
    void SetHeadPartModel(TESActorBase::SEX aeSex, RACE_HEAD_PART aePart, const char* apFilename);
    void SetBodyPartModel(TESActorBase::SEX aeSex, RACE_BODY_PART aePart, const char* apFilename);
    void SetBodyTextureModel(TESActorBase::SEX aeSex, const char* apFilename);
    // Sets the head part texture of sex auiSex and part auiPart (a RACE_HEAD_PART); a part past the last or a sex
    // index above 2 does nothing.
    void SetHeadTextureName(uint32_t auiSex, uint32_t auiPart, const char* apFilename);
    // Sets the body part texture of sex auiSex and part auiPart (a RACE_BODY_PART); out-of-range indices do nothing.
    void SetBodyTextureName(uint32_t auiSex, uint32_t auiPart, const char* apFilename);
    bool CreateHead(
        BSFaceGenNiNode*& apOutNodeBiped,
        BSFaceGenNiNode*& apOutNodeSkinned,
        TESNPC* apNPC,
        bool abPrepareObjects,
        bool abForceNoSkinning,
        TESActorBase::SEX aeSex
    );

    static const char* GetHeadPartNodeName(RACE_HEAD_PART aePart);
    static RACE_BODY_PART BodyPartToRaceBodyPart(BIPED_OBJECT aePart);
    static bool GetFaceGenTexturing();

    RACE_DATA Data;
    TESAttributes MaleAttributes;
    TESAttributes FemaleAttributes;
    BSSimpleList<TESHair*> HairList;
    TESHair* pDefaultHair[2];
    char cDefaultHairColor[2];
    float fClampFaceGeoValue;
    float fClampFaceGeoValue2;
    BSSimpleList<TESEyes*> EyeList;
    TESModel ppHeadModelFiles[2][RACE_HEAD_PART_COUNT];
    TESTexture ppHeadTextureFiles[2][RACE_HEAD_PART_COUNT];
    TESTexture ppBodyTextureFiles[2][RACE_BODY_PART_COUNT];
    TESModel ppBodyModelFiles[2][RACE_BODY_PART_COUNT];
    BGSTextureModel pBodyTextureModels[2];
    FR2MatrixVTC<float> MeanFaceCoordFemale[2][2];
    FR2MatrixVTC<float> MeanFaceCoordMale[2][2];
    uint16_t sFaceCoordNum;
    BGSVoiceType* pDefaultVoiceType[2];
    TESRace* pOldRace;
    TESRace* pYoungRace;
    BSStringT<char> strEditorID;
    NiTPrimitiveArray<FaceGenUndo*> FaceGenUndoStates;

    static inline HeadPartNames ppHeadPartNames{
        const_cast<char*>("FaceGenFace"),
        const_cast<char*>("FaceGenEars"),
        const_cast<char*>("FaceGenMouth"),
        const_cast<char*>("FaceGenTeethLower"),
        const_cast<char*>("FaceGenTeethUpper"),
        const_cast<char*>("FaceGenTongue"),
        const_cast<char*>("FaceGenEyeLeft"),
        const_cast<char*>("FaceGenEyeRight")
    };
    static inline HeadPartFileDefaults pppHeadModelFileDefaults{
        {const_cast<char*>("Characters\\Head\\HeadHuman.nif"),
         const_cast<char*>(""),
         const_cast<char*>("Characters\\Head\\MouthHuman.nif"),
         const_cast<char*>("Characters\\Head\\TeethLowerHuman.nif"),
         const_cast<char*>("Characters\\Head\\TeethUpperHuman.nif"),
         const_cast<char*>("Characters\\Head\\TongueHuman.nif"),
         const_cast<char*>("Characters\\Head\\EyeLeftHuman.nif"),
         const_cast<char*>("Characters\\Head\\EyeRightHuman.nif")},
        {const_cast<char*>("Characters\\Head\\HeadFemale.nif"),
         const_cast<char*>(""),
         const_cast<char*>("Characters\\Head\\MouthHuman.nif"),
         const_cast<char*>("Characters\\Head\\TeethLowerHuman.nif"),
         const_cast<char*>("Characters\\Head\\TeethUpperHuman.nif"),
         const_cast<char*>("Characters\\Head\\TongueHuman.nif"),
         const_cast<char*>("Characters\\Head\\EyeLeftHuman.nif"),
         const_cast<char*>("Characters\\Head\\EyeRightHuman.nif")}
    };
    static inline HeadPartFileDefaults pppHeadTextureFileDefaults{
        {const_cast<char*>("Characters\\Head\\HeadHuman.dds"),
         const_cast<char*>("Characters\\Head\\EarsHuman.dds"),
         const_cast<char*>("Characters\\Head\\MouthHuman.dds"),
         const_cast<char*>("Characters\\Head\\TeethLowerHuman.dds"),
         const_cast<char*>("Characters\\Head\\TeethUpperHuman.dds"),
         const_cast<char*>("Characters\\Head\\TongueHuman.dds")},
        {const_cast<char*>("Characters\\Head\\HeadHuman.dds"),
         const_cast<char*>("Characters\\Head\\EarsHuman.dds"),
         const_cast<char*>("Characters\\Head\\MouthHuman.dds"),
         const_cast<char*>("Characters\\Head\\TeethLowerHuman.dds"),
         const_cast<char*>("Characters\\Head\\TeethUpperHuman.dds"),
         const_cast<char*>("Characters\\Head\\TongueHuman.dds")}
    };
    static inline BodyPartFileDefaults pppBodyTextureFileDefaults{
        {const_cast<char*>("Characters\\Male\\UpperBodyMale.dds"), const_cast<char*>("Characters\\Male\\HandMale.dds")},
        {const_cast<char*>("Characters\\Female\\UpperBodyFemale.dds"),
         const_cast<char*>("Characters\\Female\\HandFemale.dds")}
    };
    static inline BodyPartFileDefaults pppBodyModelFileDefaults{
        {const_cast<char*>("characters\\_Male\\UpperBody.nif"),
         const_cast<char*>("characters\\_Male\\LeftHand.nif"),
         const_cast<char*>("characters\\_Male\\RightHand.nif")},
        {const_cast<char*>("characters\\_Female\\UpperBody.nif"),
         const_cast<char*>("characters\\_Female\\LeftHand.nif"),
         const_cast<char*>("characters\\_Female\\RightHand.nif")}
    };
    static inline BodyTextureModelDefaults ppBodyTextureModelDefaults{
        const_cast<char*>("Characters\\_Male\\UpperBodyHumanMale.egt"),
        const_cast<char*>("Characters\\_Male\\UpperBodyHumanFemale.egt")
    };
    // bFaceGenTexturing:General: whether faces are textured from the FaceGen texture files.
    static SettingT<INISettingCollection> bFaceGenTexturing;
    static SettingT<GameSettingCollection> fRaceGeneticVariation;
};
