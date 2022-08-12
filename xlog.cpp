#include "xlog.h"

#include <unordered_map>
#include <mutex>

#include <boost/log/utility/setup.hpp>

std::string XLog::GetSeverityString(Severity sev) noexcept
{
    switch (sev)
    {
        case Severity::INFO:
            return "INFO";
        case Severity::DEBUG:
            return "DEBUG";
        case Severity::WARNING:
        case Severity::WARNING2:
            return "WARNING";
        case Severity::ERROR:
            return "ERROR";
        case Severity::FATAL:
            return "FATAL";
    }

    return "???";
}

XLog::LoggerType& XLog::GetNamedLogger(const std::string_view channel) noexcept
{
    static std::mutex LoggerMutex;
    static std::unordered_map<std::string, XLog::LoggerType, StringHash, std::equal_to<>> LoggerList; // NOLINT(cert-err58-cpp)

    std::scoped_lock lock(LoggerMutex);

#if CXX_VERSION >= 20L
    auto found = LoggerList.find(channel);
#else
    auto found = LoggerList.find(std::string(channel));
#endif
    if (found == LoggerList.end())
    {
        std::string channelString(channel);
        std::pair<decltype(found), bool> emplaced = LoggerList.emplace(channelString, boost::log::keywords::channel = channelString);
        if (emplaced.second)
        {
            return emplaced.first->second;
        }
        else
        {
            // TODO - Is there a better way to handle this?
            exit(1);
        }
    }
    else
    {
        return found->second;
    }
}

void XLog::InitializeLogging()
{
    static bool isInitialized = false;

    if(!isInitialized)
    {
        isInitialized = true;

        boost::log::add_console_log(std::cout, boost::log::keywords::format = &XLogFormatters::default_formatter);

#ifdef LOGGING_USE_SOURCE_LOCATION
        boost::log::core::get()->add_global_attribute("SourceLocation", boost::log::attributes::mutable_constant<std::source_location>(std::source_location::current()));
#endif

        boost::log::add_common_attributes();
    }
}

#ifdef LOGGING_USE_SOURCE_LOCATION
XLog::fatal_exception::fatal_exception(XLog::LoggerType& logger, const std::string& what_arg, std::source_location sloc) : std::runtime_error(what_arg)
{
    print_fatal(logger, sloc);
}

XLog::fatal_exception::fatal_exception(XLog::LoggerType& logger, const char* what_arg, std::source_location sloc) : std::runtime_error(what_arg)
{
    print_fatal(logger, sloc);
}

void XLog::fatal_exception::print_fatal(XLog::LoggerType& logger, std::source_location sloc) const
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

#ifdef LOGGING_USE_SOURCE_LOCATION
    if(severity > XLog::Severity::INFO && severity != XLog::Severity::WARNING2)
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
