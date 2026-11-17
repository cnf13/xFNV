#pragma once

#include <cstdint>

// The kinds of shape the visual debugger displays.
enum hkDisplayGeometryType : int32_t {
	HK_DISPLAY_RECTANGLE = 0,
	HK_DISPLAY_SPHERE = 1,
	HK_DISPLAY_BOX = 2,
	HK_DISPLAY_AABB = 3,
	HK_DISPLAY_CONE = 4,
	HK_DISPLAY_SEMICIRCLE = 5,
	HK_DISPLAY_CONVEX = 6,
	HK_DISPLAY_PLANE = 7,
	HK_DISPLAY_CAPSULE = 8,
	HK_DISPLAY_CYLINDER = 9,
};
