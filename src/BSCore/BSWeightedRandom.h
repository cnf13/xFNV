#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BGSBodyPart;

// A set of results with weights from which one is picked at random, in proportion to its weight. The PDB places the
// instance over BGSBodyPart* in HitData.cpp; the exe's instance is per element type.
template <class T> class BSWeightedRandom {
public:
	BSWeightedRandom() : iTotalWeight(0) {}

	// Adds aResult with weight aiWeight: the weight to the total and to Weights, the result to Results.
	void AddWeightedResult(T aResult, uint32_t aiWeight);
	// Picks a result at random in proportion to the weights; null for an empty set.
	T PickResult();
	// Whether aResult is in Results.
	bool IsResultInSet(T aResult);

	// The results, parallel to Weights.
	BSSimpleArray<T, 1024> Results;
	BSSimpleArray<uint32_t, 1024> Weights;
	// The sum of the weights.
	uint32_t iTotalWeight;
};

// The members' generic bodies are in BSWeightedRandom.cpp, instantiated there for the exe's two instances.
extern template class BSWeightedRandom<BGSBodyPart*>;
extern template class BSWeightedRandom<char*>;
