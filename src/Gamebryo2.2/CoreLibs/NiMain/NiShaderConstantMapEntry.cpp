#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderConstantMapEntry.h"

#include "BSShader/SpeedTreeLeafShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderAttributeDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiShaderConstantMapEntry::NiShaderConstantMapEntry()
  : m_bEnabled(true), m_uiFlags(0), m_uiExtra(0), m_uiShaderRegister(0), m_uiRegisterCount(0), m_uiDataSize(0),
    m_uiDataStride(0), m_pvDataSource(nullptr), m_bOwnData(false), m_bVariableHookupValid(false),
    m_bRegisterOnly(false) {}

void NiShaderConstantMapEntry::SetData(uint32_t uiDataSize, uint32_t uiDataStride, void* pvDataSource, bool bCopyData) {
    void* m_pvDataSource;
    void* v7;
    void* v8;
    m_uiDataSize = uiDataSize;
    m_uiDataStride = uiDataStride;
    if (bCopyData) {
        m_pvDataSource = this->m_pvDataSource;
        v8 = this->m_pvDataSource;
        m_bOwnData = true;
        _NiFree(v8);
        v7 = _NiMalloc(uiDataSize);
        this->m_pvDataSource = v7;
        rt::memcpy(v7, pvDataSource, uiDataSize);
    }
    else {
        m_bOwnData = false;
        this->m_pvDataSource = pvDataSource;
    }
}

// Initializes the attribute type and flag lookup tables used by shader constant map entries.
void NiShaderConstantMapEntry::InitializeAttribTable() {
    ms_aeAttribType[0] = NiShaderAttributeDesc::ATTRIB_TYPE_UNDEFINED;
    ms_aeAttribType[1] = NiShaderAttributeDesc::ATTRIB_TYPE_BOOL;
    ms_aeAttribType[2] = NiShaderAttributeDesc::ATTRIB_TYPE_STRING;
    ms_aeAttribType[3] = NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT;
    ms_aeAttribType[4] = NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT;
    ms_aeAttribType[5] = NiShaderAttributeDesc::ATTRIB_TYPE_POINT2;
    ms_aeAttribType[6] = NiShaderAttributeDesc::ATTRIB_TYPE_POINT3;
    ms_aeAttribType[7] = NiShaderAttributeDesc::ATTRIB_TYPE_POINT4;
    ms_aeAttribType[8] = NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX3;
    ms_aeAttribType[9] = NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4;
    ms_aeAttribType[10] = NiShaderAttributeDesc::ATTRIB_TYPE_COLOR;
    ms_aeAttribType[11] = NiShaderAttributeDesc::ATTRIB_TYPE_TEXTURE;
    ms_aeAttribType[12] = NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT8;
    ms_aeAttribType[13] = NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT12;
    ms_aucAttribFlags[0] = 0;
    ms_aucAttribFlags[1] = 1;
    ms_aucAttribFlags[2] = 2;
    ms_aucAttribFlags[3] = 3;
    ms_aucAttribFlags[4] = 4;
    ms_aucAttribFlags[5] = 5;
    ms_aucAttribFlags[6] = 6;
    ms_aucAttribFlags[7] = 7;
    ms_aucAttribFlags[8] = 8;
    ms_aucAttribFlags[9] = 9;
    ms_aucAttribFlags[10] = 10;
    ms_aucAttribFlags[11] = 11;
    ms_aucAttribFlags[12] = 12;
    ms_aucAttribFlags[13] = 13;
    ms_bAttribTableInitialized = 1;
}

// Initializes the object type and flag lookup tables used by shader constant map entries.
void NiShaderConstantMapEntry::InitializeObjectTable() {
    ms_ausObjectFlags[0] = 0;
    ms_ausObjectFlags[1] = 0x100;
    ms_ausObjectFlags[2] = 0x200;
    ms_ausObjectFlags[3] = 0x300;
    ms_ausObjectFlags[4] = 0x400;
    ms_ausObjectFlags[5] = 0x500;
    ms_ausObjectFlags[6] = 0x600;
    ms_aeObjectType[0] = NiShaderAttributeDesc::OT_UNDEFINED;
    ms_aeObjectType[1] = NiShaderAttributeDesc::OT_EFFECT_GENERALLIGHT;
    ms_aeObjectType[2] = NiShaderAttributeDesc::OT_EFFECT_POINTLIGHT;
    ms_aeObjectType[3] = NiShaderAttributeDesc::OT_EFFECT_DIRECTIONALLIGHT;
    ms_aeObjectType[4] = NiShaderAttributeDesc::OT_EFFECT_SPOTLIGHT;
    ms_aeObjectType[5] = NiShaderAttributeDesc::OT_EFFECT_SHADOWPOINTLIGHT;
    ms_aeObjectType[6] = NiShaderAttributeDesc::OT_EFFECT_SHADOWDIRECTIONALLIGHT;
    ms_aeObjectType[7] = NiShaderAttributeDesc::OT_EFFECT_SHADOWSPOTLIGHT;
    ms_aeObjectType[8] = NiShaderAttributeDesc::OT_EFFECT_ENVIRONMENTMAP;
    ms_ausObjectFlags[7] = 0x700;
    ms_ausObjectFlags[8] = 0x800;
    ms_bObjectTableInitialized = 1;
}

void NiShaderConstantMapEntry::_SDMInit() {
    NiShaderConstantMapEntry::InitializeAttribTable();
    NiShaderConstantMapEntry::InitializeObjectTable();
}
