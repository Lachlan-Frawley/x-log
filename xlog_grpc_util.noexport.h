#include "xlog.h"
#include "xlog.grpc.pb.h"

constexpr char BASE_SOCKET_PATH[] = "/tmp/xlog";

struct xlog_socket_candidate
{
    std::string path;
    std::string program_name;
    int pid;
};

xlogProto::SeverityMessage make_severity_message(xlog::Severity severity);
xlog::Severity severity_from_message(const xlogProto::SeverityMessage& msg);

std::string GET_THIS_PROGRAM_LOG_SOCKET_LOCATION();
std::vector<xlog_socket_candidate> TRY_GET_PROGRAM_LOG_SOCKET(const std::string& program_name, int pid = -1);
bool TRY_SETUP_THIS_PROGRAM_SOCKET();
void TRY_SHUTDOWN_THIS_PROGRAM_SOCKET();
