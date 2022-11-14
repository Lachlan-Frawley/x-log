#include <iostream>

#include "xlog.h"

#include "xlog_test_backend.internal.h"

#include "xlog_test_utils.h"

#include <gtest/gtest.h>

#include <cstdlib>

#include <system_error>

// Check if a timestamp is within a reasonable range (range in milliseconds)
// Based on: https://stackoverflow.com/a/39003411

::testing::AssertionResult TimestampInRange(const boost::posix_time::ptime& timestamp, long threshold)
{
    auto diff = boost::posix_time::microsec_clock::local_time() - timestamp;
    auto abs_diff = std::abs(diff.total_milliseconds());

    if(abs_diff > threshold)
    {
        return ::testing::AssertionFailure()
            << abs_diff << " ms is outside of the given threshold of " << threshold << " ms";
    }

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult TimestampInTwoWayRange(const boost::posix_time::ptime& timestamp, long rangeAbove, long rangeBelow)
{
    auto diff = boost::posix_time::microsec_clock::local_time() - timestamp;
    auto diff_ms = diff.total_milliseconds();
    auto abs_diff = std::abs(diff_ms);

    if(diff_ms < 0)
    {
        if(abs_diff > rangeBelow)
        {
            return ::testing::AssertionFailure()
                << abs_diff << " ms is outside of the given threshold (below) of " << rangeBelow << " ms";
        }
    }
    else
    {
        if(abs_diff > rangeAbove)
        {
            return ::testing::AssertionFailure()
                << abs_diff << " ms is outside of the given threshold (above) of " << rangeAbove << " ms";
        }
    }

    return ::testing::AssertionSuccess();
}

xlog_test_backend_ptr G_BACKEND;
XLOG_GET_LOGGER("test")

constexpr size_t MIN_RANDOM_STRING_LENGTH = 32;
constexpr size_t MAX_RANDOM_STRING_LENGTH = 256;

// Make a random string with some arbitrary characters in it
#define MAKE_RANDOM_STRING() xlog_test::rng::make_random_string(MIN_RANDOM_STRING_LENGTH, MAX_RANDOM_STRING_LENGTH)

// Make a random string with some arbitrary characters in it, but will not have any null terminators present
#define MAKE_RANDOM_STRING_SAFE() xlog_test::rng::make_random_string_safe(MIN_RANDOM_STRING_LENGTH, MAX_RANDOM_STRING_LENGTH)

constexpr size_t TEST_ITERATIONS = 100;

#define MAKE_RANDOM_ERRC() std::make_error_code(std::errc::permission_denied)

#define XLOG_TEST_BOILERPLATE_A(record_varname) auto output = G_BACKEND->locked_backend()->pop_record(); \
EXPECT_TRUE(output.has_value()) << "No value present in test backend"; \
auto record_varname = output.value(); \
EXPECT_TRUE(record_varname.m_valid) << "Record Invalid: " << record_varname.m_error;

#define XLOG_TEST_BOILERPLATE_B0(record_varname, channel, severity, message, timestamp_range) EXPECT_EQ(record_varname.m_channel, channel); \
EXPECT_EQ(record_varname.m_severity, severity); \
EXPECT_EQ(record_varname.m_message, message); \
EXPECT_TRUE(TimestampInRange(record_varname.m_timestamp, timestamp_range));

#define XLOG_TEST_BOILERPLAT_B1(record_varname, channel, severity, message, timestamp_range_upper, timestamp_range_lower) EXPECT_EQ(record_varname.m_channel, channel); \
EXPECT_EQ(record_varname.m_severity, severity); \
EXPECT_EQ(record_varname.m_message, message); \
EXPECT_TRUE(TimestampInTwoWayRange(record_varname.m_timestamp, timestamp_range_upper, timestamp_range_lower));

#define XLOG_SEV_INFO xlog::Severity::INFO
#define XLOG_SEV_DEBUG xlog::Severity::DEBUG
#define XLOG_SEV_DEBUG2 xlog::Severity::DEBUG2
#define XLOG_SEV_WARN xlog::Severity::WARNING
#define XLOG_SEV_WARN2 xlog::Severity::WARNING2
#define XLOG_SEV_ERROR xlog::Severity::ERROR
#define XLOG_SEV_ERROR2 xlog::Severity::ERROR2

// These can't be used in the XLOG_<LEVEL>_XYZ format
#define XLOG_SEV_FATAL xlog::Severity::FATAL
#define XLOG_SEV_INTERNAL xlog::Severity::INTERNAL

#define XLOG_LEVEL2SEV(level) XLOG_SEV_ ## level

#include "xlog_gtest_format.h"

#include "xlog_gtest_fatal.h"

TEST(Invariants, EnsureNoMissedLogs)
{
    auto consumed_count = G_BACKEND->locked_backend()->consumed_count();
    auto popped_count = G_BACKEND->locked_backend()->popped_count();

    EXPECT_EQ(consumed_count, popped_count);
}

int main(int argc, char** argv)
{
    xlog::InitializeLogging();
    G_BACKEND = get_test_backend();

    ::testing::InitGoogleTest(&argc, argv);
    auto rv = RUN_ALL_TESTS();

    return rv;
}
