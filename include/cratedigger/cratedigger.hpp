#pragma once
/**
 * @file cratedigger.hpp
 * @brief Main header for Crate Digger C++ Library
 *
 * Crate Digger is a library for parsing rekordbox media exports
 * and track analysis files.
 *
 * Design Philosophy (per INTRODUCTION_JAVA_TO_CPP.md):
 * - Pure C++20 (Standard Library Only, No Framework Headers)
 * - Headless (No GUI/Window dependencies)
 * - Deterministic (Time Injection, Random Injection)
 * - AI-Friendly (Self-Description API, Structured Logging)
 * - Python-First (nanobind for NumPy interop)
 *
 * @author Daito Manabe / Rhizomatiks
 * @version 1.0.0
 */

#include "types.hpp"
#include "database.hpp"
#include "api_schema.hpp"
#include "logging.hpp"

namespace cratedigger {

/// Library version
constexpr const char* VERSION = "1.0.0";

/// Library name
constexpr const char* NAME = "crate_digger";

} // namespace cratedigger
