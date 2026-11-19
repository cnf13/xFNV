#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialResource.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "SDK/Windows/winbase.h"

NiMaterialResource::~NiMaterialResource() {}

void NiMaterialResource::SetType(const NiFixedString& kType) {
    m_kType.Assign(kType);
}

void NiMaterialResource::SetSemantic(const NiFixedString& kSemantic) {
    m_kSemantic.Assign(kSemantic);
}

void NiMaterialResource::SetLabel(const NiFixedString& kLabel) {
    m_kLabel.Assign(kLabel);
}

void NiMaterialResource::SetVariable(const NiFixedString& kVariable) {
    m_kVariable.Assign(kVariable);
}

void NiMaterialResource::SetDefaultValue(const NiFixedString& kValue) {
    m_kDefaultValue.Assign(kValue);
}
