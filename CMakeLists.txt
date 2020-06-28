cmake_minimum_required(VERSION 3.10)

# set the project name
project(wrongthink VERSION 0.1)

# set project version configuration file
configure_file(include/WrongthinkConfig.h.in WrongthinkConfig.h)

 # specify the C++ standard
 set(CMAKE_CXX_STANDARD 17)
 set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)

# This branch assumes that gRPC and all its dependencies are already installed
# on this system, so they can be located by find_package().

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# Proto file
get_filename_component(wt_proto "protocol/proto/wrongthink.proto" ABSOLUTE)
get_filename_component(wt_proto_path "${wt_proto}" PATH)

# Generated sources
set(wt_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/wrongthink.pb.cc")
set(wt_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/wrongthink.pb.h")
set(wt_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/wrongthink.grpc.pb.cc")
set(wt_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/wrongthink.grpc.pb.h")
add_custom_command(
      OUTPUT "${wt_proto_srcs}" "${wt_proto_hdrs}" "${wt_grpc_srcs}" "${wt_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${wt_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${wt_proto}"
      DEPENDS "${wt_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_executable(wrongthink "wrongthink.cpp"
  ${wt_proto_srcs}
  ${wt_grpc_srcs})
target_link_libraries(wrongthink
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

add_executable(test_client "test_client.cpp"
  ${wt_proto_srcs}
  ${wt_grpc_srcs})
target_link_libraries(test_client
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})


# include directories
target_include_directories(wrongthink PUBLIC
                           include
                           ${CMAKE_CURRENT_BINARY_DIR}
                           )
# include directories
target_include_directories(test_client PUBLIC
                          include
                          ${CMAKE_CURRENT_BINARY_DIR}
                          )
