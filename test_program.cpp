#include "xlog.h"
GET_LOGGER("Test Program")

#include <thread>
#include <iostream>

// Custom log macro to make life a little easier
#define LOG_AT(sev) CUSTOM_LOG_SEV(__logger, sev) << "Logging @ " << XLog::GetSeverityString(sev)

// Semi 'state machine' to traverse log levels
XLog::Severity get_next(XLog::Severity given)
{
    switch(given)
    {
        case XLog::Severity::INFO:
            return XLog::Severity::DEBUG;
        case XLog::Severity::DEBUG:
            return XLog::Severity::DEBUG2;
        case XLog::Severity::DEBUG2:
            return XLog::Severity::WARNING;
        case XLog::Severity::WARNING:
            return XLog::Severity::WARNING2;
        case XLog::Severity::WARNING2:
            return XLog::Severity::ERROR;
        case XLog::Severity::ERROR:
            return XLog::Severity::ERROR2;
        case XLog::Severity::ERROR2:
            return XLog::Severity::FATAL;
        case XLog::Severity::FATAL:
            return XLog::Severity::INTERNAL;
        case XLog::Severity::INTERNAL:
            return XLog::Severity::INFO;
    }

    return XLog::Severity::INFO;
}

int main(int argc, char** argv)
{
    XLog::LogSettings settings
    {
        .s_default_level = XLog::Severity::INFO,
#ifdef XLOG_ENABLE_EXTERNAL_LOG_CONTROL
        .s_external_control =
        {
            .enabled = true,
            .allow_anyone_access = true,
            .setup_failure_is_fatal = true
        },
#endif // XLOG_ENABLE_EXTERNAL_LOG_CONTROL
#ifdef XLOG_USE_SYSLOG_LOG
        .s_syslog =
        {
            .enabled = true,
            .facility = boost::log::sinks::syslog::facility::user
        },
#endif // XLOG_USE_SYSLOG_LOG
#ifdef XLOG_USE_JOURNAL_LOG
        .s_journal =
        {
            .enabled = true
        },
#endif // XLOG_USE_JOURNAL_LOG
#ifdef XLOG_USE_SIGCPP
        .s_signal =
        {
            .initialize_in_xlog = true
        }
#endif // XLOG_USE_SIGCPP
    };

    XLog::InitializeLogging(settings);

    // Loop over each level every second
    auto current_sev = XLog::Severity::INFO;
    while(true)
    {
        LOG_AT(current_sev);
        current_sev = get_next(current_sev);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return 0;
}
