#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiNode;
class Sky;

// Base of the sky objects (Atmosphere, Clouds, Stars, Sun): holds the sky scene root they attach to.
class SkyObject {
public:
    SkyObject();
    virtual ~SkyObject();

    virtual NiNode* GetRoot() {
        return spRoot;
    }

    // The engine's SkyObject::Initialize has SEH and creates the sky root node.
    virtual void Initialize(NiNode* apSkyRoot);

    virtual void Update(Sky*, float) {}

    // The deleting destructor frees with ourFree.
    static void operator delete(void* apMemory);

    // The root set up by Initialize. Public because the derived classes read it.
    NiPointer<NiNode> spRoot;
};
