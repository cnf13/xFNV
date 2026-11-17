#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

class hkpBallSocketChainData;
class hkpConstraintChainInstance;
class hkpConstraintInstance;
class hkpConstraintMotor;
class hkpEntity;

// Static constraint-chain construction utilities; no instance state.
class hkpConstraintChainUtil {
public:
    // Builds an hkpPoweredChainData and an hkpConstraintChainInstance from the constraints' pivots and motors, cloning
    // the motors when cloneMotors is set; returns null on failure.
    static hkpConstraintChainInstance*
    buildPoweredChain(hkArray<hkpConstraintInstance*>& constraints, hkBool cloneMotors);
    // Havok SDK signature (hkpUtilities.lib, tied by bytes).
    static void addConstraintToBallSocketChain(
        hkpConstraintChainInstance* instance,
        hkpEntity* entityToAppend,
        const hkVector4& pivotWs
    );
    // Havok SDK signature (hkpUtilities.lib, tied by bytes).
    static void addConstraintToPoweredChain(
        hkpConstraintChainInstance* instance,
        hkpEntity* entityToAppend,
        const hkVector4& pivotWs,
        hkpConstraintMotor* motor0,
        hkpConstraintMotor* motor1,
        hkpConstraintMotor* motor2
    );
    // Havok SDK signature (hkpUtilities.lib, tied by bytes).
    static void getAdjointEntities(
        const hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& allConstraints,
        const hkpEntity* entity,
        hkArray<hkpEntity*, hkContainerHeapAllocator>& entitiesOut,
        hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& constraintsOut
    );
    // Havok SDK signature (hkpUtilities.lib, tied by reloc,string): a breadth-first search from both ends for the
    // bodies and constraints linking entityA to entityB.
    static hkResult findConstraintLinkBetweenEntities(
        const hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& allConstraints,
        hkpEntity* entityA,
        hkpEntity* entityB,
        hkArray<hkpEntity*, hkContainerHeapAllocator>& entitiesOut,
        hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& constraintsOut
    );
    // Adds the constraint's link to the ball-and-socket chain at the end, or at the front, of the chain that holds one
    // of its entities; HK_FAILURE when its pivots cannot be read or neither entity is an end of the chain.
    static hkResult addConstraintToChain(
        hkpConstraintInstance* newChildInstance,
        hkpConstraintChainInstance* chainInstance,
        hkpBallSocketChainData* chainData
    );
};
