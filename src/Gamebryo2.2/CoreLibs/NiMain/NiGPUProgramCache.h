#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTFixedStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPtrSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"

class bhkBlendCollisionObject;
class bhkLimitedHingeConstraint;
class bhkNiCollisionObject;
class bhkRigidBody;
class BSFadeNode;
class BSMultiBoundNode;
class BSResizableTriShape;
class BSSegmentedTriShape;
class NiBinaryStream;
class NiGeometry;
class NiGPUProgram;
class NiLines;
class NiNode;
class NiParticles;
class NiTriBasedGeom;
class NiTriShape;
class NiTriStrips;

// A material's cache of compiled GPU programs, optionally kept on disk. Only the virtual table is declared.
class NiGPUProgramCache : public NiRefObject {
public:
	class NiGPUProgramDesc;

	NiGPUProgramCache(uint32_t uiVersion, int32_t abAutoWriteCacheToDisk, int32_t abLocked)
		: m_kCachedPrograms(37), m_kValidTargets(10), m_uiVersion(uiVersion),
		  m_bAutoWriteCacheToDisk(abAutoWriteCacheToDisk != 0), m_bLoaded(false), m_bLocked(abLocked != 0) {}

	virtual const NiRTTI* GetRTTI() const;
	virtual NiNode* IsNode() { return nullptr; }
	virtual BSFadeNode* IsFadeNode() { return nullptr; }
	virtual BSMultiBoundNode* IsMultiBoundNode() { return nullptr; }
	virtual NiGeometry* IsGeometry() { return nullptr; }
	virtual NiTriBasedGeom* IsTriBasedGeom() { return nullptr; }
	virtual NiTriStrips* IsTriStrips() { return nullptr; }
	virtual NiTriShape* IsTriShape() { return nullptr; }
	virtual BSSegmentedTriShape* IsSegmentedTriShape() { return nullptr; }
	virtual BSResizableTriShape* IsResizableTriShape() { return nullptr; }
	virtual NiParticles* IsParticlesGeom() { return nullptr; }
	virtual NiLines* IsLinesGeom() { return nullptr; }
	virtual bhkNiCollisionObject* IsBhkNiCollisionObject() { return nullptr; }
	virtual bhkBlendCollisionObject* IsBhkBlendCollisionObject() { return nullptr; }
	virtual bhkRigidBody* IsBhkRigidBody() const { return nullptr; }
	virtual bhkLimitedHingeConstraint* IsBhkLimitedHingeConstraint() { return nullptr; }
	~NiGPUProgramCache() override;
	virtual NiGPUProgram* FindCachedProgram(const char* pcName,
		NiTObjectPtrSet<NiPointer<NiMaterialResource>>& kUniforms);
	virtual NiGPUProgram* GenerateProgram(const char* pcName, const char* pcProgramText,
		NiTObjectPtrSet<NiPointer<NiMaterialResource>>& kUniforms);
	virtual const NiTObjectPtrSet<NiFixedString>* GetValidCompileTargetsForPlatform();
	virtual const NiFixedString& GetShaderProfile();
	virtual const NiFixedString& GetWorkingDirectory();
	virtual bool SetWorkingDirectory(const NiFixedString& kFilename);
	virtual const char* GetPlatformSpecificCodeID() const = 0;
	virtual const char* GetPlatform() const = 0;
	virtual bool Load();
	virtual bool Save();
	virtual bool Clear();
	virtual bool IsLoaded() const;
	virtual bool IsLocked() const;

protected:
	// Returns whether the program's uniforms were pruned.
	virtual bool PruneUnusedConstants(NiGPUProgram* pkProgram,
		NiTObjectPtrSet<NiPointer<NiMaterialResource>>& kUniforms);
	virtual void ComputeFullFilename();
	virtual bool AppendEntry(NiGPUProgramCache::NiGPUProgramDesc* pkDesc, const NiFixedString& kName);
	virtual bool SaveEntry(NiBinaryStream& kStream, NiGPUProgramCache::NiGPUProgramDesc* pkDesc,
		const NiFixedString& kName);
	// KEEP (NiGPUProgramCache.cpp says why).
	virtual bool LoadEntry(NiBinaryStream& kStream, bool& bDeprecated);
	virtual bool SaveGPUProgram(NiBinaryStream& kStream, NiGPUProgram* pkProgram) = 0;
	virtual bool LoadGPUProgram(NiBinaryStream& kStream, const NiFixedString& kName, NiGPUProgram*& pkProgram,
		bool bSkip) = 0;

	// Whether the working directory exists, creating it when missing.
	bool ValidateDirectory();
	// Writes pcString's length with its terminator (0 for null), then its characters.
	bool SaveString(NiBinaryStream& kStream, const char* pcString);
	// Reads a length-prefixed string of uiBufferSize bytes into pcBuffer.
	bool LoadString(NiBinaryStream& kStream, char* pcBuffer, uint32_t stBufferSize);

public:
	// The cache's state, public only so the layout pins below can name them.
	NiTFixedStringMap<NiPointer<NiGPUProgramDesc>> m_kCachedPrograms;
	NiFixedString m_kFilename;
	NiFixedString m_kWorkingDir;
	NiFixedString m_kPathAndFilename;
	NiFixedString m_kShaderProfile;
	NiTObjectPtrSet<NiFixedString> m_kValidTargets;
	uint32_t m_uiVersion;
	bool m_bAutoWriteCacheToDisk;
	bool m_bLoaded;
	bool m_bLocked;

	static NiRTTI ms_RTTI;
	// Whether Load rewrites a cache file of an older version (Gamebryo's default true).
	static inline bool ms_bOverwriteDeprecatedCacheFiles = true;
};

// One cached entry of NiGPUProgramCache: a compiled program and the uniforms (material resources) it takes.
class NiGPUProgramCache::NiGPUProgramDesc : public NiRefObject {
public:
	// The constructor: the ref object's vtable and count, then the empty program and uniform
	// set.
	NiGPUProgramDesc();
	~NiGPUProgramDesc() override;

	NiPointer<NiGPUProgram> m_spProgram;
	NiTObjectPtrSet<NiPointer<NiMaterialResource>> m_kUniforms;
};
