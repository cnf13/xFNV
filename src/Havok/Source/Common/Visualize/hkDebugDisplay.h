#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

#include <stdint.h>
#include <xmmintrin.h>

class hkAabb;
class hkCriticalSection;
class hkDebugDisplayHandler;
class hkDisplayGeometry;
class hkQsTransform;
class hkTransform;

// The Havok debug drawing singleton that forwards lines, rays and shapes to the debug display handlers.
class hkDebugDisplay : public hkReferencedObject, public hkSingleton<hkDebugDisplay> {
public:
	hkDebugDisplay();
	~hkDebugDisplay() override;

	// Hands geometries, placed at transform, to every handler under the array lock.
	void addGeometry(const hkArray<hkDisplayGeometry*>& geometries, const hkTransform& transform, hkUlong id,
		int32_t tag, hkUlong shapeIdHint);
	// Removes debugDisplay from the handler list under the array lock; the last handler fills its slot.
	void removeDebugDisplayHandler(hkDebugDisplayHandler* debugDisplay);
	// The line from start to end, sent to every handler.
	void displayLine(const hkVector4& start, const hkVector4& end, int32_t color, int32_t id, int32_t tag);
	// The arrow from start along dir: its shaft, and four head lines from the tip back to 85% of the way, set off
	// sideways by 15% of its length.
	void displayArrow(const hkVector4& from, const hkVector4& dir, int32_t color, int32_t id, int32_t tag);
	// The line from start to end, transformed by worldFromModel.
	void displayLineModelSpace(const hkQsTransform& worldFromModel, const hkVector4& start, const hkVector4& end,
		int32_t color, int32_t id, int32_t tag);
	// The line from start to end, transformed by worldFromModel through the hkQsTransform overload.
	void displayLineModelSpace(const hkTransform& worldFromModel, const hkVector4& start, const hkVector4& end,
		int32_t color, int32_t id, int32_t tag);
	// The ray from start along direction, transformed by worldFromModel.
	void displayRayModelSpace(const hkQsTransform& worldFromModel, const hkVector4& start, const hkVector4& direction,
		int32_t color, int32_t id, int32_t tag);
	// The ray from start along direction, transformed by worldFromModel through the hkQsTransform overload.
	void displayRayModelSpace(const hkTransform& worldFromModel, const hkVector4& start, const hkVector4& direction,
		int32_t color, int32_t id, int32_t tag);
	// Adds debugDisplay to the handler list under the array lock.
	void addDebugDisplayHandler(hkDebugDisplayHandler* debugDisplay);
	// Draws the bones of a model space pose, transformed by worldFromModel.
	void displayModelSpacePose(int32_t numTransforms, const int16_t* parentIndices, const hkQsTransform* modelSpacePose,
		const hkQsTransform& worldFromModel, int32_t id, int32_t tag, int32_t color, float boneAxisSize,
		int32_t showRootTranslation);
	// Draws the bones of a local space pose: the pose is taken to model space first.
	void displayLocalSpacePose(int32_t numTransforms, const int16_t* parentIndices, const hkQsTransform* localSpacePose,
		const hkQsTransform& worldFromModel, int32_t id, int32_t tag, int32_t color, float boneAxisSize,
		int32_t showRootTranslation);
	void setGeometryColor(int32_t color, hkUlong id, int32_t tag);
	// Havok SDK signature (hkVisualize.lib, tied by bytes).
	void displayTriangle(const hkVector4& a, const hkVector4& b, const hkVector4& c, int32_t color, int32_t id,
		int32_t tag);
	// Havok SDK (PDB name; thiscall, ret 0x10, 0xC99210): the triangle drawn in color lit by two fixed lights. this is
	// not read: the body draws through s_instance.
	void displayLitTriangle(const hkVector4& a, const hkVector4& b, const hkVector4& c, int32_t color);
	// Havok SDK signature (hkVisualize.lib, tied by target).
	void displayStarModelSpace(const hkQsTransform& worldFromModel, const hkVector4& position, float scale,
		int32_t color, int32_t id, int32_t tag);
	void addGeometryInstance(hkUlong instanceId, const hkTransform& transform, hkUlong id, int32_t tag,
		hkUlong shapeIdHint);
	void updateGeometry(const hkTransform& transform, hkUlong id, int32_t tag);
	void removeGeometry(hkUlong id, int32_t tag, hkUlong shapeIdHint);
	void updateCamera(const hkVector4& from, const hkVector4& to, const hkVector4& up, float nearPlane, float farPlane,
		float fov, const char* name);
	void updateBehavior(hkArray<int32_t, hkContainerHeapAllocator>& wordVarIdx,
		hkArray<int32_t, hkContainerHeapAllocator>& wordStack, hkArray<int32_t, hkContainerHeapAllocator>& quadVarIdx,
		hkArray<hkVector4, hkContainerHeapAllocator>& quadStack, hkArray<char*, hkContainerHeapAllocator>& activeNodes,
		hkArray<int32_t, hkContainerHeapAllocator>& activeStateIds,
		hkArray<int32_t, hkContainerHeapAllocator>& activeTransitions,
		hkArray<hkQsTransform, hkContainerHeapAllocator>& transforms);
	void displayPoint(const hkVector4& position, int32_t color, int32_t id, int32_t tag);
	// The ray from start along direction: a line to start + direction, sent to every handler without the array
	// lock.
	void displayRay(const hkVector4& start, const hkVector4& direction, int32_t color, int32_t id, int32_t tag);
	// The twelve edges of aabb, each through displayLine.
	void displayAabb(const hkAabb& aabb, int32_t color, int32_t id, int32_t tag);
	// Inferred type: a2: float from 0xc98c17 (parameter 2 of hkDebugDisplay::displayStarModelSpace)
	void displayStar(__m128* a1, float a2, int32_t a3, int32_t a4, int32_t a5);
	// Havok SDK signature (hkVisualize.lib, tied by target).
	void displayStarModelSpace(const hkTransform& worldFromModel, const hkVector4& position, float scale, int32_t color,
		int32_t id, int32_t tag);
	void displayPlane(__m128* a1, __m128* a2, uint32_t a3, int32_t a4, int32_t a5, int32_t a6);
	void displayText(const char* text, int32_t color, int32_t id, int32_t tag);
	void display3dText(const char* text, const hkVector4& pos, int32_t color, int32_t id, int32_t tag);
	void displayGeometry(const hkArrayBase<hkDisplayGeometry*>& geometries, const hkTransform& transform, int32_t color,
		int32_t id, int32_t tag);
	void displayGeometry(const hkArrayBase<hkDisplayGeometry*>& geometries, int32_t color, int32_t id, int32_t tag);
	void clear();
	void displayFrame(hkTransform* a1, uint32_t a2, int32_t a3, int32_t a4);

	hkArray<hkDebugDisplayHandler*> m_debugDisplayHandlers;
	mutable hkCriticalSection* m_arrayLock;

	// hkSingleton<hkDebugDisplay>::s_instance.
	static inline hkDebugDisplay* s_instance;
};

// Not named in the PDB or IDA; name chosen here. a3 is the hkQsTransform its only caller
// (0xC99870) builds; the exe passes it to hkVector4::setTransformedPos_0 (0xC7F7A0) as an hkQsTransform.
void displayFrameQs(hkDebugDisplay* a1, int32_t edx0, const hkQsTransform* a3, uint32_t a4, int32_t a5, int32_t a6);
// The singleton's create function: a new hkDebugDisplay (Havok SDK signature, hkVisualize.lib, tied by bytes,data).
hkReferencedObject* hkDebugDisplaycreate();

inline float DOUBLE_2_0 = 2.0f;
inline constexpr fnvd::Vtable hkDebugDisplay_vftable{};
