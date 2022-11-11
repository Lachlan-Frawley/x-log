#include "xlog.h"

static XLog::LogSettings LOGGER_SETTINGS;

#include <stdlib.h>
#include <sys/stat.h>

#include <mutex>
#include <atomic>
#include <unordered_map>

#include <boost/log/utility/setup.hpp>
#include <boost/log/expressions/predicates/channel_severity_filter.hpp>

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", XLog::Severity)
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)

#ifdef XLOG_ENABLE_EXTERNAL_LOG_CONTROL
#include "xlog_grpc.noexport.h"
#include <grpcpp/server_builder.h>
static xlog_grpc_server xlog_log_control;
static std::unique_ptr<grpc::Server> ServerPointer;
#endif // XLOG_ENABLE_EXTERNAL_LOG_CONTROL


#ifdef XLOG_USE_SYSLOG_LOG
#include <boost/log/sinks/syslog_backend.hpp>
static boost::shared_ptr<boost::log::sinks::synchronous_sink<boost::log::sinks::syslog_backend>> SYSLOG_BACKEND_PTR;
static boost::log::sinks::syslog::custom_severity_mapping<XLog::Severity> SYSLOG_SEV_MAPPER("Severity");
#endif //XLOG_USE_SYSLOG_LOG

#ifdef XLOG_USE_JOURNAL_LOG
#include "xlog_journal.noexport.h"
static boost::shared_ptr<boost::log::sinks::synchronous_sink<xlog_journal_backend>> JOURNAL_BACKEND_PTR;
#endif // XLOG_USE_JOURNAL_LOG

#include "xlog_log_internal.noexport.h"

typedef boost::log::expressions::channel_severity_filter_actor<std::string, XLog::Severity> min_severity_filter;
min_severity_filter& get_sev_filter()
{
    static min_severity_filter filter = boost::log::expressions::channel_severity_filter(channel, severity);
    return filter;
}

struct LoggerInformation
{
    XLog::LoggerType logger;

    std::string channel;
    XLog::Severity severity;
};

typedef std::unordered_map<std::string, LoggerInformation, XLog::StringHash, std::equal_to<>> LoggerMap;
static std::mutex _LoggerMutex;

static std::atomic<XLog::Severity> _DefaultSeverity = XLog::Severity::INFO;

static LoggerMap& GetLoggerMap() noexcept
{
    static LoggerMap map;
    return map;
}

#define GET_LOGGER_MAP(varname) std::scoped_lock lock(_LoggerMutex); LoggerMap& varname = GetLoggerMap();

std::string XLog::GetSeverityString(Severity sev) noexcept
{
    switch (sev)
    {
        case Severity::INFO:
            return "INFO";
        case Severity::DEBUG:
        case Severity::DEBUG2:
            return "DEBUG";
        case Severity::WARNING:
        case Severity::WARNING2:
            return "WARNING";
        case Severity::ERROR:
        case Severity::ERROR2:
            return "ERROR";
        case Severity::FATAL:
            return "FATAL";
        case Severity::INTERNAL:
            return "INTERNAL";
    }

    return "???";
}

// For atexit()
void call_exit()
{
    XLog::ShutownLogging(-1);
}

XLog::LoggerType& XLog::GetNamedLogger(const std::string_view channel) noexcept
{
    if(channel.compare(INTERNAL_LOGGER_NAME) == 0)
    {
        return INTERNAL_LOGGER;
    }

    GET_LOGGER_MAP(LoggerList)

#if __cplusplus >= 202003L
    auto found = LoggerList.find(channel);
#else
    auto found = LoggerList.find(std::string(channel));
#endif
    if (found == LoggerList.end())
    {
        std::string channelString(channel);
        std::pair<decltype(found), bool> emplaced = LoggerList.emplace(channelString, 
        LoggerInformation
        {
            .logger = LoggerType{boost::log::keywords::channel = channelString},
            .channel = channelString,
            .severity = _DefaultSeverity
        });

        if (emplaced.second)
        {
            min_severity_filter& filter = get_sev_filter();
            filter[channelString] = _DefaultSeverity;
            return emplaced.first->second.logger;
        }
        else
        {
            // TODO - Is there a better way to handle this?
            INTERNAL() << "Failed to put new logger into map - terminating";
            exit(1);
        }
    }
    else
    {
        return found->second.logger;
    }
}

void XLog::InitializeLogging(LogSettings settings)
{
    static bool isInitialized = false;

    if(!isInitialized)
    {
        isInitialized = true;
        LOGGER_SETTINGS = std::move(settings);
        {
            _DefaultSeverity.store(LOGGER_SETTINGS.s_default_level);
        }

        boost::log::add_console_log(std::clog, boost::log::keywords::format = &XLogFormatters::default_formatter);

#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
        boost::log::core::get()->add_global_attribute("SourceLocation", boost::log::attributes::mutable_constant<std::source_location>(std::source_location::current()));
#endif

        boost::log::add_common_attributes();

#ifdef XLOG_USE_SYSLOG_LOG
    #ifdef BOOST_LOG_USE_NATIVE_SYSLOG
        #define _XLOG_SET_IMPL boost::log::keywords::use_impl = boost::log::sinks::syslog::impl_types::native
    #else
        #define _XLOG_SET_IMPL boost::log::keywords::use_impl = boost::log::sinks::syslog::impl_types::udp_socket_based
    #endif // BOOST_LOG_USE_NATIVE_SYSLOG
        if(LOGGER_SETTINGS.s_syslog.enabled)
        {
            SYSLOG_SEV_MAPPER[XLog::Severity::INFO] = boost::log::sinks::syslog::level::debug;
            SYSLOG_SEV_MAPPER[XLog::Severity::DEBUG] = boost::log::sinks::syslog::level::info;
            SYSLOG_SEV_MAPPER[XLog::Severity::DEBUG2] = boost::log::sinks::syslog::level::info;
            SYSLOG_SEV_MAPPER[XLog::Severity::WARNING] = boost::log::sinks::syslog::level::warning;
            SYSLOG_SEV_MAPPER[XLog::Severity::WARNING2] = boost::log::sinks::syslog::level::warning;
            SYSLOG_SEV_MAPPER[XLog::Severity::ERROR] = boost::log::sinks::syslog::level::error;
            SYSLOG_SEV_MAPPER[XLog::Severity::ERROR2] = boost::log::sinks::syslog::level::error;
            SYSLOG_SEV_MAPPER[XLog::Severity::FATAL] = boost::log::sinks::syslog::level::critical;
            SYSLOG_SEV_MAPPER[XLog::Severity::INTERNAL] = boost::log::sinks::syslog::level::alert;

            SYSLOG_BACKEND_PTR = boost::shared_ptr<boost::log::sinks::synchronous_sink<boost::log::sinks::syslog_backend>>(new boost::log::sinks::synchronous_sink<boost::log::sinks::syslog_backend>(_XLOG_SET_IMPL, boost::log::keywords::facility = LOGGER_SETTINGS.s_syslog.facility));
            SYSLOG_BACKEND_PTR->locked_backend()->set_severity_mapper(SYSLOG_SEV_MAPPER);
            SYSLOG_BACKEND_PTR->set_formatter(&XLogFormatters::default_formatter);

            boost::log::core::get()->add_sink(SYSLOG_BACKEND_PTR);
            INTERNAL() << "Added syslog backed";
        }
    #undef _XLOG_SET_IMPL
#endif // XLOG_USE_SYSLOG_LOG

#ifdef XLOG_USE_JOURNAL_LOG
        if(LOGGER_SETTINGS.s_journal.enabled)
        {
            JOURNAL_BACKEND_PTR = boost::shared_ptr<boost::log::sinks::synchronous_sink<xlog_journal_backend>>(new boost::log::sinks::synchronous_sink<xlog_journal_backend>);

            boost::log::core::get()->add_sink(JOURNAL_BACKEND_PTR);
            INTERNAL() << "Added journal backed";
        }
#endif // XLOG_USE_JOURNAL_LOG

#ifdef XLOG_ENABLE_EXTERNAL_LOG_CONTROL
        // Dirty solution that lets us "break" from this part of the setup at any time
        bool XLOG_EXTERNAL_CONTROL_SUCCESS = false;
        while(true)
        {
            if(!LOGGER_SETTINGS.s_external_control.enabled)
            {
                INTERNAL() << "xlog external control is not enabled, aborting setup";
                XLOG_EXTERNAL_CONTROL_SUCCESS = true;
                break;
            }

            if(!TRY_SETUP_THIS_PROGRAM_SOCKET())
            {
                INTERNAL() << "Failed to setup environment for xlog socket";
                break;
            }

            grpc::ServerBuilder builder;
            builder.RegisterService(&xlog_log_control);
            builder.AddListeningPort(fmt::format("unix://{0}", GET_THIS_PROGRAM_LOG_SOCKET_LOCATION()), grpc::InsecureServerCredentials());
            ServerPointer = builder.BuildAndStart();

            if(LOGGER_SETTINGS.s_external_control.allow_anyone_access)
            {
                // Make sure log socket is accessible by anyone
                if(chmod(GET_THIS_PROGRAM_LOG_SOCKET_LOCATION().c_str(), (S_IRUSR | S_IWUSR) | (S_IRGRP | S_IWGRP) | (S_IROTH | S_IWOTH)) < 0)
                {
                    INTERNAL_ERRNO() << "; Failed to modify permissions for xlog socket";

                    // Only break if setup failures are fatal, since this is a somewhat recoverable error
                    if(LOGGER_SETTINGS.s_external_control.setup_failure_is_fatal)
                    {
                        break;
                    }
                }
            }

            if(atexit(call_exit) != 0)
            {
                INTERNAL() << "Failed to set atexit() for xlog";
                break;
            }

            XLOG_EXTERNAL_CONTROL_SUCCESS = true;
            break;
        }
        if(!XLOG_EXTERNAL_CONTROL_SUCCESS && LOGGER_SETTINGS.s_external_control.setup_failure_is_fatal)
        {
            // TODO - Probably a better way to do this
            exit(1);
        }
#endif // XLOG_ENABLE_EXTERNAL_LOG_CONTROL
    }
}

void XLog::ShutownLogging(int signal)
{
#ifdef XLOG_ENABLE_EXTERNAL_LOG_CONTROL
    if(LOGGER_SETTINGS.s_external_control.enabled)
    {
        if(ServerPointer)
        {
            ServerPointer->Shutdown();
        }
        TRY_SHUTDOWN_THIS_PROGRAM_SOCKET();
    }
#endif // XLOG_ENABLE_EXTERNAL_LOG_CONTROL
}

void XLog::SetGlobalLoggingLevel(XLog::Severity sev)
{
    GET_LOGGER_MAP(all_loggers)
    _DefaultSeverity.store(sev);
    min_severity_filter& filter = get_sev_filter();

    for(const auto& [key, value] : all_loggers)
    {
        filter[key] = sev;
    }

    boost::log::core::get()->set_filter(filter);
}

bool XLog::SetLoggingLevel(XLog::Severity sev, const std::string_view channel)
{
    GET_LOGGER_MAP(all_loggers)
#if __cplusplus >= 202003L
    auto found = all_loggers.find(channel);
#else
    auto found = all_loggers.find(std::string(channel));
#endif

    if(found != all_loggers.end())
    {
        found->second.severity = sev;

        min_severity_filter& filter = get_sev_filter();
        filter[std::string(channel)] = sev;
        boost::log::core::get()->set_filter(filter);

        return true;
    }
    else
    {
        return false;
    }
}

XLog::Severity XLog::GetGlobalLoggingLevel()
{
    return _DefaultSeverity;
}

XLog::Severity XLog::GetLoggingLevel(const std::string_view channel)
{
    GET_LOGGER_MAP(all_loggers)

#if __cplusplus >= 202003L
    auto found = all_loggers.find(channel);
#else
    auto found = all_loggers.find(std::string(channel));
#endif

    if(found == all_loggers.end())
    {
        return _DefaultSeverity;
    }
    else
    {
        return found->second.severity;
    }
}

std::unordered_map<std::string, XLog::Severity> XLog::GetAllLoggingLevels()
{
    GET_LOGGER_MAP(all_loggers)

    std::unordered_map<std::string, XLog::Severity> rValue;
    for(const auto& [key, value] : all_loggers)
    {
        rValue[key] = value.severity;
    }

    return rValue;
}

std::vector<std::string> XLog::GetAllLogHandles()
{
    GET_LOGGER_MAP(all_loggers)

    std::vector<std::string> rValue(all_loggers.size(), std::string{});
    for(const auto& [key, value] : all_loggers)
    {
        rValue.push_back(key);
    }

    return rValue;
}

#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
XLog::fatal_exception::fatal_exception(XLog::LoggerType& logger, const std::string& what_arg, const std::source_location sloc) : std::runtime_error(what_arg)
{
    print_fatal(logger, sloc);
}

XLog::fatal_exception::fatal_exception(XLog::LoggerType& logger, const char* what_arg, const std::source_location sloc) : std::runtime_error(what_arg)
{
    print_fatal(logger, sloc);
}

void XLog::fatal_exception::print_fatal(XLog::LoggerType& logger, const std::source_location sloc) const
{
    CUSTOM_LOG_SEV_SLOC(logger, XLog::Severity::FATAL, sloc) << what();
}
#else
XLog::fatal_exception::fatal_exception(XLog::LoggerType& logger, const std::string& what_arg) : std::runtime_error(what_arg)
{
    print_fatal(logger);
}

XLog::fatal_exception::fatal_exception(XLog::LoggerType& logger, const char* what_arg) : std::runtime_error(what_arg)
{
    print_fatal(logger);
}

void XLog::fatal_exception::print_fatal(XLog::LoggerType& logger) const
{
    CUSTOM_LOG_SEV(logger, XLog::Severity::FATAL) << what();
}
#endif

#include <boost/date_time/posix_time/posix_time.hpp>

#include <filesystem>

std::string get_file_name(const std::string_view path)
{
    return std::filesystem::path(path).filename();
}

void XLogFormatters::default_formatter(const boost::log::record_view& rec, boost::log::formatting_ostream& stream)
{
    auto timestamp = boost::log::extract<boost::posix_time::ptime>("TimeStamp", rec);
    auto severity = boost::log::extract<XLog::Severity>("Severity", rec);
    auto channel = boost::log::extract<std::string>("Channel", rec);
    auto message = boost::log::extract<std::string>("Message", rec);

    stream  << boost::posix_time::to_simple_string(timestamp.get()) << ' '
            << '<' << XLog::GetSeverityString(severity.get()) << "> "
            << '[' << channel.get() << "] - ";

#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
    if(severity != XLog::Severity::INFO &&
       severity != XLog::Severity::DEBUG2 &&
       severity != XLog::Severity::WARNING2 &&
       severity != XLog::Severity::ERROR2)
    {
        auto slc = boost::log::extract<std::source_location>("SourceLocation", rec);
        if(slc.empty())
        {
            stream << "[Error: Could not retrieve source line] - ";
        }
        else
        {
            auto source_loc = slc.get();
            stream << "[" << source_loc.function_name() << ", " << get_file_name(source_loc.file_name()) << ':' << source_loc.line() << "] - ";
        }
    }
#endif
    stream << message.get();
}
