# XLog
I wrote xlog as a simple and extendable logging mechanism for a long forgotten project, but since then I have been copying the source files to other projects, making small and incremental improvements, tweaks, and so on. Eventually I decided it'd be easier to do a big cleanup and move it into a nice git repository so I can easily use it anywhere!

# Requirements
- C++ 17 at minimum
 - C++ 20 enables using std::string, std::string_view, and char* as keys to query the logger map
 - Source location is enabled if present (C++ >=20)
- libfmt (downloaded via FetchContent)
- Boost Log (not sure how early a version you can get away with, minimum I've tried is 1.72)

# Notes
- Only logs to stderr
- Can be used in a multi-threaded environment
- Has its own exception that writes to the log (and includes source location)
 - Not tested in an exception-less environment

# TODO
- Add checks for exceptions and handle FATAL() macros differently if they are disabled
- Expand CODE_SEV(code) macros to give more information
- Expand ERRNO_SEV() macros to give more information
- Add new places to send the log to (i.e. files)
- Test in more environments
