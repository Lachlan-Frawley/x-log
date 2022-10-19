cmake_minimum_required(VERSION 3.16)
project(xlog VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(fmt REQUIRED)
find_package(Boost REQUIRED COMPONENTS log)
find_package(Threads REQUIRED)

set(LIBRARIES
	Threads::Threads
	Boost::log
	fmt::fmt
)

add_library(xlog-shared SHARED xlog.cpp)
target_link_libraries(xlog-shared PUBLIC ${LIBRARIES})

add_library(xlog STATIC xlog.cpp)
target_link_libraries(xlog PUBLIC ${LIBRARIES})

set(include_dest "include/xlog")
set(project_lib_dest "lib/cmake/xlog")
set(lib_dest "${project_lib_dest}/${CMAKE_BUILD_TYPE}")

install(TARGETS xlog EXPORT xlog DESTINATION lib)
install(TARGETS xlog-shared EXPORT xlog-shared DESTINATION lib)
install(FILES xlog.h DESTINATION "${include_dest}")
install(
        EXPORT xlog
        DESTINATION "${lib_dest}"
        NAMESPACE xlog::
)

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/xlog-config.cmake"
  INSTALL_DESTINATION "${project_lib_dest}"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/xlog-config.cmake DESTINATION "${project_lib_dest}")
