#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"

class NiAVObject;
class NiSkinData;

// A skin instance whose skin partitions are body parts that dismemberment can hide.
class BSDismemberSkinInstance : public NiSkinInstance {
public:
    // One skin partition: whether it is drawn, whether that just changed, and its body part.
    struct PartitionData {
        bool bEnabled;
        bool bStateChange;
        uint16_t usBodyPartNum;
    };

    // Bases of a partition's body part number: a limb's own partition is its BGSBodyPart::LIMB_ENUM value, the cap
    // on the limb's side of the cut is the limb plus SECTION_CAP, the cap on the torso's side is the limb plus
    // TORSO_CAP, and the torso section of the limb is the limb times TORSO_SECTION.
    static constexpr uint16_t SECTION_CAP = 100;
    static constexpr uint16_t TORSO_CAP = 200;
    static constexpr uint16_t TORSO_SECTION = 1000;

    BSDismemberSkinInstance();
    BSDismemberSkinInstance(
        NiSkinData* pkSkinData,
        NiAVObject* pkRootParent,
        NiAVObject** ppkBones,
        BSDismemberSkinInstance::PartitionData* apPartitionDataArray,
        uint32_t auiPartitionDataArraySize
    );
    ~BSDismemberSkinInstance() override;

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

    void SetBodyPartEnabled(uint16_t ausBodyPartNum, bool abEnabled);
    BSDismemberSkinInstance::PartitionData* GetPartitionDataArray() const;

protected:
    // Copies the partition table into apDest.
    void CopyMembers(BSDismemberSkinInstance* pkDest, NiCloningProcess& kCloning);

public:
    static NiObject* CreateObject();

    uint32_t uiPartitionDataArraySize;
    PartitionData* pPartitionDataArray;
    bool bVisiblePartitions;

    static NiRTTI ms_RTTI;
};

inline BSDismemberSkinInstance::PartitionData* BSDismemberSkinInstance::GetPartitionDataArray() const {
    return pPartitionDataArray;
}
