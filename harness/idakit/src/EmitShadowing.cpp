// Shadowing: src writes a member of `this` without `this->`, except where a local or parameter of the same name hides
// it. This records the names of the parameters and locals (all of them, the debug build's downcast locals Fields
// declares included: one more than needed only adds a `this->`): a member of `this` of one of those names keeps
// `this->` (whichever layer printed the member: a field, a call, a folded accessor's access). Without the pass nothing
// is recorded, and every member of `this` keeps `this->`, still correct.

#include "EmitPasses.hpp"

namespace idakit
{

void markShadowing(Function& function, Context&)
{
	function.shadowed.emplace();
	for (const Local& local : function.locals)
		function.shadowed->insert(local.name);
	for (const Function::Downcast& downcast : function.downcasts)
		function.shadowed->insert(downcast.name);
}

}  // namespace idakit
