#pragma once

#include "Reconstruction.hpp"

// A page of the debug text screens (the PDB's interface: GetNumSubPages, RenderPage and PrepareToRender, with
// DebugTextActor and the other pages deriving from it). The PC exe has none of the pages and no vtable of the
// class; DebugTextManager only keeps its map of page pointers, so src declares no members.
class IDebugTextPage {
public:
	IDebugTextPage() = delete;
};
