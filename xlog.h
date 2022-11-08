#pragma once

#ifndef XLOG_H
#define XLOG_H

#include <cerrno>
#include <cstring>

#include <string>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>

#include <fmt/core.h>

#include <boost/preprocessor/facilities/va_opt.hpp>

// Select correct macro format definition so that we can use variadic macros for xlog format logging
#define XLOG_FORMATTER_NOARG(fmat) fmt::format(fmat)
#define XLOG_FORMATTER_DEFAULT(fmat, ...) fmt::format(fmat, __VA_ARGS__)
#define XLOG_FORMATTER_SELECT(fmat, ...) BOOST_PP_VA_OPT((XLOG_FORMATTER_DEFAULT(fmat, __VA_ARGS__)),(XLOG_FORMATTER_NOARG(fmat)),__VA_ARGS__)
#define XLOG_FORMATTER_SELECT1(fmat, arg1, ...) BOOST_PP_VA_OPT((XLOG_FORMATTER_DEFAULT(fmat, arg1, __VA_ARGS__)),(XLOG_FORMATTER_DEFAULT(fmat, arg1)),__VA_ARGS__)

#ifdef XLOG_USE_SOURCE_LOCATION_IF_AVAILABLE
#ifdef __cpp_lib_source_location

#define XLOG_LOGGING_USE_SOURCE_LOCATION
#include <source_location>

#endif
#endif

#ifdef XLOG_ENABLE_EXTERNAL_LOG_CONTROL

namespace xlog
{
    struct ExternalLogControlSettings
    {
        // Is external log control enabled at runtime?
        bool enabled = true;

        // Is the log socket modified so that anyone can connect to it?
        bool allow_anyone_access = true;

        // Is failure to setup the log socket fatal to the application?
        bool setup_failure_is_fatal = false;
    };
}

#endif // XLOG_ENABLE_EXTERNAL_LOG_CONTROL

#ifdef XLOG_USE_SYSLOG_LOG

#include <boost/log/sinks/syslog_constants.hpp>

namespace xlog
{
    struct SyslogSettings
    {
        // Is syslog logging enabled at runtime?
        bool enabled = true;

        // Syslog facility
        boost::log::sinks::syslog::facility facility = boost::log::sinks::syslog::facility::user;
    };
}

#endif // XLOG_USE_SYSLOG_LOG

#ifdef XLOG_USE_JOURNAL_LOG

namespace xlog
{
    struct JournalSettings
    {
        // Is journal logging enabled at runtime?
        bool enabled = true;
    };
}

#endif // XLOG_USE_JOURNAL_LOG

#ifdef XLOG_USE_SIGCPP

namespace xlog
{
    struct SignalSettings
    {
        // Should xlog initialize sigcpp itself?
        // This makes the most sense if you include it as a convenient dependency, and not for your own use
        // Additionally, sigcpp will be initialized with its default settings
        bool initialize_in_xlog = true;
    };
}

#endif // XLOG_USE_SIGCPP

namespace xlog
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

    enum class ConsoleLogLocation
    {
        NONE = 0,
        CLOG = 1,
        CERR = 2,
        COUT = 3
    };

    struct LogSettings
    {
        Severity s_default_level = Severity::INFO;
        ConsoleLogLocation s_consoleStream = ConsoleLogLocation::CLOG;

#ifdef XLOG_ENABLE_EXTERNAL_LOG_CONTROL
        ExternalLogControlSettings s_external_control;
#endif // XLOG_ENABLE_EXTERNAL_LOG_CONTROL
#ifdef XLOG_USE_SYSLOG_LOG
        SyslogSettings s_syslog;
#endif // XLOG_USE_SYSLOG_LOG
#ifdef XLOG_USE_JOURNAL_LOG
        JournalSettings s_journal;
#endif // XLOG_USE_JOURNAL_LOG
#ifdef XLOG_USE_SIGCPP
        SignalSettings s_signal;
#endif // XLOG_USE_SIGCPP
    };

    typedef boost::log::sources::severity_channel_logger_mt<Severity, std::string> LoggerType;

    std::string GetSeverityString(Severity sev) noexcept;
    LoggerType& GetNamedLogger(std::string_view channel) noexcept;

    void InitializeLogging(LogSettings settings = {});
    void ShutownLogging(int signal = -1);

    void SetGlobalLoggingLevel(Severity sev);
    bool SetLoggingLevel(Severity sev, std::string_view channel);

    Severity GetGlobalLoggingLevel();
    Severity GetLoggingLevel(std::string_view channel);

    std::unordered_map<std::string, Severity> GetAllLoggingLevels();
    std::vector<std::string> GetAllLogHandles();
}

// Set attribute and return the new value
// Pretty sure I found this on stackoverflow, but can't recall where
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

//#define XLOG_PRINT_ENUM(var) static_cast<std::underlying_type_t<decltype(var)>>(var)

#define XLOG_LOGGER_VAR_NAME xlog_logger
#define XLOG_GET_LOGGER(name) static xlog::LoggerType& XLOG_LOGGER_VAR_NAME = xlog::GetNamedLogger(name);

#define XLOG_ERRC_VALUE(errc) errc.message()

inline std::string xlog_get_errno_string()
{
    // TODO - Find out max length?
    constexpr int BUFFER_SIZE = 256;

    char buffer[BUFFER_SIZE];
    char* realBuffer = ::strerror_r(errno, buffer, BUFFER_SIZE - 1);

    return { realBuffer };
}

#define XLOG_ERRNO_VALUE xlog_get_errno_string()


// Normal Stream Macros

#define XLOG_INFO CUSTOM_LOG_SEV(XLOG_LOGGER_VAR_NAME, xlog::Severity::INFO)
#define XLOG_DEBUG CUSTOM_LOG_SEV(XLOG_LOGGER_VAR_NAME, xlog::Severity::DEBUG)
#define XLOG_DEBUG2 CUSTOM_LOG_SEV(XLOG_LOGGER_VAR_NAME, xlog::Severity::DEBUG2)
#define XLOG_WARN CUSTOM_LOG_SEV(XLOG_LOGGER_VAR_NAME, xlog::Severity::WARNING)
#define XLOG_WARN2 CUSTOM_LOG_SEV(XLOG_LOGGER_VAR_NAME, xlog::Severity::WARNING2)
#define XLOG_ERROR CUSTOM_LOG_SEV(XLOG_LOGGER_VAR_NAME, xlog::Severity::ERROR)
#define XLOG_ERROR2 CUSTOM_LOG_SEV(XLOG_LOGGER_VAR_NAME, xlog::Severity::ERROR2)


// Inplace Stream Macros

#define XLOG_INFO_I(name) CUSTOM_LOG_SEV(xlog::GetNamedLogger(name), xlog::Severity::INFO)
#define XLOG_DEBUG_I(name) CUSTOM_LOG_SEV(xlog::GetNamedLogger(name), xlog::Severity::DEBUG)
#define XLOG_DEBUG2_I(name) CUSTOM_LOG_SEV(xlog::GetNamedLogger(name), xlog::Severity::DEBUG2)
#define XLOG_WARN_I(name) CUSTOM_LOG_SEV(xlog::GetNamedLogger(name), xlog::Severity::WARNING)
#define XLOG_WARN2_I(name) CUSTOM_LOG_SEV(xlog::GetNamedLogger(name), xlog::Severity::WARNING2)
#define XLOG_ERROR_I(name) CUSTOM_LOG_SEV(xlog::GetNamedLogger(name), xlog::Severity::ERROR)
#define XLOG_ERROR2_I(name) CUSTOM_LOG_SEV(xlog::GetNamedLogger(name), xlog::Severity::ERROR2)


// Normal Format Macros

#define XLOG_INFO_F(fmat, ...) XLOG_INFO << XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__)
#define XLOG_DEBUG_F(fmat, ...) XLOG_DEBUG << XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__)
#define XLOG_DEBUG2_F(fmat, ...) XLOG_DEBUG2 << XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__)
#define XLOG_WARN_F(fmat, ...) XLOG_WARN << XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__)
#define XLOG_WARN2_F(fmat, ...) XLOG_WARN2 << XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__)
#define XLOG_ERROR_F(fmat, ...) XLOG_ERROR << XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__)
#define XLOG_ERROR2_F(fmat, ...) XLOG_ERROR2 << XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__)


// Inplace Format Macros

#define XLOG_INFO_IF(name, fmat, ...) XLOG_INFO_I(name) << XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__)
#define XLOG_DEBUG_IF(name, fmat, ...) XLOG_DEBUG_I(name) << XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__)
#define XLOG_DEBUG2_IF(name, fmat, ...) XLOG_DEBUG2_I(name) << XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__)
#define XLOG_WARN_IF(name, fmat, ...) XLOG_WARN_I(name) << XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__)
#define XLOG_WARN2_IF(name, fmat, ...) XLOG_WARN2_I(name) << XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__)
#define XLOG_ERROR_IF(name, fmat, ...) XLOG_ERROR_I(name) << XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__)
#define XLOG_ERROR2_IF(name, fmat, ...) XLOG_ERROR2_I(name) << XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__)


// Error Code Stream Macros

#define XLOG_INFO_C(errc) XLOG_INFO << XLOG_ERRC_VALUE(errc)
#define XLOG_DEBUG_C(errc) XLOG_DEBUG << XLOG_ERRC_VALUE(errc)
#define XLOG_DEBUG2_C(errc) XLOG_DEBUG2 << XLOG_ERRC_VALUE(errc)
#define XLOG_WARN_C(errc) XLOG_WARN << XLOG_ERRC_VALUE(errc)
#define XLOG_WARN2_C(errc) XLOG_WARN2 << XLOG_ERRC_VALUE(errc)
#define XLOG_ERROR_C(errc) XLOG_ERROR << XLOG_ERRC_VALUE(errc)
#define XLOG_ERROR2_C(errc) XLOG_ERROR2 << XLOG_ERRC_VALUE(errc)


// Inplace Error Code Stream Macros

#define XLOG_INFO_IC(name, errc) XLOG_INFO_I(name) << XLOG_ERRC_VALUE(errc)
#define XLOG_DEBUG_IC(name, errc) XLOG_DEBUG_I(name) << XLOG_ERRC_VALUE(errc)
#define XLOG_DEBUG2_IC(name, errc) XLOG_DEBUG2_I(name) << XLOG_ERRC_VALUE(errc)
#define XLOG_WARN_IC(name, errc) XLOG_WARN_I(name) << XLOG_ERRC_VALUE(errc)
#define XLOG_WARN2_IC(name, errc) XLOG_WARN2_I(name) << XLOG_ERRC_VALUE(errc)
#define XLOG_ERROR_IC(name, errc) XLOG_ERROR_I(name) << XLOG_ERRC_VALUE(errc)
#define XLOG_ERROR2_IC(name, errc) XLOG_ERROR2_I(name) << XLOG_ERRC_VALUE(errc)


// Error Code Format Macros

#define XLOG_INFO_FC(errc, fmat, ...) XLOG_INFO << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRC_VALUE(errc), __VA_ARGS__)
#define XLOG_DEBUG_FC(errc, fmat, ...) XLOG_DEBUG << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRC_VALUE(errc), __VA_ARGS__)
#define XLOG_DEBUG2_FC(errc, fmat, ...) XLOG_DEBUG2 << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRC_VALUE(errc), __VA_ARGS__)
#define XLOG_WARN_FC(errc, fmat, ...) XLOG_WARN << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRC_VALUE(errc), __VA_ARGS__)
#define XLOG_WARN2_FC(errc, fmat, ...) XLOG_WARN2 << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRC_VALUE(errc), __VA_ARGS__)
#define XLOG_ERROR_FC(errc, fmat, ...) XLOG_ERROR << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRC_VALUE(errc), __VA_ARGS__)
#define XLOG_ERROR2_FC(errc, fmat, ...) XLOG_ERROR2 << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRC_VALUE(errc), __VA_ARGS__)


// Inplace Error Code Format Macros

#define XLOG_INFO_IFC(name, errc, fmat, ...) XLOG_INFO_I(name) << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRC_VALUE(errc), __VA_ARGS__)
#define XLOG_DEBUG_IFC(name, errc, fmat, ...) XLOG_DEBUG_I(name) << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRC_VALUE(errc), __VA_ARGS__)
#define XLOG_DEBUG2_IFC(name, errc, fmat, ...) XLOG_DEBUG2_I(name) << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRC_VALUE(errc), __VA_ARGS__)
#define XLOG_WARN_IFC(name, errc, fmat, ...) XLOG_WARN_I(name) << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRC_VALUE(errc), __VA_ARGS__)
#define XLOG_WARN2_IFC(name, errc, fmat, ...) XLOG_WARN2_I(name) << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRC_VALUE(errc), __VA_ARGS__)
#define XLOG_ERROR_IFC(name, errc, fmat, ...) XLOG_ERROR_I(name) << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRC_VALUE(errc), __VA_ARGS__)
#define XLOG_ERROR2_IFC(name, errc, fmat, ...) XLOG_ERROR2_I(name) << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRC_VALUE(errc), __VA_ARGS__)


// Errno Stream Macros

#define XLOG_INFO_E XLOG_INFO << XLOG_ERRNO_VALUE
#define XLOG_DEBUG_E XLOG_DEBUG << XLOG_ERRNO_VALUE
#define XLOG_DEBUG2_E XLOG_DEBUG2 << XLOG_ERRNO_VALUE
#define XLOG_WARN_E XLOG_WARN << XLOG_ERRNO_VALUE
#define XLOG_WARN2_E XLOG_WARN2 << XLOG_ERRNO_VALUE
#define XLOG_ERROR_E XLOG_ERROR << XLOG_ERRNO_VALUE
#define XLOG_ERROR2_E XLOG_ERROR2 << XLOG_ERRNO_VALUE


// Inplace Errno Stream Macros

#define XLOG_INFO_IE(name) XLOG_INFO_I(name) << XLOG_ERRNO_VALUE
#define XLOG_DEBUG_IE(name) XLOG_DEBUG_I(name) << XLOG_ERRNO_VALUE
#define XLOG_DEBUG2_IE(name) XLOG_DEBUG2_I(name) << XLOG_ERRNO_VALUE
#define XLOG_WARN_IE(name) XLOG_WARN_I(name) << XLOG_ERRNO_VALUE
#define XLOG_WARN2_IE(name) XLOG_WARN2_I(name) << XLOG_ERRNO_VALUE
#define XLOG_ERROR_IE(name) XLOG_ERROR_I(name) << XLOG_ERRNO_VALUE
#define XLOG_ERROR2_IE(name) XLOG_ERROR2_I(name) << XLOG_ERRNO_VALUE


// Errno Format Macros

#define XLOG_INFO_FE(fmat, ...) XLOG_INFO << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRNO_VALUE, __VA_ARGS__)
#define XLOG_DEBUG_FE(fmat, ...) XLOG_DEBUG << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRNO_VALUE, __VA_ARGS__)
#define XLOG_DEBUG2_FE(fmat, ...) XLOG_DEBUG2 << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRNO_VALUE, __VA_ARGS__)
#define XLOG_WARN_FE(fmat, ...) XLOG_WARN << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRNO_VALUE, __VA_ARGS__)
#define XLOG_WARN2_FE(fmat, ...) XLOG_WARN2 << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRNO_VALUE, __VA_ARGS__)
#define XLOG_ERROR_FE(fmat, ...) XLOG_ERROR << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRNO_VALUE, __VA_ARGS__)
#define XLOG_ERROR2_FE(fmat, ...) XLOG_ERROR2 << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRNO_VALUE, __VA_ARGS__)


// Inplace Errno Format Macros

#define XLOG_INFO_IFE(name, fmat, ...) XLOG_INFO_I(name) << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRNO_VALUE, __VA_ARGS__)
#define XLOG_DEBUG_IFE(name, fmat, ...) XLOG_DEBUG_I(name) << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRNO_VALUE, __VA_ARGS__)
#define XLOG_DEBUG2_IFE(name, fmat, ...) XLOG_DEBUG2_I(name) << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRNO_VALUE, __VA_ARGS__)
#define XLOG_WARN_IFE(name, fmat, ...) XLOG_WARN_I(name) << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRNO_VALUE, __VA_ARGS__)
#define XLOG_WARN2_IFE(name, fmat, ...) XLOG_WARN2_I(name) << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRNO_VALUE, __VA_ARGS__)
#define XLOG_ERROR_IFE(name, fmat, ...) XLOG_ERROR_I(name) << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRNO_VALUE, __VA_ARGS__)
#define XLOG_ERROR2_IFE(name, fmat, ...) XLOG_ERROR2_I(name) << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRNO_VALUE, __VA_ARGS__)

namespace xlog
{
    class fatal_exception : public std::runtime_error
    {
    public:
    #ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
        explicit fatal_exception(xlog::LoggerType& logger, const std::string& what_arg, std::source_location sloc = std::source_location::current());
        explicit fatal_exception(xlog::LoggerType& logger, const char* what_arg, std::source_location sloc = std::source_location::current());
    #else
        explicit fatal_exception(xlog::LoggerType& logger, const std::string& what_arg);
        explicit fatal_exception(xlog::LoggerType& logger, const char* what_arg);
    #endif

    private:
    #ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
        void print_fatal(xlog::LoggerType& logger, std::source_location sloc) const;
    #else
        void print_fatal(xlog::LoggerType& logger) const;
    #endif
    };
}

#define XLOG_FATAL_ERRC_VALUE(errc) errc.message()
#define XLOG_FATAL_ERRNO_VALUE get_errno_string()

// Fatal exceptions

#define XLOG_FATAL(msg) throw xlog::fatal_exception(XLOG_LOGGER_VAR_NAME, msg);
#define XLOG_FATAL_G(msg) throw xlog::fatal_exception(xlog::GetNamedLogger("Global"), msg)

#define XLOG_FATAL_F(fmat, ...) throw xlog::fatal_exception(XLOG_LOGGER_VAR_NAME, XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__))
#define XLOG_FATAL_FG(fmat, ...) throw xlog::fatal_exception(XLOG_LOGGER_VAR_NAME, XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__))

/*
 * This should work with std::error_code & boost::system::error_code
 */
#define XLOG_FATAL_C(errc) XLOG_FATAL(XLOG_FATAL_ERRC_VALUE(errc))
#define XLOG_FATAL_FC(errc, fmat, ...) throw xlog::fatal_exception(XLOG_LOGGER_VAR_NAME, XLOG_FORMATTER_SELECT1(fmat, XLOG_FATAL_ERRC_VALUE(errc), __VA_ARGS__))
#define XLOG_FATAL_FGC(errc, fmat, ...) throw xlog::fatal_exception(xlog::GetNamedLogger("Global"), XLOG_FORMATTER_SELECT1(fmat, XLOG_FATAL_ERRC_VALUE(errc), __VA_ARGS__))

/*
 * Fatal ERRNO messages
 */
#define XLOG_FATAL_E() XLOG_FATAL(XLOG_FATAL_ERRNO_VALUE)
#define XLOG_FATAL_FE(fmat, ...) throw xlog::fatal_exception(XLOG_LOGGER_VAR_NAME, XLOG_FORMATTER_SELECT1(fmat, XLOG_FATAL_ERRNO_VALUE, __VA_ARGS__))
#define XLOG_FATAL_FGE(fmat, ...) throw xlog::fatal_exception(xlog::GetNamedLogger("Global"), XLOG_FORMATTER_SELECT1(fmat, XLOG_FATAL_ERRNO_VALUE, __VA_ARGS__))

// Named Fatal Exceptions

#define XLOG_FATAL_I(name, msg) throw xlog::fatal_exception(XLOG_LOGGER_VAR_NAME, msg);
#define XLOG_FATAL_IF(name, fmat, ...) throw xlog::fatal_exception(xlog::GetNamedLogger(name), XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__))

#define XLOG_FATAL_IC(name, errc) XLOG_FATAL_I(XLOG_FATAL_ERRC_VALUE(errc))
#define XLOG_FATAL_IFC(name, errc, fmat, ...) throw xlog::fatal_exception(xlog::GetNamedLogger(name), XLOG_FORMATTER_SELECT1(fmat, XLOG_FATAL_ERRC_VALUE(errc), __VA_ARGS__))

#define XLOG_FATAL_IE(namme) XLOG_FATAL_I(XLOG_FATAL_ERRNO_VALUE)
#define XLOG_FATAL_IFE(name, fmat, ...) throw xlog::fatal_exception(xlog::GetNamedLogger(name), XLOG_FORMATTER_SELECT1(fmat, XLOG_FATAL_ERRNO_VALUE, __VA_ARGS__))

/*
 * Default log formatter
 */
namespace xlog::formatters
{
    void default_formatter(const boost::log::record_view& rec, boost::log::formatting_ostream& stream);
}

/*
 * String hashes to allow different "strings" to 
 * be used as keys for the logger map (see source file)
 */
namespace xlog
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

#endif // XLOG_H
