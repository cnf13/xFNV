#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"

class hkKdTreeBuilderSparseWriter;

// The inputs of the kd-tree builders.
class hkKdTreeBuildInput {
public:
    // The two buffers of projected entries the build sorts between.
    struct WorkingSet {
        hkKdTreeProjectedEntry* m_entriesIn;
        hkKdTreeProjectedEntry* m_entriesOut;
    };

    // The tree a build writes its nodes into.
    struct TreeStream {
        hkKdTreeNode* m_root;
    };

    // One step of the recursive build: the buffers holding its primitives, its node and the last node it may use, its
    // primitives' range, their quantised bounds, the writer and the axes on which an empty split is allowed.
    struct BuildRecursiveInput {
        WorkingSet* m_buffers;
        int32_t m_currentNodeIdx;
        int32_t m_maxNodeIdx;
        int32_t m_startPrimIdx;
        int32_t m_endPrimIdx;
        hkKdTreeProjectedEntry m_bounds;
        hkKdTreeBuilderSparseWriter* m_writer;
        hkVector4Comparison m_allowedEmptySplits;
    };

    // The primitives a kd-tree is built over: their count, ids and projected bounds.
    class Mediator : public hkReferencedObject {
    public:
        ~Mediator() override;

        virtual int32_t getNumPrimitives() const = 0;
        virtual hkUlong getPrimitiveId(int32_t primitiveIdx) const = 0;
        virtual void projectPrimitive(hkUlong primitiveId, hkKdTreeProjectedEntry& bounds) = 0;
    };
};
