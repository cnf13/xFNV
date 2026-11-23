#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderAttributeDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"

#include <stdint.h>

// One shader constant of a constant map: its key, kind flags, register range and the data it is set from.
class NiShaderConstantMapEntry : public NiRefObject {
public:
    // The entry kind, in bits 28-31 of m_uiFlags.
    enum Mapping {
        SCME_MAP_CONSTANT = 0x10000000,
        SCME_MAP_DEFINED = 0x20000000,
        SCME_MAP_ATTRIBUTE = 0x30000000,
        SCME_MAP_GLOBAL = 0x40000000,
        SCME_MAP_OPERATOR = 0x50000000,
        SCME_MAP_OBJECT = 0x60000000,
        SCME_MAP_MASK = static_cast<int32_t>(0xF0000000),
    };

    // The attribute type byte of m_uiFlags (an index into ms_aeAttribType).
    enum AttribTypeBits : uint32_t {
        SCME_ATTRIB_TYPE_POINT2 = 0x5,
        SCME_ATTRIB_TYPE_POINT3 = 0x6,
        SCME_ATTRIB_TYPE_POINT4 = 0x7,
        SCME_ATTRIB_TYPE_MATRIX3 = 0x8,
        SCME_ATTRIB_TYPE_MATRIX4 = 0x9,
        SCME_ATTRIB_TYPE_MASK = 0xFF,
    };

    // The object type byte of m_uiFlags (an index into ms_aeObjectType).
    enum : uint32_t {
        SCME_OBJECT_TYPE_MASK = 0xFF00,
        SCME_OBJECT_TYPE_SHIFT = 8,
    };

    // An operator entry's m_uiExtra: the operation, result flags and the indices of its two operand entries.
    enum OperatorType {
        SCME_OPERATOR_MULTIPLY = 0x10000000,
        SCME_OPERATOR_DIVIDE = 0x20000000,
        SCME_OPERATOR_ADD = 0x30000000,
        SCME_OPERATOR_SUBTRACT = 0x40000000,
        SCME_OPERATOR_MASK = static_cast<int32_t>(0xF0000000),
        SCME_OPERATOR_RESULT_INVERSE = 0x1000000,
        SCME_OPERATOR_RESULT_TRANSPOSE = 0x2000000,
        SCME_OPERATOR_RESULT_MASK = 0xF000000,
        SCME_OPERATOR_ENTRY1_MASK = 0xFFF,
        SCME_OPERATOR_ENTRY2_MASK = 0xFFF000,
        SCME_OPERATOR_ENTRY2_SHIFT = 12,
    };

    // The tables hold 15 attribute entries (InitializeAttribTable) and 9 object entries (InitializeObjectTable).
    using AttribFlagTable = uint8_t[NiShaderAttributeDesc::ATTRIB_TYPE_COUNT];
    using AttribTypeTable = NiShaderAttributeDesc::AttributeType[NiShaderAttributeDesc::ATTRIB_TYPE_COUNT];
    using ObjectFlagTable = uint16_t[9];
    using ObjectTypeTable = NiShaderAttributeDesc::ObjectType[9];

    NiShaderConstantMapEntry();

    ~NiShaderConstantMapEntry() override;

    void SetData(uint32_t uiDataSize, uint32_t uiDataStride, void* pvDataSource, bool bCopyData);
    // Sets m_kVariableName.
    void SetVariableName(const NiFixedString& kVariableName);

    static void InitializeAttribTable();
    // Fills the object flag and type tables.
    static void InitializeObjectTable();
    // Runs both table initialisers (the static data manager's initialisation).
    static void _SDMInit();

    bool m_bEnabled;
    NiFixedString m_kKey;
    uint32_t m_uiInternal;
    uint32_t m_uiFlags;
    uint32_t m_uiExtra;
    uint32_t m_uiShaderRegister;
    uint32_t m_uiRegisterCount;
    NiFixedString m_kVariableName;
    uint32_t m_uiDataSize;
    uint32_t m_uiDataStride;
    void* m_pvDataSource;
    bool m_bOwnData;
    bool m_bVariableHookupValid;
    bool m_bRegisterOnly;

    // Flags byte per object type, and object type per flags byte (bits 8-15 of m_uiFlags).
    static inline ObjectFlagTable ms_ausObjectFlags{};
    static inline ObjectTypeTable ms_aeObjectType{};
    // Flags byte per attribute type, and attribute type per flags byte (bits 0-7 of m_uiFlags).
    static inline AttribFlagTable ms_aucAttribFlags{};
    static inline AttribTypeTable ms_aeAttribType{};
    static inline uint8_t ms_bAttribTableInitialized;
    static inline uint8_t ms_bObjectTableInitialized;
};

NiSmartPointer(NiShaderConstantMapEntry);
