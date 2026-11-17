#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"

// Tells the dispatcher which shape types stand in for which: the convex shapes collide as HK_SHAPE_CONVEX, the meshes
// and lists as HK_SHAPE_COLLECTION, and so on, so that an agent registered for the general type covers them.
void hkRegisterAlternateShapeTypes(hkpCollisionDispatcher* dis) {
    dis->registerAlternateShapeType(HK_SHAPE_SPHERE, HK_SHAPE_CONVEX);
    dis->registerAlternateShapeType(HK_SHAPE_TRIANGLE, HK_SHAPE_CONVEX);
    dis->registerAlternateShapeType(HK_SHAPE_BOX, HK_SHAPE_CONVEX);
    dis->registerAlternateShapeType(HK_SHAPE_CAPSULE, HK_SHAPE_CONVEX);
    dis->registerAlternateShapeType(HK_SHAPE_CYLINDER, HK_SHAPE_CONVEX);
    dis->registerAlternateShapeType(HK_SHAPE_CONVEX_VERTICES, HK_SHAPE_CONVEX);
    dis->registerAlternateShapeType(HK_SHAPE_CONVEX_TRANSLATE, HK_SHAPE_CONVEX);
    dis->registerAlternateShapeType(HK_SHAPE_CONVEX_TRANSFORM, HK_SHAPE_CONVEX);
    dis->registerAlternateShapeType(HK_SHAPE_CONVEX_PIECE, HK_SHAPE_CONVEX);

    dis->registerAlternateShapeType(HK_SHAPE_TRIANGLE_COLLECTION, HK_SHAPE_COLLECTION);
    dis->registerAlternateShapeType(HK_SHAPE_LIST, HK_SHAPE_COLLECTION);
    dis->registerAlternateShapeType(HK_SHAPE_EXTENDED_MESH, HK_SHAPE_COLLECTION);
    dis->registerAlternateShapeType(HK_SHAPE_COMPRESSED_MESH, HK_SHAPE_COLLECTION);
    dis->registerAlternateShapeType(HK_SHAPE_USER0, HK_SHAPE_COLLECTION);
    dis->registerAlternateShapeType(HK_SHAPE_USER1, HK_SHAPE_COLLECTION);

    dis->registerAlternateShapeType(HK_SHAPE_MOPP, HK_SHAPE_BV_TREE);
    dis->registerAlternateShapeType(HK_SHAPE_CONVEX, HK_SHAPE_SPHERE_REP);
    dis->registerAlternateShapeType(HK_SHAPE_PLANE, HK_SHAPE_HEIGHT_FIELD);
    dis->registerAlternateShapeType(HK_SHAPE_SAMPLED_HEIGHT_FIELD, HK_SHAPE_HEIGHT_FIELD);
}

const char* hkGetShapeTypeName(hkpShapeType type) {
    const char* result;
    switch (type) {
        case HK_SHAPE_ALL: {
            result = "HK_SHAPE_ALL";
            break;
        }
        case HK_SHAPE_INVALID: {
            result = "HK_SHAPE_INVALID";
            break;
        }
        case HK_SHAPE_SPHERE: {
            result = "HK_SHAPE_SPHERE";
            break;
        }
        case HK_SHAPE_CYLINDER: {
            result = "HK_SHAPE_CYLINDER";
            break;
        }
        case HK_SHAPE_TRIANGLE: {
            result = "HK_SHAPE_TRIANGLE";
            break;
        }
        case HK_SHAPE_BOX: {
            result = "HK_SHAPE_BOX";
            break;
        }
        case HK_SHAPE_CAPSULE: {
            result = "HK_SHAPE_CAPSULE";
            break;
        }
        case HK_SHAPE_CONVEX_VERTICES: {
            result = "HK_SHAPE_CONVEX_VERTICES";
            break;
        }
        case HK_SHAPE_COLLECTION: {
            result = "HK_SHAPE_COLLECTION";
            break;
        }
        case HK_SHAPE_BV_TREE: {
            result = "HK_SHAPE_BV_TREE";
            break;
        }
        case HK_SHAPE_LIST: {
            result = "HK_SHAPE_LIST";
            break;
        }
        case HK_SHAPE_MOPP: {
            result = "HK_SHAPE_MOPP";
            break;
        }
        case HK_SHAPE_CONVEX_TRANSLATE: {
            result = "HK_SHAPE_CONVEX_TRANSLATE";
            break;
        }
        case HK_SHAPE_CONVEX_TRANSFORM: {
            result = "HK_SHAPE_CONVEX_TRANSFORM";
            break;
        }
        case HK_SHAPE_SAMPLED_HEIGHT_FIELD: {
            result = "HK_SHAPE_SAMPLED_HEIGHT_FIELD";
            break;
        }
        case HK_SHAPE_EXTENDED_MESH: {
            result = "HK_SHAPE_EXTENDED_MESH";
            break;
        }
        case HK_SHAPE_TRANSFORM: {
            result = "HK_SHAPE_TRANSFORM";
            break;
        }
        case HK_SHAPE_COMPRESSED_MESH: {
            result = "HK_SHAPE_COMPRESSED_MESH";
            break;
        }
        case HK_SHAPE_MAX_ID_SPU: {
            result = "HK_SHAPE_MAX_ID_SPU";
            break;
        }
        case HK_SHAPE_CONVEX: {
            result = "HK_SHAPE_CONVEX";
            break;
        }
        case HK_SHAPE_MOPP_EMBEDDED: {
            result = "HK_SHAPE_MOPP_EMBEDDED";
            break;
        }
        case HK_SHAPE_CONVEX_PIECE: {
            result = "HK_SHAPE_CONVEX_PIECE";
            break;
        }
        case HK_SHAPE_MULTI_SPHERE: {
            result = "HK_SHAPE_MULTI_SPHERE";
            break;
        }
        case HK_SHAPE_CONVEX_LIST: {
            result = "HK_SHAPE_CONVEX_LIST";
            break;
        }
        case HK_SHAPE_TRIANGLE_COLLECTION: {
            result = "HK_SHAPE_TRIANGLE_COLLECTION";
            break;
        }
        case HK_SHAPE_MULTI_RAY: {
            result = "HK_SHAPE_MULTI_RAY";
            break;
        }
        case HK_SHAPE_HEIGHT_FIELD: {
            result = "HK_SHAPE_HEIGHT_FIELD";
            break;
        }
        case HK_SHAPE_SPHERE_REP: {
            result = "HK_SHAPE_SPHERE_REP";
            break;
        }
        case HK_SHAPE_BV: {
            result = "HK_SHAPE_BV";
            break;
        }
        case HK_SHAPE_PLANE: {
            result = "HK_SHAPE_PLANE";
            break;
        }
        case HK_SHAPE_PHANTOM_CALLBACK: {
            result = "HK_SHAPE_PHANTOM_CALLBACK";
            break;
        }
        case HK_SHAPE_USER0: {
            result = "HK_SHAPE_USER0";
            break;
        }
        case HK_SHAPE_USER1: {
            result = "HK_SHAPE_USER1";
            break;
        }
        case HK_SHAPE_USER2: {
            result = "HK_SHAPE_USER2";
            break;
        }
        default: {
            result = 0;
            break;
        }
    }
    return result;
}
