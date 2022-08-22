cmake_minimum_required(VERSION 3.16)
project(xlog VERSION 1.0.0 DESCRIPTION "eXtendable LOGging framework")

set(CMAKE_CXX_STANDARD 17)

include(GNUInstallDirs)

find_package(Boost REQUIRED COMPONENTS log)
find_package(Threads REQUIRED)

find_package(PkgConfig)
pkg_check_modules(FMT REQUIRED IMPORTED_TARGET fmt)

add_library(xlog xlog.cpp)
target_link_libraries(xlog Threads::Threads Boost::log PkgConfig::FMT)
configure_file(xlog.pc.in xlog.pc @ONLY)
set_target_properties(xlog PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER xlog.h)
install(TARGETS xlog
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_BINARY_DIR}/xlog.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

add_executable(example example.cpp)
target_link_libraries(example xlog)
