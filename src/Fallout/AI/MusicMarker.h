#pragma once

#include "Reconstruction.hpp"

class AudioMarker;
class TESObjectREFR;

// A music marker: its reference and its audio marker data. IDA's names; the PDB has no such class.
class MusicMarker {
public:
	TESObjectREFR* pReference;
	AudioMarker* pMarker;
};
