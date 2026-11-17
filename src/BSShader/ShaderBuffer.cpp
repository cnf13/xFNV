#include "BSShader/ShaderBuffer.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/string.h"

ShaderBuffer::ShaderBuffer(eShaderBufferType aeBufferType)
	: pFileName(nullptr), pShaderBuffer(nullptr), ShaderHash(37, true) {
	if (aeBufferType > 0) {
		char acFileName[100];
		BSsprintf(acFileName, 100, "Data\\Shaders\\shaderpackage%03i.sdp", aeBufferType);
		size_t uiSize = std::strlen(acFileName) + 1;
		pFileName = static_cast<char*>(MemoryManager_Singleton.Allocate(uiSize));
		std::memcpy(pFileName, acFileName, uiSize);
	}
}

ShaderBuffer::~ShaderBuffer() {
	if (pFileName) {
		MemoryManager_Singleton.Deallocate(pFileName);
		pFileName = nullptr;
	}
	if (pShaderBuffer) {
		MemoryManager_Singleton.Deallocate(pShaderBuffer);
		pShaderBuffer = nullptr;
	}
}

ShaderBufferEntry* ShaderBuffer::Get(const char* apName) {
	ShaderBufferEntry* pEntry = nullptr;
	ShaderHash.GetAt(apName, pEntry);
	return pEntry;
}

ShaderBuffer::eShaderBufferType ShaderBuffer::GetCurrentBufferType() {
	int32_t result = 0;
	switch (BSShaderManager::eShaderLevel) {
	case 1: {
		result = 1;
		break;
	}
	case 2: {
		result = 2;
		break;
	}
	case 3: {
		result = 5;
		break;
	}
	case 4: {
		result = 8;
		break;
	}
	case 5: {
		if (BSShaderManager::b30Shaders) {
			if (BSShaderManager::bHDR) {
				result = ((uint8_t)BSShaderManager::bFPFiltering) + 12;
			} else {
				result = 4;
			}
		} else {
			if (BSShaderManager::bHDR) {
				result = ((uint8_t)BSShaderManager::bFPFiltering) + 10;
			} else {
				result = 3;
			}
		}
		break;
	}
	case 6: {
		if (BSShaderManager::b30Shaders) {
			if (BSShaderManager::bHDR) {
				result = ((uint8_t)BSShaderManager::bFPFiltering) + 16;
			} else {
				result = 7;
			}
		} else {
			if (BSShaderManager::bHDR) {
				result = ((uint8_t)BSShaderManager::bFPFiltering) + 14;
			} else {
				result = 6;
			}
		}
		break;
	}
	case 7: {
		if (BSShaderManager::bHDR) {
			result = ((uint8_t)BSShaderManager::bFPFiltering) + 18;
		} else {
			result = 9;
		}
		break;
	}
	default: {
		return static_cast<ShaderBuffer::eShaderBufferType>(result);
	}
	}
	return static_cast<ShaderBuffer::eShaderBufferType>(result);
}

ShaderBufferData::ShaderBufferData(const char* apName, const char* apData, uint32_t aiSize)
	: uiSize(aiSize), pData(nullptr) {
	if (aiSize && apData) {
		pData = static_cast<char*>(MemoryManager_Singleton.Allocate(aiSize));
		std::memcpy(pData, apData, uiSize);
	}
	std::memset(pName, 0, sizeof(pName));
	if (apName) {
		std::strncpy(pName, apName, sizeof(pName) - 1);
	}
}

// The exe's EH frame (it only guards the allocation during the constructor) is left out.
void ShaderBufferCreator::Add(const char* apName, char* apShader, uint32_t aiSize) {
	void* pMemory = MemoryManager_Singleton.Allocate(sizeof(ShaderBufferData));
	ShaderBufferData* pData = pMemory ? ::new (pMemory) ShaderBufferData(apName, apShader, aiSize) : nullptr;
	DataArray.Add(pData);
}
