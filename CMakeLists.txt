cmake_minimum_required(VERSION 3.14)
project(crate_digger VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# C++20 Strict Configuration
# ============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# Options
# ============================================================================
option(CRATE_DIGGER_BUILD_PYTHON "Build Python bindings (nanobind)" ON)
option(CRATE_DIGGER_BUILD_CLI "Build CLI tool" ON)
option(CRATE_DIGGER_BUILD_TESTS "Build tests" ON)

# ============================================================================
# Core Library (Pure C++20, No Framework Dependencies)
# ============================================================================
add_library(crate_digger_core STATIC
    src/core/database.cpp
    src/core/database_util.cpp
    src/core/rekordbox_pdb.cpp
    src/core/rekordbox_anlz.cpp
    src/core/api_schema.cpp
    src/core/logging.cpp
)

target_include_directories(crate_digger_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(crate_digger_core PUBLIC cxx_std_20)

# Strict warning flags
target_compile_options(crate_digger_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)

# ============================================================================
# CLI Tool (JSONL I/O with --schema support)
# ============================================================================
if(CRATE_DIGGER_BUILD_CLI)
    add_executable(crate_digger_cli
        src/cli/main.cpp
    )

    target_link_libraries(crate_digger_cli PRIVATE crate_digger_core)

    set_target_properties(crate_digger_cli PROPERTIES
        OUTPUT_NAME "crate-digger"
    )
endif()

# ============================================================================
# Python Bindings (nanobind - MUST per INTRODUCTION_JAVA_TO_CPP.md)
# ============================================================================
if(CRATE_DIGGER_BUILD_PYTHON)
    find_package(Python 3.8 COMPONENTS Interpreter Development.Module REQUIRED)

    # Try to find nanobind
    execute_process(
        COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
        OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT
        RESULT_VARIABLE NANOBIND_RESULT
    )

    if(NANOBIND_RESULT EQUAL 0)
        list(APPEND CMAKE_PREFIX_PATH "${nanobind_ROOT}")
        find_package(nanobind CONFIG REQUIRED)

        nanobind_add_module(crate_digger_py
            src/python/bindings.cpp
        )

        target_link_libraries(crate_digger_py PRIVATE crate_digger_core)

        set_target_properties(crate_digger_py PROPERTIES
            OUTPUT_NAME "crate_digger"
        )
    else()
        message(WARNING "nanobind not found. Python bindings will not be built. Install with: pip install nanobind")
    endif()
endif()

# ============================================================================
# Tests
# ============================================================================
if(CRATE_DIGGER_BUILD_TESTS)
    enable_testing()

    # Database tests
    add_executable(test_database tests/test_database.cpp)
    target_link_libraries(test_database PRIVATE crate_digger_core)
    add_test(NAME test_database COMMAND test_database)

    # API schema tests
    add_executable(test_api_schema tests/test_api_schema.cpp)
    target_link_libraries(test_api_schema PRIVATE crate_digger_core)
    add_test(NAME test_api_schema COMMAND test_api_schema)
endif()

# ============================================================================
# Installation
# ============================================================================
include(GNUInstallDirs)

install(TARGETS crate_digger_core
    EXPORT crate_digger_targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/cratedigger
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
