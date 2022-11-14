#define XLOG_TEST_FORMATTED_NOARGS(level) TEST(Formatted, NoArgsFormat_ ## level) \
{                                                                                \
    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR)\
    {                                                                            \
        auto random_string = MAKE_RANDOM_STRING();                               \
        XLOG_ ## level ## _F(random_string);                                     \
        XLOG_TEST_BOILERPLATE_A(record)                                          \
        XLOG_TEST_BOILERPLATE_B0(record, XLOG_LOGGER_VAR_NAME.channel(), XLOG_LEVEL2SEV(level), random_string, 500) \
    }                                                                            \
}                                                                                \
TEST(Formatted, ErrorCode_NoArgsFormat_ ## level)                                  \
{                                                                                \
    auto fmt_string = "Got an error during operations: '{0}'";                   \
                                                                                 \
    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR)\
    {                                                                            \
        auto code = MAKE_RANDOM_ERRC();                                          \
        auto expected_output = fmt::format(fmt_string, XLOG_ERRC_VALUE(code));   \
        XLOG_ ## level ## _FC(code, fmt_string);                                 \
        XLOG_TEST_BOILERPLATE_A(record)                                          \
        XLOG_TEST_BOILERPLATE_B0(record, XLOG_LOGGER_VAR_NAME.channel(), XLOG_LEVEL2SEV(level), expected_output, 500) \
    }                                                                            \
} \
TEST(Formatted, Errno_NoArgsFormat_ ## level) \
{ \
    auto fmt_string = "Got an error during operations: '{0}'"; \
 \
    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR) \
    { \
        auto rstrErrno = MAKE_RANDOM_STRING(); \
        xlog_set_errno_test_string(rstrErrno); \
        auto expected_output = fmt::format(fmt_string, rstrErrno); \
 \
        XLOG_ ## level ## _FE(fmt_string); \
        XLOG_TEST_BOILERPLATE_A(record) \
        XLOG_TEST_BOILERPLATE_B0(record, XLOG_LOGGER_VAR_NAME.channel(), XLOG_LEVEL2SEV(level), expected_output, 500) \
    } \
}

#define XLOG_TEST_FORMATTED_FMTARGS(level) TEST(Formatted, ArgsFormat_ ## level)  \
{                                                                                \
    auto fmt_string = "I {1} am {0} formatted {2}!";                             \
    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR)\
    {                                                                            \
        auto rstrA = MAKE_RANDOM_STRING();                                       \
        auto rstrB = MAKE_RANDOM_STRING();                                       \
        auto rstrC = MAKE_RANDOM_STRING();                                       \
                                                                                 \
        auto expected_output = fmt::format(fmt_string, rstrA, rstrB, rstrC);     \
                                                                                 \
        XLOG_ ## level ## _F(fmt_string, rstrA, rstrB, rstrC);                   \
        XLOG_TEST_BOILERPLATE_A(record)                                          \
        XLOG_TEST_BOILERPLATE_B0(record, XLOG_LOGGER_VAR_NAME.channel(), XLOG_LEVEL2SEV(level), expected_output, 500) \
    }                                                                            \
}                                                                                \
TEST(Formatted, ErrorCode_ArgsFormat_ ## level)                                    \
{                                                                                \
    auto fmt_string = "{0}; Got an error during operations: '{1}'";              \
                                                                                 \
    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR)\
    {                                                                            \
        auto code = MAKE_RANDOM_ERRC();                                          \
        auto rstrA = MAKE_RANDOM_STRING();                                       \
        auto expected_output = fmt::format(fmt_string, XLOG_ERRC_VALUE(code), rstrA);                                \
                                                                                 \
        XLOG_ ## level ## _FC(code, fmt_string, rstrA);                                   \
        XLOG_TEST_BOILERPLATE_A(record)                                          \
        XLOG_TEST_BOILERPLATE_B0(record, XLOG_LOGGER_VAR_NAME.channel(), XLOG_LEVEL2SEV(level), expected_output, 500)  \
    }                                                                            \
} \
TEST(Formatted, Errno_ArgsFormat_ ## level) \
{ \
    auto fmt_string = "{0}; Got an error during operations: '{1}'"; \
 \
    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR) \
    { \
        auto rstrErrno = MAKE_RANDOM_STRING(); \
        xlog_set_errno_test_string(rstrErrno); \
        auto rstrA = MAKE_RANDOM_STRING(); \
        auto expected_output = fmt::format(fmt_string, rstrErrno, rstrA); \
 \
        XLOG_ ## level ## _FE(fmt_string, rstrA); \
        XLOG_TEST_BOILERPLATE_A(record) \
        XLOG_TEST_BOILERPLATE_B0(record, XLOG_LOGGER_VAR_NAME.channel(), XLOG_LEVEL2SEV(level), expected_output, 500) \
    } \
}

XLOG_TEST_FORMATTED_NOARGS(INFO)
XLOG_TEST_FORMATTED_NOARGS(DEBUG)
XLOG_TEST_FORMATTED_NOARGS(DEBUG2)
XLOG_TEST_FORMATTED_NOARGS(WARN)
XLOG_TEST_FORMATTED_NOARGS(WARN2)
XLOG_TEST_FORMATTED_NOARGS(ERROR)
XLOG_TEST_FORMATTED_NOARGS(ERROR2)

XLOG_TEST_FORMATTED_FMTARGS(INFO)
XLOG_TEST_FORMATTED_FMTARGS(DEBUG)
XLOG_TEST_FORMATTED_FMTARGS(DEBUG2)
XLOG_TEST_FORMATTED_FMTARGS(WARN)
XLOG_TEST_FORMATTED_FMTARGS(WARN2)
XLOG_TEST_FORMATTED_FMTARGS(ERROR)
XLOG_TEST_FORMATTED_FMTARGS(ERROR2)
