#pragma once

#include "BSMain/BSSceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

// The game's scene graph roots: the world, the menus and the 3D menus each have one.
class SceneGraph : public BSSceneGraph {
public:
	SceneGraph(const char* apSceneGraphName, bool abMenuSceneGraph, NiCamera* apCamera);

	const NiRTTI* GetRTTI() const override;
	float GetFarDistance() override;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(SceneGraph);

inline constexpr fnvd::Vtable SceneGraph_vftable{};
