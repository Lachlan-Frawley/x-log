# XLog
I wrote xlog as a simple and extendable logging mechanism for a long forgotten project, but since then I have been copying the source files to other projects, making small and incremental improvements, tweaks, and so on. Eventually I decided it'd be easier to do a big cleanup and move it into a nice git repository so I can easily use it anywhere!

# Requirements
- C++ 17 at minimum
- C++ 20 enables using ```std::string```, ```std::string_view```, and ```char*``` as keys to query the logger map
- Source location is enabled if present and enabled (C++ >=20)
- Boost Log v2 (not sure how early a version you can get away with, minimum I've tried is 1.72)
- libfmt
- Protobuf & gRPC (if external log control is enabled)
- sigcpp (if enabled)

# Notes
- Logs to std::clog, syslog, or journal
- Can be used in a multi-threaded environment
- Has its own exception that writes to the log (and includes source location)
- Not tested in an exception-less environment

# CMake Default Options
- ```-DENABLE_EXTERNAL_LOG_CONTROL=OFF```, when set to ```ON```, enables external log management (requires gRPC, Protobuf)
- ```-DUSE_SOURCE_LOCATION=ON```, If C++20 support is available, this enables logging source location for some log lines
- ```-DUSE_SYSLOG_LOG=OFF```, Enable logging to syslog
- ```-DUSE_JOURNAL_LOG=OFF```, Enable logging to journald
- ```-DUSE_SIGCPP=OFF```, Use Sigcpp (another of my projects) for signal management to allow automatic closure of the external control socket

# Features I want to add
- Log to files
- Add instance loggers (i.e. named instances)
- Allow adding and removing log sinks via external management
- Add checks for exceptions and handle FATAL() macros differently if they are disabled
- Test in more environments
- Automated testing

## Syslog & Journal Logging
Both disabled by default, enable via:

```-DUSE_SYSLOG_LOG=ON```

and

```-DUSE_JOURNAL_LOG=ON```

## External Log Control
Sometimes we want to be able to control logging without having to restart the program, and since xlog allows runtime changing of the log levels, it seems reasonable to have some kind of method to connect to a program that is running and edit its logging configuration.

To accomplish this we create a Unix socket like so: ```/tmp/xlog/${PID}-${PROGNAME}.socket```, now an external program can connect and manipulate the programs log settings.

To facilitate this, a gRPC interface exists which defines how the external program must communicate with the xlog instance running inside the target. To install gRPC, follow the instructuctions here: https://grpc.io/docs/languages/cpp/quickstart

This is disabled by default and can be enabled like so via CMake:

```-DENABLE_EXTERNAL_LOG_CONTROL=ON```

## Sigcpp Integration
Sigcpp is another project I wrote to (ideally) make it easier to manage signals and allow multiple locations to add callbacks on signals (to allow cleanups and so on). Sigcpp can be used here to automatically close external log control sockets (if enabled)


Sigcpp is disabled by default and can be enabled like so via CMake:

```-DUSE_SIGCPP=ON```

# How to use
Pretty much all of the logging macros look for a special named variable which is filled in by the ```XLOG_GET_LOGGER(name)``` macro. This macro is meant to be used in a source file, but can be used in other places (i.e. functions), but this can lead to some interesting behaviour, so just put it in your source file for now (there are other mechanisms to log without using this macro).

## Severity Levels
There are a few severity levels and while they pretty much behave the way you'd expect, it's still worth going over.
```
INFO,
DEBUG,
DEBUG2,
WARNING,
WARNING2,
ERROR,
ERROR2,
FATAL,
INTERNAL    // Used by xlog as the internal log level (can never be disabled)
```
Log levels with a '2' in their name will not log source location (with the exception of INFO, which never logs source locations). The source location sent to the log is stripped down slightly, only showing the offending function, file (path stripped), and line number.

## Logging
```c++
XLOG_<LEVEL> // Works with stream operator '<<'
XLOG_<LEVEL>_<QUALIFIER> // Qualifiers modify the behaviour of the macro

// Qualifiers
XLOG_<LEVEL>_I(name) // Inplace logging, i.e. takes a name and can be used in headers

XLOG_<LEVEL>_F(fmat, ...) // Formatted logging, still technically a stream so you can use '<<' after

XLOG_<LEVEL>_C(err) // Error code logging, can take std::error_code, or boost::system::error_code

XLOG_<LEVEL>_E // ERRNO logging

// Qualifiers can be combined, for example:
XLOG_<LEVEL>_IFC(name, err, fmt, ...) // Inplace, formated, error code logging

// Precedence -> Inplace 'I' > Formatted 'F' > ERRNO 'E' & Error Code 'C'
// Obviously ERRNO and Error Code can't be combined

// <LEVEL>'s with a 2 in their name do not log source locations (if enabled)
LOG_WARN2() << "If source location is enabled, I write a warning without the source location in it!";

// Fatal exceptions are pretty similar, however they cannot use the stream modifier
XLOG_FATAL_<QUALIFIER>(*args*)

// The simplest being:
XLOG_FATAL(msg)

// Fatal exceptions have an extra qualifier: GLOBAL
// This logs to the named logger "Global"
XLOG_FATAL_G(msg)

// Qualifiers can still be combined, for example:
XLOG_FATAL_FGE(fmat, ...) // Formatted, Global, ERRNO logging

// Precedence -> Inplace 'I' > Formatted 'F' > Global 'G' > ERRNO 'E' & Error Code 'C'
// Inplace and Global are mutually exclusive
```
