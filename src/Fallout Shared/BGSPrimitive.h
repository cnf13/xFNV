#pragma once

#include "BSCore/BSCoreDefines.h"
#include "BSCore/NewOverloads.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class BGSPrimitiveBox;
class BGSPrimitivePlane;
class BGSPrimitiveSphere;
class BSFadeNode;
class BSMultiBoundShape;
class BSOcclusionPlane;
class BSPortal;
class NiTriStrips;

// A reference's editor primitive (box, sphere, plane or line).
class BGSPrimitive {
public:
    enum PRIMITIVE_TYPE : int32_t {
        PRIMITIVE_NONE = 0,
        PRIMITIVE_BOX = 1,
        PRIMITIVE_SPHERE = 2,
        PRIMITIVE_PLANE = 3,
        PRIMITIVE_LINE = 4,
    };

    BGSPrimitive();
    virtual ~BGSPrimitive();

    virtual void Build() = 0;
    virtual void SetColor(NiColorA& aColor) = 0;
    virtual void SetRadii(NiPoint3& aRadii) = 0;
    virtual bool IsInside(NiPoint3& aPoint) = 0;
    // The multibound shape of the primitive's bounds at the given rotation.
    virtual BSMultiBoundShape* MakeMultiBoundShape(NiPoint3& akAngle);

    // A copy of the primitive's color.
    NiColorA GetColor();
    // A copy of the primitive's radii.
    NiPoint3 GetRadii();
    // The primitive's radii.
    NiPoint3& QRadii();
    PRIMITIVE_TYPE GetType();
    // The primitive's node, spNode's object.
    BSFadeNode* GetNode();
    // Releases the primitive's geometry objects.
    void ClearObjects();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    // Creates a primitive of shape eType: a box of twice aRadii, a sphere of radius aRadii.x or a plane of twice
    // (aRadii.x, aRadii.z); null for any other shape.
    static BGSPrimitive* CreateAbstract(PRIMITIVE_TYPE eType, NiPoint3& aRadii, NiColorA& aColor);
    // A new primitive of apCopy's shape, radii and color.
    static BGSPrimitive* Copy(BGSPrimitive* apCopy);
    // Creates a plane primitive of size aSize.
    static BGSPrimitivePlane* CreatePlane(NiPoint2& aSize, NiColorA& aColor);
    // Creates a box primitive of size aSize.
    static BGSPrimitiveBox* CreateBox(NiPoint3& aSize, NiColorA& aColor);
    // Creates a sphere primitive of radius afRadius.
    static BGSPrimitiveSphere* CreateSphere(float afRadius, NiColorA& aColor);

    PRIMITIVE_TYPE eType;
    NiColorA Color;
    NiPoint3 Radii;
    NiPointer<NiTriStrips> spGeometry;
    NiPointer<NiRefObject> spFrame;
    NiPointer<BSFadeNode> spNode;
    int32_t iScaleHandlesEnabled;
};

// A plane primitive, which can also become an occlusion plane or a portal.
class BGSPrimitivePlane : public BGSPrimitive {
public:
    BGSPrimitivePlane();
    ~BGSPrimitivePlane() override;

    void Build() override;
    void SetColor(NiColorA& aColor) override;
    void SetRadii(NiPoint3& aRadii) override;
    bool IsInside(NiPoint3& aPoint) override;

    // The occlusion plane and the portal the plane's bounds make.
    virtual BSOcclusionPlane* MakeOcclusionPlane();
    virtual BSPortal* MakePortal();
};

class BGSPrimitiveBox : public BGSPrimitive {
public:
    BGSPrimitiveBox();
    ~BGSPrimitiveBox() override;

    void Build() override;
    void SetColor(NiColorA&) override;
    void SetRadii(NiPoint3&) override;
    bool IsInside(NiPoint3&) override;
    BSMultiBoundShape* MakeMultiBoundShape(NiPoint3&) override;

    // No PC function reads or writes pFacePlanes, but CreateBox allocates a box large enough to hold it, so the class
    // ends with it.
    BGSPrimitivePlane* pFacePlanes[6];
};

// The unit sphere's 112 vertices (x, y, z) BGSPrimitiveSphere builds and scales its geometry from.
using pSphereVertices_t = double[112][3];
inline pSphereVertices_t pSphereVertices{
    {0.0, -1.0, -0.0},
    {0.0, -0.901068, -0.433679},
    {0.0, -1.0, -0.0},
    {0.338941, -0.901068, -0.270549},
    {0.0, -1.0, 0.0},
    {0.422893, -0.901068, 0.096118},
    {0.0, -1.0, 0.0},
    {0.188699, -0.901068, 0.390474},
    {-0.0, -1.0, 0.0},
    {-0.187455, -0.901068, 0.391073},
    {-0.0, -1.0, 0.0},
    {-0.422585, -0.901068, 0.097464},
    {-0.0, -1.0, -0.0},
    {-0.339801, -0.901068, -0.269467},
    {-0.0, -1.0, -0.0},
    {-0.001382, -0.901068, -0.433677},
    {-0.001382, -0.901068, -0.433677},
    {-0.00249, -0.623846, -0.781544},
    {0.338077, -0.901068, -0.271627},
    {0.609261, -0.623846, -0.489508},
    {0.423197, -0.901068, 0.09477},
    {0.762659, -0.623846, 0.170789},
    {0.189942, -0.901068, 0.389871},
    {0.342302, -0.623846, 0.7026},
    {-0.186208, -0.901068, 0.391668},
    {-0.335572, -0.623846, 0.705839},
    {-0.422272, -0.901068, 0.09881},
    {-0.760992, -0.623846, 0.178069},
    {-0.340657, -0.901068, -0.268384},
    {-0.61391, -0.623846, -0.483664},
    {-0.002763, -0.901068, -0.43367},
    {-0.004979, -0.623846, -0.781532},
    {-0.004979, -0.623846, -0.781532},
    {-0.00621, -0.223186, -0.974756},
    {0.607698, -0.623846, -0.491446},
    {0.757944, -0.223186, -0.61295},
    {0.763199, -0.623846, 0.168358},
    {0.95189, -0.223186, 0.209983},
    {0.344538, -0.623846, 0.701506},
    {0.429721, -0.223186, 0.874944},
    {-0.333322, -0.623846, 0.706904},
    {-0.415731, -0.223186, 0.881678},
    {-0.76042, -0.623846, 0.180493},
    {-0.948425, -0.223186, 0.225117},
    {-0.615449, -0.623846, -0.481705},
    {-0.767611, -0.223186, -0.600801},
    {-0.007468, -0.623846, -0.781512},
    {-0.009315, -0.223186, -0.974731},
    {-0.009315, -0.223186, -0.974731},
    {-0.009318, 0.221634, -0.975085},
    {0.755988, -0.223186, -0.615362},
    {0.756263, 0.221634, -0.615585},
    {0.952554, -0.223186, 0.206949},
    {0.952901, 0.221634, 0.207025},
    {0.432506, -0.223186, 0.873571},
    {0.432663, 0.221634, 0.873888},
    {-0.41292, -0.223186, 0.882998},
    {-0.41307, 0.221634, 0.883319},
    {-0.947703, -0.223186, 0.228136},
    {-0.948048, 0.221634, 0.228219},
    {-0.76952, -0.223186, -0.598353},
    {-0.7698, 0.221634, -0.598571},
    {-0.012421, -0.223186, -0.974697},
    {-0.012425, 0.221634, -0.975051},
    {-0.012425, 0.221634, -0.975051},
    {-0.009971, 0.6226, -0.782477},
    {0.754297, 0.221634, -0.617992},
    {0.605322, 0.6226, -0.495938},
    {0.953555, 0.221634, 0.203989},
    {0.765226, 0.6226, 0.163701},
    {0.435444, 0.221634, 0.872506},
    {0.349443, 0.6226, 0.700185},
    {-0.410255, 0.221634, 0.88463},
    {-0.329229, 0.6226, 0.709914},
    {-0.947316, 0.221634, 0.231239},
    {-0.760219, 0.6226, 0.185569},
    {-0.771703, 0.221634, -0.596115},
    {-0.619291, 0.6226, -0.478381},
    {-0.01553, 0.221634, -0.975006},
    {-0.012463, 0.6226, -0.782441},
    {-0.012463, 0.6226, -0.782441},
    {-0.00693, 0.900376, -0.435058},
    {0.60374, 0.6226, -0.497863},
    {0.335696, 0.900376, -0.276825},
    {0.765744, 0.6226, 0.161263},
    {0.425774, 0.900376, 0.089667},
    {0.351673, 0.6226, 0.699068},
    {0.195539, 0.900376, 0.3887},
    {-0.326966, 0.6226, 0.710959},
    {-0.181802, 0.900376, 0.395312},
    {-0.759624, 0.6226, 0.18799},
    {-0.422371, 0.900376, 0.104528},
    {-0.620811, 0.6226, -0.476406},
    {-0.345188, 0.900376, -0.264895},
    {-0.014955, 0.6226, -0.782397},
    {-0.008315, 0.900376, -0.435034},
    {-0.008315, 0.900376, -0.435034},
    {-3e-05, 0.999999, -0.001592},
    {0.334812, 0.900376, -0.277893},
    {0.001226, 0.999999, -0.001017},
    {0.426058, 0.900376, 0.088309},
    {0.00156, 0.999999, 0.000323},
    {0.196777, 0.900376, 0.388075},
    {0.00072, 0.999999, 0.001421},
    {-0.180541, 0.900376, 0.395889},
    {-0.000661, 0.999999, 0.001449},
    {-0.422037, 0.900376, 0.105871},
    {-0.001545, 0.999999, 0.000388},
    {-0.34603, 0.900376, -0.263793},
    {-0.001267, 0.999999, -0.000966},
    {-0.009701, 0.900376, -0.435005},
    {-3.6e-05, 0.999999, -0.001592}
};
