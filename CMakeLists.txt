cmake_minimum_required(VERSION 3.16)
project(xlog VERSION 1.0.0 DESCRIPTION "eXtendable LOGging framework")

set(CMAKE_CXX_STANDARD 17)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED COMPONENTS log log_setup thread date_time)
find_package(fmt REQUIRED)

add_library(xlog STATIC xlog.cpp)
target_include_directories(xlog PRIVATE .)
target_link_libraries(xlog libboost_log_setup.a libboost_log.a libboost_date_time.a Boost::thread fmt::fmt-header-only)
configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/xlog-config.cmake.in
    ${PROJECT_BINARY_DIR}/xlog-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xlog)
set_target_properties(xlog PROPERTIES
    PUBLIC_HEADER xlog.h)
install(TARGETS xlog EXPORT xlog-targets
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${PROJECT_BINARY_DIR}/xlog-config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xlog)
export(TARGETS xlog NAMESPACE xlog::
    FILE ${PROJECT_BINARY_DIR}/xlog-targets.cmake)
install(EXPORT xlog-targets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xlog
    NAMESPACE xlog::)

add_executable(example example.cpp)
target_link_libraries(example xlog)
