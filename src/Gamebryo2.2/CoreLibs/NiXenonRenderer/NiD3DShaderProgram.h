#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

class bhkBlendCollisionObject;
class bhkLimitedHingeConstraint;
class bhkNiCollisionObject;
class bhkRigidBody;
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

// Base of the D3D vertex and pixel shader programs: name, compiled code and the creator that built it. Not an
// NiObject, but it repeats NiObject's run-time type virtuals.
class NiD3DShaderProgram : public NiGPUProgram {
public:
    NiD3DShaderProgram(NiDX9Renderer* pkRenderer, NiGPUProgram::ProgramType eType);
    ~NiD3DShaderProgram() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    // type queries: the object as the named class, or null
    NiNode* IsNode() override {
        return nullptr;
    }
    BSFadeNode* IsFadeNode() override {
        return nullptr;
    }
    BSMultiBoundNode* IsMultiBoundNode() override {
        return nullptr;
    }
    NiGeometry* IsGeometry() override {
        return nullptr;
    }
    NiTriBasedGeom* IsTriBasedGeom() override {
        return nullptr;
    }
    NiTriStrips* IsTriStrips() override {
        return nullptr;
    }
    NiTriShape* IsTriShape() override {
        return nullptr;
    }
    BSSegmentedTriShape* IsSegmentedTriShape() override {
        return nullptr;
    }
    BSResizableTriShape* IsResizableTriShape() override {
        return nullptr;
    }
    NiParticles* IsParticlesGeom() override {
        return nullptr;
    }
    NiLines* IsLinesGeom() override {
        return nullptr;
    }
    bhkNiCollisionObject* IsBhkNiCollisionObject() override {
        return nullptr;
    }
    bhkBlendCollisionObject* IsBhkBlendCollisionObject() override {
        return nullptr;
    }
    bhkRigidBody* IsBhkRigidBody() const override {
        return nullptr;
    }
    bhkLimitedHingeConstraint* IsBhkLimitedHingeConstraint() override {
        return nullptr;
    }
    uint32_t GetVariableCount() override {
        return 0;
    }
    const char* GetVariableName(uint32_t) override {
        return nullptr;
    }
    virtual const char* GetName() const {
        return m_pszName;
    }
    virtual void SetName(const char* pszName);
    virtual const char* GetShaderProgramName() const {
        return m_pszShaderProgramName;
    }
    virtual void SetShaderProgramName(const char* pszName);
    virtual uint32_t GetCodeSize() const {
        return m_uiCodeSize;
    }
    virtual void* GetCode() const {
        return m_pvCode;
    }

    virtual void SetCode(uint32_t uiSize, void* pvCode) {
        m_uiCodeSize = uiSize;
        m_pvCode = pvCode;
    }

    virtual NiD3DShaderProgramCreator* GetCreator() const {
        return m_pkCreator;
    }
    virtual void SetCreator(NiD3DShaderProgramCreator* pkCreator) {
        m_pkCreator = pkCreator;
    }
    virtual bool SetShaderConstant(NiShaderConstantMapEntry*, const void*, uint32_t) {
        return false;
    }

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
