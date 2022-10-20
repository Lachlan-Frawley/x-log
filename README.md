# XLog
I wrote xlog as a simple and extendable logging mechanism for a long forgotten project, but since then I have been copying the source files to other projects, making small and incremental improvements, tweaks, and so on. Eventually I decided it'd be easier to do a big cleanup and move it into a nice git repository so I can easily use it anywhere!

# Requirements
- C++ 17 at minimum
- C++ 20 enables using std::string, std::string_view, and char* as keys to query the logger map
- Source location is enabled if present (C++ >=20)
- Boost Log (not sure how early a version you can get away with, minimum I've tried is 1.72)
- libfmt

# Notes
- Only logs to stderr
- Can be used in a multi-threaded environment
- Has its own exception that writes to the log (and includes source location)
- Not tested in an exception-less environment

# TODO
- Log to syslog or journal
- Log to files
- Add fatal CODE and ERRNO macros
- Add log control socket & external log control program
- Add instance loggers (i.e. named instances)
- Add checks for exceptions and handle FATAL() macros differently if they are disabled
- Expand CODE_SEV(code) macros to give more information
- Expand ERRNO_SEV() macros to give more information
- Test in more environments

# How to use
Pretty much all of the logging macros look for a special named variable which is filled in by the ```GET_LOGGER(name)``` macro. This macro is meant to be used in a source file, but can be used in other places (i.e. functions), but this can lead to some interesting behaviour, so just put it in your source file for now (there are other mechanisms to log without using this macro).

## Severity Levels
There are a few severity levels and while they pretty much behave the way you'd expect, it's still worth going over.
```
INFO,
DEBUG,
DEBUG2,
WARNING,
WARNING2, // For warnings where we don't really need to know the source location (if enabled)
ERROR,
ERROR2,
FATAL
```
Essentially, if source location exists, then logs with the severity of DEBUG or higher will have their source location in the log line (excluding WARNING2/WARN2). The information is a bit stripped down though, only showing the file name (not path), and the offending line. Of course, feel free to change it to do what you need to.

Currently, all severity levels are always enabled. I am looking to change this, and because all of the following macros eventually come back to the ```CUSTOM_LOG_SEV()``` macro from the Boost log library, only enabled severity levels will execute the code associated with log statements.

## Normal Logging
```
LOG_INFO()
LOG_DEBUG()
LOG_DEBUG2()
LOG_WARN()
LOG_WARN2()
LOG_ERROR()
LOG_ERROR2()
```
These are macros are used to write to the log using the C++ stream operator:
```
LOG_WARN2() << "If source location is enabled, I write a warning without the source location it it!" << std::endl;
```

## Error Codes
If you are an avid user of Boost, or happen to use ```std::error_code```, then you might want to log it without having to expand out the data wrapped in it every single time. 
```
CODE_INFO(errc)
CODE_DEBUG(errc)
CODE_DEBUG2(errc)
CODE_WARN(errc)
CODE_WARN2(errc)
CODE_ERROR(errc)
CODE_ERROR2(errc)
```
These macros expand to:
```
CODE_<SERVERITY>(errc) LOG_<SEVERITY>() << ERRC_STREAM(errc)
```
Where ```ERRC_STREAM(errc)``` is another macro that can be modified however you like to print out the information wrapped up in the object. By default the macro only logs the message of the error code.

## ERRNO Logging
Sometimes we have to interface with linux kernel code which uses ```errno``` to express errors. Having to extract the string value for a given error number and log it would be a pain, so there are some macros to make life a bit easier.
```
ERRNO_INFO()
ERRNO_DEBUG()
ERRNO_DEBUG2()
ERRNO_WARN()
ERRNO_WARN2()
ERRNO_ERROR()
ERRNO_ERROR2()
```
These macros expand similar to the error code macros:
```
ERRNO_<SEVERITY>() LOG_<SEVERITY>() << ERRNO_STREAM
```
Where ```ERRNO_STREAM``` is another customiseable macro. Currently, it calls a function name ```get_errno_string()``` which returns the error string through ```strerror_r``` (which is thread safe), but of course, this could be changed if you want to.

## Fatal Logging
Despite there being a fatal severity, there is no ```LOG_FATAL()```, why is that? Well because the fatal severity is expressed as an exception (currently!!! subject to change maybe). There are a few macros used for this:
```
FATAL(msg)
FATAL_FMT(fmt, ...)
CODE_FATAL(errc)
ERRNO_FATAL()
```
As well as some utilities that log fatal to the global logger:
```
FATAL_GLOBAL(msg)
FATAL_GLOBAL_FMT(fmt, ...)
CODE_FATAL_GLOBAL(errc)
ERRNO_FATAL_GLOBAL()
```
These all essentially throw an exception, and of course the fatal versions of the previous log types use customiseable macros to express their messages; these are however, not the *same* customiseable macros:
```
// For error codes (default)
ERRC_MSG(errc) errc.message()

// For ERRNO (default)
ERRNO_MESSAGE() get_errno_string()
```
Because the argument to construct a fatal exception must be a string, we can't use the exact same macros, but these are both pretty similar to their stream-based counterparts.

## Inplace/Named Logging
Sometimes we might want to log in a header file (where using the ```GET_LOGGER``` macro would be disasterous!), or perhaps we want to use a different logger even though we've already defined one in our source file? Well then this is the solution to that problem:
```
// Normal Logging
LOG_INFO_INPLACE(name)
LOG_DEBUG_INPLACE(name)
LOG_DEBUG2_INPLACE(name)
LOG_WARN_INPLACE(name)
LOG_WARN2_INPLACE(name)
LOG_ERROR_INPLACE(name)
LOG_ERROR2_INPLACE(name)

// Error Codes
CODE_INFO_INPLACE(name, errc)
CODE_DEBUG_INPLACE(name, errc)
CODE_DEBUG2_INPLACE(name, errc)
CODE_WARN_INPLACE(name, errc)
CODE_WARN2_INPLACE(name, errc)
CODE_ERROR_INPLACE(name, errc)
CODE_ERROR2_INPLACE(name, errc)

// ERRNO Logging
ERRNO_INFO_INPLACE(name)
ERRNO_DEBUG_INPLACE(name)
ERRNO_DEBUG2_INPLACE(name)
ERRNO_WARN_INPLACE(name)
ERRNO_WARN2_INPLACE(name)
ERRNO_ERROR_INPLACE(name)
ERRNO_ERROR2_INPLACE(name)

// Fatal Logging
FATAL_NAMED(name, msg)
FATAL_NAMED_FMT(name, fmt, ...)
CODE_FATAL_NAMED(name, errc)
ERRNO_FATAL_NAMED(name)
```

## External Log Control (TODO)
Sometimes we want to be able to control logging without having to restart the program, and since xlog allows runtime changing of the log levels, it seems reasonable to have some kind of method to connect to a program that is running and edit its logging configuration.

To accomplish this we create a Unix socket like so: ```/tmp/xlog-${PROGNAME}-${PID}.socket```, now an external program can connect and manipulate the programs log settings.

To facilitate this, a gRPC interface exists which defines how the external program must communicate with the xlog instance running inside the target.

Of course I can see now that some people might not find value in this, so it can be disabled via

```-DENABLE_EXTERNAL_LOG_CONTROL=off```

When building the cmake project.

TODO :)
