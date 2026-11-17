#pragma once

class TESObjectCELL;

// A mixin for forms that live in a cell; it contributes only its vtable pointer and has no virtual destructor.
class TESChildCell {
public:
    TESChildCell() {}

    // the cell a save game files the form under
    virtual TESObjectCELL* GetSaveParentCell() = 0;
};
