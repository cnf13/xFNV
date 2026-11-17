#include "Havok/Source/Common/Base/System/Io/FileSystem/hkFileSystem.h"

#include "Havok/Source/Common/Base/Object/hkSingleton.h"

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkFileSystem("hkFileSystem", nullptr,
	reinterpret_cast<void**>(&hkFileSystem::s_instance));
