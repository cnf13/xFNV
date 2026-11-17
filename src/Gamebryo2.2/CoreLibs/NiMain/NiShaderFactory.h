#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalConstantEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderAttributeDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTFixedStringMap.h"
#include "Reconstruction.hpp"

class NiRenderer;
class NiShader;

// The position of a list walk (Gamebryo's NiTListIterator, a void*); the program directories are a list of strings.
typedef void* NiTListIterator;

// The registry of shader libraries, shaders and global shader constants.
// Only the members called so far; the rest is not reconstructed yet.
class NiShaderFactory : public NiRefObject {
public:
	// ReportError's error codes (Gamebryo's NiShaderError); the exe passes NISHADERERR_UNKNOWN in this file. The exe
	// keeps the code as a dword (ReportError pushes its argument slot to the callback).
	enum NiShaderError : uint32_t {
		NISHADERERR_UNKNOWN = 1,
		// Gamebryo's value; NiD3DShaderConstantMap::SetupDefinedConstantValue returns it without the time extra data.
		NISHADERERR_ENTRYNOTFOUND = 0x10,
	};

	// Gamebryo's callback types (NiShaderFactory.h).
	typedef bool (*NISHADERFACTORY_CLASSCREATIONCALLBACK)(const char*, NiRenderer*, int32_t, char*[], bool,
		NiShaderLibrary**);
	typedef uint32_t (*NISHADERFACTORY_RUNPARSERCALLBACK)(const char*, NiRenderer*, const char*, bool);
	// The recoverable flag: the exe passes the caller's whole dword on (ReportError, 0xAA0078); bool would be
	// zero-extended. PDB: bool.
	typedef uint32_t (*NISHADERFACTORY_ERRORCALLBACK)(const char*, NiShaderError, int32_t);

	NiShaderFactory();

	// Slots 2-15. The exe's entries are shared folded stubs: they return 0 or do nothing.
	virtual NiShader* RetrieveShader(const char*, uint32_t, bool) { return nullptr; }
	virtual NiShader* FindShader(const char*, uint32_t) { return nullptr; }
	// The vtable lists overloads in reverse declaration order: slot 4 takes the shader, slot 5 the name.
	virtual bool ReleaseShaderFromLibrary(const char*, uint32_t) { return false; }
	virtual bool ReleaseShaderFromLibrary(NiShader*) { return false; }
	virtual void InsertShader(NiShader*, uint32_t) {}
	virtual void RemoveShader(const char*, uint32_t) {}
	virtual void RemoveAllShaders() {}
	virtual bool IsDefaultImplementation(NiShader*) { return false; }
	virtual void InsertLibrary(NiShaderLibrary*) {}
	virtual NiShaderLibrary* FindLibrary(const char*) { return nullptr; }
	virtual void RemoveLibrary(const char*) {}
	virtual void RemoveAllLibraries() {}
	virtual NiShaderLibrary* GetFirstLibrary() { return nullptr; }
	virtual NiShaderLibrary* GetNextLibrary() { return nullptr; }
	// Slots 16-27 hold _purecall.
	virtual uint32_t GetMinorPixelShaderVersion(const uint32_t uiVersion) = 0;
	virtual uint32_t GetMajorPixelShaderVersion(const uint32_t uiVersion) = 0;
	virtual uint32_t CreatePixelShaderVersion(const uint32_t uiMajorVersion, const uint32_t uiMinorVersion) = 0;
	virtual uint32_t GetMinorVertexShaderVersion(const uint32_t uiVersion) = 0;
	virtual uint32_t GetMajorVertexShaderVersion(const uint32_t uiVersion) = 0;
	virtual uint32_t CreateVertexShaderVersion(const uint32_t uiMajorVersion, const uint32_t uiMinorVersion) = 0;
	virtual const char* GetRendererString() const = 0;
	virtual ~NiShaderFactory();

protected:
	virtual const char* GetFirstProgramDirectory(NiTListIterator& kIter) = 0;
	virtual const char* GetNextProgramDirectory(NiTListIterator& kIter) = 0;
	virtual void AddProgramDirectory(const char* pacDirectory) = 0;
	virtual void RemoveProgramDirectory(const char* pacDirectory) = 0;
	virtual void RemoveAllProgramDirectories() = 0;

public:
	// cdecl, ret 0, 0xAA0000. Releases every shader of the active factory, if any.
	static void ReleaseAllShaders();

	// cdecl, ret 0, 0xAA0400. Adds (or updates) a global constant; false without a factory.
	static bool RegisterGlobalShaderConstant(const NiFixedString& kKey, NiShaderAttributeDesc::AttributeType eType,
		uint32_t uiDataSize, const void* pvInitialData);

	// cdecl, ret 0, 0xAA04E0. Drops one reference to a global constant; false without a
	// factory.
	static bool ReleaseGlobalShaderConstant(const NiFixedString& kKey);

	// cdecl, variadic, 0xAA0020. Formats the message into a 2048-byte buffer and hands it to the factory's error
	// callback; 0 without a factory or callback. abRecoverable: the exe passes the caller's whole dword on (0xAA006A
	// loads it and pushes it to the callback); bool would be zero-extended. PDB: bool bRecoverable.
	static uint32_t ReportError(uint32_t eError, int32_t bRecoverable, const char* pacFmt, ...);
	static NiShaderFactory* GetInstance();

	// The PDB's NiTFixedStringMap<NiGlobalConstantEntry *>: the exe's vtable 0x10A2154 is that instance's by RTTI, its
	// DeleteItem (0xAA0330) releases no reference, and RegisterGlobalShaderConstant takes the map's reference by hand.
	NiTFixedStringMap<NiGlobalConstantEntry*> m_kGlobalConstantMap;
	NISHADERFACTORY_CLASSCREATIONCALLBACK m_pfnClassCreate;
	NISHADERFACTORY_RUNPARSERCALLBACK m_pfnRunParser;
	NISHADERFACTORY_ERRORCALLBACK m_pfnErrorCallback;

	static inline NiShaderFactory* ms_pkShaderFactory;
};
inline constexpr fnvd::Vtable NiShaderFactory_vftable{};
