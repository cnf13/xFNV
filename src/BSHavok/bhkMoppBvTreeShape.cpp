#include "BSHavok/bhkMoppBvTreeShape.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkBvTreeShape.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/bhkStream.h"
#include "BSHavok/hkScaledMoppBvTreeShape.h"
#include "BSMain/BSCloningProcess.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppCompilerInput.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppUtility.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h"

#include <bit>
#include <xmmintrin.h>

NiRTTI bhkMoppBvTreeShape::ms_RTTI("bhkMoppBvTreeShape", &bhkBvTreeShape::ms_RTTI);

bhkMoppBvTreeShape::bhkMoppBvTreeShape() {
    ++iCount;
}

// The base chain counts each level, then the MOPP code is built over apShape.
bhkMoppBvTreeShape::bhkMoppBvTreeShape(bhkShape* apShape) {
    ++iCount;
    Init(static_cast<bhkShapeCollection*>(apShape));
}

bhkMoppBvTreeShape::~bhkMoppBvTreeShape() {
    bhkSerializable::Kill();
    --iCount;
}

const NiRTTI* bhkMoppBvTreeShape::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* bhkMoppBvTreeShape::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    bhkSerializable::CloneCritLock();
    bhkMoppBvTreeShape* pClone = new bhkMoppBvTreeShape;
    // The clone is filled through this object's virtual bhkShape::CopyMembers.
    CopyMembers(pClone, arCloneData);
    bhkSerializable::CloneCritUnlock();
    return pClone;
}

void bhkMoppBvTreeShape::LoadBinary(NiStream& arStream) {
    bool bCreated;
    bhkBvTreeShape::LoadBinary(arStream);
    hkMoppBvTreeShapeCinfo* cinfo = static_cast<hkMoppBvTreeShapeCinfo*>(&CreateCinfo(bCreated));
    hkpMoppCode* pc = nullptr;
    bhkStream::LoadBinary(arStream, pc);
    cinfo->m_code = pc;
}

void bhkMoppBvTreeShape::LinkObject(NiStream& arStream) {
    bhkBvTreeShape::LinkObject(arStream);
}

bool bhkMoppBvTreeShape::RegisterStreamables(NiStream& arStream) {
    return bhkBvTreeShape::RegisterStreamables(arStream);
}

// Writes the tree, then its MOPP code, with the creation info's scale byte-swapped for a big-endian stream; the swap is
// undone afterwards unless the info was made here (KillCinfo has dropped it then). CreateCinfo and KillCinfo are called
// virtually.
void bhkMoppBvTreeShape::SaveBinary(NiStream& arStream) {
    bool bCreated;
    bhkSerializable* pThis = this;
    hkMoppBvTreeShapeCinfo* pCinfo = static_cast<hkMoppBvTreeShapeCinfo*>(&pThis->CreateCinfo(bCreated));
    // Compares the flag's raw byte with 1: any other byte swaps.
    if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        pCinfo->fScale = std::bit_cast<float>(std::byteswap<uint32_t>(std::bit_cast<uint32_t>(pCinfo->fScale)));
    }
    bhkBvTreeShape::SaveBinary(arStream);
    bhkStream::SaveBinary(arStream, pCinfo->m_code);
    pThis->KillCinfo(bCreated);
    if (!bCreated && *static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSaveLittleEndian)) != 1) {
        pCinfo->fScale = std::bit_cast<float>(std::byteswap<uint32_t>(std::bit_cast<uint32_t>(pCinfo->fScale)));
    }
}

bool bhkMoppBvTreeShape::IsEqual(NiObject* apObject) {
    return bhkBvTreeShape::IsEqual(apObject);
}

void bhkMoppBvTreeShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    hkReferencedObject* phkObject;
    const hkpMoppCode* pcode;
    NiPoint3 offset;
    char apDest[32];
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    bhkBvTreeShape::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkMoppBvTreeShape::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* element = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, element);
    if (this && (phkObject = this->phkObject) != nullptr) {
        pcode = static_cast<hkpMoppBvTreeShape*>(phkObject)->m_code;
    }
    else {
        pcode = nullptr;
    }
    BSsprintf(apDest, 32, "0x%8X", pcode);
    char* ViewerString_9 = NiGetViewerString("MoppCode", static_cast<const char*>(apDest));
    uint32_t v10 = apStrings->m_usSize;
    element = ViewerString_9;
    if (v10 >= apStrings->m_usMaxSize) {
        apStrings->SetSize(v10 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v10, element);
    HK2NI(offset, pcode->m_info.m_offset);
    char* v11 = offset.GetViewerString("MoppCode Offset");
    uint32_t v12 = apStrings->m_usSize;
    uint32_t v13 = apStrings->m_usMaxSize;
    element = v11;
    if (v12 >= v13) {
        apStrings->SetSize(v12 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v12, element);
    v3->eMemContext = eMemContext;
}

// The size of the MOPP shape's construction info in a file: its serialized form's.
uint32_t bhkMoppBvTreeShape::QCinfoSize() const {
    return sizeof(hkMoppBvTreeShapeCinfo::Serialized);
}

// Reads the creation info's serialized form (the 32-bit program's structure) and fills the native info from it. Version
// 6 and later: the whole form, then the scale byte-swapped when the source is big-endian. Older files have no scale:
// the form's first QCinfoSize() - 4 bytes are read, the scale is kept, and a warning is given; the size read is
// returned.
int32_t bhkMoppBvTreeShape::LoadCInfo(NiStream& arStream) {
    uint32_t uiSize = QCinfoSize();
    bool bCreated;
    hkMoppBvTreeShapeCinfo* pCinfo = static_cast<hkMoppBvTreeShapeCinfo*>(&CreateCinfo(bCreated));
    hkMoppBvTreeShapeCinfo::Serialized kForm{};
    if (arStream.m_BSStreamHeader.uiVersion >= 6) {
        arStream.Istr()->BinaryRead(&kForm, sizeof(kForm), nullptr, 0);
        pCinfo->ReadSerialized(kForm);
        // Compares the flag's raw byte with 1: any other byte swaps.
        if (*static_cast<const uint8_t*>(static_cast<const void*>(&arStream.m_bSourceIsLittleEndian)) != 1) {
            pCinfo->fScale = std::bit_cast<float>(std::byteswap<uint32_t>(std::bit_cast<uint32_t>(pCinfo->fScale)));
        }
    }
    else {
        uiSize -= 4;
        kForm.fScale = pCinfo->fScale;
        arStream.Istr()->BinaryRead(&kForm, uiSize, nullptr, 0);
        pCinfo->ReadSerialized(kForm);
        const char* pAuthor = arStream.m_BSStreamHeader.cAuthor;
        if (!arStream.m_BSStreamHeader.cAuthor[0]) {
            pAuthor = "Please";
        }
        char acBuffer[512];
        hkOstream kStream(acBuffer, 512, true);
        kStream << "File " << static_cast<const char*>(arStream.m_acFileName) << " contains an old bhkMoppBvTreeShape! "
                << pAuthor << " re-export\n";
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING, 0x234F2250, acBuffer,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\BSHavok\\bhkMoppBvTreeShape.cpp", 187
        );
    }
    return uiSize;
}

// Builds the Havok shape from the creation info: its MOPP code (or, when it has none, one built over the shape
// collection, chunk-subdivided for an extended mesh) copied with the offset scaled by the info's scale (xyz by it, w by
// its inverse) into an hkScaledMoppBvTreeShape that keeps the unscaled code. The new shape becomes the wrapped object,
// all inside the Havok memory context.
void bhkMoppBvTreeShape::Init(const hkSerializableCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    const hkMoppBvTreeShapeCinfo& rCinfo = static_cast<const hkMoppBvTreeShapeCinfo&>(arInfo);
    hkpMoppCode* pGenCode = nullptr;
    if (!rCinfo.m_code) {
        hkpMoppCompilerInput kInput;
        hkpShapeCollection* pCollection = rCinfo.m_shapeCollection;
        if (pCollection->m_type.m_storage == HK_SHAPE_EXTENDED_MESH) {
            kInput.m_enableChunkSubdivision.m_bool = 1;
        }
        pGenCode = hkpMoppUtility::buildCode(pCollection, kInput, nullptr);
    }
    const hkpMoppCode* pCode = rCinfo.m_code;
    if (!pCode) {
        pCode = pGenCode;
    }
    float fScale = rCinfo.fScale;
    float fInvScale = 1.0f / rCinfo.fScale;
    hkVector4 kScale;
    kScale.set(fScale, fScale, fScale, fInvScale);
    hkpMoppCode::CodeInfo kInfo;
    kInfo.m_offset.setMul4(pCode->m_info.m_offset, kScale);
    // The copy runs over the code's own data (not freed by the copy).
    hkpMoppCode* pScaledCode =
        static_cast<hkpMoppCode*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpMoppCode)));
    pScaledCode->m_memSizeAndFlags = sizeof(hkpMoppCode);
    int32_t iSize = pCode->m_data.m_size;
    uint8_t* pData = pCode->m_data.m_data;
    ::new (pScaledCode) hkpMoppCode(kInfo, pData, iSize, hkpMoppCode::BUILD_NOT_SET);
    void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkScaledMoppBvTreeShape));
    static_cast<hkScaledMoppBvTreeShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkScaledMoppBvTreeShape);
    hkScaledMoppBvTreeShape* pShape =
        ::new (pMemory) hkScaledMoppBvTreeShape(rCinfo.m_shapeCollection, pScaledCode, pCode);
    pScaledCode->removeReference();
    if (pGenCode) {
        pGenCode->removeReference();
    }
    bhkSerializable* pThis = this;
    pThis->SethkObject(pShape);
    pShape->removeReference();
    pThis->PostInit(arInfo);
}

hkSerializableCinfo& bhkMoppBvTreeShape::CreateCinfo(bool& arCreated) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    bool bCreated = this->pInfo == nullptr;
    arCreated = bCreated;
    if (bCreated) {
        hkMoppBvTreeShapeCinfo* pCinfo =
            static_cast<hkMoppBvTreeShapeCinfo*>(MemoryManager_Singleton.Allocate(sizeof(hkMoppBvTreeShapeCinfo)));
        if (pCinfo) {
            pCinfo->m_userData = 0;
            pCinfo->fScale = 1.0f;
            pCinfo->m_shapeCollection = nullptr;
            pCinfo->m_code = nullptr;
        }
        this->pInfo = pCinfo;
        if (this->phkObject) {
            CopyCinfo(*pCinfo);
        }
    }
    return *this->pInfo;
}

void bhkMoppBvTreeShape::PostInit(const hkSerializableCinfo& arInfo) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);

    const hkMoppBvTreeShapeCinfo& rCinfo = static_cast<const hkMoppBvTreeShapeCinfo&>(arInfo);
    if (rCinfo.m_code) {
        rCinfo.m_code->removeReference();
    }
    bhkSerializable::PostInit(arInfo);
}

// The creation info's scale is multiplied by the cloning process's, and a reference is added to its MOPP code for the
// clone's info; then the tree's members are copied. CreateCinfo is called virtually, and the info it may make is
// deliberately not killed, as the game does.
void bhkMoppBvTreeShape::CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData) {
    bool bCreated;
    bhkSerializable* pThis = this;
    hkMoppBvTreeShapeCinfo& rCinfo = static_cast<hkMoppBvTreeShapeCinfo&>(pThis->CreateCinfo(bCreated));
    // The clone process is always a BSCloningProcess; only Scale.x is read.
    rCinfo.fScale = static_cast<BSCloningProcess&>(arCloneData).Scale.x * rCinfo.fScale;
    const hkpMoppCode* pCode = rCinfo.m_code;
    if (pCode) {
        pCode->addReference();
    }
    bhkBvTreeShape::CopyMembers(apDest, arCloneData);
}

// Builds the MOPP code over the collection's Havok shape (chunk-subdivided for an extended mesh) and hands it, with
// scale 1, to the virtual Init in a creation info, inside the Havok memory context.
void bhkMoppBvTreeShape::Init(bhkShapeCollection* apShapeCollection) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    // EXE-INLINED 0x537B80
    alignas(16) hkMoppBvTreeShapeCinfo kCinfo;
    kCinfo.fScale = 1.0f;
    kCinfo.m_userData = 0;
    kCinfo.m_code = nullptr;
    kCinfo.m_shapeCollection = static_cast<hkpShapeCollection*>(apShapeCollection->phkObject);
    hkpMoppCompilerInput kInput;
    hkpShapeCollection* pCollection = static_cast<hkpShapeCollection*>(apShapeCollection->phkObject);
    if (pCollection->m_type.m_storage == HK_SHAPE_EXTENDED_MESH) {
        kInput.m_enableChunkSubdivision.m_bool = 1;
    }
    kCinfo.m_code = hkpMoppUtility::buildCode(pCollection, kInput, nullptr);
    bhkSerializable* pThis = this;
    pThis->Init(kCinfo);
}

// Fills a freshly created creation info from the wrapped shape (the hkScaledMoppBvTreeShape Init builds): the tree's
// base info, the unscaled MOPP code, and the scale, recovered as the scaled code's offset x over the unscaled one's.
void bhkMoppBvTreeShape::CopyCinfo(hkMoppBvTreeShapeCinfo& arInfo) {
    bhkBvTreeShape::CopyCinfo(arInfo);
    const hkScaledMoppBvTreeShape* pShape = this ? static_cast<hkScaledMoppBvTreeShape*>(this->phkObject) : nullptr;
    arInfo.m_code = pShape ? pShape->pOriginalCode : nullptr;
    if (this) {
        // The wrapped object is reloaded after the store above.
        pShape = static_cast<hkScaledMoppBvTreeShape*>(this->phkObject);
        if (pShape && pShape->m_code && pShape->pOriginalCode) {
            arInfo.fScale = pShape->m_code->m_info.m_offset(0) / pShape->pOriginalCode->m_info.m_offset(0);
        }
    }
}

NiObject* bhkMoppBvTreeShape::CreateObject() {
    return new bhkMoppBvTreeShape;
}

// The MOPP tree over apCode, keeping a reference to the unscaled code apOriginal.
hkScaledMoppBvTreeShape::hkScaledMoppBvTreeShape(
    const hkpShapeCollection* apCollection,
    const hkpMoppCode* apCode,
    const hkpMoppCode* apOriginal
)
  : hkpMoppBvTreeShape(apCollection, apCode), pOriginalCode(apOriginal) {
    if (apOriginal) {
        apOriginal->addReference();
    }
}

hkpMoppCode::hkpMoppCode(
    const CodeInfo& arInfo,
    const uint8_t* apMoppData,
    int32_t aiMoppBytes,
    BuildType aeBuildType
) {
    m_info.m_offset = arInfo.m_offset;
    m_data.m_size = aiMoppBytes;
    m_data.m_data = const_cast<uint8_t*>(apMoppData);
    m_data.m_capacityAndFlags = aiMoppBytes | hkArray<uint8_t>::DONT_DEALLOCATE_FLAG;
    m_buildType.m_storage = aeBuildType;
}
