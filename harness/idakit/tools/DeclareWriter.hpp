// idakit declare's writer of a declaration and its `// Not reconstructed yet.` forwarder (DeclareCommand.cpp's
// Writer), for the declare modes outside DeclareCommand.cpp: declare --hk-symbols (DeclareHavok.cpp) declares a
// function on the class it moves to through it, and rewrites one in place with what its header and .cpp need.
#pragma once

#include "idakit/SrcEdit.hpp"
#include "idakit/SrcModel.hpp"
#include "idakit/SrcTypes.hpp"

#include <cstdint>
#include <memory>

namespace idakit
{

class DeclarationWriter
{
public:
	DeclarationWriter(const SrcModel& model, SrcTypes& types, FileEdits& edits);
	~DeclarationWriter();

	// A new declaration (outcome `declared`): in its owner's class (at flush) or at the end of d.header, its
	// forwarder at the end of d.source, with the includes and forward declarations they need. Throws Skip before
	// any edit.
	void add(const Decl& d);
	// A declaration rewritten where it is: every type it names one src has, the includes and forward declarations
	// d.header needs for it before line `before`, the includes d.source needs for its forwarder (none for an empty
	// source). Throws Skip before any edit.
	void inPlace(const Decl& d, int64_t before);
	// the class blocks add() collected, and each header's forward declarations
	void flush();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

}  // namespace idakit
