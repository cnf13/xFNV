#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiTriShape;
class NiTriShapeData;

class BSPolygon2D {
public:
    BSPolygon2D();
    BSPolygon2D(BSSimpleArray<NiPoint3, 1024>* apVertexSimpleArray);
    ~BSPolygon2D();

    // The signed area of the polygon.
    float Area();
    // Whether afPoint lies inside the polygon.
    bool IsPointInsidePoly(NiPoint2 afPoint);
    // Whether afPoint lies inside the triangle of vertices aiA, aiB and aiC.
    bool IsPointInsideTri(NiPoint2 afPoint, int32_t aiIndex1, int32_t aiIndex2, int32_t aiIndex3);
    // Records the indices of the concave vertices.
    void StoreConcaveVertices();
    // Reverses a counter-clockwise outline (non-negative area), then records its concave vertices.
    void Initialize();
    void Triangulate();
    bool IsAnEar(int32_t aiIndex, int32_t aiPrev, int32_t aiNext);
    NiTriShape* CreateTriShape(NiTriShapeData* data);
    NiTriShapeData* CreateTriData();
    // spRootNode's object, through NiPointer's operator->.
    NiNode* GetRootNode();
    // sp3D's object, through NiPointer's operator->.
    NiTriShape* Get3D();

protected:
    int32_t InternalAngle(NiPoint3 arCurr, NiPoint3 arPrev, NiPoint3 arNext);

public:
    void CenterAroundOrigin();
    void RemoveZ();
    float AreaofTriangle(const int32_t aiIndex1, const int32_t aiIndex2, const int32_t aiIndex3);

    // Frees through ourFree, as the game's deleting destructor does.
    static void operator delete(void* apMemory);

    BSSimpleArray<NiPoint3>* pVertexSimpleArray;
    // pConcaveVertexIndexList is an array, not a list, despite its name.
    BSSimpleArray<uint16_t>* pConcaveVertexIndexList;
    BSSimpleArray<uint16_t>* pIndexSimpleArray;
    NiPoint3* pVertexArray;
    uint16_t* pIndexArray;
    NiPoint3* pNormalArray;
    NiPoint2* pTexcoordArray;
    NiColorA* pColorArray;
    bool bClockwise;
    int32_t iVertexCount;
    int32_t iTriangleCount;
    NiPointer<NiNode> spRootNode;
    NiPointer<NiTriShape> sp3D;
    NiPoint3 vWorldTranslate;
};
