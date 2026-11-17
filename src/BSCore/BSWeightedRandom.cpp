#include "BSCore/BSWeightedRandom.h"

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/TESConditionFunctions.h"

// Walks the results, taking the first whose weight exceeds what is left of a random number below the total weight.
template <class T> T BSWeightedRandom<T>::PickResult() {
	T kResult = T();
	bool bResultFound = false;
	uint32_t uiRandom = RandomUnsignedInt(iTotalWeight);
	for (uint32_t i = 0; i < Results.iSize && !bResultFound; ++i) {
		if (uiRandom < Weights[i]) {
			kResult = Results[i];
			bResultFound = true;
		}
		uiRandom -= Weights[i];
	}
	return kResult;
}

template <class T> void BSWeightedRandom<T>::AddWeightedResult(T aResult, uint32_t aiWeight) {
	iTotalWeight += aiWeight;
	Weights.Add(aiWeight);
	Results.Add(aResult);
}

template <class T> bool BSWeightedRandom<T>::IsResultInSet(T aResult) {
	return Results.IsInArray(aResult);
}

// The two explicit instantiations; the header declares them extern.
template class BSWeightedRandom<BGSBodyPart*>;
template class BSWeightedRandom<char*>;
