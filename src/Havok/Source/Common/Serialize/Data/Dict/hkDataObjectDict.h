#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/StringMap/hkStringMap.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObjectImpl.h"
#include "Havok/Source/Common/Serialize/Data/hkDataWorld.h"
#include "Havok/Source/Common/Serialize/Util/hkSerializeMultiMap.h"

#include <stdint.h>

class hkDataClassDict;
class hkDataObjectDict;
class hkDataWorldDict;
class StructArrayImplementation;

int32_t RTC_NumErrors_2();
hkDataObject::Type getVecTypeFromNumReals(int32_t r_eax);

// A dictionary member's value or default, read by the member's type (an integer, a pointer, a real, a string, an
// object, an array or a vector's reals).
union Union {
	int64_t i;
	void* p;
	float r;
	char* s;
	hkDataObjectImpl* o;
	hkDataArrayImpl* a;
	float* ra;
};

// An interned string that holds a reference count. Declared at file scope in this header because hkDataClassDict's
// fields need it.
class InternedStringRefCounted : public hkReferencedObject {
public:
	InternedStringRefCounted() = default;
	// One reference, on the interned string apString.
	InternedStringRefCounted(const char* apString) : m_cachedString(apString) {}

	const char* getString() const { return m_cachedString; }

	const char* m_cachedString;
};

// A reference to an interned string.
struct InternedStringHandle {
	const char* getString() const { return m_cached->getString(); }

	hkRefPtr<InternedStringRefCounted> m_cached;
};

// An interned name: the one copy of a string the world's tracker hands out, so that two names are equal when their
// pointers are. Callers hold an interned name as a const char*, so the conversion is implicit here.
struct InternedString {
	InternedString() : m_cachedString(nullptr) {}
	InternedString(const char* apString) : m_cachedString(apString) {}

	bool operator==(const InternedString& arOther) const { return m_cachedString == arOther.m_cachedString; }
	bool operator!=(const InternedString& arOther) const { return m_cachedString != arOther.m_cachedString; }
	const char* getString() const { return m_cachedString; }

	const char* m_cachedString;
};

// The dictionary world: the classes and objects a tagfile load builds.
class hkDataWorldDict : public hkDataWorld {
public:
	// The world's classes, objects and interned strings.
	class ObjectTracker;

	// A world with a new, empty tracker.
	hkDataWorldDict();
	~hkDataWorldDict() override;

	hkDataClassImpl* newClass(const hkDataClass::Cinfo& cinfo) override;
	// A new hkDataObjectDict of the class.
	hkDataObjectImpl* newObject(const hkDataClass& klass) const override;
	// A new array of the member's type, sized to the member's tuple count for a tuple, and assigned to the
	// object's member.
	hkDataArrayImpl* newArray(hkDataObject& obj, const hkDataClass::MemberInfo& minfo) const override;
	hkEnum<hkDataWorld::DataWorldType, int32_t> getType() const override;
	// The tracker's top-level object.
	hkDataObject getContents() const override;
	// Every tracked class.
	void findAllClasses(hkArray<hkDataClassImpl*>& classesOut) const override;
	// The tracked class called name, or null.
	hkDataClassImpl* findClass(const char* name) const override;
	// The object of handle.p0.
	hkDataObject findObject(const hkDataObject::Handle& handle) const override;

	// Renames klass's member oldName to newName in the class and in every object and struct array of the class.
	void renameClassMember(hkDataClass& klass, const char* oldName, const char* newName);
	// Removes klass's member name from every object and struct array of the class, then from the class.
	void removeClassMember(hkDataClass& klass, const char* name);
	// The class called name, copied from worldFrom with the classes of its members first when this world lacks it,
	// else this world's.
	hkDataClassImpl* copyClassFromWorld(const char* name, const hkDataWorld& worldFrom);
	void findObjectsByExactClass(const char* className,
		hkArray<hkDataObjectImpl*, hkContainerHeapAllocator>& objectsOut);
	void findObjectsByBaseClass(const char* className,
		hkArray<hkDataObjectImpl*, hkContainerHeapAllocator>& objectsOut);
	void castObject(hkDataObject& obj, const hkDataClass& castClass);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void setClassVersion(hkDataClass& klass, int32_t newVersion);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void renameClass(hkDataClass& klass, const char* newName);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void setClassParent(hkDataClass& klass, hkDataClass& parent);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void addClassMember(hkDataClass& klass, const char* name, hkDataObject::Type type, const char* typeName,
		int32_t tupleCount);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	hkDataClassImpl* wrapClass(const hkClass& klass);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void removeClass(hkDataClass& klass);

	// A new array of type, of klass for the struct and object kinds, with tupleCount elements per element.
	static hkDataArrayImpl* createArray(hkDataObject::Type type, const hkDataClassImpl* klass, int32_t tupleCount);

	ObjectTracker* m_tracker;
};

// A serialized class built at load time: its members and enums held in arrays.
class hkDataClassDict : public hkDataClassImpl {
public:
	// An enum the class declares: its interned name and its items.
	struct Enum;
	// One declared member.
	struct MemberInfo;

	// The class name, interned through root's tracker; no parent, enums or members.
	hkDataClassDict(hkDataWorldDict* root, const char* name, int32_t version);
	~hkDataClassDict() override;

	// m_world.
	hkDataWorldDict* getWorld() const override { return m_world; }
	// The interned name's text.
	const char* getName() const override;
	// m_version.
	int32_t getVersion() const override { return m_version; }
	// m_parent.
	hkDataClassImpl* getParent() const override { return m_parent; }
	// Whether k is this class or one of its descendants.
	hkBool isSuperClass(const hkDataClassImpl* k) const override;
	// The size of m_memberInfo.
	int32_t getNumDeclaredMembers() const override { return m_memberInfo.getSize(); }
	// Member i's description: its name, this class, its type and tuple count, and its class by name.
	void getDeclaredMemberInfo(int32_t i, hkDataClass::MemberInfo& info) const override;
	// The index of the declared member called name, or -1.
	int32_t getDeclaredMemberIndexByName(const char* nameIn) const override;
	int32_t getNumMembers() const override;
	// Member i among all members, the ancestors' first.
	void getMemberInfo(int32_t memberIndex, hkDataClass::MemberInfo& info) const override;
	// The index of the member called name among all members, or -1.
	int32_t getMemberIndexByName(const char* nameIn) const override;

	// The member called name (interned), the ancestors' included, or null.
	const MemberInfo* getMemberInfoByName(InternedString name) const;
	// Drops the declared member called name (interned).
	hkResult removeMember(InternedString name);
	// Stores into result the interned copy of name in the world's tracker (m_world->m_tracker's m_interns, made there
	// on first use), or null for a null name. Its one caller is StructArrayImplementation::Object::intern.
	const char** intern(const char** result, const char* name) const;
	// Appends the declared member {name, typeName, no default, type, tupleCount} to
	// m_memberInfo (+0x28, 0x18-byte elements), typeName's string counted once more.
	// hkDataWorldDict::newClass passes the tracker's handle for the cinfo member's class name.
	void addMember(InternedString name, hkDataObject::Type type, const InternedStringHandle& typeName,
		int32_t tupleCount);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void selfDestruct();

	static hkDataClassDict* Construct(void* apMemory, hkDataWorldDict* root, const char* name, int32_t version);

	hkDataWorldDict* m_world;
	InternedStringHandle m_name;
	int32_t m_version;
	hkDataClassDict* m_parent;
	hkArray<Enum*> m_enums;
	hkArray<MemberInfo> m_memberInfo;
	hkArray<MemberInfo> m_hiddenMemberInfo;
};

// One declared member of an hkDataClassDict.
struct hkDataClassDict::MemberInfo {
	// Interned in the world's tracker, so members compare by pointer.
	InternedString m_name;
	// The class of an object or struct member, by its interned name.
	InternedStringHandle m_typeName;
	// The default value.
	Union m_default;
	hkDataObject::Type m_type;
	int32_t m_tupleCount;
};

// An enum an hkDataClassDict declares (hkserialize.pdb's record; the SDK ships no header for it).
struct hkDataClassDict::Enum {
	// One enumerator: its interned name and value.
	struct Item {
		InternedString name;
		int32_t value;
	};

	InternedString m_name;
	hkArray<Item> m_items;
};

// The dictionary world's bookkeeping: its classes by name, the objects and arrays of each class, and the strings
// every name is interned into.
class hkDataWorldDict::ObjectTracker {
public:
	// Empty maps and no top-level object.
	ObjectTracker();
	// Releases every tracked object and class and the interned names; the containers free themselves.
	~ObjectTracker();

	// The interned copy of name, made on first use; null for a null name.
	InternedString intern(const char* name) {
		if (!name) {
			return InternedString();
		}
		char* pInterned = m_interns.getWithDefault(name, nullptr);
		if (!pInterned) {
			pInterned = hkString::strDup(name);
			m_interns.insert(pInterned, pInterned);
		}
		return InternedString(pInterned);
	}

	// Appends every tracked class to classesOut.
	void findTrackedClasses(hkArray<hkDataClassImpl*>& classesOut) const;
	// Appends the tracked objects of the class called className, and with baseClass those of the classes derived from
	// it, to objectsOut.
	void findTrackedObjectsByBase(const char* className, int32_t baseClass, int32_t addStructs,
		hkArray<hkDataObjectImpl*>& objectsOut) const;
	// Appends the tracked struct arrays of c to arraysOut.
	void getTrackedStructArrays(hkDataClassDict* c, int32_t baseClass, hkArray<hkDataArrayImpl*>& arraysOut);
	// Moves the class and its interned name handle from oldName to newName (interned here).
	void retrackRenamedClass(const char* oldName, const char* newName);
	// Removes every tracked object of klass and of the classes derived from it.
	void removeClassContent(hkDataClassDict* klass);
	// Writes name's interned copy into *result.
	char** intern(char** result, const char* name);
	// The handle of name's interned class name, made on first use; the shared empty handle for a null name.
	InternedStringHandle* internClassNameHandle(InternedStringHandle* result, const char* name);
	// Moves klass from oldParent's derived classes to those of its current parent.
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	void retrackDerivedClass(hkDataClassDict* oldParent, hkDataClassDict* klass);
	// Frees every tracked class name and empties m_classes.
	void clear();
	// Tracks klass by name and under its parent.
	void trackClass(hkDataClassDict* klass);
	// Drops klass from the tracker's maps.
	void untrackClass(hkDataClassDict* klass);
	// Moves obj from its class's tracked objects to newClass's.
	void retractCastedObject(hkDataObjectDict* obj, hkDataClassDict* newClass);

	hkStringMap<hkDataClassDict*> m_classes;
	hkSerializeMultiMap<hkDataClassDict*, hkDataClassDict*> m_derivedFromParent;
	hkDataObjectDict* m_topLevelObject;
	hkSerializeMultiMap<hkDataClassDict*, hkDataObjectDict*> m_objectsFromClass;
	hkStringMap<char*> m_interns;
	hkStringMap<InternedStringRefCounted*> m_internedClassNames;
	hkSerializeMultiMap<hkDataClassDict*, StructArrayImplementation*> m_arraysFromClass;
};

// An array of plain values of type T, tagged TYPE. Each instantiation's reserve and setSize use hkArrayUtil::_reserve
// with sizeof(T).
template <typename T, int32_t TYPE> class BasicArrayImplementation : public hkDataArrayImpl {
public:
	hkDataObject::Type getType() const override { return (hkDataObject::Type)TYPE; }
	void clear() override;
	void reserve(int32_t n) override;
	void setSize(int32_t n) override;
	int32_t getSize() const override { return m_data.m_size; }

	hkArray<T> m_data;
};
// For every T: clear sets the size to 0; reserve grows the array as hkArray::reserve grows it (at least doubled) and
// zeroes the new elements.
template <typename T, int32_t TYPE> void BasicArrayImplementation<T, TYPE>::clear() {
	m_data.m_size = 0;
}

template <typename T, int32_t TYPE> void BasicArrayImplementation<T, TYPE>::reserve(int32_t n) {
	m_data.reserve(n);
}

template <typename T, int32_t TYPE> void BasicArrayImplementation<T, TYPE>::setSize(int32_t n) {
	m_data.reserve(n);
	for (int32_t i = m_data.m_size; i < n; ++i) {
		m_data.m_data[i] = T();
	}
	m_data.m_size = n;
}
// The array of bytes.
class ByteArrayImplementation : public BasicArrayImplementation<uint8_t, hkDataObject::TYPE_BYTE> {
public:
	~ByteArrayImplementation() override;

	int32_t asInt(int32_t i) const override;
	void setInt(int32_t i, int32_t val) override;
	// Copies n bytes from v, growing the array.
	void setAll(const uint8_t* vals, int32_t n) override;
};

// The array of reals.
class RealArrayImplementation : public BasicArrayImplementation<float, hkDataObject::TYPE_REAL> {
public:
	~RealArrayImplementation() override;

	float asReal(int32_t i) const override;
	void setReal(int32_t i, float val) override;
	// Copies n reals from v, growing the array.
	void setAll(const float* vals, int32_t n) override;
};

// The array of vectors: m_numReal reals per element, stored contiguously.
class VecArrayImplementation : public hkDataArrayImpl {
public:
	// An empty array of numRealPerElem reals per element.
	VecArrayImplementation(int32_t numRealPerElem);
	~VecArrayImplementation() override;

	const float* asVec(int32_t index) const override;
	void setVec(int32_t index, const float* val) override;
	hkDataObject::Type getType() const override;
	void clear() override;
	// Reserves n * m_numReal reals.
	void reserve(int32_t n) override;
	// Resizes to n * m_numReal reals.
	void setSize(int32_t n) override;
	int32_t getSize() const override;

	hkArray<float> m_data;
	int32_t m_numReal;
};

// The array of object pointers, each holding a reference.
class PointerArrayImplementation : public BasicArrayImplementation<hkDataObjectImpl*, hkDataObject::TYPE_OBJECT> {
public:
	// An empty array of objects of class a1, holding a reference on it.
	PointerArrayImplementation(const hkDataClassImpl* a1);
	~PointerArrayImplementation() override;

	// m_class.
	const hkDataClassImpl* getClass() const override { return m_class; }
	hkDataObjectImpl* asObject(int32_t i) const override;
	void setObject(int32_t i, hkDataObjectImpl* val) override;

	hkDataClassDict* m_class;
};

// The array of 64-bit integers.
class IntArrayImplementation : public BasicArrayImplementation<int64_t, hkDataObject::TYPE_INT> {
public:
	~IntArrayImplementation() override;

	int32_t asInt(int32_t i) const override;
	void setInt(int32_t i, int32_t val) override;
	int64_t asInt64(int32_t i) const override;
	void setInt64(int32_t i, int64_t val) override;
	// Each copies n values from v, widened, growing the array. The signed and unsigned 32- and
	// 64-bit overloads and int8/char share their bodies.
	void setAll(const hkBool* v, int32_t n) override;
	void setAll(const char* v, int32_t n) override;
	void setAll(const int8_t* v, int32_t n) override;
	void setAll(const uint8_t* v, int32_t n) override;
	void setAll(const int16_t* v, int32_t n) override;
	void setAll(const uint16_t* v, int32_t n) override;
	void setAll(const int32_t* v, int32_t n) override;
	void setAll(const uint32_t* v, int32_t n) override;
	void setAll(const int64_t* v, int32_t n) override;
	void setAll(const uint64_t* v, int32_t n) override;
};

// The array of strings, each a copy the array owns.
class CstringArrayImplementation : public BasicArrayImplementation<char*, hkDataObject::TYPE_CSTRING> {
public:
	~CstringArrayImplementation() override;

	const char* asString(int32_t i) const override;
	void setString(int32_t i, const char* val) override;
};

// An array of tuples: m_tupleCount consecutive elements of m_impl per element. asArray hands out m_view, a window
// onto one tuple.
class ArrayOfTuplesImplementation : public hkDataArrayImpl {
public:
	// One tuple of the owner: element i is the owner's element m_baseIndex + i.
	struct View : public hkDataArrayImpl {
		~View() override;

		hkDataObject::Type getType() const override;
		const hkDataClassImpl* getClass() const override;
		void clear() override;
		// A tuple keeps its size.
		void reserve(int32_t n) override {}
		void setSize(int32_t n) override {}
		int32_t getSize() const override;
		const float* asVec(int32_t index) const override;
		void setVec(int32_t index, const float* val) override;
		const char* asString(int32_t index) const override;
		void setString(int32_t index, const char* val) override;
		float asReal(int32_t index) const override;
		void setReal(int32_t index, float val) override;
		int32_t asInt(int32_t index) const override;
		void setInt(int32_t index, int32_t val) override;
		int64_t asInt64(int32_t index) const override;
		void setInt64(int32_t index, int64_t val) override;
		hkDataObjectImpl* asObject(int32_t index) const override;
		void setObject(int32_t index, hkDataObjectImpl* val) override;
		hkDataArrayImpl* asArray(int32_t index) const override;
		void setArray(int32_t index, hkDataArrayImpl* val) override;
		void setAll(const uint8_t* v, int32_t n) override;
		void setAll(const float* v, int32_t n) override;

		ArrayOfTuplesImplementation* m_owner;
		int32_t m_baseIndex;
	};

	ArrayOfTuplesImplementation(hkDataArrayImpl* impl, int32_t tupleCount) : m_impl(impl), m_tupleCount(tupleCount) {
		++m_impl->m_count;
		++m_view.m_count;
		m_view.m_owner = this;
	}

	~ArrayOfTuplesImplementation() override;

	hkDataObject::Type getType() const override;
	const hkDataClassImpl* getClass() const override;
	void clear() override;
	void reserve(int32_t n) override;
	void setSize(int32_t n) override;
	int32_t getSize() const override;
	hkDataArrayImpl* asArray(int32_t idx) const override;
	// Copies the array val into tuple index, by element type.
	void setArray(int32_t idx, hkDataArrayImpl* src) override;

	// mutable: asArray is const and points the view at the tuple.
	mutable View m_view;
	hkDataArrayImpl* m_impl;
	int32_t m_tupleCount;
};

// An array of structs, stored by member: one value array per member name.
class StructArrayImplementation : public hkDataArrayImpl {
public:
	// A member's name and its values.
	struct MemberData {
		const char* m_memberName;
		hkDataArrayImpl* m_values;
	};

	// An object view of one array element.
	struct Object : public hkDataObjectImpl {
		Object(StructArrayImplementation* impl, int32_t index) : m_impl(impl), m_index(index) {}

		hkDataObject::Handle getHandle() const override;
		const hkDataClassImpl* getClass() const override;
		hkDataObject::Value accessByName(const char* name) override;
		int32_t hasMember(const char* name) const override;
		int32_t getMemberIterator() const override;
		int32_t isValid(int32_t it) const override;
		int32_t getNextMember(int32_t it) const override;
		const char* getMemberName(int32_t it) const override;
		const hkDataObject::Value getMemberValue(int32_t it) const override;
		// A member's handle is its interned name: these take the const char* itself.
		hkDataArrayImpl* asArray(const char* name) override;
		const char* asString(const char* name) override;
		int64_t asInt(const char* name) override;
		hkDataObjectImpl* asObject(const char* name) override;
		const float* asVec(const char* nameIn, int32_t n) override;
		float asReal(const char* name) override;

		virtual const char** intern(const char** result, const char* name) const;
		virtual void selfDestruct();
		virtual hkHalf asHalf(const char* name);
		virtual void assign(const char* name, int8_t value);
		virtual void assign(const char* name, int16_t value);
		virtual void assign(const char* name, int32_t value);
		void assign(const char* handle, const hkDataObject::Value& value) override;
		void assign(const char* handle, hkDataObjectImpl* value) override;
		void assign(const char* handle, hkDataArrayImpl* value) override;
		void assign(const char* handle, const char* value) override;
		void assign(const char* handle, float value) override;
		void assign(const char* handle, hkHalf value) override;
		void assign(const char* handle, const float* value, int32_t nreal) override;
		void assign(const char* handle, int64_t value) override;

		StructArrayImplementation* m_impl;
		int32_t m_index;
	};

	// An empty array of structs of class a1, holding a reference on it, with a value array per member of the class;
	// tracked by the class's world.
	StructArrayImplementation(const hkDataClassDict* a1);
	~StructArrayImplementation() override;

	// m_class.
	const hkDataClassImpl* getClass() const override { return m_class; }
	// A new object view of element index.
	hkDataObjectImpl* asObject(int32_t i) const override;
	// Copies each member of val into element index.
	void setObject(int32_t index, hkDataObjectImpl* val) override;
	hkDataObject::Type getType() const override { return hkDataObject::TYPE_STRUCT; }
	void clear() override;
	void reserve(int32_t n) override;
	void setSize(int32_t n) override;
	// m_size.
	int32_t getSize() const override { return m_size; }
	hkDataArrayImpl* swizzleObjectMember(const char* name) const override;

	// The index of the member called name (interned through the class's world), or -1 when
	// there is none.
	int32_t _find(const char* name) const;
	// Drops the member called name (interned).
	void removeMember(InternedString s);
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	int32_t _addMember(const hkDataClassDict::MemberInfo& a1);
	// The index of the member src names, interned first; the body only reads src (getWithDefault, strDup), and every
	// caller (the Object accessors) passes a const name.
	int32_t _findOrAdd(const char* src);
	int32_t _addMemberByName(char* a1);

	hkArray<MemberData> m_memberData;
	hkDataClassDict* m_class;
	int32_t m_size;
};

// An object stored as a dictionary: one value per member name.
class hkDataObjectDict : public hkDataObjectImpl {
public:
	// A member's interned name and its value (a real, an integer or a pointer, by the member's type).
	struct MemberData {
		InternedString m_memberName;
		Union m_value;
	};

	// One member's description and its slot in m_memberData: what _readAccess and _writeAccess fill in. It is passed
	// uninitialised.
	struct MemberAccess {
		const hkDataClassDict::MemberInfo* info;
		int32_t index;
	};

	// An object of class klass, holding a reference on it, with no members.
	hkDataObjectDict(hkDataClassDict* klass) : m_class(klass) {
		if (m_class) {
			++m_class->m_count;
		}
	}

	~hkDataObjectDict() override;

	hkDataObject::Handle getHandle() const override;
	// m_class.
	const hkDataClassImpl* getClass() const override { return m_class; }
	hkDataObject::Value accessByName(const char* name) override;
	int32_t hasMember(const char* name) const override;
	// The first member, 0.
	int32_t getMemberIterator() const override { return 0; }
	int32_t isValid(int32_t it) const override;
	int32_t getNextMember(int32_t it) const override;
	const char* getMemberName(int32_t it) const override;
	// Returns the Value of member it: this object and the member's name.
	const hkDataObject::Value getMemberValue(int32_t it) const override;
	// A member's handle is its interned name, so the accessors take a const char*.
	hkDataArrayImpl* asArray(const char* nameIn) override;
	const char* asString(const char* nameIn) override;
	int64_t asInt(const char* nameIn) override;
	hkDataObjectImpl* asObject(const char* nameIn) override;
	const float* asVec(const char* nameIn, int32_t nreal) override;
	float asReal(const char* nameIn) override;
	void assign(const char* handle, const hkDataObject::Value& value) override;
	void assign(const char* handle, hkDataObjectImpl* value) override;
	void assign(const char* handle, hkDataArrayImpl* value) override;
	void assign(const char* handle, const char* value) override;
	void assign(const char* handle, float r) override;
	void assign(const char* handle, hkHalf r) override;
	void assign(const char* handle, const float* r, int32_t nreal) override;
	void assign(const char* handle, int64_t valueIn) override;

	// Releases every member's value by the class's member type and empties m_memberData. The SDK's clear(int) is
	// another function.
	void selfDestruct();
	// Drops member name and releases its value by the class's member type; nothing when the object has no such
	// member.
	void removeMember(InternedString name);
	// The description of the member handle names and its slot in m_memberData, for reading the member, which is
	// added with its type's empty value when missing; type is not read.
	void _readAccess(MemberAccess& access, InternedString handle, hkDataObject::Type type);
	// The same for writing the member, whose value is made of the type.
	void _writeAccess(MemberAccess& access, InternedString handle, hkDataObject::Type type);

	// Releases count values of a member of type at value: an array's reference, or each vector's reals, object's
	// reference or string, clearing the pointer.
	static void releaseValue(hkDataObject::Type type, Union* value, int32_t count);

	hkDataClassDict* m_class;
	hkArray<MemberData> m_memberData;
};

extern InternedStringRefCounted InternedNullRefCounted;
