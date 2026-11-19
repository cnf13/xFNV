#include "Havok/Source/Common/Visualize/hkVersionReporter.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringBuf.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"
#include "Havok/Source/Common/Visualize/Serialize/hkDisplaySerializeOStream.h"

// Builds the version packet in a buffer (the command byte, the two protocol versions, then the platform and the
// compiler strings, each after its 16-bit length), then writes the buffer's size and the buffer to connection.
hkResult hkVersionReporter::sendVersionInformation(hkStreamWriter* connection) {
    using rawData_t = hkArray<char, hkContainerHeapAllocator>;
    rawData_t rawData;
    hkDisplaySerializeOStream commandStream(rawData);
    commandStream.write8u(HK_VERSION_INFORMATION);
    commandStream.write32(m_protocolVersion);
    commandStream.write32(m_protocolMinimumCompatible);
    int32_t platformLength = hkString::strLen("PC_32");
    if (platformLength > 0xFFFF) {
        platformLength = 0xFFFF;
    }
    commandStream.write16u(platformLength);
    commandStream.writeRaw("PC_32", platformLength);
    hkStringBuf compiler;
    compiler.printf("%s:%i", "MSVC", 1500);
    // The string's length: its size less the terminating zero.
    int32_t compilerLength = compiler.m_string.m_size - 1;
    if (compilerLength > 0xFFFF) {
        compilerLength = 0xFFFF;
    }
    commandStream.write16u(compilerLength);
    commandStream.writeRaw(compiler.m_string.m_data, compilerLength);
    hkDisplaySerializeOStream connectionStream(connection);
    connectionStream.write32(rawData.m_size);
    connectionStream.writeRaw(rawData.m_data, rawData.m_size);
    return HK_SUCCESS;
}
