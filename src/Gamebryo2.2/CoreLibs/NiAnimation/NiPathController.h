#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"

#include <stdint.h>

class NiFloatData;
class NiMatrix3;

// Moves its target along a path curve.
class NiPathController : public NiTimeController {
public:
    enum BankDir {
        NEGATIVE = -1,
        POSITIVE = 1,
    };

    // Bits of m_uFlags.
    enum {
        CVDATA_NEEDS_UPDATE_MASK = 0x1,
        CURVE_TYPE_OPEN_MASK = 0x2,
        ALLOW_FLIP_MASK = 0x4,
        BANK_MASK = 0x8,
        CONST_VELOCITY_MASK = 0x10,
        FOLLOW_MASK = 0x20,
        FLIP_MASK = 0x40,
    };

    NiPathController();
    ~NiPathController() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void ProcessClone(NiCloningProcess& kCloning) override;
    void Update(NiUpdateData& UpdateData) override;
    bool IsTransformController() const override {
        return true;
    }

protected:
    bool TargetIsRequiredType() const override;

public:
    // Length of the whole path; computed (with the partial lengths) when the cached value is negative.
    float GetTotalPathLength();

protected:
    // The base's members, then clones of the path and percent data, the flags and the partial
    // lengths onto apDest.
    void CopyMembers(NiPathController* pkDest, NiCloningProcess& kCloning);

    // The path keys bracketing fPercent of the path, and the normalised time between them.
    void GetPercentLoc(float fPercent, uint32_t& uiI1, uint32_t& uiI2, float& fNormTime);
    // Orientation along the path between keys uiI1 and uiI2 (follow, bank and flip applied).
    void CalcRefFrame(uint32_t ui1, uint32_t ui2, float fNormTime, NiMatrix3& kRefFrame);
    NiPoint3 GetLoc(uint32_t uiI1, uint32_t uiI2, float fNormTime);
    // The path's speed between keys aiI0 and aiI1 at afTime.
    float PathSpeed(int32_t iI0, int32_t iI1, float fTime) const;
    // The path's length between keys aiI0 and aiI1 up to afTime.
    float PathLength(int32_t iI0, int32_t iI1, float fTime) const;
    // The path keys bracketing the point afPercent of the way along the path's length, and the normalised time between
    // them (Newton iteration on PathLength).
    void GetPercentLocFromLength(float afPercent, uint32_t& auiI1, uint32_t& auiI2, float& afNormTime);

public:
    // The path data's keys, key count, key type and key size; all zero without path data.
    void GetPathData(NiPosKey*& apPathKeys, uint32_t& auiNumKeys, NiPosKey::KeyType& aeType, uint8_t& aucSize) const;

    static NiObject* CreateObject();

    uint16_t m_uFlags;
    uint32_t m_uiLastPathIdx;
    uint32_t m_uiLastPctIdx;
    NiPointer<NiPosData> m_spPathData;
    NiPointer<NiFloatData> m_spPctData;
    float* m_pfPartialLength;
    float m_fTotalLength;
    float m_fMaxBankAngle;
    float m_fSmoothing;
    int16_t m_sFollowAxis;
    float m_fMaxCurvature;
    BankDir m_eBankDir;

    static NiRTTI ms_RTTI;
};

// Guessed bound: the database gives none; the next data item starts 0x14 bytes on.
using PathLength_s_afModCoeff_t = float[5];
