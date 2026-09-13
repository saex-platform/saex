cmake_minimum_required(VERSION 3.25)
project(SaexNativeDependencyProbe VERSION 0.1.1 LANGUAGES CXX)
include(CTest)

if(NOT CMAKE_VERSION STREQUAL "4.3.3" OR NOT CMAKE_GENERATOR STREQUAL "Visual Studio 17 2022")
  message(FATAL_ERROR "SAEX_SDK_TOOLCHAIN_MISMATCH: CMake/generator differs from the lock")
endif()
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4 OR NOT CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
  message(FATAL_ERROR "SAEX_SDK_X86_ONLY: standalone Windows MSVC Win32 probe required")
endif()
if(NOT CMAKE_CXX_COMPILER_VERSION STREQUAL "19.44.35228.0")
  message(FATAL_ERROR "SAEX_SDK_TOOLCHAIN_MISMATCH: compiler is not the reviewed lock version")
endif()
if(NOT CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION STREQUAL "10.0.26100.0")
  message(FATAL_ERROR "SAEX_SDK_TOOLCHAIN_MISMATCH: Windows SDK differs from the lock")
endif()

get_filename_component(SAEX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(SAEX_SDK_SOURCE "${SAEX_ROOT}/out/dependencies/plugin-sdk-sa/b55e89b336a81448c1aa1a5b188431c9845ebaa9-n1-p1")
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/dependency.py" verify
  RESULT_VARIABLE SAEX_VERIFY_RESULT OUTPUT_VARIABLE SAEX_VERIFY_OUTPUT ERROR_VARIABLE SAEX_VERIFY_ERROR)
if(NOT SAEX_VERIFY_RESULT EQUAL 0)
  message(FATAL_ERROR "${SAEX_VERIFY_ERROR}")
endif()
add_custom_target(saex_sdk_verify
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/dependency.py" verify
  COMMENT "Verify SDK source, patches and recipe before compilation" VERBATIM)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

add_library(saex_sdk_private STATIC "${SAEX_SDK_SOURCE}/shared/PluginBase.cpp")
add_dependencies(saex_sdk_private saex_sdk_verify)
target_include_directories(saex_sdk_private PRIVATE "${SAEX_SDK_SOURCE}/shared")
target_compile_definitions(saex_sdk_private PRIVATE GTASA PLUGIN_SGV_10US RW)
target_compile_options(saex_sdk_private PRIVATE /W4 /WX /permissive- /utf-8 /EHsc /Zp8 /Gd)

add_executable(saex_sdk_probe "${SAEX_ROOT}/src/engine/plugin_sdk/sdk_probe.cpp")
target_include_directories(saex_sdk_probe PRIVATE "${SAEX_SDK_SOURCE}/shared" "${SAEX_SDK_SOURCE}/plugin_sa/game_sa")
target_compile_definitions(saex_sdk_probe PRIVATE GTASA PLUGIN_SGV_10US RW)
target_compile_options(saex_sdk_probe PRIVATE /W4 /WX /permissive- /utf-8 /EHsc /Zp8 /Gd)
target_link_options(saex_sdk_probe PRIVATE /WX /INCREMENTAL:NO /DEBUG:FULL)
target_link_libraries(saex_sdk_probe PRIVATE saex_sdk_private)
add_test(NAME engine.sdk_build_probe COMMAND saex_sdk_probe)
set_tests_properties(engine.sdk_build_probe PROPERTIES TIMEOUT 10)

# No vendor include directories, definitions or standard propagate to a consumer.
get_target_property(SAEX_PUBLIC_INCLUDES saex_sdk_private INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(SAEX_PUBLIC_DEFINES saex_sdk_private INTERFACE_COMPILE_DEFINITIONS)
get_target_property(SAEX_PUBLIC_FEATURES saex_sdk_private INTERFACE_COMPILE_FEATURES)
if(SAEX_PUBLIC_INCLUDES OR SAEX_PUBLIC_DEFINES OR SAEX_PUBLIC_FEATURES)
  message(FATAL_ERROR "SAEX_SDK_PUBLIC_SURFACE_LEAK")
endif()
file(WRITE "${CMAKE_BINARY_DIR}/sdk-build-settings.json"
  "{\"compiler\":\"${CMAKE_CXX_COMPILER_VERSION}\",\"cmake\":\"${CMAKE_VERSION}\",\"generator\":\"${CMAKE_GENERATOR}\",\"windowsSdk\":\"${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}\",\"architecture\":\"x86\",\"cxxStandard\":23,\"standardFlag\":\"${CMAKE_CXX23_STANDARD_COMPILE_OPTION}\",\"packing\":8,\"callingConvention\":\"cdecl\",\"publicVendorIncludes\":false}\n")
