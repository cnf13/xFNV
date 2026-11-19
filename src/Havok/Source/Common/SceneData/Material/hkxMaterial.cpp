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

static void downHeap(hkxMaterial::TextureStage* apArray, int32_t k, int32_t n, const hkxMaterial* x0);
// Heap sort of hkxMaterial::TextureStage entries using _texStageLessFn. a1 is the m_data of the texture stage array
// sorted by hkxMaterial::sortTextureStageOrder. x0 is the heap sort's empty comparator slot, which the one caller fills
// with its this; it is never read here.
static void heapSortTextureStages(hkxMaterial::TextureStage* a1, int32_t a2, const hkxMaterial* x0);

// Heap sort of texture stages: builds the heap, then swaps the first stage with the last of the heap and sifts the new
// first one down, n times.
static void heapSortTextureStages(hkxMaterial::TextureStage* a1, int32_t a2, const hkxMaterial* x0) {
    int32_t n = a2;
    for (int32_t k = n / 2; k > 0; --k) {
        downHeap(a1, k, n, x0);
    }
    hkxMaterial::TextureStage* pLast = &a1[n - 1];
    while (n >= 1) {
        hkxMaterial::TextureStage kTemp(*a1);
        hkReferencedObject* pTempTexture = kTemp.m_texture.m_pntr;
        // EXE-INLINED none: hkxMaterial::TextureStage::operator=
        if (a1 != pLast) {
            hkReferencedObject* pTexture = pLast->m_texture.m_pntr;
            if (pTexture) {
                pTexture->addReference();
            }
            if (a1->m_texture.m_pntr) {
                a1->m_texture.m_pntr->removeReference();
            }
            a1->m_texture.m_pntr = pTexture;
        }
        a1->m_usageHint.m_storage = pLast->m_usageHint.m_storage;
        a1->m_tcoordChannel = pLast->m_tcoordChannel;
        // EXE-INLINED none: hkxMaterial::TextureStage::operator= (its self-assignment test, against the local,
        // never holds)
        if (pTempTexture) {
            pTempTexture->addReference();
        }
        if (pLast->m_texture.m_pntr) {
            pLast->m_texture.m_pntr->removeReference();
        }
        pLast->m_texture.m_pntr = pTempTexture;
        pLast->m_usageHint.m_storage = kTemp.m_usageHint.m_storage;
        pLast->m_tcoordChannel = kTemp.m_tcoordChannel;
        --n;
        --pLast;
        downHeap(a1, 1, n, x0);
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
            // EXE-INLINED none: _texStageLessFn::operator()
            if (kLeft.m_usageHint.m_storage < kRight.m_usageHint.m_storage ||
                (kLeft.m_usageHint.m_storage == kRight.m_usageHint.m_storage &&
                 kLeft.m_tcoordChannel < kRight.m_tcoordChannel)) {
                ++iChild;
            }
        }
        hkxMaterial::TextureStage* pChild = &apArray[iChild - 1];
        // EXE-INLINED none: _texStageLessFn::operator()
        if (!(kTemp.m_usageHint.m_storage < pChild->m_usageHint.m_storage ||
              (kTemp.m_usageHint.m_storage == pChild->m_usageHint.m_storage &&
               kTemp.m_tcoordChannel < pChild->m_tcoordChannel))) {
            break;
        }
        hkxMaterial::TextureStage* pParent = &apArray[k - 1];
        // EXE-INLINED none: hkxMaterial::TextureStage::operator=
        if (pParent != pChild) {
            hkReferencedObject* pTexture = pChild->m_texture.m_pntr;
            if (pTexture) {
                pTexture->addReference();
            }
            if (pParent->m_texture.m_pntr) {
                pParent->m_texture.m_pntr->removeReference();
            }
            pParent->m_texture.m_pntr = pTexture;
        }
        pParent->m_usageHint.m_storage = pChild->m_usageHint.m_storage;
        pParent->m_tcoordChannel = pChild->m_tcoordChannel;
        k = iChild;
    }
    hkxMaterial::TextureStage* pFinal = &apArray[k - 1];
    // EXE-INLINED none: hkxMaterial::TextureStage::operator= (its self-assignment test, against the local, never
    // holds)
    hkReferencedObject* pTexture = kTemp.m_texture.m_pntr;
    if (pTexture) {
        pTexture->addReference();
    }
    if (pFinal->m_texture.m_pntr) {
        pFinal->m_texture.m_pntr->removeReference();
    }
    pFinal->m_texture.m_pntr = pTexture;
    pFinal->m_usageHint.m_storage = kTemp.m_usageHint.m_storage;
    pFinal->m_tcoordChannel = kTemp.m_tcoordChannel;
}

// The members' destructors release the extra data, the sub-materials and the stages' textures, free the arrays and
// release m_name; the base's destroys the attribute groups.
hkxMaterial::~hkxMaterial() {}

// Equivalent to hkAlgorithm::heapSort(m_stages.begin(), m_stages.getSize()); this is passed as the empty comparator's
// argument, as the game does.
void hkxMaterial::sortTextureStageOrder() {
    heapSortTextureStages(m_stages.m_data, m_stages.m_size, this);
}
