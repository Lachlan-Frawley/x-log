#include "xlog_grpc.noexport.h"

::grpc::Status xlog_grpc_server::GetDefaultLogLevel(::grpc::ServerContext* context, const ::xlogProto::Void* request, ::xlogProto::SeverityMessage* response)
{
    response->CopyFrom(make_severity_message(xlog::GetGlobalLoggingLevel()));
    return ::grpc::Status::OK;
}

::grpc::Status xlog_grpc_server::SetDefaultLogLevel(::grpc::ServerContext* context, const ::xlogProto::SeverityMessage* request, ::xlogProto::Void* response)
{
    if(request->value() == xlogProto::Severity::SEV_UNKNOWN)
    {
        return { ::grpc::StatusCode::INVALID_ARGUMENT, "Severity is set as unknown, please use a known severity" };
    }

    xlog::SetGlobalLoggingLevel(severity_from_message(*request));

    return ::grpc::Status::OK;
}

::grpc::Status xlog_grpc_server::GetChannelLogLevel(::grpc::ServerContext* context, const ::xlogProto::LogChannel* request, ::xlogProto::SeverityMessage* response)
{
    auto xlog_sev = xlog::GetLoggingLevel(request->channel());
    response->CopyFrom(make_severity_message(xlog_sev));
    return ::grpc::Status::OK;
}

::grpc::Status xlog_grpc_server::SetChannelSeverity(::grpc::ServerContext* context, const ::xlogProto::SetChannelSeverityMessage* request, ::xlogProto::Void* response)
{
    if(request->severity().value() == xlogProto::Severity::SEV_UNKNOWN)
    {
        return { ::grpc::StatusCode::INVALID_ARGUMENT, "Severity is set as unknown, please use a known severity" };
    }

    auto xlog_sev = severity_from_message(request->severity());
    auto status = xlog::SetLoggingLevel(xlog_sev, request->channel());
    if(!status)
    {
        return { ::grpc::StatusCode::INVALID_ARGUMENT, "Failed to set severity for given channel" };
    }
    else
    {
        return ::grpc::Status::OK;
    }
}

::grpc::Status xlog_grpc_server::GetAllLogLevels(::grpc::ServerContext* context, const ::xlogProto::Void* request, ::xlogProto::AllLogLevelsMessage* response)
{
    auto all_levels = xlog::GetAllLoggingLevels();
    for(const auto& [key, value] : all_levels)
    {
        response->mutable_values()->insert({key, make_severity_message(value)});
    }

    return ::grpc::Status::OK;
}

::grpc::Status xlog_grpc_server::GetAllLogHandles(::grpc::ServerContext* context, const ::xlogProto::Void* request, ::xlogProto::AllLogHandlesMessage* response)
{
    auto all_handles = xlog::GetAllLogHandles();
    for(const auto& handle : all_handles)
    {
        response->add_values(handle);
    }

    return ::grpc::Status::OK;
}
