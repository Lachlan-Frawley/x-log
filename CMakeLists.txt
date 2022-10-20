cmake_minimum_required(VERSION 3.16)
project(xlog VERSION 0.1.0 LANGUAGES CXX)

option(ENABLE_EXTERNAL_LOG_CONTROL "Allow external programs to manage runtime logging by connecting to a Unix socket" ON)

set(CMAKE_CXX_STANDARD 17)

find_package(fmt REQUIRED)
find_package(Boost REQUIRED COMPONENTS log)
find_package(Threads REQUIRED)

set(LIBRARIES
	Threads::Threads
	Boost::log
	fmt::fmt
)

set(SOURCE_FILES xlog.cpp)

set(EXPORT_HEADERS xlog.h)

if(ENABLE_EXTERNAL_LOG_CONTROL)
	find_package(Protobuf REQUIRED)
	find_package(gRPC CONFIG REQUIRED)

	include(GRPC.cmake)
	file(GLOB ProtoFiles xlog.proto)
	PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})
	PROTOBUF_GENERATE_GRPC_CPP(ProtoGRPCSources ProtoGRPCHeaders ${ProtoFiles})

	set(SOURCE_FILES
		${SOURCE_FILES}
		${ProtoSources}
		${ProtoGRPCSources})

	set(EXPORT_HEADERS
		${EXPORT_HEADERS}
		xlog.proto
		${ProtoHeaders}
		${ProtoGRPCHeaders})

	set(LIBRARIES
		${LIBRARIES}
		protobuf::libprotobuf
		gRPC::grpc
		gRPC::grpc++
		)
endif(ENABLE_EXTERNAL_LOG_CONTROL)

add_library(xlog-shared SHARED ${SOURCE_FILES})
target_link_libraries(xlog-shared PUBLIC ${LIBRARIES})

add_library(xlog STATIC ${SOURCE_FILES})
target_link_libraries(xlog PUBLIC ${LIBRARIES})

if(ENABLE_EXTERNAL_LOG_CONTROL)
	target_include_directories(xlog-shared PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
	target_include_directories(xlog PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif(ENABLE_EXTERNAL_LOG_CONTROL)

set_target_properties(xlog-shared PROPERTIES VERSION ${CMAKE_PROJECT_VERSION} SOVERSION 1)

set(include_dest "include/xlog")
set(project_lib_dest "lib/cmake/xlog")
set(lib_dest "${project_lib_dest}")

install(TARGETS xlog EXPORT xlog DESTINATION lib)
install(TARGETS xlog-shared EXPORT xlog-shared DESTINATION lib)
install(FILES ${EXPORT_HEADERS} DESTINATION "${include_dest}")
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
