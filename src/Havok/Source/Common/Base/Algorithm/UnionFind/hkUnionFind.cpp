#include "Havok/Source/Common/Base/Algorithm/UnionFind/hkUnionFind.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalBuffer.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

hkUnionFind::hkUnionFind(hkFixedArray<int32_t>* apParents, int32_t aiNumNodes)
  : m_parents(apParents), m_numNodes(aiNumNodes) {
    // Every node starts as its own root.
    for (int32_t i = 0; i < aiNumNodes; ++i) {
        m_parents->m_data.m_storage[i] = -1;
    }
}

void hkUnionFind::collapseTree() {
    int32_t* m_storage = m_parents->m_data.m_storage;
    int32_t* v2 = m_storage;
    int32_t* v3 = &m_storage[m_numNodes];
    if (m_storage != v3) {
        do {
            if (*v2 >= 0 && m_storage[*v2] >= 0) {
                do {
                    *v2 = m_storage[*v2];
                    m_storage = m_parents->m_data.m_storage;
                }
                while (m_storage[*v2] >= 0);
            }
            ++v2;
        }
        while (v2 != v3);
    }
}

void hkUnionFind::addEdge(int32_t i1, int32_t i2) {
    int32_t* m_storage = m_parents->m_data.m_storage;
    int32_t v4 = i1;
    if (m_storage[i1] >= 0) {
        int32_t v6;
        int32_t v7;
        int32_t v5 = m_storage[i1];
        do {
            v6 = v5;
            v5 = m_storage[v5];
        }
        while (v5 >= 0);
        do {
            v7 = m_storage[v4];
            m_storage[v4] = v6;
            m_storage = m_parents->m_data.m_storage;
            v4 = v7;
        }
        while (m_storage[v7] >= 0);
    }
    int32_t v8 = i2;
    int32_t* v9 = m_parents->m_data.m_storage;
    int32_t v10 = v9[i2];
    int32_t v11 = i2;
    bool v12 = v10 < 0;
    if (v10 >= 0) {
        int32_t v13 = v9[i2];
        do {
            v11 = v13;
            v13 = v9[v13];
        }
        while (v13 >= 0);
        v12 = v10 < 0;
    }
    if (!v12) {
        int32_t v14;
        do {
            v14 = v9[v8];
            v9[v8] = v11;
            v9 = m_parents->m_data.m_storage;
            v8 = v14;
        }
        while (v9[v14] >= 0);
    }
    if (v4 != v8) {
        int32_t* v15 = m_parents->m_data.m_storage;
        if (v4 >= v8) {
            v15[v8] += v15[v4];
            m_parents->m_data.m_storage[v4] = v8;
        }
        else {
            v15[v4] += v15[v8];
            m_parents->m_data.m_storage[v8] = v4;
        }
    }
}

void hkUnionFind::assignGroups(hkArray<int32_t, hkContainerHeapAllocator>& elementsPerGroup) {
    collapseTree();
    int32_t v3 = 0;
    for (int32_t i = 0; v3 < m_numNodes; ++v3) {
        int32_t* m_storage = m_parents->m_data.m_storage;
        int32_t v5 = m_storage[v3];
        if (v5 >= 0) {
            m_storage[v3] = m_storage[v5];
        }
        else {
            int32_t v6 = -v5;
            hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
            if (elementsPerGroup.m_size ==
                (elementsPerGroup.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*Instance->m_heap, &elementsPerGroup, sizeof(int32_t));
            }
            elementsPerGroup.m_data[elementsPerGroup.m_size++] = v6;
            m_parents->m_data.m_storage[v3] = i++;
        }
    }
}

void hkUnionFind::sortByGroupId(
    const hkArray<int32_t, hkContainerHeapAllocator>& elementsPerGroup,
    hkArray<int32_t, hkContainerHeapAllocator>& orderedGroups
) const {
    int32_t v24;
    int32_t m_numNodes = this->m_numNodes;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    hkArray<int32_t, hkContainerHeapAllocator>* v5 = &orderedGroups;
    int32_t v6 = orderedGroups.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK;
    if (v6 < m_numNodes) {
        int32_t v7 = 2 * v6;
        if (m_numNodes >= v7) {
            v7 = m_numNodes;
        }
        hkArrayUtil::_reserve(*m_heap, &orderedGroups, v7, sizeof(int32_t));
    }
    const hkArray<int32_t, hkContainerHeapAllocator>* v8 = &elementsPerGroup;
    v5->m_size = m_numNodes;
    int32_t m_size = v8->m_size;
    if (m_size > 0) {
        int32_t v13;
        int32_t v23;
        hkMemoryAllocator* m_temp = (&hkMemoryRouter::getInstance())->m_temp;
        v24 = sizeof(uint32_t) * m_size;
        uint32_t* v12 = static_cast<uint32_t*>(m_temp->bufAlloc(v24));
        if (v24 / 4) {
            v13 = v24 / 4;
            v23 = v24 / 4;
        }
        else {
            v13 = 0x80000000u;
            v23 = 0x80000000u;
        }
        int32_t v14 = 0;
        for (int32_t i = 0; i < m_size; ++i) {
            v12[i] = v14;
            v14 += v8->m_data[i];
        }
        int32_t* m_data = (&orderedGroups)->begin();
        int32_t* m_storage = m_parents->m_data.m_storage;
        int32_t v18 = 0;
        if (this->m_numNodes > 0) {
            do {
                int32_t v19 = m_storage[v18];
                int32_t v20 = v12[v19];
                v12[v19] = v20 + 1;
                m_data[v20] = v18++;
            }
            while (v18 < this->m_numNodes);
            v13 = v23;
        }
        hkMemoryAllocator* v21 = (&hkMemoryRouter::getInstance())->m_temp;
        if (v13 >= 0) {
            v21->bufFree(v12, sizeof(uint32_t) * v13);
        }
    }
}

// The local array of new group sizes takes its storage from the temp allocator; its capacity is the granted bytes / 4
// (DONT_DEALLOCATE_FLAG when none), and it is zeroed.
void hkUnionFind::reindex(
    const hkFixedArray<int32_t>& rindex,
    int32_t numNewGroups,
    hkArray<int32_t, hkContainerHeapAllocator>& elementsPerGroup
) {
    int32_t v6;
    int32_t* v7;
    int32_t* pNewElementsPerGroup;
    int32_t iCapacity;
    for (int32_t i = 0; i < m_numNodes; *v7 = v6) {
        int32_t* m_storage = m_parents->m_data.m_storage;
        v6 = rindex.m_data.m_storage[m_storage[i]];
        v7 = &m_storage[i++];
    }
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    int32_t aiNumNewGroups_1 = numNewGroups;
    hkMemoryAllocator* m_temp = Instance->m_temp;
    if (numNewGroups) {
        numNewGroups *= 4;
        pNewElementsPerGroup = static_cast<int32_t*>(m_temp->bufAlloc(numNewGroups));
        iCapacity = numNewGroups / 4;
        if (!iCapacity) {
            iCapacity = static_cast<int32_t>(hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG);
        }
    }
    else {
        pNewElementsPerGroup = nullptr;
        iCapacity = static_cast<int32_t>(hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG);
    }
    if (aiNumNewGroups_1 > 0) {
        for (int32_t k = 0; k < aiNumNewGroups_1; ++k) {
            pNewElementsPerGroup[k] = 0;
        }
    }
    int32_t m_size = 0;
    if (elementsPerGroup.m_size > 0) {
        do {
            pNewElementsPerGroup[rindex.m_data.m_storage[m_size]] += elementsPerGroup.m_data[m_size];
            ++m_size;
        }
        while (m_size < elementsPerGroup.m_size);
    }
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t aiNumNewGroups_3 = elementsPerGroup.m_capacityAndFlags & hkArrayBase<int32_t>::CAPACITY_MASK;
    if (aiNumNewGroups_3 < aiNumNewGroups_1) {
        int32_t aiNumNewGroups_2 = 2 * aiNumNewGroups_3;
        if (aiNumNewGroups_1 >= aiNumNewGroups_2) {
            aiNumNewGroups_2 = aiNumNewGroups_1;
        }
        hkArrayUtil::_reserve(*m_heap, &elementsPerGroup, aiNumNewGroups_2, sizeof(int32_t));
    }
    int32_t aiNumNewGroups_4 = 0;
    for (elementsPerGroup.m_size = aiNumNewGroups_1; aiNumNewGroups_4 < aiNumNewGroups_1; ++aiNumNewGroups_4) {
        elementsPerGroup.m_data[aiNumNewGroups_4] = pNewElementsPerGroup[aiNumNewGroups_4];
    }
    hkMemoryAllocator* m_temp_1 = (&hkMemoryRouter::getInstance())->m_temp;
    if (iCapacity >= 0) {
        m_temp_1->bufFree(pNewElementsPerGroup, sizeof(int32_t) * iCapacity);
    }
}

int32_t hkUnionFind::moveBiggestGroupToIndexZero(hkArray<int32_t, hkContainerHeapAllocator>& elementsPerGroup) {
    int32_t* v15;
    int32_t v17;
    int32_t v2 = 0;
    int32_t m_size = elementsPerGroup.m_size;
    int32_t v5 = *(&elementsPerGroup)->begin();
    int32_t v6 = 1;
    if (m_size <= 1) {
        return 0;
    }
    int32_t* v7 = (&elementsPerGroup)->begin() + 1;
    do {
        if (*v7 > v5) {
            v5 = *v7;
            v2 = v6;
        }
        ++v6;
        ++v7;
    }
    while (v6 < m_size);
    if (!v2) {
        return 0;
    }
    hkMemoryAllocator* m_temp = (&hkMemoryRouter::getInstance())->m_temp;
    v17 = sizeof(int32_t) * m_size;
    int32_t* v11 = static_cast<int32_t*>(m_temp->bufAlloc(v17));
    int32_t v18 = v17 / 4;
    if (!(v17 / 4)) {
        v18 = 0x80000000u;
    }
    for (int32_t i = 0; i < m_size; ++i) {
        v11[i] = i;
    }
    *v11 = v2;
    v11[v2] = 0;
    int32_t v13 = elementsPerGroup.m_data[v2];
    elementsPerGroup.m_data[v2] = *(&elementsPerGroup)->begin();
    *(&elementsPerGroup)->begin() = v13;
    for (int32_t j = 0; j < m_numNodes; *v15 = v11[*v15]) {
        v15 = &m_parents->m_data.m_storage[j++];
    }
    hkMemoryAllocator* v16 = (&hkMemoryRouter::getInstance())->m_temp;
    if (v18 >= 0) {
        v16->bufFree(v11, sizeof(int32_t) * v18);
    }
    return v2;
}

int32_t hkUnionFind::findRootOfNode(int32_t i) {
    int32_t result = i;
    int32_t* m_storage = m_parents->m_data.m_storage;
    if (m_storage[i] >= 0) {
        int32_t v5;
        int32_t v6;
        int32_t v4 = m_storage[i];
        do {
            v5 = v4;
            v4 = m_storage[v4];
        }
        while (v4 >= 0);
        do {
            v6 = m_storage[result];
            m_storage[result] = v5;
            m_storage = m_parents->m_data.m_storage;
            result = v6;
        }
        while (m_storage[v6] >= 0);
    }
    return result;
}

void hkUnionFind::unionRoots(int32_t r1, int32_t r2) {
    int32_t* m_storage = m_parents->m_data.m_storage;
    if (r1 >= r2) {
        m_storage[r2] += m_storage[r1];
        m_parents->m_data.m_storage[r1] = r2;
    }
    else {
        m_storage[r1] += m_storage[r2];
        m_parents->m_data.m_storage[r2] = r1;
    }
}
