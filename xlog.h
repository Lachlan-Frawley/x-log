#pragma once

#ifndef _XLOG_H
#define _XLOG_H

#include <errno.h>
#include <string.h>

#include <string>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>

#include <fmt/core.h>

#ifdef __cpp_lib_source_location
#ifdef XLOG_USE_SOURCE_LOCATION_IF_AVAILABLE

#define XLOG_LOGGING_USE_SOURCE_LOCATION
#include <source_location>

#endif
#endif

namespace XLog
{
    /*
     * Why are there DEBUGS, WARNS, and ERRORS where we don't care about source location?
     *
     * Well sometimes a log is VERY unique so we don't need it since it's
     * clear where to look for it!
     *
     * While I do sometimes find source location annoying, I recognize that a blanket
     * option for ON/OFF isn't really suitable, so having multiple macros that
     * allow you to conditionally apply that is useful in my opinion.
     *
     * Of course if you aren't using C++ 20 then it means nothing, but perhaps it can
     * be argued that using the correct macros makes the eventual transition easier?
     *
     * I doubt I'll ever allow FATAL to not have a source location, they should be fairly rare
     * outside of very specific situations where the error basically stops the program from
     * executing, so even though that satisfies the "uniqueness" criteria I mentioned before,
     * I still think source location is 'correct' (whatever that means...)
     *
     */
    enum class Severity
    {
        INFO = 0,
        DEBUG,
        DEBUG2, // For debugs where we don't really need to know the source location (if enabled)
        WARNING,
        WARNING2, // For warnings where we don't really need to know the source location (if enabled)
        ERROR,
        ERROR2, // For errors where we don't really need to know the source location (if enabled)
        FATAL,
        INTERNAL // For xlog itself, can never be disabled (knowing why your logger failed is *really* important)
    };

    typedef boost::log::sources::severity_channel_logger_mt<Severity, std::string> LoggerType;

    std::string GetSeverityString(Severity sev) noexcept;
    LoggerType& GetNamedLogger(const std::string_view channel) noexcept;
    void InitializeLogging();
    void ShutownLogging(int signal = -1);

    void SetGlobalLoggingLevel(Severity sev);
    bool SetLoggingLevel(Severity sev, const std::string_view channel);

    Severity GetGlobalLoggingLevel();
    Severity GetLoggingLevel(const std::string_view channel);

    std::unordered_map<std::string, Severity> GetAllLoggingLevels();
    std::vector<std::string> GetAllLogHandles();
}

// Set attribute and return the new value
template<typename ValueType>
ValueType set_get_attrib(const char* name, ValueType value) {
    auto attr = boost::log::attribute_cast<boost::log::attributes::mutable_constant<ValueType>>(boost::log::core::get()->get_global_attributes()[name]);
    attr.set(value);
    return attr.get();
}

#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
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

inline std::string get_errno_string()
{
    constexpr int BUFFER_SIZE = 256;

    char buffer[BUFFER_SIZE];
    char* realBuffer = ::strerror_r(errno, buffer, BUFFER_SIZE - 1);

    return std::string(realBuffer);
}

#define ERRNO_STREAM get_errno_string()

#define LOG_INFO() CUSTOM_LOG_SEV(__logger, XLog::Severity::INFO)
#define LOG_DEBUG() CUSTOM_LOG_SEV(__logger, XLog::Severity::DEBUG)
#define LOG_DEBUG2() CUSTOM_LOG_SEV(__logger, XLog::Severity::DEBUG2)
#define LOG_WARN() CUSTOM_LOG_SEV(__logger, XLog::Severity::WARNING)
#define LOG_WARN2() CUSTOM_LOG_SEV(__logger, XLog::Severity::WARNING2)
#define LOG_ERROR() CUSTOM_LOG_SEV(__logger, XLog::Severity::ERROR)
#define LOG_ERROR2() CUSTOM_LOG_SEV(__logger, XLog::Severity::ERROR2)

#define CODE_INFO(errc) LOG_INFO() << ERRC_STREAM(errc)
#define CODE_DEBUG(errc) LOG_DEBUG() << ERRC_STREAM(errc)
#define CODE_DEBUG2(errc) LOG_DEBUG2() << ERRC_STREAM(errc)
#define CODE_WARN(errc) LOG_WARN() << ERRC_STREAM(errc)
#define CODE_WARN2(errc) LOG_WARN2() << ERRC_STREAM(errc)
#define CODE_ERROR(errc) LOG_ERROR() << ERRC_STREAM(errc)
#define CODE_ERROR2(errc) LOG_ERROR2() << ERRC_STREAM(errc)

#define LOG_INFO_INPLACE(name) CUSTOM_LOG_SEV(XLog::GetNamedLogger(name), XLog::Severity::INFO)
#define LOG_DEBUG_INPLACE(name) CUSTOM_LOG_SEV(XLog::GetNamedLogger(name), XLog::Severity::DEBUG)
#define LOG_DEBUG2_INPLACE(name) CUSTOM_LOG_SEV(XLog::GetNamedLogger(name), XLog::Severity::DEBUG2)
#define LOG_WARN_INPLACE(name) CUSTOM_LOG_SEV(XLog::GetNamedLogger(name), XLog::Severity::WARNING)
#define LOG_WARN2_INPLACE(name) CUSTOM_LOG_SEV(XLog::GetNamedLogger(name), XLog::Severity::WARNING2)
#define LOG_ERROR_INPLACE(name) CUSTOM_LOG_SEV(XLog::GetNamedLogger(name), XLog::Severity::ERROR)
#define LOG_ERROR2_INPLACE(name) CUSTOM_LOG_SEV(XLog::GetNamedLogger(name), XLog::Severity::ERROR2)

#define CODE_INFO_INPLACE(name, errc) LOG_INFO_INPLACE(name) << ERRC_STREAM(errc)
#define CODE_DEBUG_INPLACE(name, errc) LOG_DEBUG_INPLACE(name) << ERRC_STREAM(errc)
#define CODE_DEBUG2_INPLACE(name, errc) LOG_DEBUG2_INPLACE(name) << ERRC_STREAM(errc)
#define CODE_WARN_INPLACE(name, errc) LOG_WARN_INPLACE(name) << ERRC_STREAM(errc)
#define CODE_WARN2_INPLACE(name, errc) LOG_WARN2_INPLACE(name) << ERRC_STREAM(errc)
#define CODE_ERROR_INPLACE(name, errc) LOG_ERROR_INPLACE(name) << ERRC_STREAM(errc)
#define CODE_ERROR2_INPLACE(name, errc) LOG_ERROR2_INPLACE(name) << ERRC_STREAM(errc)

#define ERRNO_INFO() LOG_INFO() << ERRNO_STREAM
#define ERRNO_DEBUG() LOG_DEBUG() << ERRNO_STREAM
#define ERRNO_DEBUG2() LOG_DEBUG2() << ERRNO_STREAM
#define ERRNO_WARN() LOG_WARN() << ERRNO_STREAM
#define ERRNO_WARN2() LOG_WARN2() << ERRNO_STREAM
#define ERRNO_ERROR() LOG_ERROR() << ERRNO_STREAM
#define ERRNO_ERROR2() LOG_ERROR2() << ERRNO_STREAM

#define ERRNO_INFO_INPLACE(name) LOG_INFO_INPLACE(name) << ERRNO_STREAM
#define ERRNO_DEBUG_INPLACE(name) LOG_DEBUG_INPLACE(name) << ERRNO_STREAM
#define ERRNO_DEBUG2_INPLACE(name) LOG_DEBUG2_INPLACE(name) << ERRNO_STREAM
#define ERRNO_WARN_INPLACE(name) LOG_WARN_INPLACE(name) << ERRNO_STREAM
#define ERRNO_WARN2_INPLACE(name) LOG_WARN2_INPLACE(name) << ERRNO_STREAM
#define ERRNO_ERROR_INPLACE(name) LOG_ERROR_INPLACE(name) << ERRNO_STREAM
#define ERRNO_ERROR2_INPLACE(name) LOG_ERROR2_INPLACE(name) << ERRNO_STREAM

namespace XLog
{
    class fatal_exception : public std::runtime_error
    {
    public:
    #ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
        explicit fatal_exception(XLog::LoggerType& logger, const std::string& what_arg, const std::source_location sloc = std::source_location::current());
        explicit fatal_exception(XLog::LoggerType& logger, const char* what_arg, const std::source_location sloc = std::source_location::current());

        template<typename... FormatArgs>
        explicit fatal_exception(XLog::LoggerType& logger, const std::source_location sloc, std::string_view format, FormatArgs&&... args) : std::runtime_error(fmt::vformat(format, fmt::make_format_args(std::forward<FormatArgs>(args)...)))
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
    #ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
        void print_fatal(XLog::LoggerType& logger, const std::source_location sloc) const;
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
#define FATAL(msg) throw XLog::fatal_exception(__logger, msg);
#define CODE_FATAL(errc) FATAL(ERRC_MSG(errc))
#define FATAL_NAMED(name, msg) throw XLog::fatal_exception(XLog::GetNamedLogger(name), msg);

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

#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
#define FATAL_FMT(fmt, ...) throw XLog::fatal_exception(__logger, std::source_location::current(), fmt, __VA_ARGS__);
#define FATAL_NAMED_FMT(name, fmt, ...) throw XLog::fatal_exception(XLog::GetNamedLogger(name), std::source_location::current(), fmt, __VA_ARGS__);
#define FATAL_GLOBAL_FMT(fmt, ...) FATAL_NAMED_FMT("Global", std::source_location::current(), fmt, __VA_ARGS__)
#else
#define FATAL_FMT(fmt, ...) throw XLog::fatal_exception(__logger, fmt, __VA_ARGS__);
#define FATAL_NAMED_FMT(name, fmt, ...) throw XLog::fatal_exception(XLog::GetNamedLogger(name), fmt, __VA_ARGS__);
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

#endif // _XLOG_H
