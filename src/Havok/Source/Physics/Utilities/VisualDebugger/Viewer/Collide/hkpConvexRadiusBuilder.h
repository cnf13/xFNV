#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkDisplayConvex;
class hkDisplayGeometry;
class hkTransform;
class hkpConvexRadiusBuilder;
class hkpShape;

// Inferred type: r_eax: const hkpShape* from 0xde40d1 (parameter 0 of hkpConvexRadiusBuilder::buildShapeDisplay)
double _getPlaneToSurfaceDistance(const hkpShape* r_eax);
// Inferred type: shape: hkpShape* from 0xde421f (parameter 0 of hkpConvexRadiusBuilder::buildShapeDisplay), its members
// used at 0xde3206
void addAabbPlanes(const hkpShape* shape, float radius, hkArray<hkVector4>& planes);

// Builds the visual debugger's convex radius display geometries for a shape. Only the members the reconstructed code
// uses are declared.
class hkpConvexRadiusBuilder : public hkReferencedObject {
public:
	// The builder's settings: the smallest convex radius it shows.
	struct hkpConvexRadiusBuilderEnvironment {
		hkpConvexRadiusBuilderEnvironment();

		float m_minVisibleRadius;
	};

	hkpConvexRadiusBuilder(const hkpConvexRadiusBuilderEnvironment& env);

	// Empties displayGeometries and appends the display geometries of shape at the identity transform.
	void buildDisplayGeometries(const hkpShape* shape, hkArray<hkDisplayGeometry*>& displayGeometries);

	// Appends the convex radius display geometries of shape, placed at transform, and recurses into its children.
	void buildShapeDisplay(const hkpShape* shape, const hkTransform& transform,
		hkArray<hkDisplayGeometry*>& displayGeometries);

	// Forgets the current raw geometry.
	void resetCurrentRawGeometry();
	hkDisplayGeometry* getCurrentRawGeometry(hkArray<hkDisplayGeometry*, hkContainerHeapAllocator>& displayGeometries);

	// The convex display of the hull of points, each moved by transform, or null when the points have no hull.
	static hkDisplayConvex* createConvexDisplayFromPoints(const hkArray<hkVector4>& points,
		const hkTransform& transform);

	hkpConvexRadiusBuilderEnvironment m_environment;
	hkDisplayGeometry* m_currentGeometry;
};

// The registry of the functions that build the convex radius display of user shape types, one per shape type. The
// exe's vtable overrides only the destructor, whose body (0xDE10E0) is folded with hkpUserShapeDisplayBuilder's.
class hkpUserConvexRadiusBuilder : public hkReferencedObject, public hkSingleton<hkpUserConvexRadiusBuilder> {
public:
	// A user shape type's builder: adds the display geometries of shape, placed at transform.
	using ConvexRadiusBuilderFunction = void (*)(const hkpShape* shape, const hkTransform& transform,
		hkArray<hkDisplayGeometry*>& displayGeometries, hkpConvexRadiusBuilder* builder);

	// One registered builder and the shape type it is for.
	struct UserShapeBuilder {
		ConvexRadiusBuilderFunction f;
		hkpShapeType type;
	};

	~hkpUserConvexRadiusBuilder() override;

	// Adds f for type unless type has a builder already.
	void registerUserConvexRadiusDisplayBuilder(void (*f)(const hkpShape*, const hkTransform&,
													hkArray<hkDisplayGeometry*, hkContainerHeapAllocator>&,
													hkpConvexRadiusBuilder*),
		hkpShapeType type);

	hkArray<UserShapeBuilder> m_userConvexRadiusBuilders;

	// hkSingleton<hkpUserConvexRadiusBuilder>::s_instance: the exe's singleton init node (0x11B8660) for the create
	// function 0xDE43E0 points at it.
	static inline hkpUserConvexRadiusBuilder* s_instance;
};

inline constexpr fnvd::Vtable hkpConvexRadiusBuilder_vftable{};
inline constexpr fnvd::Vtable hkpUserConvexRadiusBuilder_vftable{};
