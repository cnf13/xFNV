#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <stdint.h>

class hkDisplayConvex;
class hkDisplayGeometry;
class hkpConvexRadiusBuilder;
class hkpShape;
class hkTransform;

double _getPlaneToSurfaceDistance(const hkpShape* shape);
void addAabbPlanes(const hkpShape* shape, float radius, hkArray<hkVector4>& planes);

// Builds the visual debugger's convex radius display geometries for a shape. Only some of its members are declared.
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
    void buildShapeDisplay(
        const hkpShape* shape,
        const hkTransform& transform,
        hkArray<hkDisplayGeometry*>& displayGeometries
    );

    // Forgets the current raw geometry.
    void resetCurrentRawGeometry();
    hkDisplayGeometry* getCurrentRawGeometry(hkArray<hkDisplayGeometry*, hkContainerHeapAllocator>& displayGeometries);

    // The convex display of the hull of points, each moved by transform, or null when the points have no hull.
    static hkDisplayConvex*
    createConvexDisplayFromPoints(const hkArray<hkVector4>& points, const hkTransform& transform);

    hkpConvexRadiusBuilderEnvironment m_environment;
    hkDisplayGeometry* m_currentGeometry;
};

// The registry of the functions that build the convex radius display of user shape types, one per shape type.
class hkpUserConvexRadiusBuilder : public hkReferencedObject, public hkSingleton<hkpUserConvexRadiusBuilder> {
public:
    // A user shape type's builder: adds the display geometries of shape, placed at transform.
    using ConvexRadiusBuilderFunction = void (*)(
        const hkpShape* shape,
        const hkTransform& transform,
        hkArray<hkDisplayGeometry*>& displayGeometries,
        hkpConvexRadiusBuilder* builder
    );

    struct UserShapeBuilder {
        ConvexRadiusBuilderFunction f;
        hkpShapeType type;
    };

    ~hkpUserConvexRadiusBuilder() override;

    // Adds f for type unless type has a builder already.
    void registerUserConvexRadiusDisplayBuilder(
        void (*f)(
            const hkpShape*,
            const hkTransform&,
            hkArray<hkDisplayGeometry*, hkContainerHeapAllocator>&,
            hkpConvexRadiusBuilder*
        ),
        hkpShapeType type
    );

    hkArray<UserShapeBuilder> m_userConvexRadiusBuilders;

    static inline hkpUserConvexRadiusBuilder* s_instance;
};
