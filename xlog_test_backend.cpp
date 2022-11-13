#include "xlog_test_backend.internal.h"

#include <boost/log/attributes/value_extraction.hpp>

void xlog_test_backend::consume(const boost::log::record_view& rec)
{
    auto timestamp = boost::log::extract<boost::posix_time::ptime>("TimeStamp", rec);
    auto sev = boost::log::extract<xlog::Severity>("Severity", rec);
    auto cnl = boost::log::extract<std::string>("Channel", rec);
    auto message = boost::log::extract<std::string>("Message", rec);

    if(timestamp.empty() || sev.empty() || cnl.empty() || message.empty())
    {
        m_records.emplace(LogRecordData{
            .m_timestamp = {},
            .m_severity = {},
            .m_channel = {},
            .m_message = {},
            .m_valid = false,
            .m_error = "TODO"
        });
    }

    m_records.emplace(LogRecordData{
        .m_timestamp = timestamp.get(),
        .m_severity = sev.get(),
        .m_channel = cnl.get(),
        .m_message = message.get(),
        .m_valid = true,
        .m_error = {}
    });
}

std::optional<LogRecordData> xlog_test_backend::pop_record()
{
    if(m_records.empty())
    {
        return {};
    }

    LogRecordData data = m_records.front();
    m_records.pop();
    return data;
}
