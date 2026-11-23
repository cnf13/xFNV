#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include <stdint.h>

class hkDisplayGeometry;
class hkpShape;
class hkTransform;

// Builds the display geometries of a shape for the visual debugger.
class hkpShapeDisplayBuilder : public hkReferencedObject {
public:
    // The tessellation the builder uses for spheres.
    struct hkpShapeDisplayBuilderEnvironment {
        hkpShapeDisplayBuilderEnvironment();

        int32_t m_spherePhiRes;
        int32_t m_sphereThetaRes;
    };

    hkpShapeDisplayBuilder(const hkpShapeDisplayBuilderEnvironment& env);

    ~hkpShapeDisplayBuilder() override;

    // Empties displayGeometries and appends the display geometries of shape at the identity transform.
    void buildDisplayGeometries(const hkpShape* shape, hkArray<hkDisplayGeometry*>& displayGeometries);

    // Appends the display geometries of shape, placed at transform.
    void buildShapeDisplay(
        const hkpShape* shape,
        const hkTransform& transform,
        hkArray<hkDisplayGeometry*>& displayGeometries
    );

    // Forgets the current raw geometry.
    void resetCurrentRawGeometry();
    // Havok SDK signature (hkpUtilities.lib, tied by bytes).
    hkDisplayGeometry* getCurrentRawGeometry(hkArray<hkDisplayGeometry*, hkContainerHeapAllocator>& displayGeometries);

    // Builds the display geometries of shape and hands them to the debug display at t under id, then deletes them.
    static void addObjectToDebugDisplay(const hkpShape* shape, hkTransform& t, hkUlong id);

    hkpShapeDisplayBuilderEnvironment m_environment;
    hkDisplayGeometry* m_currentGeometry;
};

// The registry of display builders for user shape types: the debug display calls the one registered for a shape's
// type. A singleton; it overrides only the destructor.
class hkpUserShapeDisplayBuilder : public hkReferencedObject, public hkSingleton<hkpUserShapeDisplayBuilder> {
public:
    // Appends the display geometries of shape, placed at transform.
    using ShapeBuilderFunction = void (*)(
        const hkpShape* shape,
        const hkTransform& transform,
        hkArray<hkDisplayGeometry*>& displayGeometries,
        hkpShapeDisplayBuilder* builder
    );

    // One registered builder and the shape type it builds.
    struct UserShapeBuilder {
        ShapeBuilderFunction f;
        hkpShapeType type;
    };

    ~hkpUserShapeDisplayBuilder() override;

    // Adds f for type unless type has a builder already.
    void registerUserShapeDisplayBuilder(
        void (*f)(
            const hkpShape*,
            const hkTransform&,
            hkArray<hkDisplayGeometry*, hkContainerHeapAllocator>&,
            hkpShapeDisplayBuilder*
        ),
        hkpShapeType type
    );

    hkArray<UserShapeBuilder> m_userShapeBuilders;

    // hkSingleton<hkpUserShapeDisplayBuilder>::s_instance.
    static inline hkpUserShapeDisplayBuilder* s_instance;
};
