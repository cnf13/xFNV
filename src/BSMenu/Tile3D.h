#pragma once

#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSMenu/Tile.h"

class NiControllerManager;

// A tile that shows an animated NIF model.
class Tile3D : public Tile {
public:
    Tile3D();
    ~Tile3D() override;

    void Init(Tile* apParent, const char* apName, Tile* apReplacedTile) override;
    NiNode* MakeNode() override;
    int32_t GetType() override;
    const char* GetTypeName() override;
    // Clears the animation name when the "animation" trait is parsed.
    Tile* PostParse(int32_t aiTrait, float afValue, const char* axString) override;
    void SetAlphaAndColor(NiNode* apNode, float afAlpha, const NiColorA& aColor) override;

    // True while the current animation sequence is animating.
    bool IsPlaying();
    // Takes the model's NiControllerManager as this tile's animation manager; false when the model has none.
    bool FindManagerNode(NiNode* apNode);
    // Loads the tile's NIF file into the tile's node, replacing its children, and sets the model up for the
    // interface (tile shaders, no culling while visible).
    void UpdateNIF();
    // Removes the material property of apNode and its descendants and compacts their child arrays.
    void RemoveProperties(NiNode* pTarget);
    // Starts the named animation sequence, finishing and stopping the current one; false when it cannot start.
    bool PlaySequence(const char* apSequenceName);
    // Starts the animation the tile's trait names and advances or poses the current sequence (afValue -1 plays on
    // with the frame timer, a percentage or a time otherwise); false while there is no model or nothing to update.
    bool UpdateSequence(float afValue, bool abIsPercentage);
    // Replaces every geometry's shade property under apNode with a new tile shader property using its base texture.
    void SetupShaderPropertiesRecurse(NiNode* apNode);

    static void operator delete(void* apMemory);

    // The model's NiControllerManager (PlaySequence reads its sequence map).
    NiControllerManager* pAniManager;
    // The animation sequence that is playing.
    NiControllerSequence* pCurrentSequence;
    // The name of the current animation.
    BSStringT<char> strCurrentName;
    // The path of the NIF file.
    BSStringT<char> strFilename;

    // The frame timer of the animated tiles.
    static BSTimer xTimer;
};
