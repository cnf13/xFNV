#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DUtility.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderFactory.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreatorAsm.h"

// Passes fixed buffer sizes: 3 for the drive, 0x104 for the directory and the name, 0x100 for the extension.
bool NiD3DUtility::GetSplitPath(const char* pcFilename, char* pcDrive, char* pcDir, char* pcFname, char* pcExt) {
	_splitpath_s(pcFilename, pcDrive, 3, pcDir, 0x104, pcFname, 0x104, pcExt, 0x100);
	return true;
}

// The extension buffer is passed as 0x100 characters; the directory is the only one sized 0x104.
bool NiD3DUtility::GetFileExtension(const char* pcFilename, char* pcExt, size_t uiMaxLen) {
	char acDrive[4];
	char acExt[0x100];
	char acFName[0x100];
	char acDir[0x104];
	_splitpath_s(pcFilename, acDrive, 3, acDir, 0x104, acFName, 0x100, acExt, 0x100);
	size_t uiExtLen = 0;
	while (acExt[uiExtLen] != '\0') {
		++uiExtLen;
	}
	strncpy_s(pcExt, uiMaxLen, acExt, uiExtLen);
	return true;
}

// Equivalent to NiBinaryStream::Write: writes one one-byte component.
uint32_t WriteBytes(NiBinaryStream* pkStream, const void* pvBuffer, uint32_t uiBytes) {
	uint32_t uiSize = 1;
	return pkStream->BinaryWrite(pvBuffer, uiBytes, &uiSize, 1);
}

// Assembles shader code for the Asm creator; D3DX fills the two buffer pointers, and the caller's out-parameters take
// the copied code and its size. The constant-data out-parameters are only cleared.
bool NiD3DShaderProgramCreatorAsm::LoadShaderCodeFromBuffer(const void* pvBuffer, uint32_t uiBufferSize, void*& pvCode,
	uint32_t& uiCodeSize, void*& pvConstantData, uint32_t& uiConstantSize, int32_t bRecoverable) {
	pvCode = nullptr;
	uiCodeSize = 0;
	pvConstantData = nullptr;
	uiConstantSize = 0;
	if (!pvBuffer || !uiBufferSize) {
		NiShaderFactory::ReportError(NiShaderFactory::NISHADERERR_UNKNOWN, bRecoverable, "Invalid shader buffer\n");
		return false;
	}
	uint32_t uiFlags = (m_uiShaderCreationFlags | NiD3DShaderProgramCreator::ms_uiUniversalShaderCreationFlags) & 3;
	ID3DXBuffer* pkCode;
	ID3DXBuffer* pkErrors;
	if (D3DXAssembleShader_Thunk(static_cast<const char*>(pvBuffer), uiBufferSize, nullptr, nullptr, uiFlags, &pkCode,
			&pkErrors) >= 0) {
		uiCodeSize = pkCode->GetBufferSize();
		pvCode = _NiMalloc(uiCodeSize);
		rt::memcpy(pvCode, pkCode->GetBufferPointer(), uiCodeSize);
		pkCode->Release();
		if (pkErrors) {
			pkErrors->Release();
		}
		return true;
	}
	char* pcErr = nullptr;
	if (pkErrors) {
		void* pvBuff = pkErrors->GetBufferPointer();
		if (pvBuff) {
			uint32_t uiLen = pkErrors->GetBufferSize();
			pcErr = static_cast<char*>(_NiMalloc(uiLen));
			NiStrcpy(pcErr, uiLen, static_cast<const char*>(pvBuff));
			NiShaderFactory::ReportError(NiShaderFactory::NISHADERERR_UNKNOWN, bRecoverable,
				"Failed to assemble shader from memory\nError: %s\n", pcErr);
		}
		pkErrors->Release();
	} else {
		NiShaderFactory::ReportError(NiShaderFactory::NISHADERERR_UNKNOWN, bRecoverable,
			"Failed to assemble shader from memory\nError: NONE REPORTED\n");
	}
	_NiFree(pcErr);
	if (pkCode) {
		pkCode->Release();
	}
	return false;
}
