#pragma once

#include "Reconstruction.hpp"

// The pool the fixed strings share (IDA names only its entry; not in the PDB).
class BSStringPool {
public:
	// One pooled string's header; not reconstructed further (IDA names the three words unk00, unk04 and unk08).
	struct Entry {
		void* unk00;
		void* unk04;
		void* unk08;
	};
};

// A string held as a pointer to a shared string pool entry. IDA name; not in the PDB.
class BSFixedString {
public:
	BSStringPool::Entry* pData;
};
