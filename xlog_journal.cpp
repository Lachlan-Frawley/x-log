#include "xlog_journal.noexport.h"

#include "xlog.h"

#undef LOG_INFO
#undef LOG_DEBUG

#include <systemd/sd-journal.h>

#include <boost/log/attributes/value_extraction.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

// Keep up-to-date with SYSLOG_SEV_MAPPER in xlog.cpp
// TODO - Find a better way of keeping this all in-sync
int sev2priority(xlog::Severity sev)
{
    switch (sev)
    {
        case xlog::Severity::INFO:
            return LOG_DEBUG;
        case xlog::Severity::DEBUG:
        case xlog::Severity::DEBUG2:
            return LOG_INFO;
        case xlog::Severity::WARNING:
        case xlog::Severity::WARNING2:
            return LOG_WARNING;
        case xlog::Severity::ERROR:
        case xlog::Severity::ERROR2:
            return LOG_ERR;
        case xlog::Severity::FATAL:
            return LOG_CRIT;
        case xlog::Severity::INTERNAL:
            return LOG_ALERT;
    }

    return LOG_EMERG;
}

#include <fmt/core.h>

#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION

#define XLOG_GET_FILE fmt::format("CODE_FILE={0}", location.file_name())
#define XLOG_GET_LINE fmt::format("CODE_LINE={0}", location.line())
#define XLOG_GET_FUNC fmt::format("CODE_FUNC={0}", location.function_name())

#define XLOG_USE_STR(var) var.c_str()

#define XLOG_GET_SOURCE_LOCATION auto location = boost::log::extract<std::source_location>("SourceLocation", rec).get();

#else

#define XLOG_GET_FILE ""
#define XLOG_GET_LINE ""
#define XLOG_GET_FUNC ""

#define XLOG_USE_STR(var) ""

#define XLOG_GET_SOURCE_LOCATION

#endif // XLOG_LOGGING_USE_SOURCE_LOCATION

void xlog_journal_backend::consume(const boost::log::record_view& rec)
{
    auto sev = boost::log::extract<xlog::Severity>("Severity", rec);
    auto channel = boost::log::extract<std::string>("Channel", rec);
    auto message = boost::log::extract<std::string>("Message", rec);

    XLOG_GET_SOURCE_LOCATION
    const std::string _file = XLOG_GET_FILE;
    const std::string _line = XLOG_GET_LINE;
    const std::string _func = XLOG_GET_FUNC;

    auto result = sd_journal_send_with_location(
        XLOG_USE_STR(_file),
        XLOG_USE_STR(_line),
        XLOG_USE_STR(_func),
        "MESSAGE=%s", message.get().c_str(),
        "CHANNEL=%s", channel.get().c_str(),
        "PRIORITY=%i", sev2priority(sev.get()),
        NULL);

    if(result != 0)
    {
        // TODO - Do something with this information
    }
}
