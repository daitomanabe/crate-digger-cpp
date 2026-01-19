#pragma once
/**
 * @file api_schema.hpp
 * @brief Self-Description API (MUST per INTRODUCTION_JAVA_TO_CPP.md Section 2.1)
 *
 * Enables AI agents to understand the binary specification without reading source code.
 * Command: {"cmd": "describe_api"} or CLI --schema
 *
 * Returns JSON schema with:
 * - commands: Available command list
 * - params: Parameter names, types, ranges (min/max), units
 * - inputs: Expected input data formats (Tensor shapes, etc.)
 */

#include <string>
#include <string_view>
#include <vector>
#include <optional>

namespace cratedigger {

// ============================================================================
// Schema Definition Types
// ============================================================================

/// Parameter type enumeration
enum class ParamType {
    Int,
    Float,
    String,
    Bool,
    IntArray,
    FloatArray,
    StringArray
};

/// Parameter schema definition
struct ParamSchema {
    std::string name;
    ParamType type{ParamType::String};
    std::string description;
    std::optional<double> min_value;
    std::optional<double> max_value;
    std::optional<std::string> unit;
    std::optional<std::string> default_value;
    bool required{false};
};

/// Command schema definition
struct CommandSchema {
    std::string name;
    std::string description;
    std::vector<ParamSchema> params;
    std::string returns;
};

/// Input tensor shape definition
struct TensorShape {
    std::string name;
    std::vector<int> dims;  // -1 for dynamic dimension
    std::string dtype;      // e.g., "float32", "int64"
    std::string description;
};

/// Full API schema
struct ApiSchema {
    std::string name;
    std::string version;
    std::string description;
    std::vector<CommandSchema> commands;
    std::vector<TensorShape> inputs;
    std::vector<TensorShape> outputs;
};

// ============================================================================
// API Schema Generation
// ============================================================================

/**
 * @brief Generate the complete API schema for Crate Digger
 * @return ApiSchema containing all commands and parameters
 *
 * This function enables AI agents to auto-generate correct control scripts.
 * (Hallucination prevention per Section 2.1)
 */
[[nodiscard]] ApiSchema describe_api();

/**
 * @brief Convert API schema to JSON string
 * @param schema The API schema to serialize
 * @return JSON string representation
 */
[[nodiscard]] std::string to_json(const ApiSchema& schema);

/**
 * @brief Convert a single command schema to JSON
 * @param cmd The command schema
 * @return JSON string representation
 */
[[nodiscard]] std::string to_json(const CommandSchema& cmd);

/**
 * @brief Convert parameter type to string
 * @param type The parameter type
 * @return String representation
 */
[[nodiscard]] std::string_view to_string(ParamType type);

} // namespace cratedigger
