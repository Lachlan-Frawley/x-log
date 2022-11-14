cmake_minimum_required(VERSION 3.16)
project(xlog VERSION 0.4.0 LANGUAGES CXX)

option(ENABLE_EXTERNAL_LOG_CONTROL "Allow external programs to manage runtime logging by connecting to a Unix socket" OFF)
option(USE_SOURCE_LOCATION "If available, include source location in some logging commands" ON)
option(USE_SYSLOG_LOG "Enable logging using syslog" OFF)
option(USE_JOURNAL_LOG "Enable logging using journald" OFF)

option(BUILD_TESTS "Build tests and test program" OFF)

set(SET_OPTS)

if(ENABLE_EXTERNAL_LOG_CONTROL)
	add_compile_definitions(XLOG_ENABLE_EXTERNAL_LOG_CONTROL)
	set(SET_OPTS
"${SET_OPTS}
set(XLOG_ENABLE_EXTERNAL_LOG_CONTROL ON)")
endif(ENABLE_EXTERNAL_LOG_CONTROL)

if(USE_SOURCE_LOCATION)
	add_compile_definitions(XLOG_USE_SOURCE_LOCATION_IF_AVAILABLE)
	set(SET_OPTS
"${SET_OPTS}
set(XLOG_USE_SOURCE_LOCATION_IF_AVAILABLE ON)")
endif(USE_SOURCE_LOCATION)

if(USE_SYSLOG_LOG)
	add_compile_definitions(XLOG_USE_SYSLOG_LOG)
	set(SET_OPTS
"${SET_OPTS}
set(XLOG_USE_SYSLOG_LOG ON)")
endif(USE_SYSLOG_LOG)

if(USE_JOURNAL_LOG)
	add_compile_definitions(XLOG_USE_JOURNAL_LOG)
	set(SET_OPTS
"${SET_OPTS}
set(XLOG_USE_JOURNAL_LOG ON)")
endif(USE_JOURNAL_LOG)

set(CMAKE_CXX_STANDARD 17)
if("cxx_std_20" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
	message("C++ 20 support detected")
	set(CMAKE_CXX_STANDARD 20)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(fmt REQUIRED)
find_package(Boost REQUIRED COMPONENTS log)
find_package(Threads REQUIRED)

set(LIBRARIES
	Threads::Threads
	Boost::log
	fmt::fmt
)

set(LIB_SOURCE_FILES xlog.cpp)
set(TEST_PROG_SOURCE_FILES test_program.cpp)
set(GTEST_SOURCE_FILES xlog_gtest.cpp xlog_test_backend.cpp xlog_test_utils.cpp)
set(MACRO_GEN_SOURCE_FILES xlog_macro_gen.cpp)

set(EXPORT_HEADERS xlog.h)

if(USE_JOURNAL_LOG)
	set(LIB_SOURCE_FILES ${LIB_SOURCE_FILES} xlog_journal.cpp)
	set(LIBRARIES ${LIBRARIES} systemd)
endif(USE_JOURNAL_LOG)

if(BUILD_TESTS)
	find_package(GTest REQUIRED)
endif(BUILD_TESTS)

if(ENABLE_EXTERNAL_LOG_CONTROL)
	find_package(Protobuf REQUIRED)
	find_package(gRPC CONFIG REQUIRED)

	include(GRPC.cmake)
	file(GLOB ProtoFiles xlog.proto)
	PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})
	PROTOBUF_GENERATE_GRPC_CPP(ProtoGRPCSources ProtoGRPCHeaders ${ProtoFiles})

	set(LIB_SOURCE_FILES
		${LIB_SOURCE_FILES}
		xlog_grpc.cpp
		xlog_grpc_util.cpp
		${ProtoSources}
		${ProtoGRPCSources})

	set(TEST_PROG_SOURCE_FILES
		${TEST_PROG_SOURCE_FILES}
		${ProtoSources}
		${ProtoGRPCSources})

	set(EXPORT_HEADERS
		${EXPORT_HEADERS}
		xlog.proto)

	set(LIBRARIES
		${LIBRARIES}
		protobuf::libprotobuf
		gRPC::grpc
		gRPC::grpc++
		)
endif(ENABLE_EXTERNAL_LOG_CONTROL)

add_library(xlog-shared SHARED ${LIB_SOURCE_FILES})
target_link_libraries(xlog-shared PUBLIC ${LIBRARIES})

add_library(xlog STATIC ${LIB_SOURCE_FILES})
target_link_libraries(xlog PUBLIC ${LIBRARIES})

if(BUILD_TESTS)
	add_executable(xlog-test-prog ${TEST_PROG_SOURCE_FILES})
	target_link_libraries(xlog-test-prog PUBLIC xlog)

	add_library(xlog-test-lib STATIC ${LIB_SOURCE_FILES})
	target_link_libraries(xlog-test-lib PUBLIC ${LIBRARIES})
	target_compile_definitions(xlog-test-lib PRIVATE XLOG_USE_TEST_BACKENDS)

	add_executable(xlog-gtest ${GTEST_SOURCE_FILES})
	target_link_libraries(xlog-gtest PUBLIC GTest::GTest xlog-test-lib)

	add_executable(xlog-macro-gen ${MACRO_GEN_SOURCE_FILES})
endif(BUILD_TESTS)

if(ENABLE_EXTERNAL_LOG_CONTROL)
	target_include_directories(xlog-shared PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
	target_include_directories(xlog PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

	if(BUILD_TESTS)
		target_include_directories(xlog-test-prog PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
	endif(BUILD_TESTS)

	find_package(cli REQUIRED)
	find_package(CLI11 REQUIRED)
	add_executable(xlog-manager xlog_manager.cpp)
	target_link_libraries(xlog-manager PUBLIC xlog cli::cli CLI11::CLI11)
	target_include_directories(xlog-manager PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif(ENABLE_EXTERNAL_LOG_CONTROL)

set_target_properties(xlog-shared PROPERTIES VERSION ${CMAKE_PROJECT_VERSION} SOVERSION 1)

set(include_dest "include/xlog")
set(project_lib_dest "lib/cmake/xlog")
set(lib_dest "${project_lib_dest}")

install(TARGETS xlog EXPORT xlog DESTINATION lib)
install(TARGETS xlog-shared EXPORT xlog-shared DESTINATION lib)
if(ENABLE_EXTERNAL_LOG_CONTROL)
	install(TARGETS xlog-manager DESTINATION bin)
endif(ENABLE_EXTERNAL_LOG_CONTROL)
install(FILES ${EXPORT_HEADERS} DESTINATION "${include_dest}")
install(
        EXPORT xlog
        DESTINATION "${lib_dest}"
        NAMESPACE xlog::
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/xlog-config.cmake"
	INSTALL_DESTINATION "${project_lib_dest}"
)

file(READ "${CMAKE_CURRENT_BINARY_DIR}/xlog-config.cmake" CONFIG_CONTENTS)
string(REPLACE "__INSERT_SET_OPTIONS__" "${SET_OPTS}" CONFIG_CONTENTS "${CONFIG_CONTENTS}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/xlog-config.cmake" "${CONFIG_CONTENTS}")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/xlog-config.cmake DESTINATION "${project_lib_dest}")
