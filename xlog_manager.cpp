#include <iostream>
#include <cctype>

#include "xlog_grpc_util.noexport.h"

#include <grpcpp/create_channel.h>

#include <cli/cli.h>
#include <cli/loopscheduler.h>
#include <cli/clilocalsession.h>
#include <cli/filehistorystorage.h>

#include <CLI/CLI.hpp>

#ifdef __cpp_lib_source_location
#define XLOG_LOGGING_USE_SOURCE_LOCATION
#endif

void to_lower(std::string& value)
{
    for(char& c : value)
    {
        c = std::tolower(c);
    }
}

std::string log_level_to_string(xlogProto::Severity sev)
{
    switch(sev)
    {
        case xlogProto::Severity::SEV_INFO:
            return "INFO";
        case xlogProto::Severity::SEV_DEBUG:
            return "DEBUG";
        case xlogProto::Severity::SEV_WARNING:
            return "WARNING";
        case xlogProto::Severity::SEV_ERROR:
            return "ERROR";
        case xlogProto::Severity::SEV_FATAL:
            return "FATAL";
    }

    return "UNKNOWN";
}

bool string_to_log_level(std::string val, xlogProto::Severity& sev_out)
{
    to_lower(val);

    if(val.compare("info") == 0)
    {
        sev_out = xlogProto::Severity::SEV_INFO;
        return true;
    }

    if(val.compare("debug") == 0)
    {
        sev_out = xlogProto::Severity::SEV_DEBUG;
        return true;
    }

    if(val.compare("warning") == 0 || val.compare("warn") == 0)
    {
        sev_out = xlogProto::Severity::SEV_WARNING;
        return true;
    }

    if(val.compare("error") == 0 || val.compare("err") == 0)
    {
        sev_out = xlogProto::Severity::SEV_ERROR;
        return true;
    }

    if(val.compare("fatal") == 0)
    {
        sev_out = xlogProto::Severity::SEV_FATAL;
        return true;
    }

    sev_out = xlogProto::Severity::SEV_UNKNOWN;
    return false;
}

std::string bool_to_enabled(bool val)
{
    if(val)
    {
        return "enabled";
    }
    else
    {
        return "disabled";
    }
}

typedef std::unique_ptr<xlogProto::RuntimeLogManagement::Stub>& StubRef;

void GetGlobalLevel(StubRef stub, std::ostream& out)
{
    grpc::ClientContext context;
    xlogProto::Void _vd;

    xlogProto::SeverityMessage message;
    auto status = stub->GetDefaultLogLevel(&context, _vd, &message);
    if(!status.ok())
    {
        out << "Failed to call stub 'GetDefaultLogLevel' -> " << status.error_message() << std::endl;
    }
    else
    {
        out
            << "Level: " << log_level_to_string(message.value()) << std::endl
#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
            << "Source Location: " << bool_to_enabled(message.use_source_location()) << std::endl
#endif
            ;
    }
}

void GetChannelLevel(StubRef stub, std::ostream& out, const std::string& channel)
{
    grpc::ClientContext context;
    xlogProto::LogChannel channelMessage;
    channelMessage.set_channel(channel);

    xlogProto::SeverityMessage message;
    auto status = stub->GetChannelLogLevel(&context, channelMessage, &message);
    if(!status.ok())
    {
        out << "Failed to call stub 'GetChannelLogLevel' -> " << status.error_message() << std::endl;
    }
    else
    {
        out
            << "Level: " << log_level_to_string(message.value()) << std::endl
#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
            << "Source Location: " << bool_to_enabled(message.use_source_location()) << std::endl
#endif
            ;
    }
}

void GetAllLogLevels(StubRef stub, std::ostream& out)
{
    grpc::ClientContext context;
    xlogProto::Void _vd;

    xlogProto::AllLogLevelsMessage message;
    auto status = stub->GetAllLogLevels(&context, _vd, &message);
    if(!status.ok())
    {
        out << "Failed to call stub 'GetAllLogLevels' -> " << status.error_message() << std::endl;
    }
    else
    {
        for(const auto& [handle, sev] : message.values())
        {
            if(handle.empty())
            {
                continue;
            }

            out
                << "Handle = " << handle
                << ", Log Level = " << log_level_to_string(sev.value())
#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
                << ", SLOC = " << bool_to_enabled(sev.use_source_location())
#endif
                << std::endl;
        }
    }
}

void GetAllHandles(StubRef stub, std::ostream& out)
{
    grpc::ClientContext context;
    xlogProto::Void _vd;

    xlogProto::AllLogHandlesMessage message;
    auto status = stub->GetAllLogHandles(&context, _vd, &message);
    if(!status.ok())
    {
        out << "Failed to call stub 'GetAllLogHandles' -> " << status.error_message() << std::endl;
    }
    else
    {
        for(const auto& handle : message.values())
        {
            if(handle.empty())
            {
                continue;
            }

            out
                << "Handle = " << handle
                << std::endl;
        }
    }
}

void SetGlobalLevel(StubRef stub, std::ostream& out, const std::string& level, bool with_sloc)
{
    grpc::ClientContext context;
    xlogProto::Void _vd;

    xlogProto::SeverityMessage severity;
    severity.set_use_source_location(with_sloc);

    xlogProto::Severity sev;
    if(!string_to_log_level(level, sev))
    {
        out << "Could not convert log level string to valid log level" << std::endl;
        return;
    }
    severity.set_value(sev);

    auto status = stub->SetDefaultLogLevel(&context, severity, &_vd);
    if(!status.ok())
    {
        out << "Failed to call stub 'SetDefaultLogLevel' -> " << status.error_message() << std::endl;
    }
}

void SetChannelLevel(StubRef stub, std::ostream& out, const std::string& channel, const std::string& level, bool with_sloc)
{
    grpc::ClientContext context;
    xlogProto::Void _vd;

    xlogProto::SeverityMessage severity;
    severity.set_use_source_location(with_sloc);

    xlogProto::Severity sev;
    if(!string_to_log_level(level, sev))
    {
        out << "Could not convert log level string to valid log level" << std::endl;
        return;
    }
    severity.set_value(sev);

    xlogProto::SetChannelSeverityMessage setMessage;
    setMessage.set_channel(channel);
    setMessage.mutable_severity()->CopyFrom(severity);

    auto status = stub->SetChannelSeverity(&context, setMessage, &_vd);
    if(!status.ok())
    {
        out << "Failed to call stub 'SetChannelSeverity' -> " << status.error_message() << std::endl;
    }
}

int main(int argc, char** argv)
{
    CLI::App app{"xlog External Management Tool"};

    std::string app_name;
    int app_pid = -1;

    // Command options
    bool use_shell = false;

    bool get_default_level = false;
    std::string get_channel_level;
    bool get_all_levels = false;
    bool get_all_handles = false;

    std::string set_default_level;
#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
    std::tuple<std::string, bool> set_default_level_sloc;
#endif
    std::tuple<std::string, std::string> set_channel_level;
#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
    std::tuple<std::string, std::string, bool> set_channel_level_sloc;
#endif

    auto name_opt = app.add_option("-n, --name", app_name, "Name of the application to manage")
        ->required(true);

    app.add_option("-p, --pid", app_pid, "PID of the application to manage")
        ->needs(name_opt)
        ->required(false);

    // Direct commands
    auto command_group = app.add_option_group("Actions", "Actions to execute on an xlog instance")
        ->require_option(0, 1)
        ->ignore_case();
    auto shell_opt = command_group->add_flag("--shell", use_shell, "Enter a telnet-style shell to execute commands");

    auto get_default_level_opt = command_group->add_flag("--get-default-level", get_default_level, "Get the default/global log level");
    auto get_channel_level_opt = command_group->add_option("--get-channel-level", get_channel_level, "Get the level of a specific log channel");
    auto get_all_levels_opt = command_group->add_flag("--get-all-levels", get_all_levels, "Get all log channels and their associated severity");
    auto get_all_handles_opt = command_group->add_flag("--get-all-channels", get_all_handles, "Get all log channels");

    auto set_default_level_opt = command_group->add_option("--set-default-level", set_default_level, "Set the default/global log level");
#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
    auto set_default_level_sloc_opt = command_group->add_option("--set-default-level", set_default_level_sloc, "Set the default/global log level");
#endif
    auto set_channel_level_opt = command_group->add_option("--set-channel-level", set_channel_level, "Set the level of a specific log channel");
#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
    auto set_channel_level_sloc_opt = command_group->add_option("--set-channel-level", set_channel_level_sloc, "Set the level of a specific log channel");
#endif

        app.footer(
R"""(
Valid Log Levels:
    INFO
    DEBUG
    WARNING  or  WARN
    ERROR    or  ERR
    FATAL

Log levels are case insensitive for ease of use
)"""
#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
R"""(
If source lines are enabled, extra commands for setting the log level to also log the source line will be present.

The following log levels support source lines:
    DEBUG
    WARNING
    ERROR
    FATAL (always has source lines)
)"""
#endif
);

    CLI11_PARSE(app, argc, argv);

    //std::cout << "Command Group Count = " << command_group->count_all() << std::endl;
    //return 0;

    auto candidates = TRY_GET_PROGRAM_LOG_SOCKET(app_name, app_pid);
    if(candidates.empty())
    {
        // No candidates
        std::cout << "No candidates to connect to" << std::endl;
        return 1;
    }
    else if(candidates.size() != 1)
    {
        // Can't work out which candidate
        std::cout << "Too many candidates for connection (" << candidates.size() << ')' << std::endl;
        return 1;
    }

    std::string socket = candidates.front();
    auto channel = grpc::CreateChannel(fmt::format("unix://{0}", socket), grpc::InsecureChannelCredentials());
    auto stub = xlogProto::RuntimeLogManagement::NewStub(channel);

    // Default to using shell
    if(command_group->count_all() == 0)
    {
        use_shell = true;
    }

    if(use_shell)
    {
        auto root_menu = std::make_unique<cli::Menu>("xlog");

        root_menu->Insert(
            "GetGlobalLevel",
            [&stub](std::ostream& out) { GetGlobalLevel(stub, out); },
            "Get the global/default logging level");

        root_menu->Insert(
            "GetChannelLevel",
            [&stub](std::ostream& out, const std::string& channel) { GetChannelLevel(stub, out, channel); },
            "Get logging level for the given channel");

        root_menu->Insert(
            "GetAllLevels",
            [&stub](std::ostream& out) { GetAllLogLevels(stub, out); },
            "Get all log handles & their severities");

        root_menu->Insert(
            "GetAllHandles",
            [&stub](std::ostream& out) { GetAllHandles(stub, out); },
            "Get all log handles");

        root_menu->Insert(
            "SetGlobalLevel",
            [&stub](std::ostream& out, const std::string& level) { SetGlobalLevel(stub, out, level, false); },
            "Set the global/default log level");

#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
        root_menu->Insert(
            "SetGlobalLevel",
            [&stub](std::ostream& out, const std::string& level, bool with_sloc) { SetGlobalLevel(stub, out, level, with_sloc); },
            "Set the global/default log level");
#endif

        root_menu->Insert(
            "SetChannelLevel",
            [&stub](std::ostream& out, const std::string& channel, const std::string& level) { SetChannelLevel(stub, out, channel, level, false); },
            "Set logging level for the given channel");

#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
        root_menu->Insert(
            "SetChannelLevel",
            [&stub](std::ostream& out, const std::string& channel, const std::string& level, bool with_sloc) { SetChannelLevel(stub, out, channel, level, with_sloc); },
            "Set logging level for the given channel");
#endif

        cli::Cli cli(std::move(root_menu), std::make_unique<cli::FileHistoryStorage>(".cli"));
        cli.StdExceptionHandler(
                [](std::ostream& out, const std::string& cmd, const std::exception& e)
                {
                    out << "Exception caught in cli handler: "
                        << e.what()
                        << " handling command: "
                        << cmd
                        << ".\n";
                }
            );

        cli::LoopScheduler loop;
        cli::CliLocalTerminalSession session(cli, loop, std::cout);
        session.ExitAction(
                [&loop](auto& out)
                {
                    loop.Stop();
                }
            );

        loop.Run();
    }
    else if(get_default_level)
    {
        GetGlobalLevel(stub, std::cout);
    }
    else if(*get_channel_level_opt)
    {
        GetChannelLevel(stub, std::cout, get_channel_level);
    }
    else if(get_all_levels)
    {
        GetAllLogLevels(stub, std::cout);
    }
    else if(get_all_handles)
    {
        GetAllHandles(stub, std::cout);
    }
    else if(*set_default_level_opt)
    {
        SetGlobalLevel(stub, std::cout, set_default_level, false);
    }
    else if(*set_channel_level_opt)
    {
        SetChannelLevel(stub, std::cout, std::get<0>(set_channel_level), std::get<1>(set_channel_level), false);
    }
#ifdef XLOG_LOGGING_USE_SOURCE_LOCATION
    else if(*set_default_level_sloc_opt)
    {
        SetGlobalLevel(stub, std::cout, std::get<0>(set_default_level), std::get<1>(set_default_level));
    }
    else if(*set_channel_level_sloc_opt)
    {
        SetChannelLevel(stub, std::cout, std::get<0>(set_channel_level), std::get<1>(set_channel_level), std::get<2>(set_channel_level));
    }
#endif
    else
    {
        std::cerr << "Given command is unknown or invalid" << std::endl;
        return 1;
    }

    return 0;
}
