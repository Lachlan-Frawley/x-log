#include "xlog.h"

#ifdef XLOG_ENABLE_EXTERNAL_LOG_CONTROL
#include "xlog_grpc.noexport.h"
#include <grpcpp/server_builder.h>
static xlog_grpc_server xlog_log_control;
static std::unique_ptr<grpc::Server> ServerPointer;
#endif // XLOG_ENABLE_EXTERNAL_LOG_CONTROL

#include <stdlib.h>
#include <sys/stat.h>

#include <mutex>
#include <atomic>
#include <unordered_map>

#include <boost/log/utility/setup.hpp>
#include <boost/log/expressions/predicates/channel_severity_filter.hpp>

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", XLog::Severity)
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)

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
            exit(1);
        }
    }
    else
    {
        return found->second.logger;
    }
}

void XLog::InitializeLogging()
{
    static bool isInitialized = false;

    if(!isInitialized)
    {
        isInitialized = true;

        boost::log::add_console_log(std::cout, boost::log::keywords::format = &XLogFormatters::default_formatter);

#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
        boost::log::core::get()->add_global_attribute("SourceLocation", boost::log::attributes::mutable_constant<std::source_location>(std::source_location::current()));
#endif

        boost::log::add_common_attributes();

#ifdef XLOG_ENABLE_EXTERNAL_LOG_CONTROL
        if(!TRY_SETUP_THIS_PROGRAM_SOCKET())
        {
            // TODO - Error
            return;
        }

        grpc::ServerBuilder builder;
        builder.RegisterService(&xlog_log_control);
        builder.AddListeningPort(fmt::format("unix://{0}", GET_THIS_PROGRAM_LOG_SOCKET_LOCATION()), grpc::InsecureServerCredentials());
        ServerPointer = builder.BuildAndStart();
        //chmod(GET_THIS_PROGRAM_LOG_SOCKET_LOCATION().c_str(), (S_IRUSR | S_IWUSR) | (S_IRGRP | S_IWGRP) | (S_IROTH | S_IWOTH));

        if(atexit(call_exit) != 0)
        {
            // TODO - Error
            return;
        }
#endif // XLOG_ENABLE_EXTERNAL_LOG_CONTROL
    }
}

void XLog::ShutownLogging(int signal)
{
#ifdef XLOG_ENABLE_EXTERNAL_LOG_CONTROL
    if(ServerPointer)
    {
        ServerPointer->Shutdown();
    }
    TRY_SHUTDOWN_THIS_PROGRAM_SOCKET();
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

bool XLog::SetLoggingLevel(XLog::Severity sev, std::string_view channel)
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

XLog::Severity XLog::GetLoggingLevel(std::string_view channel)
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

std::string get_file_name(const std::string& path)
{
#ifdef __linux__
    return { path.begin() + static_cast<int>(1 + path.find_last_of('/')), path.end() };
#elif _WIN32
    return { path.begin() + static_cast<int>(1 + path.find_last_of('\\')), path.end() };
#else
#error Your platform does not seem to be Windows or Linux; please add the relevant entry in log_formatters...
#endif
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
