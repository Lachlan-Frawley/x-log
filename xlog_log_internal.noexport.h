#include "xlog.h"

// This shouldn't be publicly visible
static const std::string INTERNAL_LOGGER_NAME("xlog_internal");
static XLog::LoggerType INTERNAL_LOGGER{boost::log::keywords::channel = INTERNAL_LOGGER_NAME};
#define INTERNAL() CUSTOM_LOG_SEV(INTERNAL_LOGGER, XLog::Severity::INTERNAL)
#define INTERNAL_CODE(errc) INTERNAL() << ERRC_STREAM(errc)
#define INTERNAL_ERRNO() INTERNAL() << ERRNO_STREAM
