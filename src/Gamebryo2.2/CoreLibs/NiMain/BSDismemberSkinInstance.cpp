#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

#include <bit>
#include <cstdint>

NiRTTI BSDismemberSkinInstance::ms_RTTI("BSDismemberSkinInstance", &NiObject::ms_RTTI);

namespace {

    // The engine's new[] of partition entries (no constructor), allocated through MemoryManager::Allocate directly. The
    // size saturates on overflow so the allocation fails rather than comes back short.
    BSDismemberSkinInstance::PartitionData* AllocatePartitionData(uint32_t auiCount) {
        uint64_t ulSize = static_cast<uint64_t>(sizeof(BSDismemberSkinInstance::PartitionData)) * auiCount;
        size_t stSize = ulSize > SIZE_MAX ? SIZE_MAX : static_cast<size_t>(ulSize);
        return static_cast<BSDismemberSkinInstance::PartitionData*>(MemoryManager_Singleton.Allocate(stSize));
    }

    // Converts one entry between the file's and the machine's byte order: the body part as a 16-bit value, then the
    // whole entry as a 32-bit one (the order is reversed when converting back).
    void SwapBodyPart(BSDismemberSkinInstance::PartitionData* apData) {
        apData->usBodyPartNum = std::byteswap<uint16_t>(apData->usBodyPartNum);
    }

    void SwapEntry(BSDismemberSkinInstance::PartitionData* apData) {
        uint32_t* puiEntry = reinterpret_cast<uint32_t*>(apData);
        *puiEntry = std::byteswap<uint32_t>(*puiEntry);
    }

} // namespace

// bVisiblePartitions is deliberately left unset, as the game does.
BSDismemberSkinInstance::BSDismemberSkinInstance()
  : NiSkinInstance(nullptr, nullptr, nullptr), uiPartitionDataArraySize(0), pPartitionDataArray(nullptr) {}

BSDismemberSkinInstance::BSDismemberSkinInstance(
    NiSkinData* pkSkinData,
    NiAVObject* pkRootParent,
    NiAVObject** ppkBones,
    BSDismemberSkinInstance::PartitionData* apPartitionDataArray,
    uint32_t auiPartitionDataArraySize
)
  : NiSkinInstance(pkSkinData, pkRootParent, ppkBones), uiPartitionDataArraySize(auiPartitionDataArraySize),
    pPartitionDataArray(apPartitionDataArray), bVisiblePartitions(true) {}

// The partition table is the caller's ourMalloc block, which the instance owns.
BSDismemberSkinInstance::~BSDismemberSkinInstance() {
    if (pPartitionDataArray) {
        ourFree(pPartitionDataArray);
        pPartitionDataArray = nullptr;
        uiPartitionDataArraySize = 0;
    }
}

// Unlike NiSkinInstance, always cloned. The clone keeps the original's root and bones where those are not cloned.
NiObject* BSDismemberSkinInstance::CreateClone(NiCloningProcess& kCloning) {
    BSDismemberSkinInstance* pkClone = new BSDismemberSkinInstance;
    CopyMembers(pkClone, kCloning);

    NiObject* pkClonedObject = nullptr;
    if (kCloning.m_pkCloneMap->GetAt(m_pkRootParent, pkClonedObject)) {
        pkClone->m_pkRootParent = static_cast<NiAVObject*>(pkClonedObject);
    }
    else {
        pkClone->m_pkRootParent = m_pkRootParent;
    }

    uint32_t uiBones = m_spSkinData->m_uiBones;
    pkClone->m_ppkBones = static_cast<NiAVObject**>(_NiMalloc(sizeof(NiAVObject*) * uiBones));
    for (uint32_t i = 0; i < uiBones; i++) {
        if (kCloning.m_pkCloneMap->GetAt(m_ppkBones[i], pkClonedObject)) {
            pkClone->m_ppkBones[i] = static_cast<NiAVObject*>(pkClonedObject);
        }
        else {
            pkClone->m_ppkBones[i] = m_ppkBones[i];
        }
    }

    return pkClone;
}

void BSDismemberSkinInstance::LoadBinary(NiStream& kStream) {
    NiSkinInstance::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, uiPartitionDataArraySize);
    pPartitionDataArray = AllocatePartitionData(uiPartitionDataArraySize);

    // Read as raw entries: two bytes and a 16-bit body part each.
    uint32_t auiComponentSizes[3] = {sizeof(bool), sizeof(bool), sizeof(uint16_t)};
    kStream.Istr()->BinaryRead(
        pPartitionDataArray, sizeof(PartitionData) * uiPartitionDataArraySize, auiComponentSizes, 3
    );

    bVisiblePartitions = false;
    for (uint32_t i = 0; i < uiPartitionDataArraySize; i++) {
        bVisiblePartitions |= pPartitionDataArray[i].bEnabled;
    }
}

void BSDismemberSkinInstance::LinkObject(NiStream& kStream) {
    NiSkinInstance::LinkObject(kStream);
}

bool BSDismemberSkinInstance::RegisterStreamables(NiStream& kStream) {
    return NiSkinInstance::RegisterStreamables(kStream);
}

void BSDismemberSkinInstance::SaveBinary(NiStream& kStream) {
    NiSkinInstance::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, uiPartitionDataArraySize);

    // The entries are written as one 32-bit block, so a big-endian file gets them converted in place first and
    // restored afterwards.
    if (!kStream.m_bSaveLittleEndian) {
        for (uint32_t i = 0; i < uiPartitionDataArraySize; i++) {
            SwapBodyPart(&pPartitionDataArray[i]);
            SwapEntry(&pPartitionDataArray[i]);
        }
    }

    uint32_t uiComponentSize = sizeof(PartitionData);
    kStream.Ostr()->BinaryWrite(
        pPartitionDataArray, sizeof(PartitionData) * uiPartitionDataArraySize, &uiComponentSize, 1
    );

    if (!kStream.m_bSaveLittleEndian) {
        for (uint32_t i = 0; i < uiPartitionDataArraySize; i++) {
            SwapEntry(&pPartitionDataArray[i]);
            SwapBodyPart(&pPartitionDataArray[i]);
        }
    }
}

bool BSDismemberSkinInstance::IsEqual(NiObject* pkObject) {
    if (!NiSkinInstance::IsEqual(pkObject)) {
        return false;
    }

    BSDismemberSkinInstance* pkSkinInstance = static_cast<BSDismemberSkinInstance*>(pkObject);

    if (uiPartitionDataArraySize != pkSkinInstance->uiPartitionDataArraySize) {
        return false;
    }

    for (uint32_t i = 0; i < uiPartitionDataArraySize; i++) {
        const PartitionData& rkData = pPartitionDataArray[i];
        const PartitionData& rkOtherData = pkSkinInstance->pPartitionDataArray[i];
        if (rkData.bEnabled != rkOtherData.bEnabled || rkData.bStateChange != rkOtherData.bStateChange ||
            rkData.usBodyPartNum != rkOtherData.usBodyPartNum) {
            return false;
        }
    }

    return true;
}

void BSDismemberSkinInstance::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiSkinInstance::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(ms_RTTI.m_pcName));
    pkStrings->Add(NiGetViewerString("Num Partitions", uiPartitionDataArraySize));
    pkStrings->Add(NiGetViewerString("Visible", bVisiblePartitions));

    for (uint32_t i = 0; i < uiPartitionDataArraySize; i++) {
        const PartitionData& rkData = pPartitionDataArray[i];
        char acPartition[64];
        BSsprintf(
            acPartition, sizeof(acPartition), "%d = BodyPart %d [%s%s]", i, rkData.usBodyPartNum,
            rkData.bEnabled ? "Enabled" : "Disabled", rkData.bStateChange ? ", State Change" : ""
        );
        pkStrings->Add(NiGetViewerString(acPartition));
    }
}

void BSDismemberSkinInstance::SetBodyPartEnabled(uint16_t ausBodyPartNum, bool abEnabled) {
    uint32_t i;
    bVisiblePartitions = false;
    for (i = 0; i < uiPartitionDataArraySize; ++i) {
        if (pPartitionDataArray[i].usBodyPartNum == ausBodyPartNum) {
            pPartitionDataArray[i].bEnabled = (bool)((uint8_t)abEnabled);
        }
        bVisiblePartitions |= ((uint8_t)pPartitionDataArray[i].bEnabled);
    }
}

void BSDismemberSkinInstance::CopyMembers(BSDismemberSkinInstance* pkDest, NiCloningProcess& kCloning) {
    NiSkinInstance::CopyMembers(pkDest, kCloning);

    pkDest->uiPartitionDataArraySize = uiPartitionDataArraySize;
    if (uiPartitionDataArraySize) {
        pkDest->pPartitionDataArray = AllocatePartitionData(uiPartitionDataArraySize);
        for (uint32_t i = 0; i < uiPartitionDataArraySize; i++) {
            pkDest->pPartitionDataArray[i].bEnabled = pPartitionDataArray[i].bEnabled;
            pkDest->pPartitionDataArray[i].bStateChange = pPartitionDataArray[i].bStateChange;
            pkDest->pPartitionDataArray[i].usBodyPartNum = pPartitionDataArray[i].usBodyPartNum;
        }
    }
    else {
        pkDest->pPartitionDataArray = nullptr;
    }

    pkDest->bVisiblePartitions = bVisiblePartitions;
}

NiObject* BSDismemberSkinInstance::CreateObject() {
    return new BSDismemberSkinInstance;
}
