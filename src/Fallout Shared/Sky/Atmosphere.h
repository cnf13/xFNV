#pragma once

#include "Fallout Shared/Sky/SkyObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class BSFogProperty;
class NiNode;
class NiTriShape;
class Sky;

// The sky's atmosphere: the atmosphere geometry and the fog property the weather drives.
class Atmosphere : public SkyObject {
public:
    Atmosphere();
    ~Atmosphere() override;

    void Update(Sky* apSky, float afSecondsPassed) override;

    virtual void Initialize(NiNode* apSkyRoot, BSFogProperty* apFog);

    using SkyObject::Initialize;

    BSFogProperty* GetFog();
    NiTriShape* GetAthmosphereGeo() {
        return spAtmosphere;
    }
    void SetUpdateFogDistance(bool a2);

    // The atmosphere geometry.
    NiPointer<NiTriShape> spAtmosphere;
    NiPointer<BSFogProperty> spFog;
    // The non-HDR sky quad and the node it hangs from; SkyShader::CreateSkyQuad builds the quad.
    NiPointer<NiNode> spSkyQuadNode;
    NiPointer<NiTriShape> spSkyQuad;
    bool bUpdateFogDistance;
    uint8_t padding19[3];
};
