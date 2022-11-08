#include "xlog.h"
XLOG_GET_LOGGER("Test Program")

#include <thread>

// Custom log macro to make life a little easier
#define LOG_AT(sev) CUSTOM_LOG_SEV(XLOG_LOGGER_VAR_NAME, sev) << "Logging @ " << xlog::GetSeverityString(sev)

// Semi 'state machine' to traverse log levels
xlog::Severity get_next(xlog::Severity given)
{
    switch(given)
    {
        case xlog::Severity::INFO:
            return xlog::Severity::DEBUG;
        case xlog::Severity::DEBUG:
            return xlog::Severity::DEBUG2;
        case xlog::Severity::DEBUG2:
            return xlog::Severity::WARNING;
        case xlog::Severity::WARNING:
            return xlog::Severity::WARNING2;
        case xlog::Severity::WARNING2:
            return xlog::Severity::ERROR;
        case xlog::Severity::ERROR:
            return xlog::Severity::ERROR2;
        case xlog::Severity::ERROR2:
            return xlog::Severity::FATAL;
        case xlog::Severity::FATAL:
            return xlog::Severity::INTERNAL;
        case xlog::Severity::INTERNAL:
            return xlog::Severity::INFO;
    }

    return xlog::Severity::INFO;
}

int main(int argc, char** argv)
{
    xlog::LogSettings settings
    {
        .s_default_level = xlog::Severity::INFO,
        .s_consoleStream = xlog::ConsoleLogLocation::CLOG,
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

    xlog::InitializeLogging(settings);

    XLOG_INFO_F("Hello {0}!", "World");
    XLOG_INFO_F("I am testing formatted logging!");

    // Loop over each level every second
    auto current_sev = xlog::Severity::INFO;
    while(true)
    {
        LOG_AT(current_sev);
        current_sev = get_next(current_sev);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    XLOG_FATAL_F("Message: {0}", "Goodbye world");

    return 0;
}
