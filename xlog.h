#pragma once

/*
 * Because the logging header should be included 
 * pretty much everywhere, we can use it to
 * disable optimization everywhere...
 */
#ifdef __XLOG_DebugMode
#pragma optimize("", off)
#endif

#ifdef __linux__
#include <errno.h>
#include <string.h>
#endif

#include <string>
#include <string_view>
#include <type_traits>
#include <stdexcept>

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

#include <fmt/core.h>

#ifdef __cpp_lib_source_location
#define LOGGING_USE_SOURCE_LOCATION
#include <source_location>
#endif

namespace XLog
{
    enum class Severity
    {
        INFO,
        WARNING2, // For warnings where we don't really need to know the source location (if enabled)
        DEBUG,
        WARNING,
        ERROR,
        FATAL
    };

    typedef boost::log::sources::severity_channel_logger_mt<Severity, std::string> LoggerType;

    inline std::string GetSeverityString(Severity sev) noexcept;
    LoggerType& GetNamedLogger(const std::string_view channel) noexcept;
    void InitializeLogging();
}

// Set attribute and return the new value
template<typename ValueType>
ValueType set_get_attrib(const char* name, ValueType value) {
    auto attr = boost::log::attribute_cast<boost::log::attributes::mutable_constant<ValueType>>(boost::log::core::get()->get_global_attributes()[name]);
    attr.set(value);
    return attr.get();
}

#ifdef LOGGING_USE_SOURCE_LOCATION
#define CUSTOM_LOG_SEV_SLOC(logger, sev, sloc) \
   BOOST_LOG_STREAM_WITH_PARAMS( \
      (logger), \
         (set_get_attrib("SourceLocation", sloc)) \
         (::boost::log::keywords::severity = (sev)) \
   )
#define CUSTOM_LOG_SEV(logger, sev) CUSTOM_LOG_SEV_SLOC(logger, sev, std::source_location::current())
#else
#define CUSTOM_LOG_SEV(logger, sev) BOOST_LOG_SEV(logger, sev)
#endif

#define PRINT_ENUM(var) static_cast<std::underlying_type_t<decltype(var)>>(var)

#define GET_LOGGER(name) static XLog::LoggerType& __logger = XLog::GetNamedLogger(name);

#define ERRC_STREAM(errc) errc.message() << std::endl

#ifdef __linux__

inline std::string get_errno_string()
{
    constexpr int BUFFER_SIZE = 256;

    char buffer[BUFFER_SIZE];
    char* realBuffer = ::strerror_r(errno, buffer, BUFFER_SIZE - 1);

    return std::string(realBuffer);
}

#define ERRNO_STREAM get_errno_string()
#endif

#define LOG_INFO() CUSTOM_LOG_SEV(__logger, XLog::Severity::INFO)
#define LOG_DEBUG() CUSTOM_LOG_SEV(__logger, XLog::Severity::DEBUG)
#define LOG_WARN() CUSTOM_LOG_SEV(__logger, XLog::Severity::WARNING)
#define LOG_WARN2() CUSTOM_LOG_SEV(__logger, XLog::Severity::WARNING2)
#define LOG_ERROR() CUSTOM_LOG_SEV(__logger, XLog::Severity::ERROR)

#define CODE_INFO(errc) LOG_INFO() << ERRC_STREAM(errc)
#define CODE_DEBUG(errc) LOG_DEBUG() << ERRC_STREAM(errc)
#define CODE_WARN(errc) LOG_WARN() << ERRC_STREAM(errc)
#define CODE_WARN2(errc) LOG_WARN2() << ERRC_STREAM(errc)
#define CODE_ERROR(errc) LOG_ERROR() << ERRC_STREAM(errc)

#define LOG_INFO_INPLACE(name) CUSTOM_LOG_SEV(XLog::GetNamedLogger(name), XLog::Severity::INFO)
#define LOG_DEBUG_INPLACE(name) CUSTOM_LOG_SEV(XLog::GetNamedLogger(name), XLog::Severity::DEBUG)
#define LOG_WARN_INPLACE(name) CUSTOM_LOG_SEV(XLog::GetNamedLogger(name), XLog::Severity::WARNING)
#define LOG_WARN2_INPLACE(name) CUSTOM_LOG_SEV(XLog::GetNamedLogger(name), XLog::Severity::WARNING2)
#define LOG_ERROR_INPLACE(name) CUSTOM_LOG_SEV(XLog::GetNamedLogger(name), XLog::Severity::ERROR)

#define CODE_INFO_INPLACE(name, errc) LOG_INFO_INPLACE(name) << ERRC_STREAM(errc)
#define CODE_DEBUG_INPLACE(name, errc) LOG_DEBUG_INPLACE(name) << ERRC_STREAM(errc)
#define CODE_WARN_INPLACE(name, errc) LOG_WARN_INPLACE(name) << ERRC_STREAM(errc)
#define CODE_WARN2_INPLACE(name, errc) LOG_WARN2_INPLACE(name) << ERRC_STREAM(errc)
#define CODE_ERROR_INPLACE(name, errc) LOG_ERROR_INPLACE(name) << ERRC_STREAM(errc)

#ifdef __linux__
#define ERRNO_INFO() LOG_INFO() << ERRNO_STREAM
#define ERRNO_DEBUG() LOG_DEBUG() << ERRNO_STREAM
#define ERRNO_WARN() LOG_WARN() << ERRNO_STREAM
#define ERRNO_ERROR() LOG_ERROR() << ERRNO_STREAM

#define ERRNO_INFO_INPLACE(name) LOG_INFO_INPLACE(name) << ERRNO_STREAM
#define ERRNO_DEBUG_INPLACE(name) LOG_DEBUG_INPLACE(name) << ERRNO_STREAM
#define ERRNO_WARN_INPLACE(name) LOG_WARN_INPLACE(name) << ERRNO_STREAM
#define ERRNO_ERROR_INPLACE(name) LOG_ERROR_INPLACE(name) << ERRNO_STREAM
#endif

namespace XLog
{
    class fatal_exception : public std::runtime_error
    {
    public:
    #ifdef LOGGING_USE_SOURCE_LOCATION
        explicit fatal_exception(XLog::LoggerType& logger, const std::string& what_arg, std::source_location sloc = std::source_location::current());
        explicit fatal_exception(XLog::LoggerType& logger, const char* what_arg, std::source_location sloc = std::source_location::current());

        template<typename... FormatArgs>
        explicit fatal_exception(XLog::LoggerType& logger, std::source_location sloc, std::string_view format, FormatArgs&&... args) : std::runtime_error(fmt::vformat(format, fmt::make_format_args(std::forward<FormatArgs>(args)...)))
        {
            print_fatal(logger, sloc);
        }
    #else
        explicit fatal_exception(XLog::LoggerType& logger, const std::string& what_arg);
        explicit fatal_exception(XLog::LoggerType& logger, const char* what_arg);

        template<typename... FormatArgs>
        explicit fatal_exception(XLog::LoggerType& logger, std::string_view format, FormatArgs&&... args) : std::runtime_error(fmt::vformat(format, fmt::make_format_args(std::forward<FormatArgs>(args)...)))
        {
            print_fatal(logger);
        }
    #endif

    private:
    #ifdef LOGGING_USE_SOURCE_LOCATION
        void print_fatal(XLog::LoggerType& logger, std::source_location sloc) const;
    #else
        void print_fatal(XLog::LoggerType& logger) const;
    #endif
    };
}

#define ERRC_MSG(errc) errc.message()

/*
 * This should work with std::error_code & boost::system::error_code
 * TODO - Expand
 */
#define FATAL(msg) throw fatal_exception(__logger, msg);
#define CODE_FATAL(errc) FATAL(ERRC_MSG(errc))
#define FATAL_NAMED(name, msg) throw fatal_exception(XLog::GetNamedLogger(name), msg);
#define CODE_FATAL_NAMED(name, errc) FATAL_NAMED(name, ERRC_MSG(errc))
#define FATAL_GLOBAL(msg) FATAL_NAMED("Global", msg)
#define CODE_FATAL_GLOBAL(errc) FATAL_GLOBAL(ERRC_MSG(errc))

/*
 * Log ERRNO messages
 * TODO - Expand
 */
#ifdef __linux__
#define ERRNO_MESSAGE get_errno_string()

#define ERRNO_FATAL() FATAL(ERRNO_MESSAGE)
#define ERRNO_FATAL_NAMED(name) FATAL_NAMED(name, ERRNO_MESSAGE)
#define ERRNO_FATAL_GLOBAL() FATAL_GLOBAL(ERNNO_MESSAGE)
#endif

#ifdef LOGGING_USE_SOURCE_LOCATION
#define FATAL_FMT(fmt, ...) throw fatal_exception(__logger, std::source_location::current(), fmt, __VA_ARGS__);
#define FATAL_NAMED_FMT(name, fmt, ...) throw fatal_exception(XLog::GetNamedLogger(name), std::source_location::current(), fmt, __VA_ARGS__);
#define FATAL_GLOBAL_FMT(fmt, ...) FATAL_NAMED_FMT("Global", std::source_location::current(), fmt, __VA_ARGS__)
#else
#define FATAL_FMT(fmt, ...) throw fatal_exception(__logger, fmt, __VA_ARGS__);
#define FATAL_NAMED_FMT(name, fmt, ...) throw fatal_exception(XLog::GetNamedLogger(name), fmt, __VA_ARGS__);
#define FATAL_GLOBAL_FMT(fmt, ...) FATAL_NAMED_FMT("Global", fmt, __VA_ARGS__)
#endif

/*
 * Default log formatter
 */
namespace XLogFormatters
{
    void default_formatter(const boost::log::record_view& rec, boost::log::formatting_ostream& stream);
}

/*
 * String hashes to allow different "strings" to 
 * be used as keys for the logger map (see source file)
 */
namespace XLog
{
    struct StringHash
    {
        using hash_type = std::hash<std::string_view>;
        using is_transparent = void;

        inline size_t operator()(const char* str) const
        {
            return hash_type{}(str);
        }

        inline size_t operator()(std::string_view str) const
        {
            return hash_type{}(str);
        }

        inline size_t operator()(const std::string& str) const
        {
            return hash_type{}(str);
        }
    };

    struct WStringHash
    {
        using hash_type = std::hash<std::wstring_view>;
        using is_transparent = void;

        inline size_t operator()(const wchar_t* str) const
        {
            return hash_type{}(str);
        }

        inline size_t operator()(std::wstring_view str) const
        {
            return hash_type{}(str);
        }

        inline size_t operator()(const std::wstring& str) const
        {
            return hash_type{}(str);
        }
    };
}
