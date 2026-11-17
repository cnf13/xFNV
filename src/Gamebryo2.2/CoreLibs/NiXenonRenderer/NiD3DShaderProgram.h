#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

class BSFadeNode;
class BSMultiBoundNode;
class BSResizableTriShape;
class BSSegmentedTriShape;
class NiD3DRenderState;
class NiD3DShaderProgramCreator;
class NiDX9Renderer;
class NiGeometry;
class NiLines;
class NiNode;
class NiParticles;
class NiShaderConstantMapEntry;
class NiTriBasedGeom;
class NiTriShape;
class NiTriStrips;
class bhkBlendCollisionObject;
class bhkLimitedHingeConstraint;
class bhkNiCollisionObject;
class bhkRigidBody;

// Base of the D3D vertex and pixel shader programs: name, compiled code and the creator that built it. Not an
// NiObject, but it repeats NiObject's run-time type virtuals.
class NiD3DShaderProgram : public NiGPUProgram {
public:
	NiD3DShaderProgram(NiDX9Renderer* pkRenderer, NiGPUProgram::ProgramType eType);
	~NiD3DShaderProgram() override;

	virtual const NiRTTI* GetRTTI() const { return &ms_RTTI; }
	// type queries: the object as the named class, or null
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
	virtual uint32_t GetVariableCount() { return 0; }
	virtual const char* GetVariableName(uint32_t) { return nullptr; }
	virtual const char* GetName() const { return m_pszName; }
	virtual void SetName(const char* pszName);
	virtual const char* GetShaderProgramName() const { return m_pszShaderProgramName; }
	virtual void SetShaderProgramName(const char* pszName);
	virtual uint32_t GetCodeSize() const { return m_uiCodeSize; }
	virtual void* GetCode() const { return m_pvCode; }

	virtual void SetCode(uint32_t uiSize, void* pvCode) {
		m_uiCodeSize = uiSize;
		m_pvCode = pvCode;
	}

	virtual NiD3DShaderProgramCreator* GetCreator() const { return m_pkCreator; }
	virtual void SetCreator(NiD3DShaderProgramCreator* pkCreator) { m_pkCreator = pkCreator; }
	virtual bool SetShaderConstant(NiShaderConstantMapEntry*, const void*, uint32_t) { return false; }

	virtual bool SetShaderConstantArray(NiShaderConstantMapEntry*, const void*, uint32_t, uint32_t, uint16_t*) {
		return false;
	}

	// Takes the renderer's device (one reference) and render state; null releases the device.
	void SetD3DRenderer(NiDX9Renderer* pkD3DRenderer);

	char* m_pszName;
	char* m_pszShaderProgramName;
	uint32_t m_uiCodeSize;
	void* m_pvCode;
	NiD3DShaderProgramCreator* m_pkCreator;
	IDirect3DDevice9* m_pkD3DDevice;
	NiDX9Renderer* m_pkD3DRenderer;
	NiD3DRenderState* m_pkD3DRenderState;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiD3DShaderProgram_vftable{};
