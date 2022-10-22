#include "xlog.h"
GET_LOGGER("Test Program")

#include <iostream>
#include <thread>

#include <csignal>
#include <unordered_map>
std::unordered_map<int, sighandler_t> base_signals;

void handle_signal(int signal)
{
    LOG_INFO() << "Stopping!" << std::endl;
    XLog::ShutownLogging(signal);

    std::signal(signal, base_signals[signal]);
    std::raise(signal);
}

void setup_signal(int signal)
{
    base_signals[signal] = std::signal(signal, handle_signal);
}

#define LOG_AT(sev) CUSTOM_LOG_SEV(__logger, sev) << "Logging @ " << XLog::GetSeverityString(sev)

XLog::Severity get_next(XLog::Severity given)
{
    switch(given)
    {
        case XLog::Severity::INFO:
            return XLog::Severity::DEBUG2;
        case XLog::Severity::DEBUG2:
            return XLog::Severity::DEBUG;
        case XLog::Severity::DEBUG:
            return XLog::Severity::WARNING2;
        case XLog::Severity::WARNING2:
            return XLog::Severity::WARNING;
        case XLog::Severity::WARNING:
            return XLog::Severity::ERROR2;
        case XLog::Severity::ERROR2:
            return XLog::Severity::ERROR;
    }

    return XLog::Severity::INFO;
}

int main(int argc, char** argv)
{
    XLog::InitializeLogging();

    setup_signal(SIGINT);
    setup_signal(SIGTERM);
    setup_signal(SIGABRT);

    auto current_sev = XLog::Severity::INFO;
    while(true)
    {
        LOG_AT(current_sev);
        current_sev = get_next(current_sev);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    LOG_AT(XLog::Severity::FATAL);

    return 0;
}
