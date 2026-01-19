/**
 * @file test_database.cpp
 * @brief Unit tests for Crate Digger database parser
 */

#include "cratedigger/cratedigger.hpp"
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

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) throw std::runtime_error("Assertion failed: " #a " == " #b)

#define ASSERT_NE(a, b) \
    if ((a) == (b)) throw std::runtime_error("Assertion failed: " #a " != " #b)

// ============================================================================
// Tests
// ============================================================================

TEST(track_id_comparison) {
    TrackId a{1};
    TrackId b{1};
    TrackId c{2};

    ASSERT_TRUE(a == b);
    ASSERT_TRUE(a != c);
    ASSERT_TRUE(a < c);
}

TEST(artist_id_comparison) {
    ArtistId a{100};
    ArtistId b{100};
    ArtistId c{200};

    ASSERT_TRUE(a == b);
    ASSERT_TRUE(a != c);
    ASSERT_TRUE(a < c);
}

TEST(case_insensitive_compare) {
    CaseInsensitiveCompare cmp;

    ASSERT_TRUE(!cmp("abc", "ABC"));  // Equal
    ASSERT_TRUE(!cmp("ABC", "abc"));  // Equal
    ASSERT_TRUE(cmp("aaa", "bbb"));   // Less
    ASSERT_TRUE(!cmp("bbb", "aaa"));  // Greater
}

TEST(error_creation) {
    auto error = make_error(ErrorCode::FileNotFound, "test.pdb not found");

    ASSERT_EQ(error.code, ErrorCode::FileNotFound);
    ASSERT_TRUE(error.message.find("test.pdb") != std::string::npos);
    ASSERT_TRUE(!error.source_file.empty());
    ASSERT_TRUE(error.source_line > 0);
}

TEST(database_open_nonexistent) {
    auto result = Database::open("/nonexistent/path/to/file.pdb");

    ASSERT_TRUE(!result.has_value());
    ASSERT_EQ(result.error().code, ErrorCode::FileNotFound);
}

// ============================================================================
// Safety Curtain Tests
// ============================================================================

TEST(safety_validate_bpm) {
    // Within range
    ASSERT_EQ(validate_bpm(120.0f), 120.0f);
    ASSERT_EQ(validate_bpm(128.5f), 128.5f);

    // Below minimum
    ASSERT_EQ(validate_bpm(10.0f), SafetyLimits::MIN_BPM);
    ASSERT_EQ(validate_bpm(0.0f), SafetyLimits::MIN_BPM);

    // Above maximum
    ASSERT_EQ(validate_bpm(350.0f), SafetyLimits::MAX_BPM);
    ASSERT_EQ(validate_bpm(999.0f), SafetyLimits::MAX_BPM);
}

TEST(safety_validate_duration) {
    // Within range
    ASSERT_EQ(validate_duration(300), 300u);
    ASSERT_EQ(validate_duration(3600), 3600u);

    // At maximum
    ASSERT_EQ(validate_duration(SafetyLimits::MAX_DURATION_SECONDS),
              SafetyLimits::MAX_DURATION_SECONDS);

    // Above maximum
    ASSERT_EQ(validate_duration(100000), SafetyLimits::MAX_DURATION_SECONDS);
}

TEST(safety_validate_rating) {
    // Within range
    ASSERT_EQ(validate_rating(0), 0);
    ASSERT_EQ(validate_rating(3), 3);
    ASSERT_EQ(validate_rating(5), 5);

    // Above maximum
    ASSERT_EQ(validate_rating(10), SafetyLimits::MAX_RATING);
}

TEST(safety_is_valid_bpm) {
    ASSERT_TRUE(is_valid_bpm(120.0f));
    ASSERT_TRUE(is_valid_bpm(SafetyLimits::MIN_BPM));
    ASSERT_TRUE(is_valid_bpm(SafetyLimits::MAX_BPM));

    ASSERT_TRUE(!is_valid_bpm(10.0f));
    ASSERT_TRUE(!is_valid_bpm(350.0f));
}

TEST(safety_is_valid_rating) {
    ASSERT_TRUE(is_valid_rating(0));
    ASSERT_TRUE(is_valid_rating(5));

    ASSERT_TRUE(!is_valid_rating(6));
    ASSERT_TRUE(!is_valid_rating(255));
}

// ============================================================================
// Cue Point Tests
// ============================================================================

TEST(cue_point_type_to_string) {
    ASSERT_EQ(cue_point_type_to_string(CuePointType::Cue), "cue");
    ASSERT_EQ(cue_point_type_to_string(CuePointType::FadeIn), "fade_in");
    ASSERT_EQ(cue_point_type_to_string(CuePointType::FadeOut), "fade_out");
    ASSERT_EQ(cue_point_type_to_string(CuePointType::Load), "load");
    ASSERT_EQ(cue_point_type_to_string(CuePointType::Loop), "loop");
}

TEST(cue_point_properties) {
    CuePoint cue;
    cue.type = CuePointType::Cue;
    cue.time_ms = 5000;
    cue.hot_cue_number = 0;

    // Memory cue (not a hot cue)
    ASSERT_TRUE(!cue.is_hot_cue());
    ASSERT_TRUE(!cue.is_loop());
    ASSERT_EQ(cue.time_seconds(), 5.0f);

    // Hot cue
    cue.hot_cue_number = 1;
    ASSERT_TRUE(cue.is_hot_cue());

    // Loop
    cue.type = CuePointType::Loop;
    cue.loop_time_ms = 9000;
    ASSERT_TRUE(cue.is_loop());
    ASSERT_EQ(cue.loop_duration_ms(), 4000u);
}

TEST(cue_point_loop_duration) {
    CuePoint cue;
    cue.type = CuePointType::Loop;
    cue.time_ms = 10000;
    cue.loop_time_ms = 18000;

    ASSERT_EQ(cue.loop_duration_ms(), 8000u);

    // Not a loop
    cue.type = CuePointType::Cue;
    ASSERT_EQ(cue.loop_duration_ms(), 0u);
}

} // anonymous namespace

int main() {
    std::cout << "\n=== Crate Digger Tests ===\n\n";

    // Tests are automatically registered and run by TestRunner constructors

    std::cout << "\n=== Summary ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}
