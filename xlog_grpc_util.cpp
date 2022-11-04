#include "xlog_grpc_util.noexport.h"

#include <regex>
#include <iostream>
#include <filesystem>

#include <fcntl.h>
#include <unistd.h>
#include <fmt/core.h>

#include "xlog_log_internal.noexport.h"

extern const char* __progname;

static auto SOCKET_REGEX = std::regex(R"(^([0-9]+)-(.+)\.socket$)");

xlogProto::SeverityMessage make_severity_message(XLog::Severity severity)
{
    xlogProto::SeverityMessage msg;
    msg.set_use_source_location(false);

    switch(severity)
    {
        case XLog::Severity::INFO:
            msg.set_value(xlogProto::Severity::SEV_INFO);
            break;
        case XLog::Severity::FATAL:
            msg.set_value(xlogProto::Severity::SEV_FATAL);
            break;
        case XLog::Severity::DEBUG:
            msg.set_use_source_location(true);
        case XLog::Severity::DEBUG2:
            msg.set_value(xlogProto::Severity::SEV_DEBUG);
            break;
        case XLog::Severity::WARNING:
            msg.set_use_source_location(true);
        case XLog::Severity::WARNING2:
            msg.set_value(xlogProto::Severity::SEV_WARNING);
            break;
        case XLog::Severity::ERROR:
            msg.set_use_source_location(true);
        case XLog::Severity::ERROR2:
            msg.set_value(xlogProto::Severity::SEV_ERROR);
            break;
    }

    return msg;
}

XLog::Severity severity_from_message(const xlogProto::SeverityMessage& msg)
{
    if(msg.value() == xlogProto::Severity::SEV_INFO)
    {
        return XLog::Severity::INFO;
    }
    else if(msg.value() == xlogProto::Severity::SEV_FATAL)
    {
        return XLog::Severity::FATAL;
    }

    if(msg.use_source_location())
    {
        switch(msg.value())
        {
            case xlogProto::Severity::SEV_DEBUG:
                return XLog::Severity::DEBUG;
            case xlogProto::Severity::SEV_WARNING:
                return XLog::Severity::WARNING;
            case xlogProto::Severity::SEV_ERROR:
                return XLog::Severity::ERROR;
        }
    }
    else
    {
        switch(msg.value())
        {
            case xlogProto::Severity::SEV_DEBUG:
                return XLog::Severity::DEBUG2;
            case xlogProto::Severity::SEV_WARNING:
                return XLog::Severity::WARNING2;
            case xlogProto::Severity::SEV_ERROR:
                return XLog::Severity::ERROR2;
        }
    }

    return XLog::Severity::INFO;
}

std::string GET_THIS_PROGRAM_LOG_SOCKET_LOCATION()
{
    // Might as well cache it since it should probably never change
    static auto CACHED = fmt::format("{0}/{1}-{2}.socket", BASE_SOCKET_PATH, getpid(), __progname);
    return CACHED;
}

std::vector<xlog_socket_candidate> TRY_GET_PROGRAM_LOG_SOCKET(const std::string& program_name, int pid)
{
    std::vector<xlog_socket_candidate> candidates;

    std::error_code err;
    auto itr = std::filesystem::directory_iterator(BASE_SOCKET_PATH, std::filesystem::directory_options::skip_permission_denied, err);
    if(err)
    {
        XLOG_INTERNAL_FC(err, "{0}; Failed to get directory iterator");
        return candidates;
    }

    for(const auto& sockFile : itr)
    {
        bool is_socket = sockFile.is_socket(err);
        if(err)
        {
            XLOG_INTERNAL_FC(err, "{0}; Failed to check if file is socket");
        }

        if(!is_socket)
        {
            std::cout << "File is not socket" << std::endl;
            continue;
        }

        std::string match_name = sockFile.path().filename();

        std::smatch sock_match;
        if(std::regex_search(match_name, sock_match, SOCKET_REGEX))
        {
            if(sock_match.size() != 3)
            {
                XLOG_INTERNAL_FC(err, "{0}; sock_match size != 3 ({1})", sock_match.size());
                continue;
            }

            if(sock_match[2].compare(program_name) == 0)
            {
                std::string captured_pid(sock_match[1]);
                auto cand = xlog_socket_candidate
                {
                    .path = sockFile.path(),
                    .program_name = program_name,
                    .pid = std::atoi(captured_pid.c_str())
                };

                if(pid < 0)
                {
                    candidates.emplace_back(std::move(cand));
                }
                else
                {
                    if(cand.pid == pid)
                    {
                        candidates.emplace_back(std::move(cand));
                    }
                }
            }
        }
    }

    return candidates;
}

bool TRY_SETUP_THIS_PROGRAM_SOCKET()
{
    const auto LOG_SOCKET = GET_THIS_PROGRAM_LOG_SOCKET_LOCATION();

    std::error_code err;
    if(!std::filesystem::exists(BASE_SOCKET_PATH, err))
    {
        std::filesystem::create_directories(BASE_SOCKET_PATH, err);
        if(err)
        {
            XLOG_INTERNAL_FC(err, "{0}; Failed to create directory");
            return false;
        }
    }
    else
    {
        if(!std::filesystem::is_directory(BASE_SOCKET_PATH, err))
        {
            XLOG_INTERNAL_FC(err, "{0}; Base socket path is not a directory");
            return false;
        }
    }

    auto socket_exists = std::filesystem::exists(LOG_SOCKET, err);
    if(err)
    {
        XLOG_INTERNAL_FC(err, "{0}; Failed to check if socket exists");
        return false;
    }

    if(socket_exists)
    {
        if(::unlink(LOG_SOCKET.c_str()) != 0)
        {
            XLOG_INTERNAL_FE("{0}; Failed to unlink socket");
            return false;
        }
    }

    return true;
}

void TRY_SHUTDOWN_THIS_PROGRAM_SOCKET()
{
    const auto LOG_SOCKET = GET_THIS_PROGRAM_LOG_SOCKET_LOCATION();
    ::unlink(LOG_SOCKET.c_str());
}
