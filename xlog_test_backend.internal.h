#pragma once

#include <queue>
#include <atomic>
#include <optional>

#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "xlog.h"

struct LogRecordData
{
    boost::posix_time::ptime m_timestamp;
    xlog::Severity m_severity;
    std::string m_channel;
    std::string m_message;

    const bool m_valid;
    const std::string m_error;
};

#include <boost/log/sinks/basic_sink_backend.hpp>

class xlog_test_backend final :
    public boost::log::sinks::basic_sink_backend<
        boost::log::sinks::combine_requirements<
            boost::log::sinks::synchronized_feeding
        >::type
    >
{
public:
    void consume(const boost::log::record_view& rec);

    std::optional<LogRecordData> pop_record();

    int consumed_count() const;
    int popped_count() const;

private:
    std::queue<LogRecordData> m_records;
    std::atomic_int m_consumed;
    std::atomic_int m_popped;
};

#include <boost/log/sinks/sync_frontend.hpp>
typedef boost::shared_ptr<boost::log::sinks::synchronous_sink<xlog_test_backend>> xlog_test_backend_ptr;

extern xlog_test_backend_ptr get_test_backend();
extern void xlog_set_errno_test_string(std::string value);