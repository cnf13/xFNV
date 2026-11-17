#include "Havok/Source/Common/SceneData/Graph/hkxNode.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeHolder.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkxNodeAnnotationDataTypeInfo = {
    "hkxNodeAnnotationData",
    finishLoadedObjecthkxNodeAnnotationData,
    cleanupLoadedObjecthkxNodeAnnotationData,
    nullptr,
    &typeid(hkxNode::AnnotationData),
    sizeof(hkxNode::AnnotationData)
};

// The reflected members of hkxNode::AnnotationData and hkxNode.
using hkxNode_AnnotationDataClass_MembersArray = const hkClassMember[2];
static hkxNode_AnnotationDataClass_MembersArray hkxNode_AnnotationDataClass_Members{
    {"time", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"description", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr}
};
using hkxNodeClass_MembersArray = const hkClassMember[7];
static hkxNodeClass_MembersArray hkxNodeClass_Members{
    {"name", nullptr, nullptr, {hkClassMember::TYPE_STRINGPTR}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr},
    {"object",
     &hkReferencedObjectClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x18,
     nullptr},
    {"keyFrames", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_MATRIX4}, 0, {0}, 0x1C, nullptr},
    {"children",
     &hkxNodeClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x28,
     nullptr},
    {"annotations",
     &hkxNodeAnnotationDataClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x34,
     nullptr},
    {"userProperties",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_STRINGPTR},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x40,
     nullptr},
    {"selected", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x44, nullptr}
};

hkClass hkxNodeAnnotationDataClass(
    "hkxNodeAnnotationData",
    nullptr,
    8,
    nullptr,
    0,
    nullptr,
    0,
    hkxNode_AnnotationDataClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkxNodeClass(
    "hkxNode",
    &hkxAttributeHolderClass,
    0x48,
    nullptr,
    0,
    nullptr,
    0,
    hkxNodeClass_Members,
    7,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxNodeTypeInfo = {"hkxNode",          finishLoadedObjecthkxNode, cleanupLoadedObjecthkxNode,
                              getVtablehkxNode(), &typeid(hkxNode),          sizeof(hkxNode)};

void cleanupLoadedObjecthkxNode(void* p) {
    static_cast<hkxNode*>(p)->~hkxNode();
}

const void* getVtablehkxNode() {
    return hkGetVtableOf<hkxNode>();
}

const hkClass& hkxNode::AnnotationData::staticClass() {
    return hkxNodeAnnotationDataClass;
}

// The members' destructors release the strings, the object and the children and free the arrays; the base's
// destroys the attribute groups.
hkxNode::~hkxNode() {}

hkxNode* hkxNode::findChildByName(const char* childName) const {
    hkxNode* m_pntr;
    if (!childName) {
        return nullptr;
    }
    int32_t v4 = 0;
    if (m_children.m_size <= 0) {
        return nullptr;
    }
    while (true) {
        m_pntr = m_children.m_data[v4].val();
        const char* m_stringAndFlag = m_pntr->m_name.m_stringAndFlag;
        if ((reinterpret_cast<uintptr_t>(m_stringAndFlag) & ~uintptr_t(1)) != 0 &&
            !hkString::strCasecmp(
                reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(m_stringAndFlag) & ~uintptr_t(1)), childName
            )) {
            break;
        }
        if (++v4 >= m_children.m_size) {
            return nullptr;
        }
    }
    return m_pntr;
}

hkxNode* hkxNode::findDescendantByName(const char* name) const {
    hkxNode* m_pntr;
    if (!name) {
        return nullptr;
    }
    int32_t v4 = 0;
    if (m_children.m_size <= 0) {
        return nullptr;
    }
    while (true) {
        m_pntr = m_children.m_data[v4].val();
        const char* m_stringAndFlag = m_pntr->m_name.m_stringAndFlag;
        if ((reinterpret_cast<uintptr_t>(m_stringAndFlag) & ~uintptr_t(1)) != 0 &&
            !hkString::strCasecmp(
                reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(m_stringAndFlag) & ~uintptr_t(1)), name
            )) {
            break;
        }
        hkxNode* result = m_pntr->findDescendantByName(name);
        if (result) {
            return result;
        }
        if (++v4 >= m_children.m_size) {
            return nullptr;
        }
    }
    return m_pntr;
}

void hkxNode::replaceAllObjects(const hkReferencedObject* oldObject, const hkReferencedObject* newObject) {
    if (m_object.m_pntr == oldObject && m_object.m_pntr != newObject) {
        if (newObject) {
            newObject->addReference();
        }
        hkReferencedObject* m_pntr = m_object.m_pntr;
        if (m_pntr) {
            m_pntr->removeReference();
        }
        // The const_cast is needed because the parameters are const while the variant holds a non-const pointer.
        m_object.m_pntr = const_cast<hkReferencedObject*>(newObject);
    }
    for (int32_t i = 0; i < m_children.m_size; ++i) {
        m_children.m_data[i].val()->replaceAllObjects(oldObject, newObject);
    }
}

int32_t hkxNode::getNumDescendants() const {
    int32_t m_size = m_children.m_size;
    int32_t v3 = 0;
    if (m_size > 0) {
        do {
            int32_t NumDescendants = m_children.m_data[v3++].val()->getNumDescendants();
            m_size += NumDescendants;
        }
        while (v3 < m_children.m_size);
    }
    return m_size;
}

hkRefVariant hkxNode::findVariantByObject(const hkReferencedObject* object) const {
    hkRefVariant result;
    if (m_object.m_pntr == object) {
        hkReferencedObject* m_pntr = m_object.m_pntr;
        if (m_pntr) {
            m_pntr->addReference();
        }
        result.m_pntr = m_object.m_pntr;
        return result;
    }

    int32_t v7 = 0;
    if (m_children.m_size <= 0) {
    LABEL_8:;
        result.m_pntr = nullptr;
        return result;
    }

    hkRefVariant v10;
    while (true) {
        v10 = m_children.m_data[v7].val()->findVariantByObject(object);
        if (v10.m_pntr) {
            break;
        }
        bool v8 = ++v7 < m_children.m_size;
        v10.m_pntr = nullptr;
        if (!v8) {
            goto LABEL_8;
        }
    }
    v10.m_pntr->addReference();
    hkReferencedObject* v9 = v10.m_pntr;
    result.m_pntr = v10.m_pntr;
    if (v9) {
        v9->removeReference();
    }
    return result;
}

hkResult hkxNode::getPathToNode(const hkxNode* theNode, hkArray<const hkxNode*, hkContainerHeapAllocator>& pathOut) {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    if (pathOut.m_size == (pathOut.m_capacityAndFlags & hkArrayBase<const hkxNode*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*Instance->m_heap, &pathOut, sizeof(const hkxNode*));
    }
    pathOut.m_data[pathOut.m_size] = this;
    hkResult result = HK_FAILURE;
    ++pathOut.m_size;
    if (this == theNode) {
        return HK_SUCCESS;
    }
    int32_t v6 = 0;
    if (m_children.m_size <= 0) {
    LABEL_8:;
        --pathOut.m_size;
    }
    else {
        while (true) {
            result = m_children.m_data[v6].val()->getPathToNode(theNode, pathOut);
            if (result == HK_SUCCESS) {
                break;
            }
            if (++v6 >= m_children.m_size) {
                goto LABEL_8;
            }
        }
    }
    return result;
}

const hkClass& hkxNode::staticClass() {
    return hkxNodeClass;
}
