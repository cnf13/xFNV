#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

NiRTTI NiShader::ms_RTTI("NiShader", nullptr);
NiRTTI NiShader::NiShaderInstanceDescriptor::ms_RTTI("NiShader::NiShaderInstanceDescriptor", nullptr);

NiShader::NiShader() : m_uiImplementation(0), m_bInitialized(false) {}

NiShader::~NiShader() {}

uint32_t NiShader::GetImplementation() const {
    return m_uiImplementation;
}

void NiShader::SetName(const NiFixedString& kName) {
    m_kName.Assign(kName);
}

const NiRTTI* NiShader::NiShaderInstanceDescriptor::GetRTTI() const {
    return &NiShader::NiShaderInstanceDescriptor::ms_RTTI;
}

// The type queries: a shader instance descriptor is none of these classes.
NiNode* NiShader::NiShaderInstanceDescriptor::IsNode() {
    return nullptr;
}

BSFadeNode* NiShader::NiShaderInstanceDescriptor::IsFadeNode() {
    return nullptr;
}

BSMultiBoundNode* NiShader::NiShaderInstanceDescriptor::IsMultiBoundNode() {
    return nullptr;
}

NiGeometry* NiShader::NiShaderInstanceDescriptor::IsGeometry() {
    return nullptr;
}

NiTriBasedGeom* NiShader::NiShaderInstanceDescriptor::IsTriBasedGeom() {
    return nullptr;
}

NiTriStrips* NiShader::NiShaderInstanceDescriptor::IsTriStrips() {
    return nullptr;
}

NiTriShape* NiShader::NiShaderInstanceDescriptor::IsTriShape() {
    return nullptr;
}

BSSegmentedTriShape* NiShader::NiShaderInstanceDescriptor::IsSegmentedTriShape() {
    return nullptr;
}

BSResizableTriShape* NiShader::NiShaderInstanceDescriptor::IsResizableTriShape() {
    return nullptr;
}

NiParticles* NiShader::NiShaderInstanceDescriptor::IsParticlesGeom() {
    return nullptr;
}

NiLines* NiShader::NiShaderInstanceDescriptor::IsLinesGeom() {
    return nullptr;
}

bhkNiCollisionObject* NiShader::NiShaderInstanceDescriptor::IsBhkNiCollisionObject() {
    return nullptr;
}

bhkBlendCollisionObject* NiShader::NiShaderInstanceDescriptor::IsBhkBlendCollisionObject() {
    return nullptr;
}

bhkRigidBody* NiShader::NiShaderInstanceDescriptor::IsBhkRigidBody() const {
    return nullptr;
}

bhkLimitedHingeConstraint* NiShader::NiShaderInstanceDescriptor::IsBhkLimitedHingeConstraint() {
    return nullptr;
}

NiShader::NiShaderInstanceDescriptor::~NiShaderInstanceDescriptor() {}

void NiShader::NiShaderInstanceDescriptor::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}
