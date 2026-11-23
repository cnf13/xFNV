#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"

#include <stdint.h>

class NiObjectNET;
class NiStream;

// One furniture marker: where an actor sits, lies or stands on the furniture, and which way it faces.
struct FurnitureMark {
    // The default (all bits set) marks a loaded slot, NiTArray's T(0) an empty one.
    FurnitureMark(uint32_t aiNumber = 0xFFFFFFFFu) : sHeading(0), cNumber(static_cast<uint8_t>(aiNumber)) {}

    // Position, then all other fields; NaN coordinates compare unequal.
    bool operator==(const FurnitureMark& arSource) const;
    // Wraps afHeading (radians) into [0, 2 pi) and stores it in thousandths.
    void SetHeading(float afHeading);
    void LoadBinary(NiStream& arStream);
    void SaveBinary(NiStream& arStream);
    const NiPoint3& QPosition();
    // The heading in radians, rounded to float.
    float GetHeading();
    // Returns the type byte zero-extended to 32 bits; callers pass or compare the whole value (for example to
    // TESFurniture::IsSleepMarker).
    uint32_t GetType() const;
    void SetType(uint8_t acType);
    void SetPosition(const NiPoint3& arPos);

    NiPoint3 Position;
    // The heading in thousandths of a radian.
    uint16_t sHeading;
    uint8_t cNumber;
};

// The "FRN" extra data of a model: where an actor stands, sits or lies when using the furniture.
class BSFurnitureMarker : public NiExtraData {
public:
    BSFurnitureMarker();
    ~BSFurnitureMarker() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

    // Marker auiIndex.
    FurnitureMark& QMark(uint32_t aiMark);
    // The number of markers, aMarks' effective size.
    uint32_t QNumMarks();
    void Init();
    void Add(const FurnitureMark& arMark);

    // Creates the "FRN" tag.
    static void InitSDM();
    // The furniture marker extra data of apObject, or null.
    static BSFurnitureMarker* Find(NiObjectNET* apObject);
    static NiObject* CreateObject();
    static void KillSDM();

    NiTObjectArray<FurnitureMark> aMarks;

    static inline NiFixedString* pTag;
    static NiRTTI ms_RTTI;
};
