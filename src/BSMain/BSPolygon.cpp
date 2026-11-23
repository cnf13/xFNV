#include "BSMain/BSPolygon.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShapeData.h"

#include <cmath>

// No vertices; the output arrays and bClockwise are deliberately left unset.
BSPolygon2D::BSPolygon2D()
  : pVertexSimpleArray(nullptr), pConcaveVertexIndexList(nullptr), pIndexSimpleArray(nullptr), iVertexCount(0),
    iTriangleCount(0), spRootNode(nullptr), sp3D(nullptr), vWorldTranslate(NiPoint3::ZERO) {}

// The output arrays and bClockwise are deliberately left unset; Initialize fixes the winding.
BSPolygon2D::BSPolygon2D(BSSimpleArray<NiPoint3, 1024>* apVertexSimpleArray)
  : pVertexSimpleArray(apVertexSimpleArray), pConcaveVertexIndexList(nullptr), pIndexSimpleArray(nullptr),
    iVertexCount(apVertexSimpleArray->iSize), iTriangleCount(0), spRootNode(nullptr), sp3D(nullptr),
    vWorldTranslate(NiPoint3::ZERO) {
    Initialize();
}

// The vertex array is the polygon's own until it is triangulated.
BSPolygon2D::~BSPolygon2D() {
    if (!iTriangleCount) {
        delete pVertexSimpleArray;
    }
}

float BSPolygon2D::Area() {
    int32_t v3;
    float v5;
    int32_t i;
    float v4 = 0.0f;
    for (i = 0; i < iVertexCount; ++i) {
        v3 = (i + 1) % iVertexCount;
        v5 = (pVertexSimpleArray->pBuffer[i].x * pVertexSimpleArray->pBuffer[v3].y) + v4;
        v4 = v5 - (pVertexSimpleArray->pBuffer[i].y * pVertexSimpleArray->pBuffer[v3].x);
    }
    return static_cast<float>(v4 * 0.5);
}

bool BSPolygon2D::IsPointInsidePoly(NiPoint2 afPoint) {
    int32_t i;
    for (i = 0; i < iTriangleCount; ++i) {
        if (IsPointInsideTri(
                afPoint,
                pIndexSimpleArray->pBuffer[3 * i],
                pIndexSimpleArray->pBuffer[(3 * i) + 1],
                pIndexSimpleArray->pBuffer[(3 * i) + 2]
            )) {
            return true;
        }
    }
    return false;
}

bool BSPolygon2D::IsPointInsideTri(NiPoint2 afPoint, int32_t aiIndex1, int32_t aiIndex2, int32_t aiIndex3) {
    const float afX = afPoint.x;
    const float afY = afPoint.y;
    float v16 = pVertexSimpleArray->pBuffer[aiIndex3].x - pVertexSimpleArray->pBuffer[aiIndex2].x;
    float v20 = pVertexSimpleArray->pBuffer[aiIndex3].y - pVertexSimpleArray->pBuffer[aiIndex2].y;
    float v12 = afX - pVertexSimpleArray->pBuffer[aiIndex2].x;
    float v13 = afY - pVertexSimpleArray->pBuffer[aiIndex2].y;
    float v10 = v16 * v13 - v20 * v12;
    bool v7 = false;
    if (v10 <= 0.0) {
        float v8 = pVertexSimpleArray->pBuffer[aiIndex1].y - pVertexSimpleArray->pBuffer[aiIndex3].y;
        float v9 = afY - pVertexSimpleArray->pBuffer[aiIndex3].y;
        float v14 = afX - pVertexSimpleArray->pBuffer[aiIndex3].x;
        float v15 = pVertexSimpleArray->pBuffer[aiIndex1].x - pVertexSimpleArray->pBuffer[aiIndex3].x;
        float v18 = v15 * v9 - v8 * v14;
        if (v18 <= 0.0) {
            float v11 = pVertexSimpleArray->pBuffer[aiIndex2].y - pVertexSimpleArray->pBuffer[aiIndex1].y;
            float v17 = afY - pVertexSimpleArray->pBuffer[aiIndex1].y;
            float v21 = pVertexSimpleArray->pBuffer[aiIndex2].x - pVertexSimpleArray->pBuffer[aiIndex1].x;
            float v22 = afX - pVertexSimpleArray->pBuffer[aiIndex1].x;
            float v19 = v21 * v17 - v11 * v22;
            if (v19 <= 0.0) {
                return true;
            }
        }
    }
    return v7;
}

void BSPolygon2D::StoreConcaveVertices() {
    if (!pConcaveVertexIndexList) {
        pConcaveVertexIndexList = new BSSimpleArray<uint16_t, 1024>;
        bool bfoundcolinear = false;
        for (int32_t b = 0; b < iVertexCount; ++b) {
            int32_t a;
            if (bfoundcolinear) {
                --b;
            }
            if (b) {
                a = b - 1;
            }
            else {
                a = iVertexCount - 1;
            }
            int32_t c = (b + 1) % iVertexCount;
            int32_t iangle = InternalAngle(
                pVertexSimpleArray->pBuffer[b],
                pVertexSimpleArray->pBuffer[a],
                pVertexSimpleArray->pBuffer[c]
            );
            bfoundcolinear = false;
            if (iangle == -1) {
                uint16_t aItem = b;
                pConcaveVertexIndexList->Add(aItem);
            }
            else {
                if (!iangle) {
                    pVertexSimpleArray->Remove(static_cast<uint32_t>(b), true);
                    --iVertexCount;
                    bfoundcolinear = true;
                }
            }
        }
    }
}

// The area is compared as a double; NaN keeps the order.
void BSPolygon2D::Initialize() {
    if (Area() >= 0.0) {
        pVertexSimpleArray->Reverse();
    }
    StoreConcaveVertices();
}

// Each index wrap copies the list's size into a signed local, so it is an int32_t here; the steps `++i %
// indexlist.iSize` divide unsigned.
void BSPolygon2D::Triangulate() {
    using indexlist_t = BSSimpleArray<uint32_t, 1024>;
    indexlist_t indexlist;
    for (uint32_t a = 0; a < pVertexSimpleArray->iSize; ++a) {
        indexlist.Add(a);
    }
    int32_t i = 2;
    NiPoint3 vert = pVertexSimpleArray->pBuffer[2];
    NiPoint3 first = *pVertexSimpleArray->pBuffer;
    while (!(vert == first) && indexlist.iSize > 3) {
        int32_t j = i - 2;
        int32_t k = i - 1;
        int32_t iCount = indexlist.iSize;
        if (iCount >= 0) {
            if (j >= 0) {
                j %= iCount;
            }
            else {
                j = j % iCount + iCount;
            }
        }
        iCount = indexlist.iSize;
        if (iCount >= 0) {
            if (k >= 0) {
                k %= iCount;
            }
            else {
                k = k % iCount + iCount;
            }
        }
        if (IsAnEar(indexlist.pBuffer[k], indexlist.pBuffer[j], indexlist.pBuffer[i])) {
            if (!pIndexSimpleArray) {
                pIndexSimpleArray = new BSSimpleArray<uint16_t, 1024>;
            }
            uint16_t aItem = indexlist.pBuffer[i];
            pIndexSimpleArray->Add(aItem);
            uint16_t v13 = indexlist.pBuffer[k];
            pIndexSimpleArray->Add(v13);
            uint16_t v12 = indexlist.pBuffer[j];
            pIndexSimpleArray->Add(v12);
            ++iTriangleCount;
            indexlist.Remove(--i, true);
            if (pConcaveVertexIndexList) {
                uint32_t iindex_3;
                int32_t j_2 = i - 1;
                int32_t k_2 = i + 1;
                iCount = indexlist.iSize;
                if (iCount >= 0) {
                    if (j_2 >= 0) {
                        j_2 %= iCount;
                    }
                    else {
                        j_2 = j_2 % iCount + iCount;
                    }
                }
                iCount = indexlist.iSize;
                if (iCount >= 0) {
                    if (k_2 >= 0) {
                        k_2 %= iCount;
                    }
                    else {
                        k_2 = k_2 % iCount + iCount;
                    }
                }
                // A search for the index of the first item equal to the vertex at i, or -1.
                const uint32_t& rFind = indexlist.pBuffer[i];
                BSSimpleArray<uint16_t>* pList = pConcaveVertexIndexList;
                iindex_3 = -1;
                for (uint32_t iindex = 0; iindex < pList->iSize; ++iindex) {
                    if (pList->pBuffer && pList->pBuffer[iindex] == rFind) {
                        iindex_3 = iindex;
                        break;
                    }
                }
                if (iindex_3 != -1) {
                    int32_t angle = InternalAngle(
                        pVertexSimpleArray->pBuffer[indexlist.pBuffer[i]],
                        pVertexSimpleArray->pBuffer[indexlist.pBuffer[j_2]],
                        pVertexSimpleArray->pBuffer[indexlist.pBuffer[k_2]]
                    );
                    if (angle == 1) {
                        pConcaveVertexIndexList->Remove(iindex_3, true);
                    }
                }
                j_2 = i - 1;
                k_2 = i - 2;
                iCount = indexlist.iSize;
                if (iCount >= 0) {
                    if (j_2 >= 0) {
                        j_2 %= iCount;
                    }
                    else {
                        j_2 = j_2 % iCount + iCount;
                    }
                }
                iCount = indexlist.iSize;
                if (iCount >= 0) {
                    if (k_2 >= 0) {
                        k_2 %= iCount;
                    }
                    else {
                        k_2 = k_2 % iCount + iCount;
                    }
                }
                // The same search, for the vertex at j_2.
                const uint32_t& rFind_2 = indexlist.pBuffer[j_2];
                BSSimpleArray<uint16_t>* pList_2 = pConcaveVertexIndexList;
                iindex_3 = -1;
                for (uint32_t iindex_2 = 0; iindex_2 < pList_2->iSize; ++iindex_2) {
                    if (pList_2->pBuffer && pList_2->pBuffer[iindex_2] == rFind_2) {
                        iindex_3 = iindex_2;
                        break;
                    }
                }
                if (iindex_3 != -1) {
                    int32_t angle_2 = InternalAngle(
                        pVertexSimpleArray->pBuffer[indexlist.pBuffer[j_2]],
                        pVertexSimpleArray->pBuffer[indexlist.pBuffer[k_2]],
                        pVertexSimpleArray->pBuffer[indexlist.pBuffer[i]]
                    );
                    if (angle_2 == 1) {
                        pConcaveVertexIndexList->Remove(iindex_3, true);
                    }
                }
            }
            int32_t z = i - 1;
            iCount = indexlist.iSize;
            if (iCount >= 0) {
                if (z >= 0) {
                    z %= iCount;
                }
                else {
                    z = z % iCount + iCount;
                }
            }
            if (!indexlist.pBuffer[z]) {
                vert = pVertexSimpleArray->pBuffer[indexlist.pBuffer[++i % indexlist.iSize]];
            }
        }
        else {
            vert = pVertexSimpleArray->pBuffer[indexlist.pBuffer[++i % indexlist.iSize]];
        }
    }
    if (pIndexSimpleArray && (indexlist.iSize == 3)) {
        uint16_t v11 = indexlist.pBuffer[2];
        pIndexSimpleArray->Add(v11);
        uint16_t v10 = indexlist.pBuffer[1];
        pIndexSimpleArray->Add(v10);
        uint16_t v9 = indexlist.pBuffer[0];
        pIndexSimpleArray->Add(v9);
        ++iTriangleCount;
    }

    if (iTriangleCount > 0) {
        NiNode* v1;
        NiTriShapeData* data = CreateTriData();
        sp3D = CreateTriShape(data);
        v1 = new NiNode(0);
        spRootNode = v1;
        spRootNode->AttachChild(sp3D.m_pObject, 1);
        if (sp3D.m_pObject) {
            pVertexArray = nullptr;
            pNormalArray = nullptr;
            pTexcoordArray = nullptr;
            pColorArray = nullptr;
            pIndexArray = nullptr;
        }
    }
}

bool BSPolygon2D::IsAnEar(int32_t aiIndex, int32_t aiPrev, int32_t aiNext) {
    if (!pConcaveVertexIndexList || !pConcaveVertexIndexList->iSize) {
        return true;
    }
    uint16_t aValue = aiIndex;
    if (pConcaveVertexIndexList->IsInArray(aValue)) {
        return false;
    }
    for (int32_t iVertex : *pConcaveVertexIndexList) {
        if (iVertex != aiPrev && iVertex != aiNext) {
            NiPoint2 afPoint;
            afPoint.x = pVertexSimpleArray->pBuffer[iVertex].x;
            afPoint.y = pVertexSimpleArray->pBuffer[iVertex].y;
            if (IsPointInsideTri(afPoint, aiPrev, aiIndex, aiNext)) {
                return false;
            }
        }
    }
    return true;
}

NiTriShape* BSPolygon2D::CreateTriShape(NiTriShapeData* data) {
    return new NiTriShape(data);
}

NiTriShapeData* BSPolygon2D::CreateTriData() {
    pVertexArray = new NiPoint3[static_cast<uint32_t>(this->iVertexCount)];
    pNormalArray = new NiPoint3[static_cast<uint32_t>(this->iVertexCount)];
    this->pTexcoordArray = new NiPoint2[static_cast<uint32_t>(this->iVertexCount)];
    pColorArray = new NiColorA[static_cast<uint32_t>(this->iVertexCount)];
    pIndexArray = static_cast<uint16_t*>(_NiMalloc(sizeof(uint16_t) * (3 * iTriangleCount)));
    rt::memcpy(pVertexArray, pVertexSimpleArray->pBuffer, sizeof(NiPoint3) * this->iVertexCount);
    rt::memcpy(pIndexArray, pIndexSimpleArray->pBuffer, 3 * sizeof(uint16_t) * iTriangleCount);
    for (int32_t n = 0; n < this->iVertexCount; ++n) {
        NiPoint3* v1 = &pNormalArray[n];
        v1->x = 0.0f;
        v1->y = 0.0f;
        v1->z = 1.0f;
    }
    float v33 = 100.0f;
    for (int32_t ii = 0; ii < this->iVertexCount; ++ii) {
        float v11 = pVertexArray[ii].x / v33;
        float v12 = pVertexArray[ii].y / v33;
        NiPoint2* pTexcoordArray = this->pTexcoordArray;
        pTexcoordArray[ii].x = v11;
        pTexcoordArray[ii].y = v12;
    }
    for (int32_t stSizeInBytes = 0; stSizeInBytes < this->iVertexCount; ++stSizeInBytes) {
        NiColorA* v3 = &pColorArray[stSizeInBytes];
        v3->r = 1.0f;
        v3->g = 1.0f;
        v3->b = 1.0f;
        v3->a = 1.0f;
    }
    return new NiTriShapeData(
        this->iVertexCount,
        pVertexArray,
        pNormalArray,
        pColorArray,
        this->pTexcoordArray,
        1,
        NiGeometryData::NBT_METHOD_NONE,
        iTriangleCount,
        pIndexArray
    );
}

int32_t BSPolygon2D::InternalAngle(NiPoint3 arCurr, NiPoint3 arPrev, NiPoint3 arNext) {
    float v5 = ((arCurr.x - arPrev.x) * (arNext.y - arCurr.y)) - ((arCurr.y - arPrev.y) * (arNext.x - arCurr.x));
    if (v5 < 0.0) {
        return 1;
    }
    if (!(v5 > 0.0)) {
        return 0;
    }
    return (-1);
}

void BSPolygon2D::CenterAroundOrigin() {
    NiPoint3 v4;
    NiPoint3 result;
    NiPoint3 v7 = NiPoint3::ZERO;
    for (int32_t i = 0; i < iVertexCount; ++i) {
        v7 += pVertexSimpleArray->pBuffer[i];
    }
    float fScalar = static_cast<float>(iVertexCount);
    result = v7 / fScalar;
    for (int32_t j = 0; j < iVertexCount; ++j) {
        NiPoint3* v3 = &(v4 = pVertexSimpleArray->pBuffer[j] - result);
        pVertexSimpleArray->pBuffer[j] = *v3;
    }
    vWorldTranslate = result;
}

void BSPolygon2D::RemoveZ() {
    for (int32_t i = 0; i < iVertexCount; ++i) {
        NiPoint3* v1 = &pVertexSimpleArray->pBuffer[i];
        float x = v1->x;
        float y = v1->y;
        NiPoint3* v3 = &pVertexSimpleArray->pBuffer[i];
        v3->x = x;
        v3->y = y;
        v3->z = 0.0;
    }
}

float BSPolygon2D::AreaofTriangle(const int32_t aiIndex1, const int32_t aiIndex2, const int32_t aiIndex3) {
    NiPoint3 result;
    NiPoint3 v10;
    result = pVertexSimpleArray->pBuffer[aiIndex2] - pVertexSimpleArray->pBuffer[aiIndex1];
    v10 = pVertexSimpleArray->pBuffer[aiIndex3] - pVertexSimpleArray->pBuffer[aiIndex1];
    float v8 = (v10.y * result.x - v10.x * result.y) / 2.0;
    return std::fabs(static_cast<double>(v8));
}

void BSPolygon2D::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void BSWrap(float& aVal, float aMin, float aMax) {
    if (!(aMax < static_cast<double>(aMin))) {
        if (!(aMax < static_cast<double>(aVal))) {
            if (aMin > static_cast<double>(aVal)) {
                float v4 = aMax - aMin;
                float v3 = aVal - aMin;
                aVal = std::fmodf(v3, v4) + aMax;
            }
        }
        else {
            float v6 = aMax - aMin;
            float v5 = aVal - aMin;
            aVal = std::fmodf(v5, v6) + aMin;
        }
    }
}
