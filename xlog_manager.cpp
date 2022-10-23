#include <iostream>
#include <cctype>

#include "xlog_grpc_util.noexport.h"

#include <grpcpp/create_channel.h>

#include <cli/cli.h>
#include <cli/loopscheduler.h>
#include <cli/clilocalsession.h>

#include <CLI/CLI.hpp>

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
            << "Source Location: " << bool_to_enabled(message.use_source_location()) << std::endl;
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
            << "Source Location: " << bool_to_enabled(message.use_source_location()) << std::endl;
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
                << ", SLOC = " << bool_to_enabled(sev.use_source_location())
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
    std::string command = "shell";

    auto name_opt = app.add_option("NAME", app_name, "Name of the application to manage");
    name_opt->required(true);
    app.add_option("-p,--pid", app_pid, "PID of the application to manage")
        ->needs(name_opt)
        ->required(false);

    // TODO - Add commands as fixed arguments

    CLI11_PARSE(app, argc, argv);

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

    to_lower(command);
    if(command.compare("shell") == 0)
    {
        auto root_menu = std::make_unique<cli::Menu>("xlog");

        root_menu->Insert(
            "getgloballevel",
            [&stub](std::ostream& out) { GetGlobalLevel(stub, out); },
            "Get the global/default logging level");

        root_menu->Insert(
            "getchannellevel",
            [&stub](std::ostream& out, const std::string& channel) { GetChannelLevel(stub, out, channel); },
            "Get logging level for the given channel");

        root_menu->Insert(
            "getalllevels",
            [&stub](std::ostream& out) { GetAllLogLevels(stub, out); },
            "Get all log handles & their severities");

        root_menu->Insert(
            "getallhandles",
            [&stub](std::ostream& out) { GetAllHandles(stub, out); },
            "Get all log handles");

        root_menu->Insert(
            "setgloballevel",
            [&stub](std::ostream& out, const std::string& level) { SetGlobalLevel(stub, out, level, false); },
            "Set the global/default log level");

        root_menu->Insert(
            "setgloballevel",
            [&stub](std::ostream& out, const std::string& level, bool with_sloc) { SetGlobalLevel(stub, out, level, with_sloc); },
            "Set the global/default log level");

        root_menu->Insert(
            "setchannellevel",
            [&stub](std::ostream& out, const std::string& channel, const std::string& level) { SetChannelLevel(stub, out, channel, level, false); },
            "Set logging level for the given channel");

        root_menu->Insert(
            "setchannellevel",
            [&stub](std::ostream& out, const std::string& channel, const std::string& level, bool with_sloc) { SetChannelLevel(stub, out, channel, level, with_sloc); },
            "Set logging level for the given channel");

        cli::Cli cli(std::move(root_menu));
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

    return 0;
}
