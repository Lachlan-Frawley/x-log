#include <iostream>

#include "xlog.h"

#include <array>
#include <string>

constexpr std::array LOG_LEVELS =
{
    "INFO",
    "DEBUG", "DEBUG2",
    "WARN", "WARN2",
    "ERROR", "ERROR2"
};

constexpr std::array SEV_LEVELS =
{
    "INFO",
    "DEBUG", "DEBUG2",
    "WARNING", "WARNING2",
    "ERROR", "ERROR2"
};

int main(int argc, char** argv)
{
    std::cout << "Hello World!" << std::endl;

    return 0;
}
