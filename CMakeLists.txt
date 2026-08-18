cmake_minimum_required(VERSION 3.20)

project(Utils LANGUAGES CXX)

# MemCheck is a Debug variant instrumented with MSVC AddressSanitizer. This
# registers it both for standalone Utils builds and as a dependency of a
# parent project (e.g. massive-poly-pusher) that defines its own MemCheck
# configuration - IN_LIST guards against double-registering the config type
# when the parent already has. Utils must select the same runtime library
# and base compiler/linker flags a MemCheck-aware parent uses, or MemCheck
# builds silently link the release CRT here (see CMAKE_MSVC_RUNTIME_LIBRARY
# below) while the parent links the debug CRT, corrupting STL types (e.g.
# std::string) passed across the Utils DLL boundary.
if(CMAKE_CONFIGURATION_TYPES)
    if(NOT "MemCheck" IN_LIST CMAKE_CONFIGURATION_TYPES)
        list(APPEND CMAKE_CONFIGURATION_TYPES MemCheck)
        set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Semicolon separated list of supported configuration types" FORCE)
    endif()

    # /RTC1 (runtime checks) cannot be combined with /fsanitize=address under MSVC.
    string(REPLACE "/RTC1" "" MPP_UTILS_MEMCHECK_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_MEMCHECK "${MPP_UTILS_MEMCHECK_CXX_FLAGS}" CACHE STRING "Flags used by the C++ compiler for MemCheck builds" FORCE)
    unset(MPP_UTILS_MEMCHECK_CXX_FLAGS)

    # ASan is incompatible with incremental linking.
    string(REPLACE "/INCREMENTAL" "" MPP_UTILS_MEMCHECK_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
    set(CMAKE_SHARED_LINKER_FLAGS_MEMCHECK "${MPP_UTILS_MEMCHECK_SHARED_LINKER_FLAGS} /INCREMENTAL:NO" CACHE STRING "Flags used by the shared library linker for MemCheck builds" FORCE)
    unset(MPP_UTILS_MEMCHECK_SHARED_LINKER_FLAGS)

    mark_as_advanced(CMAKE_CXX_FLAGS_MEMCHECK CMAKE_SHARED_LINKER_FLAGS_MEMCHECK)
endif()

set(UTILS_SOURCES
    src/FileSystem.cpp
    src/Image.cpp
    src/StringUtils.cpp
    src/StructuredData.cpp
    src/tinyxml2.cpp
    src/XmlReader.cpp
    src/XmlWriter.cpp
    src/YamlReader.cpp
    src/YamlWriter.cpp
)

set(UTILS_PUBLIC_HEADERS
    include/utils/FileSystem.h
    include/utils/Image.h
    include/utils/Platform.h
    include/utils/StringUtils.h
    include/utils/StructuredData.h
    include/utils/tinyxml2.h
    include/utils/XmlExceptions.h
    include/utils/XmlReader.h
    include/utils/XmlWriter.h
    include/utils/YamlExceptions.h
    include/utils/YamlReader.h
    include/utils/YamlWriter.h
)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
# Match Utils' own runtime library selection (MSVC defaults to /MD), otherwise
# linking against the vendored static yaml-cpp fails with LNK2038. MemCheck
# is a Debug variant (see above) and must select the debug CRT the same way.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<OR:$<CONFIG:Debug>,$<CONFIG:MemCheck>>:Debug>DLL" CACHE STRING "" FORCE)
add_subdirectory(vendor/yaml-cpp EXCLUDE_FROM_ALL)
if(MSVC AND TARGET yaml-cpp)
    # yaml-cpp is linked statically into Utils.dll (not a separate DLL), so
    # unlike GLEW/Assimp/SDL it must share Utils' ASan instrumentation: MSVC's
    # STL embeds a /FAILIFMISMATCH marker for container ASan annotations, and
    # linking an unannotated static lib into an annotated DLL fails with
    # LNK2038 ("mismatch detected for 'annotate_string'").
    target_compile_options(yaml-cpp PRIVATE $<$<CONFIG:MemCheck>:/fsanitize=address>)
endif()

set(UTILS_VENDOR_HEADERS
    vendor/include/stb/stb_image.h
    vendor/include/stb/stb_image_write.h
)

add_library(Utils SHARED
    ${UTILS_SOURCES}
    ${UTILS_PUBLIC_HEADERS}
    ${UTILS_VENDOR_HEADERS}
)
add_library(Utils::Utils ALIAS Utils)

target_compile_features(Utils PUBLIC cxx_std_20)

target_compile_definitions(Utils
    PRIVATE
        UTILS_DLL_EXPORT
        $<$<PLATFORM_ID:Windows>:UNICODE;_UNICODE>
)

target_include_directories(Utils
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/utils>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/include
)

target_link_libraries(Utils PRIVATE yaml-cpp::yaml-cpp)

set_target_properties(Utils PROPERTIES
    DEBUG_POSTFIX d
    CXX_EXTENSIONS OFF
)

if(MSVC)
    # /fsanitize=address is applied only to Utils itself, not the vendored
    # yaml-cpp dependency it links, matching the parent project's approach
    # of instrumenting its own code but not legacy/vendored dependencies.
    target_compile_options(Utils PRIVATE /W3 /sdl /permissive- $<$<CONFIG:MemCheck>:/fsanitize=address>)
endif()

source_group("Source Files" FILES ${UTILS_SOURCES})
source_group("Header Files" FILES ${UTILS_PUBLIC_HEADERS})
source_group("Header Files\\stb" FILES ${UTILS_VENDOR_HEADERS})
