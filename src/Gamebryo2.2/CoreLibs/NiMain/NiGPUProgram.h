#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

class bhkBlendCollisionObject;
class bhkLimitedHingeConstraint;
class bhkNiCollisionObject;
class bhkRigidBody;
class BSFadeNode;
class BSMultiBoundNode;
class BSResizableTriShape;
class BSSegmentedTriShape;
class NiGeometry;
class NiLines;
class NiNode;
class NiParticles;
class NiTriBasedGeom;
class NiTriShape;
class NiTriStrips;

// A compiled vertex or pixel shader program. Not an NiObject, but it repeats NiObject's run-time type virtuals, then
// adds the variable queries, which NiGPUProgramCache::PruneUnusedConstants calls.
class NiGPUProgram : public NiRefObject {
public:
    enum ProgramType {
        PROGRAM_VERTEX = 0,
        PROGRAM_VERTEX_CLIPPING_PLANES = 1,
        PROGRAM_PIXEL = 2,
        PROGRAM_MAX = 3,
    };

    // The program type; the derived programs' constructors pass it (NiD3DShaderProgram's).
    explicit NiGPUProgram(ProgramType eType) : m_eProgramType(eType) {}
    ~NiGPUProgram() override;

    virtual const NiRTTI* GetRTTI() const {
        return &ms_RTTI;
    }
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
    // Gamebryo's defaults, no variables; NiD3DVertexShader keeps them.
    virtual uint32_t GetVariableCount() {
        return 0;
    }
    virtual const char* GetVariableName(uint32_t) {
        return nullptr;
    }

    ProgramType GetProgramType() const {
        return m_eProgramType;
    }

    ProgramType m_eProgramType;

    static NiRTTI ms_RTTI;
};
