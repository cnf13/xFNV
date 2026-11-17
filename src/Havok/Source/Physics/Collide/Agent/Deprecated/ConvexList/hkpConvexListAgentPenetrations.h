#pragma once

class hkpCdBody;
class hkpCdBodyPairCollector;
class hkpCollisionInput;

void staticGetPenetrationsFunc(
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpCdBodyPairCollector& collector
);
