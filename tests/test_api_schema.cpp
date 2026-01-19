/**
 * @file test_api_schema.cpp
 * @brief Unit tests for API Schema (describe_api)
 *
 * Per INTRODUCTION_JAVA_TO_CPP.md Section 2.1:
 * This ensures the self-description API works correctly for AI agents.
 */

#include "cratedigger/api_schema.hpp"
#include <iostream>
#include <cassert>

using namespace cratedigger;

namespace {

int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    void test_##name(); \
    struct TestRunner_##name { \
        TestRunner_##name() { \
            std::cout << "Running test: " << #name << "... "; \
            try { \
                test_##name(); \
                std::cout << "PASSED\n"; \
                ++tests_passed; \
            } catch (const std::exception& e) { \
                std::cout << "FAILED: " << e.what() << "\n"; \
                ++tests_failed; \
            } \
        } \
    } test_runner_##name; \
    void test_##name()

#define ASSERT_TRUE(cond) \
    if (!(cond)) throw std::runtime_error("Assertion failed: " #cond)

#define ASSERT_FALSE(cond) \
    if (cond) throw std::runtime_error("Assertion failed: !" #cond)

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) throw std::runtime_error("Assertion failed: " #a " == " #b)

// ============================================================================
// Tests
// ============================================================================

TEST(param_type_to_string) {
    ASSERT_EQ(to_string(ParamType::Int), "int");
    ASSERT_EQ(to_string(ParamType::Float), "float");
    ASSERT_EQ(to_string(ParamType::String), "string");
    ASSERT_EQ(to_string(ParamType::Bool), "bool");
    ASSERT_EQ(to_string(ParamType::IntArray), "int[]");
    ASSERT_EQ(to_string(ParamType::FloatArray), "float[]");
    ASSERT_EQ(to_string(ParamType::StringArray), "string[]");
}

TEST(describe_api_returns_valid_schema) {
    auto schema = describe_api();

    ASSERT_EQ(schema.name, "crate_digger");
    ASSERT_FALSE(schema.version.empty());
    ASSERT_FALSE(schema.description.empty());
    ASSERT_FALSE(schema.commands.empty());
}

TEST(describe_api_has_open_command) {
    auto schema = describe_api();

    bool found = false;
    for (const auto& cmd : schema.commands) {
        if (cmd.name == "open") {
            found = true;
            ASSERT_FALSE(cmd.description.empty());
            ASSERT_FALSE(cmd.params.empty());
            ASSERT_EQ(cmd.params[0].name, "path");
            ASSERT_EQ(cmd.params[0].type, ParamType::String);
            ASSERT_TRUE(cmd.params[0].required);
            break;
        }
    }
    ASSERT_TRUE(found);
}

TEST(describe_api_has_get_track_command) {
    auto schema = describe_api();

    bool found = false;
    for (const auto& cmd : schema.commands) {
        if (cmd.name == "get_track") {
            found = true;
            ASSERT_FALSE(cmd.description.empty());
            ASSERT_FALSE(cmd.params.empty());
            ASSERT_EQ(cmd.params[0].name, "track_id");
            ASSERT_EQ(cmd.params[0].type, ParamType::Int);
            break;
        }
    }
    ASSERT_TRUE(found);
}

TEST(describe_api_has_describe_api_command) {
    auto schema = describe_api();

    bool found = false;
    for (const auto& cmd : schema.commands) {
        if (cmd.name == "describe_api") {
            found = true;
            ASSERT_FALSE(cmd.description.empty());
            break;
        }
    }
    ASSERT_TRUE(found);
}

TEST(describe_api_has_tensor_definitions) {
    auto schema = describe_api();

    ASSERT_FALSE(schema.inputs.empty());
    ASSERT_FALSE(schema.outputs.empty());

    // Check for track_ids input
    bool found_input = false;
    for (const auto& input : schema.inputs) {
        if (input.name == "track_ids") {
            found_input = true;
            ASSERT_EQ(input.dtype, "int64");
            break;
        }
    }
    ASSERT_TRUE(found_input);
}

TEST(schema_to_json_format) {
    auto schema = describe_api();
    auto json = to_json(schema);

    // Basic JSON structure checks
    ASSERT_TRUE(json.find("\"name\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"version\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"commands\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"inputs\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"outputs\"") != std::string::npos);

    // Should be valid JSON (starts with { ends with })
    ASSERT_TRUE(json.front() == '{');
    ASSERT_TRUE(json.back() == '}');
}

TEST(command_to_json_format) {
    CommandSchema cmd;
    cmd.name = "test_cmd";
    cmd.description = "A test command";
    cmd.params.push_back({
        .name = "param1",
        .type = ParamType::Int,
        .description = "First param",
        .required = true
    });
    cmd.returns = "int";

    auto json = to_json(cmd);

    ASSERT_TRUE(json.find("\"name\":\"test_cmd\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"description\":\"A test command\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"params\":[") != std::string::npos);
    ASSERT_TRUE(json.find("\"returns\":\"int\"") != std::string::npos);
}

TEST(json_escaping) {
    CommandSchema cmd;
    cmd.name = "test";
    cmd.description = "A \"quoted\" description\nwith newline";
    cmd.returns = "";

    auto json = to_json(cmd);

    // Should escape quotes and newlines
    ASSERT_TRUE(json.find("\\\"quoted\\\"") != std::string::npos);
    ASSERT_TRUE(json.find("\\n") != std::string::npos);
}

} // anonymous namespace

int main() {
    std::cout << "\n=== API Schema Tests ===\n\n";

    // Tests are automatically registered and run by TestRunner constructors

    std::cout << "\n=== Summary ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}
