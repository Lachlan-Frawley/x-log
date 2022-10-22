#include "xlog_grpc_util.noexport.h"

class xlog_grpc_server final : public xlogProto::RuntimeLogManagement::Service
{
public:
    ::grpc::Status GetDefaultLogLevel(::grpc::ServerContext* context, const ::xlogProto::Void* request, ::xlogProto::SeverityMessage* response) override;
    ::grpc::Status SetDefaultLogLevel(::grpc::ServerContext* context, const ::xlogProto::SeverityMessage* request, ::xlogProto::Void* response) override;
    ::grpc::Status GetChannelLogLevel(::grpc::ServerContext* context, const ::xlogProto::LogChannel* request, ::xlogProto::SeverityMessage* response) override;
    ::grpc::Status SetChannelSeverity(::grpc::ServerContext* context, const ::xlogProto::SetChannelSeverityMessage* request, ::xlogProto::Void* response) override;
    ::grpc::Status GetAllLogLevels(::grpc::ServerContext* context, const ::xlogProto::Void* request, ::xlogProto::AllLogLevelsMessage* response) override;
    ::grpc::Status GetAllLogHandles(::grpc::ServerContext* context, const ::xlogProto::Void* request, ::xlogProto::AllLogHandlesMessage* response) override;
};
