cmake_minimum_required(VERSION 3.20)

project(Utils LANGUAGES CXX)

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
# linking against the vendored static yaml-cpp fails with LNK2038.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "" FORCE)
add_subdirectory(vendor/yaml-cpp EXCLUDE_FROM_ALL)

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
    target_compile_options(Utils PRIVATE /W3 /sdl /permissive-)
endif()

source_group("Source Files" FILES ${UTILS_SOURCES})
source_group("Header Files" FILES ${UTILS_PUBLIC_HEADERS})
source_group("Header Files\\stb" FILES ${UTILS_VENDOR_HEADERS})
