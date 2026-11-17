#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingSpline.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <stdint.h>

class Actor;
class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class TESObjectREFR;

// One stop of a computed path: where it is, the direction the path leaves it in and the door it passes through (the
class PathingNode {
public:
    PathingNode();
    PathingNode(const PathingLocation* a1, const NiPoint3* a2);
    ~PathingNode();

    // Makes apActor take the node's action.
    bool ActivateNode(Actor* apActor);
    // Copies apSource into the node.
    PathingNode* Copy(PathingNode* apSource);
    bool IsTransitionNode();
    void SetTransitionFlag(bool abFlag);
    bool IsTeleportDoorNode() const;
    PathingLocation& GetLocation();
    // The node's point and tangent as a spline key.
    PathingSplineKey GetSplineKey();
    void SetTeleportDoor(TESObjectREFR* a1);
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    void SetTangent(const NiPoint3& aTangent);

    // Frees with ourFree.
    static void operator delete(void* apMemory);

    uint32_t iFlags;
    PathingLocation Location;
    NiPoint3 Tangent;
    TESObjectREFR* pDoorRef;
};
