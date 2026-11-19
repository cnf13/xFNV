#include "Havok/Source/Physics/Utilities/Constraint/Chain/hkpPoweredChainMapper.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/Powered/hkpPoweredChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpLimitedForceConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Utilities/Constraint/Bilateral/hkpConstraintUtils.h"
#include "Havok/Source/Physics/Utilities/Constraint/Chain/hkpConstraintChainUtil.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpPoweredChainMapperTargetTypeInfo = {
    "hkpPoweredChainMapperTarget",
    nullptr,
    cleanupLoadedObjecthkpPoweredChainMapperTarget,
    nullptr,
    &typeid(hkpPoweredChainMapper::Target),
    sizeof(hkpPoweredChainMapper::Target)
};
hkTypeInfo hkpPoweredChainMapperLinkInfoTypeInfo = {
    "hkpPoweredChainMapperLinkInfo",
    nullptr,
    cleanupLoadedObjecthkpPoweredChainMapperLinkInfo,
    nullptr,
    &typeid(hkpPoweredChainMapper::LinkInfo),
    sizeof(hkpPoweredChainMapper::LinkInfo)
};

// The links' limit constraints and the chains lose their reference; the three arrays' storage goes with them.
hkpPoweredChainMapper::~hkpPoweredChainMapper() {
    for (int32_t i = 0; i < m_links.m_size; ++i) {
        if (m_links.m_data[i].m_limitConstraint) {
            m_links.m_data[i].m_limitConstraint->removeReference();
        }
    }
    for (int32_t i = 0; i < m_chains.m_size; ++i) {
        m_chains.m_data[i]->removeReference();
    }
}

void hkpPoweredChainMapper::getMotors(
    int32_t linkIndex,
    int32_t coordinateIndex,
    hkArray<hkpConstraintMotor*, hkContainerHeapAllocator>& motorsOut
) {
    hkpPoweredChainMapper* v4 = this;
    hkpPoweredChainMapper::LinkInfo* v5 = &m_links.m_data[linkIndex];
    for (int32_t i = 0; i < v5->m_numTargets; ++i) {
        hkpPoweredChainMapper::Target* v7 = &v4->m_targets.m_data[i + v5->m_firstTargetIdx];
        hkpConstraintMotor* v8 = v7->m_chain->m_infos.m_data[v7->m_infoIndex].m_motors[coordinateIndex];
        if (v8) {
            hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
            if (motorsOut.m_size == (motorsOut.m_capacityAndFlags & hkArrayBase<hkpConstraintMotor*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*Instance->m_heap, &motorsOut, sizeof(hkpConstraintMotor*));
            }
            v4 = this;
            motorsOut.m_data[motorsOut.m_size++] = v8;
        }
    }
}

// The motors are collected in a local hkArray (no storage, not ours). Each limit is divided by the motor count and
// converted to a float; every motor is a limited force motor and gets both. The count is tested against zero, then
// signed, and reloaded after each motor.
void hkpPoweredChainMapper::setForceLimits(int32_t linkIndex, int32_t coordinageIndex, float minForce, float maxForce) {
    hkArray<hkpConstraintMotor*> motors;
    getMotors(linkIndex, coordinageIndex, motors);
    if (motors.m_size != 0) {
        const float numMotors = static_cast<float>(motors.m_size);
        minForce = minForce / numMotors;
        maxForce = maxForce / numMotors;
        for (int32_t i = 0; i < motors.m_size; ++i) {
            hkpLimitedForceConstraintMotor* pMotor = static_cast<hkpLimitedForceConstraintMotor*>(motors.m_data[i]);
            pMotor->m_minForce = minForce;
            pMotor->m_maxForce = maxForce;
        }
    }
}

void hkpPoweredChainMapper::setMotors(int32_t linkIndex, int32_t coordinateIndex, hkpConstraintMotor* newMotor) {
    hkpPoweredChainMapper* pThis = this;
    int32_t v5 = 0;
    hkpPoweredChainMapper* v11 = pThis;
    hkpPoweredChainMapper::LinkInfo* linkIndexa = &pThis->m_links.m_data[linkIndex];
    hkpPoweredChainMapper::LinkInfo* v4 = linkIndexa;
    if (linkIndexa->m_numTargets > 0) {
        while (true) {
            hkpPoweredChainMapper::Target* m_data = pThis->m_targets.begin();
            int32_t v7 = v5 + v4->m_firstTargetIdx;
            hkpPoweredChainData* m_chain = m_data[v7].m_chain;
            hkpPoweredChainMapper::Target* v9 = &m_data[v7];
            hkReferencedObject* v10 = m_chain->m_infos.m_data[v9->m_infoIndex].m_motors[coordinateIndex];
            if (v10) {
                v10->removeReference();
                v4 = linkIndexa;
            }
            m_chain->m_infos.m_data[v9->m_infoIndex].m_motors[coordinateIndex] = newMotor;
            if (newMotor) {
                newMotor->addReference();
                v4 = linkIndexa;
            }
            if (++v5 >= v4->m_numTargets) {
                break;
            }
            pThis = v11;
        }
    }
}

void hkpPoweredChainMapper::setTargetOrientation(int32_t linkIndex, const hkQuaternion& newTarget) {
    hkpPoweredChainMapper::LinkInfo* v3 = &m_links.m_data[linkIndex];
    for (int32_t i = 0; i < v3->m_numTargets; ++i) {
        hkpPoweredChainMapper::Target* v5 = &m_targets.m_data[i + v3->m_firstTargetIdx];
        v5->m_chain->m_infos.m_data[v5->m_infoIndex].m_aTc = newTarget;
    }
}

// The SDK's buildChainMapper: a link for each of allConstraints and a powered chain for each pair of end bodies
// (hkpConstraintChainUtil's findConstraintLinkBetweenEntities, then buildPoweredChain); each link gets the run of
// m_targets naming the chain links made from its constraint and, with m_createLimitConstraints, a limit constraint
// converted from it. The constraints no chain uses go to unusedConstraints when it is given. A pair no chain joins, a
// chain that cannot be built or is not powered, or a chain constraint not in allConstraints warns, releases the
// mapper and gives null.
hkpPoweredChainMapper* hkpPoweredChainMapper::buildChainMapper(
    hkpPoweredChainMapper::Config config,
    const hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& allConstraints,
    const hkArray<hkpPoweredChainMapper::ChainEndpoints, hkContainerHeapAllocator>& pairs,
    hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>* unusedConstraints
) {
    // A chain link made from a constraint: the chain, the link's index in it and the index of the constraint's link in
    // the mapper.
    struct ChainTarget {
        hkpPoweredChainData* m_chain;
        int32_t m_infoIndex;
        int32_t m_linkIndex;
    };
    using ConstraintIndexMap = hkPointerMap<hkpConstraintInstance*, int32_t>;

    // EXE-INLINED 0x56D280: hkReferencedObject::operator new
    hkpPoweredChainMapper* pMemory = static_cast<hkpPoweredChainMapper*>(
        hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkpPoweredChainMapper))
    );
    pMemory->m_memSizeAndFlags = sizeof(hkpPoweredChainMapper);
    hkpPoweredChainMapper* pMapper = ::new (pMemory) hkpPoweredChainMapper;
    hkArray<ChainTarget> targets;
    // The number of chain links made from each constraint.
    hkArray<int32_t> numTargets;
    {
        const int32_t numConstraints = allConstraints.m_size;
        // EXE-INLINED none: hkArray<int>::setSize
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        numTargets._reserve(heap, numConstraints);
        numTargets.m_size = numConstraints;
        hkString::memSet(numTargets.m_data, 0, numConstraints * static_cast<int32_t>(sizeof(int32_t)));
    }
    ConstraintIndexMap constraintToIndex;
    for (int32_t i = 0; i < allConstraints.m_size; ++i) {
        constraintToIndex.insert(allConstraints.m_data[i], i);
    }
    {
        const int32_t numLinks = allConstraints.m_size;
        // EXE-INLINED none: hkArray<hkpPoweredChainMapper::LinkInfo>::setSize, with LinkInfo's constructor on the new
        // elements (MSVC's placement new tests the address)
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        pMapper->m_links._reserve(heap, numLinks);
        LinkInfo* pNewLink = pMapper->m_links.m_data + pMapper->m_links.m_size;
        for (int32_t c = numLinks - pMapper->m_links.m_size; c > 0; --c) {
            if (pNewLink != nullptr) {
                pNewLink->m_firstTargetIdx = -1;
                pNewLink->m_numTargets = 0;
                pNewLink->m_limitConstraint = nullptr;
            }
            ++pNewLink;
        }
        pMapper->m_links.m_size = numLinks;
        hkString::memSet(pMapper->m_links.m_data, 0, numLinks * static_cast<int32_t>(sizeof(LinkInfo)));
    }

    for (int32_t i = 0; i < pairs.m_size; ++i) {
        const ChainEndpoints& rPair = pairs.m_data[i];
        hkArray<hkpConstraintInstance*> chainConstraints;
        hkArray<hkpEntity*> chainEntities;
        bool failed = false;
        if (hkpConstraintChainUtil::findConstraintLinkBetweenEntities(
                allConstraints, rPair.m_start, rPair.m_end, chainEntities, chainConstraints
            ) == HK_FAILURE) {
            char buf[512];
            hkErrStream stream(buf, 512);
            stream << "Cannot find a chain of constraints linking one of the entity pairs.";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAAA88u), buf,
                ".\\Constraint\\Chain\\hkpPoweredChainMapper.cpp", 68
            );
            failed = true;
        }
        else {
            hkpConstraintChainInstance* pChain =
                hkpConstraintChainUtil::buildPoweredChain(chainConstraints, config.m_cloneMotors);
            if (pChain == nullptr) {
                char buf[512];
                hkErrStream stream(buf, 512);
                stream << "Failed to build a chain.";
                hkError::s_instance->message(
                    hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBADDAAu), buf,
                    ".\\Constraint\\Chain\\hkpPoweredChainMapper.cpp", 78
                );
                failed = true;
            }
            else {
                {
                    // EXE-INLINED none: hkArray<hkpConstraintChainInstance*>::pushBack
                    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                    if (pMapper->m_chains.m_size == pMapper->m_chains.getCapacity()) {
                        hkArrayUtil::_reserveMore(heap, &pMapper->m_chains, sizeof(hkpConstraintChainInstance*));
                    }
                    pMapper->m_chains.m_data[pMapper->m_chains.m_size] = pChain;
                    pMapper->m_chains.m_size++;
                }
                for (int32_t j = 0; j < chainConstraints.m_size; ++j) {
                    const int32_t it = constraintToIndex.findKey(chainConstraints.m_data[j]);
                    if (!constraintToIndex.isValid(it)) {
                        {
                            char buf[512];
                            hkErrStream stream(buf, 512);
                            stream << "Internal error.";
                            hkError::s_instance->message(
                                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA99DDu), buf,
                                ".\\Constraint\\Chain\\hkpPoweredChainMapper.cpp", 93
                            );
                        }
                        pChain->removeReference();
                        failed = true;
                        break;
                    }
                    const int32_t linkIndex = constraintToIndex.getValue(it);
                    LinkInfo* pLink = &pMapper->m_links.m_data[linkIndex];
                    // EXE-INLINED none: hkArray<ChainTarget>::expandOne
                    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                    if (targets.m_size == targets.getCapacity()) {
                        hkArrayUtil::_reserveMore(heap, &targets, sizeof(ChainTarget));
                    }
                    ChainTarget* pTarget = &targets.m_data[targets.m_size];
                    targets.m_size++;
                    pTarget->m_linkIndex = linkIndex;
                    numTargets.m_data[linkIndex]++;
                    if (pChain->m_data->getType() != hkpConstraintData::CONSTRAINT_TYPE_POWERED_CHAIN) {
                        {
                            char buf[512];
                            hkErrStream stream(buf, 512);
                            stream << "Internal error; invalid chain type.";
                            hkError::s_instance->message(
                                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA9D6Du), buf,
                                ".\\Constraint\\Chain\\hkpPoweredChainMapper.cpp", 108
                            );
                        }
                        pChain->removeReference();
                        failed = true;
                        break;
                    }
                    pTarget->m_chain = static_cast<hkpPoweredChainData*>(pChain->m_data);
                    pTarget->m_infoIndex = j;
                    if (config.m_createLimitConstraints.m_bool && pLink->m_limitConstraint == nullptr) {
                        pLink->m_limitConstraint = hkpConstraintUtils::convertToLimits(chainConstraints.m_data[j]);
                    }
                }
            }
        }
        if (failed) {
            pMapper->removeReference();
            return nullptr;
        }
    }

    // Each link's run of targets starts after the runs of the links before it.
    int32_t numAllTargets = 0;
    for (int32_t i = 0; i < pMapper->m_links.m_size; ++i) {
        pMapper->m_links.m_data[i].m_firstTargetIdx = numAllTargets;
        numAllTargets += numTargets.m_data[i];
        pMapper->m_links.m_data[i].m_numTargets = 0;
    }
    {
        // EXE-INLINED none: hkArray<hkpPoweredChainMapper::Target>::setSize (Target has no constructor)
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        pMapper->m_targets._reserve(heap, numAllTargets);
        pMapper->m_targets.m_size = numAllTargets;
    }
    for (int32_t t = 0; t < targets.m_size; ++t) {
        const ChainTarget& rTarget = targets.m_data[t];
        LinkInfo& rLink = pMapper->m_links.m_data[rTarget.m_linkIndex];
        const int32_t targetIndex = rLink.m_firstTargetIdx + rLink.m_numTargets;
        pMapper->m_targets.m_data[targetIndex].m_chain = rTarget.m_chain;
        pMapper->m_targets.m_data[targetIndex].m_infoIndex = rTarget.m_infoIndex;
        rLink.m_numTargets++;
    }
    if (unusedConstraints != nullptr) {
        for (int32_t i = 0; i < pMapper->m_links.m_size; ++i) {
            if (pMapper->m_links.m_data[i].m_numTargets == 0) {
                hkpConstraintInstance* const* ppConstraint = &allConstraints.m_data[i];
                // EXE-INLINED none: hkArray<hkpConstraintInstance*>::pushBack
                hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                if (unusedConstraints->m_size == unusedConstraints->getCapacity()) {
                    hkArrayUtil::_reserveMore(heap, unusedConstraints, sizeof(hkpConstraintInstance*));
                }
                unusedConstraints->m_data[unusedConstraints->m_size] = *ppConstraint;
                unusedConstraints->m_size++;
            }
        }
    }
    return pMapper;
}
