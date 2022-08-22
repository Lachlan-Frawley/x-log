#include "xlog.h"

GET_LOGGER("example")

int main(int argc, char** argv)
{
    XLog::InitializeLogging();

    LOG_INFO() << "Info!";

    FATAL_FMT("Number of args = {0}", argc);

    return 0;
}
