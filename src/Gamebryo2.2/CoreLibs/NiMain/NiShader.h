#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"

class bhkBlendCollisionObject;
class bhkLimitedHingeConstraint;
class bhkNiCollisionObject;
class bhkRigidBody;
class BSFadeNode;
class BSMultiBoundNode;
class BSResizableTriShape;
class BSSegmentedTriShape;
class NiDynamicEffectState;
class NiGeometry;
class NiGeometryBufferData;
class NiGPUProgram;
class NiLines;
class NiNode;
class NiParticles;
class NiPropertyState;
class NiShaderConstantMap;
class NiSkinInstance;
class NiTriBasedGeom;
class NiTriShape;
class NiTriStrips;

// Base of the renderer's shaders: the per-geometry pipeline hooks the renderer
// calls for each draw. Not an NiObject, but it repeats NiObject's run-time type
// virtuals.
class NiShader : public NiRefObject {
public:
    class NiShaderInstanceDescriptor;

    NiShader();
    ~NiShader() override;

    virtual const NiRTTI* GetRTTI() const {
        return &ms_RTTI;
    }
    // type queries: the object as the named class, or null
    virtual NiNode* IsNode() {
        return nullptr;
    }
    virtual BSFadeNode* IsFadeNode() {
        return nullptr;
    }
    virtual BSMultiBoundNode* IsMultiBoundNode() {
        return nullptr;
    }
    virtual NiGeometry* IsGeometry() {
        return nullptr;
    }
    virtual NiTriBasedGeom* IsTriBasedGeom() {
        return nullptr;
    }
    virtual NiTriStrips* IsTriStrips() {
        return nullptr;
    }
    virtual NiTriShape* IsTriShape() {
        return nullptr;
    }
    virtual BSSegmentedTriShape* IsSegmentedTriShape() {
        return nullptr;
    }
    virtual BSResizableTriShape* IsResizableTriShape() {
        return nullptr;
    }
    virtual NiParticles* IsParticlesGeom() {
        return nullptr;
    }
    virtual NiLines* IsLinesGeom() {
        return nullptr;
    }
    virtual bhkNiCollisionObject* IsBhkNiCollisionObject() {
        return nullptr;
    }

    virtual bhkBlendCollisionObject* IsBhkBlendCollisionObject() {
        return nullptr;
    }

    virtual bhkRigidBody* IsBhkRigidBody() const {
        return nullptr;
    }

    virtual bhkLimitedHingeConstraint* IsBhkLimitedHingeConstraint() {
        return nullptr;
    }

    virtual bool IsInitialized() {
        return m_bInitialized;
    }

    virtual bool Initialize() {
        m_bInitialized = true;
        return true;
    }

    virtual bool SetupGeometry(NiGeometry*) {
        return true;
    }

    virtual uint32_t PreProcessPipeline(
        NiGeometry*,
        const NiSkinInstance*,
        NiGeometryData::RendererData*,
        const NiPropertyState*,
        const NiDynamicEffectState*,
        const NiTransform&,
        const NiBound&
    ) {
        return 0;
    }

    virtual uint32_t UpdatePipeline(
        NiGeometry*,
        const NiSkinInstance*,
        NiGeometryData::RendererData*,
        const NiPropertyState*,
        const NiDynamicEffectState*,
        const NiTransform&,
        const NiBound&
    ) {
        return 0;
    }

    virtual uint32_t SetupRenderingPass(
        NiGeometry*,
        const NiSkinInstance*,
        NiGeometryData::RendererData*,
        const NiPropertyState*,
        const NiDynamicEffectState*,
        const NiTransform&,
        const NiBound&
    ) {
        return 0;
    }

    virtual uint32_t SetupTransformations(
        NiGeometry*,
        const NiSkinInstance*,
        const NiSkinPartition::Partition*,
        NiGeometryData::RendererData*,
        const NiPropertyState*,
        const NiDynamicEffectState*,
        const NiTransform&,
        const NiBound&
    ) {
        return 0;
    }

    virtual uint32_t SetupBoneMatrices(
        NiGeometry*,
        const NiSkinInstance*,
        const NiSkinPartition::Partition*,
        NiGeometryData::RendererData*,
        const NiPropertyState*,
        const NiDynamicEffectState*,
        const NiTransform&,
        const NiBound&
    ) {
        return 0;
    }

    virtual uint32_t SetupShaderPrograms(
        NiGeometry*,
        const NiSkinInstance*,
        const NiSkinPartition::Partition*,
        NiGeometryData::RendererData*,
        const NiPropertyState*,
        const NiDynamicEffectState*,
        const NiTransform&,
        const NiBound&
    ) {
        return 0;
    }

    virtual NiGeometryData::RendererData* PrepareGeometryForRendering(
        NiGeometry*,
        const NiSkinPartition::Partition*,
        NiGeometryData::RendererData*,
        const NiPropertyState*
    ) {
        return nullptr;
    }

    virtual uint32_t PostRender(
        NiGeometry*,
        const NiSkinInstance*,
        const NiSkinPartition::Partition*,
        NiGeometryData::RendererData*,
        const NiPropertyState*,
        const NiDynamicEffectState*,
        const NiTransform&,
        const NiBound&
    ) {
        return 0;
    }

    virtual uint32_t PostProcessPipeline(
        NiGeometry*,
        const NiSkinInstance*,
        NiGeometryData::RendererData*,
        const NiPropertyState*,
        const NiDynamicEffectState*,
        const NiTransform&,
        const NiBound&
    ) {
        return 0;
    }

    virtual void SetupGeometryTextures(const NiPropertyState*) {}
    virtual void SetupGeometryConstants(const NiPropertyState*) {}
    virtual void SetupGeometryAlphaBlending(const NiPropertyState*) {}
    virtual void SetupGeometryAlphaTesting(const NiPropertyState*) {}
    virtual void SetupGeometryRenderStates(const NiPropertyState*, bool) {}
    virtual void PostGeometry(const NiPropertyState*) {}
    virtual uint32_t FirstPass() {
        return 0;
    }
    virtual uint32_t NextPass() {
        return 0;
    }
    virtual bool IsGenericallyConfigurable() {
        return false;
    }

    virtual NiShaderDeclaration* CreateShaderDeclaration(uint32_t, uint32_t) {
        return nullptr;
    }

    virtual bool AppendRenderPass(uint32_t&) {
        return false;
    }

    virtual bool
    SetAlphaOverride(uint32_t, bool, bool, NiAlphaProperty::AlphaFunction, bool, NiAlphaProperty::AlphaFunction) {
        return false;
    }

    virtual bool SetUsesNiRenderState(bool) {
        return false;
    }

    virtual bool SetGPUProgram(uint32_t, NiGPUProgram*, uint32_t&) {
        return false;
    }

    virtual NiShaderConstantMap* CreateShaderConstantMap(uint32_t, uint32_t) {
        return nullptr;
    }

    virtual bool AppendTextureSampler(uint32_t, uint32_t&, const NiFixedString&, const NiFixedString&, uint32_t) {
        return false;
    }

    virtual NiShader::NiShaderInstanceDescriptor* GetShaderInstanceDesc() {
        return nullptr;
    }

    uint32_t GetImplementation() const;

    void SetName(const NiFixedString& kName);

    NiFixedString m_kName;
    uint32_t m_uiImplementation;
    bool m_bInitialized;

    static NiRTTI ms_RTTI;
    // Gamebryo's default implementation id, the one the second FindShader call asks for.
    static const uint32_t DEFAULT_IMPLEMENTATION = 0xFFFFFFFF;
};

// What identifies a shader instance; the base of the per-material descriptors. Its vtable puts the rtti block of
// NiObject first and the destructor last.
class NiShader::NiShaderInstanceDescriptor : public NiMemObject {
public:
    virtual const NiRTTI* GetRTTI() const;
    virtual NiNode* IsNode();
    virtual BSFadeNode* IsFadeNode();
    virtual BSMultiBoundNode* IsMultiBoundNode();
    virtual NiGeometry* IsGeometry();
    virtual NiTriBasedGeom* IsTriBasedGeom();
    virtual NiTriStrips* IsTriStrips();
    virtual NiTriShape* IsTriShape();
    virtual BSSegmentedTriShape* IsSegmentedTriShape();
    virtual BSResizableTriShape* IsResizableTriShape();
    virtual NiParticles* IsParticlesGeom();
    virtual NiLines* IsLinesGeom();
    virtual bhkNiCollisionObject* IsBhkNiCollisionObject();
    virtual bhkBlendCollisionObject* IsBhkBlendCollisionObject();
    virtual bhkRigidBody* IsBhkRigidBody() const;
    virtual bhkLimitedHingeConstraint* IsBhkLimitedHingeConstraint();
    virtual ~NiShaderInstanceDescriptor();

    static void operator delete(void* apMemory);

    static NiRTTI ms_RTTI;
};
