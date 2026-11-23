#include "Havok/Source/Common/SceneData/Material/hkxMaterial.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeHolder.h"

namespace {
    // Orders texture stages by usage hint, then texture coordinate channel.
    class _texStageLessFn {
    public:
        bool operator()(const hkxMaterial::TextureStage& a, const hkxMaterial::TextureStage& b) const {
            return a.m_usageHint.m_storage < b.m_usageHint.m_storage ||
                (a.m_usageHint.m_storage == b.m_usageHint.m_storage && a.m_tcoordChannel < b.m_tcoordChannel);
        }
    };
} // namespace

static void downHeap(hkxMaterial::TextureStage* apArray, int32_t k, int32_t n, const hkxMaterial* x0);
// Heap sort of hkxMaterial::TextureStage entries using _texStageLessFn. pArr is the m_data of the texture stage array
// sorted by hkxMaterial::sortTextureStageOrder. x0 is the heap sort's empty comparator slot, which the one caller fills
// with its this; it is never read here.
static void heapSortTextureStages(hkxMaterial::TextureStage* pArr, int32_t iSize, const hkxMaterial* x0);

// Heap sort of texture stages: builds the heap, then swaps the first stage with the last of the heap and sifts the new
// first one down, n times.
static void heapSortTextureStages(hkxMaterial::TextureStage* pArr, int32_t iSize, const hkxMaterial* x0) {
    int32_t n = iSize;
    for (int32_t k = n / 2; k > 0; --k) {
        downHeap(pArr, k, n, x0);
    }
    hkxMaterial::TextureStage* pLast = &pArr[n - 1];
    while (n >= 1) {
        hkxMaterial::TextureStage kTemp(*pArr);
        *pArr = *pLast;
        *pLast = kTemp;
        --n;
        --pLast;
        downHeap(pArr, 1, n, x0);
    }
}

// Sifts apArray[k - 1] down the heap of the first n stages, ordered by usage hint, then texture coordinate channel. x0
// is the empty comparator, passed on and never read. Copying a stage's hkRefVariant adds a reference to the texture it
// takes and removes the one it held.
static void downHeap(hkxMaterial::TextureStage* apArray, int32_t k, int32_t n, const hkxMaterial* x0) {
    hkxMaterial::TextureStage* pStage = &apArray[k - 1];
    hkxMaterial::TextureStage kTemp(*pStage);
    int32_t iHalf = n / 2;
    while (k <= iHalf) {
        int32_t iChild = 2 * k;
        if (iChild < n) {
            const hkxMaterial::TextureStage& kLeft = apArray[iChild - 1];
            const hkxMaterial::TextureStage& kRight = apArray[iChild];
            if (_texStageLessFn()(kLeft, kRight)) {
                ++iChild;
            }
        }
        hkxMaterial::TextureStage* pChild = &apArray[iChild - 1];
        if (!_texStageLessFn()(kTemp, *pChild)) {
            break;
        }
        apArray[k - 1] = *pChild;
        k = iChild;
    }
    apArray[k - 1] = kTemp;
}

// The members' destructors release the extra data, the sub-materials and the stages' textures, free the arrays and
// release m_name; the base's destroys the attribute groups.
hkxMaterial::~hkxMaterial() {}

// Equivalent to hkAlgorithm::heapSort(m_stages.begin(), m_stages.getSize()); this is passed as the empty comparator's
// argument.
void hkxMaterial::sortTextureStageOrder() {
    heapSortTextureStages(m_stages.m_data, m_stages.m_size, this);
}
