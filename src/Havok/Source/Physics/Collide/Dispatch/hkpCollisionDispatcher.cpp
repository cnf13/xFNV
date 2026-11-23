#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdPointCollector.h"
#include "Havok/Source/Physics/Collide/Agent/Query/hkpLinearCastCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/Util/Null/hkpNullAgent.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionOutput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgentEntry.h"
#include "Havok/Source/Physics/Collide/Dispatch/Agent3Bridge/hkpAgent3Bridge.h"
#include "Havok/Source/Physics/Collide/Dispatch/ContactMgr/hkpContactMgrFactory.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "SDK/MSVC/string.h"

#include <cmath>

// The null agent3's empty destroy function, typed as the
// hkAgent3::DestroyFunc it is registered as.
static void nullAgentDestroy(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner,
    hkpCollisionDispatcher* dispatcher
);
static hkpAgentData* nullAgentProcess(
    const hkpAgent3ProcessInput& input,
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    hkVector4* separatingNormalOut,
    hkpProcessCollisionOutput& result
);
static hkpAgentData* nullAgentCreate(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData);
// hkpNullAgent's empty staticGetPenetrations, typed as the dispatcher's GetPenetrationsFunc.
static void nullGetPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
);
static void staticLinearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& castCollector,
    hkpCdPointCollector* startCollector
);
static void staticGetClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
);

static void nullAgentDestroy(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner,
    hkpCollisionDispatcher* dispatcher
) {
    ;
}

static hkpAgentData* nullAgentProcess(
    const hkpAgent3ProcessInput& input,
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    hkVector4* separatingNormalOut,
    hkpProcessCollisionOutput& result
) {
    return agentData;
}

static hkpAgentData* nullAgentCreate(const hkpAgent3Input& input, hkpAgentEntry* entry, hkpAgentData* agentData) {
    entry->m_streamCommand = 0;
    return agentData;
}

static void nullGetPenetrations(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
) {
    ;
}

static void staticLinearCast(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpLinearCastCollisionInput& input,
    hkpCdPointCollector& castCollector,
    hkpCdPointCollector* startCollector
) {
    ;
}

static void staticGetClosestPoints(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdPointCollector& collector
) {
    ;
}

hkpCollisionDispatcher::hkpCollisionDispatcher(
    CreateFunc defaultCreationFunction,
    hkpContactMgrFactory* defaultContactMgrFactory
)
  : m_defaultCollisionAgent(defaultCreationFunction), m_numAgent3Types(0), m_debugAgent2Table(nullptr),
    m_debugAgent2TablePred(nullptr), m_debugAgent3Table(nullptr), m_debugAgent3TablePred(nullptr) {
    for (int32_t i = 0; i < HK_MAX_AGENT2_TYPES; i++) {
        m_agent2Func[i].m_isFlipped.m_bool = false;
        m_agent2Func[i].m_isPredictive.m_bool = false;
    }
    m_collisionAgentRegistered.m_bool = false;
    m_checkEnabled.m_bool = true;
    m_midphaseAgent3Registered.m_bool = false;
    for (int32_t i = 0; i < 8; i++) {
        for (int32_t j = 0; j < 8; j++) {
            m_contactMgrFactory[i][j] = defaultContactMgrFactory;
            if (defaultContactMgrFactory) {
                defaultContactMgrFactory->addReference();
            }
        }
    }
    // Each shape type is its own alternate.
    for (int32_t i = 0; i < HK_MAX_SHAPE_TYPE; i++) {
        m_hasAlternateType[i] = 1u << i;
    }
    resetCreationFunctions();
    m_agent3Registered.m_bool = false;
}

hkpCollisionDispatcher::~hkpCollisionDispatcher() {
    disableDebugging();
    for (int32_t i = 0; i < 8; i++) {
        for (int32_t j = 0; j < 8; j++) {
            if (m_contactMgrFactory[i][j]) {
                m_contactMgrFactory[i][j]->removeReference();
            }
        }
    }
}

void hkpCollisionDispatcher::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
    int32_t m_capacityAndFlags = m_shapeInheritance.m_capacityAndFlags;
    if (m_capacityAndFlags >= 0 || collector->m_includeUnmanagedArrayMemory) {
        int32_t AllocatedSize = hkStatisticsCollector::getAllocatedSize(
            m_shapeInheritance.m_data,
            static_cast<int32_t>(sizeof(*m_shapeInheritance.m_data)) * m_capacityAndFlags
        );
        collector->addChunk(
            hkStatisticsCollector::MEM_ARRAY,
            "shapeInheritanceArray",
            m_shapeInheritance.m_data,
            static_cast<int32_t>(sizeof(*m_shapeInheritance.m_data)) * m_shapeInheritance.m_size,
            AllocatedSize
        );
    }
    for (int32_t i = 0; i < 8; i++) {
        for (int32_t j = 0; j < 8; j++) {
            collector->addReferencedObject("contactMgrFactory", m_contactMgrFactory[i][j], 0);
        }
    }
}

// The agent's functions go into the next agent2 entry, with AgentFuncs copied whole; the pair gets the agent3 bridge in
// the agent3 tables and the new agent2 entry in the agent2 tables, the continuous ones only for a predictive agent.
void hkpCollisionDispatcher::registerCollisionAgent(AgentFuncs& f, hkpShapeType typeA, hkpShapeType typeB) {
    m_agent2Func[m_numAgent2Types] = f;
    internalRegisterCollisionAgent(
        m_agent3Types,
        HK_AGENT_TYPE_BRIDGE,
        typeA,
        typeB,
        typeA,
        typeB,
        m_debugAgent3Table,
        0
    );
    internalRegisterCollisionAgent(m_agent2Types, m_numAgent2Types, typeA, typeB, typeA, typeB, m_debugAgent2Table, 0);
    if (f.m_isPredictive.m_bool) {
        internalRegisterCollisionAgent(
            m_agent3TypesPred,
            HK_AGENT_TYPE_BRIDGE,
            typeA,
            typeB,
            typeA,
            typeB,
            m_debugAgent3TablePred,
            0
        );
        internalRegisterCollisionAgent(
            m_agent2TypesPred,
            m_numAgent2Types,
            typeA,
            typeB,
            typeA,
            typeB,
            m_debugAgent2TablePred,
            0
        );
    }
    m_numAgent2Types++;
}

// The agent's functions go into the next agent2 entry, which is entered for the pair in the discrete table and, for a
// predictive agent, in the continuous one.
void hkpCollisionDispatcher::registerCollisionAgent2(AgentFuncs& f, hkpShapeType typeA, hkpShapeType typeB) {
    m_agent2Func[m_numAgent2Types] = f;
    internalRegisterCollisionAgent(m_agent2Types, m_numAgent2Types, typeA, typeB, typeA, typeB, m_debugAgent2Table, 0);
    if (f.m_isPredictive.m_bool) {
        internalRegisterCollisionAgent(
            m_agent2TypesPred,
            m_numAgent2Types,
            typeA,
            typeB,
            typeA,
            typeB,
            m_debugAgent2TablePred,
            0
        );
    }
    m_numAgent2Types++;
}

// KEEP: the exe builds the entry in a 16-aligned 0x50-byte local Agent3FuncsIntern and copies it into m_agent3Func
// whole (rep movsd, 0x14 dwords), and nothing writes the local's last 12 bytes (0x44-0x4F, after m_symmetric): stack
// bytes nothing wrote reach the dispatcher. The exe also fills the local's first 0x40 bytes from f (rep movsd, 0x10
// dwords), Agent3Funcs's tail padding 0x33-0x3F included (hkPredGskCylinderAgent3::registerAgent3 leaves it unwritten).
// For an asymmetric pair the flipped version is registered first, for typeB against typeA. The predictive flag is
// read from the local copy, the symmetry and reuse flags from f.
int32_t hkpCollisionDispatcher::registerAgent3(Agent3Funcs& f, hkpShapeType typeA, hkpShapeType typeB) {
    m_agent3Registered.m_bool = true;
    alignas(16) Agent3FuncsIntern f3;
    static_cast<Agent3Funcs&>(f3) = f;
    f3.m_symmetric = hkAgent3::IS_SYMMETRIC;
    if (typeA != typeB && !f.m_ignoreSymmetricVersion.m_bool) {
        f3.m_symmetric = hkAgent3::IS_NOT_SYMMETRIC_AND_FLIPPED;
        m_agent3Func[m_numAgent3Types] = f3;
        internalRegisterCollisionAgent(
            m_agent3Types,
            m_numAgent3Types,
            typeB,
            typeA,
            typeB,
            typeA,
            m_debugAgent3Table,
            0
        );
        if (f3.m_isPredictive.m_bool) {
            internalRegisterCollisionAgent(
                m_agent3TypesPred,
                m_numAgent3Types,
                typeB,
                typeA,
                typeB,
                typeA,
                m_debugAgent3TablePred,
                0
            );
        }
        m_numAgent3Types++;
        f3.m_symmetric = hkAgent3::IS_NOT_SYMMETRIC;
    }
    int32_t agentType;
    if (!f.m_reusePreviousEntry.m_bool) {
        agentType = m_numAgent3Types++;
        m_agent3Func[agentType] = f3;
    }
    else {
        agentType = m_numAgent3Types - 1;
    }
    internalRegisterCollisionAgent(m_agent3Types, agentType, typeA, typeB, typeA, typeB, m_debugAgent3Table, 0);
    if (f3.m_isPredictive.m_bool) {
        internalRegisterCollisionAgent(
            m_agent3TypesPred,
            agentType,
            typeA,
            typeB,
            typeA,
            typeB,
            m_debugAgent3TablePred,
            0
        );
    }
    return agentType;
}

// Removes an existing entry for the pair, updates the alternate-type bits and appends the pair.
void hkpCollisionDispatcher::registerAlternateShapeType(hkpShapeType primaryType, hkpShapeType alternateType) {
    for (int32_t i = 0; i < m_shapeInheritance.m_size; ++i) {
        if (m_shapeInheritance.m_data[i].m_primaryType == primaryType &&
            m_shapeInheritance.m_data[i].m_alternateType == alternateType) {
            m_shapeInheritance.removeAtAndCopy(i);
            i--;
        }
    }
    updateHasAlternateType(primaryType, alternateType, 0);
    ShapeInheritance& rEntry = m_shapeInheritance.expandOne();
    rEntry.m_alternateType = alternateType;
    rEntry.m_primaryType = primaryType;
}

void hkpCollisionDispatcher::setEnableChecks(hkBool checkEnabled) {
    m_checkEnabled = checkEnabled;
}

// Per shape type, a heading and one line per other type whose debug entry has a priority below 100: the predictive
// (continuous) table first, then the discrete one. Each heading's hkStringBuf is released after its row.
void hkpCollisionDispatcher::debugPrintTable() {
    hkError::s_instance->sectionBegin(0x5E4345E4, "hkpCollisionDispatcher::debugPrintTable");
    if (!m_debugAgent2Table || !m_debugAgent2TablePred) {
        return;
    }
    char acBuffer[512];
    char acLine[256];
    alignas(hkErrStream) uint8_t aStream[sizeof(hkErrStream)];
    for (int32_t i = 0; i < HK_MAX_SHAPE_TYPE; i++) {
        hkStringBuf kHeading(
            "\nEntries for (continuous)",
            hkGetShapeTypeName(hkpShapeType(i)),
            nullptr,
            nullptr,
            nullptr,
            nullptr
        );
        hkErrStream* pStream = ::new (aStream) hkErrStream(acBuffer, 512);
        *pStream << kHeading;
        hkError::s_instance
            ->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\Dispatch\\hkpCollisionDispatcher.cpp", 547);
        pStream->~hkErrStream();
        for (int32_t j = 1; j < HK_MAX_SHAPE_TYPE; j++) {
            const DebugEntry& rEntry = (*m_debugAgent2TablePred)[i][j];
            if (rEntry.m_priority < 100) {
                const char* pcName = hkGetShapeTypeName(hkpShapeType(j));
                const char* pcTypeA = hkGetShapeTypeName(hkpShapeType(rEntry.m_typeA));
                const char* pcTypeB = hkGetShapeTypeName(hkpShapeType(rEntry.m_typeB));
                hkString::snprintf(
                    acLine,
                    255,
                    "vs %30s <%i:%s-%s>",
                    pcName,
                    static_cast<int32_t>(rEntry.m_priority),
                    pcTypeA,
                    pcTypeB
                );
                pStream = ::new (aStream) hkErrStream(acBuffer, 512);
                *pStream << static_cast<const char*>(acLine);
                hkError::s_instance
                    ->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\Dispatch\\hkpCollisionDispatcher.cpp", 562);
                pStream->~hkErrStream();
            }
        }
    }
    for (int32_t i = 0; i < HK_MAX_SHAPE_TYPE; i++) {
        hkStringBuf kHeading(
            "\nEntries for (discrete)",
            hkGetShapeTypeName(hkpShapeType(i)),
            nullptr,
            nullptr,
            nullptr,
            nullptr
        );
        hkErrStream* pStream = ::new (aStream) hkErrStream(acBuffer, 512);
        *pStream << kHeading;
        hkError::s_instance
            ->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\Dispatch\\hkpCollisionDispatcher.cpp", 570);
        pStream->~hkErrStream();
        for (int32_t j = 1; j < HK_MAX_SHAPE_TYPE; j++) {
            const DebugEntry& rEntry = (*m_debugAgent2Table)[i][j];
            if (rEntry.m_priority < 100) {
                const char* pcName = hkGetShapeTypeName(hkpShapeType(j));
                const char* pcTypeA = hkGetShapeTypeName(hkpShapeType(rEntry.m_typeA));
                const char* pcTypeB = hkGetShapeTypeName(hkpShapeType(rEntry.m_typeB));
                hkString::snprintf(
                    acLine,
                    255,
                    "vs %30s <%i:%s-%s>",
                    pcName,
                    static_cast<int32_t>(rEntry.m_priority),
                    pcTypeA,
                    pcTypeB
                );
                pStream = ::new (aStream) hkErrStream(acBuffer, 512);
                *pStream << static_cast<const char*>(acLine);
                hkError::s_instance
                    ->message(hkError::MESSAGE_REPORT, -1, acBuffer, ".\\Dispatch\\hkpCollisionDispatcher.cpp", 585);
                pStream->~hkErrStream();
            }
        }
    }
    hkError::s_instance->sectionEnd();
}

void hkpCollisionDispatcher::initCollisionQualityInfo(hkpCollisionDispatcher::InitCollisionQualityInfo& input) {
    double v8;
    double v10;
    float inputc;
    float inputd;
    m_expectedMinPsiDeltaTime = input.m_minDeltaTime;
    hkpCollisionQualityInfo* v4 = &m_collisionQualityInfo[1];
    m_expectedMaxLinearVelocity = input.m_maxLinearVelocity;
    double v5 = input.m_gravityLength * 0.5 * input.m_minDeltaTime * input.m_minDeltaTime;
    m_collisionQualityInfo[1].m_enableToiWeldRejection.m_bool = input.m_enableToiWeldRejection.m_bool;
    float inputa = v5;
    m_collisionQualityInfo[1].m_keepContact = input.m_collisionTolerance;
    m_collisionQualityInfo[1].m_create4dContact = input.m_collisionTolerance;
    m_collisionQualityInfo[1].m_manifoldTimDistance = input.m_collisionTolerance;
    double v6 = inputa;
    if (input.m_enableNegativeManifoldTims.m_bool) {
        m_collisionQualityInfo[1].m_manifoldTimDistance = -2.0 * v6;
    }
    m_collisionQualityInfo[1].m_createContact = input.m_collisionTolerance;
    if (input.m_enableNegativeToleranceToCreateNon4dContacts.m_bool) {
        double v7 = v6 * -1.0;
        v8 = -1.0;
        m_collisionQualityInfo[1].m_createContact = v7;
    }
    else {
        v8 = -1.0;
    }
    m_collisionQualityInfo[1].m_useContinuousPhysics.m_storage = 0;
    m_collisionQualityInfo[1].m_maxContraintViolation = HK_REAL_MAX;
    m_collisionQualityInfo[1].m_useSimpleToiHandling.m_bool = 0;
    uint16_t m_defaultConstraintPriority = input.m_defaultConstraintPriority;
    v4->m_minSeparation = -5e+19f;
    v4->m_constraintPriority = m_defaultConstraintPriority;
    v4->m_minExtraSeparation = -5e+19f;
    v4->m_toiSeparation = -1e+19f;
    v4->m_toiExtraSeparation = -1e+19f;
    float inputb = std::fabs((-4.999999990253224e+18));
    v4->m_toiAccuracy = inputb;
    v4->m_minSafeDeltaTime = 1.0f;
    v4->m_minAbsoluteSafeDeltaTime = 1.0f;
    v4->m_minToiDeltaTime = 1.0f;
    m_collisionQualityInfo[7] = *v4;
    m_collisionQualityInfo[2] = *v4;
    m_collisionQualityInfo[3] = *v4;
    m_collisionQualityInfo[4] = *v4;
    m_collisionQualityInfo[5] = *v4;
    m_collisionQualityInfo[7].m_manifoldTimDistance = input.m_collisionTolerance;
    m_collisionQualityInfo[7].m_create4dContact = input.m_collisionTolerance;
    m_collisionQualityInfo[7].m_createContact = input.m_collisionTolerance;
    if (input.m_wantContinuousCollisionDetection.m_bool &&
        ((m_collisionQualityInfo[2].m_minSeparation = -0.5f,
          (m_collisionQualityInfo[2].m_minExtraSeparation = -0.5 /
               (input.m_numToisTillAllowedPenetrationSimplifiedToi - 1.0),
           (m_collisionQualityInfo[2].m_toiSeparation = -0.25f,
            (inputc = -0.3499999940395355 / (input.m_numToisTillAllowedPenetrationSimplifiedToi - 1.0),
             (m_collisionQualityInfo[2].m_toiExtraSeparation = inputc,
              (m_collisionQualityInfo[2].m_toiAccuracy = inputc * -0.30000001192092896,
               (m_collisionQualityInfo[2].m_maxContraintViolation = m_collisionQualityInfo[2].m_minExtraSeparation * v8,
                (m_collisionQualityInfo[2].m_minSafeDeltaTime = 5.0 / input.m_maxLinearVelocity,
                 (m_collisionQualityInfo[2].m_minAbsoluteSafeDeltaTime = 0.004999999888241291 /
                      input.m_maxLinearVelocity,
                  (m_collisionQualityInfo[2].m_minToiDeltaTime = m_collisionQualityInfo[2].m_minExtraSeparation * -2.0 /
                       input.m_maxLinearVelocity,
                   (m_collisionQualityInfo[2].m_constraintPriority = input.m_toiConstraintPriority,
                    (m_collisionQualityInfo[2].m_useContinuousPhysics.m_storage = 1,
                     (m_collisionQualityInfo[2].m_useSimpleToiHandling.m_bool = 1,
                      input.m_wantContinuousCollisionDetection.m_bool))))))))))))) &&
         (m_collisionQualityInfo[3].m_minSeparation = -0.5f,
          (m_collisionQualityInfo[3].m_minExtraSeparation = -0.5 / (input.m_numToisTillAllowedPenetrationToi - 1.0),
           (m_collisionQualityInfo[3].m_toiSeparation = -0.25f,
            (inputd = -0.3499999940395355 / (input.m_numToisTillAllowedPenetrationToi - 1.0),
             (m_collisionQualityInfo[3].m_toiExtraSeparation = inputd,
              (m_collisionQualityInfo[3].m_toiAccuracy = inputd * -0.30000001192092896,
               (m_collisionQualityInfo[3].m_maxContraintViolation = m_collisionQualityInfo[3].m_minExtraSeparation * v8,
                (m_collisionQualityInfo[3].m_minSafeDeltaTime = 5.0 / input.m_maxLinearVelocity,
                 (m_collisionQualityInfo[3].m_minAbsoluteSafeDeltaTime = 0.004999999888241291 /
                      input.m_maxLinearVelocity,
                  (m_collisionQualityInfo[3].m_minToiDeltaTime = m_collisionQualityInfo[3].m_minExtraSeparation * -2.0 /
                       input.m_maxLinearVelocity,
                   (m_collisionQualityInfo[3].m_constraintPriority = input.m_toiConstraintPriority,
                    (m_collisionQualityInfo[3].m_useContinuousPhysics.m_storage = 1,
                     input.m_wantContinuousCollisionDetection.m_bool)))))))))))))) {
        v10 = 0.004999999888241291;
        m_collisionQualityInfo[4].m_minSeparation = -0.5f;
        m_collisionQualityInfo[4].m_minExtraSeparation = -0.5 / (input.m_numToisTillAllowedPenetrationToiHigher - 1.0);
        m_collisionQualityInfo[4].m_toiSeparation = -0.25f;
        float inpute = -0.3499999940395355 / (input.m_numToisTillAllowedPenetrationToiHigher - 1.0);
        m_collisionQualityInfo[4].m_toiExtraSeparation = inpute;
        m_collisionQualityInfo[4].m_toiAccuracy = inpute * -0.30000001192092896;
        double v11 = m_collisionQualityInfo[4].m_minExtraSeparation * v8;
        m_collisionQualityInfo[4].m_maxContraintViolation = v11;
        m_collisionQualityInfo[4].m_minSafeDeltaTime = 5.0 / input.m_maxLinearVelocity;
        m_collisionQualityInfo[4].m_minAbsoluteSafeDeltaTime = 0.004999999888241291 / input.m_maxLinearVelocity;
        m_collisionQualityInfo[4].m_minToiDeltaTime = v11 / input.m_maxLinearVelocity;
        m_collisionQualityInfo[4].m_constraintPriority = input.m_toiHigherConstraintPriority;
        m_collisionQualityInfo[4].m_useContinuousPhysics.m_storage = 1;
    }
    else {
        v10 = 0.004999999888241291;
    }
    m_collisionQualityInfo[5].m_keepContact = input.m_collisionTolerance;
    if (input.m_wantContinuousCollisionDetection.m_bool) {
        m_collisionQualityInfo[5].m_minSeparation = -0.4f;
        m_collisionQualityInfo[5].m_minExtraSeparation = -0.5999999940395355 /
            (input.m_numToisTillAllowedPenetrationToiForced - 1.0);
        m_collisionQualityInfo[5].m_toiSeparation = -0.2f;
        float inputf = -0.41999998867511756 / (input.m_numToisTillAllowedPenetrationToiForced - 1.0);
        m_collisionQualityInfo[5].m_toiExtraSeparation = inputf;
        m_collisionQualityInfo[5].m_toiAccuracy = -0.30000001192092896 * inputf;
        double v12 = v8 * m_collisionQualityInfo[5].m_minExtraSeparation;
        m_collisionQualityInfo[5].m_maxContraintViolation = v12;
        m_collisionQualityInfo[5].m_minSafeDeltaTime = 5.0 / input.m_maxLinearVelocity;
        m_collisionQualityInfo[5].m_minAbsoluteSafeDeltaTime = v10 / input.m_maxLinearVelocity;
        m_collisionQualityInfo[5].m_minToiDeltaTime = v12 / input.m_maxLinearVelocity;
        m_collisionQualityInfo[5].m_constraintPriority = input.m_toiForcedConstraintPriority;
        m_collisionQualityInfo[5].m_useContinuousPhysics.m_storage = 1;
    }
    m_collisionQualityInfo[6] = m_collisionQualityInfo[5];
    m_collisionQualityInfo[6].m_manifoldTimDistance = input.m_collisionTolerance;
    m_collisionQualityInfo[6].m_createContact = input.m_collisionTolerance;
    std::memset(m_collisionQualityTable, 1, sizeof(m_collisionQualityTable));
    m_collisionQualityTable[0][0] = 0;
    m_collisionQualityTable[0][1] = 0;
    m_collisionQualityTable[0][9] = 3;
    m_collisionQualityTable[0][2] = 1;
    m_collisionQualityTable[0][3] = 2;
    m_collisionQualityTable[0][4] = 4;
    m_collisionQualityTable[0][5] = 5;
    m_collisionQualityTable[0][6] = 4;
    m_collisionQualityTable[0][8] = 6;
    m_collisionQualityTable[1][0] = 0;
    m_collisionQualityTable[1][1] = 0;
    m_collisionQualityTable[1][9] = 3;
    m_collisionQualityTable[1][2] = 1;
    m_collisionQualityTable[1][3] = 1;
    m_collisionQualityTable[1][4] = 3;
    m_collisionQualityTable[1][5] = 3;
    m_collisionQualityTable[1][6] = 3;
    m_collisionQualityTable[1][8] = 3;
    m_collisionQualityTable[9][0] = 3;
    m_collisionQualityTable[9][1] = 3;
    m_collisionQualityTable[9][9] = 3;
    m_collisionQualityTable[9][2] = 1;
    m_collisionQualityTable[9][3] = 1;
    m_collisionQualityTable[9][4] = 3;
    m_collisionQualityTable[9][5] = 3;
    m_collisionQualityTable[9][6] = 3;
    m_collisionQualityTable[9][8] = 3;
    m_collisionQualityTable[2][0] = 1;
    m_collisionQualityTable[2][1] = 1;
    m_collisionQualityTable[2][9] = 1;
    m_collisionQualityTable[2][2] = 1;
    m_collisionQualityTable[2][3] = 1;
    m_collisionQualityTable[2][4] = 1;
    m_collisionQualityTable[2][5] = 1;
    m_collisionQualityTable[2][6] = 3;
    m_collisionQualityTable[2][8] = 1;
    m_collisionQualityTable[3][0] = 2;
    m_collisionQualityTable[3][1] = 1;
    m_collisionQualityTable[3][9] = 1;
    m_collisionQualityTable[3][2] = 1;
    m_collisionQualityTable[3][3] = 1;
    m_collisionQualityTable[3][4] = 1;
    m_collisionQualityTable[3][5] = 1;
    m_collisionQualityTable[3][6] = 3;
    m_collisionQualityTable[3][8] = 1;
    m_collisionQualityTable[4][0] = 4;
    m_collisionQualityTable[4][1] = 3;
    m_collisionQualityTable[4][9] = 3;
    m_collisionQualityTable[4][2] = 1;
    m_collisionQualityTable[4][3] = 1;
    m_collisionQualityTable[4][4] = 1;
    m_collisionQualityTable[4][5] = 3;
    m_collisionQualityTable[4][6] = 3;
    m_collisionQualityTable[4][8] = 3;
    m_collisionQualityTable[5][0] = 5;
    m_collisionQualityTable[5][1] = 3;
    m_collisionQualityTable[5][9] = 3;
    m_collisionQualityTable[5][2] = 1;
    m_collisionQualityTable[5][3] = 1;
    m_collisionQualityTable[5][4] = 3;
    m_collisionQualityTable[5][5] = 3;
    m_collisionQualityTable[5][6] = 3;
    m_collisionQualityTable[5][8] = 3;
    m_collisionQualityTable[6][0] = 4;
    m_collisionQualityTable[8][2] = 1;
    m_collisionQualityTable[8][3] = 1;
    m_collisionQualityTable[6][1] = 3;
    m_collisionQualityTable[6][9] = 3;
    m_collisionQualityTable[6][2] = 3;
    m_collisionQualityTable[6][3] = 3;
    m_collisionQualityTable[6][4] = 3;
    m_collisionQualityTable[6][5] = 3;
    m_collisionQualityTable[6][6] = 3;
    m_collisionQualityTable[6][8] = 3;
    m_collisionQualityTable[8][0] = 6;
    m_collisionQualityTable[8][1] = 3;
    m_collisionQualityTable[8][9] = 3;
    m_collisionQualityTable[8][4] = 3;
    m_collisionQualityTable[8][5] = 3;
    m_collisionQualityTable[8][6] = 3;
    m_collisionQualityTable[8][8] = 3;
}

// Entry 0 of both function tables is the null agent (HK_AGENT_TYPE_NULL), which every type pair is reset to. The
// first loop rewrites the four debug tables only when the agent2 one exists (its pointer is reloaded per cell); after
// the bridge registers, the agent3 debug tables are reset again when they exist. The null agent3 entry leaves its
// flags as they were.
void hkpCollisionDispatcher::resetCreationFunctions() {
    m_numAgent2Types = 1;
    m_numAgent3Types = 1;
    for (int32_t i = 0; i < HK_MAX_SHAPE_TYPE; i++) {
        for (int32_t j = 0; j < HK_MAX_SHAPE_TYPE; j++) {
            if (m_debugAgent2Table) {
                m_debugAgent2Table[0][i][j].m_priority = 100;
                m_debugAgent2TablePred[0][i][j].m_priority = 100;
                m_debugAgent3Table[0][i][j].m_priority = 100;
                m_debugAgent3TablePred[0][i][j].m_priority = 100;
            }
            m_agent2Types[i][j] = 0;
            m_agent3Types[i][j] = 0;
            m_agent2TypesPred[i][j] = 0;
            m_agent3TypesPred[i][j] = 0;
        }
    }

    AgentFuncs& f2 = m_agent2Func[0];
    f2.m_createFunc = &hkpNullAgent::createNullAgent;
    f2.m_getPenetrationsFunc = &nullGetPenetrations;
    f2.m_getClosestPointFunc = &staticGetClosestPoints;
    f2.m_linearCastFunc = &staticLinearCast;
    f2.m_isFlipped.m_bool = false;
    f2.m_isPredictive.m_bool = true;

    Agent3FuncsIntern& f3 = m_agent3Func[0];
    f3.m_createFunc = &nullAgentCreate;
    f3.m_destroyFunc = &nullAgentDestroy;
    f3.m_cleanupFunc = nullptr;
    f3.m_removePointFunc = nullptr;
    f3.m_commitPotentialFunc = nullptr;
    f3.m_createZombieFunc = nullptr;
    f3.m_updateFilterFunc = nullptr;
    f3.m_calcStatisticsFunc = nullptr;
    f3.m_sepNormalFunc = nullptr;
    f3.m_invalidateTimFunc = nullptr;
    f3.m_warpTimeFunc = nullptr;
    f3.m_processFunc = &nullAgentProcess;
    f3.m_symmetric = hkAgent3::IS_SYMMETRIC;

    hkAgent3Bridge::registerAgent3(this);
    m_collisionAgentRegistered.m_bool = false;

    if (m_debugAgent3Table) {
        for (int32_t i = 0; i < HK_MAX_SHAPE_TYPE; i++) {
            for (int32_t j = 0; j < HK_MAX_SHAPE_TYPE; j++) {
                m_debugAgent3Table[0][i][j].m_priority = 100;
                m_debugAgent3TablePred[0][i][j].m_priority = 100;
            }
        }
    }
}

void hkpCollisionDispatcher::registerContactMgrFactory(
    hkpContactMgrFactory* fac,
    int32_t responseA,
    int32_t responseB
) {
    fac->addReference();
    hkpContactMgrFactory** v5 = &m_contactMgrFactory[responseB][responseA];
    (*v5)->removeReference();
    *v5 = fac;
    fac->addReference();
    int32_t v6 = responseB + 8 * responseA;
    hkpContactMgrFactory* v7 = m_contactMgrFactory[0][v6];
    hkpContactMgrFactory** v8 = &m_contactMgrFactory[0][v6];
    v7->removeReference();
    *v8 = fac;
}

void hkpCollisionDispatcher::internalRegisterCollisionAgent(
    uint8_t agentTypesTable[HK_MAX_SHAPE_TYPE][HK_MAX_SHAPE_TYPE],
    int32_t agentType,
    hkpShapeType typeA,
    hkpShapeType typeB,
    hkpShapeType origA,
    hkpShapeType origB,
    DebugTable* debugTable,
    int32_t depth
) {
    m_collisionAgentRegistered.m_bool = true;

    // Register the agent for the types that have typeA or typeB as their alternate type too, one level deeper.
    for (ShapeInheritance& si : m_shapeInheritance) {
        if (si.m_alternateType == typeA) {
            internalRegisterCollisionAgent(
                agentTypesTable,
                agentType,
                si.m_primaryType,
                typeB,
                origA,
                origB,
                debugTable,
                depth + 1
            );
        }
        if (si.m_alternateType == typeB) {
            internalRegisterCollisionAgent(
                agentTypesTable,
                agentType,
                typeA,
                si.m_primaryType,
                origA,
                origB,
                debugTable,
                depth + 1
            );
        }
    }

    int32_t startA = typeA;
    int32_t endA = typeA + 1;
    int32_t startB = typeB;
    int32_t endB = typeB + 1;
    int32_t priority = depth;
    if (typeA == HK_SHAPE_ALL) {
        startA = HK_FIRST_SHAPE_TYPE;
        endA = HK_MAX_SHAPE_TYPE;
        priority++;
    }
    if (typeB == HK_SHAPE_ALL) {
        startB = HK_FIRST_SHAPE_TYPE;
        endB = HK_MAX_SHAPE_TYPE;
        priority++;
    }

    for (int32_t a = startA; a < endA; ++a) {
        for (int32_t b = startB; b < endB; ++b) {
            agentTypesTable[a][b] = static_cast<uint8_t>(agentType);
            if (debugTable) {
                DebugEntry& entry = (*debugTable)[a][b];
                if (m_checkEnabled.m_bool && priority > entry.m_priority) {
                    // The warning text is formatted but its HK_WARN is compiled out. The names are looked up in this
                    // order: the entry's two types, then typeA and typeB.
                    char buf[1024];
                    const char* nameOldA = hkGetShapeTypeName(static_cast<hkpShapeType>(entry.m_typeA));
                    const char* nameOldB = hkGetShapeTypeName(static_cast<hkpShapeType>(entry.m_typeB));
                    const char* nameA = hkGetShapeTypeName(typeA);
                    const char* nameB = hkGetShapeTypeName(typeB);
                    hkString::snprintf(
                        buf,
                        1000,
                        "Agent handling types <%s-%s> would override more specialized agent <%s-%s>\nMaybe the order "
                        "of registering your collision agent is wrong, make sure you register your alternate type "
                        "agents first",
                        nameA,
                        nameB,
                        nameOldA,
                        nameOldB
                    );
                }
                entry.m_typeA = static_cast<char>(origA);
                entry.m_priority = static_cast<char>(priority);
                entry.m_typeB = static_cast<char>(origB);
            }
        }
    }
}

void hkpCollisionDispatcher::updateHasAlternateType(
    hkpShapeType primaryType,
    hkpShapeType alternateType,
    int32_t depth
) {
    m_hasAlternateType[primaryType] |= m_hasAlternateType[alternateType];
    for (int32_t i = 0; i < m_shapeInheritance.m_size; ++i) {
        hkpCollisionDispatcher::ShapeInheritance* m_data = m_shapeInheritance.begin();
        hkpShapeType m_alternateType = m_data[i].m_alternateType;
        if (m_alternateType == primaryType) {
            updateHasAlternateType(m_data[i].m_primaryType, m_alternateType, depth + 1);
        }
    }
}

void hkpCollisionDispatcher::disableDebugging() {
    hkpCollisionDispatcher::DebugEntry* v2 = (*m_debugAgent2Table)[0];
    if (v2) {
        hkMemoryRouter::getInstance().m_debug->blockFree(v2, sizeof(DebugTable));
        hkpCollisionDispatcher::DebugEntry* v4 = (*m_debugAgent2TablePred)[0];
        hkMemoryRouter::getInstance().m_debug->blockFree(v4, sizeof(DebugTable));
        hkpCollisionDispatcher::DebugEntry* v6 = (*m_debugAgent3Table)[0];
        hkMemoryRouter::getInstance().m_debug->blockFree(v6, sizeof(DebugTable));
        hkpCollisionDispatcher::DebugEntry* v8 = (*m_debugAgent3TablePred)[0];
        hkMemoryRouter::getInstance().m_debug->blockFree(v8, sizeof(DebugTable));
        m_debugAgent2Table = nullptr;
        m_debugAgent2TablePred = nullptr;
        m_debugAgent3Table = nullptr;
        m_debugAgent3TablePred = nullptr;
    }
}

void hkpCollisionDispatcher::registerContactMgrFactoryWithAll(hkpContactMgrFactory* fac, int32_t responseA) {
    hkpContactMgrFactory** v3 = m_contactMgrFactory[responseA];
    hkpContactMgrFactory** v4 = &m_contactMgrFactory[0][responseA];
    int32_t v5 = 8;
    do {
        fac->addReference();
        (*v4)->removeReference();
        *v4 = fac;
        fac->addReference();
        (*v3)->removeReference();
        *v3++ = fac;
        v4 += 8;
        --v5;
    }
    while (v5);
}
