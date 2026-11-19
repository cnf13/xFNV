#include "BSShader/BSShaderTextureSet.h"

#include "BSShader/BSShaderManager.h"
#include "Fallout/Interface/Menus/RaceSexMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/ehvec.h"

NiRTTI BSTextureSet::ms_RTTI("BSTextureSet", &NiObject::ms_RTTI);
NiRTTI BSShaderTextureSet::ms_RTTI("BSShaderTextureSet", &BSTextureSet::ms_RTTI);

void BSTextureSet::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
}

void BSTextureSet::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
}

bool BSTextureSet::RegisterStreamables(NiStream& kStream) {
    return NiObject::RegisterStreamables(kStream);
}

void BSTextureSet::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
}

bool BSTextureSet::IsEqual(NiObject* pkObject) {
    return NiObject::IsEqual(pkObject);
}

BSShaderTextureSet::BSShaderTextureSet() {}

// The texture names are destroyed by their own destructors.
BSShaderTextureSet::~BSShaderTextureSet() {}

// The count read from the stream is not bounded: a count above 6 writes past pTextureNames, as the game does. The count
// is signed, and it is zeroed before the read.
void BSShaderTextureSet::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
    int32_t iCount = 0;
    NiStreamLoadBinary(kStream, iCount);
    BSStringT<char>* pName = pTextureNames;
    for (int32_t i = 0; i < iCount; ++i) {
        char* pcName = nullptr;
        kStream.LoadCString(pcName);
        pName->Set(pcName, 0);
        _NiFree(pcName);
        ++pName;
    }
}

void BSShaderTextureSet::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, uint32_t(6));
    for (const BSStringT<char>& rName : pTextureNames) {
        kStream.SaveCString(rName.pString);
    }
}

// Always false; the base comparison is not run. Shared by the shader property IsEqual stubs.
bool BSShaderTextureSet::IsEqual(NiObject*) {
    return false;
}

const char* BSShaderTextureSet::GetTextureFilename(BSShaderProperty::TextureTypeEnum aiType) {
    return pTextureNames[aiType].pString;
}

void BSShaderTextureSet::GetTexture(BSShaderProperty::TextureTypeEnum aiType, NiPointer<NiTexture>& aspTexture) {
    NiPointer<NiTexture> spTexture;
    const char* pcName = GetTextureFilename(aiType);
    if (pcName) {
        BSShaderManager::GetTexture(pcName, true, spTexture, true, aiType == BSShaderProperty::TEXTYPE_ENV);
    }
    aspTexture = spTexture;
}

void BSShaderTextureSet::SetTextureFilename(BSShaderProperty::TextureTypeEnum aiType, const char* apFilename) {
    pTextureNames[aiType].Set(apFilename, 0);
}

NiObject* BSShaderTextureSet::CreateObject() {
    return new BSShaderTextureSet;
}
