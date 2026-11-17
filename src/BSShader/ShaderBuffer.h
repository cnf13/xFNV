#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"

// One shader in a loaded shader buffer: its name, its size and its data, which runs on from pData inside the
// buffer (BSShader's compile functions take the address of pData).
class ShaderBufferEntry {
public:
    char pName[0x100];
    uint32_t uiSize;
    uint8_t pData[4];
};

// A file of precompiled shaders loaded into memory, with a hash from shader name to its entry in the buffer.
class ShaderBuffer {
public:
    enum eShaderBufferType : int32_t {
        SHADERBUFFERTYPE_NONE = 0x0,
        SHADERBUFFERTYPE_FIRST = 0x1,
        SHADERBUFFERTYPE_DEFAULT = 0x1,
        SHADERBUFFERTYPE_LAST = 0x1,
        SHADERBUFFERTYPE_COUNT = 0x2,
    };

    // Builds the empty 37-bucket hash (copying keys) and, for a buffer type above 0, the shader package's file
    // name "Data\Shaders\shaderpackageNNN.sdp".
    ShaderBuffer(eShaderBufferType aeBufferType);
    ~ShaderBuffer();

    // The entry of the shader apName; null when the buffer has none.
    ShaderBufferEntry* Get(const char* apName);
    // Loads the shader package apFileName into pShaderBuffer and hashes its entries; true when it was read whole.
    bool Run(const char* apFileName);

    static ShaderBuffer::eShaderBufferType GetCurrentBufferType();

    char* pFileName;
    char* pShaderBuffer;
    NiTStringPointerMap<ShaderBufferEntry*> ShaderHash;
};

// One shader added to a shader buffer being built by ShaderBufferCreator::Add: its name and a copy of its data. It has
// no vtable.
class ShaderBufferData {
public:
    // Copies the name and aiSize bytes of apData.
    ShaderBufferData(const char* apName, const char* apData, uint32_t aiSize);

    char pName[0x100];
    uint32_t uiSize;
    char* pData;
};

// Builds a shader buffer: the shaders added so far, each copied into a ShaderBufferData.
class ShaderBufferCreator {
public:
    void Add(const char* apName, char* apShader, uint32_t aiSize);

    NiTPrimitiveArray<ShaderBufferData*> DataArray;
};
