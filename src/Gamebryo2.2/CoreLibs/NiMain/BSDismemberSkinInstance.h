#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Reconstruction.hpp"

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

	BSDismemberSkinInstance();
	BSDismemberSkinInstance(NiSkinData* pkSkinData, NiAVObject* pkRootParent, NiAVObject** ppkBones,
		BSDismemberSkinInstance::PartitionData* apPartitionDataArray, uint32_t auiPartitionDataArraySize);
	~BSDismemberSkinInstance() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

	void SetBodyPartEnabled(uint16_t ausBodyPartNum, bool abEnabled);
	// pPartitionDataArray; the exe's shared one-field getter 0x9E32D0 (thiscall, ret 0).
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
inline constexpr fnvd::Vtable BSDismemberSkinInstance_vftable{};
