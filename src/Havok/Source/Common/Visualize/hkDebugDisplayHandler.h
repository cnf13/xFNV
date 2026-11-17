#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

class hkDisplayGeometry;
class hkTransform;

class hkDebugDisplayHandler {
public:
	enum Command : int32_t {
		HK_STEP = 0x0,
		HK_ADD_GEOMETRY = 0x1,
		HK_UPDATE_GEOMETRY = 0x2,
		HK_SET_COLOR_GEOMETRY = 0x3,
		HK_REMOVE_GEOMETRY = 0x4,
		HK_DISPLAY_GEOMETRY_WITH_TRANSFORM = 0x5,
		HK_DISPLAY_GEOMETRY = 0x6,
		HK_DISPLAY_POINT = 0x7,
		HK_DISPLAY_LINE = 0x8,
		HK_DISPLAY_TEXT = 0x9,
		HK_SEND_STATISTICS_DUMP_DEPRICATED = 0xa,
		HK_HOLD_IMMEDIATE = 0xb,
		HK_ADD_CAMERA = 0xc,
		HK_UPDATE_CAMERA = 0xd,
		HK_REMOVE_CAMERA = 0xe,
		HK_SEND_MEMSTATS_DUMP = 0xf,
		HK_ADD_GEOMETRY_INSTANCE = 0x10,
		HK_DISPLAY_TEXT_3D = 0x11,
		HK_UPDATE_BEHAVIOR = 0x12,
		HK_LIST_BEHAVIORS = 0x13,
		HK_DISPLAY_ANNOTATION = 0x14,
	};

	virtual ~hkDebugDisplayHandler();

	virtual hkResult addGeometry(const hkArrayBase<hkDisplayGeometry*>&, const hkTransform&, hkUlong, int32_t,
		hkUlong) = 0;
	virtual hkResult addGeometryInstance(hkUlong, const hkTransform&, hkUlong, int32_t, hkUlong) = 0;
	virtual hkResult setGeometryColor(int32_t, hkUlong, int32_t) = 0;
	virtual hkResult updateGeometry(const hkTransform&, hkUlong, int32_t) = 0;
	virtual hkResult removeGeometry(hkUlong, int32_t, hkUlong) = 0;
	virtual hkResult updateCamera(const hkVector4&, const hkVector4&, const hkVector4&, float, float, float,
		const char*) = 0;
	virtual hkResult updateBehavior(const hkArrayBase<int32_t>&, const hkArrayBase<int32_t>&,
		const hkArrayBase<int32_t>&, const hkArrayBase<hkVector4>&, const hkArrayBase<char*>&,
		const hkArrayBase<int32_t>&, const hkArrayBase<int32_t>&, const hkArrayBase<hkQsTransform>&) = 0;
	virtual hkResult displayPoint(const hkVector4&, int32_t, int32_t, int32_t) = 0;
	virtual hkResult displayLine(const hkVector4&, const hkVector4&, int32_t, int32_t, int32_t) = 0;
	virtual hkResult displayTriangle(const hkVector4&, const hkVector4&, const hkVector4&, int32_t, int32_t,
		int32_t) = 0;
	virtual hkResult displayText(const char*, int32_t, int32_t, int32_t) = 0;
	virtual hkResult display3dText(const char*, const hkVector4&, int32_t, int32_t, int32_t) = 0;
	// The SDK's inline body.
	virtual hkResult displayAnnotation(const char*, int32_t, int32_t) { return HK_SUCCESS; }
	// The SDK's order: the MSVC ABI puts overloaded virtuals in reverse declaration order, so the plain form gets the
	// exe's slot 14 (0xDDC940, four stack dwords) and the transforming form slot 15 (0xDDC920, five).
	virtual hkResult displayGeometry(const hkArrayBase<hkDisplayGeometry*>&, const hkTransform&, int32_t, int32_t,
		int32_t) = 0;
	virtual hkResult displayGeometry(const hkArrayBase<hkDisplayGeometry*>&, int32_t, int32_t, int32_t) = 0;
	virtual hkResult sendMemStatsDump(const char*, int32_t) = 0;
	virtual hkResult holdImmediate() = 0;
	// The SDK's inline bodies.
	virtual void lockForUpdate() {}
	virtual void unlockForUpdate() {}

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable hkDebugDisplayHandler_vftable{};
