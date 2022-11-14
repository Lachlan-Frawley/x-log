TEST(Exceptions, FatalNoQualifiers)
{
    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR)
    {
        auto message = MAKE_RANDOM_STRING_SAFE();
        try
        {
            XLOG_FATAL(message);
            FAIL() << "Expected xlog::fatal_exception to be thrown (no exception thrown)";
        }
        catch(const xlog::fatal_exception& e)
        {
            EXPECT_EQ(e.what(), message);
            XLOG_TEST_BOILERPLATE_A(record)
            XLOG_TEST_BOILERPLAT_B0(record, XLOG_LOGGER_VAR_NAME.channel(), xlog::Severity::FATAL, message, 500)
        }
        catch(...)
        {
            FAIL() << "Expected xlog::fatal_exception to be thrown (wrong exception thrown)";
        }
    }
}

TEST(Exceptions, FatalGlobal)
{
    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR)
    {
        auto message = MAKE_RANDOM_STRING_SAFE();
        try
        {
            XLOG_FATAL_G(message);
            FAIL() << "Expected xlog::fatal_exception to be thrown (no exception thrown)";
        }
        catch(const xlog::fatal_exception& e)
        {
            EXPECT_EQ(e.what(), message);
            XLOG_TEST_BOILERPLATE_A(record)
            XLOG_TEST_BOILERPLAT_B0(record, XLOG_GLOBAL_NAME, xlog::Severity::FATAL, message, 500)
        }
        catch(...)
        {
            FAIL() << "Expected xlog::fatal_exception to be thrown (wrong exception thrown)";
        }
    }
}

TEST(Exceptions, FatalErrorCodeNoArgsFormat)
{
    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR)
    {
        auto errc = MAKE_RANDOM_ERRC();
        auto message = errc.message();

        try
        {
            XLOG_FATAL_C(errc);
            FAIL() << "Expected xlog::fatal_exception to be thrown (no exception thrown)";
        }
        catch(const xlog::fatal_exception& e)
        {
            EXPECT_EQ(e.what(), message);
            XLOG_TEST_BOILERPLATE_A(record)
            XLOG_TEST_BOILERPLAT_B0(record, XLOG_LOGGER_VAR_NAME.channel(), xlog::Severity::FATAL, message, 500)
        }
        catch(...)
        {
            FAIL() << "Expected xlog::fatal_exception to be thrown (wrong exception thrown)";
        }
    }
}

TEST(Exceptions, FatalErrnoNoArgsFormat)
{
    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR)
    {
        auto rstrErrno = MAKE_RANDOM_STRING_SAFE();
        xlog_set_errno_test_string(rstrErrno);
        auto message = rstrErrno;

        try
        {
            XLOG_FATAL_E();
            FAIL() << "Expected xlog::fatal_exception to be thrown (no exception thrown)";
        }
        catch(const xlog::fatal_exception& e)
        {
            EXPECT_EQ(e.what(), message);
            XLOG_TEST_BOILERPLATE_A(record)
            XLOG_TEST_BOILERPLAT_B0(record, XLOG_LOGGER_VAR_NAME.channel(), xlog::Severity::FATAL, message, 500)
        }
        catch(...)
        {
            FAIL() << "Expected xlog::fatal_exception to be thrown (wrong exception thrown)";
        }
    }
}

TEST(Exceptions, FatalFormat)
{
    auto fmt_string = "I {1} am {0} formatted {2}!";

    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR)
    {
        auto rstrA = MAKE_RANDOM_STRING_SAFE();
        auto rstrB = MAKE_RANDOM_STRING_SAFE();
        auto rstrC = MAKE_RANDOM_STRING_SAFE();

        auto expected_output = fmt::format(fmt_string, rstrA, rstrB, rstrC);

        try
        {
            XLOG_FATAL_F(fmt_string, rstrA, rstrB, rstrC);
            FAIL() << "Expected xlog::fatal_exception to be thrown (no exception thrown)";
        }
        catch(const xlog::fatal_exception& e)
        {
            EXPECT_EQ(e.what(), expected_output);
            XLOG_TEST_BOILERPLATE_A(record)
            XLOG_TEST_BOILERPLAT_B0(record, XLOG_LOGGER_VAR_NAME.channel(), xlog::Severity::FATAL, expected_output, 500)
        }
        catch(...)
        {
            FAIL() << "Expected xlog::fatal_exception to be thrown (wrong exception thrown)";
        }
    }
}

TEST(Exceptions, FatalFormatGlobal)
{
    auto fmt_string = "I {1} am {0} formatted {2}!";

    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR)
    {
        auto rstrA = MAKE_RANDOM_STRING_SAFE();
        auto rstrB = MAKE_RANDOM_STRING_SAFE();
        auto rstrC = MAKE_RANDOM_STRING_SAFE();

        auto expected_output = fmt::format(fmt_string, rstrA, rstrB, rstrC);

        try
        {
            XLOG_FATAL_FG(fmt_string, rstrA, rstrB, rstrC); \
            FAIL() << "Expected xlog::fatal_exception to be thrown (no exception thrown)";
        }
        catch(const xlog::fatal_exception& e)
        {
            EXPECT_EQ(e.what(), expected_output);
            XLOG_TEST_BOILERPLATE_A(record)
            XLOG_TEST_BOILERPLAT_B0(record, XLOG_GLOBAL_NAME, xlog::Severity::FATAL, expected_output, 500)
        }
        catch(...)
        {
            FAIL() << "Expected xlog::fatal_exception to be thrown (wrong exception thrown)";
        }
    }
}

TEST(Exceptions, FatalErrorCodeArgsFormat)
{
    auto fmt_string = "I {1} am {0} formatted {2}!";

    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR)
    {
        auto errc = MAKE_RANDOM_ERRC();

        auto rstrB = MAKE_RANDOM_STRING_SAFE();
        auto rstrC = MAKE_RANDOM_STRING_SAFE();

        auto expected_output = fmt::format(fmt_string, XLOG_FATAL_ERRC_VALUE(errc), rstrB, rstrC);

        try
        {
            XLOG_FATAL_FC(errc, fmt_string, rstrB, rstrC);
            FAIL() << "Expected xlog::fatal_exception to be thrown (no exception thrown)";
        }
        catch(const xlog::fatal_exception& e)
        {
            EXPECT_EQ(e.what(), expected_output);
            XLOG_TEST_BOILERPLATE_A(record)
            XLOG_TEST_BOILERPLAT_B0(record, XLOG_LOGGER_VAR_NAME.channel(), xlog::Severity::FATAL, expected_output, 500)
        }
        catch(...)
        {
            FAIL() << "Expected xlog::fatal_exception to be thrown (wrong exception thrown)";
        }
    }
}

TEST(Exceptions, FatalErrnoArgsFormat)
{
    auto fmt_string = "I {1} am {0} formatted {2}!";

    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR)
    {
        auto rstrErrno = MAKE_RANDOM_STRING_SAFE();
        xlog_set_errno_test_string(rstrErrno);

        auto rstrB = MAKE_RANDOM_STRING_SAFE();
        auto rstrC = MAKE_RANDOM_STRING_SAFE();

        auto expected_output = fmt::format(fmt_string, rstrErrno, rstrB, rstrC);

        try
        {
            XLOG_FATAL_FE(fmt_string, rstrB, rstrC);
            FAIL() << "Expected xlog::fatal_exception to be thrown (no exception thrown)";
        }
        catch(const xlog::fatal_exception& e)
        {
            EXPECT_EQ(e.what(), expected_output);
            XLOG_TEST_BOILERPLATE_A(record)
            XLOG_TEST_BOILERPLAT_B0(record, XLOG_LOGGER_VAR_NAME.channel(), xlog::Severity::FATAL, expected_output, 500)
        }
        catch(...)
        {
            FAIL() << "Expected xlog::fatal_exception to be thrown (wrong exception thrown)";
        }
    }
}

TEST(Exceptions, FatalErrorCodeArgsFormatGlobal)
{
    auto fmt_string = "I {1} am {0} formatted {2}!";

    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR)
    {
        auto errc = MAKE_RANDOM_ERRC();

        auto rstrB = MAKE_RANDOM_STRING_SAFE();
        auto rstrC = MAKE_RANDOM_STRING_SAFE();

        auto expected_output = fmt::format(fmt_string, XLOG_FATAL_ERRC_VALUE(errc), rstrB, rstrC);

        try
        {
            XLOG_FATAL_FGC(errc, fmt_string, rstrB, rstrC);
            FAIL() << "Expected xlog::fatal_exception to be thrown (no exception thrown)";
        }
        catch(const xlog::fatal_exception& e)
        {
            EXPECT_EQ(e.what(), expected_output);
            XLOG_TEST_BOILERPLATE_A(record)
            XLOG_TEST_BOILERPLAT_B0(record, XLOG_GLOBAL_NAME, xlog::Severity::FATAL, expected_output, 500)
        }
        catch(...)
        {
            FAIL() << "Expected xlog::fatal_exception to be thrown (wrong exception thrown)";
        }
    }
}

TEST(Exceptions, FatalErrnoArgsFormatGlobal)
{
    auto fmt_string = "I {1} am {0} formatted {2}!";

    for(auto XLOG_TEST_ITR = 0; XLOG_TEST_ITR < TEST_ITERATIONS; ++XLOG_TEST_ITR)
    {
        auto rstrErrno = MAKE_RANDOM_STRING_SAFE();
        xlog_set_errno_test_string(rstrErrno);

        auto rstrB = MAKE_RANDOM_STRING_SAFE();
        auto rstrC = MAKE_RANDOM_STRING_SAFE();

        auto expected_output = fmt::format(fmt_string, rstrErrno, rstrB, rstrC);

        try
        {
            XLOG_FATAL_FGE(fmt_string, rstrB, rstrC);
            FAIL() << "Expected xlog::fatal_exception to be thrown (no exception thrown)";
        }
        catch(const xlog::fatal_exception& e)
        {
            EXPECT_EQ(e.what(), expected_output);
            XLOG_TEST_BOILERPLATE_A(record)
            XLOG_TEST_BOILERPLAT_B0(record, XLOG_GLOBAL_NAME, xlog::Severity::FATAL, expected_output, 500)
        }
        catch(...)
        {
            FAIL() << "Expected xlog::fatal_exception to be thrown (wrong exception thrown)";
        }
    }
}
