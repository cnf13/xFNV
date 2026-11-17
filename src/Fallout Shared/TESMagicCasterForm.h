#pragma once

class TESForm;

// An empty base: references of a base form deriving from it can cast magic.
class TESMagicCasterForm {
public:
    static bool CanBeMagicCaster(TESForm* apForm);
};
