#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesConnectivity.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

static void finishLoadedObjecthkpConvexVerticesConnectivity(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
static const void* getVtablehkpConvexVerticesConnectivity();
static void cleanupLoadedObjecthkpConvexVerticesConnectivity(void* p);

// The class record's member table.
using hkpConvexVerticesConnectivityClassMemberArray = const hkClassMember[2];
static hkpConvexVerticesConnectivityClassMemberArray hkpConvexVerticesConnectivityClass_Members{
    {"vertexIndices", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, 0, 0x8, nullptr},
    {"numVerticesPerFace",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_UINT8},
     0,
     0,
     0x14,
     nullptr}
};
hkClass hkpConvexVerticesConnectivityClass(
    "hkpConvexVerticesConnectivity",
    &hkReferencedObjectClass,
    0x20,
    nullptr,
    0,
    nullptr,
    0,
    hkpConvexVerticesConnectivityClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpConvexVerticesConnectivityTypeInfo{
    "hkpConvexVerticesConnectivity",
    finishLoadedObjecthkpConvexVerticesConnectivity,
    cleanupLoadedObjecthkpConvexVerticesConnectivity,
    getVtablehkpConvexVerticesConnectivity(),
    &typeid(hkpConvexVerticesConnectivity),
    sizeof(hkpConvexVerticesConnectivity)
};

// The type info's finish-loaded-object callback for hkpConvexVerticesConnectivity: runs its finish constructor.
static void finishLoadedObjecthkpConvexVerticesConnectivity(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag aFlag;
        aFlag.m_finishing = finishing;
        ::new (p) hkpConvexVerticesConnectivity(aFlag);
    }
}

static const void* getVtablehkpConvexVerticesConnectivity() {
    return hkGetVtableOf<hkpConvexVerticesConnectivity>();
}

static void cleanupLoadedObjecthkpConvexVerticesConnectivity(void* p) {
    static_cast<hkpConvexVerticesConnectivity*>(p)->~hkpConvexVerticesConnectivity();
}

int32_t hkpConvexVerticesConnectivity::clear() {
    m_vertexIndices.m_size = 0;
    m_numVerticesPerFace.m_size = 0;
    return 0;
}

// The two arrays free their own storage.
hkpConvexVerticesConnectivity::~hkpConvexVerticesConnectivity() {}

void hkpConvexVerticesConnectivity::addFace(int32_t* indices, int32_t numIndices) {
    hkArray<uint8_t, hkContainerHeapAllocator>* p_m_numVerticesPerFace = &m_numVerticesPerFace;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (p_m_numVerticesPerFace->m_size ==
        (p_m_numVerticesPerFace->m_capacityAndFlags & hkArrayBase<uint8_t>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, p_m_numVerticesPerFace, sizeof(uint8_t));
    }
    p_m_numVerticesPerFace->m_data[p_m_numVerticesPerFace->m_size++] = numIndices;
    int32_t v6 = 0;
    if (numIndices > 0) {
        hkArray<uint16_t, hkContainerHeapAllocator>* p_m_vertexIndices = &m_vertexIndices;
        do {
            uint16_t v8 = indices[v6];
            hkMemoryRouter* v9 = &hkMemoryRouter::getInstance();
            if (p_m_vertexIndices->m_size ==
                (p_m_vertexIndices->m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*v9->m_heap, p_m_vertexIndices, sizeof(uint16_t));
            }
            p_m_vertexIndices->m_data[p_m_vertexIndices->m_size++] = v8;
            ++v6;
        }
        while (v6 < numIndices);
    }
}

hkBool* hkpConvexVerticesConnectivity::isClosed(hkBool* apResult) {
    int32_t i;
    hkBool* v18;
    hkUlongMapBase ErrorMessage;
    hkpConvexVerticesConnectivity* v2 = this;
    int32_t v3 = 0;
    ErrorMessage.m_elem = 0;
    ErrorMessage.m_numElems = 0;
    ErrorMessage.m_hashMod = -1;
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    ErrorMessage.reserve(*Instance->m_heap, 0);
    int32_t m_size = v2->m_numVerticesPerFace.m_size;
    int32_t v27 = m_size;
    int32_t v22 = 0;
    int32_t v24 = 0;
    if (m_size > 0) {
        do {
            int32_t v6 = v2->m_numVerticesPerFace.m_data[v3];
            uint16_t* v7 = &v2->m_vertexIndices.m_data[v22];
            int32_t v8 = v7[v6 - 1];
            int32_t v9 = 0;
            int32_t v23 = v6;
            uint16_t* v25 = v7;
            if (v2->m_numVerticesPerFace.m_data[v3]) {
                while (1) {
                    int32_t v10 = v7[v9];
                    int32_t v11 = v8 >= v10 ? v8 | v10 << 16 : v10 | v8 << 16;
                    uint32_t v12 = v11 + 1;
                    uint32_t v13 = (v8 < v10) + 1;
                    int32_t Key = ErrorMessage.findKey(v11 + 1);
                    if (Key > ErrorMessage.m_hashMod) {
                        hkMemoryRouter* v16 = &hkMemoryRouter::getInstance();
                        ErrorMessage.insert(*v16->m_heap, v12, v13);
                    }
                    else {
                        uint32_t val = ErrorMessage.m_elem[Key].val;
                        if ((val & v13) != 0) {
                            goto LABEL_26;
                        }
                        ErrorMessage.m_elem[Key].val = v13 | val;
                    }
                    ++v9;
                    v8 = v10;
                    if (v9 >= v23) {
                        break;
                    }
                    v7 = v25;
                }
                v2 = this;
                m_size = v27;
                v3 = v24;
                v6 = v23;
            }
            v22 += v6;
            v24 = ++v3;
        }
        while (v3 < m_size);
    }
    for (i = 0; i <= ErrorMessage.m_hashMod; ++i) {
        if (ErrorMessage.m_elem[i].key != -1) {
            break;
        }
    }
    if (i > ErrorMessage.m_hashMod) {
    LABEL_24:;
        v18 = apResult;
        apResult->m_bool = 1;
        hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
        ErrorMessage.clearAndDeallocate(*m_heap);
    }
    else {
        while (ErrorMessage.m_elem[i].val == 3) {
            if (++i <= ErrorMessage.m_hashMod) {
                do {
                    if (ErrorMessage.m_elem[i].key != -1) {
                        break;
                    }
                    ++i;
                }
                while (i <= ErrorMessage.m_hashMod);
                if (i <= ErrorMessage.m_hashMod) {
                    continue;
                }
            }
            goto LABEL_24;
        }
    LABEL_26:;
        v18 = apResult;
        apResult->m_bool = 0;
        hkMemoryAllocator* v21 = (&hkMemoryRouter::getInstance())->m_heap;
        ErrorMessage.clearAndDeallocate(*v21);
    }
    return v18;
}

const hkClass& hkpConvexVerticesConnectivity::staticClass() {
    return hkpConvexVerticesConnectivityClass;
}
