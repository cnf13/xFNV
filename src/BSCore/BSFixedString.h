#pragma once

// The pool the fixed strings share.
class BSStringPool {
public:
	// One pooled string's header; its three members are not declared further.
	struct Entry {
		void* unk00;
		void* unk04;
		void* unk08;
	};
};

// A string held as a pointer to a shared string pool entry.
class BSFixedString {
public:
	BSStringPool::Entry* pData;
};
