#include "xlog.h"
#include "xlog.grpc.pb.h"

constexpr char BASE_SOCKET_PATH[] = "/tmp/xlog";

xlogProto::SeverityMessage make_severity_message(XLog::Severity severity);
XLog::Severity severity_from_message(const xlogProto::SeverityMessage& msg);

std::string GET_THIS_PROGRAM_LOG_SOCKET_LOCATION();
std::vector<std::string> TRY_GET_PROGRAM_LOG_SOCKET(const std::string& program_name, int pid = -1);
bool TRY_SETUP_THIS_PROGRAM_SOCKET();
void TRY_SHUTDOWN_THIS_PROGRAM_SOCKET();
