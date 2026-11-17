#include "Havok/Source/Common/Compat/Deprecated/hkSerializeDeprecated.h"

#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Packfile/hkPackfileWriter.h"
#include "Havok/Source/Common/Serialize/Resource/hkObjectResource.h"
#include "Havok/Source/Common/Serialize/Resource/hkResource.h"
#include "Havok/Source/Common/Serialize/Util/hkSerializeUtil.h"

// The address of a static hkSerializeDeprecated, constructed on the first call: the create function of the singleton
// node hkSingletonRegisterhkSerializeDeprecated, hence the SDK create functions' return type.
static hkReferencedObject* createSerializeDeprecatedInstance();

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkSerializeDeprecated("hkSerializeDeprecated",
	&createSerializeDeprecatedInstance, reinterpret_cast<void**>(&hkSerializeDeprecated::s_instance));
static uint32_t hkSerializeDeprecated_staticInstance_guard;

// Returns the hkSerializeDeprecated singleton, a function static built on the first call.
static hkReferencedObject* createSerializeDeprecatedInstance() {
	// The function static (not named in the PDB or IDA; name chosen here), guarded by bit 1 of
	// hkSerializeDeprecated_staticInstance_guard. The exe inlines the constructor (the reference count, then the
	// vtable; it writes no memory size) and registers its thunk with atexit. The hybrid binds the name to the exe's
	// object at its address, untyped memory there, as FNVD_FUNCTION_STATIC does.
	static hkSerializeDeprecated hkSerializeDeprecated_staticInstance;
	return &hkSerializeDeprecated_staticInstance;
}

// The exe's slot holds hkBaseObject's deleting destructor (0xDE10B0): the singleton adds nothing to destroy.
hkSerializeDeprecated::~hkSerializeDeprecated() {}

hkResult hkSerializeDeprecated::saveXmlPackfile(const void* object, const hkClass& klass, hkStreamWriter* writer,
	const hkPackfileWriter::Options& options, hkPackfileWriter::AddObjectListener* userListener) {
	return static_cast<hkResult>(1);
}

int32_t hkSerializeDeprecated::isLoadable(const hkSerializeUtil::FormatDetails& details) {
	return 0;
}

hkResource* hkSerializeDeprecated::loadOldPackfile(hkStreamReader& sr, const hkSerializeUtil::FormatDetails& details,
	hkSerializeUtil::ErrorDetails* errorOut) {
	if (errorOut) {
		errorOut->id.m_storage = 7;
		errorOut->defaultMessage
			.operator=("Call hkSerializeDeprecated::initDeprecated() to include loading support for this file");
	}
	return 0;
}

hkObjectResource* hkSerializeDeprecated::loadOldPackfileOnHeap(hkStreamReader& sr,
	const hkSerializeUtil::FormatDetails& details, hkSerializeUtil::ErrorDetails* errorOut) {
	if (errorOut) {
		errorOut->id.m_storage = 7;
		errorOut->defaultMessage
			.operator=("Call hkSerializeDeprecated::initDeprecated() to include loading support for this file");
	}
	return 0;
}
