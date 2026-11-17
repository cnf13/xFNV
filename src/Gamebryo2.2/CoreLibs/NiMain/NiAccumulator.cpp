#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

NiRTTI NiAccumulator::ms_RTTI("NiAccumulator", &NiObject::ms_RTTI);

NiAccumulator::NiAccumulator() : m_pkCamera(nullptr) {}

NiAccumulator::~NiAccumulator() {}

void NiAccumulator::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
}

void NiAccumulator::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
}

bool NiAccumulator::RegisterStreamables(NiStream& kStream) {
    return NiObject::RegisterStreamables(kStream);
}

void NiAccumulator::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
}

bool NiAccumulator::IsEqual(NiObject* pkObject) {
    return NiObject::IsEqual(pkObject);
}

void NiAccumulator::StartAccumulating(const NiCamera* pkCamera) {
    m_pkCamera = const_cast<NiCamera*>(pkCamera);
}

void NiAccumulator::FinishAccumulating() {
    m_pkCamera = nullptr;
}

bool NiAccumulator::IsAccumulating() const {
    return m_pkCamera != nullptr;
}

void NiAccumulator::CopyMembers(NiAccumulator* pkDest, NiCloningProcess& kCloning) {
    NiObject::CopyMembers(pkDest, kCloning);
}
