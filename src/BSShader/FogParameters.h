#pragma once

// The fog a shader is given: its far and near distances and its power.
class FogParameters {
public:
	float fDistFar;
	float fDistNear;
	float fPower;
	// The shaders store 0 here.
	float unk0C;
};
