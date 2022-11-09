#!/bin/bash

LEVELS=(INFO DEBUG DEBUG2 WARN WARN2 ERROR ERROR2)
SEVERITIES=(INFO DEBUG DEBUG2 WARNING WARNING2 ERROR ERROR2)
LLEN=$((${#LEVELS[@]} - 1))

QUALIFIERS=(N I F IF C IC FC IFC E IE FE IFE)

WRITE_OUT()
{
    echo "$@"
}

WRITE_PATTERN_S()
{
    local ARGS="$1"
    local LOGGER_NAME="$2"
    local QF="$3"

    if [ -n "$ARGS" ]; then
        ARGS="(${ARGS})"
    fi

    if [ -n "$QF" ]; then
        QF="_${QF}"
    fi

    for i in $(seq 0 ${LLEN}); do
        WRITE_OUT "#define XLOG_${LEVELS[i]}${QF}${ARGS} CUSTOM_LOG_SEV(${LOGGER_NAME}, xlog::Severity::${SEVERITIES[i]})"
    done
}

WRITE_PATTERN_S2()
{
    local ARGS="$1"
    local LOGGER_NAME="$2"
    local QF="$3"
    local STREAM_ARG="$4"

    if [ -n "$ARGS" ]; then
        ARGS="(${ARGS})"
    fi

    if [ -n "$QF" ]; then
        QF="_${QF}"
    fi

    local STREAM_APPEND=''
    if [[ "$QF" =~ I ]]; then
        STREAM_APPEND='_I(name)'
    fi

    for i in $(seq 0 ${LLEN}); do
        WRITE_OUT "#define XLOG_${LEVELS[i]}${QF}${ARGS} XLOG_${LEVELS[i]}${STREAM_APPEND} << ${STREAM_ARG}"
    done
}

# Precedence -> Inplace 'I' > Formatted 'F' > ERRNO 'E' & Error Code 'C'
# XLOG_FORMATTER_SELECT(fmat, ...)
# XLOG_FORMATTER_SELECT1(fmat, arg1, ...)
# XLOG_FORMATTER_NOARG(fmat)
# XLOG_FORMATTER_DEFAULT(fmat, ...)

GENERATE_QFF()
{
    local OTHER_QFS="$1"

    if [ -z "$OTHER_QFS" ]; then
        echo 'F'
        return
    fi

    local BUILD_STR=''

    if [[ "${OTHER_QFS}" =~ I ]]; then
          BUILD_STR="${BUILD_STR}I"
    fi

    BUILD_STR="${BUILD_STR}F"

    if [[ "${OTHER_QFS}" =~ E ]]; then
        BUILD_STR="${BUILD_STR}E"
    elif [[ "${OTHER_QFS}" =~ C ]]; then
        BUILD_STR="${BUILD_STR}C"
    fi

    echo "${BUILD_STR}"
}

GENERATE_QFM()
{
    local OTHER_QFS="$1"

    if [ -z "$OTHER_QFS" ]; then
        echo 'M'
        return
    fi

    local BUILD_STR=''

    if [[ "${OTHER_QFS}" =~ I ]]; then
        BUILD_STR="${BUILD_STR}I"
    fi

    BUILD_STR="${BUILD_STR}M"

    if [[ "${OTHER_QFS}" =~ E ]]; then
        BUILD_STR="${BUILD_STR}E"
    elif [[ "${OTHER_QFS}" =~ C ]]; then
        BUILD_STR="${BUILD_STR}C"
    fi

    echo "${BUILD_STR}"
}

SEPERATED_APPEND()
{
    local INC_STRING="$1"
    local VALUE="$2"
    local SEPERATOR="$3"

    if [ -z "$SEPERATOR" ]; then
        SEPERATOR=','
    fi

    if [ ${#INC_STRING} -eq 0 ]; then
        echo "${VALUE}"
    else
        echo "${INC_STRING}${SEPERATOR} ${VALUE}"
    fi
}

SETUP_ARGS()
{
    local EXTRA_ARGS="$1"

    # The extra space is deliberate
    local WORDS=($(echo "$EXTRA_ARGS " | sed -rn 's/[, ]+/\n/gp'))
    local BUILD_STR=''

    if printf '%s\0' "${WORDS[@]}" | grep -Fxqz -- 'name'; then
        BUILD_STR="$(SEPERATED_APPEND "${BUILD_STR}" 'name')"
    fi

    if printf '%s\0' "${WORDS[@]}" | grep -Fxqz -- 'errc'; then
        BUILD_STR="$(SEPERATED_APPEND "${BUILD_STR}" 'errc')"
    fi

    BUILD_STR="$(SEPERATED_APPEND "${BUILD_STR}" 'fmat')"

    echo "${BUILD_STR}"
}

SETUP_FMAT_ARGS()
{
    local ALL_ARGS="$1"

    # The extra space is deliberate
    local WORDS=($(echo "$ALL_ARGS " | sed -rn 's/[, ]+/\n/gp'))
    local BUILD_STR=''

    if printf '%s\0' "${WORDS[@]}" | grep -Fxqz -- 'fmat'; then
        BUILD_STR="$(SEPERATED_APPEND "${BUILD_STR}" 'fmat')"
    fi

    for w in "${WORDS[@]}"; do
        if [ "$w" != 'name' ] && [ "$w" != 'fmat' ]; then
            BUILD_STR="$(SEPERATED_APPEND "${BUILD_STR}" "$w")"
        fi
    done

    echo "${BUILD_STR}"
}

GENERATE_VARIADIC_FORMATTER()
{
    local ALL_ARGS="$1"

    # The extra space is deliberate
    local WORDS=($(echo "$ALL_ARGS " | sed -rn 's/[, ]+/\n/gp'))

    if [ ${#WORDS[@]} -eq 0 ]; then
        echo 'XLOG_FORMATTER_SELECT'
    else
        echo 'XLOG_FORMATTER_SELECT1'
    fi
}

GENERATE_FORMATTER_NV_V()
{
    local ALL_ARGS="$1"

    echo 'XLOG_FORMATTER_DEFAULT'
}

GENERATE_FORMATTER_NV_NV()
{
    local ALL_ARGS="$1"

    # The extra space is deliberate
    local WORDS=($(echo "$ALL_ARGS " | sed -rn 's/[, ]+/\n/gp'))

    if [ ${#WORDS[@]} -eq 0 ]; then
        echo 'XLOG_FORMATTER_NOARG'
    else
        echo 'XLOG_FORMATTER_DEFAULT'
    fi
}

WRITE_PATTERN_F()
{
    local EXTRA_ARGS="$1"
    local LOGGER_NAME="$2"
    local EXTRA_QF="$3"

    local ORDERED_QFF="$(GENERATE_QFF "${EXTRA_QF}")"
    local ORDERED_QFM="$(GENERATE_QFM "${EXTRA_QF}")"
    local ALL_ARGS="$(SETUP_ARGS "${EXTRA_ARGS}")"
    local FORMATTER_ARGS="$(SETUP_FMAT_ARGS "${ALL_ARGS}")"

    local FORMATTER_VARIADIC="$(GENERATE_VARIADIC_FORMATTER ${EXTRA_ARGS})"
    local FORMATTER_NV_V="$(GENERATE_FORMATTER_NV_V ${EXTRA_ARGS})"
    local FORMATTER_NV_NV="$(GENERATE_FORMATTER_NV_NV ${EXTRA_ARGS})"

    local STREAM_APPEND=''
    if [[ "$EXTRA_QF" =~ I ]]; then
        STREAM_APPEND='_I(name)'
    fi

    WRITE_OUT '#ifdef XLOG_USE_BOOST_PP_VARIADIC'
    for i in $(seq 0 ${LLEN}); do
        WRITE_OUT "#define XLOG_${LEVELS[i]}_${ORDERED_QFF}(${ALL_ARGS}, ...) XLOG_${LEVELS[i]}${STREAM_APPEND} << ${FORMATTER_VARIADIC}(${FORMATTER_ARGS}, __VA_ARGS__)"
    done
    WRITE_OUT
    for i in $(seq 0 ${LLEN}); do
        WRITE_OUT "#define XLOG_${LEVELS[i]}_${ORDERED_QFM}(${ALL_ARGS}, ...) XLOG_${LEVELS[i]}_${ORDERED_QFF}(${FORMATTER_ARGS}, __VA_ARGS__)"
    done
    WRITE_OUT '#else'
    for i in $(seq 0 ${LLEN}); do
        WRITE_OUT "#define XLOG_${LEVELS[i]}_${ORDERED_QFF}(${ALL_ARGS}, ...) XLOG_${LEVELS[i]}${STREAM_APPEND} << ${FORMATTER_NV_V}(${FORMATTER_ARGS}, __VA_ARGS__)"
    done
    WRITE_OUT
    for i in $(seq 0 ${LLEN}); do
        WRITE_OUT "#define XLOG_${LEVELS[i]}_${ORDERED_QFM}(${ALL_ARGS}, ...) XLOG_${LEVELS[i]}${STREAM_APPEND} << ${FORMATTER_NV_NV}(${FORMATTER_ARGS})"
    done
    WRITE_OUT '#endif // XLOG_USE_BOOST_PP_VARIADIC'
}

WRITE_HEAD()
{
    local NAME="$1"

    WRITE_OUT
    WRITE_OUT
    WRITE_OUT "// ${NAME}"
    WRITE_OUT
}

WRITE_COMMENT_START()
{
    WRITE_OUT '/*'
}

WRITE_COMMENT_END()
{
    WRITE_OUT '*/'
}

WRITE_TODO()
{
    WRITE_OUT '// TODO'
}

WRITE_PATTERN_F 'name, errc' 'XLOG_LOGGER_VAR_NAME' 'IC'

exit 0

WRITE_HEAD 'Normal Stream Macros'
WRITE_PATTERN_S '' 'XLOG_LOGGER_VAR_NAME' ''
WRITE_HEAD 'Inplace Stream Macros'
WRITE_COMMENT_START
WRITE_PATTERN_S 'name' 'xlog::GetNamedLogger(name)' 'I'
WRITE_COMMENT_END
WRITE_HEAD 'Normal Format Macros'
WRITE_PATTERN_F '' 'XLOG_LOGGER_VAR_NAME' ''
WRITE_HEAD 'Inplace Format Macros'
WRITE_TODO
WRITE_HEAD 'Error Code Stream Macros'
WRITE_PATTERN_S2 'errc' 'XLOG_LOGGER_VAR_NAME' 'C' 'XLOG_ERRC_VALUE(errc)'
WRITE_HEAD 'Inplace Error Code Stream Macros'
WRITE_COMMENT_START
WRITE_PATTERN_S2 'name, errc' 'xlog::GetNamedLogger(name)' 'IC' 'XLOG_ERRC_VALUE(errc)'
WRITE_COMMENT_END
WRITE_HEAD 'Error Code Format Macros'
WRITE_TODO
WRITE_HEAD 'Inplace Error Code Format Macros'
WRITE_TODO
WRITE_HEAD 'Errno Stream Macros'
WRITE_PATTERN_S2 '' 'XLOG_LOGGER_VAR_NAME' 'E' 'XLOG_ERRNO_VALUE'
WRITE_HEAD 'Inplace Errno Stream Macros'
WRITE_COMMENT_START
WRITE_PATTERN_S2 'name' 'xlog::GetNamedLogger(name)' 'IE' 'XLOG_ERRNO_VALUE'
WRITE_COMMENT_END

#if [[ "$q" =~ F ]]; then
#    m=${q//F/M}
#fi

#for q in "${QUALIFIERS[@]}"; do
#    for i in $(seq 0 ${LLEN}); do
#        if [ "$q" == 'N' ]; then
#            : #WRITE_OUT "XLOG_${LEVELS[i]} CUSTOM_LOG_SEV(XLOG_LOGGER_VAR_NAME, xlog::Severity::${SEVERITIES[i]})"
#        else
#            :
#        fi
#    done
#done
