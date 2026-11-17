#pragma once

class AudioMarker;
class TESObjectREFR;

// A music marker: its reference and its audio marker data.
class MusicMarker {
public:
	TESObjectREFR* pReference;
	AudioMarker* pMarker;
};
