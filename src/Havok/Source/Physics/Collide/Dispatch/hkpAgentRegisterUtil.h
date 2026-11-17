#pragma once

class hkpCollisionDispatcher;

// Registers Havok's collision agents with a dispatcher.
class hkpAgentRegisterUtil {
public:
    // Registers the agents for bounding volume trees and MOPPs, with the dispatcher's checks off.
    static void _registerBvTreeAgents(hkpCollisionDispatcher* dis);
    // Registers the agents for lists, collections, convex lists and height fields.
    static void _registerListAgents(hkpCollisionDispatcher* dis);
    // Registers the agents between two terminal (non-container) shapes.
    static void _registerTerminalAgents(hkpCollisionDispatcher* dis);
    // Registers every agent, in this order: the alternate shape types, the BV, multi-sphere and tree agents, the shape
    // collection agent with the dispatcher's checks off, then the list, transform, phantom and terminal agents.
    static void registerAllAgents(hkpCollisionDispatcher* dis);
};
