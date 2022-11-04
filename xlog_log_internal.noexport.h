#include "xlog.h"

// This shouldn't be publicly visible
static const std::string INTERNAL_LOGGER_NAME("xlog_internal");
static xlog::LoggerType INTERNAL_LOGGER{boost::log::keywords::channel = INTERNAL_LOGGER_NAME};

#define XLOG_INTERNAL CUSTOM_LOG_SEV(INTERNAL_LOGGER, xlog::Severity::INTERNAL)
#define XLOG_INTERNAL_F(fmat, ...) XLOG_INTERNAL << XLOG_FORMATTER_SELECT(fmat, __VA_ARGS__)

#define XLOG_INTERNAL_C(errc) XLOG_INTERNAL << XLOG_ERRC_VALUE(errc)
#define XLOG_INTERNAL_FC(errc, fmat, ...) XLOG_INTERNAL << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRC_VALUE(errc), __VA_ARGS__)

#define XLOG_INTERNAL_E XLOG_INTERNAL << XLOG_ERRNO_VALUE
#define XLOG_INTERNAL_FE(fmat, ...) XLOG_INTERNAL << XLOG_FORMATTER_SELECT1(fmat, XLOG_ERRNO_VALUE, __VA_ARGS__)
