#include <iostream>

#include "xlog.h"

#include "xlog_test_backend.internal.h"

#include <gtest/gtest.h>

#include <cstdlib>

// Check if a timestamp is within a reasonable range (range in milliseconds)
// Based on: https://stackoverflow.com/a/39003411

::testing::AssertionResult TimestampInRange(const boost::posix_time::ptime& timestamp, double threshold)
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

::testing::AssertionResult TimestampInTwoWayRange(const boost::posix_time::ptime& timestamp, double rangeAbove, double rangeBelow)
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

#define XLOG_TEST_BOILERPLATE_A(record_varname) auto output = G_BACKEND->locked_backend()->pop_record(); \
EXPECT_TRUE(output.has_value()) << "No value present in test backend"; \
auto record_varname = output.value(); \
EXPECT_TRUE(record_varname.m_valid) << "Record Invalid: " << record_varname.m_error;

#define XLOG_TEST_BOILERPLAT_B0(record_varname, channel, severity, message, timestamp_range) EXPECT_EQ(record_varname.m_channel, channel); \
EXPECT_EQ(record_varname.m_severity, severity); \
EXPECT_EQ(record_varname.m_message, message); \
EXPECT_TRUE(TimestampInRange(record_varname.m_timestamp, timestamp_range));

#define XLOG_TEST_BOILERPLAT_B1(record_varname, channel, severity, message, timestamp_range_upper, timestamp_range_lower) EXPECT_EQ(record_varname.m_channel, channel); \
EXPECT_EQ(record_varname.m_severity, severity); \
EXPECT_EQ(record_varname.m_message, message); \
EXPECT_TRUE(TimestampInTwoWayRange(record_varname.m_timestamp, timestamp_range_upper, timestamp_range_lower));

TEST(Formatted, NormalFormat)
{
    XLOG_INFO_F("I am formatted!");
    XLOG_TEST_BOILERPLATE_A(record)
    XLOG_TEST_BOILERPLAT_B0(record, "test", xlog::Severity::INFO, "I am formatted!", 500)
}

int main(int argc, char** argv)
{
    xlog::InitializeLogging();
    G_BACKEND = get_test_backend();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
