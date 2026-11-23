#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTFixedStringMap.h"

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
class NiLines;
class NiNode;
class NiParticles;
class NiPropertyState;
class NiShader;
class NiSkinInstance;
class NiTriBasedGeom;
class NiTriShape;
class NiTriStrips;

// A named material: builds the shaders that render geometry with it. Only some of its members are declared.
class NiMaterial : public NiRefObject {
public:
    // NI_MAX_PATH characters; empty when no default directory is set.
    using WorkingDirectoryArray = char[260];

    NiMaterial(const NiFixedString& kName);
    ~NiMaterial() override;

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
    virtual bool IsShaderCurrent(
        NiShader*,
        const NiGeometry*,
        const NiSkinInstance*,
        const NiPropertyState*,
        const NiDynamicEffectState*,
        uint32_t
    ) = 0;
    virtual NiShader* GetCurrentShader(
        const NiGeometry*,
        const NiSkinInstance*,
        const NiPropertyState*,
        const NiDynamicEffectState*,
        uint32_t
    ) = 0;
    virtual void UnloadShaders() = 0;
    virtual void SetWorkingDirectory(const char* pcWorkingDir) {}

    // The directory program caches are read from and written to, or null when none is set.
    static const char* GetDefaultWorkingDirectory();

    // The material named kName, or null when the name is empty or no material has it.
    static NiMaterial* GetMaterial(const NiFixedString& kName);
    // Creates ms_pkMaterials (37 buckets) and clears the default working directory.
    static void _SDMInit();
    // Unloads the shaders of every material in ms_pkMaterials.
    static void UnloadShadersForAllMaterials();
    // Deletes ms_pkMaterials.
    static void _SDMShutdown();

    NiFixedString m_kMaterialName;

    static NiRTTI ms_RTTI;
    static inline WorkingDirectoryArray ms_acDefaultWorkingDirectory{};
    // Every live material by name; _SDMInit creates it, _SDMShutdown empties and deletes it.
    static inline NiTFixedStringMap<NiMaterial*>* ms_pkMaterials;
};
