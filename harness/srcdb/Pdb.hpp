// The PDB's share of the src model: the functions of the Xbox 360 debug PDB (env/cache/Fallout_Debug.pdb, repacked
// by harness/tools/pdb_repack.py) with their signatures, parameter names and variables, its records' data members
// and its global data, for typing the exe functions src does not declare and for telling which src declarations have
// the PDB's types. The PDB is another build for another platform: its signatures are evidence that idakit sync
// checks against the exe before it uses one, never a layout.
//
// Rows (Model.hpp): pdb_function (name, class: the owner of a member function, type: an fn key whose parameters
// exclude `this`, static, code_size), pdb_param (names), pdb_local, pdb_field, pdb_global; its types join the type
// table under the model's keys. Its template instantiations' names are what the index's instances TU instantiates
// (Index.cpp).
#pragma once

#include "Store.hpp"

#include <string>
#include <vector>

namespace srcdb
{

// Adds the PDB's rows to the model; returns the number of functions. Throws Error when the PDB cannot be read.
size_t indexPdb(Store& store, const std::string& path);

// The names of the PDB's class, struct and union instantiations of templates (`NiTArray<NiPointer<NiAVObject>,
// NiTNewInterface<NiPointer<NiAVObject> > >`, allocators included), each once, as the PDB spells them. Throws Error
// when the PDB cannot be read.
std::vector<std::string> pdbTemplateInstances(const std::string& path);

}  // namespace srcdb
